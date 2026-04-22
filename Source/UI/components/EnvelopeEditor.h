#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../DesignTokens.h"
#include "../Fonts.h"

namespace SynthVibe
{
    class EnvelopeEditor : public juce::Component
    {
    public:
        EnvelopeEditor(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& attackID,
                       const juce::String& decayID,
                       const juce::String& sustainID,
                       const juce::String& releaseID,
                       juce::Colour lineColour = Tokens::env)
            : apvtsRef(apvts),
              aID(attackID), dID(decayID), sID(sustainID), rID(releaseID),
              colour(lineColour)
        {
            auto makeAttach = [&](const juce::String& id) {
                return std::make_unique<juce::ParameterAttachment>(
                    *apvts.getParameter(id),
                    [this](float) { repaint(); });
            };
            aAttach = makeAttach(attackID);
            dAttach = makeAttach(decayID);
            sAttach = makeAttach(sustainID);
            rAttach = makeAttach(releaseID);
            aAttach->sendInitialUpdate();
            dAttach->sendInitialUpdate();
            sAttach->sendInitialUpdate();
            rAttach->sendInitialUpdate();
        }

        void paint(juce::Graphics& g) override
        {
            const float a = apvtsRef.getRawParameterValue(aID)->load();
            const float d = apvtsRef.getRawParameterValue(dID)->load();
            const float s = apvtsRef.getRawParameterValue(sID)->load();
            const float r = apvtsRef.getRawParameterValue(rID)->load();

            auto b = getLocalBounds().toFloat().reduced(6.f);
            b.removeFromBottom(14.f); // space for readout line

            const float totalTime = a + d + 0.5f + r; // visual sustain window = 0.5s
            const auto xFor = [&](float t) {
                return b.getX() + (t / totalTime) * b.getWidth();
            };
            const auto yFor = [&](float level) {
                return b.getBottom() - level * b.getHeight();
            };

            juce::Path path;
            path.startNewSubPath(xFor(0.f), yFor(0.f));
            path.lineTo(xFor(a), yFor(1.f));

            // Decay exp curve
            const int decaySteps = 40;
            for (int i = 1; i <= decaySteps; ++i)
            {
                const float t  = (float) i / decaySteps * d;
                const float y  = s + (1.f - s) * std::exp(-t / std::max(0.0001f, d));
                path.lineTo(xFor(a + t), yFor(y));
            }
            path.lineTo(xFor(a + d + 0.5f), yFor(s));

            const int releaseSteps = 40;
            for (int i = 1; i <= releaseSteps; ++i)
            {
                const float t = (float) i / releaseSteps * r;
                const float y = s * std::exp(-t / std::max(0.0001f, r));
                path.lineTo(xFor(a + d + 0.5f + t), yFor(y));
            }

            g.setColour(colour.withAlpha(0.25f));
            g.strokePath(path, juce::PathStrokeType(3.f));
            g.setColour(colour);
            g.strokePath(path, juce::PathStrokeType(1.5f));

            // Nodes
            const juce::Point<float> nodes[] = {
                { xFor(a),              yFor(1.f) },
                { xFor(a + d),          yFor(s)   },
                { xFor(a + d + 0.5f),   yFor(s)   },
                { xFor(a + d + 0.5f+r), yFor(0.f) }
            };
            for (auto& n : nodes)
            {
                g.setColour(Tokens::panel);
                g.fillEllipse(n.x - 4.f, n.y - 4.f, 8.f, 8.f);
                g.setColour(colour);
                g.drawEllipse(n.x - 4.f, n.y - 4.f, 8.f, 8.f, 1.5f);
            }

            // Readout line
            juce::String readout;
            readout << "a " << juce::String((int) (a * 1000)) << "ms"
                    << " · d " << juce::String((int) (d * 1000)) << "ms"
                    << " · s " << juce::String(s, 2)
                    << " · r " << juce::String((int) (r * 1000)) << "ms";
            g.setColour(Tokens::ink3);
            g.setFont(Fonts::mono(Tokens::Font::label));
            g.drawText(readout,
                       getLocalBounds().removeFromBottom(14).reduced(6, 0),
                       juce::Justification::centredLeft);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            dragNode = findNearestNode(e.position);
            if (dragNode >= 0)
            {
                aAttach->beginGesture();
                dAttach->beginGesture();
                sAttach->beginGesture();
                rAttach->beginGesture();
            }
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (dragNode < 0) return;
            auto b = getLocalBounds().toFloat().reduced(6.f);
            b.removeFromBottom(14.f);

            const float a = apvtsRef.getRawParameterValue(aID)->load();
            const float d = apvtsRef.getRawParameterValue(dID)->load();
            const float r = apvtsRef.getRawParameterValue(rID)->load();
            const float totalTime = a + d + 0.5f + r;

            const float normX = juce::jlimit(0.f, 1.f, (e.position.x - b.getX()) / b.getWidth());
            const float normY = juce::jlimit(0.f, 1.f, 1.f - (e.position.y - b.getY()) / b.getHeight());
            const float timeAtX = normX * totalTime;

            switch (dragNode)
            {
                case 0: // attack
                    aAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, 4.f, timeAtX));
                    break;
                case 1: // decay corner
                    dAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, 4.f, timeAtX - a));
                    sAttach->setValueAsPartOfGesture(normY);
                    break;
                case 2: // sustain-end y-only
                    sAttach->setValueAsPartOfGesture(normY);
                    break;
                case 3: // release end
                    rAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, 8.f, timeAtX - a - d - 0.5f));
                    break;
            }
        }

        void mouseUp(const juce::MouseEvent&) override
        {
            if (dragNode < 0) return;
            aAttach->endGesture();
            dAttach->endGesture();
            sAttach->endGesture();
            rAttach->endGesture();
            dragNode = -1;
        }

    private:
        int findNearestNode(juce::Point<float> p) const
        {
            const float a = apvtsRef.getRawParameterValue(aID)->load();
            const float d = apvtsRef.getRawParameterValue(dID)->load();
            const float s = apvtsRef.getRawParameterValue(sID)->load();
            const float r = apvtsRef.getRawParameterValue(rID)->load();
            auto b = getLocalBounds().toFloat().reduced(6.f);
            b.removeFromBottom(14.f);
            const float totalTime = a + d + 0.5f + r;

            const juce::Point<float> nodes[] = {
                { b.getX() + (a / totalTime) * b.getWidth(),            b.getBottom() - 1.f * b.getHeight() },
                { b.getX() + ((a + d) / totalTime) * b.getWidth(),      b.getBottom() - s   * b.getHeight() },
                { b.getX() + ((a + d + 0.5f) / totalTime) * b.getWidth(), b.getBottom() - s * b.getHeight() },
                { b.getX() + ((a + d + 0.5f + r) / totalTime) * b.getWidth(), b.getBottom() - 0.f * b.getHeight() }
            };
            int best = -1;
            float bestDist = 14.f; // hit radius in px
            for (int i = 0; i < 4; ++i)
            {
                const float dist = p.getDistanceFrom(nodes[i]);
                if (dist < bestDist) { best = i; bestDist = dist; }
            }
            return best;
        }

        juce::AudioProcessorValueTreeState& apvtsRef;
        juce::String aID, dID, sID, rID;
        juce::Colour colour;
        std::unique_ptr<juce::ParameterAttachment> aAttach, dAttach, sAttach, rAttach;
        int dragNode = -1;
    };
}
