#include "PresetManager.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& a) : apvts(a) {}

juce::File PresetManager::getPresetsDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
               .getChildFile("AISynth/Presets");
}

void PresetManager::savePreset(const juce::String& name)
{
    auto dir = getPresetsDirectory();
    dir.createDirectory();

    if (auto xml = apvts.copyState().createXml())
    {
        auto file = dir.getChildFile(name + ".aisynth");
        xml->writeTo(file);
    }
}

bool PresetManager::loadPreset(const juce::String& name)
{
    auto file = getPresetsDirectory().getChildFile(name + ".aisynth");
    if (!file.existsAsFile()) return false;

    if (auto xml = juce::XmlDocument::parse(file))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));

    return true;
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (auto& f : getPresetsDirectory().findChildFiles(
             juce::File::findFiles, false, "*.aisynth"))
        names.add(f.getFileNameWithoutExtension());
    return names;
}
