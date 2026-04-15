#pragma once

#include <JuceHeader.h>
#include "audio/AudioEngine.h"
#include "ui/PianoRoll.h"
#include "ui/TransportComponent.h"
#include "audio/AudioRecorder.h"
#include "ui/ArrangementView.h"
#include "ui/SampleBrowser.h"
#include "ui/InstrumentBrowser.h"
#include "InstrumentLibrary.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent , private juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void loadSampleLibrary();
    void importWavToLibrary (const juce::File& sourceFile);
    
    void updateArrangementVideoPlayback();
    void importVideoToTrack(const juce::File& file);
   

private:
    //==============================================================================
    // Your private member variables go here...
    AudioEngine audioEngine;
    PianoRoll pianoRoll;
    TransportComponent transport;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Viewport pianoViewport;
    AudioRecorder recorder;
    SampleBrowser sampleBrowser;
    InstrumentBrowser instrumentBrowser;
    InstrumentLibrary instrumentLibrary;
    
    juce::File getSampleLibraryFolder() const;
    
    juce::TextButton arrangementButton { "Arrangement" };
    juce::TextButton pianoToolButton { "Piano Tool" };

    juce::VideoComponent videoPlayer {true};
    juce::TextButton importVideoButton { "Import Video" };
    
    ArrangementView arrangementView;
    juce::Viewport arrangementViewport;
    
    juce::TextButton addTrackButton { "+ Track" };
    juce::TextButton removeTrackButton { "- Track" };
    
    juce::File currentLoadedVideoFile;
    
    juce::File pendingVideoFile;
    bool waitingForVideoDuration = false;
    
    int videoDurationPollCount = 0;
    
    juce::TextButton loadSampleInstrumentButton { "Load Sample Instr" };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
