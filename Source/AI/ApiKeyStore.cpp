#include "ApiKeyStore.h"

namespace
{
    constexpr const char* kKey = "anthropic_api_key";

    juce::File defaultPath()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                 .getChildFile("AISynth")
                 .getChildFile("settings.json");
    }
}

ApiKeyStore::ApiKeyStore() : path(defaultPath()) {}
ApiKeyStore::ApiKeyStore(juce::File p) : path(std::move(p)) {}

juce::File ApiKeyStore::getStorePath() const { return path; }

juce::String ApiKeyStore::load() const
{
    if (! path.existsAsFile()) return {};

    auto text = path.loadFileAsString();
    auto parsed = juce::JSON::parse(text);
    if (! parsed.isObject()) return {};

    return parsed.getProperty(kKey, juce::var()).toString();
}

void ApiKeyStore::save(const juce::String& key)
{
    path.getParentDirectory().createDirectory();

    auto* obj = new juce::DynamicObject();
    obj->setProperty(kKey, key);
    auto json = juce::JSON::toString(juce::var(obj));
    path.replaceWithText(json);
}

void ApiKeyStore::clear()
{
    if (path.existsAsFile())
        path.deleteFile();
}
