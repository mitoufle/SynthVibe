#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FxSlotCard.h"
#include "../DesignTokens.h"
#include "../../Parameters/ParameterIDs.h"

namespace SynthVibe
{
    class FxChainStrip : public juce::Component
    {
    public:
        static constexpr int kNumSlots = 10;

        explicit FxChainStrip(juce::AudioProcessorValueTreeState& apvts)
        {
            struct SlotIds { const char* type; const char* bypass; const char* mix;
                             const char* p1; const char* p2; const char* p3; const char* p4; };
            const SlotIds slots[kNumSlots] = {
                { ParamIDs::fx1Type,  ParamIDs::fx1Bypass,  ParamIDs::fx1Mix,
                  ParamIDs::fx1P1,    ParamIDs::fx1P2,      ParamIDs::fx1P3,    ParamIDs::fx1P4  },
                { ParamIDs::fx2Type,  ParamIDs::fx2Bypass,  ParamIDs::fx2Mix,
                  ParamIDs::fx2P1,    ParamIDs::fx2P2,      ParamIDs::fx2P3,    ParamIDs::fx2P4  },
                { ParamIDs::fx3Type,  ParamIDs::fx3Bypass,  ParamIDs::fx3Mix,
                  ParamIDs::fx3P1,    ParamIDs::fx3P2,      ParamIDs::fx3P3,    ParamIDs::fx3P4  },
                { ParamIDs::fx4Type,  ParamIDs::fx4Bypass,  ParamIDs::fx4Mix,
                  ParamIDs::fx4P1,    ParamIDs::fx4P2,      ParamIDs::fx4P3,    ParamIDs::fx4P4  },
                { ParamIDs::fx5Type,  ParamIDs::fx5Bypass,  ParamIDs::fx5Mix,
                  ParamIDs::fx5P1,    ParamIDs::fx5P2,      ParamIDs::fx5P3,    ParamIDs::fx5P4  },
                { ParamIDs::fx6Type,  ParamIDs::fx6Bypass,  ParamIDs::fx6Mix,
                  ParamIDs::fx6P1,    ParamIDs::fx6P2,      ParamIDs::fx6P3,    ParamIDs::fx6P4  },
                { ParamIDs::fx7Type,  ParamIDs::fx7Bypass,  ParamIDs::fx7Mix,
                  ParamIDs::fx7P1,    ParamIDs::fx7P2,      ParamIDs::fx7P3,    ParamIDs::fx7P4  },
                { ParamIDs::fx8Type,  ParamIDs::fx8Bypass,  ParamIDs::fx8Mix,
                  ParamIDs::fx8P1,    ParamIDs::fx8P2,      ParamIDs::fx8P3,    ParamIDs::fx8P4  },
                { ParamIDs::fx9Type,  ParamIDs::fx9Bypass,  ParamIDs::fx9Mix,
                  ParamIDs::fx9P1,    ParamIDs::fx9P2,      ParamIDs::fx9P3,    ParamIDs::fx9P4  },
                { ParamIDs::fx10Type, ParamIDs::fx10Bypass, ParamIDs::fx10Mix,
                  ParamIDs::fx10P1,   ParamIDs::fx10P2,     ParamIDs::fx10P3,   ParamIDs::fx10P4 },
            };

            for (int i = 0; i < kNumSlots; ++i)
            {
                const auto& s = slots[i];
                cards[i] = std::make_unique<FxSlotCard>(apvts,
                    s.type, s.bypass, s.mix,
                    s.p1, s.p2, s.p3, s.p4,
                    i + 1);
                addAndMakeVisible(*cards[i]);
            }
        }

        int getNumSlots() const noexcept { return kNumSlots; }

        void resized() override
        {
            using namespace Tokens;
            // 2 rows × 5 cols. Cards are flexible; gaps are spaceSm.
            auto area = getLocalBounds().reduced(spaceMd);
            const int rowH = (area.getHeight() - spaceSm) / 2;

            auto layoutRow = [&](juce::Rectangle<int> row, int firstSlot)
            {
                const int colW = (row.getWidth() - 4 * spaceSm) / 5;
                for (int c = 0; c < 5; ++c)
                {
                    auto cell = row.removeFromLeft(colW);
                    cards[firstSlot + c]->setBounds(cell);
                    if (c < 4) row.removeFromLeft(spaceSm);
                }
            };

            layoutRow(area.removeFromTop(rowH), 0);
            area.removeFromTop(spaceSm);
            layoutRow(area, 5);
        }

    private:
        std::array<std::unique_ptr<FxSlotCard>, kNumSlots> cards;
    };
}
