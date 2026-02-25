/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Include the implementation here so it is compiled without modifying
// the Projucer / Xcode project file.  This is the standard JUCE unity-build
// pattern — each .cpp is compiled exactly once.
#include "PitchDetector.cpp"

//==============================================================================
PFixAudioProcessor::PFixAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

PFixAudioProcessor::~PFixAudioProcessor()
{
}

//==============================================================================
const juce::String PFixAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PFixAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PFixAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PFixAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PFixAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PFixAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PFixAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PFixAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PFixAudioProcessor::getProgramName (int index)
{
    return {};
}

void PFixAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PFixAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate     = sampleRate;
    analysisBuffer.assign (kAnalysisSize, 0.0f);
    analysisBufferFill    = 0;
    totalSamplesProcessed = 0;
    pitchQueue.reset();

    juce::ignoreUnused (samplesPerBlock);
}

void PFixAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PFixAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PFixAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const int numInputChannels  = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples        = buffer.getNumSamples();

    // Clear any output-only channels (prevents garbage on extra outputs)
    for (int ch = numInputChannels; ch < numOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    // Nothing to analyse without input
    if (numInputChannels == 0)
    {
        totalSamplesProcessed += numSamples;
        return;
    }

    // ── Mix down to mono and accumulate into the analysis buffer ─────────────
    const float* chL = buffer.getReadPointer (0);
    const float* chR = (numInputChannels > 1) ? buffer.getReadPointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mono = chR ? (chL[i] + chR[i]) * 0.5f : chL[i];
        analysisBuffer[analysisBufferFill++] = mono;

        if (analysisBufferFill >= kAnalysisSize)
        {
            // ── Run YIN on the completed analysis window ─────────────────
            const float pitchHz =
                pitchDetector.detectPitch (analysisBuffer.data(),
                                           kAnalysisSize,
                                           currentSampleRate);

            // Timestamp = position of the last sample in this window
            const double timestamp =
                static_cast<double> (totalSamplesProcessed + i + 1)
                / currentSampleRate;

            pitchQueue.push ({ pitchHz, timestamp });
            analysisBufferFill = 0;
        }
    }

    totalSamplesProcessed += numSamples;
}

//==============================================================================
bool PFixAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PFixAudioProcessor::createEditor()
{
    return new PFixAudioProcessorEditor (*this);
}

//==============================================================================
void PFixAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PFixAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PFixAudioProcessor();
}
