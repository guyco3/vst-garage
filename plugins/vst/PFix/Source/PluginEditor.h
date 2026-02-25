/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PitchGraphComponent.h"

//==============================================================================
class PFixAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit PFixAudioProcessorEditor (PFixAudioProcessor&);
    ~PFixAudioProcessorEditor() override;

    //==============================================================================
    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    PFixAudioProcessor&   audioProcessor;
    PitchGraphComponent   pitchGraph;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PFixAudioProcessorEditor)
};
