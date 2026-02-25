/*
  ==============================================================================
    PitchDataQueue.h  –  Lock-free SPSC queue for pitch data

    Passes PitchPoint structs from the audio thread (producer) to the
    UI thread (consumer) without any locks or heap allocations.

    Built on top of juce::AbstractFifo which provides all the index
    arithmetic for a single-producer / single-consumer ring buffer.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

/** One pitch measurement produced every ~analysisSize / sampleRate seconds. */
struct PitchPoint
{
    float  pitchHz   { 0.0f };  ///< Fundamental in Hz; 0 = unvoiced / silence
    double timestamp { 0.0  };  ///< Seconds elapsed since the processor started
};

/**
 * Single-Producer Single-Consumer lock-free ring buffer.
 *
 * Capacity: 4 096 frames ≈ 3+ minutes of unread data at one frame per ~46 ms,
 * so the audio thread never has to wait even if the UI is briefly suspended.
 *
 * Audio thread:  call push()
 * UI thread:     call pop() / numReady()
 */
class PitchDataQueue
{
public:
    static constexpr int kCapacity = 4096;

    PitchDataQueue() : fifo (kCapacity) {}

    // ── Producer (audio thread) ───────────────────────────────────────────────

    /** Enqueues a point.  Silently drops it when the ring buffer is full. */
    void push (PitchPoint pt) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);

        if      (n1 > 0) ringBuf[s1] = pt;
        else if (n2 > 0) ringBuf[s2] = pt;

        fifo.finishedWrite (n1 + n2 > 0 ? 1 : 0);
    }

    // ── Consumer (UI thread) ──────────────────────────────────────────────────

    /** Dequeues the oldest point.  Returns false when the queue is empty. */
    bool pop (PitchPoint& out) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (1, s1, n1, s2, n2);

        if      (n1 > 0) out = ringBuf[s1];
        else if (n2 > 0) out = ringBuf[s2];

        const bool hadData = (n1 + n2) > 0;
        fifo.finishedRead (hadData ? 1 : 0);
        return hadData;
    }

    int  numReady () const noexcept { return fifo.getNumReady(); }
    void reset    ()       noexcept { fifo.reset(); }

private:
    juce::AbstractFifo                 fifo;
    std::array<PitchPoint, kCapacity>  ringBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchDataQueue)
};
