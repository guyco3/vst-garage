"""
src/midi/mpe_engine.py

MPE (MIDI Polyphonic Expression) channel manager.

MPE spec summary
----------------
  Channel 1  (index 0) : Master channel – global pitch-bend range etc.
  Channels 2-16 (index 1-15) : Per-note channels – one per active finger.

Each time a finger presses a key it is assigned the first free note-channel
from the pool.  When the finger lifts the channel is returned to the pool.
This gives every simultaneous note its own independent:
  • Pitch bend  (X-axis slide / vibrato)
  • CC 74       (Y-axis timbre / brightness)
  • Velocity    (press depth / speed)
"""

from __future__ import annotations

from typing import Dict, List, Tuple

import rtmidi
from rtmidi.midiconstants import CONTROL_CHANGE, NOTE_OFF, NOTE_ON, PITCH_BEND

# MPE-defined CC numbers
CC_TIMBRE = 74   # Slide / Brightness  (Y-axis)
CC_ALL_NOTES_OFF = 123


class MPEEngine:
    """
    Thread-safe MPE output engine.

    Opens a virtual MIDI port named ``port_name``.  Connect to it from
    your DAW (Ableton Live → MIDI preferences, Logic → MIDI environment).
    """

    def __init__(self, port_name: str = "Optical MIDI Out") -> None:
        self.midi_out = rtmidi.MidiOut()

        available = self.midi_out.get_ports()
        print(f"[MPE] System MIDI ports: {available if available else '(none)'}")

        self.midi_out.open_virtual_port(port_name)
        print(f"[MPE] Virtual port '{port_name}' opened.")

        # finger_key → (channel_index, note_number)
        # channel_index is 0-based: index 0 = MIDI ch 1, index 1 = MIDI ch 2, …
        self.active_notes: Dict[object, Tuple[int, int]] = {}

        # Free MPE note-channels: MIDI channels 2-16 → indices 1-15
        self.available_channels: List[int] = list(range(1, 16))

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def note_on(
        self,
        finger_id: object,
        note_num: int,
        velocity: int = 64,
    ) -> None:
        """
        Assign a free MPE channel to *finger_id* and send NOTE_ON.

        Args:
            finger_id : any hashable key, e.g. (hand_index, landmark_id)
            note_num  : MIDI note 0-127
            velocity  : 1-127
        """
        if not self.available_channels:
            return  # All 15 MPE note-channels are busy (shouldn't normally happen)

        chan = self.available_channels.pop(0)
        self.active_notes[finger_id] = (chan, note_num)

        # Reset per-note pitch-bend to centre (value 8192)
        pb_centre_lsb = 0x00
        pb_centre_msb = 0x40  # 0x40 << 7 = 8192
        self._send([PITCH_BEND | chan, pb_centre_lsb, pb_centre_msb])

        velocity = max(1, min(127, int(velocity)))
        self._send([NOTE_ON | chan, note_num & 0x7F, velocity])

    def update_expression(
        self,
        finger_id: object,
        pitch_bend: int,
        y_val: float,
    ) -> None:
        """
        Send per-note MPE expression for an already-sounding note.

        Args:
            finger_id  : same key used in note_on()
            pitch_bend : unsigned 14-bit (0-16383, centre = 8192)
            y_val      : normalised 0.0-1.0 → CC 74 (timbre / brightness)
        """
        if finger_id not in self.active_notes:
            return

        chan, _ = self.active_notes[finger_id]

        # Pitch Bend: split 14-bit value into LSB and MSB
        pb = max(0, min(16383, int(pitch_bend)))
        pb_lsb = pb & 0x7F
        pb_msb = (pb >> 7) & 0x7F
        self._send([PITCH_BEND | chan, pb_lsb, pb_msb])

        # CC 74 – Timbre / Brightness (Y-axis)
        cc_val = max(0, min(127, int(y_val * 127)))
        self._send([CONTROL_CHANGE | chan, CC_TIMBRE, cc_val])

    def note_off(self, finger_id: object, note_num: int | None = None) -> None:
        """
        Send NOTE_OFF and return the channel to the free pool.

        Args:
            finger_id : key used in note_on()
            note_num  : optional override; uses stored note if omitted
        """
        if finger_id not in self.active_notes:
            return

        chan, stored_note = self.active_notes.pop(finger_id)
        actual_note = stored_note if note_num is None else note_num

        self._send([NOTE_OFF | chan, actual_note & 0x7F, 0])

        # Reset pitch-bend to centre before the channel is reused
        self._send([PITCH_BEND | chan, 0x00, 0x40])

        self.available_channels.append(chan)

    def all_notes_off(self) -> None:
        """Panic: send CC 123 (All Notes Off) on every MIDI channel."""
        for channel in range(16):
            self._send([CONTROL_CHANGE | channel, CC_ALL_NOTES_OFF, 0])
        self.active_notes.clear()
        self.available_channels = list(range(1, 16))

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _send(self, message: list) -> None:
        try:
            self.midi_out.send_message(message)
        except Exception as exc:  # pragma: no cover
            print(f"[MPE] MIDI send error: {exc}")
