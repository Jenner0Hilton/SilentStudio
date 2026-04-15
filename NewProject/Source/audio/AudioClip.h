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
    double startTimeSeconds = 0.0;
    double lengthSeconds = 1.0;
    double sourceOffsetSeconds = 0.0; // where playback begins inside the WAV
    juce::Colour colour = juce::Colours::skyblue;
    
    std::shared_ptr<juce::AudioBuffer<float>> audioData;
    double sourceSampleRate = 44100.0;
};
