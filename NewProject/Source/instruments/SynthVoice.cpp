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
    oscillator.initialise ([this] (float x) { return getOscSample (x); });

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
    level = velocity;

        if (playbackMode == InstrumentPlaybackMode::Oscillator)
        {
            auto freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
            oscillator.setFrequency (freq);
        }
        else if (playbackMode == InstrumentPlaybackMode::Sample)
        {
            if (sampleData != nullptr)
            {
                sampleReadPosition = 0.0;

                double semitoneRatio = std::pow (2.0,
                                                 (midiNoteNumber - sampleRootMidiNote) / 12.0);

                sampleReadIncrement = (sampleDataSampleRate / currentSampleRate) * semitoneRatio;
            }
        }

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
        float value = 0.0f;

        if (playbackMode == InstrumentPlaybackMode::Oscillator)
        {
            value = oscillator.processSample (0.0f);
        }
        else if (playbackMode == InstrumentPlaybackMode::Sample)
        {
            if (sampleData != nullptr)
            {
                int numSourceSamples = sampleData->getNumSamples();

                if ((int) sampleReadPosition >= numSourceSamples - 1)
                {
                    clearCurrentNote();
                    break;
                }

                int i0 = (int) std::floor (sampleReadPosition);
                int i1 = juce::jmin (i0 + 1, numSourceSamples - 1);
                float frac = (float) (sampleReadPosition - (double) i0);

                float sampleValue = 0.0f;

                for (int ch = 0; ch < sampleData->getNumChannels(); ++ch)
                {
                    const float* data = sampleData->getReadPointer (ch);
                    float s0 = data[i0];
                    float s1 = data[i1];
                    sampleValue += s0 + frac * (s1 - s0);
                }

                sampleValue /= (float) sampleData->getNumChannels();
                value = sampleValue;

                sampleReadPosition += sampleReadIncrement;
            }
        }

        value *= level * env;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.addSample (channel, startSample + sample, value);
    }

    if (! adsr.isActive())
        clearCurrentNote();
}

float SynthVoice::getOscSample (float phase)
{
    switch (instrumentType)
    {
        case InstrumentType::Sine:
            return std::sin (phase);

        case InstrumentType::Square:
            return std::sin (phase) >= 0.0f ? 1.0f : -1.0f;

        case InstrumentType::Saw:
            return juce::jmap (phase, 0.0f, juce::MathConstants<float>::twoPi, -1.0f, 1.0f);

        case InstrumentType::Triangle:
        {
            float t = phase / juce::MathConstants<float>::twoPi;
            return 2.0f * std::abs (2.0f * (t - std::floor (t + 0.5f))) - 1.0f;
        }
    }

    return 0.0f;
}

void SynthVoice::setSampleData (std::shared_ptr<juce::AudioBuffer<float>> newSample,
                                double sampleRate,
                                int rootNote)
{
    sampleData = std::move (newSample);
    sampleDataSampleRate = sampleRate;
    sampleRootMidiNote = rootNote;
}
