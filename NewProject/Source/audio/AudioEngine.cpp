/*
  ==============================================================================

    AudioEngine.cpp
    Created: 10 Feb 2026 9:47:11am
    Author:  Mark Hilton

  ==============================================================================
*/

#include "AudioEngine.h"
#include "../ui/PianoRoll.h"
#include <JuceHeader.h>

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
            
            auto loopBeats = numBars.load() * 4.0;
            // loop after 4 bars for now
            if (playheadBeat >= loopBeats)
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

void AudioEngine::play()
{
    playing.store(true);
}

void AudioEngine::stop()
{
    playing.store(false);
    playheadBeat = 0.0;
    
    panic();
}

void AudioEngine::panic()
{
    // Immediately stop any sound, no tail
    synth.allNotesOff (0, false);   // channel=0 means all channels in JUCE Synthesiser
    //synth.handleController (1, 64, 0); // sustain off if I add sustain pedals later on
}

bool AudioEngine::exportWav (const juce::File& outFile,
                             double lengthBeats,
                             int numChannels,
                             int sampleRate)
{
    // Snapshot notes safely
    std::vector<Note> localNotes;
    {
        const std::scoped_lock lock (noteMutex);
        localNotes = notes;
    }

    const double bpmLocal = bpm.load();
    if (bpmLocal <= 0.0 || lengthBeats <= 0.0)
        return false;

    const double secondsPerBeat = 60.0 / bpmLocal;
    const double totalSeconds = lengthBeats * secondsPerBeat;
    const int totalSamples = (int) std::ceil (totalSeconds * (double) sampleRate);

    // Set up a temporary synth for offline render (clean state)
    juce::Synthesiser offlineSynth;
    for (int i = 0; i < 8; ++i)
        offlineSynth.addVoice (new SynthVoice());
    offlineSynth.addSound (new SynthSound());
    offlineSynth.setCurrentPlaybackSampleRate ((double) sampleRate);

    // Prepare voices (your SynthVoice has prepareToPlay)
    for (int i = 0; i < offlineSynth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*> (offlineSynth.getVoice (i)))
            voice->prepareToPlay ((double) sampleRate, 512, numChannels);

    // Build a MidiMessageSequence for the entire render
    juce::MidiMessageSequence seq;
    for (const auto& n : localNotes)
    {
        const double startSec = n.startBeat * secondsPerBeat;
        const double endSec   = (n.startBeat + n.lengthBeats) * secondsPerBeat;

        seq.addEvent (juce::MidiMessage::noteOn  (1, n.midiNote, (juce::uint8) 100), startSec);
        seq.addEvent (juce::MidiMessage::noteOff (1, n.midiNote), endSec);
    }
    seq.updateMatchedPairs();

    // Render in blocks
    juce::AudioBuffer<float> renderBuffer (numChannels, totalSamples);
    renderBuffer.clear();

    int pos = 0;
    const int blockSize = 512;

    while (pos < totalSamples)
    {
        const int num = juce::jmin (blockSize, totalSamples - pos);

        juce::AudioBuffer<float> block (numChannels, num);
        block.clear();

        // Convert sequence events in [pos, pos+num) seconds into a MidiBuffer
        juce::MidiBuffer midi;
        const double blockStartSec = (double) pos / (double) sampleRate;
        const double blockEndSec   = (double) (pos + num) / (double) sampleRate;

        for (int i = 0; i < seq.getNumEvents(); ++i)
        {
            auto* ev = seq.getEventPointer(i);
            const double t = ev->message.getTimeStamp();

            if (t >= blockStartSec && t < blockEndSec)
            {
                const int sampleOffset = (int) std::round ((t - blockStartSec) * (double) sampleRate);
                midi.addEvent (ev->message, juce::jlimit (0, num - 1, sampleOffset));
            }
        }

        offlineSynth.renderNextBlock (block, midi, 0, num);

        // Copy block into renderBuffer
        for (int ch = 0; ch < numChannels; ++ch)
            renderBuffer.copyFrom (ch, pos, block, ch, 0, num);

        pos += num;
    }

    // Write WAV
    juce::WavAudioFormat wav;
    // IMPORTANT: this must be a std::unique_ptr<juce::OutputStream>
    std::unique_ptr<juce::OutputStream> stream (outFile.createOutputStream());

    if (! stream)
        return false;

    // AudioFormatWriterOptions is immutable: use withX() methods
    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate ((double) sampleRate)
                     .withNumChannels ((int) numChannels)
                     .withBitsPerSample (16);

    // New JUCE 8 API: passes ownership of 'stream' to the writer if successful
    DBG("Saving to: " << outFile.getFullPathName());
    auto writer = wav.createWriterFor (stream, options);

    if (! writer)
        return false;

    // stream should now be nullptr because the writer took ownership
    // (if writer creation failed, stream would remain non-null)

    return writer->writeFromAudioSampleBuffer (renderBuffer, 0, totalSamples);
}
