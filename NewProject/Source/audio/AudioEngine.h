/*
  ==============================================================================

    AudioEngine.h
    Created: 10 Feb 2026 9:47:42am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../instruments/SynthVoice.h"
#include "../instruments/SynthSound.h"

#include <atomic>
#include <mutex>
//#include "../ui/PianoRoll.h"
#include "../Note.h"
#include "AudioTrack.h"
#include "../InstrumentType.h"
#include "../InstrumentPlaybackMode.h"
#include "../UserInstrument.h"
#include <memory>
#include <unordered_map>

class AudioEngine
{
public:
    enum class PlaybackMode
    {
        Piano,
        Arrangement
    };
    AudioEngine();
    ~AudioEngine();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();
    
    void setNotes (std::vector<Note> newNotes);  // called by UI thread
    void setPlaying (bool shouldPlay) { playing.store(shouldPlay); }
    //void setBpm (double newBpm) { bpm.store(newBpm); }
    
    void play();
    void stop();
    void panic();
    void setBpm(double newBpm) { bpm.store(newBpm); }
    double getPlayheadBeat() const { return playheadBeat; }
    double getBpm() const { return bpm.load(); }
    
    bool exportWav (const juce::File& outFile,
                       double lengthBeats,
                       int numChannels = 2,
                       int sampleRate = 44100);
    
    void setNumBars (int newNumBars) { numBars.store (juce::jmax (1, newNumBars)); }
       int getNumBars() const { return numBars.load(); }
    void setArrangementTracks (std::vector<AudioTrack> newTracks);
    
    void setPlaybackMode (PlaybackMode newMode) { playbackMode = newMode; }
    
    double getArrangementPlayheadSeconds() const { return arrangementPlayheadSeconds; }
    
    double getCurrentSampleRate() const { return currentSampleRate; }
    
    void setCurrentInstrument (InstrumentType type);
    
    //juce::Synthesiser& getSynthForInstrument (InstrumentType type);
   // void prepareSynth (juce::Synthesiser& synth, double sampleRate, int samplesPerBlock, int numChannels);
    
    bool loadSampleInstrumentFromFile (const juce::File& file, int rootMidiNote = 60);
    void setPlaybackModeForVoices (InstrumentPlaybackMode mode);
    
    void setUserInstruments (const std::vector<UserInstrument>& instruments);
    void rebuildSampleSynths();

private:
    double currentSampleRate { 44100.0 };
    int currentSampleRootMidiNote = 60;
    std::atomic<int> numBars { 4 };
    juce::Synthesiser synth;
    juce::Synthesiser sineSynth;
    juce::Synthesiser squareSynth;
    juce::Synthesiser sawSynth;
    juce::Synthesiser triangleSynth;
           
    std::mutex noteMutex;
    std::vector<Note> notes;

    std::atomic<bool> playing { false };
    std::atomic<double> bpm { 120.0 };

    double playheadBeat = 0.0;
    double arrangementPlayheadSeconds = 0.0;
    
    double getArrangementEndTimeSeconds (const std::vector<AudioTrack>& tracks) const;
    
    std::mutex arrangementMutex;
    std::vector<AudioTrack> arrangementTracks;
    PlaybackMode playbackMode { PlaybackMode::Piano };
    
    InstrumentType currentInstrument = InstrumentType::Sine;
    InstrumentPlaybackMode currentPlaybackMode = InstrumentPlaybackMode::Oscillator;
    std::shared_ptr<juce::AudioBuffer<float>> currentSampleInstrument;
    double currentSampleInstrumentRate = 44100.0;
    
    std::unordered_map<std::string, std::unique_ptr<juce::Synthesiser>> sampleSynths;
    std::unordered_map<std::string, UserInstrument> userInstrumentMap;
    
    juce::Synthesiser& getSynthForInstrument (InstrumentType type);
    void prepareSynth (juce::Synthesiser& synth, double sampleRate, int samplesPerBlock, int numChannels);
    bool loadSampleIntoSynth (juce::Synthesiser& synth, const juce::File& file, int rootMidiNote);
};
    


