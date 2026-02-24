/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      keyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // Helper to configure a rotary slider + centred label
    auto setupKnob = [this](juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        addAndMakeVisible (l);
    };

    setupKnob (attackSlider,  attackLabel,  "Attack");
    setupKnob (decaySlider,   decayLabel,   "Decay");
    setupKnob (sustainSlider, sustainLabel, "Sustain");
    setupKnob (releaseSlider, releaseLabel, "Release");

    setupKnob (lfoFreqSlider,       lfoFreqLabel,       "LFO Freq");
    setupKnob (reverbSizeSlider,    reverbSizeLabel,    "Room Size");
    setupKnob (reverbDampingSlider, reverbDampingLabel, "Damping");
    setupKnob (reverbWetSlider,     reverbWetLabel,     "Wet");
    setupKnob (reverbWidthSlider,   reverbWidthLabel,   "Width");

    // Bind sliders to the APVTS parameters
    attackAttachment        = std::make_unique<SliderAttachment> (p.apvts, "attack",        attackSlider);
    decayAttachment         = std::make_unique<SliderAttachment> (p.apvts, "decay",         decaySlider);
    sustainAttachment       = std::make_unique<SliderAttachment> (p.apvts, "sustain",       sustainSlider);
    releaseAttachment       = std::make_unique<SliderAttachment> (p.apvts, "release",       releaseSlider);
    lfoFreqAttachment       = std::make_unique<SliderAttachment> (p.apvts, "lfoFreq",       lfoFreqSlider);
    reverbSizeAttachment    = std::make_unique<SliderAttachment> (p.apvts, "reverbSize",    reverbSizeSlider);
    reverbDampingAttachment = std::make_unique<SliderAttachment> (p.apvts, "reverbDamping", reverbDampingSlider);
    reverbWetAttachment     = std::make_unique<SliderAttachment> (p.apvts, "reverbWet",     reverbWetSlider);
    reverbWidthAttachment   = std::make_unique<SliderAttachment> (p.apvts, "reverbWidth",   reverbWidthSlider);

    addAndMakeVisible (keyboardComponent);
    setSize (700, 480);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
}

//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const int padding       = 10;
    const int titleH        = 30;
    const int sectionLabelH = 20;
    const int knobH         = 110 + 20; // slider + knob label
    const int rowGap        = 8;

    // Title
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawFittedText ("DSP Synthesiser", 0, 0, getWidth(), titleH, juce::Justification::centred, 1);

    // Section headers
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (juce::Colours::lightblue);
    g.drawFittedText ("ENVELOPE",
                      padding, titleH, getWidth() - padding * 2, sectionLabelH,
                      juce::Justification::left, 1);

    const int row2Y = titleH + sectionLabelH + knobH + rowGap;
    g.drawFittedText ("LFO / REVERB",
                      padding, row2Y, getWidth() - padding * 2, sectionLabelH,
                      juce::Justification::left, 1);
}

void NewProjectAudioProcessorEditor::resized()
{
    const int padding       = 10;
    const int totalW        = getWidth() - padding * 2;
    const int titleH        = 30;
    const int sectionLabelH = 20;
    const int knobLabelH    = 20;
    const int knobH         = 110;
    const int rowGap        = 8;
    const int keyboardH     = 120;

    // --- Row 1: ADSR (4 knobs) ---
    const int numADSR  = 4;
    const int adsrColW = totalW / numADSR;
    const int row1Y    = titleH + sectionLabelH;

    juce::Slider* adsrSliders[] = { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider };
    juce::Label*  adsrLabels[]  = { &attackLabel,  &decayLabel,  &sustainLabel,  &releaseLabel  };

    for (int i = 0; i < numADSR; ++i)
    {
        const int x = padding + i * adsrColW;
        adsrLabels[i]->setBounds  (x, row1Y,              adsrColW, knobLabelH);
        adsrSliders[i]->setBounds (x, row1Y + knobLabelH, adsrColW, knobH);
    }

    // --- Row 2: LFO + Reverb (5 knobs) ---
    const int numFX  = 5;
    const int fxColW = totalW / numFX;
    const int row2Y  = titleH + sectionLabelH + knobLabelH + knobH + rowGap + sectionLabelH;

    juce::Slider* fxSliders[] = { &lfoFreqSlider, &reverbSizeSlider, &reverbDampingSlider, &reverbWetSlider, &reverbWidthSlider };
    juce::Label*  fxLabels[]  = { &lfoFreqLabel,  &reverbSizeLabel,  &reverbDampingLabel,  &reverbWetLabel,  &reverbWidthLabel  };

    for (int i = 0; i < numFX; ++i)
    {
        const int x = padding + i * fxColW;
        fxLabels[i]->setBounds  (x, row2Y,              fxColW, knobLabelH);
        fxSliders[i]->setBounds (x, row2Y + knobLabelH, fxColW, knobH);
    }

    // --- Keyboard ---
    const int keyboardY = row2Y + knobLabelH + knobH + rowGap;
    keyboardComponent.setBounds (padding, keyboardY, totalW, keyboardH);
}
