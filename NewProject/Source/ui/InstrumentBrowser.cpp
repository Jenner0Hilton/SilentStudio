/*
  ==============================================================================

    InstrumentBrowser.cpp
    Created: 15 Apr 2026 12:51:19pm
    Author:  Mark Hilton

  ==============================================================================
*/

#include "InstrumentBrowser.h"

InstrumentBrowser::InstrumentBrowser()
{
    items.push_back ({ "Sine", BrowserInstrumentKind::BuiltIn, InstrumentType::Sine, {} });
    items.push_back ({ "Square", BrowserInstrumentKind::BuiltIn, InstrumentType::Square, {} });
    items.push_back ({ "Saw", BrowserInstrumentKind::BuiltIn, InstrumentType::Saw, {} });
    items.push_back ({ "Triangle", BrowserInstrumentKind::BuiltIn, InstrumentType::Triangle, {} });
}

void InstrumentBrowser::setUserInstruments (const std::vector<UserInstrument>& userInstruments)
{
    items.erase (std::remove_if (items.begin(), items.end(),
                                 [] (const InstrumentItem& item)
                                 {
                                     return item.kind == BrowserInstrumentKind::UserSample;
                                 }),
                 items.end());

    for (const auto& instrument : userInstruments)
    {
        InstrumentItem item;
        item.name = instrument.name;
        item.kind = BrowserInstrumentKind::UserSample;
        item.userInstrument = instrument;
        items.push_back (item);
    }

    repaint();
}

int InstrumentBrowser::yToIndex (float y) const
{
    int index = (int) std::floor ((y - 36.0f) / (float) rowHeight);
    return juce::jlimit (0, (int) items.size() - 1, index);
}

void InstrumentBrowser::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey.darker (0.6f));

    g.setColour (juce::Colours::white);
    g.drawText ("Instrument Browser", 10, 6, getWidth() - 20, 24,
                juce::Justification::centredLeft);

    int y = 36;
    for (int i = 0; i < (int) items.size(); ++i)
    {
        juce::Rectangle<int> row (4, y, getWidth() - 8, rowHeight - 4);

        g.setColour (i == selectedIndex ? juce::Colours::steelblue : juce::Colours::darkgrey);
        g.fillRoundedRectangle (row.toFloat(), 4.0f);

        g.setColour (juce::Colours::white);
        g.drawText (items[(size_t) i].name, row.reduced (8, 0), juce::Justification::centredLeft);

        y += rowHeight;
    }
}

void InstrumentBrowser::resized()
{
}

void InstrumentBrowser::mouseDown (const juce::MouseEvent& e)
{
    if (e.y < 36 || items.empty())
        return;

    selectedIndex = yToIndex ((float) e.y);

    if (onInstrumentChosen)
        onInstrumentChosen (items[(size_t) selectedIndex]);

    repaint();
}
