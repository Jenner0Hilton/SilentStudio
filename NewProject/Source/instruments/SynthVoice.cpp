/*
  ==============================================================================

    SynthVoice.cpp
    Created: 10 Feb 2026 9:49:03am
    Author:  Mark Hilton

  ==============================================================================
*/

#include "SynthVoice.h"
#include "SynthSound.h"

SynthVoice::SynthVoice()
{
    oscillator.initialise ([] (float x) { return std::sin (x); });

    adsrParams.attack  = 0.01f;
    adsrParams.decay   = 0.1f;
    adsrParams.sustain = 0.8f;
    adsrParams.release = 0.2f;

    adsr.setParameters (adsrParams);
}

bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::prepareToPlay (double sampleRate, int, int)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;

    oscillator.prepare (spec);
    adsr.setSampleRate (sampleRate);
}

void SynthVoice::startNote (int midiNoteNumber,
                            float velocity,
                            juce::SynthesiserSound*,
                            int)
{
    auto freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    oscillator.setFrequency (freq);

    level = velocity;
    adsr.noteOn();
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
        adsr.noteOff();
    else
        clearCurrentNote();
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& buffer,
                                  int startSample,
                                  int numSamples)
{
    if (! isVoiceActive())
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto env = adsr.getNextSample();
        auto value = oscillator.processSample (0.0f) * level * env;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.addSample (channel, startSample + sample, value);
    }

    if (! adsr.isActive())
        clearCurrentNote();
}
