/*
  ==============================================================================
    PitchDetector.h  –  YIN pitch detection algorithm

    de Cheveigné, A. & Kawahara, H. (2002).
    "YIN, a fundamental frequency estimator for speech and music."
    Journal of the Acoustical Society of America, 111(4), 1917–1930.

    Design constraints (audio-thread safe):
      • No heap allocation after construction
      • No locks
      • No virtual dispatch
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <cmath>

/**
 * Detects the fundamental frequency of a mono audio frame using YIN.
 *
 * Typical usage:
 *   PitchDetector pd (2048);
 *   float hz = pd.detectPitch (monoSamples, 2048, 44100.0);
 */
class PitchDetector
{
public:
    /**
     * @param analysisSize  Buffer size in samples.  Must be a power-of-two >= 512.
     *                      2048 works well for vocals at 44100 Hz:
     *                        • detects as low as ~43 Hz (bass), up to ~1 200 Hz
     */
    explicit PitchDetector (int analysisSize = 2048);

    /**
     * Returns the fundamental frequency in Hz, or 0.0f when no clear pitch
     * is detected (silence, noise, or unvoiced consonants).
     *
     * @param samples     Pointer to at least analysisSize mono float samples.
     * @param numSamples  Must be >= analysisSize.
     * @param sampleRate  Current sample rate of the host (Hz).
     */
    float detectPitch (const float* samples, int numSamples, double sampleRate);

    int   getAnalysisSize () const noexcept { return analysisSize; }

    /** Confidence threshold for the CMNDF minimum (0.05 – 0.5, default 0.15). */
    void  setThreshold (float t)  noexcept { threshold = juce::jlimit (0.05f, 0.5f, t); }
    float getThreshold ()   const noexcept { return threshold; }

private:
    /** Refines the integer tau estimate using parabolic interpolation. */
    float parabolicInterpolation (int tau) const noexcept;

    int                analysisSize;
    float              threshold { 0.15f };
    std::vector<float> yinBuf;   // length = analysisSize / 2, allocated once in ctor
};
