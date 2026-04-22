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
    auto setupSynth = [] (juce::Synthesiser& synth, InstrumentType type)
        {
            for (int i = 0; i < 8; ++i)
            {
                auto* voice = new SynthVoice();
                voice->setInstrumentType (type);
                voice->setPlaybackMode (InstrumentPlaybackMode::Oscillator);
                synth.addVoice (voice);
            }

            synth.addSound (new SynthSound());
        };

        setupSynth (sineSynth, InstrumentType::Sine);
        setupSynth (squareSynth, InstrumentType::Square);
        setupSynth (sawSynth, InstrumentType::Saw);
        setupSynth (triangleSynth, InstrumentType::Triangle);
}
AudioEngine::~AudioEngine() {}

/*void AudioEngine::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

       for (int i = 0; i < synth.getNumVoices(); ++i)
           if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
               voice->prepareToPlay (sampleRate, samplesPerBlock, 2);
   // juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;
    prepareSynth (sineSynth, sampleRate, samplesPerBlock, 2);
    prepareSynth (squareSynth, sampleRate, samplesPerBlock, 2);
    prepareSynth (sawSynth, sampleRate, samplesPerBlock, 2);
    prepareSynth (triangleSynth, sampleRate, samplesPerBlock, 2);
    
    for (auto& [id, synth] : sampleSynths)
        prepareSynth (*synth, sampleRate, samplesPerBlock, 2);
    
    rebuildSampleSynths();
}*/
void AudioEngine::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    prepareSynth (sineSynth, sampleRate, samplesPerBlock, 2);
    prepareSynth (squareSynth, sampleRate, samplesPerBlock, 2);
    prepareSynth (sawSynth, sampleRate, samplesPerBlock, 2);
    prepareSynth (triangleSynth, sampleRate, samplesPerBlock, 2);
    
    rebuildSampleSynths();
}

void AudioEngine::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    
    if (playbackMode == PlaybackMode::Piano)
    {
        juce::MidiBuffer sineMidi;
        juce::MidiBuffer squareMidi;
        juce::MidiBuffer sawMidi;
        juce::MidiBuffer triangleMidi;

        std::unordered_map<std::string, juce::MidiBuffer> sampleMidiBuffers;

        if (playing.load())
        {
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

                auto addEventToBuffer = [&] (juce::MidiBuffer& midiBuffer, bool isNoteOn, int midiNote, int sampleOffset)
                {
                    if (isNoteOn)
                        midiBuffer.addEvent (juce::MidiMessage::noteOn (1, midiNote, (juce::uint8) 100), sampleOffset);
                    else
                        midiBuffer.addEvent (juce::MidiMessage::noteOff (1, midiNote), sampleOffset);
                };

                if (n.playbackMode == InstrumentPlaybackMode::Oscillator)
                {
                    juce::MidiBuffer* targetMidi = nullptr;

                    switch (n.instrument)
                    {
                        case InstrumentType::Sine:     targetMidi = &sineMidi; break;
                        case InstrumentType::Square:   targetMidi = &squareMidi; break;
                        case InstrumentType::Saw:      targetMidi = &sawMidi; break;
                        case InstrumentType::Triangle: targetMidi = &triangleMidi; break;
                    }

                    if (targetMidi == nullptr)
                        continue;

                    if (noteOnBeat >= blockStartBeat && noteOnBeat < blockEndBeat)
                    {
                        auto t = (noteOnBeat - blockStartBeat) / blockBeats;
                        int sampleOffset = (int) std::round (t * blockSamples);
                        addEventToBuffer (*targetMidi, true, n.midiNote, sampleOffset);
                    }

                    if (noteOffBeat >= blockStartBeat && noteOffBeat < blockEndBeat)
                    {
                        auto t = (noteOffBeat - blockStartBeat) / blockBeats;
                        int sampleOffset = (int) std::round (t * blockSamples);
                        addEventToBuffer (*targetMidi, false, n.midiNote, sampleOffset);
                    }
                }
                else if (n.playbackMode == InstrumentPlaybackMode::Sample
                         && n.userInstrumentId.isNotEmpty())
                {
                    auto& midiBuffer = sampleMidiBuffers[n.userInstrumentId.toStdString()];

                    if (noteOnBeat >= blockStartBeat && noteOnBeat < blockEndBeat)
                    {
                        auto t = (noteOnBeat - blockStartBeat) / blockBeats;
                        int sampleOffset = (int) std::round (t * blockSamples);
                        addEventToBuffer (midiBuffer, true, n.midiNote, sampleOffset);
                    }

                    if (noteOffBeat >= blockStartBeat && noteOffBeat < blockEndBeat)
                    {
                        auto t = (noteOffBeat - blockStartBeat) / blockBeats;
                        int sampleOffset = (int) std::round (t * blockSamples);
                        addEventToBuffer (midiBuffer, false, n.midiNote, sampleOffset);
                    }
                }
            }

            playheadBeat += blockBeats;

            auto loopBeats = numBars.load() * 4.0;
            if (playheadBeat >= loopBeats)
                playheadBeat = 0.0;
        }

        sineSynth.renderNextBlock (*bufferToFill.buffer, sineMidi, bufferToFill.startSample, bufferToFill.numSamples);
        squareSynth.renderNextBlock (*bufferToFill.buffer, squareMidi, bufferToFill.startSample, bufferToFill.numSamples);
        sawSynth.renderNextBlock (*bufferToFill.buffer, sawMidi, bufferToFill.startSample, bufferToFill.numSamples);
        triangleSynth.renderNextBlock (*bufferToFill.buffer, triangleMidi, bufferToFill.startSample, bufferToFill.numSamples);

        for (auto& [instrumentId, midiBuffer] : sampleMidiBuffers)
        {
            auto it = sampleSynths.find (instrumentId);
            if (it != sampleSynths.end() && it->second != nullptr)
                it->second->renderNextBlock (*bufferToFill.buffer, midiBuffer, bufferToFill.startSample, bufferToFill.numSamples);
        }
    }
    
    if (playbackMode == PlaybackMode::Arrangement)
    {
        if (playing.load())
        {
            std::vector<AudioTrack> localTracks;
            {
                const std::scoped_lock lock (arrangementMutex);
                localTracks = arrangementTracks;
            }
            
            bool anySolo = false;
            for (const auto& track : localTracks)
            {
                if (track.solo)
                {
                    anySolo = true;
                    break;
                }
            }

            auto sr = currentSampleRate;
            auto blockSamples = bufferToFill.numSamples;
            auto blockDurationSeconds = (double) blockSamples / sr;

            auto blockStartTime = arrangementPlayheadSeconds;
            auto blockEndTime   = blockStartTime + blockDurationSeconds;

            for (const auto& track : localTracks)
            {
                if (anySolo)
                    {
                        if (! track.solo)
                            continue;
                    }
                    else
                    {
                        if (track.muted)
                            continue;
                    }
                
                for (const auto& clip : track.clips)
                {
                    if (clip.audioData == nullptr)
                        continue;

                    auto clipStartTime = clip.startTimeSeconds;
                    auto clipEndTime   = clip.startTimeSeconds + clip.lengthSeconds;

                    // no overlap
                    if (clipEndTime <= blockStartTime || clipStartTime >= blockEndTime)
                        continue;

                    auto* clipBuffer = clip.audioData.get();
                    int clipNumChannels = clipBuffer->getNumChannels();
                    int clipNumSamples  = clipBuffer->getNumSamples();

                    double overlapStartTime = juce::jmax (blockStartTime, clipStartTime);
                    double overlapEndTime   = juce::jmin (blockEndTime, clipEndTime);

                    double clipOffsetSeconds = clip.sourceOffsetSeconds + (overlapStartTime - clipStartTime);
                    double blockOffsetSeconds  = overlapStartTime - blockStartTime;
                    double overlapDurationSecs = overlapEndTime - overlapStartTime;

                    int sourceStartSample = (int) std::round (clipOffsetSeconds * clip.sourceSampleRate);
                    int destStartSample   = bufferToFill.startSample
                                          + (int) std::round (blockOffsetSeconds * sr);
                    int samplesToCopy     = (int) std::round (overlapDurationSecs * sr);

                    sourceStartSample = juce::jlimit (0, clipNumSamples - 1, sourceStartSample);

                    int maxDestSamples = bufferToFill.buffer->getNumSamples() - destStartSample;
                    int maxSourceSamples = clipNumSamples - sourceStartSample;
                    samplesToCopy = juce::jlimit (0,
                                                  juce::jmin (maxSourceSamples, maxDestSamples),
                                                  samplesToCopy);

                    if (samplesToCopy <= 0)
                        continue;

                    for (int destChannel = 0; destChannel < bufferToFill.buffer->getNumChannels(); ++destChannel)
                    {
                        int sourceChannel = juce::jmin (destChannel, clipNumChannels - 1);

                        bufferToFill.buffer->addFrom (destChannel,
                                                      destStartSample,
                                                      *clipBuffer,
                                                      sourceChannel,
                                                      sourceStartSample,
                                                      samplesToCopy,
                                                      1.0f);
                    }
                }
            }

            arrangementPlayheadSeconds += blockDurationSeconds;

            double arrangementEndTime = getArrangementEndTimeSeconds (localTracks);

            // Add a small tail so the playhead can pass the end a little
            double loopLengthSeconds = juce::jmax (1.0, arrangementEndTime + 0.25);

            if (arrangementPlayheadSeconds >= loopLengthSeconds)
                arrangementPlayheadSeconds = 0.0;
        }
    }
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
    sineSynth.allNotesOff (0, false);
    squareSynth.allNotesOff (0, false);
    sawSynth.allNotesOff (0, false);
    triangleSynth.allNotesOff (0, false);
    for (auto& [id, synth] : sampleSynths)
        synth->allNotesOff (0, false);
    
    playing.store(false);
    playheadBeat = 0.0;
    arrangementPlayheadSeconds = 0.0;
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
    
    double maxEndBeat = 0.0;

    for (const auto& n : localNotes)
        maxEndBeat = juce::jmax (maxEndBeat, n.startBeat + n.lengthBeats);
    
    const double bpmLocal = bpm.load();
    
    if (bpmLocal <= 0.0)
        return false;

    const double tailBeats = 1.0; //adds 1 beat of release
    if (lengthBeats <= 0.0)
        lengthBeats = maxEndBeat + tailBeats;

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

void AudioEngine::setArrangementTracks (std::vector<AudioTrack> newTracks)
{
    const std::scoped_lock lock (arrangementMutex);
    arrangementTracks = std::move (newTracks);
}

double AudioEngine::getArrangementEndTimeSeconds (const std::vector<AudioTrack>& tracks) const
{
    double maxEndTime = 0.0;

    for (const auto& track : tracks)
    {
        if (track.type == TrackType::Video)
        {
            for (const auto& clip : track.videoClips)
                maxEndTime = juce::jmax (maxEndTime, clip.startTimeSeconds + clip.lengthSeconds);
        }
        else
        {
            for (const auto& clip : track.clips)
                maxEndTime = juce::jmax (maxEndTime, clip.startTimeSeconds + clip.lengthSeconds);
        }
    }

    return maxEndTime;
}

void AudioEngine::setCurrentInstrument (InstrumentType type)
{
    currentInstrument = type;

        auto applyToSynth = [type] (juce::Synthesiser& synth)
        {
            for (int i = 0; i < synth.getNumVoices(); ++i)
            {
                if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
                    voice->setInstrumentType (type);
            }
        };

        applyToSynth (sineSynth);
        applyToSynth (squareSynth);
        applyToSynth (sawSynth);
        applyToSynth (triangleSynth);
}

juce::Synthesiser& AudioEngine::getSynthForInstrument (InstrumentType type)
{
    switch (type)
    {
        case InstrumentType::Sine:     return sineSynth;
        case InstrumentType::Square:   return squareSynth;
        case InstrumentType::Saw:      return sawSynth;
        case InstrumentType::Triangle: return triangleSynth;
    }

    return sineSynth;
}

void AudioEngine::prepareSynth (juce::Synthesiser& synthToPrepare,
                                double sampleRate,
                                int samplesPerBlock,
                                int numChannels)
{
    synthToPrepare.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synthToPrepare.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*> (synthToPrepare.getVoice (i)))
            voice->prepareToPlay (sampleRate, samplesPerBlock, numChannels);
    }
}

void AudioEngine::setPlaybackModeForVoices (InstrumentPlaybackMode mode)
{
    currentPlaybackMode = mode;

    auto applyToSynth = [mode] (juce::Synthesiser& synth)
    {
        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
                voice->setPlaybackMode (mode);
        }
    };

    applyToSynth (sineSynth);
    applyToSynth (squareSynth);
    applyToSynth (sawSynth);
    applyToSynth (triangleSynth);
}

bool AudioEngine::loadSampleInstrumentFromFile (const juce::File& file, int rootMidiNote)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr)
        return false;

    const double sourceSampleRate = reader->sampleRate;
    const int numChannels = (int) reader->numChannels;
    const int sourceNumSamples = (int) reader->lengthInSamples;

    if (numChannels <= 0 || sourceNumSamples <= 0 || currentSampleRate <= 0.0)
        return false;

    juce::AudioBuffer<float> sourceBuffer;
    sourceBuffer.setSize (numChannels, sourceNumSamples);
    reader->read (&sourceBuffer, 0, sourceNumSamples, 0, true, true);

    const double ratio = currentSampleRate / sourceSampleRate;
    const int resampledNumSamples = (int) std::ceil ((double) sourceNumSamples * ratio);

    auto resampledBuffer = std::make_shared<juce::AudioBuffer<float>>();
    resampledBuffer->setSize (numChannels, resampledNumSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = sourceBuffer.getReadPointer (ch);
        float* dst = resampledBuffer->getWritePointer (ch);

        for (int i = 0; i < resampledNumSamples; ++i)
        {
            const double srcPos = (double) i / ratio;
            const int i0 = juce::jlimit (0, sourceNumSamples - 1, (int) std::floor (srcPos));
            const int i1 = juce::jlimit (0, sourceNumSamples - 1, i0 + 1);

            const float frac = (float) (srcPos - (double) i0);
            const float s0 = src[i0];
            const float s1 = src[i1];

            dst[i] = s0 + frac * (s1 - s0);
        }
    }

    currentSampleInstrument = resampledBuffer;
    currentSampleInstrumentRate = currentSampleRate;
    currentSampleRootMidiNote = rootMidiNote;

    auto applySampleToSynth = [this] (juce::Synthesiser& synth)
    {
        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
            {
                voice->setSampleData (currentSampleInstrument,
                                      currentSampleInstrumentRate,
                                      currentSampleRootMidiNote);
            }
        }
    };

    applySampleToSynth (sineSynth);
    applySampleToSynth (squareSynth);
    applySampleToSynth (sawSynth);
    applySampleToSynth (triangleSynth);

    setPlaybackModeForVoices (InstrumentPlaybackMode::Sample);

    DBG ("Loaded sample instrument: " << file.getFileName()
         << " | root note: " << rootMidiNote
         << " | resampled SR: " << currentSampleRate);

    return true;
}

bool AudioEngine::loadSampleIntoSynth (juce::Synthesiser& synth,
                                       const juce::File& file,
                                       int rootMidiNote)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr || currentSampleRate <= 0.0)
        return false;

    const double sourceSampleRate = reader->sampleRate;
    const int numChannels = (int) reader->numChannels;
    const int sourceNumSamples = (int) reader->lengthInSamples;

    if (numChannels <= 0 || sourceNumSamples <= 0)
        return false;

    juce::AudioBuffer<float> sourceBuffer;
    sourceBuffer.setSize (numChannels, sourceNumSamples);
    reader->read (&sourceBuffer, 0, sourceNumSamples, 0, true, true);

    const double ratio = currentSampleRate / sourceSampleRate;
    const int resampledNumSamples = (int) std::ceil ((double) sourceNumSamples * ratio);

    auto resampledBuffer = std::make_shared<juce::AudioBuffer<float>>();
    resampledBuffer->setSize (numChannels, resampledNumSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = sourceBuffer.getReadPointer (ch);
        float* dst = resampledBuffer->getWritePointer (ch);

        for (int i = 0; i < resampledNumSamples; ++i)
        {
            const double srcPos = (double) i / ratio;
            const int i0 = juce::jlimit (0, sourceNumSamples - 1, (int) std::floor (srcPos));
            const int i1 = juce::jlimit (0, sourceNumSamples - 1, i0 + 1);

            const float frac = (float) (srcPos - (double) i0);
            const float s0 = src[i0];
            const float s1 = src[i1];

            dst[i] = s0 + frac * (s1 - s0);
        }
    }

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
        {
            voice->setPlaybackMode (InstrumentPlaybackMode::Sample);
            voice->setSampleData (resampledBuffer, currentSampleRate, rootMidiNote);
        }
    }

    return true;
}

void AudioEngine::setUserInstruments (const std::vector<UserInstrument>& instruments)
{
    userInstrumentMap.clear();

        for (const auto& instrument : instruments)
            userInstrumentMap[instrument.id.toStdString()] = instrument;

        rebuildSampleSynths();
}

void AudioEngine::rebuildSampleSynths()
{
    sampleSynths.clear();

    if (currentSampleRate <= 0.0)
    {
        DBG ("rebuildSampleSynths skipped: currentSampleRate not ready");
        return;
    }

    for (const auto& [id, instrument] : userInstrumentMap)
    {
        if (instrument.playbackMode != InstrumentPlaybackMode::Sample)
            continue;

        auto synth = std::make_unique<juce::Synthesiser>();

        for (int i = 0; i < 8; ++i)
        {
            auto* voice = new SynthVoice();
            voice->setPlaybackMode (InstrumentPlaybackMode::Sample);
            synth->addVoice (voice);
        }

        synth->addSound (new SynthSound());

        prepareSynth (*synth, currentSampleRate, 512, 2);

        if (instrument.sampleFile.existsAsFile())
        {
            bool ok = loadSampleIntoSynth (*synth, instrument.sampleFile, instrument.rootMidiNote);
            DBG ("Loaded sample synth id=" << id
                 << " file=" << instrument.sampleFile.getFileName()
                 << " ok=" << (ok ? "yes" : "no"));
        }
        else
        {
            DBG ("Missing sample file for instrument id=" << id
                 << " path=" << instrument.sampleFile.getFullPathName());
        }

        sampleSynths[id] = std::move (synth);
    }

    DBG ("rebuildSampleSynths done, count=" << (int) sampleSynths.size());
}
