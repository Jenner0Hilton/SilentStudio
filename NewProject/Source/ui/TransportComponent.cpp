/*
  ==============================================================================

    TransportComponent.cpp
    Created: 10 Feb 2026 9:50:37am
    Author:  Mark Hilton

  ==============================================================================
*/

#include <JuceHeader.h>
#include "TransportComponent.h"

//==============================================================================
TransportComponent::TransportComponent()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);

    playButton.onClick = [this]
    {
        if (onPlay) onPlay();
    };

    stopButton.onClick = [this]
    {
        if (onStop) onStop();
    };
    
    
    //BPM slider //
        addAndMakeVisible(bpmSlider);
        bpmSlider.setRange(40.0, 240.0, 0.1);
        bpmSlider.setValue(120.0);
        bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        bpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 24);

        bpmSlider.onValueChange = [this]
        {
            if (onBpmChanged)
                onBpmChanged(bpmSlider.getValue());
        };

        // Label
        addAndMakeVisible(bpmLabel);
        bpmLabel.setText("BPM", juce::dontSendNotification);
        bpmLabel.setJustificationType(juce::Justification::centredLeft);
    
    
    // Export WAV file Button
    addAndMakeVisible(exportButton);
    exportButton.onClick = [this] { if (onExport) onExport(); };
    
    
    //barsSlider
    addAndMakeVisible (barsSlider);
    addAndMakeVisible (barsLabel);

    barsLabel.setText ("Bars", juce::dontSendNotification);
    barsLabel.setJustificationType (juce::Justification::centredLeft);

    barsSlider.setRange (1, 1000, 1); // if i want to allow for just an absurd amount of bars just change 32 to like 1000 or something give user 33 minutes of usable music score.
    barsSlider.setValue (4);
    barsSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    barsSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 50, 24);

    barsSlider.onValueChange = [this]
    {
        if (onBarsChanged)
            onBarsChanged ((int) barsSlider.getValue());
    };
    
    
    // Record Button
    addAndMakeVisible (recordButton);
    recordButton.setClickingTogglesState (true);

    recordButton.onClick = [this]
    {
        if (onRecordToggled)
            onRecordToggled (recordButton.getToggleState());
    };
    
    //audioSettingsButton
    addAndMakeVisible (audioSettingsButton);

    audioSettingsButton.onClick = [this]
    {
        if (onAudioSettings)
            onAudioSettings();
    };

}

TransportComponent::~TransportComponent()
{
}

void TransportComponent::setBpmValue (double bpm)
{
    bpmSlider.setValue(bpm, juce::dontSendNotification);
}

void TransportComponent::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void TransportComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..
    auto area = getLocalBounds().reduced(10);
    playButton.setBounds(area.removeFromLeft(100));
    stopButton.setBounds(area.removeFromLeft(100));
    recordButton.setBounds (area.removeFromLeft (100));
    area.removeFromLeft (20);
    
    area.removeFromLeft(20);

    bpmLabel.setBounds(area.removeFromLeft(40));
    bpmSlider.setBounds(area.removeFromLeft(260));
    
    exportButton.setBounds(area.removeFromLeft(120));
    
    barsLabel.setBounds(area.removeFromLeft(40));
    barsSlider.setBounds(area.removeFromLeft(100));
    
    audioSettingsButton.setBounds (area.removeFromLeft (140));
    area.removeFromLeft (10);
}
