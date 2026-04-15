/*
  ==============================================================================

    UserInstrument.h
    Created: 15 Apr 2026 2:28:07pm
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "InstrumentType.h"
#include "InstrumentPlaybackMode.h"

struct UserInstrument
{
    juce::String id;
    juce::String name;

    InstrumentPlaybackMode playbackMode = InstrumentPlaybackMode::Oscillator;
    InstrumentType oscillatorType = InstrumentType::Sine;

    juce::File sampleFile;
    int rootMidiNote = 60;

    float attack  = 0.01f;
    float decay   = 0.1f;
    float sustain = 0.8f;
    float release = 0.2f;
};
