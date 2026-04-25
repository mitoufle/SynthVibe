#include "FxSlot.h"
#include "FxTypeMap.h"

namespace SynthVibe
{
    void FxSlot::prepare(double sr, int maxBlock)
    {
        drive .prepare(sr, maxBlock);
        chorus.prepare(sr, maxBlock);
        delay .prepare(sr, maxBlock);
        reverb.prepare(sr, maxBlock);
        eq3   .prepare(sr, maxBlock);
        activeType = FxType::None;
        bypass     = false;
    }

    void FxSlot::setParams(const FxSlotParams& p)
    {
        // On a type change, reset the newly-active effect so its internal
        // state (delay buffers, biquad memory) starts clean rather than
        // leaking the previous effect's residue.
        if (p.type != activeType)
        {
            activeType = p.type;
            switch (activeType)
            {
                case FxType::Drive:  drive.reset();  break;
                case FxType::Chorus: chorus.reset(); break;
                case FxType::Delay:  delay.reset();  break;
                case FxType::Reverb: reverb.reset(); break;
                case FxType::Eq3:    eq3.reset();    break;
                default: break;
            }
        }
        bypass = p.bypass;

        switch (activeType)
        {
            case FxType::Drive:  drive .setParams(toDriveParams (p)); break;
            case FxType::Chorus: chorus.setParams(toChorusParams(p)); break;
            case FxType::Delay:  delay .setParams(toDelayParams (p)); break;
            case FxType::Reverb: reverb.setParams(toReverbParams(p)); break;
            case FxType::Eq3:    eq3   .setParams(toEq3Params   (p)); break;
            default: break;       // None + stubs: nothing to set
        }
    }

    void FxSlot::process(juce::AudioBuffer<float>& buffer)
    {
        if (bypass) return;
        switch (activeType)
        {
            case FxType::Drive:  drive .process(buffer); break;
            case FxType::Chorus: chorus.process(buffer); break;
            case FxType::Delay:  delay .process(buffer); break;
            case FxType::Reverb: reverb.process(buffer); break;
            case FxType::Eq3:    eq3   .process(buffer); break;
            default: break;       // None + stubs: passthrough
        }
    }

    void FxSlot::reset()
    {
        drive.reset();
        chorus.reset();
        delay.reset();
        reverb.reset();
        eq3.reset();
    }
}
