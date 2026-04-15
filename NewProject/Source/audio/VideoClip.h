/*
  ==============================================================================

    VideoClip.h
    Created: 15 Apr 2026 8:35:03am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct VideoClip
{
    juce::File file;
    juce::String name = "Video";

    double startTimeSeconds = 0.0;
    double lengthSeconds = 1.0;

    juce::Colour colour = juce::Colours::mediumpurple;
};
