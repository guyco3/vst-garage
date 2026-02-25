/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PitchDetector.h"
#include "PitchDataQueue.h"
#include <vector>

//==============================================================================
/**
*/
class PFixAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    PFixAudioProcessor();
    ~PFixAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Safe to call from any thread — returns reference to the lock-free queue
        that the audio thread writes to and the UI thread reads from. */
    PitchDataQueue& getPitchQueue() noexcept { return pitchQueue; }

private:
    //==============================================================================
    // ── Pitch analysis (all accessed only on the audio thread) ───────────────
    static constexpr int kAnalysisSize = 2048;  // ~46 ms at 44100 Hz

    PitchDetector      pitchDetector  { kAnalysisSize };
    PitchDataQueue     pitchQueue;
    std::vector<float> analysisBuffer;           // accumulated mono samples
    int                analysisBufferFill  { 0 };
    double             currentSampleRate   { 44100.0 };
    long long          totalSamplesProcessed { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PFixAudioProcessor)
};
