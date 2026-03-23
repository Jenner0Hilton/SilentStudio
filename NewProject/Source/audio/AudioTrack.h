/*
  ==============================================================================

    AudioTrack.h
    Created: 23 Mar 2026 6:32:50am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "AudioClip.h"

struct AudioTrack
{
    juce::String name = "Track";
    std::vector<AudioClip> clips;
};
