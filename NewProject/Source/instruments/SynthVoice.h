/*
  ==============================================================================

    SynthVoice.h
    Created: 10 Feb 2026 9:49:03am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice();

    bool canPlaySound (juce::SynthesiserSound*) override;

    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound*,
                    int currentPitchWheelPosition) override;

    void stopNote (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock (juce::AudioBuffer<float>&,
                          int startSample,
                          int numSamples) override;

private:
    juce::dsp::Oscillator<float> oscillator;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    //juce::dsp::ADSR adsr;
    //juce::dsp::ADSR::Parameters adsrParams;

    double currentSampleRate { 44100.0 };
    float level { 0.0f };
};

