/*
  ==============================================================================

    ProjectManager.cpp
    Created: 22 Apr 2026 9:29:11am
    Author:  Mark Hilton

  ==============================================================================
*/

#include "ProjectManager.h"

juce::File ProjectManager::getProjectsFolder() const
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                    .getChildFile ("MyDAW")
                    .getChildFile ("Projects");

    if (! folder.exists())
        folder.createDirectory();

    return folder;
}

juce::File ProjectManager::getProjectFolder (const juce::String& projectName) const
{
    auto folder = getProjectsFolder().getChildFile (projectName);
    if (! folder.exists())
        folder.createDirectory();

    return folder;
}

juce::File ProjectManager::getProjectFile (const juce::String& projectName) const
{
    return getProjectFolder (projectName).getChildFile ("project.json");
}

juce::StringArray ProjectManager::listProjects() const
{
    juce::StringArray names;
    juce::Array<juce::File> folders;

    getProjectsFolder().findChildFiles (folders, juce::File::findDirectories, false);

    for (const auto& f : folders)
        names.add (f.getFileName());

    names.sort (true);
    return names;
}
