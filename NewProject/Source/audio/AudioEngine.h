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

class AudioEngine
{
public:
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

private:
    double currentSampleRate { 44100.0 };
    std::atomic<int> numBars { 4 };
    juce::Synthesiser synth;
           
    std::mutex noteMutex;
    std::vector<Note> notes;

    std::atomic<bool> playing { false };
    std::atomic<double> bpm { 120.0 };

    double playheadBeat = 0.0;
};
    


