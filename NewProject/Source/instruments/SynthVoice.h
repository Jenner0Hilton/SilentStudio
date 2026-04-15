/*
  ==============================================================================

    SynthVoice.h
    Created: 10 Feb 2026 9:49:03am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../InstrumentType.h"
#include "../InstrumentPlaybackMode.h"

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
    
    void setInstrumentType (InstrumentType type) { instrumentType = type; }
    void setPlaybackMode (InstrumentPlaybackMode mode) { playbackMode = mode; }

    void setSampleData (std::shared_ptr<juce::AudioBuffer<float>> newSample,
                            double sampleRate,
                            int rootNote);
    
    

private:
    float getOscSample (float phase);
    
    juce::dsp::Oscillator<float> oscillator;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    //juce::dsp::ADSR adsr;
    //juce::dsp::ADSR::Parameters adsrParams;

    double currentSampleRate { 44100.0 };
    float level { 0.0f };
    
    InstrumentType instrumentType = InstrumentType::Sine;
    InstrumentPlaybackMode playbackMode = InstrumentPlaybackMode::Oscillator;
    
    std::shared_ptr<juce::AudioBuffer<float>> sampleData;
    double sampleDataSampleRate = 44100.0;
    int sampleRootMidiNote = 60;

    double sampleReadPosition = 0.0;
    double sampleReadIncrement = 1.0;
};

