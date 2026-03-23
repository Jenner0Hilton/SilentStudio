/*
  ==============================================================================

    AudioRecorder.h
    Created: 11 Mar 2026 10:36:07am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class AudioRecorder : public juce::AudioIODeviceCallback
{
public:
    AudioRecorder();
    ~AudioRecorder() override;

    void startRecording (const juce::File& file, double sampleRate, int numChannels);
    void stop();

    bool isRecording() const { return activeWriter.load() != nullptr; }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    juce::TimeSliceThread backgroundThread { "Audio Recorder Thread" };

    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;

    juce::CriticalSection writerLock;
    double currentSampleRate = 44100.0;
    int currentNumChannels = 1;
};
