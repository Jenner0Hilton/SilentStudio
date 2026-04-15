/*
  ==============================================================================

    SampleBrowser.cpp
    Created: 14 Apr 2026 6:05:59am
    Author:  Mark Hilton

  ==============================================================================
*/

#include "SampleBrowser.h"

SampleBrowser::SampleBrowser()
{
}

void SampleBrowser::addFile (const juce::File& file)
{
    BrowserItem item;
    item.file = file;
    item.name = file.getFileName();

    items.push_back (item);
    repaint();
}

int SampleBrowser::yToIndex (float y) const
{
    int index = (int) std::floor (y / (float) rowHeight);
    return juce::jlimit (0, (int) items.size() - 1, index);
}

void SampleBrowser::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey.darker (0.6f));

    g.setColour (juce::Colours::white);
    g.drawText ("Wav Browser", 10, 6, getWidth() - 20, 24,
                juce::Justification::centredLeft);

    int y = 36;
    for (int i = 0; i < (int) items.size(); ++i)
    {
        juce::Rectangle<int> row (4, y, getWidth() - 8, rowHeight - 4);

        g.setColour (i == selectedIndex
                     ? juce::Colours::steelblue
                     : juce::Colours::darkgrey);
        g.fillRoundedRectangle (row.toFloat(), 4.0f);

        g.setColour (juce::Colours::white);
        g.drawText (items[(size_t) i].name,
                    row.reduced (8, 0),
                    juce::Justification::centredLeft);

        y += rowHeight;
    }
}

void SampleBrowser::resized()
{
}

void SampleBrowser::mouseDown (const juce::MouseEvent& e)
{
    if (items.empty() || e.y < 36)
        return;

    selectedIndex = yToIndex ((float) e.y - 36.0f);
    repaint();

    if (selectedIndex >= 0 && selectedIndex < (int) items.size())
    {
        if (onFileChosen)
            onFileChosen (items[(size_t) selectedIndex].file);
    }
}

void SampleBrowser::mouseDrag (const juce::MouseEvent& e)
{
    if (dragIndex < 0 || dragIndex >= (int) items.size())
        return;

    if (e.getDistanceFromDragStart() > 8)
    {
        startDragging (items[(size_t) dragIndex].file.getFullPathName(), this);
        dragIndex = -1;
    }
}

void SampleBrowser::clear()
{
    items.clear();
    selectedIndex = -1;
    repaint();
}
