/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#else
     : apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Add 8 polyphonic DSP voices (2x sawtooth oscs, ladder filter, LFO, ADSR)
    for (auto i = 0; i < 8; ++i)
        synth.addVoice (new DSPVoice());

    synth.addSound (new SineWaveSound());
}

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Skew factor 0.4 gives more resolution at shorter times
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "attack",  "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f), 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "decay",   "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.4f), 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "sustain", "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "release", "Release",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.4f), 0.5f));

    // LFO frequency
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "lfoFreq", "LFO Freq",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.5f), 3.0f));

    // Reverb
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverbSize",    "Room Size",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverbDamping", "Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverbWet",     "Wet Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.33f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverbWidth",   "Width",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String NewProjectAudioProcessor::getProgramName (int index)
{
    return {};
}

void NewProjectAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    midiCollector.reset (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = (juce::uint32) getTotalNumOutputChannels();

    // Prepare each DSP voice with the audio spec
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<DSPVoice*> (synth.getVoice (i)))
            voice->prepare (spec);

    // Prepare reverb effects chain
    fxChain.prepare (spec);
}

void NewProjectAudioProcessor::releaseResources()
{
    keyboardState.reset();
    fxChain.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear the output buffer
    buffer.clear();

    // Push current ADSR parameter values to all voices
    juce::ADSR::Parameters adsrParams;
    adsrParams.attack  = apvts.getRawParameterValue ("attack")->load();
    adsrParams.decay   = apvts.getRawParameterValue ("decay")->load();
    adsrParams.sustain = apvts.getRawParameterValue ("sustain")->load();
    adsrParams.release = apvts.getRawParameterValue ("release")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<DSPVoice*> (synth.getVoice (i)))
            voice->setADSRParameters (adsrParams);

    // Push LFO frequency to all voices
    const float lfoFreq = apvts.getRawParameterValue ("lfoFreq")->load();
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<DSPVoice*> (synth.getVoice (i)))
            voice->setLFOFrequency (lfoFreq);

    // Update reverb parameters
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize   = apvts.getRawParameterValue ("reverbSize")->load();
    reverbParams.damping    = apvts.getRawParameterValue ("reverbDamping")->load();
    reverbParams.wetLevel   = apvts.getRawParameterValue ("reverbWet")->load();
    reverbParams.dryLevel   = 1.0f - reverbParams.wetLevel;
    reverbParams.width      = apvts.getRawParameterValue ("reverbWidth")->load();
    reverbParams.freezeMode = 0.0f;
    fxChain.get<reverbIndex>().setParameters (reverbParams);

    // Collect any MIDI from external hardware and merge into midiMessages
    midiCollector.removeNextBlockOfMessages (midiMessages, buffer.getNumSamples());

    // Let the keyboard state process the buffer
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // Render all active synth voices into the buffer
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply reverb to the full mix
    auto block        = juce::dsp::AudioBlock<float> (buffer);
    auto contextToUse = juce::dsp::ProcessContextReplacing<float> (block);
    fxChain.process (contextToUse);
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}
