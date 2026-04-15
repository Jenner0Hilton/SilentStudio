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
    setTrackCount (3);
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
        const auto& track = tracks[(size_t) i];

        g.setColour (juce::Colours::black.withAlpha (0.15f));
        g.fillRect (headerWidth, y, getWidth() - headerWidth, trackHeight);

        // Track name
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawText (track.name,
                    10, y + 2, headerWidth - 20, 22,
                    juce::Justification::centredLeft);

        // Mute button
        auto muteRect = getMuteButtonRect (i);
        g.setColour (track.muted ? juce::Colours::red : juce::Colours::darkgrey);
        g.fillRoundedRectangle (muteRect, 4.0f);
        g.setColour (juce::Colours::white);
        g.drawText ("M", muteRect, juce::Justification::centred);

        // Solo button
        auto soloRect = getSoloButtonRect (i);
        g.setColour (track.solo ? juce::Colours::goldenrod : juce::Colours::darkgrey);
        g.fillRoundedRectangle (soloRect, 4.0f);
        g.setColour (juce::Colours::white);
        g.drawText ("S", soloRect, juce::Justification::centred);
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
        
        if (track.type == TrackType::Video)
        {
            for (int clipIndex = 0; clipIndex < (int) track.videoClips.size(); ++clipIndex)
            {
                const auto& clip = track.videoClips[(size_t) clipIndex];

                auto x = timeToX (clip.startTimeSeconds);
                auto w = (float) (clip.lengthSeconds * pixelsPerSecond);
                w = juce::jmax (w, 24.0f); // minimum visible width

                auto y = (float) trackToY (trackIndex) + 10.0f;

                juce::Rectangle<float> clipRect { x, y, w, (float) trackHeight - 20.0f };

                g.setColour (clip.colour.withAlpha (0.9f));
                g.fillRoundedRectangle (clipRect, 6.0f);

                g.setColour (juce::Colours::white.withAlpha (0.85f));
                g.drawRoundedRectangle (clipRect, 6.0f, 1.5f);

                auto textWidth = clipRect.getWidth() - 16.0f;
                if (textWidth > 4.0f)
                {
                    g.setColour (juce::Colours::white);
                    g.drawText ("Video " + clip.name,
                                clipRect.getX() + 8.0f,
                                clipRect.getY() + 2.0f,
                                textWidth,
                                18.0f,
                                juce::Justification::centredLeft,
                                true);
                }
            }

            continue;
        }

        for (int clipIndex = 0; clipIndex < (int) track.clips.size(); ++clipIndex)
        {
            const auto& clip = track.clips[(size_t) clipIndex];
            auto clipRect = getClipRect (trackIndex, clipIndex);

            bool isSelected = (trackIndex == selectedTrackIndex && clipIndex == selectedClipIndex);

            g.setColour (isSelected
                         ? juce::Colours::yellow.withAlpha (0.95f)
                         : clip.colour.withAlpha (0.9f));
            g.fillRoundedRectangle (clipRect, 6.0f);
            
            drawWaveformInClip (g, clip, clipRect.reduced (4.0f, 16.0f));

            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.drawRoundedRectangle (clipRect, 6.0f, isSelected ? 2.5f : 1.5f);

            g.setColour (juce::Colours::white);
            g.drawText (clip.name,
                        clipRect.getX() + 8.0f,
                        clipRect.getY() + 2.0f,
                        clipRect.getWidth() - 16.0f,
                        18.0f,
                        juce::Justification::centredLeft,
                        true);
            
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.drawLine (clipRect.getX() + 4.0f, clipRect.getY() + 6.0f,
                        clipRect.getX() + 4.0f, clipRect.getBottom() - 6.0f, 2.0f);

            g.drawLine (clipRect.getRight() - 4.0f, clipRect.getY() + 6.0f,
                        clipRect.getRight() - 4.0f, clipRect.getBottom() - 6.0f, 2.0f);
        }
    }
}

void ArrangementView::drawPlayhead (juce::Graphics& g)
{
    auto x = timeToX (playheadBeat);
    g.setColour (juce::Colours::red);
    g.drawLine (x, 0.0f, x, (float) getHeight(), 2.0f);
    
    if (hasClickPosition)
    {
        auto clickX = timeToX (lastClickedTimeSeconds);
        auto clickY = (float) trackToY (lastClickedTrack);

        g.setColour (juce::Colours::yellow.withAlpha (0.7f));
        g.drawRect (clickX, clickY, 12.0f, (float) trackHeight, 2.0f);
    }
}


void ArrangementView::addClipToTrack (const juce::File& file,
                                     int trackIndex,
                                     double startTimeSeconds, double targetSampleRate)
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
    clip.colour = juce::Colours::transparentBlack;
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
    
    int headerTrack = -1;
        auto buttonType = hitTestHeaderButton (e.position, headerTrack);

        if (buttonType != HeaderButtonType::None && headerTrack >= 0)
        {
            auto& track = tracks[(size_t) headerTrack];

            if (buttonType == HeaderButtonType::Mute)
            {
                track.muted = ! track.muted;
            }
            else if (buttonType == HeaderButtonType::Solo)
            {
                track.solo = ! track.solo;
            }

            repaint();
            return;
        }

        if (e.x < headerWidth)
        {
            int trackIndex = headerYToTrack ((float) e.y);
            showRenameTrackDialog (trackIndex);
            return;
        }

    //if (e.x < headerWidth){
      //  return;
    //}

    lastClickedTrack = yToTrack ((float) e.y);
    lastClickedTimeSeconds = juce::jmax (0.0, xToTime ((float) e.x));
    hasClickPosition = true;

    int hitTrack = -1;
    int hitClip = -1;

    if (hitTestClip (e.position, hitTrack, hitClip))
    {
        selectedTrackIndex = hitTrack;
        selectedClipIndex = hitClip;
        isDraggingClip = true;

        dragMode = getDragModeForPosition (e.position, hitTrack, hitClip);

        const auto& clip = tracks[(size_t) hitTrack].clips[(size_t) hitClip];

        dragOffsetSeconds = xToTime ((float) e.x) - clip.startTimeSeconds;

        dragStartMouseTime = juce::jmax (0.0, xToTime ((float) e.x));
        originalClipStartTime = clip.startTimeSeconds;
        originalClipLength = clip.lengthSeconds;
        originalSourceOffset = clip.sourceOffsetSeconds;
    }
    else
    {
        selectedTrackIndex = -1;
        selectedClipIndex = -1;
        isDraggingClip = false;
        dragMode = DragMode::None;
    }

    repaint();
}


void ArrangementView::mouseDrag (const juce::MouseEvent& e)
{
    if (! isDraggingClip || selectedTrackIndex < 0 || selectedClipIndex < 0)
        return;

    double mouseTime = juce::jmax (0.0, xToTime ((float) e.x));
    double delta = mouseTime - dragStartMouseTime;

    constexpr double minClipLength = 0.05; // 50 ms minimum

    if (dragMode == DragMode::Move)
    {
        int newTrack = yToTrack ((float) e.y);
        double newStart = juce::jmax (0.0, originalClipStartTime + delta);

        if (newTrack == selectedTrackIndex)
        {
            tracks[(size_t) selectedTrackIndex]
                 .clips[(size_t) selectedClipIndex]
                 .startTimeSeconds = newStart;
        }
        else
        {
            auto movedClip = tracks[(size_t) selectedTrackIndex]
                                   .clips[(size_t) selectedClipIndex];

            movedClip.startTimeSeconds = newStart;

            tracks[(size_t) selectedTrackIndex].clips.erase (
                tracks[(size_t) selectedTrackIndex].clips.begin() + selectedClipIndex);

            tracks[(size_t) newTrack].clips.push_back (movedClip);

            selectedTrackIndex = newTrack;
            selectedClipIndex = (int) tracks[(size_t) newTrack].clips.size() - 1;
        }
    }
    else if (dragMode == DragMode::TrimStart)
    {
        auto& clip = tracks[(size_t) selectedTrackIndex].clips[(size_t) selectedClipIndex];

            const double originalRightEdge = originalClipStartTime + originalClipLength;

            double newStart = juce::jmax (0.0, xToTime ((float) e.x));

            // Prevent trim-start from collapsing the clip too small
            double maxStart = originalRightEdge - minClipLength;

            // Prevent sourceOffsetSeconds from going below zero
            double minStart = originalClipStartTime - originalSourceOffset;

            newStart = juce::jlimit (minStart, maxStart, newStart);

            clip.startTimeSeconds = newStart;
            clip.lengthSeconds = originalRightEdge - newStart;
            clip.sourceOffsetSeconds = originalSourceOffset + (newStart - originalClipStartTime);

            // Extra hard safety clamp
            if (clip.audioData != nullptr)
            {
                double totalSourceLength =
                    (double) clip.audioData->getNumSamples() / clip.sourceSampleRate;

                clip.sourceOffsetSeconds = juce::jlimit (0.0, totalSourceLength, clip.sourceOffsetSeconds);
                clip.lengthSeconds = juce::jlimit (minClipLength,
                                                   totalSourceLength - clip.sourceOffsetSeconds,
                                                   clip.lengthSeconds);
            }
    }
    else if (dragMode == DragMode::TrimEnd)
    {
        auto& clip = tracks[(size_t) selectedTrackIndex].clips[(size_t) selectedClipIndex];

            double newLength = juce::jmax (minClipLength, originalClipLength + delta);

            if (clip.audioData != nullptr)
            {
                double totalSourceLength =
                    (double) clip.audioData->getNumSamples() / clip.sourceSampleRate;

                // IMPORTANT: use CURRENT source offset
                double maxLength = totalSourceLength - clip.sourceOffsetSeconds;

                newLength = juce::jmin (newLength, maxLength);
            }

            clip.lengthSeconds = newLength;
        if (clip.audioData != nullptr)
        {
            double totalSourceLength =
                (double) clip.audioData->getNumSamples() / clip.sourceSampleRate;

            clip.lengthSeconds = juce::jlimit (
                minClipLength,
                totalSourceLength - clip.sourceOffsetSeconds,
                clip.lengthSeconds
            );
        }
    }
    
    lastClickedTrack = selectedTrackIndex;
    lastClickedTimeSeconds = mouseTime;
    hasClickPosition = true;

    repaint();
}

void ArrangementView::mouseUp (const juce::MouseEvent&)
{
    isDraggingClip = false;
    dragMode = DragMode::None;
}

//rectange hit testing
/*juce::Rectangle<float> ArrangementView::getClipRect (int trackIndex, int clipIndex) const
{
    const auto& clip = tracks[(size_t) trackIndex].clips[(size_t) clipIndex];

    auto x = beatToX (clip.startTimeSeconds);
    auto w = (float) (clip.lengthSeconds * pixelsPerSecond);
    auto y = (float) trackToY (trackIndex) + 10.0f;

    return { x, y, w, (float) trackHeight - 20.0f };
}*/
juce::Rectangle<float> ArrangementView::getClipRect (int trackIndex, int clipIndex) const
{
    const auto& clip = tracks[(size_t) trackIndex].clips[(size_t) clipIndex];

    auto x = timeToX (clip.startTimeSeconds);
    auto w = (float) (clip.lengthSeconds * pixelsPerSecond);
    
    constexpr float minVisualWidth = 16.0f;
    w = juce::jmax (w, minVisualWidth);
    
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

void ArrangementView::drawWaveformInClip (juce::Graphics& g,
                                          const AudioClip& clip,
                                          juce::Rectangle<float> clipRect)
{
    if (clip.audioData == nullptr)
        return;
    
    if (clipRect.getWidth() < 2.0f || clipRect.getHeight() < 2.0f)
        return;
    
    auto* buffer = clip.audioData.get();
    const int numChannels = buffer->getNumChannels();
    const int numSamples = buffer->getNumSamples();

    if (numChannels <= 0 || numSamples <= 0 || clip.sourceSampleRate <= 0.0)
        return;

    // Visible region of the source audio
    int visibleStartSample = (int) std::round (clip.sourceOffsetSeconds * clip.sourceSampleRate);
    int visibleNumSamples  = (int) std::round (clip.lengthSeconds * clip.sourceSampleRate);

    visibleStartSample = juce::jlimit (0, numSamples - 1, visibleStartSample);
    visibleNumSamples  = juce::jlimit (1, numSamples - visibleStartSample, visibleNumSamples);

    g.setColour (juce::Colours::cyan.withAlpha (0.65f));

    const int width = juce::jmax (1, (int) clipRect.getWidth());
    const float centreY = clipRect.getCentreY();
    const float halfHeight = clipRect.getHeight() * 0.35f;
    
    visibleNumSamples  = juce::jlimit (1, numSamples - visibleStartSample, visibleNumSamples);
    
    for (int x = 0; x < width; ++x)
    {
        int startSample = visibleStartSample
                        + juce::jmap (x, 0, width, 0, visibleNumSamples);

        int endSample   = visibleStartSample
                        + juce::jmap (x + 1, 0, width, 0, visibleNumSamples);

        if (endSample <= startSample)
            endSample = startSample + 1;

        startSample = juce::jlimit (visibleStartSample,
                                    visibleStartSample + visibleNumSamples - 1,
                                    startSample);

        endSample   = juce::jlimit (visibleStartSample + 1,
                                    visibleStartSample + visibleNumSamples,
                                    endSample);

        float minValue = 1.0f;
        float maxValue = -1.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = buffer->getReadPointer (ch);

            for (int i = startSample; i < endSample; ++i)
            {
                float s = data[i];
                minValue = juce::jmin (minValue, s);
                maxValue = juce::jmax (maxValue, s);
            }
        }

        float y1 = centreY - maxValue * halfHeight;
        float y2 = centreY - minValue * halfHeight;

        float drawX = clipRect.getX() + (float) x;
        g.drawLine (drawX, y1, drawX, y2);
    }
}

bool ArrangementView::isInterestedInDragSource (const SourceDetails&)
{
    return true;
}

void ArrangementView::itemDropped (const SourceDetails& dragSourceDetails)
{
    juce::String path = dragSourceDetails.description.toString();
    juce::File file (path);

    if (! file.existsAsFile())
        return;

    auto localPos = dragSourceDetails.localPosition;

    int trackIndex = yToTrack ((float) localPos.y);

    // If you're now using seconds-based arrangement:
    double startTimeSeconds = juce::jmax (0.0, xToTime ((float) localPos.x));

    //addClipToTrack (file, trackIndex, startTimeSeconds, 44100);

    DBG ("Dropped file: " << file.getFileName()
         << " on track " << trackIndex
         << " at " << startTimeSeconds << " sec");
}

ArrangementView::DragMode ArrangementView::getDragModeForPosition (juce::Point<float> pos,
                                                                   int trackIndex,
                                                                   int clipIndex) const
{
    auto rect = getClipRect (trackIndex, clipIndex);

    if (! rect.contains (pos))
        return DragMode::None;

    if (std::abs (pos.x - rect.getX()) <= edgeHitWidth)
        return DragMode::TrimStart;

    if (std::abs (pos.x - rect.getRight()) <= edgeHitWidth)
        return DragMode::TrimEnd;

    return DragMode::Move;
}

void ArrangementView::setTrackCount (int newCount)
{
    newCount = juce::jmax (1, newCount);

    int currentCount = (int) tracks.size();

    if (newCount > currentCount)
    {
        for (int i = currentCount; i < newCount; ++i)
        {
            AudioTrack track;
            if (i == 0){
                track.name = "Video Track";
                track.type = TrackType::Video;
                tracks.push_back (track);
            }
            else
            {
                track.name = "Track " + juce::String (i + 1);
                tracks.push_back (track);
            }
        }
    }
    else if (newCount < currentCount)
    {
        tracks.resize ((size_t) newCount);
    }

    repaint();
}

void ArrangementView::addTrack()
{
    AudioTrack track;
    track.name = "Track " + juce::String ((int) tracks.size() + 1);
    tracks.push_back (track);
    repaint();
}

void ArrangementView::removeLastTrack()
{
    if (tracks.size() <= 1)
        return;

    tracks.pop_back();

    if (selectedTrackIndex >= (int) tracks.size())
    {
        selectedTrackIndex = -1;
        selectedClipIndex = -1;
    }

    repaint();
}

int ArrangementView::headerYToTrack (float y) const
{
    int track = (int) std::floor (y / (float) trackHeight);
    return juce::jlimit (0, (int) tracks.size() - 1, track);
}

void ArrangementView::renameTrack (int trackIndex, const juce::String& newName)
{
    if (trackIndex < 0 || trackIndex >= (int) tracks.size())
        return;

    auto trimmed = newName.trim();

    if (trimmed.isNotEmpty())
        tracks[(size_t) trackIndex].name = trimmed;
    else
        tracks[(size_t) trackIndex].name = "Track " + juce::String (trackIndex + 1);

    repaint();
}

void ArrangementView::showRenameTrackDialog (int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= (int) tracks.size())
        return;

    auto* alert = new juce::AlertWindow ("Rename Track",
                                         "Enter a new name for the track:",
                                         juce::AlertWindow::NoIcon);

    alert->addTextEditor ("trackName", tracks[(size_t) trackIndex].name, "Track name:");
    alert->addButton ("OK", 1);
    alert->addButton ("Cancel", 0);

    alert->enterModalState (true,
        juce::ModalCallbackFunction::create ([this, alert, trackIndex] (int result)
        {
            if (result == 1)
                renameTrack (trackIndex, alert->getTextEditorContents ("trackName"));
        }),
        true);
}

void ArrangementView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.x < headerWidth)
    {
        int trackIndex = headerYToTrack ((float) e.y);
        showRenameTrackDialog (trackIndex);
    }
}

juce::Rectangle<float> ArrangementView::getMuteButtonRect (int trackIndex) const
{
    float y = (float) trackToY (trackIndex) + 10.0f;
    return { 8.0f, y + 28.0f, 24.0f, 24.0f };
}

juce::Rectangle<float> ArrangementView::getSoloButtonRect (int trackIndex) const
{
    float y = (float) trackToY (trackIndex) + 10.0f;
    return { 38.0f, y + 28.0f, 24.0f, 24.0f };
}

ArrangementView::HeaderButtonType ArrangementView::hitTestHeaderButton (juce::Point<float> pos,
                                                                        int& outTrackIndex) const
{
    outTrackIndex = -1;

    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        if (getMuteButtonRect (i).contains (pos))
        {
            outTrackIndex = i;
            return HeaderButtonType::Mute;
        }

        if (getSoloButtonRect (i).contains (pos))
        {
            outTrackIndex = i;
            return HeaderButtonType::Solo;
        }
    }

    return HeaderButtonType::None;
}

void ArrangementView::addVideoClipToTrack (const juce::File& file,
                                           int trackIndex,
                                           double startTimeSeconds, double lengthSeconds)
{
    if (trackIndex < 0 || trackIndex >= (int) tracks.size())
        return;

    if (tracks[(size_t) trackIndex].type != TrackType::Video)
        return;

    VideoClip clip;
    clip.file = file;
    clip.name = file.getFileName();
    clip.startTimeSeconds = startTimeSeconds;
    clip.lengthSeconds = lengthSeconds;
    clip.colour = juce::Colours::mediumpurple;

    tracks[(size_t) trackIndex].videoClips.push_back (clip);
    repaint();
}


void ArrangementView::setVideoClipLength (int trackIndex, int clipIndex, double newLengthSeconds)
{
    if (trackIndex < 0 || trackIndex >= (int) tracks.size())
        return;

    if (clipIndex < 0 || clipIndex >= (int) tracks[(size_t) trackIndex].videoClips.size())
        return;

    tracks[(size_t) trackIndex].videoClips[(size_t) clipIndex].lengthSeconds = newLengthSeconds;
    repaint();
}
