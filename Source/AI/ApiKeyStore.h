#pragma once
#include <juce_core/juce_core.h>

class ApiKeyStore
{
public:
    // Production: path defaults to user-config-dir/AISynth/settings.json
    ApiKeyStore();
    // Test: explicit path
    explicit ApiKeyStore(juce::File path);

    juce::String load() const;
    void save(const juce::String& key);
    void clear();
    juce::File getStorePath() const;

private:
    juce::File path;
};
