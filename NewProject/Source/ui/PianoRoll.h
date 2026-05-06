/*
  ==============================================================================

    PianoRoll.h
    Created: 10 Feb 2026 9:50:12am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../Note.h"
#include "../InstrumentType.h"

class PianoRoll : public juce::Component
{
public:
    PianoRoll();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    void setBpm (double newBpm) { bpm = newBpm; repaint(); }

    const std::vector<Note>& getNotes() const { return notes; }

    // simple settings
    double beatsPerBar = 4.0;
    double gridBeat = 0.25;           // 1/16 note
    int lowestNote = 36;              // C2
    int highestNote = 84;             // C6

    double pixelsPerBeat = 80.0;
    double rowHeight = 16.0;
    double playheadBeat = 0.0;
    void setPlayhead(double beat) { playheadBeat = beat; repaint(); }
    void setNumBars (int newNumBars);
    int getNumBars() const { return numBars; }
    int getNoteIndexAt (juce::Point<float> pos) const;
    juce::Rectangle<float> getNoteRect (const Note& n) const;
    
    //functions for drag select
    bool keyPressed (const juce::KeyPress& key) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    
    void setCurrentInstrument (InstrumentType type) { currentInstrument = type; }
    
    void setCurrentBuiltInInstrument (InstrumentType type);
    void setCurrentUserInstrument (const juce::String& instrumentId);
    void setCurrentPlaybackMode (InstrumentPlaybackMode mode);
    
    void setNotes (const std::vector<Note>& newNotes);
    
    enum class NoteDragMode
    {
        None,
        Move,
        ResizeEnd
    };

    NoteDragMode noteDragMode = NoteDragMode::None;

    int selectedNoteIndex = -1;
    bool isDraggingNote = false;

    double originalNoteStartBeat = 0.0;
    double originalNoteLengthBeats = 1.0;
    double dragStartBeat = 0.0;

    float noteEdgeHitWidth = 8.0f;
    
    void setDefaultNoteLengthBeats (double newLength);
    
    std::vector<Note> copiedNotes;
    double copiedMinStartBeat = 0.0;
    int copiedMinMidiNote = 60;

    std::set<int> selectedNoteIndices;
    
private:
    double defaultNoteLengthBeats = 1.0;
    double bpm = 120.0;
    int numBars = 4;
    std::vector<Note> notes;

    int yToMidi (float y) const;
    double xToBeat (float x) const;
    float midiToY (int midi) const;
    float beatToX (double beat) const;

    double snapBeat (double beat) const;
    
    //select drag
    std::vector<int> selectedIndices;
    std::vector<Note> clipboardNotes;

    bool isSelecting = false;
    juce::Point<float> selectionStart;
    juce::Rectangle<float> selectionRect;
    
    void updateSelectionFromRect();
    void copySelectedNotes();
    void pasteClipboardAt (double targetBeat, int targetMidi);
    bool isNoteSelected (int index) const;
    
    juce::Point<float> lastMousePosition;
    
    InstrumentType currentInstrument = InstrumentType::Sine;
    
    InstrumentPlaybackMode currentPlaybackMode = InstrumentPlaybackMode::Oscillator;
    juce::String currentUserInstrumentId;
    
    juce::Rectangle<float> getNoteRect (int noteIndex) const;
    bool hitTestNote (juce::Point<float> pos, int& outNoteIndex) const;
    NoteDragMode getNoteDragModeForPosition (juce::Point<float> pos, int noteIndex) const;
    
   // bool keyPressed (const juce::KeyPress& key) override;

   
};
