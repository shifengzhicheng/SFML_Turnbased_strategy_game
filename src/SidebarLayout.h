#pragma once

#include "Config.h"

#include <SFML/Graphics/Rect.hpp>

namespace sidebar_layout
{
    inline constexpr float PanelInset = 12.f;
    inline constexpr float CardLeft = static_cast<float>(config::PanelX) + PanelInset;
    inline constexpr float CardWidth = static_cast<float>(config::PanelWidth) - PanelInset * 2.f;

    inline constexpr float HeaderCardY = 8.f;
    inline constexpr float HeaderCardH = 70.f;
    inline constexpr float StatusCardY = 84.f;
    inline constexpr float StatusCardH = 156.f;
    inline constexpr float ActionCardY = 246.f;
    inline constexpr float ActionCardH = 90.f;
    inline constexpr float ProduceCardY = 342.f;
    inline constexpr float ProduceCardH = 328.f;
    inline constexpr float HelpCardY = 676.f;
    inline constexpr float HelpCardH = 38.f;

    inline constexpr float LaneButtonTop = 199.f;
    inline constexpr float LaneButtonWidth = (static_cast<float>(config::PanelWidth) - 40.f) / 3.f;
    inline constexpr float LaneButtonHeight = 37.f;
    inline constexpr float LaneButtonGap = 4.f;
    inline constexpr float LaneButtonLeft = static_cast<float>(config::PanelX) + 16.f;
    inline constexpr float LaneHitPaddingX = 8.f;
    inline constexpr float LaneHitPaddingY = 6.f;

    inline sf::FloatRect laneButtonRect(int laneIndex)
    {
        return sf::FloatRect(LaneButtonLeft + static_cast<float>(laneIndex) * (LaneButtonWidth + LaneButtonGap),
                             LaneButtonTop,
                             LaneButtonWidth,
                             LaneButtonHeight);
    }

    inline sf::FloatRect laneButtonHitRect(int laneIndex)
    {
        const sf::FloatRect visual = laneButtonRect(laneIndex);
        return sf::FloatRect(visual.left - LaneHitPaddingX,
                             visual.top - LaneHitPaddingY,
                             visual.width + LaneHitPaddingX * 2.f,
                             visual.height + LaneHitPaddingY * 2.f);
    }

    inline sf::FloatRect laneHitStripRect()
    {
        constexpr float width = LaneButtonWidth * static_cast<float>(lane::Count)
            + LaneButtonGap * static_cast<float>(lane::Count - 1);
        return sf::FloatRect(LaneButtonLeft - LaneHitPaddingX,
                             LaneButtonTop - LaneHitPaddingY,
                             width + LaneHitPaddingX * 2.f,
                             LaneButtonHeight + LaneHitPaddingY * 2.f);
    }

    inline sf::FloatRect masteryButtonRect(int buttonY)
    {
        return sf::FloatRect(static_cast<float>(config::MasteryButtonX),
                             static_cast<float>(buttonY + config::MasteryButtonInsetY),
                             static_cast<float>(config::MasteryButtonWidth),
                             static_cast<float>(config::MasteryButtonHeight));
    }
}
