/*
  ==============================================================================

    SampleBrowser.h
    Created: 14 Apr 2026 6:05:59am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct BrowserItem
{
    juce::File file;
    juce::String name;
};

class SampleBrowser  : public juce::Component,
                       public juce::DragAndDropContainer
{
public:
    SampleBrowser();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    void addFile (const juce::File& file);
    
    std::function<void(const juce::File&)> onFileChosen;
    void clear();

private:
    std::vector<BrowserItem> items;
    int selectedIndex = -1;
    int dragIndex = -1;
    juce::Point<float> dragStart;

    int rowHeight = 32;

    int yToIndex (float y) const;
};
