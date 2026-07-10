#include "Viewport.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(1);
        }
    }

    bool near(float a, float b, float tolerance = 0.02f)
    {
        return std::abs(a - b) <= tolerance;
    }
}

int main()
{
    const sf::Vector2f logical(1388.f, 720.f);

    const viewport::Layout exact = viewport::calculate({1388u, 720u}, logical);
    require(near(exact.normalized.left, 0.f) && near(exact.normalized.top, 0.f),
            "native aspect should not add an offset");
    require(near(exact.normalized.width, 1.f) && near(exact.normalized.height, 1.f),
            "native aspect should fill the window");

    const viewport::Layout wide = viewport::calculate({1920u, 720u}, logical);
    require(wide.normalized.left > 0.f && near(wide.normalized.height, 1.f),
            "wide windows should use horizontal pillarboxing");
    const sf::Vector2f wideCenter = viewport::mapPixelToLogical({960, 360}, {1920u, 720u}, logical);
    require(near(wideCenter.x, logical.x * 0.5f) && near(wideCenter.y, logical.y * 0.5f),
            "wide-window center should map to logical center");
    const sf::Vector2f outside = viewport::mapPixelToLogical({0, 360}, {1920u, 720u}, logical);
    require(outside.x < 0.f, "pillarbox pixels must remain outside the logical canvas");

    const viewport::Layout tall = viewport::calculate({1388u, 1000u}, logical);
    require(tall.normalized.top > 0.f && near(tall.normalized.width, 1.f),
            "tall windows should use vertical letterboxing");
    const sf::Vector2f tallCenter = viewport::mapPixelToLogical({694, 500}, {1388u, 1000u}, logical);
    require(near(tallCenter.x, logical.x * 0.5f) && near(tallCenter.y, logical.y * 0.5f),
            "tall-window center should map to logical center");

    return 0;
}
