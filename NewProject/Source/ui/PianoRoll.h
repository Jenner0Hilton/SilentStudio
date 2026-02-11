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

private:
    double bpm = 120.0;
    std::vector<Note> notes;

    int yToMidi (float y) const;
    double xToBeat (float x) const;
    float midiToY (int midi) const;
    float beatToX (double beat) const;

    double snapBeat (double beat) const;
};
