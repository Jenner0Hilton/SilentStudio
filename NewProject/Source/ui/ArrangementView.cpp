/*
  ==============================================================================

    ArrangementView.cpp
    Created: 23 Mar 2026 6:34:47am
    Author:  Mark Hilton

  ==============================================================================
*/

#include "ArrangementView.h"

ArrangementView::ArrangementView()
{
    setWantsKeyboardFocus (true);
    
    AudioTrack track1;
    track1.name = "Track 1";
    

    AudioTrack track2;
    track2.name = "Track 2";
 

    AudioTrack track3;
    track3.name = "Track 3";
    

    tracks.push_back (track1);
    tracks.push_back (track2);
    tracks.push_back (track3);
}

void ArrangementView::setPlayheadBeat (double newBeat)
{
    playheadBeat = newBeat;
    repaint();
}

void ArrangementView::setNumBars (int newNumBars)
{
    numBars = juce::jmax (1, newNumBars);
    repaint();
}

float ArrangementView::beatToX (double beat) const
{
    return (float) (headerWidth + beat * ((pixelsPerSecond * 60.0) / bpm));
}
float ArrangementView::timeToX (double seconds) const
{
    return (float) (headerWidth + seconds * pixelsPerSecond);
}


int ArrangementView::trackToY (int trackIndex) const
{
    return trackIndex * trackHeight;
}

void ArrangementView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker (0.8f));

    drawGrid (g);
    drawTracks (g);
    drawClips (g);
    drawPlayhead (g);
}

void ArrangementView::resized()
{
}

void ArrangementView::drawGrid (juce::Graphics& g)
{
    auto totalBeats = numBars * beatsPerBar;

    g.setColour (juce::Colours::dimgrey);
    g.fillRect (0, 0, headerWidth, getHeight());

    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        auto y = trackToY (i);
        g.setColour (juce::Colours::grey.withAlpha (0.25f));
        g.drawLine ((float) headerWidth, (float) y, (float) getWidth(), (float) y);
    }

    for (double beat = 0; beat <= totalBeats; ++beat)
    {
        auto x = beatToX (beat);
        bool barLine = std::fmod (beat, beatsPerBar) < 0.001;

        g.setColour (barLine ? juce::Colours::white.withAlpha (0.25f)
                             : juce::Colours::white.withAlpha (0.08f));
        g.drawLine (x, 0.0f, x, (float) getHeight());

        if (barLine)
        {
            int barNumber = (int) (beat / beatsPerBar) + 1;
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.drawText (juce::String (barNumber),
                        (int) x + 4, 2, 30, 18,
                        juce::Justification::left);
        }
    }
}

void ArrangementView::drawTracks (juce::Graphics& g)
{
    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        auto y = trackToY (i);

        g.setColour (juce::Colours::black.withAlpha (0.15f));
        g.fillRect (headerWidth, y, getWidth() - headerWidth, trackHeight);

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawText (tracks[(size_t) i].name,
                    10, y, headerWidth - 20, trackHeight,
                    juce::Justification::centredLeft);
    }

    g.setColour (juce::Colours::grey.withAlpha (0.4f));
    for (int i = 0; i <= (int) tracks.size(); ++i)
    {
        auto y = i * trackHeight;
        g.drawLine (0.0f, (float) y, (float) getWidth(), (float) y);
    }
}

void ArrangementView::drawClips (juce::Graphics& g)
{
    for (int trackIndex = 0; trackIndex < (int) tracks.size(); ++trackIndex)
    {
        const auto& track = tracks[(size_t) trackIndex];

        for (int clipIndex = 0; clipIndex < (int) track.clips.size(); ++clipIndex)
        {
            const auto& clip = track.clips[(size_t) clipIndex];
            auto clipRect = getClipRect (trackIndex, clipIndex);

            bool isSelected = (trackIndex == selectedTrackIndex && clipIndex == selectedClipIndex);

            g.setColour (isSelected
                         ? juce::Colours::yellow.withAlpha (0.95f)
                         : clip.colour.withAlpha (0.9f));
            g.fillRoundedRectangle (clipRect, 6.0f);

            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.drawRoundedRectangle (clipRect, 6.0f, isSelected ? 2.5f : 1.5f);

            g.setColour (juce::Colours::white);
            g.drawText (clip.name,
                        clipRect.getX() + 8.0f,
                        clipRect.getY(),
                        clipRect.getWidth() - 16.0f,
                        clipRect.getHeight(),
                        juce::Justification::centredLeft,
                        true);
        }
    }
}

void ArrangementView::drawPlayhead (juce::Graphics& g)
{
    auto x = beatToX (playheadBeat);
    g.setColour (juce::Colours::red);
    g.drawLine (x, 0.0f, x, (float) getHeight(), 2.0f);
    
    if (hasClickPosition)
    {
        auto clickX = beatToX (lastClickedBeat);
        auto clickY = (float) trackToY (lastClickedTrack);

        g.setColour (juce::Colours::yellow.withAlpha (0.7f));
        g.drawRect (clickX, clickY, 12.0f, (float) trackHeight, 2.0f);
    }
}


void ArrangementView::addClipToTrack (const juce::File& file,
                                     int trackIndex,
                                     double startTimeSeconds)
{
    if (trackIndex < 0 || trackIndex >= (int) tracks.size())
        return;

    double lengthSeconds = 1.0;
    auto audioBuffer = std::make_shared<juce::AudioBuffer<float>>();
    double sourceSampleRate = 44100.0;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader != nullptr)
    {
        sourceSampleRate = reader->sampleRate;

        const int numChannels = (int) reader->numChannels;
        const int numSamples  = (int) reader->lengthInSamples;

        audioBuffer->setSize (numChannels, numSamples);
        reader->read (audioBuffer.get(), 0, numSamples, 0, true, true);

        lengthSeconds = (double) numSamples / sourceSampleRate;
    }
    else
    {
        return;
    }

    AudioClip clip;
    clip.file = file;
    clip.name = file.getFileName();
    clip.startTimeSeconds = startTimeSeconds;
    clip.lengthSeconds = lengthSeconds;
    clip.colour = juce::Colours::lightblue;
    clip.audioData = audioBuffer;
    clip.sourceSampleRate = sourceSampleRate;

    tracks[(size_t) trackIndex].clips.push_back (clip);

    selectedTrackIndex = trackIndex;
    selectedClipIndex = (int) tracks[(size_t) trackIndex].clips.size() - 1;

    repaint();
}

double ArrangementView::xToTime (float x) const
{
    return ((double) x - (double) headerWidth) / pixelsPerSecond;
}

int ArrangementView::yToTrack (float y) const
{
    int track = (int) std::floor (y / (float) trackHeight);
    return juce::jlimit (0, (int) tracks.size() - 1, track);
}

double ArrangementView::snapBeat (double beat) const
{
    const double gridBeat = 0.25; // 1/16 note grid
    auto snapped = std::round (beat / gridBeat) * gridBeat;
    return juce::jmax (0.0, snapped);
}

void ArrangementView::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    
    if (e.x < headerWidth)
        return;

    lastClickedTrack = yToTrack ((float) e.y);
    lastClickedBeat = snapBeat (xToTime ((float) e.x));
    hasClickPosition = true;

    int hitTrack = -1;
    int hitClip = -1;

    if (hitTestClip (e.position, hitTrack, hitClip))
    {
        selectedTrackIndex = hitTrack;
        selectedClipIndex = hitClip;
        isDraggingClip = true;

        const auto& clip = tracks[(size_t) hitTrack].clips[(size_t) hitClip];
        dragOffsetSeconds = xToTime((float) e.x) - clip.startTimeSeconds;
    }
    else
    {
        selectedTrackIndex = -1;
        selectedClipIndex = -1;
        isDraggingClip = false;
    }

    repaint();
}

void ArrangementView::mouseDrag (const juce::MouseEvent& e)
{
    if (! isDraggingClip || selectedTrackIndex < 0 || selectedClipIndex < 0)
        return;

    auto newTrack = yToTrack ((float) e.y);
    auto newBeat = snapBeat (xToTime ((float) e.x) - dragOffsetSeconds);
    newBeat = juce::jmax (0.0, newBeat);

    auto clip = tracks[(size_t) selectedTrackIndex].clips[(size_t) selectedClipIndex];

    if (newTrack == selectedTrackIndex)
    {
        tracks[(size_t) selectedTrackIndex].clips[(size_t) selectedClipIndex].startTimeSeconds = newBeat;
    }
    else
    {
        tracks[(size_t) selectedTrackIndex].clips.erase (
            tracks[(size_t) selectedTrackIndex].clips.begin() + selectedClipIndex);

        clip.startTimeSeconds = newBeat;
        tracks[(size_t) newTrack].clips.push_back (clip);

        selectedTrackIndex = newTrack;
        selectedClipIndex = (int) tracks[(size_t) newTrack].clips.size() - 1;
    }

    lastClickedTrack = selectedTrackIndex;
    lastClickedBeat = newBeat;
    hasClickPosition = true;

    repaint();
}

void ArrangementView::mouseUp (const juce::MouseEvent&)
{
    isDraggingClip = false;
}

//rectange hit testing
juce::Rectangle<float> ArrangementView::getClipRect (int trackIndex, int clipIndex) const
{
    const auto& clip = tracks[(size_t) trackIndex].clips[(size_t) clipIndex];

    auto x = beatToX (clip.startTimeSeconds);
    auto w = (float) (clip.lengthSeconds * pixelsPerSecond);
    auto y = (float) trackToY (trackIndex) + 10.0f;

    return { x, y, w, (float) trackHeight - 20.0f };
}

bool ArrangementView::hitTestClip (juce::Point<float> pos, int& outTrackIndex, int& outClipIndex) const
{
    for (int trackIndex = 0; trackIndex < (int) tracks.size(); ++trackIndex)
    {
        const auto& track = tracks[(size_t) trackIndex];

        for (int clipIndex = 0; clipIndex < (int) track.clips.size(); ++clipIndex)
        {
            if (getClipRect (trackIndex, clipIndex).contains (pos))
            {
                outTrackIndex = trackIndex;
                outClipIndex = clipIndex;
                return true;
            }
        }
    }

    outTrackIndex = -1;
    outClipIndex = -1;
    return false;
}

void ArrangementView::setBpm (double newBpm)
{
    bpm = newBpm;
}
 
bool ArrangementView::keyPressed (const juce::KeyPress& key)
{
    if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        && selectedTrackIndex >= 0
        && selectedClipIndex >= 0)
    {
        auto& clips = tracks[(size_t) selectedTrackIndex].clips;

        if (selectedClipIndex < (int) clips.size())
        {
            clips.erase (clips.begin() + selectedClipIndex);

            selectedClipIndex = -1;
            selectedTrackIndex = -1;
            isDraggingClip = false;

            repaint();
            return true;
        }
    }

    return false;
}
