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
#include "ProjectManager.h"

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
    
    void saveProjectAs (const juce::String& projectName);
    void loadProject (const juce::String& projectName);

    juce::var noteToVar (const Note& note) const;
    juce::var audioClipToVar (const AudioClip& clip) const;
    juce::var videoClipToVar (const VideoClip& clip) const;
    juce::var trackToVar (const AudioTrack& track) const;

    bool loadNoteFromVar (const juce::var& v, Note& outNote) const;
    bool loadAudioClipFromVar (const juce::var& v, AudioClip& outClip) const;
    bool loadVideoClipFromVar (const juce::var& v, VideoClip& outClip) const;
    bool loadTrackFromVar (const juce::var& v, AudioTrack& outTrack) const;
   

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
    ProjectManager projectManager;
    juce::String currentProjectName = "Untitled";
    
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
    
    juce::TextButton saveProjectButton{"Save Project"};
    juce::TextButton loadProjectButton{"Load Project"};
    
    juce::TextButton clipEditModeButton { "Mode: Trim" };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
