#!/usr/bin/env python3
"""
src/main.py  -  Optical MIDI Controller (MediaPipe Tasks API, mediapipe >= 0.10)

Controls (window must be focused):
  Q        quit
  C        calibration mode - click 4 corners TL->TR->BR->BL
  R        reset calibration
  + / =    raise Z-press threshold
  -        lower Z-press threshold
"""

from __future__ import annotations

import json
import os
import sys
import time
from typing import Dict, List, Set, Tuple

import cv2
import mediapipe as mp
from mediapipe.tasks import python as mp_tasks
from mediapipe.tasks.python import vision as mp_vision
import numpy as np

# Make sibling packages importable when run as  python src/main.py
sys.path.insert(0, os.path.dirname(__file__))

from core.spatial import SpatialEngine   # noqa: E402
from midi.mpe_engine import MPEEngine    # noqa: E402
from utils.filters import OneEuroFilter  # noqa: E402

# ---------------------------------------------------------------------------
# MediaPipe landmark indices
# ---------------------------------------------------------------------------
FINGER_TIPS: Dict[str, int] = {"index": 8, "middle": 12, "ring": 16, "pinky": 20}
FINGER_MCP:  Dict[str, int] = {"index": 5, "middle":  9, "ring": 13, "pinky": 17}

# Hand skeleton connections for OpenCV drawing
HAND_CONNECTIONS: List[Tuple[int, int]] = [
    (0, 1), (1, 2), (2, 3), (3, 4),
    (0, 5), (5, 6), (6, 7), (7, 8),
    (5, 9), (9, 10), (10, 11), (11, 12),
    (9, 13), (13, 14), (14, 15), (15, 16),
    (13, 17), (17, 18), (18, 19), (19, 20),
    (0, 17),
]

BLACK_KEY_CLASSES: Set[int] = {1, 3, 6, 8, 10}
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

# Y-zone thresholds for press detection (normalised 0-1, 0=top of frame)
# Must match ky=0.76 used in _draw_keyboard.
KEY_ZONE_TOP:     float = 0.76   # fingertip enters zone  → note on
KEY_ZONE_RELEASE: float = 0.73   # fingertip leaves zone  → note off (3% hysteresis)


# ===========================================================================
class OpticalController:
# ===========================================================================

    def __init__(self, config_path: str | None = None) -> None:
        # ── config ──────────────────────────────────────────────────────────
        if config_path is None:
            base = os.path.dirname(os.path.dirname(__file__))
            config_path = os.path.join(base, "config", "default_mapping.json")
        with open(config_path) as fh:
            self.config = json.load(fh)

        self.note_start:       int   = self.config["keyboard"]["note_start"]
        self.note_end:         int   = self.config["keyboard"]["note_end"]
        self.num_notes:        int   = self.note_end - self.note_start
        self.press_z_thresh:   float = self.config["keyboard"]["press_z_threshold"]
        self.release_z_thresh: float = self.config["keyboard"]["release_z_threshold"]

        # ── core engines ────────────────────────────────────────────────────
        self.spatial = SpatialEngine()
        self.mpe     = MPEEngine()

        # ── MediaPipe HandLandmarker (Tasks API, VIDEO mode) ─────────────────
        base_dir   = os.path.dirname(os.path.dirname(__file__))
        model_path = os.path.join(base_dir, "models", "hand_landmarker.task")
        if not os.path.exists(model_path):
            raise FileNotFoundError(
                f"Model not found: {model_path}\n"
                "Download with:\n"
                "  curl -L -o models/hand_landmarker.task \\\n"
                "    https://storage.googleapis.com/mediapipe-models/"
                "hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
            )
        options = mp_vision.HandLandmarkerOptions(
            base_options=mp_tasks.BaseOptions(model_asset_path=model_path),
            running_mode=mp_vision.RunningMode.VIDEO,
            num_hands=2,
            min_hand_detection_confidence=0.7,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5,
        )
        self.detector = mp_vision.HandLandmarker.create_from_options(options)
        self._t0_ms   = int(time.time() * 1000)

        # ── One-Euro filters (x and y per finger per hand) ───────────────────
        beta = self.config["system"]["smoothing_beta"]
        freq = float(self.config["system"]["target_fps"])
        self.filters_x: Dict[Tuple, OneEuroFilter] = {}
        self.filters_y: Dict[Tuple, OneEuroFilter] = {}
        for hi in range(2):
            for tip_id in FINGER_TIPS.values():
                k = (hi, tip_id)
                self.filters_x[k] = OneEuroFilter(freq=freq, min_cutoff=1.0, beta=beta)
                self.filters_y[k] = OneEuroFilter(freq=freq, min_cutoff=1.0, beta=beta)

        # ── state ────────────────────────────────────────────────────────────
        self.active_fingers: Dict[Tuple, int] = {}
        self.calibrating:        bool = False
        self.calibration_points: list = []
        self.frame_w: int = 640
        self.frame_h: int = 480

    # ==========================================================================
    # Helpers
    # ==========================================================================

    def _note_from_x(self, x: float) -> int:
        return self.note_start + int(max(0.0, min(0.9999, x)) * self.num_notes)

    def _velocity_from_z_delta(self, z_delta: float) -> int:
        return int(min(127, max(1, (z_delta - self.press_z_thresh) * 1200 + 60)))

    def _ts(self) -> int:
        """Milliseconds since controller start (required by detect_for_video)."""
        return int(time.time() * 1000) - self._t0_ms

    # ==========================================================================
    # Drawing
    # ==========================================================================

    def _draw_hand(self, frame: np.ndarray, landmarks: list) -> None:
        h, w = frame.shape[:2]
        for a, b in HAND_CONNECTIONS:
            p1 = (int(landmarks[a].x * w), int(landmarks[a].y * h))
            p2 = (int(landmarks[b].x * w), int(landmarks[b].y * h))
            cv2.line(frame, p1, p2, (0, 140, 200), 2)
        for lm in landmarks:
            cv2.circle(frame, (int(lm.x * w), int(lm.y * h)), 4, (0, 210, 255), -1)

    def _draw_keyboard(self, frame: np.ndarray, active_notes: Set[int]) -> None:
        h, w   = frame.shape[:2]
        ky, kh = int(h * 0.76), int(h * 0.20)
        overlay = frame.copy()
        for i in range(self.num_notes):
            x0   = int(w * i       / self.num_notes)
            x1   = int(w * (i + 1) / self.num_notes)
            note = self.note_start + i
            black  = (note % 12) in BLACK_KEY_CLASSES
            active = note in active_notes
            if black:
                color = (0, 230, 80) if active else (35, 35, 35)
                cv2.rectangle(overlay, (x0, ky), (x1 - 1, ky + int(kh * 0.62)), color, -1)
                cv2.rectangle(overlay, (x0, ky), (x1 - 1, ky + int(kh * 0.62)), (70, 70, 70), 1)
            else:
                color = (0, 255, 120) if active else (215, 215, 215)
                cv2.rectangle(overlay, (x0, ky), (x1 - 1, ky + kh), color, -1)
                cv2.rectangle(overlay, (x0, ky), (x1 - 1, ky + kh), (70, 70, 70), 1)
        cv2.addWeighted(overlay, 0.75, frame, 0.25, 0, frame)
        for i in range(self.num_notes):
            note = self.note_start + i
            if (note % 12) not in BLACK_KEY_CLASSES:
                xm   = int(w * (i + 0.5) / self.num_notes)
                name = NOTE_NAMES[note % 12] + str(note // 12 - 1)
                cv2.putText(frame, name, (xm - 9, ky + kh - 5),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.28, (40, 40, 40), 1)

    def _draw_hud(self, frame: np.ndarray, active_notes: Set[int]) -> None:
        h, w = frame.shape[:2]
        cv2.rectangle(frame, (0, 0), (w, 48), (18, 18, 18), -1)
        cal  = "YES" if self.spatial.homography_matrix is not None else "NO"
        text = (f"Notes:{len(active_notes)}  Cal:{cal}  Z:{self.press_z_thresh:.3f}"
                "  [Q]Quit [C]Cal [R]Reset [+/-]Z")
        cv2.putText(frame, text, (8, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.48, (220, 220, 0), 1)
        if self.calibrating:
            msg = f"  Click corner {len(self.calibration_points)+1}/4  (TL->TR->BR->BL)"
            cv2.putText(frame, msg, (4, h - 12), cv2.FONT_HERSHEY_SIMPLEX, 0.58, (0, 140, 255), 2)
            for pt in self.calibration_points:
                cv2.circle(frame, pt, 8, (0, 200, 255), -1)
                cv2.circle(frame, pt, 8, (255, 255, 255), 1)

    # ==========================================================================
    # MIDI logic
    # ==========================================================================

    def _process_hand(self, landmarks: list, hand_idx: int, t: float) -> Set[int]:
        """Process one hand's landmarks; return set of currently active notes."""
        active: Set[int] = set()
        for name, tip_id in FINGER_TIPS.items():
            k   = (hand_idx, tip_id)
            tip = landmarks[tip_id]

            fx = self.filters_x[k](tip.x, t)
            fy = self.filters_y[k](tip.y, t)
            px, py = self.spatial.project_point(fx, fy)

            # Press = fingertip has entered the keyboard strip at the bottom
            pressing   = fy >= KEY_ZONE_TOP
            was_active = k in self.active_fingers

            if pressing:
                note = self._note_from_x(px)
                active.add(note)

                if not was_active:
                    # new key press
                    self.mpe.note_on(k, note, velocity=80)
                    self.active_fingers[k] = note
                elif self.active_fingers[k] != note:
                    # finger moved to a different key
                    self.mpe.note_off(k, self.active_fingers[k])
                    self.mpe.note_on(k, note, velocity=80)
                    self.active_fingers[k] = note
                # (pitch-bend / expression disabled)

            elif was_active and fy < KEY_ZONE_RELEASE:
                # fingertip left the zone (hysteresis prevents chattering)
                self.mpe.note_off(k, self.active_fingers.pop(k))

        return active

    # ==========================================================================
    # Mouse callback (calibration)
    # ==========================================================================

    def _on_mouse(self, event: int, x: int, y: int, flags: int, param) -> None:
        if event == cv2.EVENT_LBUTTONDOWN and self.calibrating:
            self.calibration_points.append((x, y))
            print(f"  Point {len(self.calibration_points)}: ({x}, {y})")
            if len(self.calibration_points) == 4:
                self.spatial.calibrate(self.calibration_points, self.frame_w, self.frame_h)
                self.calibrating = False
                print("[Calibration] Complete!\n")

    # ==========================================================================
    # Main loop
    # ==========================================================================

    def run(self) -> None:
        cam_idx = self.config["system"]["camera_index"]
        cap     = cv2.VideoCapture(cam_idx)
        cap.set(cv2.CAP_PROP_FPS, self.config["system"]["target_fps"])

        WIN = "Optical MIDI Controller"
        cv2.namedWindow(WIN)
        cv2.setMouseCallback(WIN, self._on_mouse)

        print("\n+--------------------------------------+")
        print("|  Optical MIDI Controller  (ready)   |")
        print("+--------------------------------------+")
        print("Virtual MIDI port 'Optical MIDI Out' is open.")
        print("Open your DAW and select it as a MIDI input.\n")
        print("Controls:  Q=Quit  C=Calibrate  R=Reset  +/-=Z-threshold\n")

        try:
            while cap.isOpened():
                ret, frame = cap.read()
                if not ret:
                    print("[WARNING] Camera read failed. Exiting.")
                    break

                frame = cv2.flip(frame, 1)   # mirror = natural interaction
                self.frame_h, self.frame_w = frame.shape[:2]
                t = time.time()

                # MediaPipe inference
                rgb      = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
                result   = self.detector.detect_for_video(mp_image, self._ts())

                all_active: Set[int] = set()
                num_hands = 0

                if result.hand_landmarks:
                    num_hands = len(result.hand_landmarks)
                    for hi, landmarks in enumerate(result.hand_landmarks):
                        self._draw_hand(frame, landmarks)
                        all_active |= self._process_hand(landmarks, hi, t)

                # Release notes for any hand that has disappeared
                stale = [fk for fk in self.active_fingers if fk[0] >= num_hands]
                for fk in stale:
                    self.mpe.note_off(fk, self.active_fingers.pop(fk))

                self._draw_keyboard(frame, all_active)
                self._draw_hud(frame, all_active)
                cv2.imshow(WIN, frame)

                key = cv2.waitKey(1) & 0xFF
                if key == ord("q"):
                    break
                elif key == ord("c"):
                    self.calibrating        = True
                    self.calibration_points = []
                    print("[Calibration] Click 4 corners: TL -> TR -> BR -> BL")
                elif key == ord("r"):
                    self.spatial.reset()
                    print("[Calibration] Reset.")
                elif key in (ord("+"), ord("=")):
                    self.press_z_thresh   = round(min(0.15, self.press_z_thresh + 0.005), 3)
                    self.release_z_thresh = round(self.press_z_thresh * 0.5, 3)
                    print(f"[Z] press={self.press_z_thresh:.3f}  release={self.release_z_thresh:.3f}")
                elif key == ord("-"):
                    self.press_z_thresh   = round(max(0.01, self.press_z_thresh - 0.005), 3)
                    self.release_z_thresh = round(self.press_z_thresh * 0.5, 3)
                    print(f"[Z] press={self.press_z_thresh:.3f}  release={self.release_z_thresh:.3f}")
        finally:
            print("\nShutting down - sending MIDI panic ...")
            self.mpe.all_notes_off()
            self.detector.close()
            cap.release()
            cv2.destroyAllWindows()
            print("Done.")


if __name__ == "__main__":
    OpticalController().run()
