/*
  ==============================================================================

    InstrumentLibrary.h
    Created: 15 Apr 2026 3:24:52pm
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "UserInstrument.h"

class InstrumentLibrary
{
public:
    InstrumentLibrary();

    juce::File getLibraryFolder() const;
    juce::File getLibraryJsonFile() const;

    void load();
    void save() const;

    const std::vector<UserInstrument>& getInstruments() const { return instruments; }

    bool addSampleInstrument (const juce::File& sourceFile,
                              const juce::String& instrumentName,
                              int rootMidiNote = 60);

private:
    std::vector<UserInstrument> instruments;

    static juce::var instrumentToVar (const UserInstrument& instrument);
    static std::optional<UserInstrument> varToInstrument (const juce::var& v);
};
