/*
  ==============================================================================

    ArrangementView.h
    Created: 23 Mar 2026 6:34:47am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../audio/AudioTrack.h"

class ArrangementView : public juce::Component
{
public:
    ArrangementView();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

    void setPlayheadBeat (double newBeat);
    void setNumBars (int newNumBars);
    int getNumBars() const { return numBars; }

    void addClipToTrack (const juce::File& file, int trackIndex, double startBeat);

    int getLastClickedTrack() const { return lastClickedTrack; }
    double getLastClickedBeat() const { return lastClickedBeat; }
    
    void setBpm (double newBpm);
    
    const std::vector<AudioTrack>& getTracks() const { return tracks; }

    double beatsPerBar = 4.0;
    double pixelsPerSecond = 100.0;
    int trackHeight = 80;
    int headerWidth = 120;
    
    bool keyPressed (const juce::KeyPress& key) override;

private:
    std::vector<AudioTrack> tracks;
    int numBars = 8;
    double playheadBeat = 0.0;

    int lastClickedTrack = 0;
    double lastClickedBeat = 0.0;
    bool hasClickPosition = false;
    
    double bpm = 120.0; //default bpm

    // selection + dragging
    int selectedTrackIndex = -1;
    int selectedClipIndex = -1;
    bool isDraggingClip = false;
    double dragOffsetSeconds = 0.0;

    float beatToX (double beat) const;
    int trackToY (int trackIndex) const;
    float timeToX (double seconds) const;
    double xToTime (float x) const;
    double xToBeat (float x) const;
    int yToTrack (float y) const;
    double snapBeat (double beat) const;

    juce::Rectangle<float> getClipRect (int trackIndex, int clipIndex) const;
    bool hitTestClip (juce::Point<float> pos, int& outTrackIndex, int& outClipIndex) const;

    void drawGrid (juce::Graphics& g);
    void drawTracks (juce::Graphics& g);
    void drawClips (juce::Graphics& g);
    void drawPlayhead (juce::Graphics& g);
};
