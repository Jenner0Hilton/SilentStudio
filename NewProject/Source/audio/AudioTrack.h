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
#include "VideoClip.h"

enum class TrackType
{
    Video,
    Audio
};

struct AudioTrack
{
    juce::String name = "Track";
    TrackType type = TrackType::Audio;
    
    std::vector<AudioClip> clips;
    std::vector<VideoClip> videoClips;
    
    bool muted = false;
    bool solo = false;
};
