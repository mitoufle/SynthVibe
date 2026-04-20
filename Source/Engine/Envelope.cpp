#include "Envelope.h"
#include <algorithm>

void Envelope::noteOn()
{
    // Does NOT reset currentLevel — retriggers from the current level so voice
    // steal ramps smoothly rather than snapping to zero. Call reset() first for
    // a hard restart.
    stage = Stage::Attack;
}

void Envelope::noteOff()
{
    if (stage != Stage::Idle) {
        releaseLevel = currentLevel;
        stage = Stage::Release;
    }
}

void Envelope::reset()
{
    stage        = Stage::Idle;
    currentLevel = 0.f;
    releaseLevel = 0.f;
}

float Envelope::getNextSample()
{
    const float sr = static_cast<float>(sampleRate);

    switch (stage)
    {
        case Stage::Idle:
            return 0.f;

        case Stage::Attack:
            currentLevel += 1.f / (params.attack * sr);
            if (currentLevel >= 1.f) {
                currentLevel = 1.f;
                stage = Stage::Decay;
            }
            break;

        case Stage::Decay:
            currentLevel -= (1.f - params.sustain) / (params.decay * sr);
            if (currentLevel <= params.sustain) {
                currentLevel = params.sustain;
                stage = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            currentLevel = params.sustain;
            break;

        case Stage::Release:
            currentLevel -= releaseLevel / (params.release * sr);
            if (currentLevel <= 0.f) {
                currentLevel = 0.f;
                stage = Stage::Idle;
            }
            break;
    }

    return currentLevel;
}
