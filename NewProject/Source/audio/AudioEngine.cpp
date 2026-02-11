/*
  ==============================================================================

    AudioEngine.cpp
    Created: 10 Feb 2026 9:47:11am
    Author:  Mark Hilton

  ==============================================================================
*/

#include "AudioEngine.h"
#include "../ui/PianoRoll.h"

AudioEngine::AudioEngine() {
    for (int i = 0; i < 8; ++i)
           synth.addVoice (new SynthVoice());

       synth.addSound (new SynthSound());
}
AudioEngine::~AudioEngine() {}

void AudioEngine::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

       for (int i = 0; i < synth.getNumVoices(); ++i)
           if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
               voice->prepareToPlay (sampleRate, samplesPerBlock, 2);
   // juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;
}

void AudioEngine::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

        juce::MidiBuffer midi;

        if (playing.load())
        {
            // Copy notes safely (never hold a lock during audio rendering)
            std::vector<Note> localNotes;
            {
                const std::scoped_lock lock (noteMutex);
                localNotes = notes;
            }

            auto sr = currentSampleRate;
            auto blockSamples = bufferToFill.numSamples;
            auto secondsPerBeat = 60.0 / bpm.load();

            auto blockSeconds = (double) blockSamples / sr;
            auto blockBeats = blockSeconds / secondsPerBeat;

            auto blockStartBeat = playheadBeat;
            auto blockEndBeat = playheadBeat + blockBeats;

            for (const auto& n : localNotes)
            {
                auto noteOnBeat  = n.startBeat;
                auto noteOffBeat = n.startBeat + n.lengthBeats;

                // If note-on happens within this block
                if (noteOnBeat >= blockStartBeat && noteOnBeat < blockEndBeat)
                {
                    auto t = (noteOnBeat - blockStartBeat) / blockBeats;
                    int sampleOffset = (int) std::round (t * blockSamples);

                    midi.addEvent (juce::MidiMessage::noteOn (1, n.midiNote, (juce::uint8) 100),
                                   sampleOffset);
                }

                // If note-off happens within this block
                if (noteOffBeat >= blockStartBeat && noteOffBeat < blockEndBeat)
                {
                    auto t = (noteOffBeat - blockStartBeat) / blockBeats;
                    int sampleOffset = (int) std::round (t * blockSamples);

                    midi.addEvent (juce::MidiMessage::noteOff (1, n.midiNote),
                                   sampleOffset);
                }
            }

            playheadBeat += blockBeats;

            // loop after 4 bars for now
            if (playheadBeat >= 16.0)
                playheadBeat = 0.0;
        }

        synth.renderNextBlock (*bufferToFill.buffer,
                               midi,
                               bufferToFill.startSample,
                               bufferToFill.numSamples);
}

void AudioEngine::releaseResources()
{
}

void AudioEngine::setNotes (std::vector<Note> newNotes)
{
    const std::scoped_lock lock (noteMutex);
    notes = std::move (newNotes);
}
