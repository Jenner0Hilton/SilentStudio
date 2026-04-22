/*
  ==============================================================================

    PianoRoll.cpp
    Created: 10 Feb 2026 9:50:12am
    Author:  Mark Hilton

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PianoRoll.h"

PianoRoll::PianoRoll() {
    setWantsKeyboardFocus (true);
}

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

void PianoRoll::setNumBars (int newNumBars)
{
    numBars = juce::jmax (1, newNumBars);
    repaint();
}

void PianoRoll::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    
    auto pos = e.position;
    int hitIndex = getNoteIndexAt (pos);
    lastMousePosition = e.position;

    // Right-click (or Ctrl+click on Mac) deletes a note if clicked on one
    if (e.mods.isRightButtonDown() || e.mods.isCtrlDown())
    {
        if (hitIndex != -1)
        {
            notes.erase (notes.begin() + hitIndex);
            selectedIndices.clear();
            repaint();
        }
        return;
    }

    // Left-click: if clicking on an existing note, do nothing (or toggle delete if you want)
    if (hitIndex != -1){
        selectedIndices.clear();
        selectedIndices.push_back(hitIndex);
        repaint();
        return;
    }
    // Start drag-selection
        isSelecting = true;
        selectionStart = pos;
        selectionRect = juce::Rectangle<float> (selectionStart, selectionStart);
        selectedIndices.clear();
        repaint();
    
    // Otherwise, place a new note ... if issues arise look here first!!!!
    auto midi = yToMidi ((float) e.y);
    auto beat = snapBeat (xToBeat ((float) e.x));
    
    auto maxBeat = numBars * beatsPerBar;

    if (beat >= maxBeat)
        return;

    Note n;
    n.midiNote = midi;
    n.startBeat = beat;
    n.lengthBeats = 1.0;
    
    n.instrument = currentInstrument;
    n.playbackMode = currentPlaybackMode;
    n.userInstrumentId = currentUserInstrumentId;
    
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

bool PianoRoll::isNoteSelected (int index) const
{
    return std::find (selectedIndices.begin(), selectedIndices.end(), index) != selectedIndices.end();
}

void PianoRoll::updateSelectionFromRect()
{
    selectedIndices.clear();

    for (int i = 0; i < (int) notes.size(); ++i)
    {
        if (selectionRect.intersects (getNoteRect (notes[(size_t) i])))
            selectedIndices.push_back (i);
    }

    repaint();
}

void PianoRoll::mouseDrag (const juce::MouseEvent& e)
{
    if (! isSelecting)
        return;

    selectionRect = juce::Rectangle<float> (selectionStart, e.position).getUnion (
                    juce::Rectangle<float> (selectionStart, selectionStart));

    selectionRect = selectionRect.getSmallestIntegerContainer().toFloat();
    updateSelectionFromRect();
}

void PianoRoll::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);

    if (isSelecting)
    {
        isSelecting = false;
        updateSelectionFromRect();
        repaint();
    }
}

void PianoRoll::copySelectedNotes()
{
    clipboardNotes.clear();

    if (selectedIndices.empty())
        return;

    for (int index : selectedIndices)
        clipboardNotes.push_back (notes[(size_t) index]);
}

void PianoRoll::pasteClipboardAt (double targetBeat, int targetMidi)
{
    if (clipboardNotes.empty())
        return;

    double minBeat = clipboardNotes.front().startBeat;
    int maxMidi = clipboardNotes.front().midiNote;

    for (const auto& n : clipboardNotes)
    {
        minBeat = std::min (minBeat, n.startBeat);
        maxMidi = std::max (maxMidi, n.midiNote);
    }

    double beatOffset = targetBeat - minBeat;
    int midiOffset = targetMidi - maxMidi;

    selectedIndices.clear();

    for (const auto& copied : clipboardNotes)
    {
        Note pasted = copied;
        pasted.startBeat += beatOffset;
        pasted.midiNote += midiOffset;

        pasted.startBeat = juce::jmax (0.0, pasted.startBeat);
        pasted.midiNote = juce::jlimit (lowestNote, highestNote, pasted.midiNote);

        notes.push_back (pasted);
        selectedIndices.push_back ((int) notes.size() - 1);
    }

    repaint();
}

bool PianoRoll::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('c', juce::ModifierKeys::commandModifier, 0))
    {
        copySelectedNotes();
        return true;
    }

    if (key == juce::KeyPress ('v', juce::ModifierKeys::commandModifier, 0))
    {
        auto beat = snapBeat (xToBeat (lastMousePosition.x));
        auto midi = yToMidi (lastMousePosition.y);
        pasteClipboardAt (beat, midi);
        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (! selectedIndices.empty())
        {
            std::sort (selectedIndices.rbegin(), selectedIndices.rend());

            for (int index : selectedIndices)
                notes.erase (notes.begin() + index);

            selectedIndices.clear();
            repaint();
        }
        return true;
    }

    return false;
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
    //auto beatsVisible = (double) getWidth() / pixelsPerBeat;
    auto totalBeats = numBars * beatsPerBar;
    //auto totalWidth = beatToX (totalBeats);

    for (double beat = 0; beat <= totalBeats; beat += gridBeat)
    {
        auto x = beatToX (beat);
        bool barLine = std::fmod (beat, beatsPerBar) < 1e-6;

        g.setColour (barLine ? juce::Colours::white.withAlpha (0.25f)
                             : juce::Colours::white.withAlpha (0.10f));
        g.drawLine (x, 0.0f, x, (float) getHeight());
    }

    // draw notes keeping this around incase issues arrise with size
    /*for (const auto& n : notes)
    {
        auto x = beatToX (n.startBeat);
        auto w = beatToX (n.lengthBeats);
        auto y = midiToY (n.midiNote);

        //g.setColour (juce::Colours::cornflowerblue.withAlpha (0.9f));
        bool selected = isNoteSelected ((int) (&n - &notes[0]));

        g.setColour (selected
                     ? juce::Colours::orange.withAlpha (0.95f)
                     : juce::Colours::cornflowerblue.withAlpha (0.9f));
        g.fillRoundedRectangle (x, y + 1.0f, w, (float) rowHeight - 2.0f, 4.0f);

        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (x, y + 1.0f, w, (float) rowHeight - 2.0f, 4.0f, 1.0f);
    }*/
    for (int i = 0; i < (int) notes.size(); ++i)
    {
        const auto& n = notes[(size_t) i];

        auto x = beatToX (n.startBeat);
        auto w = beatToX (n.lengthBeats);
        auto y = midiToY (n.midiNote);

        bool selected = isNoteSelected (i);

        g.setColour (selected
                     ? juce::Colours::orange.withAlpha (0.95f)
                     : juce::Colours::cornflowerblue.withAlpha (0.9f));
        g.fillRoundedRectangle (x, y + 1.0f, w, (float) rowHeight - 2.0f, 4.0f);

        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (x, y + 1.0f, w, (float) rowHeight - 2.0f, 4.0f, 1.0f);
    }
    if (isSelecting)
    {
        g.setColour (juce::Colours::yellow.withAlpha (0.15f));
        g.fillRect (selectionRect);

        g.setColour (juce::Colours::yellow);
        g.drawRect (selectionRect, 2.0f);
    }
    
    auto x = beatToX(playheadBeat);
    g.setColour(juce::Colours::red);
    g.drawLine(x, 0.0f, x, (float)getHeight(), 2.0f);
}

void PianoRoll::setCurrentBuiltInInstrument (InstrumentType type)
{
    currentInstrument = type;
}

void PianoRoll::setCurrentUserInstrument (const juce::String& instrumentId)
{
    currentUserInstrumentId = instrumentId;
}

void PianoRoll::setCurrentPlaybackMode (InstrumentPlaybackMode mode)
{
    currentPlaybackMode = mode;
}

void PianoRoll::setNotes (const std::vector<Note>& newNotes)
{
    notes = newNotes;
    repaint();
}
