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
    
    //BPM slider
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
    
    area.removeFromLeft(20);

    bpmLabel.setBounds(area.removeFromLeft(40));
    bpmSlider.setBounds(area.removeFromLeft(260));
}
