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

            const auto yFor = [&](float level) {
                return b.getBottom() - level * b.getHeight();
            };

            juce::Path path;
            path.startNewSubPath(xInZone(b, kAttackX0, 0.f), yFor(0.f));
            path.lineTo(xForAttack(a, b), yFor(1.f));

            // Matches the -60 dB convention used by the Envelope engine
            // (Envelope.h). exp(-6.908) ≈ 10⁻³, so at t=d/r the drawn curve
            // has reached its target level to within -60 dB — the audible end.
            constexpr float kExpScale = 6.908f;

            const int decaySteps = 40;
            for (int i = 1; i <= decaySteps; ++i)
            {
                const float t  = (float) i / decaySteps * d;
                const float y  = s + (1.f - s) * std::exp(-kExpScale * t / std::max(0.0001f, d));
                path.lineTo(xInZone(b, kDecayX0, timeToNorm(t, kMaxDecay) * kZoneDecay), yFor(y));
            }
            path.lineTo(xForSustainEnd(b), yFor(s));

            const int releaseSteps = 40;
            for (int i = 1; i <= releaseSteps; ++i)
            {
                const float t = (float) i / releaseSteps * r;
                const float y = s * std::exp(-kExpScale * t / std::max(0.0001f, r));
                path.lineTo(xInZone(b, kReleaseX0, timeToNorm(t, kMaxRelease) * kZoneRelease), yFor(y));
            }

            g.setColour(colour.withAlpha(0.25f));
            g.strokePath(path, juce::PathStrokeType(3.f));
            g.setColour(colour);
            g.strokePath(path, juce::PathStrokeType(1.5f));

            const juce::Point<float> nodes[] = {
                { xForAttack(a, b),       yFor(1.f) },
                { xForDecayCorner(d, b),  yFor(s)   },
                { xForSustainEnd(b),      yFor(s)   },
                { xForReleaseEnd(r, b),   yFor(0.f) }
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

            const float normX = juce::jlimit(0.f, 1.f, (e.position.x - b.getX()) / b.getWidth());
            const float normY = juce::jlimit(0.f, 1.f, 1.f - (e.position.y - b.getY()) / b.getHeight());

            switch (dragNode)
            {
                case 0: // attack
                {
                    const float n = juce::jlimit(0.f, 1.f, (normX - kAttackX0) / kZoneAttack);
                    aAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, kMaxAttack, normToTime(n, kMaxAttack)));
                    break;
                }
                case 1: // decay corner: X→decay only (sustain stays owned by node 2)
                {
                    const float n = juce::jlimit(0.f, 1.f, (normX - kDecayX0) / kZoneDecay);
                    dAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, kMaxDecay, normToTime(n, kMaxDecay)));
                    break;
                }
                case 2: // sustain-end: Y→sustain only
                    sAttach->setValueAsPartOfGesture(normY);
                    break;
                case 3: // release end
                {
                    const float n = juce::jlimit(0.f, 1.f, (normX - kReleaseX0) / kZoneRelease);
                    rAttach->setValueAsPartOfGesture(juce::jlimit(0.001f, kMaxRelease, normToTime(n, kMaxRelease)));
                    break;
                }
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
        // Fixed-zone allocation: each ADSR parameter owns a slice of the panel
        // width. This decouples each node's X position from the others' values,
        // so e.g. a long attack no longer squashes the release node off-screen
        // and the decay corner can never collide with the sustain-end node
        // (whose hit radii used to overlap at long totalTime, leaving the
        // sustain Y-handle ungrabbable).
        static constexpr float kMaxAttack  = 10.f;  // matches NormalisableRange
        static constexpr float kMaxDecay   = 10.f;
        static constexpr float kMaxRelease = 10.f;
        static constexpr float kZoneAttack  = 0.25f;
        static constexpr float kZoneDecay   = 0.25f;
        static constexpr float kZoneSustain = 0.15f;
        static constexpr float kZoneRelease = 0.35f;
        static constexpr float kAttackX0  = 0.f;
        static constexpr float kDecayX0   = kZoneAttack;
        static constexpr float kSustainX0 = kZoneAttack + kZoneDecay;
        static constexpr float kReleaseX0 = kZoneAttack + kZoneDecay + kZoneSustain;

        // Piecewise-linear time scale. The 0–1 s window (where most musical
        // values live) occupies 80 % of each zone's width; 1–10 s occupies
        // the remaining 20 %. Gives fine control at the fast end without
        // capping the usable max.
        static constexpr float kKneeTime = 1.f;
        static constexpr float kKneeNorm = 0.8f;

        static float timeToNorm(float t, float maxT) noexcept
        {
            if (t <= kKneeTime) return (t / kKneeTime) * kKneeNorm;
            const float denom = std::max(0.0001f, maxT - kKneeTime);
            return kKneeNorm + ((t - kKneeTime) / denom) * (1.f - kKneeNorm);
        }
        static float normToTime(float n, float maxT) noexcept
        {
            if (n <= kKneeNorm) return (n / kKneeNorm) * kKneeTime;
            return kKneeTime + ((n - kKneeNorm) / (1.f - kKneeNorm)) * (maxT - kKneeTime);
        }

        static float xInZone(juce::Rectangle<float> b, float zoneStart, float zoneOffset) noexcept
        {
            return b.getX() + (zoneStart + zoneOffset) * b.getWidth();
        }
        static float xForAttack(float a, juce::Rectangle<float> b) noexcept
        {
            const float n = juce::jlimit(0.f, 1.f, timeToNorm(a, kMaxAttack));
            return xInZone(b, kAttackX0, n * kZoneAttack);
        }
        static float xForDecayCorner(float d, juce::Rectangle<float> b) noexcept
        {
            const float n = juce::jlimit(0.f, 1.f, timeToNorm(d, kMaxDecay));
            return xInZone(b, kDecayX0, n * kZoneDecay);
        }
        static float xForSustainEnd(juce::Rectangle<float> b) noexcept
        {
            return xInZone(b, kSustainX0, kZoneSustain);
        }
        static float xForReleaseEnd(float r, juce::Rectangle<float> b) noexcept
        {
            const float n = juce::jlimit(0.f, 1.f, timeToNorm(r, kMaxRelease));
            return xInZone(b, kReleaseX0, n * kZoneRelease);
        }

        int findNearestNode(juce::Point<float> p) const
        {
            const float a = apvtsRef.getRawParameterValue(aID)->load();
            const float d = apvtsRef.getRawParameterValue(dID)->load();
            const float s = apvtsRef.getRawParameterValue(sID)->load();
            const float r = apvtsRef.getRawParameterValue(rID)->load();
            auto b = getLocalBounds().toFloat().reduced(6.f);
            b.removeFromBottom(14.f);

            const juce::Point<float> nodes[] = {
                { xForAttack(a, b),       b.getBottom() - 1.f * b.getHeight() },
                { xForDecayCorner(d, b),  b.getBottom() - s   * b.getHeight() },
                { xForSustainEnd(b),      b.getBottom() - s   * b.getHeight() },
                { xForReleaseEnd(r, b),   b.getBottom() - 0.f * b.getHeight() }
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
