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
class NewProjectAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    NewProjectAudioProcessor& audioProcessor;

    juce::MidiKeyboardComponent keyboardComponent;

    // ADSR knobs
    juce::Slider attackSlider,  decaySlider,  sustainSlider,  releaseSlider;
    juce::Label  attackLabel,   decayLabel,   sustainLabel,   releaseLabel;

    // LFO + Reverb knobs
    juce::Slider lfoFreqSlider, reverbSizeSlider, reverbDampingSlider, reverbWetSlider, reverbWidthSlider;
    juce::Label  lfoFreqLabel,  reverbSizeLabel,  reverbDampingLabel,  reverbWetLabel,  reverbWidthLabel;

    // Tie sliders to APVTS parameters so they stay in sync with the host
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> decayAttachment;
    std::unique_ptr<SliderAttachment> sustainAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> lfoFreqAttachment;
    std::unique_ptr<SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<SliderAttachment> reverbWetAttachment;
    std::unique_ptr<SliderAttachment> reverbWidthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
