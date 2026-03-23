/*
  ==============================================================================

    AudioRecorder.cpp
    Created: 11 Mar 2026 10:36:07am
    Author:  Mark Hilton

  ==============================================================================
*/

#include "AudioRecorder.h"

AudioRecorder::AudioRecorder()
{
    backgroundThread.startThread();
}

AudioRecorder::~AudioRecorder()
{
    stop();
}

void AudioRecorder::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        currentSampleRate = device->getCurrentSampleRate();
        currentNumChannels = juce::jmax (1, device->getActiveInputChannels().countNumberOfSetBits());
        DBG ("Recorder started");
                DBG ("Device name: " << device->getName());
                DBG ("Sample rate: " << currentSampleRate);
                DBG ("Active input channels: " << device->getActiveInputChannels().toString (2));
    }
}

void AudioRecorder::audioDeviceStopped()
{
    currentSampleRate = 0.0;
    currentNumChannels = 0;
}

/*void AudioRecorder::startRecording (const juce::File& file, double sampleRate, int numChannels)
{
    stop();

    if (sampleRate <= 0.0 || numChannels <= 0)
        return;

    file.deleteFile();

    auto fileStream = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream());

    if (fileStream == nullptr)
        return;

    juce::WavAudioFormat wavFormat;

    auto writer = wavFormat.createWriterFor (fileStream.get(),
                                             sampleRate,
                                             (unsigned int) numChannels,
                                             16,
                                             {},
                                             0);

    if (writer == nullptr)
        return;

    fileStream.release();

    auto newThreadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (
        writer, backgroundThread, 32768);

    {
        const juce::ScopedLock sl (writerLock);
        threadedWriter = std::move (newThreadedWriter);
        activeWriter.store (threadedWriter.get());
    }

    currentSampleRate = sampleRate;
    currentNumChannels = numChannels;
}*/
void AudioRecorder::startRecording (const juce::File& file, double sampleRate, int numChannels)
{
    DBG ("startRecording called");
    DBG ("Target file: " << file.getFullPathName());
    DBG ("Sample rate: " << sampleRate);
    DBG ("Num channels: " << numChannels);

    stop();

    if (sampleRate <= 0.0 || numChannels <= 0)
    {
        DBG ("startRecording failed: invalid sample rate or channel count");
        return;
    }

    file.deleteFile();

    auto fileStream = std::unique_ptr<juce::FileOutputStream> (file.createOutputStream());

    if (fileStream == nullptr)
    {
        DBG ("startRecording failed: createOutputStream returned null");
        return;
    }

    DBG ("Output stream created");

    juce::WavAudioFormat wavFormat;

    auto* writer = wavFormat.createWriterFor (fileStream.get(),
                                              sampleRate,
                                              (unsigned int) numChannels,
                                              16,
                                              {},
                                              0);

    if (writer == nullptr)
    {
        DBG ("startRecording failed: createWriterFor returned null");
        return;
    }

    DBG ("Audio writer created");

    fileStream.release();

    auto newThreadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (
        writer, backgroundThread, 32768);

    {
        const juce::ScopedLock sl (writerLock);
        threadedWriter = std::move (newThreadedWriter);
        activeWriter.store (threadedWriter.get());
    }

    DBG ("Threaded writer created and activated");

    currentSampleRate = sampleRate;
    currentNumChannels = numChannels;
}

void AudioRecorder::stop()
{
    activeWriter.store (nullptr);

    const juce::ScopedLock sl (writerLock);
    threadedWriter.reset();
}

void AudioRecorder::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                      int numInputChannels,
                                                      float* const* outputChannelData,
                                                      int numOutputChannels,
                                                      int numSamples,
                                                      const juce::AudioIODeviceCallbackContext&)
{
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear (outputChannelData[ch], numSamples);

    auto* writer = activeWriter.load();

    if (writer == nullptr)
        return;

    if (numInputChannels <= 0 || inputChannelData == nullptr || inputChannelData[0] == nullptr)
        return;

    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = juce::jmax (peak, std::abs (inputChannelData[0][i]));

    DBG ("Recorder: numInputChannels = " << numInputChannels
         << ", numSamples = " << numSamples
         << ", mic peak = " << peak
         << ", s0 = " << inputChannelData[0][0]
         << ", s1 = " << inputChannelData[0][1]
         << ", s2 = " << inputChannelData[0][2]);

    writer->write (inputChannelData, numSamples);
}
