#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Breakpoint.h"
#include "DesignTokens.h"
#include "../Parameters/ParameterIDs.h"
#include "components/ArcKnob.h"
#include "components/EnvelopeEditor.h"
#include "components/FilterResponseView.h"
#include "components/FilterTypeSelect.h"
#include "components/OscilloscopeView.h"
#include "components/PanelHeader.h"
#include "components/WaveTypeSelect.h"

class SoundTab : public juce::Component
{
public:
    explicit SoundTab(juce::AudioProcessorValueTreeState& apvts)
        : apvts(apvts),
          osc1Header("OSC 1", SynthVibe::Tokens::osc),
          osc2Header("OSC 2", SynthVibe::Tokens::osc),
          filterHeader("FILTER", SynthVibe::Tokens::filter),
          ampEnvHeader("AMP ENV", SynthVibe::Tokens::env),
          fltEnvHeader("FLT ENV", SynthVibe::Tokens::env),
          osc1Wave(apvts, ParamIDs::osc1Waveform),
          osc2Wave(apvts, ParamIDs::osc2Waveform),
          filterType(apvts, ParamIDs::filterType),
          osc1Scope(apvts, ParamIDs::osc1Waveform),
          osc2Scope(apvts, ParamIDs::osc2Waveform),
          filterResponse(apvts,
                         ParamIDs::filterType,
                         ParamIDs::filterCutoff,
                         ParamIDs::filterResonance),
          ampEnv(apvts, ParamIDs::ampAttack, ParamIDs::ampDecay,
                 ParamIDs::ampSustain, ParamIDs::ampRelease),
          fltEnv(apvts, ParamIDs::fltAttack, ParamIDs::fltDecay,
                 ParamIDs::fltSustain, ParamIDs::fltRelease),
          knobOsc1Oct("Oct", apvts, ParamIDs::osc1Octave, SynthVibe::Tokens::osc, "", 0),
          knobOsc1Semi("Semi", apvts, ParamIDs::osc1Semitone, SynthVibe::Tokens::osc, "", 0),
          knobOsc1Detune("Detune", apvts, ParamIDs::osc1Detune, SynthVibe::Tokens::osc, " ct", 1),
          knobOsc1Level("Level", apvts, ParamIDs::osc1Level, SynthVibe::Tokens::osc, "", 2),
          knobUniVoices("Voices", apvts, ParamIDs::osc1UnisonVoices, SynthVibe::Tokens::osc, "", 0),
          knobUniDetune("Detune", apvts, ParamIDs::osc1UnisonDetune, SynthVibe::Tokens::osc, " ct", 1),
          knobUniSpread("Stereo", apvts, ParamIDs::osc1UnisonSpread, SynthVibe::Tokens::osc, "", 2),
          knobOsc2Oct("Oct", apvts, ParamIDs::osc2Octave, SynthVibe::Tokens::osc, "", 0),
          knobOsc2Semi("Semi", apvts, ParamIDs::osc2Semitone, SynthVibe::Tokens::osc, "", 0),
          knobOsc2Detune("Detune", apvts, ParamIDs::osc2Detune, SynthVibe::Tokens::osc, " ct", 1),
          knobOsc2Level("Level", apvts, ParamIDs::osc2Level, SynthVibe::Tokens::osc, "", 2),
          knobCutoff("Cutoff", apvts, ParamIDs::filterCutoff, SynthVibe::Tokens::filter, " Hz", 0),
          knobResonance("Res", apvts, ParamIDs::filterResonance, SynthVibe::Tokens::filter, "", 2),
          knobFilterEnv("Env", apvts, ParamIDs::filterEnvAmt, SynthVibe::Tokens::filter, "", 2),
          knobOsc1Phase("Phase", apvts, ParamIDs::osc1Phase, SynthVibe::Tokens::osc, " deg", 0),
          knobOsc1Pwm  ("PWM",   apvts, ParamIDs::osc1Pwm,   SynthVibe::Tokens::osc, "",      2),
          knobOsc2Phase("Phase", apvts, ParamIDs::osc2Phase, SynthVibe::Tokens::osc, " deg", 0),
          knobOsc2Pwm  ("PWM",   apvts, ParamIDs::osc2Pwm,   SynthVibe::Tokens::osc, "",      2),
          knobOsc2UniVoices("Voices", apvts, ParamIDs::osc2UnisonVoices, SynthVibe::Tokens::osc, "",    0),
          knobOsc2UniDetune("Detune", apvts, ParamIDs::osc2UnisonDetune, SynthVibe::Tokens::osc, " ct", 1),
          knobOsc2UniSpread("Stereo", apvts, ParamIDs::osc2UnisonSpread, SynthVibe::Tokens::osc, "",    2),
          knobFilterDrive  ("Drive",    apvts, ParamIDs::filterDrive,    SynthVibe::Tokens::filter, "",  2),
          knobFilterKeytrack("Keytrk",  apvts, ParamIDs::filterKeytrack, SynthVibe::Tokens::filter, "",  2)
    {
        for (auto* c : std::initializer_list<juce::Component*> {
            &osc1Header, &osc2Header, &filterHeader, &ampEnvHeader, &fltEnvHeader,
            &osc1Wave, &osc2Wave, &filterType,
            &osc1Scope, &osc2Scope, &filterResponse,
            &ampEnv, &fltEnv,
            &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune, &knobOsc1Level,
            &knobUniVoices, &knobUniDetune, &knobUniSpread,
            &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune, &knobOsc2Level,
            &knobCutoff, &knobResonance, &knobFilterEnv,
            &knobOsc1Phase, &knobOsc1Pwm,
            &knobOsc2Phase, &knobOsc2Pwm,
            &knobOsc2UniVoices, &knobOsc2UniDetune, &knobOsc2UniSpread,
            &knobFilterDrive, &knobFilterKeytrack
        })
            addAndMakeVisible(c);

        osc1WaveAttach = std::make_unique<juce::ParameterAttachment>(
            *apvts.getParameter(ParamIDs::osc1Waveform),
            [this](float v) {
                const int idx = juce::jlimit(0, 3, (int) std::round(v));
                knobOsc1Pwm.setEnabled(idx == 2); // 2 = Square
                knobOsc1Pwm.repaint();
            });
        osc1WaveAttach->sendInitialUpdate();

        osc2WaveAttach = std::make_unique<juce::ParameterAttachment>(
            *apvts.getParameter(ParamIDs::osc2Waveform),
            [this](float v) {
                const int idx = juce::jlimit(0, 3, (int) std::round(v));
                knobOsc2Pwm.setEnabled(idx == 2);
                knobOsc2Pwm.repaint();
            });
        osc2WaveAttach->sendInitialUpdate();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace SynthVibe::Tokens;
        for (auto r : { osc1Bounds, osc2Bounds, filterBounds, ampEnvBounds, fltEnvBounds })
        {
            auto f = r.toFloat().reduced(2.f);
            g.setColour(panel);
            g.fillRoundedRectangle(f, radiusMd);
            g.setColour(edge);
            g.drawRoundedRectangle(f, radiusMd, 1.f);
        }
    }

    void resized() override { layoutFor(SynthVibe::breakpointForWidth(getWidth())); }

    void layoutFor(SynthVibe::BP /*bp*/)
    {
        using namespace SynthVibe::Tokens;
        const int headerH = 20;
        const int scopeH  = 60;
        const int selectH = 26;

        auto area = getLocalBounds().reduced(spaceSm);
        const int colW = area.getWidth() / 3;
        auto col1 = area.removeFromLeft(colW).reduced(spaceXs, 0);
        auto col2 = area.removeFromLeft(colW).reduced(spaceXs, 0);
        auto col3 = area.reduced(spaceXs, 0);

        auto layoutOscPanel = [&](juce::Rectangle<int>& outBounds,
                                  juce::Rectangle<int> col,
                                  SynthVibe::PanelHeader& header,
                                  SynthVibe::OscilloscopeView& scope,
                                  SynthVibe::WaveTypeSelect& waveSel,
                                  std::initializer_list<juce::Component*> pitchRow,
                                  std::initializer_list<juce::Component*> shapeRow,
                                  std::initializer_list<juce::Component*> unisonRow)
        {
            outBounds = col;
            auto c = col.reduced(spaceSm);
            header.setBounds(c.removeFromTop(headerH));
            c.removeFromTop(spaceXs);
            scope.setBounds(c.removeFromTop(scopeH));
            c.removeFromTop(spaceXs);
            waveSel.setBounds(c.removeFromTop(selectH));
            c.removeFromTop(spaceSm);
            const int rowH = c.getHeight() / 3;
            layoutKnobsRow(c.removeFromTop(rowH), pitchRow);
            layoutKnobsRow(c.removeFromTop(rowH), shapeRow);
            layoutKnobsRow(c, unisonRow);
        };

        layoutOscPanel(osc1Bounds, col1,
                       osc1Header, osc1Scope, osc1Wave,
                       { &knobOsc1Oct, &knobOsc1Semi, &knobOsc1Detune },
                       { &knobOsc1Phase, &knobOsc1Pwm, &knobOsc1Level },
                       { &knobUniVoices, &knobUniDetune, &knobUniSpread });

        layoutOscPanel(osc2Bounds, col2,
                       osc2Header, osc2Scope, osc2Wave,
                       { &knobOsc2Oct, &knobOsc2Semi, &knobOsc2Detune },
                       { &knobOsc2Phase, &knobOsc2Pwm, &knobOsc2Level },
                       { &knobOsc2UniVoices, &knobOsc2UniDetune, &knobOsc2UniSpread });

        const int filterH = col3.getHeight() * 34 / 100;
        const int envH    = (col3.getHeight() - filterH) / 2;

        filterBounds = col3.removeFromTop(filterH);
        auto cf = filterBounds.reduced(spaceSm);
        filterHeader.setBounds(cf.removeFromTop(headerH));
        cf.removeFromTop(spaceXs);
        filterResponse.setBounds(cf.removeFromTop(cf.getHeight() / 2));
        cf.removeFromTop(spaceXs);
        filterType.setBounds(cf.removeFromTop(selectH));
        cf.removeFromTop(spaceXs);
        const int filterKnobRowH = cf.getHeight() / 2;
        layoutKnobsRow(cf.removeFromTop(filterKnobRowH),
                       { &knobCutoff, &knobResonance, &knobFilterEnv });
        layoutKnobsRow(cf,
                       { &knobFilterDrive, &knobFilterKeytrack, nullptr });

        ampEnvBounds = col3.removeFromTop(envH);
        auto ca = ampEnvBounds.reduced(spaceSm);
        ampEnvHeader.setBounds(ca.removeFromTop(headerH));
        ca.removeFromTop(spaceXs);
        ampEnv.setBounds(ca);

        fltEnvBounds = col3;
        auto cfe = fltEnvBounds.reduced(spaceSm);
        fltEnvHeader.setBounds(cfe.removeFromTop(headerH));
        cfe.removeFromTop(spaceXs);
        fltEnv.setBounds(cfe);
    }

private:
    static void layoutKnobsRow(juce::Rectangle<int> bounds,
                               std::initializer_list<juce::Component*> knobs)
    {
        if (knobs.size() == 0) return;
        const int w = bounds.getWidth() / (int) knobs.size();
        auto b = bounds;
        for (auto* k : knobs)
        {
            auto slot = b.removeFromLeft(w);
            if (k != nullptr) k->setBounds(slot);
        }
    }

    juce::AudioProcessorValueTreeState& apvts;

    juce::Rectangle<int> osc1Bounds, osc2Bounds, filterBounds, ampEnvBounds, fltEnvBounds;

    SynthVibe::PanelHeader       osc1Header, osc2Header, filterHeader, ampEnvHeader, fltEnvHeader;
    SynthVibe::WaveTypeSelect    osc1Wave, osc2Wave;
    SynthVibe::FilterTypeSelect  filterType;
    SynthVibe::OscilloscopeView  osc1Scope, osc2Scope;
    SynthVibe::FilterResponseView filterResponse;
    SynthVibe::EnvelopeEditor    ampEnv, fltEnv;

    SynthVibe::ArcKnob knobOsc1Oct, knobOsc1Semi, knobOsc1Detune, knobOsc1Level;
    SynthVibe::ArcKnob knobUniVoices, knobUniDetune, knobUniSpread;
    SynthVibe::ArcKnob knobOsc2Oct, knobOsc2Semi, knobOsc2Detune, knobOsc2Level;
    SynthVibe::ArcKnob knobCutoff, knobResonance, knobFilterEnv;
    SynthVibe::ArcKnob knobOsc1Phase, knobOsc1Pwm;
    SynthVibe::ArcKnob knobOsc2Phase, knobOsc2Pwm;
    SynthVibe::ArcKnob knobOsc2UniVoices, knobOsc2UniDetune, knobOsc2UniSpread;
    SynthVibe::ArcKnob knobFilterDrive, knobFilterKeytrack;
    std::unique_ptr<juce::ParameterAttachment> osc1WaveAttach;
    std::unique_ptr<juce::ParameterAttachment> osc2WaveAttach;
};
