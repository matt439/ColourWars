#include "game/pch.h"
#include "game/objects/player_badge.h"

#include <array>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

namespace
{
    // Each row is three square pencil cells. A one-cell gap separates P and digit.
    constexpr std::array<unsigned int, 5> letter_p = { 7u, 5u, 7u, 4u, 4u };
    constexpr std::array<std::array<unsigned int, 5>, 4> digits = {{
        { 2u, 6u, 2u, 2u, 7u },
        { 7u, 1u, 7u, 4u, 7u },
        { 7u, 1u, 7u, 1u, 7u },
        { 5u, 5u, 7u, 1u, 1u }
    }};
}

PlayerBadge::PlayerBadge(const PlayerIdentityPresentation* look,
    RenderResources* render_resources) :
    CrudeDrawing("sprite_sheet_1", "pixel", render_resources), look_(look)
{
    if (look_ == nullptr)
    {
        throw std::invalid_argument("Player badge requires identity presentation");
    }
}

std::wstring PlayerBadge::label(int roster_index)
{
    if (roster_index < 0 || roster_index >= static_cast<int>(digits.size()))
    {
        throw std::invalid_argument("Player badge requires a roster index from 0 to 3");
    }
    return L"P" + std::to_wstring(roster_index + 1);
}

void PlayerBadge::draw(DrawList& draw_list, const RectangleF& rectangle,
    int roster_index) const
{
    if (roster_index < 0 || roster_index >= static_cast<int>(digits.size()))
    {
        throw std::invalid_argument("Player badge requires a roster index from 0 to 3");
    }
    if (rectangle.width <= 0.0f || rectangle.height <= 0.0f) { return; }

    const auto& look = *this->look_;
    if (!look.enabled) { return; }
    const float scale = std::min(rectangle.width / look.ui_size.x,
        rectangle.height / look.ui_size.y);
    const float border = std::min(look.border * scale,
        std::min(rectangle.width, rectangle.height) / 2.0f);
    const float cell = std::min((rectangle.width - border * 2.0f) / 9.0f,
        (rectangle.height - border * 2.0f) / 7.0f);
    const Vector2F origin = rectangle.center() - Vector2F(cell * 7.0f, cell * 5.0f) / 2.0f;

    draw_list.set_filter(TextureFilter::point);
    this->fill(draw_list, rectangle, look.background_colour);
    this->frame_rectangle(draw_list, rectangle, border, look.ink_colour);
    for (size_t row = 0; row < letter_p.size(); ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            const unsigned int mask = 1u << (2 - column);
            const Vector2F offset(static_cast<float>(column) * cell,
                static_cast<float>(row) * cell);
            if ((letter_p[row] & mask) != 0u)
            {
                this->fill(draw_list, RectangleF(origin + offset, Vector2F(cell)),
                    look.ink_colour);
            }
            if ((digits[static_cast<size_t>(roster_index)][row] & mask) != 0u)
            {
                this->fill(draw_list, RectangleF(origin + offset + Vector2F(cell * 4.0f, 0.0f),
                    Vector2F(cell)), look.ink_colour);
            }
        }
    }
}
