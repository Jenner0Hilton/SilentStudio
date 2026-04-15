/*
  ==============================================================================

    InstrumentBrowser.h
    Created: 15 Apr 2026 12:51:19pm
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../InstrumentType.h"
#include "../UserInstrument.h"

enum class BrowserInstrumentKind
{
    BuiltIn,
    UserSample
};

struct InstrumentItem
{
    juce::String name;
    BrowserInstrumentKind kind = BrowserInstrumentKind::BuiltIn;
    InstrumentType builtInType = InstrumentType::Sine;
    UserInstrument userInstrument;
};

class InstrumentBrowser : public juce::Component
{
public:
    InstrumentBrowser();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

    std::function<void(const InstrumentItem&)> onInstrumentChosen;

    void setUserInstruments (const std::vector<UserInstrument>& userInstruments);

private:
    std::vector<InstrumentItem> items;
    int selectedIndex = 0;
    int rowHeight = 32;

    int yToIndex (float y) const;
};
