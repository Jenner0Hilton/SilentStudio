/*
  ==============================================================================

    ProjectManager.h
    Created: 22 Apr 2026 9:29:11am
    Author:  Mark Hilton

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ProjectManager
{
public:
    juce::File getProjectsFolder() const;
    juce::File getProjectFolder (const juce::String& projectName) const;
    juce::File getProjectFile (const juce::String& projectName) const;

    juce::StringArray listProjects() const;
};
