/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
struct SineWaveSound : public juce::SynthesiserSound
{
    SineWaveSound() {}
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
template <typename Type>
class CustomOscillator
{
public:
    enum { oscIndex, gainIndex };

    CustomOscillator()
    {
        // Sawtooth wave: linearly maps -pi..pi to -1..1
        auto& osc = processorChain.template get<oscIndex>();
        osc.initialise ([] (Type x)
        {
            return juce::jmap (x,
                               Type (-juce::MathConstants<double>::pi),
                               Type ( juce::MathConstants<double>::pi),
                               Type (-1), Type (1));
        }, 2);
    }

    void prepare (const juce::dsp::ProcessSpec& spec) { processorChain.prepare (spec); }
    void reset()  noexcept                             { processorChain.reset();  }

    void setFrequency (Type newValue, bool force = false)
    {
        processorChain.template get<oscIndex>().setFrequency (newValue, force);
    }

    void setLevel (Type newValue)
    {
        processorChain.template get<gainIndex>().setGainLinear (newValue);
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        processorChain.process (context);
    }

private:
    juce::dsp::ProcessorChain<juce::dsp::Oscillator<Type>, juce::dsp::Gain<Type>> processorChain;
};

//==============================================================================
class DSPVoice : public juce::SynthesiserVoice
{
public:
    void setADSRParameters (const juce::ADSR::Parameters& params)
    {
        adsr.setParameters (params);
    }

    void setLFOFrequency (float freqHz)
    {
        lfo.setFrequency (freqHz);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        currentSampleRate = spec.sampleRate;
        tempBlock = juce::dsp::AudioBlock<float> (heapBlock, spec.numChannels, spec.maximumBlockSize);
        processorChain.prepare (spec);

        // Set initial ladder filter and master gain
        auto& filter = processorChain.get<filterIndex>();
        filter.setCutoffFrequencyHz (1000.0f);
        filter.setResonance (0.7f);
        processorChain.get<masterGainIndex>().setGainLinear (0.7f);

        // LFO: sine wave at 3 Hz, processed 100x less often than audio rate
        lfo.initialise ([] (float x) { return std::sin (x); }, 128);
        lfo.setFrequency (3.0f);
        lfo.prepare ({ spec.sampleRate / lfoUpdateRate, spec.maximumBlockSize, spec.numChannels });

        adsr.setSampleRate (spec.sampleRate);
    }

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int /*pitchWheelPosition*/) override
    {
        auto freqHz = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

        // OSC1: fundamental frequency
        processorChain.get<osc1Index>().setFrequency (freqHz, true);
        processorChain.get<osc1Index>().setLevel (velocity);

        // OSC2: slightly detuned (+1%) for a thicker sound
        processorChain.get<osc2Index>().setFrequency (freqHz * 1.01f, true);
        processorChain.get<osc2Index>().setLevel (velocity);

        adsr.setSampleRate (currentSampleRate);
        adsr.noteOn();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            clearCurrentNote();
            adsr.reset();
            processorChain.reset();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer,
                          int startSample, int numSamples) override
    {
        if (! adsr.isActive())
            return;

        auto output = tempBlock.getSubBlock (0, (size_t) numSamples);
        output.clear();

        // Process DSP chain in chunks so the LFO can update between them
        for (size_t pos = 0; pos < (size_t) numSamples;)
        {
            auto max   = juce::jmin ((size_t) numSamples - pos, lfoUpdateCounter);
            auto block = output.getSubBlock (pos, max);

            juce::dsp::ProcessContextReplacing<float> context (block);
            processorChain.process (context);

            pos              += max;
            lfoUpdateCounter -= max;

            if (lfoUpdateCounter == 0)
            {
                lfoUpdateCounter  = lfoUpdateRate;
                auto lfoOut       = lfo.processSample (0.0f);
                // LFO modulates ladder filter cutoff between 100 Hz and 2 kHz
                auto cutoffFreqHz = juce::jmap (lfoOut, -1.0f, 1.0f, 100.0f, 2000.0f);
                processorChain.get<filterIndex>().setCutoffFrequencyHz (cutoffFreqHz);
            }
        }

        // Apply ADSR envelope per-sample, then mix into output buffer
        for (int i = 0; i < numSamples; ++i)
        {
            const float env = adsr.getNextSample();
            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                outputBuffer.getWritePointer (ch, startSample)[i] +=
                    tempBlock.getChannelPointer (ch)[i] * env;
        }

        if (! adsr.isActive())
        {
            clearCurrentNote();
            processorChain.reset();
        }
    }

private:
    static constexpr size_t lfoUpdateRate    = 100;
    size_t                  lfoUpdateCounter = lfoUpdateRate;

    juce::HeapBlock<char>        heapBlock;
    juce::dsp::AudioBlock<float> tempBlock;

    enum { osc1Index, osc2Index, filterIndex, masterGainIndex };

    juce::dsp::ProcessorChain<CustomOscillator<float>,
                               CustomOscillator<float>,
                               juce::dsp::LadderFilter<float>,
                               juce::dsp::Gain<float>> processorChain;

    juce::dsp::Oscillator<float> lfo;
    juce::ADSR                   adsr;
    double                       currentSampleRate = 44100.0;
};

//==============================================================================
/**
*/
class NewProjectAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    NewProjectAudioProcessor();
    ~NewProjectAudioProcessor() override;

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

    // Exposed so the editor can attach a MidiKeyboardComponent to it
    juce::MidiKeyboardState keyboardState;

    // Parameter tree — public so the editor can create SliderAttachments
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    //==============================================================================
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;

    // Post-process effects chain: reverb applied to the full mix
    enum { reverbIndex };
    juce::dsp::ProcessorChain<juce::dsp::Reverb> fxChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessor)
};
