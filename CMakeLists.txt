cmake_minimum_required(VERSION 3.22)
project(AISynth VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(AISynth
    VERSION                     0.1.0
    COMPANY_NAME                "AISynth"
    PLUGIN_MANUFACTURER_CODE    AiSy
    PLUGIN_CODE                 Asn1
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "AI Synth"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

# ---------------------------------------------------------------------------
# Sources  — add new .cpp files here as features grow
# ---------------------------------------------------------------------------
target_sources(AISynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters/ParameterLayout.cpp
    Source/Parameters/ModDestinations.cpp
    Source/Engine/Oscillator.cpp
    Source/Engine/UnisonOscillator.cpp
    Source/Engine/Envelope.cpp
    Source/Engine/Filter.cpp
    Source/Engine/Voice.cpp
    Source/Engine/ModEngine.cpp
    Source/Engine/SynthEngine.cpp
    Source/Engine/ArpEngine.cpp
    Source/Presets/PresetManager.cpp
    Source/FX/Delay.cpp
    Source/FX/Chorus.cpp
    Source/FX/Drive.cpp
    Source/FX/Reverb.cpp
    Source/FX/Eq3.cpp
    Source/FX/FxTypeMap.cpp
    Source/FX/FxSlot.cpp
    Source/FX/FxRunner.cpp
    # Source/API/APIServer.cpp     <- uncomment when HTTP layer is added
    # AI layer
    Source/AI/ParamIdIndex.cpp
    Source/AI/SystemPrompt.cpp
    Source/AI/ApiKeyStore.cpp
    Source/AI/JuceHttpTransport.cpp
    Source/AI/ClaudeClient.cpp
    Source/AI/PatchApplier.cpp
)

target_include_directories(AISynth PRIVATE Source)

target_compile_definitions(AISynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(AISynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Test target  — engine-only, no plugin machinery
# ---------------------------------------------------------------------------
juce_add_console_app(AISynthTests PRODUCT_NAME "AISynth Tests")
juce_generate_juce_header(AISynthTests)

target_sources(AISynthTests PRIVATE
    Tests/main.cpp
    Tests/ArpEngineTests.cpp
    Tests/SynthEngineTests.cpp
    Tests/VoiceTests.cpp
    Tests/ParameterIdMigrationTests.cpp
    Tests/DesignTokensTests.cpp
    Tests/UIConstructionTests.cpp
    Tests/ModEngineTests.cpp
    Tests/ExponentialEnvelopeTests.cpp
    Tests/FilterCoefficientsTests.cpp
    Tests/OscillatorTests.cpp
    Tests/Eq3Tests.cpp
    Tests/FxRunnerTests.cpp
    Source/Engine/ArpEngine.cpp
    Source/Engine/Envelope.cpp
    Source/Engine/Filter.cpp
    Source/Engine/Voice.cpp
    Source/Engine/ModEngine.cpp
    Source/Engine/SynthEngine.cpp
    Source/FX/Delay.cpp
    Source/FX/Chorus.cpp
    Source/FX/Drive.cpp
    Source/FX/Reverb.cpp
    Source/FX/Eq3.cpp
    Source/FX/FxTypeMap.cpp
    Source/FX/FxSlot.cpp
    Source/FX/FxRunner.cpp
    Source/Engine/Oscillator.cpp
    Source/Engine/UnisonOscillator.cpp
    Source/Parameters/ParameterLayout.cpp
    Source/Parameters/ModDestinations.cpp
    # AI layer
    Source/AI/ParamIdIndex.cpp
    Source/AI/SystemPrompt.cpp
    Source/AI/ApiKeyStore.cpp
    Source/AI/JuceHttpTransport.cpp
    Source/AI/ClaudeClient.cpp
    Source/AI/PatchApplier.cpp
    Tests/ParamIdIndexDriftTests.cpp
    Tests/SystemPromptTests.cpp
    Tests/ApiKeyStoreTests.cpp
    Tests/ClaudeClientTests.cpp
    Tests/PatchApplierTests.cpp
)

target_include_directories(AISynthTests PRIVATE Source)

target_compile_definitions(AISynthTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

target_link_libraries(AISynthTests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_core
    juce::juce_events
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
