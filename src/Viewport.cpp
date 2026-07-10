#include "Viewport.h"

#include <algorithm>

namespace viewport
{
    Layout calculate(sf::Vector2u windowSize, sf::Vector2f logicalSize)
    {
        const float windowWidth = static_cast<float>(std::max(1u, windowSize.x));
        const float windowHeight = static_cast<float>(std::max(1u, windowSize.y));
        const float safeLogicalWidth = std::max(1.f, logicalSize.x);
        const float safeLogicalHeight = std::max(1.f, logicalSize.y);
        const float windowAspect = windowWidth / windowHeight;
        const float logicalAspect = safeLogicalWidth / safeLogicalHeight;

        sf::FloatRect normalized(0.f, 0.f, 1.f, 1.f);
        if (windowAspect > logicalAspect) {
            normalized.width = logicalAspect / windowAspect;
            normalized.left = (1.f - normalized.width) * 0.5f;
        }
        else if (windowAspect < logicalAspect) {
            normalized.height = windowAspect / logicalAspect;
            normalized.top = (1.f - normalized.height) * 0.5f;
        }

        return Layout{
            normalized,
            sf::FloatRect(normalized.left * windowWidth,
                          normalized.top * windowHeight,
                          normalized.width * windowWidth,
                          normalized.height * windowHeight)
        };
    }

    sf::View makeView(sf::Vector2u windowSize, sf::Vector2f logicalSize, sf::Vector2f logicalOffset)
    {
        sf::View view(sf::FloatRect(0.f, 0.f, logicalSize.x, logicalSize.y));
        view.move(logicalOffset);
        view.setViewport(calculate(windowSize, logicalSize).normalized);
        return view;
    }

    sf::Vector2f mapPixelToLogical(sf::Vector2i pixel, sf::Vector2u windowSize, sf::Vector2f logicalSize)
    {
        const Layout layout = calculate(windowSize, logicalSize);
        const float width = std::max(1.f, layout.pixels.width);
        const float height = std::max(1.f, layout.pixels.height);
        return sf::Vector2f(
            (static_cast<float>(pixel.x) - layout.pixels.left) * logicalSize.x / width,
            (static_cast<float>(pixel.y) - layout.pixels.top) * logicalSize.y / height);
    }
}
