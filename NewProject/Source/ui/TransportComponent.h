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

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportComponent)
};
