/*
  ==============================================================================
    PitchGraphComponent.h  –  Real-time pitch visualizer

    Renders a scrolling "piano-roll" style display:
      • Y axis  – pitch on a MIDI / logarithmic Hz scale (C2 – C6)
      • X axis  – time in seconds; newest data arrives from the right

    The component polls PitchDataQueue at 30 fps via an internal Timer and
    repaints itself each frame.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PitchDataQueue.h"
#include <deque>
#include <cmath>

class PitchGraphComponent : public juce::Component,
                            private juce::Timer
{
public:
    explicit PitchGraphComponent (PitchDataQueue& queue);
    ~PitchGraphComponent() override;

    // juce::Component
    void paint   (juce::Graphics& g) override;
    void resized () override;

    /** How many seconds of pitch history to display (default: 8). */
    void setDisplayWindow (float seconds) { displayWindowSecs = seconds; }

private:
    // ── Timer callback (30 fps) ───────────────────────────────────────────────
    void timerCallback() override;

    // ── Drawing passes ───────────────────────────────────────────────────────
    void drawBackground      (juce::Graphics& g) const;
    void drawPianoRollGrid   (juce::Graphics& g) const;
    void drawNoteLabels      (juce::Graphics& g) const;
    void drawPitchCurve      (juce::Graphics& g) const;
    void drawTimeAxis        (juce::Graphics& g) const;
    void drawCurrentPitchHUD (juce::Graphics& g) const;

    // ── Coordinate conversion ─────────────────────────────────────────────────
    float midiToY   (float midiNote)  const noexcept;
    float timeToX   (double timestamp) const noexcept;

    // ── Note-name utilities ───────────────────────────────────────────────────
    static float        hzToMidi       (float hz)        noexcept;
    static int          roundToMidi    (float hz)        noexcept;
    static juce::String midiToNoteName (int midiNote,
                                        bool showOctave = true);
    static bool         isBlackKey     (int midiNote)    noexcept;

    // ── Data ──────────────────────────────────────────────────────────────────
    PitchDataQueue& dataQueue;

    struct DisplayPoint
    {
        float  pitchHz;
        float  midiNote;   // cached, avoids recomputing log2 every frame
        double timestamp;
    };

    std::deque<DisplayPoint> history;
    float  currentPitchHz   { 0.0f };
    double newestTimestamp  { 0.0  };
    float  displayWindowSecs{ 8.0f };

    // ── Layout ────────────────────────────────────────────────────────────────
    static constexpr int   kLabelWidth = 46;
    static constexpr float kMidiMin    = 36.0f;   // C2  (~65 Hz)
    static constexpr float kMidiMax    = 84.0f;   // C6  (~1047 Hz)
    static constexpr float kMidiRange  = kMidiMax - kMidiMin;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchGraphComponent)
};
