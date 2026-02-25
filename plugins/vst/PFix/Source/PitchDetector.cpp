/*
  ==============================================================================
    PitchDetector.cpp  –  YIN pitch detection implementation

    NOTE: This file is #included directly by PluginProcessor.cpp so it is
    compiled as part of that translation unit. It does NOT need to appear as
    a separate compiled source in the Projucer / Xcode project.
  ==============================================================================
*/

#include "PitchDetector.h"

PitchDetector::PitchDetector (int size)
    : analysisSize (size),
      yinBuf (size / 2, 0.0f)
{
    // Must be a power-of-two to keep the algorithm well-behaved
    jassert (size >= 512 && (size & (size - 1)) == 0);
}

float PitchDetector::detectPitch (const float* samples, int numSamples, double sampleRate)
{
    if (numSamples < analysisSize)
        return 0.0f;

    // ── Energy gate ─────────────────────────────────────────────────────────
    // Skip very quiet frames; avoids phantom detections in silence.
    float energy = 0.0f;
    for (int i = 0; i < analysisSize; ++i)
        energy += samples[i] * samples[i];
    energy /= static_cast<float> (analysisSize);

    if (energy < 1e-6f)   // roughly –60 dBFS
        return 0.0f;

    const int halfSize = analysisSize / 2;

    // ── Step 1: Difference function ─────────────────────────────────────────
    //   d(τ) = Σ_{j=0}^{W/2−1} (x[j] − x[j+τ])²
    for (int tau = 0; tau < halfSize; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfSize; ++j)
        {
            const float delta = samples[j] - samples[j + tau];
            sum += delta * delta;
        }
        yinBuf[tau] = sum;
    }

    // ── Step 2: Cumulative mean normalised difference function (CMNDF) ───────
    //   d'(0) = 1
    //   d'(τ) = d(τ) · τ / Σ_{j=1}^{τ} d(j)
    //
    // This normalisation removes the trivial minimum at τ = 0 and makes the
    // threshold comparison meaningful across different signal levels.
    yinBuf[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < halfSize; ++tau)
    {
        runningSum += yinBuf[tau];
        yinBuf[tau] = (runningSum > 0.0f)
                      ? yinBuf[tau] * static_cast<float> (tau) / runningSum
                      : 1.0f;
    }

    // ── Step 3: First local minimum below threshold ──────────────────────────
    // Constrain the search to a musically meaningful frequency band.
    const int tauMin = static_cast<int> (std::ceil  (sampleRate / 1200.0));  // ~1200 Hz
    const int tauMax = std::min (static_cast<int> (std::floor (sampleRate / 40.0)),
                                 halfSize - 2);                               // ~40 Hz

    int tauEst = -1;
    for (int tau = tauMin; tau <= tauMax; ++tau)
    {
        if (yinBuf[tau] < threshold)
        {
            // Walk to the bottom of the dip (local minimum)
            while (tau + 1 <= tauMax && yinBuf[tau + 1] < yinBuf[tau])
                ++tau;
            tauEst = tau;
            break;
        }
    }

    if (tauEst < 1)
        return 0.0f;

    // ── Step 4: Parabolic interpolation for sub-sample precision ─────────────
    const float refinedTau = parabolicInterpolation (tauEst);
    if (refinedTau <= 0.0f)
        return 0.0f;

    const float pitchHz = static_cast<float> (sampleRate) / refinedTau;

    // Final sanity check: keep within the vocal / instrument range we care about
    return (pitchHz >= 40.0f && pitchHz <= 2000.0f) ? pitchHz : 0.0f;
}

float PitchDetector::parabolicInterpolation (int tau) const noexcept
{
    const int n = static_cast<int> (yinBuf.size());
    if (tau < 1 || tau >= n - 1)
        return static_cast<float> (tau);

    const float s0 = yinBuf[tau - 1];
    const float s1 = yinBuf[tau];
    const float s2 = yinBuf[tau + 1];

    // Vertex of the parabola through (τ−1, s0), (τ, s1), (τ+1, s2):
    //   x_min = τ + 0.5 · (s0 − s2) / (s0 − 2·s1 + s2)
    const float denom = s0 - 2.0f * s1 + s2;
    if (std::abs (denom) < 1e-8f)
        return static_cast<float> (tau);

    return static_cast<float> (tau) + 0.5f * (s0 - s2) / denom;
}
