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
    
    addAndMakeVisible(transport);
    addAndMakeVisible(pianoViewport);
    
    //swaping shell
    addAndMakeVisible (arrangementButton);
    addAndMakeVisible (pianoToolButton);

    addAndMakeVisible (arrangementViewport);
    addAndMakeVisible (pianoViewport);

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
        videoPlayer.setVisible (true);
        importVideoButton.setVisible (true);
        resized();
    };

    pianoToolButton.onClick = [this]
    {
        audioEngine.stop();
        arrangementViewport.setVisible (false);
        pianoViewport.setVisible (true);
        audioEngine.setPlaybackMode (AudioEngine::PlaybackMode::Piano);
        videoPlayer.setVisible (false);
        importVideoButton.setVisible (false);
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

            arrangementView.addClipToTrack (file, arrangementView.getLastClickedTrack(), arrangementView.getLastClickedBeat()); 
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
            "*.mov;*.mp4;*.m4v;*.avi");

        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync (flags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            fileChooser.reset();

            if (file == juce::File{})
                return;

            videoPlayer.load (juce::URL { file });
            videoPlayer.setVisible (true);

            DBG ("Loaded video: " << file.getFullPathName());
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
    importVideoButton.setBounds (topBar.removeFromLeft (140).reduced (5));
    
    if (arrangementViewport.isVisible())
    {
        auto videoArea = area.removeFromTop (220).reduced (8);
        videoPlayer.setBounds (videoArea);
        
        arrangementViewport.setBounds (area);
        
        auto arrangementWidth = (int) (arrangementView.getNumBars()
                                       * arrangementView.beatsPerBar
                                       * arrangementView.pixelsPerSecond)
        + arrangementView.headerWidth;
        
        arrangementView.setSize (arrangementWidth, area.getHeight());
    }
    else
    {
        pianoViewport.setBounds (area);
        
        auto pianoWidth = (int) (pianoRoll.getNumBars()
                                 * pianoRoll.beatsPerBar
                                 * pianoRoll.pixelsPerBeat);
        
        pianoRoll.setSize (pianoWidth, area.getHeight());
    }
}

void MainComponent::timerCallback()
{
    pianoRoll.setPlayhead (audioEngine.getPlayheadBeat());
    arrangementView.setPlayheadBeat (audioEngine.getPlayheadBeat());
    
    audioEngine.setNotes (pianoRoll.getNotes());
    audioEngine.setArrangementTracks (arrangementView.getTracks());
}
