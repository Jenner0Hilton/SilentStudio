#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1200, 900);
    DBG ("recordAudio required? " << (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio) ? "yes" : "no"));
    DBG ("recordAudio granted? " << (juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio) ? "yes" : "no"));

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 1 : 0, 2);
                                        deviceManager.addAudioCallback (&recorder);
        });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (1, 2); // 1 input channel two output channels
        deviceManager.addAudioCallback (&recorder);
    }
    
    addAndMakeVisible (loadSampleInstrumentButton);
    loadSampleInstrumentButton.setVisible (true);

    loadSampleInstrumentButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Create Sample Instrument",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.wav");

        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            fileChooser.reset();

            if (file == juce::File{})
                return;

            auto instrumentName = file.getFileNameWithoutExtension();

            bool saved = instrumentLibrary.addSampleInstrument (file, instrumentName, 60);

            if (saved)
            {
                instrumentLibrary.load();
                instrumentBrowser.setUserInstruments (instrumentLibrary.getInstruments());
                audioEngine.setUserInstruments (instrumentLibrary.getInstruments());
                DBG ("Saved sample instrument: " << instrumentName);
            }
            else
            {
                DBG ("Failed to save sample instrument");
            }
        });
    };
    
    addAndMakeVisible (instrumentBrowser);
    instrumentBrowser.setVisible (true);
    //instrumentBrowser.setSelectedInstruments (InstrumentType::Sine);
    //pianoRoll.setCurrentInstrument (InstrumentType::Sine);
    //audioEngine.setCurrentInstrument (InstrumentType::Sine);
    instrumentBrowser.onInstrumentChosen = [this](const InstrumentItem& item)
    {
        if (item.kind == BrowserInstrumentKind::BuiltIn)
        {
            pianoRoll.setCurrentPlaybackMode (InstrumentPlaybackMode::Oscillator);
            pianoRoll.setCurrentBuiltInInstrument (item.builtInType);
            pianoRoll.setCurrentUserInstrument ("");
        }
        else if (item.kind == BrowserInstrumentKind::UserSample)
        {
            pianoRoll.setCurrentPlaybackMode (InstrumentPlaybackMode::Sample);
            pianoRoll.setCurrentUserInstrument (item.userInstrument.id);
        }
    };
    
    instrumentLibrary.load();
    instrumentBrowser.setUserInstruments (instrumentLibrary.getInstruments());
    audioEngine.setUserInstruments (instrumentLibrary.getInstruments());
    
    addAndMakeVisible (sampleBrowser);
    sampleBrowser.setVisible (false);
    sampleBrowser.onFileChosen = [this](const juce::File& file)
    {
        arrangementView.addClipToTrack (file,
                                        arrangementView.getLastClickedTrack(),
                                        arrangementView.getLastClickedTimeSeconds(),audioEngine.getCurrentSampleRate());
    };
    loadSampleLibrary();
    
    
    addAndMakeVisible (clipEditModeButton);
    clipEditModeButton.setVisible (false);

    clipEditModeButton.onClick = [this]
    {
        if (arrangementView.getClipEditMode() == ArrangementView::ClipEditMode::Trim)
        {
            arrangementView.setClipEditMode (ArrangementView::ClipEditMode::Stretch);
            clipEditModeButton.setButtonText ("Mode: Stretch");
        }
        else
        {
            arrangementView.setClipEditMode (ArrangementView::ClipEditMode::Trim);
            clipEditModeButton.setButtonText ("Mode: Trim");
        }
    };
    
    
    addAndMakeVisible(transport);
    addAndMakeVisible(pianoViewport);
    
    //swaping shell
    addAndMakeVisible (arrangementButton);
    addAndMakeVisible (pianoToolButton);

    addAndMakeVisible (arrangementViewport);
    addAndMakeVisible (pianoViewport);
    
    addAndMakeVisible (addTrackButton);
    addAndMakeVisible (removeTrackButton);

    addTrackButton.setVisible (false);
    removeTrackButton.setVisible (false);

    addTrackButton.onClick = [this]
    {
        arrangementView.addTrack();
        resized();
    };

    removeTrackButton.onClick = [this]
    {
        arrangementView.removeLastTrack();
        resized();
    };

    arrangementViewport.setViewedComponent (&arrangementView, false);
    pianoViewport.setViewedComponent (&pianoRoll, false);

    arrangementViewport.setVisible (false);
    pianoViewport.setVisible (true);

    arrangementButton.onClick = [this]
    {
        audioEngine.stop();
        arrangementViewport.setVisible (true);
        pianoViewport.setVisible (false);
        audioEngine.setPlaybackMode (AudioEngine::PlaybackMode::Arrangement);
        sampleBrowser.setVisible (true);
        instrumentBrowser.setVisible (false);
        loadSampleInstrumentButton.setVisible (false);
        
        addTrackButton.setVisible(true);
        removeTrackButton.setVisible(true);
        
        videoPlayer.setVisible (true);
        importVideoButton.setVisible (true);
        
        clipEditModeButton.setVisible (true);
        resized();
    };

    pianoToolButton.onClick = [this]
    {
        audioEngine.stop();
        arrangementViewport.setVisible (false);
        pianoViewport.setVisible (true);
        audioEngine.setPlaybackMode (AudioEngine::PlaybackMode::Piano);
        
        addTrackButton.setVisible (false);
        removeTrackButton.setVisible (false);
        
        loadSampleInstrumentButton.setVisible (true);

        instrumentBrowser.setVisible (true);
        sampleBrowser.setVisible (false);
        videoPlayer.setVisible (false);
        importVideoButton.setVisible (false);
        
        clipEditModeButton.setVisible (false);
        resized();
    };
    //end of arrangement for swapping shell
    
    startTimerHz (20); // == 20 times/sec

    transport.onPlay = [this]
    {
        audioEngine.play();
    };

    transport.onStop = [this]
    {
        audioEngine.stop();
        videoPlayer.stop();
        videoPlayer.setPlayPosition(0.0);
    };
    
    transport.onBpmChanged = [this](double newBpm)
    {
        audioEngine.setBpm(newBpm);
        arrangementView.setBpm(newBpm);
    };
    
    transport.setBpmValue(audioEngine.getBpm()); // initializes the BPM
    
    transport.onExport = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Export WAV",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.wav");

        auto flags = juce::FileBrowserComponent::saveMode
                   | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            DBG("Export chooser result: " << file.getFullPathName());

            // release chooser after we’re done
            fileChooser.reset();

            if (file == juce::File{})
            {
                DBG("Export cancelled (no file selected).");
                return;
            }

            auto wavFile = file.withFileExtension(".wav");
            DBG("Saving to: " << wavFile.getFullPathName());

            bool ok = audioEngine.exportWav (wavFile, 0.0);

            DBG(juce::String(ok ? "Export OK" : "Export FAILED" ));

            if (! ok)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Export failed",
                    "Could not export WAV.");
            }
        });
    };
    
    //initializes
    pianoRoll.setNumBars (4);
    audioEngine.setNumBars (4);
    transport.onBarsChanged = [this](int newBars)
    {
        pianoRoll.setNumBars (newBars);
        audioEngine.setNumBars (newBars);

        auto bounds = pianoViewport.getBounds();
        auto totalWidth = (int) (pianoRoll.getNumBars() * pianoRoll.beatsPerBar * pianoRoll.pixelsPerBeat);
        pianoRoll.setSize (totalWidth, bounds.getHeight());
    };
    
    addAndMakeVisible (saveProjectButton);
    addAndMakeVisible (loadProjectButton);
    
    saveProjectButton.onClick = [this]
    {
        auto* alert = new juce::AlertWindow ("Save Project",
                                             "Enter a project name:",
                                             juce::AlertWindow::NoIcon);

        alert->addTextEditor ("projectName", currentProjectName, "Project name:");
        alert->addButton ("Save", 1);
        alert->addButton ("Cancel", 0);

        alert->enterModalState (true,
            juce::ModalCallbackFunction::create ([this, alert] (int result)
            {
                if (result == 1)
                {
                    auto name = alert->getTextEditorContents ("projectName").trim();
                    if (name.isNotEmpty())
                        saveProjectAs (name);
                }
            }),
            true);
    };
    
    loadProjectButton.onClick = [this]
    {
        auto projects = projectManager.listProjects();

        juce::PopupMenu menu;
        for (int i = 0; i < projects.size(); ++i)
            menu.addItem (i + 1, projects[i]);

        menu.showMenuAsync (juce::PopupMenu::Options(),
            [this, projects] (int result)
            {
                if (result > 0 && result <= projects.size())
                    loadProject (projects[result - 1]);
            });
    };
    //arrangementView.setTracks (tracks);
    //audioEngine.setArrangementTracks (tracks);
    
    transport.onRecordToggled = [this](bool shouldRecord)
    {
        if (shouldRecord)
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Save microphone recording",
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                "*.wav");

            auto flags = juce::FileBrowserComponent::saveMode
                       | juce::FileBrowserComponent::canSelectFiles;

            fileChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();

                fileChooser.reset();

                if (file == juce::File{})
                {
                    DBG ("Recording cancelled");
                    return;
                }

                auto wavFile = file.withFileExtension (".wav");

                if (auto* device = deviceManager.getCurrentAudioDevice())
                {
                    auto sampleRate = device->getCurrentSampleRate();
                    auto numInputs = juce::jmax (1, device->getActiveInputChannels().countNumberOfSetBits());

                    recorder.startRecording (wavFile, sampleRate, numInputs);
                    DBG ("Started recording: " << wavFile.getFullPathName());
                }
            });
        }
        else
        {
            recorder.stop();
            DBG ("Stopped recording");
        }
    };
    
    //audio settings
    transport.onAudioSettings = [this]
    {
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = "Audio Settings";
        options.dialogBackgroundColour = juce::Colours::darkgrey;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = true;
        options.componentToCentreAround = this;

        auto* selector = new juce::AudioDeviceSelectorComponent(
            deviceManager,
            0, 2,   // min/max input channels
            0, 2,   // min/max output channels
            false,  // show MIDI input options
            false,  // show MIDI output options
            false,  // show channels as stereo pairs
            false   // hide advanced options button
        );

        selector->setSize (500, 400);
        options.content.setOwned (selector);

        options.launchAsync();
    };
    
    transport.onImportWav = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Import WAV",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.wav");

        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            fileChooser.reset();

            if (file == juce::File{})
                return;
            
            //sampleBrowser.addFile (file);

            importWavToLibrary(file);
            
            arrangementView.addClipToTrack (file, arrangementView.getLastClickedTrack(), arrangementView.getLastClickedBeat(), audioEngine.getCurrentSampleRate());
        });
    };
    
    // video player section
    addAndMakeVisible (videoPlayer);
    addAndMakeVisible (importVideoButton);

    videoPlayer.setVisible (false);
    importVideoButton.setVisible (false);
    
    importVideoButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Import Video",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.mp4;*.mov;*.m4v");

        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            fileChooser.reset();

            if (file == juce::File{})
                return;

            importVideoToTrack (file);
        });
    };
    // end of video player
    
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    deviceManager.removeAudioCallback (&recorder);
    shutdownAudio();
    
    
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    audioEngine.prepareToPlay (sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    /*auto* buffer = bufferToFill.buffer;
    auto numSamples = bufferToFill.numSamples;

    for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
    {
        auto* data = buffer->getWritePointer(channel, bufferToFill.startSample);
        for (int i = 0; i < numSamples; ++i)
            data[i] = 0.1f; // constant tone (DC offset)
    }*/

   // bufferToFill.clearActiveBufferRegion();
    audioEngine.getNextAudioBlock (bufferToFill);
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
    audioEngine.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
   // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll (juce::Colours::darkgrey);
       // g.setColour (juce::Colours::white);
        //g.drawText ("Mini DAW Skeleton", getLocalBounds(),
          //          juce::Justification::centred, true);

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    /*auto area = getLocalBounds();
     
     transport.setBounds (area.removeFromTop (60));
     
     auto topBar = area.removeFromTop (40);
     arrangementButton.setBounds (topBar.removeFromLeft (120).reduced (5));
     pianoToolButton.setBounds (topBar.removeFromLeft (120).reduced (5));
     
     // Remaining area is the editor area
     arrangementView.setBounds (area);
     pianoViewport.setBounds (area);
     arrangementViewport.setBounds (area);
     
     auto pianoWidth = (int) (pianoRoll.getNumBars()
     * pianoRoll.beatsPerBar
     * pianoRoll.pixelsPerBeat);
     
     pianoRoll.setSize (pianoWidth, area.getHeight());
     
     auto arrangementWidth = (int) (arrangementView.getNumBars()
     * arrangementView.beatsPerBar
     * arrangementView.pixelsPerBeat)
     + arrangementView.headerWidth;
     
     arrangementView.setSize (arrangementWidth, area.getHeight());*/
    auto area = getLocalBounds();
    
    transport.setBounds (area.removeFromTop (60));
    
    auto topBar = area.removeFromTop (40);
    arrangementButton.setBounds (topBar.removeFromLeft (120).reduced (5));
    pianoToolButton.setBounds (topBar.removeFromLeft (120).reduced (5));
    saveProjectButton.setBounds (topBar.removeFromLeft (120).reduced (5));
    loadProjectButton.setBounds (topBar.removeFromLeft (120).reduced (5));
    addTrackButton.setBounds (topBar.removeFromLeft (100).reduced (5));
    removeTrackButton.setBounds (topBar.removeFromLeft (100).reduced (5));
    importVideoButton.setBounds (topBar.removeFromLeft (140).reduced (5));
    
    loadSampleInstrumentButton.setBounds (topBar.removeFromLeft (160).reduced (5));
    
    if (arrangementViewport.isVisible())
    {
        auto videoArea = area.removeFromTop (220).reduced (8);
        videoPlayer.setBounds (videoArea);
        
        auto contentArea = area;
        auto browserArea = contentArea.removeFromLeft (220).reduced (4);
        sampleBrowser.setBounds (browserArea);
        
        auto EditModeArea = contentArea.removeFromLeft (220).reduced (4);
        clipEditModeButton.setBounds (EditModeArea);
        
        arrangementViewport.setBounds (contentArea);
        
        auto arrangementWidth = (int) (arrangementView.getNumBars()
                                       * arrangementView.beatsPerBar
                                       * arrangementView.pixelsPerSecond)
        + arrangementView.headerWidth;
        
        auto arrangementHeight = arrangementView.getTrackCount() * arrangementView.trackHeight;
        arrangementView.setSize (arrangementWidth, juce::jmax (contentArea.getHeight(), arrangementHeight));
    }
    else
    {
        auto pianoArea = area;
            auto browserArea = pianoArea.removeFromLeft (220).reduced (4);

            instrumentBrowser.setBounds (browserArea);
            pianoViewport.setBounds (pianoArea);

            auto pianoWidth = (int) (pianoRoll.getNumBars()
                                     * pianoRoll.beatsPerBar
                                     * pianoRoll.pixelsPerBeat);

            pianoRoll.setSize (pianoWidth, pianoArea.getHeight());
    }
}

void MainComponent::timerCallback()
{
    if (waitingForVideoDuration)
    {
        ++videoDurationPollCount;

        double durationSeconds = videoPlayer.getVideoDuration();
        DBG ("Polled video duration: " << durationSeconds);

        if (durationSeconds > 0.0)
        {
            arrangementView.addVideoClipToTrack (
                pendingVideoFile,
                0,
                arrangementView.getLastClickedTimeSeconds(),
                durationSeconds
            );

            currentLoadedVideoFile = pendingVideoFile;
            waitingForVideoDuration = false;
            pendingVideoFile = juce::File {};
            videoDurationPollCount = 0;

            videoPlayer.stop();

            DBG ("Added video clip with real duration: " << durationSeconds);
        }
        else if (videoDurationPollCount > 100) // about 5 seconds at 20 Hz
        {
            DBG ("Timed out waiting for video duration");
            waitingForVideoDuration = false;
            pendingVideoFile = juce::File {};
            videoDurationPollCount = 0;
        }
    }
    
    pianoRoll.setPlayhead (audioEngine.getPlayheadBeat());
    arrangementView.setPlayheadBeat (audioEngine.getArrangementPlayheadSeconds());

    if (pianoViewport.isVisible())
        audioEngine.setNotes (pianoRoll.getNotes());
    else if (arrangementViewport.isVisible())
        audioEngine.setArrangementTracks (arrangementView.getTracks());

    for (const auto& n : pianoRoll.getNotes())
    {
        DBG ("Note midi=" << n.midiNote
             << " beat=" << n.startBeat
             << " mode=" << (n.playbackMode == InstrumentPlaybackMode::Sample ? "sample" : "osc")
             << " builtIn=" << (int) n.instrument
             << " userId=" << n.userInstrumentId);
    }
    updateArrangementVideoPlayback();
}

juce::File MainComponent::getSampleLibraryFolder() const
{
    auto appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
    auto folder = appData.getChildFile ("MyDAW").getChildFile ("Samples");

    if (! folder.exists())
        folder.createDirectory();

    DBG ("Sample library folder: " << folder.getFullPathName());
    return folder;
}

void MainComponent::loadSampleLibrary()
{
    sampleBrowser.clear();
    auto folder = getSampleLibraryFolder();

    juce::Array<juce::File> wavFiles;
    folder.findChildFiles (wavFiles, juce::File::findFiles, false, "*.wav");

    for (const auto& file : wavFiles)
        sampleBrowser.addFile (file);
}

void MainComponent::importWavToLibrary (const juce::File& sourceFile)
{
    auto libraryFolder = getSampleLibraryFolder();

    auto destFile = libraryFolder.getChildFile (sourceFile.getFileName());

    // avoid overwrite conflicts by appending a number
    if (destFile.existsAsFile())
        destFile = destFile.getNonexistentSibling();

    bool copied = sourceFile.copyFileTo (destFile);

    if (copied)
    {
        sampleBrowser.addFile (destFile);
        DBG ("Imported WAV to library: " << destFile.getFullPathName());
    }
    else
    {
        DBG ("Failed to copy WAV into library");
    }
}

void MainComponent::updateArrangementVideoPlayback()
{
    if (! arrangementViewport.isVisible())
        return;

    const double playheadSec = audioEngine.getArrangementPlayheadSeconds();
    const auto& tracks = arrangementView.getTracks();

    if (tracks.empty())
        return;

    const auto& videoTrack = tracks[0];
    if (videoTrack.type != TrackType::Video)
        return;

    for (const auto& clip : videoTrack.videoClips)
    {
        const double clipStart = clip.startTimeSeconds;
                const double clipEnd   = clip.startTimeSeconds + clip.lengthSeconds;

                if (playheadSec >= clipStart && playheadSec < clipEnd)
                {
                    const double clipRelativeTime = playheadSec - clipStart;

                    if (currentLoadedVideoFile != clip.file)
                    {
                        auto result = videoPlayer.load (clip.file);
                        if (result.failed())
                        {
                            DBG ("Failed to load video clip: " << result.getErrorMessage());
                            return;
                        }

                        currentLoadedVideoFile = clip.file;
                    }

                    const double currentVideoPos = videoPlayer.getPlayPosition();

                    // only seek if we're noticeably out of sync
                    if (std::abs (currentVideoPos - clipRelativeTime) > 0.05)
                        videoPlayer.setPlayPosition (clipRelativeTime);

                    if (! videoPlayer.isPlaying())
                        videoPlayer.play();

                    return;
                }
    }

    if(videoPlayer.isPlaying())
        videoPlayer.stop();
    
    currentLoadedVideoFile = juce::File{};
}

void MainComponent::importVideoToTrack (const juce::File& file)
{
    if (waitingForVideoDuration)
    {
        DBG ("Already waiting for a video duration");
        return;
    }
    videoDurationPollCount = 0;
    auto result = videoPlayer.load (file);

    if (result.failed())
    {
        DBG ("Failed to load video: " << result.getErrorMessage());
        return;
    }

    pendingVideoFile = file;
    waitingForVideoDuration = true;

    DBG ("Waiting for video duration: " << file.getFileName());
}


juce::var MainComponent::noteToVar (const Note& note) const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("midiNote", note.midiNote);
    obj->setProperty ("startBeat", note.startBeat);
    obj->setProperty ("lengthBeats", note.lengthBeats);
    obj->setProperty ("playbackMode", note.playbackMode == InstrumentPlaybackMode::Sample ? "sample" : "oscillator");
    obj->setProperty ("instrument", (int) note.instrument);
    obj->setProperty ("userInstrumentId", note.userInstrumentId);
    return juce::var (obj);
}

bool MainComponent::loadNoteFromVar (const juce::var& v, Note& outNote) const
{
    if (! v.isObject())
        return false;

    auto* obj = v.getDynamicObject();
    if (obj == nullptr)
        return false;

    outNote.midiNote = (int) obj->getProperty ("midiNote");
    outNote.startBeat = (double) obj->getProperty ("startBeat");
    outNote.lengthBeats = (double) obj->getProperty ("lengthBeats");

    auto mode = obj->getProperty ("playbackMode").toString();
    outNote.playbackMode = (mode == "sample") ? InstrumentPlaybackMode::Sample
                                              : InstrumentPlaybackMode::Oscillator;

    outNote.instrument = static_cast<InstrumentType> ((int) obj->getProperty ("instrument"));
    outNote.userInstrumentId = obj->getProperty ("userInstrumentId").toString();
    return true;
}

juce::var MainComponent::audioClipToVar (const AudioClip& clip) const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("file", clip.file.getFullPathName());
    obj->setProperty ("name", clip.name);
    obj->setProperty ("startTimeSeconds", clip.startTimeSeconds);
    obj->setProperty ("lengthSeconds", clip.lengthSeconds);
    obj->setProperty ("sourceOffsetSeconds", clip.sourceOffsetSeconds);
    obj->setProperty ("sourceLengthSeconds", clip.sourceLengthSeconds);
    return juce::var (obj);
}

bool MainComponent::loadAudioClipFromVar (const juce::var& v, AudioClip& outClip) const
{
    if (! v.isObject())
        return false;

    auto* obj = v.getDynamicObject();
    if (obj == nullptr)
        return false;

    outClip.file = juce::File (obj->getProperty ("file").toString());
    outClip.name = obj->getProperty ("name").toString();
    outClip.startTimeSeconds = (double) obj->getProperty ("startTimeSeconds");
    outClip.lengthSeconds = (double) obj->getProperty ("lengthSeconds");
    outClip.sourceOffsetSeconds = (double) obj->getProperty ("sourceOffsetSeconds");
    outClip.sourceLengthSeconds = (double) obj->getProperty ("sourceLengthSeconds");
    outClip.colour = juce::Colours::transparentBlack;
    
    if (obj->hasProperty ("sourceLengthSeconds"))
        outClip.sourceLengthSeconds = (double) obj->getProperty ("sourceLengthSeconds");
    else
        outClip.sourceLengthSeconds = outClip.lengthSeconds;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (outClip.file));
    if (reader == nullptr || audioEngine.getCurrentSampleRate() <= 0.0)
        return false;

    const double sourceSR = reader->sampleRate;
    const int numChannels = (int) reader->numChannels;
    const int sourceNumSamples = (int) reader->lengthInSamples;

    juce::AudioBuffer<float> sourceBuffer;
    sourceBuffer.setSize (numChannels, sourceNumSamples);
    reader->read (&sourceBuffer, 0, sourceNumSamples, 0, true, true);

    const double ratio = audioEngine.getCurrentSampleRate() / sourceSR;
    const int resampledNumSamples = (int) std::ceil ((double) sourceNumSamples * ratio);

    auto resampled = std::make_shared<juce::AudioBuffer<float>>();
    resampled->setSize (numChannels, resampledNumSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = sourceBuffer.getReadPointer (ch);
        float* dst = resampled->getWritePointer (ch);

        for (int i = 0; i < resampledNumSamples; ++i)
        {
            const double srcPos = (double) i / ratio;
            const int i0 = juce::jlimit (0, sourceNumSamples - 1, (int) std::floor (srcPos));
            const int i1 = juce::jlimit (0, sourceNumSamples - 1, i0 + 1);
            const float frac = (float) (srcPos - (double) i0);
            dst[i] = src[i0] + frac * (src[i1] - src[i0]);
        }
    }

    outClip.audioData = resampled;
    outClip.sourceSampleRate = audioEngine.getCurrentSampleRate();
    return true;
}

juce::var MainComponent::videoClipToVar (const VideoClip& clip) const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("file", clip.file.getFullPathName());
    obj->setProperty ("name", clip.name);
    obj->setProperty ("startTimeSeconds", clip.startTimeSeconds);
    obj->setProperty ("lengthSeconds", clip.lengthSeconds);
    return juce::var (obj);
}

bool MainComponent::loadVideoClipFromVar (const juce::var& v, VideoClip& outClip) const
{
    if (! v.isObject())
        return false;

    auto* obj = v.getDynamicObject();
    if (obj == nullptr)
        return false;

    outClip.file = juce::File (obj->getProperty ("file").toString());
    outClip.name = obj->getProperty ("name").toString();
    outClip.startTimeSeconds = (double) obj->getProperty ("startTimeSeconds");
    outClip.lengthSeconds = (double) obj->getProperty ("lengthSeconds");
    outClip.colour = juce::Colours::mediumpurple;
    return true;
}

juce::var MainComponent::trackToVar (const AudioTrack& track) const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("name", track.name);
    obj->setProperty ("type", track.type == TrackType::Video ? "video" : "audio");
    obj->setProperty ("muted", track.muted);
    obj->setProperty ("solo", track.solo);

    juce::Array<juce::var> audioClips;
    for (const auto& clip : track.clips)
        audioClips.add (audioClipToVar (clip));
    obj->setProperty ("clips", juce::var (audioClips));

    juce::Array<juce::var> videoClips;
    for (const auto& clip : track.videoClips)
        videoClips.add (videoClipToVar (clip));
    obj->setProperty ("videoClips", juce::var (videoClips));

    return juce::var (obj);
}

bool MainComponent::loadTrackFromVar (const juce::var& v, AudioTrack& outTrack) const
{
    if (! v.isObject())
        return false;

    auto* obj = v.getDynamicObject();
    if (obj == nullptr)
        return false;

    outTrack.name = obj->getProperty ("name").toString();
    outTrack.type = obj->getProperty ("type").toString() == "video" ? TrackType::Video : TrackType::Audio;
    outTrack.muted = (bool) obj->getProperty ("muted");
    outTrack.solo = (bool) obj->getProperty ("solo");
    outTrack.clips.clear();
    outTrack.videoClips.clear();

    auto clipsVar = obj->getProperty ("clips");
    if (clipsVar.isArray())
    {
        for (const auto& item : *clipsVar.getArray())
        {
            AudioClip clip;
            if (loadAudioClipFromVar (item, clip))
                outTrack.clips.push_back (clip);
        }
    }

    auto videoClipsVar = obj->getProperty ("videoClips");
    if (videoClipsVar.isArray())
    {
        for (const auto& item : *videoClipsVar.getArray())
        {
            VideoClip clip;
            if (loadVideoClipFromVar (item, clip))
                outTrack.videoClips.push_back (clip);
        }
    }

    return true;
}

void MainComponent::saveProjectAs (const juce::String& projectName)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("projectName", projectName);
    root->setProperty ("bpm", audioEngine.getBpm());
    root->setProperty ("numBars", arrangementView.getNumBars());

    juce::Array<juce::var> notesArray;
    for (const auto& n : pianoRoll.getNotes())
        notesArray.add (noteToVar (n));
    root->setProperty ("notes", juce::var (notesArray));

    juce::Array<juce::var> tracksArray;
    for (const auto& t : arrangementView.getTracks())
        tracksArray.add (trackToVar (t));
    root->setProperty ("tracks", juce::var (tracksArray));

    auto jsonText = juce::JSON::toString (juce::var (root), true);
    auto file = projectManager.getProjectFile (projectName);
    file.replaceWithText (jsonText);

    currentProjectName = projectName;
    DBG ("Saved project: " << file.getFullPathName());
}

void MainComponent::loadProject (const juce::String& projectName)
{
    auto file = projectManager.getProjectFile (projectName);
    if (! file.existsAsFile())
        return;

    auto parsed = juce::JSON::parse (file);
    if (! parsed.isObject())
        return;

    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return;

    double bpm = (double) obj->getProperty ("bpm");
    int numBars = (int) obj->getProperty ("numBars");

    transport.setBpmValue (bpm);
    audioEngine.setBpm (bpm);
    arrangementView.setBpm (bpm);

    pianoRoll.setNumBars (numBars);
    arrangementView.setNumBars (numBars);
    audioEngine.setNumBars (numBars);

    std::vector<Note> notes;
    auto notesVar = obj->getProperty ("notes");
    if (notesVar.isArray())
    {
        for (const auto& item : *notesVar.getArray())
        {
            Note n;
            if (loadNoteFromVar (item, n))
                notes.push_back (n);
        }
    }
    pianoRoll.setNotes (notes);

    std::vector<AudioTrack> tracks;
    auto tracksVar = obj->getProperty ("tracks");
    if (tracksVar.isArray())
    {
        for (const auto& item : *tracksVar.getArray())
        {
            AudioTrack t;
            if (loadTrackFromVar (item, t))
                tracks.push_back (t);
        }
    }
    arrangementView.setTracks (tracks);
    audioEngine.setArrangementTracks (tracks);

    currentProjectName = projectName;
    resized();

    DBG ("Loaded project: " << file.getFullPathName());
}
