/*
  ==============================================================================

    Note.h
    Created: 10 Feb 2026 9:51:03pm
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include "InstrumentType.h"
#include "InstrumentPlaybackMode.h"

struct Note
{
    int midiNote = 60;
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    
    InstrumentPlaybackMode playbackMode = InstrumentPlaybackMode::Oscillator;
    InstrumentType instrument = InstrumentType::Sine;
    
    juce::String userInstrumentId;
};
