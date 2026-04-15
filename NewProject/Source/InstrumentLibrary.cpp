/*
  ==============================================================================

    InstrumentLibrary.cpp
    Created: 15 Apr 2026 3:25:23pm
    Author:  Mark Hilton

  ==============================================================================
*/

#include "InstrumentLibrary.h"

InstrumentLibrary::InstrumentLibrary()
{
}

juce::File InstrumentLibrary::getLibraryFolder() const
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                    .getChildFile ("MyDAW")
                    .getChildFile ("Instruments");

    if (! folder.exists())
        folder.createDirectory();

    return folder;
}

juce::File InstrumentLibrary::getLibraryJsonFile() const
{
    return getLibraryFolder().getChildFile ("instrument_library.json");
}

juce::var InstrumentLibrary::instrumentToVar (const UserInstrument& instrument)
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty ("id", instrument.id);
    obj->setProperty ("name", instrument.name);
    obj->setProperty ("playbackMode", instrument.playbackMode == InstrumentPlaybackMode::Sample ? "sample" : "oscillator");
    obj->setProperty ("oscillatorType", (int) instrument.oscillatorType);
    obj->setProperty ("sampleFile", instrument.sampleFile.getFileName());
    obj->setProperty ("rootMidiNote", instrument.rootMidiNote);
    obj->setProperty ("attack", instrument.attack);
    obj->setProperty ("decay", instrument.decay);
    obj->setProperty ("sustain", instrument.sustain);
    obj->setProperty ("release", instrument.release);

    return juce::var (obj);
}

std::optional<UserInstrument> InstrumentLibrary::varToInstrument (const juce::var& v)
{
    if (! v.isObject())
        return std::nullopt;

    auto* obj = v.getDynamicObject();
    if (obj == nullptr)
        return std::nullopt;

    UserInstrument instrument;
    instrument.id = obj->getProperty ("id").toString();
    instrument.name = obj->getProperty ("name").toString();

    auto modeString = obj->getProperty ("playbackMode").toString();
    instrument.playbackMode = (modeString == "sample")
        ? InstrumentPlaybackMode::Sample
        : InstrumentPlaybackMode::Oscillator;

    instrument.oscillatorType = static_cast<InstrumentType> ((int) obj->getProperty ("oscillatorType"));
    instrument.rootMidiNote = (int) obj->getProperty ("rootMidiNote");

    instrument.attack  = (float) double (obj->getProperty ("attack"));
    instrument.decay   = (float) double (obj->getProperty ("decay"));
    instrument.sustain = (float) double (obj->getProperty ("sustain"));
    instrument.release = (float) double (obj->getProperty ("release"));

    auto libraryFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                           .getChildFile ("MyDAW")
                           .getChildFile ("Instruments");

    instrument.sampleFile = libraryFolder.getChildFile (obj->getProperty ("sampleFile").toString());

    return instrument;
}

void InstrumentLibrary::load()
{
    instruments.clear();

    auto jsonFile = getLibraryJsonFile();
    if (! jsonFile.existsAsFile())
        return;

    auto parsed = juce::JSON::parse (jsonFile);

    if (! parsed.isArray())
        return;

    auto* array = parsed.getArray();
    if (array == nullptr)
        return;

    for (const auto& item : *array)
    {
        auto instrument = varToInstrument (item);
        if (instrument.has_value())
            instruments.push_back (*instrument);
    }
}

void InstrumentLibrary::save() const
{
    juce::Array<juce::var> array;

    for (const auto& instrument : instruments)
        array.add (instrumentToVar (instrument));

    auto jsonString = juce::JSON::toString (juce::var (array), true);
    getLibraryJsonFile().replaceWithText (jsonString);
}

bool InstrumentLibrary::addSampleInstrument (const juce::File& sourceFile,
                                             const juce::String& instrumentName,
                                             int rootMidiNote)
{
    if (! sourceFile.existsAsFile())
        return false;

    auto libraryFolder = getLibraryFolder();

    auto copiedFile = libraryFolder.getChildFile (sourceFile.getFileName());

    if (copiedFile.existsAsFile())
        copiedFile = copiedFile.getNonexistentSibling();

    if (! sourceFile.copyFileTo (copiedFile))
        return false;

    UserInstrument instrument;
    instrument.id = juce::Uuid().toString();
    instrument.name = instrumentName;
    instrument.playbackMode = InstrumentPlaybackMode::Sample;
    instrument.sampleFile = copiedFile;
    instrument.rootMidiNote = rootMidiNote;

    instruments.push_back (instrument);
    save();
    return true;
}
