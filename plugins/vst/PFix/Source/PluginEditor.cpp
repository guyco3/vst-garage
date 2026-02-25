/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Include the implementation here so it is compiled without modifying
// the Projucer / Xcode project file.
#include "PitchGraphComponent.cpp"

//==============================================================================
PFixAudioProcessorEditor::PFixAudioProcessorEditor (PFixAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      pitchGraph (p.getPitchQueue())
{
    addAndMakeVisible (pitchGraph);

    setResizable (true, true);
    setResizeLimits (600, 300, 2400, 1200);
    setSize (900, 500);
}

PFixAudioProcessorEditor::~PFixAudioProcessorEditor()
{
}

//==============================================================================
void PFixAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void PFixAudioProcessorEditor::resized()
{
    pitchGraph.setBounds (getLocalBounds());
}
