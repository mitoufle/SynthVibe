#include "PatchApplier.h"

PatchApplier::PatchApplier(juce::AudioProcessorValueTreeState& a) : apvts(a) {}

namespace
{
    // Clamps `raw` to the parameter's natural unit range; returns the clamped value
    // and sets `wasClamped` if it differed.
    float clampToParamRange(juce::RangedAudioParameter& p, double raw, bool& wasClamped)
    {
        const auto range = p.getNormalisableRange();
        const auto lo = (double) range.start;
        const auto hi = (double) range.end;
        const double clamped = juce::jlimit(lo, hi, raw);
        wasClamped = (clamped != raw);
        return (float) clamped;
    }

    // True if the parameter expects a raw integer index/value rather than 0..1 normalized.
    bool isIndexLike(juce::AudioProcessorParameter& p)
    {
        if (dynamic_cast<juce::AudioParameterChoice*>(&p) != nullptr) return true;
        if (dynamic_cast<juce::AudioParameterInt*>(&p) != nullptr)    return true;
        return false;
    }
}

void PatchApplier::resetToDefaults()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    for (auto* p : apvts.processor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
        {
            rp->beginChangeGesture();
            rp->setValueNotifyingHost(rp->getDefaultValue());
            rp->endChangeGesture();
        }
    }
}

ApplyReport PatchApplier::apply(const Variation& variation)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    ApplyReport report;
    for (auto& [id, value] : variation.params)
    {
        auto* p = apvts.getParameter(id);
        if (p == nullptr) { ++report.unknown; continue; }

        auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p);
        if (rp == nullptr) { ++report.unknown; continue; }

        bool wasClamped = false;

        if (isIndexLike(*p))
        {
            // Value is a raw index; clamp to range, then convert to normalized.
            const float clamped = clampToParamRange(*rp, value, wasClamped);
            const float norm    = rp->convertTo0to1(clamped);
            rp->beginChangeGesture();
            rp->setValueNotifyingHost(norm);
            rp->endChangeGesture();
        }
        else
        {
            // Value is 0..1 normalized; clamp to [0,1].
            const double clampedNorm = juce::jlimit(0.0, 1.0, value);
            wasClamped = (clampedNorm != value);
            rp->beginChangeGesture();
            rp->setValueNotifyingHost((float) clampedNorm);
            rp->endChangeGesture();
        }

        if (wasClamped) ++report.clamped;
        ++report.applied;
    }
    return report;
}
