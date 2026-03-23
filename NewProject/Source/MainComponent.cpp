#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
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
    
    pianoViewport.setViewedComponent(&pianoRoll,false);
    
    addAndMakeVisible (arrangementButton);
    addAndMakeVisible (pianoToolButton);
    addAndMakeVisible (arrangementView);

    // Start with piano tool visible, arrangement hidden
    arrangementView.setVisible (false);
    pianoViewport.setVisible (true);

    arrangementButton.onClick = [this]
    {
        arrangementView.setVisible (true);
        pianoViewport.setVisible (false);
        resized();
    };

    pianoToolButton.onClick = [this]
    {
        arrangementView.setVisible (false);
        pianoViewport.setVisible (true);
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

            bool ok = audioEngine.exportWav (wavFile, 16.0);

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
  /*  auto area = getLocalBounds();
    transport.setBounds(area.removeFromTop(60));
    pianoViewport.setBounds(area);
    
    auto totalWidth = (int) (pianoRoll.getNumBars() * pianoRoll.beatsPerBar * pianoRoll.pixelsPerBeat);
    pianoRoll.setSize (totalWidth, area.getHeight());
    
    
   */
    auto area = getLocalBounds();

       transport.setBounds (area.removeFromTop (60));

       auto topBar = area.removeFromTop (40);
       arrangementButton.setBounds (topBar.removeFromLeft (120).reduced (5));
       pianoToolButton.setBounds (topBar.removeFromLeft (120).reduced (5));

       // Remaining area is the editor area
       arrangementView.setBounds (area);
       pianoViewport.setBounds (area);

       auto totalWidth = (int) (pianoRoll.getNumBars()
                                * pianoRoll.beatsPerBar
                                * pianoRoll.pixelsPerBeat);

       pianoRoll.setSize (totalWidth, area.getHeight());
}

void MainComponent::timerCallback()
{
    //audioEngine.setBpm (120.0);        // later: link to UI
    //audioEngine.setPlaying (true);     // later: transport
    pianoRoll.setPlayhead(audioEngine.getPlayheadBeat());
    audioEngine.setNotes (pianoRoll.getNotes());
}
