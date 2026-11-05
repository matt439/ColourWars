#pragma once

// The one rectangle the contract headers take: game floats, x and y at the
// top-left, as the level file writes a "rectangle". The station, gate and
// motion contracts each name it as their own Rectangle; callers convert from
// their own rectangle type at the call. Standard library only, like the rest
// of game/contracts/.
namespace rectangle_contract
{
    struct Rectangle
    {
        float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
        float left() const { return x; }
        float top() const { return y; }
        float right() const { return x + width; }
        float bottom() const { return y + height; }
    };
}
