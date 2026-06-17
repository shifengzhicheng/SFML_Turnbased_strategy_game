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
    inline constexpr float StatusCardH = 138.f;
    inline constexpr float ActionCardY = 226.f;
    inline constexpr float ActionCardH = 92.f;
    inline constexpr float ProduceCardY = 324.f;
    inline constexpr float ProduceCardH = 338.f;
    inline constexpr float HelpCardY = 668.f;
    inline constexpr float HelpCardH = 44.f;

    inline constexpr float LaneButtonTop = 188.f;
    inline constexpr float LaneButtonWidth = 60.f;
    inline constexpr float LaneButtonHeight = 30.f;
    inline constexpr float LaneButtonGap = 4.f;
    inline constexpr float LaneButtonLeft = static_cast<float>(config::PanelX) + 16.f;
    inline constexpr float LaneHitPadding = 2.f;

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
        return sf::FloatRect(visual.left - LaneHitPadding,
                             visual.top - LaneHitPadding,
                             visual.width + LaneHitPadding * 2.f,
                             visual.height + LaneHitPadding * 2.f);
    }

    inline sf::FloatRect masteryButtonRect(int buttonY)
    {
        return sf::FloatRect(static_cast<float>(config::MasteryButtonX),
                             static_cast<float>(buttonY + config::MasteryButtonInsetY),
                             static_cast<float>(config::MasteryButtonWidth),
                             static_cast<float>(config::MasteryButtonHeight));
    }
}
