"""
src/utils/filters.py

One-Euro adaptive low-pass filter for smoothing noisy real-time signals
(e.g. hand landmark coordinates from MediaPipe).

Reference:
  Casiez et al. (2012) "1€ Filter: A Simple Speed-based Low-pass Filter
  for Noisy Input in Interactive Systems", CHI 2012.

The filter adapts its cutoff frequency based on the speed of the input:
  - Slow motion  → low cutoff → heavy smoothing (removes jitter)
  - Fast motion  → high cutoff → light smoothing (preserves responsiveness)
"""

import math


class OneEuroFilter:
    """
    Instantiate once per coordinate axis per tracked point.

    Args:
        freq        : nominal sampling frequency in Hz (e.g. 60)
        min_cutoff  : minimum cutoff frequency in Hz (default 1.0)
                      Lower = smoother at rest, but more lag.
        beta        : speed coefficient (default 0.0)
                      Higher = less lag during fast motion.
        d_cutoff    : cutoff for the derivative low-pass (default 1.0)
    """

    def __init__(
        self,
        freq: float,
        min_cutoff: float = 1.0,
        beta: float = 0.0,
        d_cutoff: float = 1.0,
    ):
        self.freq = float(freq)
        self.min_cutoff = float(min_cutoff)
        self.beta = float(beta)
        self.d_cutoff = float(d_cutoff)

        self._x_prev: float | None = None
        self._dx_prev: float = 0.0
        self._t_prev: float | None = None

    def __call__(self, x: float, t: float) -> float:
        """
        Feed a new sample and return the filtered value.

        Args:
            x : raw measurement
            t : timestamp in seconds (e.g. time.time())
        """
        # Time elapsed since last sample
        t_e = (t - self._t_prev) if self._t_prev is not None else (1.0 / self.freq)
        self._t_prev = t

        # 1. Filter the derivative to suppress high-frequency bursts
        a_d = self._alpha(t_e, self.d_cutoff)
        dx = ((x - self._x_prev) / t_e) if self._x_prev is not None else 0.0
        dx_hat = a_d * dx + (1.0 - a_d) * self._dx_prev
        self._dx_prev = dx_hat

        # 2. Adapt cutoff based on instantaneous speed
        cutoff = self.min_cutoff + self.beta * abs(dx_hat)
        a = self._alpha(t_e, cutoff)

        # 3. Low-pass filter the signal itself
        x_hat = (a * x + (1.0 - a) * self._x_prev) if self._x_prev is not None else x
        self._x_prev = x_hat
        return x_hat

    def reset(self) -> None:
        """Reset internal state (call when tracking is lost)."""
        self._x_prev = None
        self._dx_prev = 0.0
        self._t_prev = None

    @staticmethod
    def _alpha(t_e: float, cutoff: float) -> float:
        r = 2.0 * math.pi * cutoff * t_e
        return r / (r + 1.0)
