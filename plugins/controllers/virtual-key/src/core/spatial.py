"""
src/core/spatial.py

Computes a perspective homography that maps four user-selected corner
points (in camera/pixel space) to the unit square [0,1]×[0,1].

This lets you aim the camera at any angle and still receive perfectly
linear MIDI values across your playing surface.

Usage
-----
engine = SpatialEngine()

# During calibration the user clicks 4 corners of the playing surface:
engine.calibrate(
    [(x0,y0), (x1,y1), (x2,y2), (x3,y3)],  # pixel coords, TL→TR→BR→BL
    frame_w=640, frame_h=480
)

# For every landmark:
px, py = engine.project_point(landmark.x, landmark.y)   # both in [0,1]
"""

from __future__ import annotations

from typing import List, Optional, Tuple

import cv2
import numpy as np


class SpatialEngine:
    def __init__(self) -> None:
        self.homography_matrix: Optional[np.ndarray] = None

    # ------------------------------------------------------------------
    # Calibration
    # ------------------------------------------------------------------

    def calibrate(
        self,
        src_points: List[Tuple[int, int]],
        frame_w: int = 640,
        frame_h: int = 480,
    ) -> None:
        """
        Compute the homography from 4 corner pixel clicks to the unit square.

        Args:
            src_points : list of 4 (x_pixel, y_pixel) in order
                         Top-Left → Top-Right → Bottom-Right → Bottom-Left
            frame_w    : pixel width of the video frame
            frame_h    : pixel height of the video frame
        """
        if len(src_points) != 4:
            raise ValueError("Exactly 4 source points are required for calibration.")

        # Normalise pixel coords to [0,1] so the homography works on the
        # same normalised space as MediaPipe landmarks.
        src_norm = np.array(
            [[x / frame_w, y / frame_h] for x, y in src_points],
            dtype="float32",
        )
        dst_norm = np.array(
            [[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 1.0]],
            dtype="float32",
        )
        self.homography_matrix, _ = cv2.findHomography(src_norm, dst_norm)

    def reset(self) -> None:
        """Clear calibration (identity pass-through)."""
        self.homography_matrix = None

    # ------------------------------------------------------------------
    # Projection
    # ------------------------------------------------------------------

    def project_point(self, x: float, y: float) -> Tuple[float, float]:
        """
        Project a normalised (0-1) coordinate pair through the homography.

        If not yet calibrated the input is returned unchanged.

        Returns:
            (px, py) both clamped to [0, 1].
        """
        if self.homography_matrix is None:
            return float(x), float(y)

        point = np.array([[[x, y]]], dtype="float32")
        transformed = cv2.perspectiveTransform(point, self.homography_matrix)
        px = float(transformed[0][0][0])
        py = float(transformed[0][0][1])

        # Clamp output to valid range (rounding errors near edges)
        px = max(0.0, min(1.0, px))
        py = max(0.0, min(1.0, py))
        return px, py
