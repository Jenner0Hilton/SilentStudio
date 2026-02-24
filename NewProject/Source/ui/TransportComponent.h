/*
  ==============================================================================

    TransportComponent.h
    Created: 10 Feb 2026 9:50:37am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class TransportComponent  : public juce::Component
{
public:
    TransportComponent();
    ~TransportComponent() override;

    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void(double)> onBpmChanged;
    
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setBpmValue (double bpm);

    private:
        juce::TextButton playButton { "Play" };
        juce::TextButton stopButton { "Stop" };
        juce::Slider bpmSlider;
        juce::Label bpmLabel;
};
