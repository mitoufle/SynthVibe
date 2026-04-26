#include <juce_core/juce_core.h>
#include "AI/ApiKeyStore.h"

struct ApiKeyStoreTests : public juce::UnitTest
{
    ApiKeyStoreTests() : juce::UnitTest("ApiKeyStore", "AISynth") {}

    juce::File makeTempPath()
    {
        return juce::File::createTempFile("AISynth-apikey-test.json");
    }

    void runTest() override
    {
        beginTest("load() on missing file returns empty string");
        {
            auto path = makeTempPath();
            path.deleteFile();
            ApiKeyStore store(path);
            expect(store.load().isEmpty());
        }

        beginTest("save then load round-trips the key");
        {
            auto path = makeTempPath();
            path.deleteFile();
            {
                ApiKeyStore store(path);
                store.save("sk-ant-test-12345");
            }
            ApiKeyStore store2(path);
            expectEquals(store2.load(), juce::String("sk-ant-test-12345"));
            path.deleteFile();
        }

        beginTest("clear() removes the saved key");
        {
            auto path = makeTempPath();
            path.deleteFile();
            ApiKeyStore store(path);
            store.save("sk-ant-test");
            store.clear();
            expect(store.load().isEmpty());
            path.deleteFile();
        }

        beginTest("getStorePath() returns the configured path");
        {
            auto path = makeTempPath();
            path.deleteFile();
            ApiKeyStore store(path);
            expectEquals(store.getStorePath().getFullPathName(), path.getFullPathName());
            path.deleteFile();
        }
    }
};

static ApiKeyStoreTests _apiKeyStoreTests;
