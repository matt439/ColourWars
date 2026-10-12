#include "game/pch.h"
#include "game/objects/monitor.h"

#include <cstdlib>

using namespace labrador;
using namespace mattmath;

namespace
{
    // 0..1 from rand(), which `seed` fixes.
    float roll_fraction()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }

    size_t roll_index(size_t count)
    {
        return static_cast<size_t>(std::rand()) % count;
    }

    Vector2F on_screen(const RectangleF& screen, const Vector2F& fraction)
    {
        return Vector2F(screen.x + fraction.x * screen.width,
            screen.y + fraction.y * screen.height);
    }

    // The bezel around the glass, as fractions of the screen, where a note can
    // be stuck so that it overlaps the glass's edge a little and no more.
    constexpr std::array<Vector2F, 6> note_spots = { {
        { 0.015f, 0.12f }, { 0.015f, 0.47f }, { 0.905f, 0.16f },
        { 0.905f, 0.52f }, { 0.16f, 0.005f }, { 0.72f, 0.005f } } };

    constexpr float note_size = 0.085f;   // of the screen's height
    constexpr float sticker_size = 0.022f;
    constexpr float coffee_radius = 0.045f;
    constexpr float lip = 0.012f;
}

MonitorPersona MonitorPersona::roll(const OpeningPresentation& look)
{
    MonitorPersona persona;
    persona.shade = roll_index(look.bezel_shades.size());
    persona.child = roll_fraction() < 0.3f;
    persona.mug = !persona.child && roll_fraction() < 0.6f;

    // Notes on distinct spots: a child's are fewer, an adult may have none.
    std::array<size_t, note_spots.size()> spots = { 0, 1, 2, 3, 4, 5 };
    const size_t notes = persona.child ? 1 + roll_index(2) : roll_index(4);
    for (size_t index = 0; index < notes; ++index)
    {
        std::swap(spots[index], spots[index + roll_index(spots.size() - index)]);
        persona.sticky_notes.push_back({ note_spots[spots[index]],
            roll_index(look.sticky_notes.size()), (roll_fraction() - 0.5f) * 0.3f });
    }

    // Stickers along the bottom of the bezel, which a child covers.
    const size_t stickers = persona.child ? 2 + roll_index(4) : roll_index(2);
    for (size_t index = 0; index < stickers; ++index)
    {
        persona.stickers.push_back({ Vector2F(0.12f + roll_fraction() * 0.7f,
            0.86f + roll_fraction() * 0.1f), roll_index(look.stickers.size()), 0.0f });
    }

    persona.coffee_ring = Vector2F(0.7f + roll_fraction() * 0.18f, 0.9f);

    if (roll_fraction() < 0.35f)
    {
        // From a corner of the bezel, jagged.
        const bool left = roll_fraction() < 0.5f;
        Vector2F at(left ? 0.004f : 0.996f, 0.6f + roll_fraction() * 0.35f);
        persona.crack.push_back(at);
        for (int step = 0; step < 4; ++step)
        {
            at += Vector2F((left ? 1.0f : -1.0f) * (0.012f + roll_fraction() * 0.018f),
                (roll_fraction() - 0.6f) * 0.06f);
            persona.crack.push_back(at);
        }
    }
    return persona;
}

Monitor::Monitor(const OpeningPresentation* look, RenderResources* render_resources,
    MonitorPersona persona) :
    CrudeDrawing(look->sprite_sheet, look->fill_frame, render_resources),
    look_(look),
    persona_(std::move(persona))
{
}

const MonitorPersona& Monitor::persona() const
{
    return this->persona_;
}

RectangleF Monitor::screen_for_glass(const RectangleF& target) const
{
    const RectangleF& glass = this->look_->glass;
    const Vector2F size(target.width / glass.width, target.height / glass.height);
    return RectangleF(Vector2F(target.x - glass.x * size.x, target.y - glass.y * size.y), size);
}

RectangleF Monitor::glass(const RectangleF& screen) const
{
    const RectangleF& glass = this->look_->glass;
    return RectangleF(on_screen(screen, glass.position()),
        Vector2F(glass.width * screen.width, glass.height * screen.height));
}

RectangleF Monitor::program(const RectangleF& screen) const
{
    const RectangleF glass = this->glass(screen);
    const float inset = lip * screen.height;
    return RectangleF(glass.x + inset, glass.y + inset,
        glass.width - inset * 2.0f, glass.height - inset * 2.0f);
}

RectangleF Monitor::palette(const RectangleF& screen) const
{
    const RectangleF program = this->program(screen);
    const float height = this->look_->palette_height * this->glass(screen).height;
    return RectangleF(program.x, program.bottom() - height, program.width, height);
}

RectangleF Monitor::picture(const RectangleF& screen) const
{
    const RectangleF program = this->program(screen);
    const RectangleF palette = this->palette(screen);
    return RectangleF(program.x, program.y, program.width,
        std::max(1.0f, palette.y - program.y));
}

RectangleF Monitor::palette_swatch(const RectangleF& screen, size_t index) const
{
    const RectangleF palette = this->palette(screen);
    const float size = palette.height * 0.66f;
    const float gap = palette.height * 0.17f;
    return RectangleF(palette.x + gap + static_cast<float>(index) * (size + gap),
        palette.y + gap, size, size);
}

RectangleF Monitor::empty_slot(const RectangleF& screen) const
{
    // After the last grey: the slot the whole match is about.
    return this->palette_swatch(screen, this->look_->palette_greys.size());
}

RectangleF Monitor::swatch(const RectangleF& screen, int which) const
{
    const RectangleF slot = this->empty_slot(screen);
    const float size = slot.width * 1.25f;
    const float gap = slot.width * 0.3f;
    return RectangleF(slot.x + static_cast<float>(which) * (size + gap),
        slot.y - size - gap * 2.0f, size, size);
}

void Monitor::draw(DrawList& draw_list, const RectangleF& screen) const
{
    const OpeningPresentation& look = *this->look_;
    const float height = screen.height;
    this->fill(draw_list, screen, look.bezel_shades[this->persona_.shade % look.bezel_shades.size()]);

    // The bezel's inner lip, then the glass, then the program on it.
    const RectangleF glass = this->glass(screen);
    const float lip_width = lip * height;
    this->fill(draw_list, RectangleF(glass.x - lip_width, glass.y - lip_width,
        glass.width + lip_width * 2.0f, glass.height + lip_width * 2.0f),
        premultiplied(look.bezel_shadow));
    this->fill(draw_list, glass, look.glass_colour);
    this->fill(draw_list, this->program(screen), look.program_colour);

    const RectangleF palette = this->palette(screen);
    this->fill(draw_list, palette, look.glass_colour);
    for (size_t index = 0; index < look.palette_greys.size(); ++index)
    {
        this->fill(draw_list, this->palette_swatch(screen, index), look.palette_greys[index]);
    }
    this->frame_rectangle(draw_list, this->empty_slot(screen),
        std::max(1.0f, palette.height * 0.06f), look.slot_outline);

    this->draw_decorations(draw_list, screen);
}

void Monitor::draw_decorations(DrawList& draw_list, const RectangleF& screen) const
{
    const OpeningPresentation& look = *this->look_;
    const float height = screen.height;

    if (this->persona_.mug)
    {
        const Vector2F centre = on_screen(screen, this->persona_.coffee_ring);
        this->ring(draw_list, centre, coffee_radius * height, 0.008f * height,
            premultiplied(Colour(look.coffee.r, look.coffee.g, look.coffee.b, 0.55f)));
    }
    for (size_t index = 1; index < this->persona_.crack.size(); ++index)
    {
        this->line(draw_list, on_screen(screen, this->persona_.crack[index - 1]),
            on_screen(screen, this->persona_.crack[index]), 0.004f * height, look.crack);
    }
    for (const MonitorPersona::Decoration& sticker : this->persona_.stickers)
    {
        const float radius = sticker_size * height;
        this->blob(draw_list, on_screen(screen, sticker.at), Vector2F(radius),
            radius / 3.0f, look.stickers[sticker.colour % look.stickers.size()]);
    }
    for (const MonitorPersona::Decoration& note : this->persona_.sticky_notes)
    {
        // Square, stuck on a little crooked: the tilt shears the scribbles.
        const float size = note_size * height;
        const RectangleF paper(on_screen(screen, note.at), Vector2F(size));
        this->fill(draw_list, RectangleF(paper.position() + Vector2F(size * 0.06f),
            paper.size()), premultiplied(look.bezel_shadow));
        this->fill(draw_list, paper, look.sticky_notes[note.colour % look.sticky_notes.size()]);
        this->fill(draw_list, RectangleF(paper.x, paper.y, size, size * 0.14f),
            premultiplied(Colour(0.0f, 0.0f, 0.0f, 0.12f)));
        for (int row = 0; row < 3; ++row)
        {
            const float y = paper.y + size * (0.32f + 0.2f * static_cast<float>(row));
            const float length = size * (row == 2 ? 0.45f : 0.72f);
            this->line(draw_list, Vector2F(paper.x + size * 0.13f, y),
                Vector2F(paper.x + size * 0.13f + length, y + length * note.tilt),
                std::max(1.0f, size * 0.045f), look.scribble);
        }
    }
}

void Monitor::draw_picker(DrawList& draw_list, const RectangleF& screen,
    const Colour& first, const Colour& second) const
{
    const OpeningPresentation& look = *this->look_;
    const RectangleF a = this->swatch(screen, 0);
    const RectangleF b = this->swatch(screen, 1);
    const float pad = a.width * 0.2f;
    const RectangleF panel(a.x - pad, a.y - pad, b.right() - a.x + pad * 2.0f, a.height + pad * 2.0f);
    this->fill(draw_list, panel, look.glass_colour);
    this->frame_rectangle(draw_list, panel, std::max(1.0f, pad * 0.25f), look.slot_outline);
    this->fill(draw_list, a, first);
    this->fill(draw_list, b, second);
}

void Monitor::draw_preview(DrawList& draw_list, const RectangleF& area,
    const Colour& colour) const
{
    this->fill(draw_list, area, premultiplied(Colour(colour.r, colour.g, colour.b,
        this->look_->preview_strength)));
}
