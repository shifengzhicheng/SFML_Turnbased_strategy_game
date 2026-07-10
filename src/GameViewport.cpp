#include "Game.h"

#include "Config.h"
#include "Viewport.h"

#include <cmath>

sf::View Game::logicalView(sf::Vector2f offset) const
{
    return viewport::makeView(renderTarget().getSize(),
                              sf::Vector2f(static_cast<float>(config::WindowWidth),
                                           static_cast<float>(config::WindowHeight)),
                              offset);
}

sf::RenderTarget& Game::renderTarget()
{
    return renderTargetOverride != nullptr ? *renderTargetOverride : static_cast<sf::RenderTarget&>(window);
}

const sf::RenderTarget& Game::renderTarget() const
{
    return renderTargetOverride != nullptr ? *renderTargetOverride : static_cast<const sf::RenderTarget&>(window);
}

sf::Vector2i Game::logicalMousePosition(sf::Vector2i windowPixel) const
{
    const sf::Vector2f logical = viewport::mapPixelToLogical(
        windowPixel,
        window.getSize(),
        sf::Vector2f(static_cast<float>(config::WindowWidth),
                     static_cast<float>(config::WindowHeight)));
    return sf::Vector2i(static_cast<int>(std::lround(logical.x)),
                        static_cast<int>(std::lround(logical.y)));
}
