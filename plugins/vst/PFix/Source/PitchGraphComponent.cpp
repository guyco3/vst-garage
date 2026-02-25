/*
  ==============================================================================
    PitchGraphComponent.cpp  –  Piano-roll pitch visualizer

    NOTE: This file is #included directly by PluginEditor.cpp so it is
    compiled as part of that translation unit. It does NOT need to appear as
    a separate compiled source in the Projucer / Xcode project.
  ==============================================================================
*/

#include "PitchGraphComponent.h"

// ── Colour palette (dark, pro-audio aesthetic) ────────────────────────────────
namespace Pal
{
    const juce::Colour bg            { 0xff0d1117 };  // near-black background
    const juce::Colour blackKeyBand  { 0xff080c12 };  // slightly darker for sharp rows
    const juce::Colour semitoneLine  { 0xff1c2330 };  // dim grid between semitones
    const juce::Colour octaveLine    { 0xff3d4451 };  // brighter grid at C notes
    const juce::Colour noteLabel     { 0xff8b949e };  // muted label text
    const juce::Colour labelBg       { 0xff161b22 };  // left-column background
    const juce::Colour labelDivider  { 0xff30363d };  // vertical divider line
    const juce::Colour pitchLine     { 0xff26de81 };  // bright green pitch curve
    const juce::Colour pitchGlow     { 0x2026de81 };  // translucent glow underneath
    const juce::Colour hudBg         { 0xcc161b22 };  // HUD pill background
    const juce::Colour hudText       { 0xffffd700 };  // gold for current note
    const juce::Colour timeTick      { 0xff3d4451 };  // time axis ticks
}

// ── Static helpers ────────────────────────────────────────────────────────────

float PitchGraphComponent::hzToMidi (float hz) noexcept
{
    if (hz <= 0.0f) return -1.0f;
    return 69.0f + 12.0f * std::log2f (hz / 440.0f);
}

int PitchGraphComponent::roundToMidi (float hz) noexcept
{
    return static_cast<int> (std::round (hzToMidi (hz)));
}

bool PitchGraphComponent::isBlackKey (int midiNote) noexcept
{
    const int s = midiNote % 12;
    return s == 1 || s == 3 || s == 6 || s == 8 || s == 10;
}

juce::String PitchGraphComponent::midiToNoteName (int midiNote, bool showOctave)
{
    static constexpr const char* names[] =
        { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    const int semitone = midiNote % 12;
    const int octave   = midiNote / 12 - 1;
    juce::String s = names[semitone];
    if (showOctave) s += juce::String (octave);
    return s;
}

// ── Construction ─────────────────────────────────────────────────────────────

PitchGraphComponent::PitchGraphComponent (PitchDataQueue& q)
    : dataQueue (q)
{
    setOpaque (true);     // we fully paint our bounds → JUCE skips painting behind us
    startTimerHz (30);    // 30 fps poll + repaint
}

PitchGraphComponent::~PitchGraphComponent()
{
    stopTimer();
}

void PitchGraphComponent::resized() {}

// ── Timer callback ────────────────────────────────────────────────────────────

void PitchGraphComponent::timerCallback()
{
    PitchPoint pt;
    while (dataQueue.pop (pt))
    {
        const float midi = (pt.pitchHz > 0.0f) ? hzToMidi (pt.pitchHz) : -1.0f;
        history.push_back ({ pt.pitchHz, midi, pt.timestamp });

        if (pt.pitchHz > 0.0f)
            currentPitchHz = pt.pitchHz;

        newestTimestamp = std::max (newestTimestamp, pt.timestamp);
    }

    // Prune history older than display window + 1 s extra buffer
    const double pruneBelow = newestTimestamp
                              - static_cast<double> (displayWindowSecs) - 1.0;
    while (!history.empty() && history.front().timestamp < pruneBelow)
        history.pop_front();

    repaint();
}

// ── Coordinate conversion ─────────────────────────────────────────────────────

float PitchGraphComponent::midiToY (float midiNote) const noexcept
{
    const float h = static_cast<float> (getHeight());
    // High MIDI note → top of screen (low y value)
    return h - (midiNote - kMidiMin) / kMidiRange * h;
}

float PitchGraphComponent::timeToX (double timestamp) const noexcept
{
    const float  graphW      = static_cast<float> (getWidth() - kLabelWidth);
    const double windowStart = newestTimestamp - static_cast<double> (displayWindowSecs);
    const float  t = static_cast<float> ((timestamp - windowStart) / displayWindowSecs);
    return static_cast<float> (kLabelWidth) + t * graphW;
}

// ── Paint orchestrator ────────────────────────────────────────────────────────

void PitchGraphComponent::paint (juce::Graphics& g)
{
    drawBackground      (g);
    drawPianoRollGrid   (g);
    drawNoteLabels      (g);
    drawPitchCurve      (g);
    drawTimeAxis        (g);
    drawCurrentPitchHUD (g);
}

// ── Background ────────────────────────────────────────────────────────────────

void PitchGraphComponent::drawBackground (juce::Graphics& g) const
{
    g.fillAll (Pal::bg);

    // Left label column
    g.setColour (Pal::labelBg);
    g.fillRect (0, 0, kLabelWidth, getHeight());

    // Vertical divider between labels and graph
    g.setColour (Pal::labelDivider);
    g.drawVerticalLine (kLabelWidth, 0.0f, static_cast<float> (getHeight()));
}

// ── Piano-roll grid ───────────────────────────────────────────────────────────

void PitchGraphComponent::drawPianoRollGrid (juce::Graphics& g) const
{
    const float w          = static_cast<float> (getWidth());
    const float h          = static_cast<float> (getHeight());
    const float semitonePx = h / kMidiRange;  // height of one semitone band

    for (int midi = static_cast<int> (kMidiMin);
             midi <= static_cast<int> (kMidiMax); ++midi)
    {
        const float y = midiToY (static_cast<float> (midi));

        // ── Black key rows get a slightly darker background tint ──────────
        if (isBlackKey (midi))
        {
            g.setColour (Pal::blackKeyBand);
            g.fillRect (static_cast<float> (kLabelWidth),
                        y - semitonePx,
                        w - static_cast<float> (kLabelWidth),
                        semitonePx);
        }

        // ── Horizontal lines ──────────────────────────────────────────────
        const int semitone = midi % 12;
        if (semitone == 0)   // C – octave boundary
        {
            g.setColour (Pal::octaveLine);
            g.drawHorizontalLine (static_cast<int> (y),
                                  static_cast<float> (kLabelWidth), w);
        }
        else
        {
            g.setColour (Pal::semitoneLine);
            g.drawHorizontalLine (static_cast<int> (y),
                                  static_cast<float> (kLabelWidth), w);
        }
    }
}

// ── Note labels ───────────────────────────────────────────────────────────────

void PitchGraphComponent::drawNoteLabels (juce::Graphics& g) const
{
    for (int midi = static_cast<int> (kMidiMin);
             midi <= static_cast<int> (kMidiMax); ++midi)
    {
        const int semitone = midi % 12;

        // Label C, E, G, A to keep the column readable
        if (semitone != 0 && semitone != 4 && semitone != 7 && semitone != 9)
            continue;

        const float y = midiToY (static_cast<float> (midi));

        if (semitone == 0)
        {
            // C notes: bold + brighter (octave landmarks)
            g.setColour (Pal::octaveLine.brighter (0.5f));
            g.setFont (juce::Font (juce::FontOptions (10.0f)).boldened());
        }
        else
        {
            g.setColour (Pal::noteLabel);
            g.setFont (juce::FontOptions (9.0f));
        }

        g.drawText (midiToNoteName (midi),
                    2, static_cast<int> (y) - 8,
                    kLabelWidth - 5, 16,
                    juce::Justification::centredRight);
    }
}

// ── Pitch curve ───────────────────────────────────────────────────────────────

void PitchGraphComponent::drawPitchCurve (juce::Graphics& g) const
{
    if (history.empty())
    {
        // No data yet — show a prompt
        g.setColour (Pal::noteLabel.withAlpha (0.4f));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("Waiting for audio input...",
                    kLabelWidth, 0,
                    getWidth() - kLabelWidth, getHeight(),
                    juce::Justification::centred);
        return;
    }

    // Build voiced segments; unvoiced gaps break the path into sub-paths.
    juce::Path curvePath;
    bool inSegment = false;

    for (const auto& pt : history)
    {
        // Skip unvoiced frames and out-of-range notes
        if (pt.pitchHz <= 0.0f
            || pt.midiNote < kMidiMin - 1.5f
            || pt.midiNote > kMidiMax + 1.5f)
        {
            inSegment = false;
            continue;
        }

        const float x = timeToX  (pt.timestamp);
        const float y = midiToY  (pt.midiNote);

        if (!inSegment)
        {
            curvePath.startNewSubPath (x, y);
            inSegment = true;
        }
        else
        {
            curvePath.lineTo (x, y);
        }
    }

    // ── Glow pass (wide, semi-transparent) ───────────────────────────────
    g.setColour (Pal::pitchGlow);
    g.strokePath (curvePath,
                  juce::PathStrokeType (7.0f,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    // ── Main line ─────────────────────────────────────────────────────────
    g.setColour (Pal::pitchLine);
    g.strokePath (curvePath,
                  juce::PathStrokeType (2.0f,
                                        juce::PathStrokeType::curved,
                                        juce::PathStrokeType::rounded));

    // ── Dot at each measurement point ─────────────────────────────────────
    g.setColour (Pal::pitchLine);
    for (const auto& pt : history)
    {
        if (pt.pitchHz <= 0.0f) continue;
        const float x = timeToX (pt.timestamp);
        const float y = midiToY (pt.midiNote);
        g.fillEllipse (x - 2.0f, y - 2.0f, 4.0f, 4.0f);
    }
}

// ── Time axis ─────────────────────────────────────────────────────────────────

void PitchGraphComponent::drawTimeAxis (juce::Graphics& g) const
{
    const float h = static_cast<float> (getHeight());
    const float w = static_cast<float> (getWidth());

    const double windowStart = newestTimestamp - static_cast<double> (displayWindowSecs);

    g.setColour (Pal::timeTick.withAlpha (0.7f));
    g.setFont (juce::FontOptions (9.0f));

    const int firstSec = static_cast<int> (std::ceil  (windowStart));
    const int lastSec  = static_cast<int> (std::floor (newestTimestamp));

    for (int sec = firstSec; sec <= lastSec; ++sec)
    {
        const float x = timeToX (static_cast<double> (sec));
        if (x < static_cast<float> (kLabelWidth) || x > w) continue;

        // Tick mark
        g.drawVerticalLine (static_cast<int> (x), h - 18.0f, h - 2.0f);

        // Time label
        g.drawText (juce::String (sec) + "s",
                    static_cast<int> (x) - 16, static_cast<int> (h) - 16, 32, 12,
                    juce::Justification::centred);
    }

    // Bottom border line
    g.setColour (Pal::semitoneLine);
    g.drawHorizontalLine (static_cast<int> (h) - 19,
                          static_cast<float> (kLabelWidth), w);
}

// ── HUD (current pitch readout) ───────────────────────────────────────────────

void PitchGraphComponent::drawCurrentPitchHUD (juce::Graphics& g) const
{
    const juce::String noteText =
        (currentPitchHz > 0.0f)
        ? midiToNoteName (roundToMidi (currentPitchHz))
              + "  " + juce::String (static_cast<int> (currentPitchHz)) + " Hz"
        : "– – –";

    constexpr int hudW = 140, hudH = 30;
    const int     hudX = getWidth()  - hudW - 10;
    const int     hudY = 10;

    // Pill background
    g.setColour (Pal::hudBg);
    g.fillRoundedRectangle (static_cast<float> (hudX), static_cast<float> (hudY),
                            static_cast<float> (hudW), static_cast<float> (hudH),
                            7.0f);

    // Pill border
    g.setColour (Pal::labelDivider);
    g.drawRoundedRectangle (static_cast<float> (hudX), static_cast<float> (hudY),
                            static_cast<float> (hudW), static_cast<float> (hudH),
                            7.0f, 1.0f);

    // Text
    g.setColour (Pal::hudText);
    g.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
    g.drawText (noteText, hudX, hudY, hudW, hudH, juce::Justification::centred);
}
