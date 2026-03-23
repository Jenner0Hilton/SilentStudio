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
    std::function<void()> onExport;
    std::function<void(int)> onBarsChanged;
    std::function<void(bool)> onRecordToggled;
    std::function<void()> onAudioSettings;
    std::function<void()> onImportWav;
    
    
    
    
    void paint (juce::Graphics& g) override;
    void resized() override;
    void setBpmValue (double bpm);

    private:
        juce::TextButton playButton { "Play" };
        juce::TextButton stopButton { "Stop" };
        juce::Slider bpmSlider;
        juce::Label bpmLabel;
        juce::Slider barsSlider;// potentially put this in public if causes issue
        juce::Label barsLabel; // same with this one
        juce::TextButton recordButton { "Record" };
        
    
        juce::TextButton exportButton { "Export WAV" };
        juce::TextButton audioSettingsButton { "Audio Settings" };
    
        juce::TextButton importButton { "Import WAV" };
        
};
