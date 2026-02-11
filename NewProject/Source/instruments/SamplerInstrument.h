/*
  ==============================================================================

    SamplerInstrument.h
    Created: 10 Feb 2026 9:49:34am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SamplerInstrument  : public juce::Component
{
public:
    SamplerInstrument();
    ~SamplerInstrument() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerInstrument)
};
