/*
  ==============================================================================

    Track.h
    Created: 10 Feb 2026 9:48:40am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class Track  : public juce::Component
{
public:
    Track();
    ~Track() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Track)
};
