/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AutoTunesAudioProcessorEditor  : public juce::AudioProcessorEditor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorEditorARAExtension
                            #endif
{
public:
    AutoTunesAudioProcessorEditor (AutoTunesAudioProcessor&);
    ~AutoTunesAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AutoTunesAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoTunesAudioProcessorEditor)
};
