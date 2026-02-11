/*
  ==============================================================================

    PianoRoll.cpp
    Created: 10 Feb 2026 9:50:12am
    Author:  Mark Hilton

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PianoRoll.h"

PianoRoll::PianoRoll() {}

int PianoRoll::yToMidi (float y) const
{
    auto idxFromTop = (int) std::floor (y / (float) rowHeight);
    auto midi = highestNote - idxFromTop;
    return juce::jlimit (lowestNote, highestNote, midi);
}

double PianoRoll::xToBeat (float x) const
{
    return (double) x / pixelsPerBeat;
}

float PianoRoll::midiToY (int midi) const
{
    auto idxFromTop = highestNote - midi;
    return (float) idxFromTop * (float) rowHeight;
}

float PianoRoll::beatToX (double beat) const
{
    return (float) (beat * pixelsPerBeat);
}

double PianoRoll::snapBeat (double beat) const
{
    auto snapped = std::round (beat / gridBeat) * gridBeat;
    return juce::jmax (0.0, snapped);
}

void PianoRoll::mouseDown (const juce::MouseEvent& e)
{
    auto midi = yToMidi ((float) e.y);
    auto beat = snapBeat (xToBeat ((float) e.x));

    Note n;
    n.midiNote = midi;
    n.startBeat = beat;
    n.lengthBeats = 1.0; // fixed 1 beat for now

    notes.push_back (n);
    repaint();
}

void PianoRoll::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.95f));

    // draw horizontal rows (notes)
    for (int midi = lowestNote; midi <= highestNote; ++midi)
    {
        auto y = midiToY (midi);
        bool isBlackKey =
            juce::MidiMessage::isMidiNoteBlack (midi);

        g.setColour (isBlackKey ? juce::Colours::darkgrey : juce::Colours::grey);
        g.fillRect (0.0f, y, (float) getWidth(), (float) rowHeight);

        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.drawLine (0.0f, y, (float) getWidth(), y);
    }

    // vertical grid lines
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    auto beatsVisible = (double) getWidth() / pixelsPerBeat;

    for (double beat = 0; beat < beatsVisible + 1.0; beat += gridBeat)
    {
        auto x = beatToX (beat);
        bool barLine = std::fmod (beat, beatsPerBar) < 1e-6;

        g.setColour (barLine ? juce::Colours::white.withAlpha (0.25f)
                             : juce::Colours::white.withAlpha (0.10f));
        g.drawLine (x, 0.0f, x, (float) getHeight());
    }

    // draw notes
    for (const auto& n : notes)
    {
        auto x = beatToX (n.startBeat);
        auto w = beatToX (n.lengthBeats);
        auto y = midiToY (n.midiNote);

        g.setColour (juce::Colours::cornflowerblue.withAlpha (0.9f));
        g.fillRoundedRectangle (x, y + 1.0f, w, (float) rowHeight - 2.0f, 4.0f);

        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (x, y + 1.0f, w, (float) rowHeight - 2.0f, 4.0f, 1.0f);
    }
}
