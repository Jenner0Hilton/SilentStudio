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
    auto pos = e.position;
    int hitIndex = getNoteIndexAt (pos);

    // Right-click (or Ctrl+click on Mac) deletes a note if clicked on one
    if (e.mods.isRightButtonDown() || e.mods.isCtrlDown())
    {
        if (hitIndex != -1)
        {
            notes.erase (notes.begin() + hitIndex);
            repaint();
        }
        return;
    }

    // Left-click: if clicking on an existing note, do nothing (or toggle delete if you want)
    if (hitIndex != -1)
        return;

    // Otherwise, place a new note
    auto midi = yToMidi ((float) e.y);
    auto beat = snapBeat (xToBeat ((float) e.x));

    Note n;
    n.midiNote = midi;
    n.startBeat = beat;
    n.lengthBeats = 1.0;

    notes.push_back (n);
    repaint();
}

juce::Rectangle<float> PianoRoll::getNoteRect (const Note& n) const
{
    auto x = beatToX (n.startBeat);
    auto w = beatToX (n.lengthBeats);
    auto y = midiToY (n.midiNote);

    return { x, y + 1.0f, w, (float) rowHeight - 2.0f };
}

int PianoRoll::getNoteIndexAt (juce::Point<float> pos) const
{
    // Search from back to front so the “topmost” note wins if they overlap
    for (int i = (int) notes.size() - 1; i >= 0; --i)
    {
        if (getNoteRect(notes[(size_t)i]).contains (pos))
            return i;
    }
    return -1;
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
    auto x = beatToX(playheadBeat);
    g.setColour(juce::Colours::red);
    g.drawLine(x, 0.0f, x, (float)getHeight(), 2.0f);
}
