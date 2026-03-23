/*
  ==============================================================================

    AudioClip.h
    Created: 23 Mar 2026 6:32:16am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct AudioClip
{
    juce::File file;
    juce::String name = "Clip";
    double startBeat = 0.0;
    double lengthBeats = 4.0;
    juce::Colour colour = juce::Colours::skyblue;
    
    std::shared_ptr<juce::AudioBuffer<float>> audioData;
    double sourceSampleRate = 44100.0;
};
