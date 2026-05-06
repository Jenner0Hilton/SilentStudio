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
    setMouseClickGrabsKeyboardFocus (true);
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
    lastMousePosition = e.position;
    DBG ("PianoRoll focused? " << (hasKeyboardFocus (true) ? "yes" : "no"));
    
    if (e.mods.isShiftDown())
    {
        isSelecting = true;
        selectionStart = e.position;
        selectionRect = juce::Rectangle<float> (selectionStart, selectionStart);

        selectedNoteIndices.clear();

        isDraggingNote = false;
        noteDragMode = NoteDragMode::None;

        repaint();
        return;
    }
    int hitNote = -1;

        if (hitTestNote (e.position, hitNote))
        {
            if (e.mods.isRightButtonDown())
                {
                    notes.erase (notes.begin() + hitNote);
                    selectedNoteIndex = -1;
                    isDraggingNote = false;
                    noteDragMode = NoteDragMode::None;
                    repaint();
                    return;
                }
            
            selectedNoteIndex = hitNote;
            isDraggingNote = true;

            noteDragMode = getNoteDragModeForPosition (e.position, hitNote);

            const auto& n = notes[(size_t) hitNote];

            dragStartBeat = xToBeat ((float) e.x);
            originalNoteStartBeat = n.startBeat;
            originalNoteLengthBeats = n.lengthBeats;

            repaint();
            return;
        }

        selectedNoteIndex = -1;
        isDraggingNote = false;
        noteDragMode = NoteDragMode::None;
    
    //grabKeyboardFocus();
    
    auto pos = e.position;
    int hitIndex = getNoteIndexAt (pos);
    //lastMousePosition = e.position;

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
   /*     isSelecting = true;
        selectionStart = pos;
        selectionRect = juce::Rectangle<float> (selectionStart, selectionStart);
        selectedIndices.clear();
        repaint();*/
    
    // Otherwise, place a new note ... if issues arise look here first!!!!
    auto midi = yToMidi ((float) e.y);
    auto beat = snapBeat (xToBeat ((float) e.x));
    
    auto maxBeat = numBars * beatsPerBar;

    if (beat >= maxBeat)
        return;

    Note n;
    n.midiNote = midi;
    n.startBeat = beat;
    n.lengthBeats = defaultNoteLengthBeats;
    
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
    if (isSelecting)
    {
        selectionRect = juce::Rectangle<float> (selectionStart, e.position).getSmallestIntegerContainer().toFloat();

        selectedIndices.clear();

        for (int i = 0; i < (int) notes.size(); ++i)
        {
            if (selectionRect.intersects (getNoteRect (i)))
                selectedIndices.push_back (i);
        }

        repaint();
        return;
    }
    if (! isDraggingNote || selectedNoteIndex < 0 || selectedNoteIndex >= (int) notes.size())
            return;

        auto& note = notes[(size_t) selectedNoteIndex];

        double mouseBeat = xToBeat ((float) e.x);
        double deltaBeat = mouseBeat - dragStartBeat;

        constexpr double minNoteLength = 0.25; // 1/16 note if grid is quarter-beat

        if (noteDragMode == NoteDragMode::ResizeEnd)
        {
            double newLength = originalNoteLengthBeats + deltaBeat;
            newLength = juce::jmax (minNoteLength, newLength);
            
            
            note.lengthBeats = snapBeat (newLength);
            note.lengthBeats = juce::jmax (minNoteLength, note.lengthBeats);
        }
        else if (noteDragMode == NoteDragMode::Move)
        {
            double newStart = originalNoteStartBeat + deltaBeat;
            newStart = juce::jmax (0.0, snapBeat (newStart));

            note.startBeat = newStart;

            // Optional: move vertically too
            note.midiNote = yToMidi ((float) e.y);
        }

        repaint();
   /*
    if (! isSelecting)
        return;

    selectionRect = juce::Rectangle<float> (selectionStart, e.position).getUnion (
                    juce::Rectangle<float> (selectionStart, selectionStart));

    selectionRect = selectionRect.getSmallestIntegerContainer().toFloat();
    updateSelectionFromRect();*/
}

void PianoRoll::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    
    isDraggingNote = false;
    noteDragMode = NoteDragMode::None;

    if (isSelecting)
    {
        isSelecting = false;
        updateSelectionFromRect();
        repaint();
        return;
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
    DBG ("PianoRoll key pressed: " << key.getTextDescription());

    const bool command = key.getModifiers().isCommandDown()
                      || key.getModifiers().isCtrlDown();

    //auto text = key.getTextCharacter();
    const int keyCode = key.getKeyCode();

    if (command && (keyCode == 'c' || keyCode == 'C'))
    {
        clipboardNotes.clear();

        if (selectedIndices.empty())
        {
            DBG ("Copy pressed, but no notes selected");
            return true;
        }

        for (int index : selectedIndices)
        {
            if (index >= 0 && index < (int) notes.size())
                clipboardNotes.push_back (notes[(size_t) index]);
        }

        DBG ("Copied notes: " << (int) clipboardNotes.size());
        return true;
    }

    if (command && (keyCode == 'v' || keyCode == 'V'))
    {
        if (clipboardNotes.empty())
        {
            DBG ("Paste pressed, but clipboard is empty");
            return true;
        }

        double pasteBeat = snapBeat (xToBeat ((float) lastMousePosition.x));
        int pasteMidi = yToMidi ((float) lastMousePosition.y);

        pasteClipboardAt (pasteBeat, pasteMidi);

        DBG ("Pasted notes");
        return true;
    }
    
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        DBG ("Delete selected notes pressed");
        DBG ("Selected count = " << (int) selectedIndices.size());

        if (selectedIndices.empty())
            return true;

        // Sort descending so erasing does not shift the remaining indexes.
        std::sort (selectedIndices.begin(), selectedIndices.end(), std::greater<int>());

        for (int index : selectedIndices)
        {
            if (index >= 0 && index < (int) notes.size())
                notes.erase (notes.begin() + index);
        }

        selectedIndices.clear();
        selectedNoteIndex = -1;
        isDraggingNote = false;
        noteDragMode = NoteDragMode::None;

        repaint();
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

    /*for (int i = 0; i < (int) notes.size(); ++i)
    {
        const auto& n = notes[(size_t) i];
        auto noteRect = getNoteRect (i);

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
    }*/
    for (int i = 0; i < (int) notes.size(); ++i)
    {
        auto noteRect = getNoteRect (i);

        bool selected = isNoteSelected (i);

        g.setColour (selected
                     ? juce::Colours::orange.withAlpha (0.95f)
                     : juce::Colours::cornflowerblue.withAlpha (0.9f));

        g.fillRoundedRectangle (noteRect, 4.0f);

        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.drawRoundedRectangle (noteRect, 4.0f, selected ? 2.0f : 1.0f);

        // Resize handle on right edge
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.drawLine (noteRect.getRight() - 4.0f,
                    noteRect.getY() + 4.0f,
                    noteRect.getRight() - 4.0f,
                    noteRect.getBottom() - 4.0f,
                    2.0f);
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

juce::Rectangle<float> PianoRoll::getNoteRect (int noteIndex) const
{
    const auto& n = notes[(size_t) noteIndex];

    auto x = beatToX (n.startBeat);
    auto w = (float) (n.lengthBeats * pixelsPerBeat);
    auto y = midiToY (n.midiNote);

    w = juce::jmax (w, 12.0f);

    return { x, y + 1.0f, w, (float) rowHeight - 2.0f };
}

bool PianoRoll::hitTestNote (juce::Point<float> pos, int& outNoteIndex) const
{
    for (int i = (int) notes.size() - 1; i >= 0; --i)
    {
        if (getNoteRect (i).contains (pos))
        {
            outNoteIndex = i;
            return true;
        }
    }

    outNoteIndex = -1;
    return false;
}

PianoRoll::NoteDragMode PianoRoll::getNoteDragModeForPosition (juce::Point<float> pos,
                                                               int noteIndex) const
{
    auto rect = getNoteRect (noteIndex);

    if (! rect.contains (pos))
        return NoteDragMode::None;

    if (std::abs (pos.x - rect.getRight()) <= noteEdgeHitWidth)
        return NoteDragMode::ResizeEnd;

    return NoteDragMode::Move;
}

void PianoRoll::setDefaultNoteLengthBeats (double newLength)
{
    defaultNoteLengthBeats = juce::jlimit (0.25, 16.0, newLength);
}
