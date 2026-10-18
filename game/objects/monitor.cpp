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

    RectangleF on_screen(const RectangleF& screen, const RectangleF& fraction)
    {
        return RectangleF(on_screen(screen, fraction.position()),
            Vector2F(fraction.width * screen.width, fraction.height * screen.height));
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
    constexpr float picker_padding = 0.2f; // of a picker swatch's width
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
    this->palette_swatches_.reserve(look->palette_frames.size());
    for (const std::string& frame : look->palette_frames)
    {
        this->palette_swatches_.emplace_back(look->palette_sheet, frame, render_resources);
    }
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
    // Keep the original square cells and spacing on wide windows; shrink
    // the whole row together when this kit has more slots than fit.
    const float slots = static_cast<float>(this->palette_swatches_.size() + 1);
    const float natural_size = palette.height * 0.66f;
    const float natural_gap = palette.height * 0.17f;
    const float fit = std::min(1.0f,
        palette.width / (slots * natural_size + (slots + 1.0f) * natural_gap));
    const float size = natural_size * fit;
    const float gap = natural_gap * fit;
    return RectangleF(palette.x + gap + static_cast<float>(index) * (size + gap),
        palette.y + (palette.height - size) * 0.5f, size, size);
}

RectangleF Monitor::empty_slot(const RectangleF& screen) const
{
    // After the last kit swatch: the slot the whole match is about.
    return this->palette_swatch(screen, this->palette_swatches_.size());
}

RectangleF Monitor::swatch(const RectangleF& screen, int which) const
{
    const RectangleF slot = this->empty_slot(screen);
    const float size = slot.width * 1.25f;
    const float gap = slot.width * 0.3f;
    const float pad = size * picker_padding;
    const RectangleF program = this->program(screen);
    const float left = std::clamp(slot.x, program.x + pad,
        std::max(program.x + pad, program.right() - size * 2.0f - gap - pad));
    return RectangleF(left + static_cast<float>(which) * (size + gap),
        this->picture(screen).bottom() - gap - size - pad, size, size);
}

RectangleF Monitor::close_button(const RectangleF& screen) const
{
    const RectangleF program = this->program(screen);
    const float size = this->palette(screen).height * 0.5f;
    const float gap = size * 0.25f;
    return RectangleF(program.right() - size - gap, program.y + gap, size, size);
}

void Monitor::draw(DrawList& draw_list, const RectangleF& screen) const
{
    const OpeningPresentation& look = *this->look_;
    const float height = screen.height;
    this->fill(draw_list, screen, look.bezel_shades[this->persona_.shade % look.bezel_shades.size()]);
    this->draw_bezel_shading(draw_list, screen);

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
    for (size_t index = 0; index < this->palette_swatches_.size(); ++index)
    {
        this->palette_swatches_[index].draw(draw_list, this->palette_swatch(screen, index));
    }
    this->frame_rectangle(draw_list, this->empty_slot(screen),
        std::max(1.0f, palette.height * 0.06f), look.slot_outline);
}

void Monitor::draw_bezel_shading(DrawList& draw_list, const RectangleF& screen) const
{
    const OpeningPresentation& look = *this->look_;
    const float width = look.monitor_art.shadow_width * screen.height;
    const RectangleF glass = this->glass(screen);
    // A few broad, square airbrush passes. Their overlap is fixed artwork,
    // with no sampling, randomness or work proportional to the window size.
    const Colour shade = premultiplied(Colour(look.bezel_shadow.r,
        look.bezel_shadow.g, look.bezel_shadow.b, look.bezel_shadow.a / 4.0f));
    for (int band = 4; band > 0; --band)
    {
        const float spread = width * static_cast<float>(band) / 4.0f;
        this->fill(draw_list, RectangleF(glass.x - spread * 0.2f,
            glass.y - spread * 0.2f, glass.width + spread * 1.2f,
            glass.height + spread * 1.2f), shade);
        this->fill(draw_list, RectangleF(screen.right() - spread, screen.y,
            spread, screen.height), shade);
        this->fill(draw_list, RectangleF(screen.x, screen.bottom() - spread,
            screen.width, spread), shade);
    }
}

void Monitor::draw_front_details(DrawList& draw_list, const RectangleF& screen) const
{
    const OpeningPresentation& look = *this->look_;
    const MonitorArtPresentation& art = look.monitor_art;
    const float pen = art.pen_width * screen.height;
    const float step = art.corner_step * screen.height;
    const RectangleF glass = this->glass(screen);

    this->frame_rectangle(draw_list, screen, pen, art.ink);
    // The highlight sits on the physical body, outside the glass. Square
    // starts and stops keep the oversized pen visible at every display size.
    this->fill(draw_list, RectangleF(screen.x + step, screen.y + pen * 2.0f,
        screen.width - step * 2.0f, pen), art.highlight);
    this->fill(draw_list, RectangleF(screen.x + pen * 2.0f, screen.y + step,
        pen, screen.height - step * 2.0f), art.highlight);
    this->frame_rectangle(draw_list, RectangleF(glass.x - pen, glass.y - pen,
        glass.width + pen * 2.0f, glass.height + pen * 2.0f), pen, art.ink);
    this->fill(draw_list, RectangleF(glass.x - pen * 2.0f, glass.bottom() + pen,
        glass.width + pen * 3.0f, pen), art.highlight);
    this->fill(draw_list, RectangleF(glass.right() + pen, glass.y,
        pen, glass.height + pen), art.highlight);
    // Stepped corner nicks stay outside the glass, clear of picture().
    for (const Vector2F& corner : { glass.position(),
        Vector2F(glass.right() - step, glass.y),
        Vector2F(glass.x, glass.bottom() - step),
        Vector2F(glass.right() - step, glass.bottom() - step) })
    {
        const bool right = corner.x > glass.x;
        const bool bottom = corner.y > glass.y;
        this->fill(draw_list, RectangleF(corner.x,
            bottom ? glass.bottom() + pen : glass.y - pen * 2.0f, step, pen), art.ink);
        this->fill(draw_list, RectangleF(right ? glass.right() + pen : glass.x - pen * 2.0f,
            corner.y, pen, step), art.ink);
    }

    const RectangleF vents = on_screen(screen, art.vent_area);
    const float pitch = vents.height / static_cast<float>(art.vent_count);
    for (int index = 0; index < art.vent_count; ++index)
    {
        const float y = vents.y + pitch * (static_cast<float>(index) + 0.5f);
        this->fill(draw_list, RectangleF(vents.x + pen, y + pen,
            vents.width - pen * 2.0f, pen), art.highlight);
        this->fill(draw_list, RectangleF(vents.x, y,
            vents.width, pen), art.ink);
    }
    const RectangleF button = on_screen(screen, art.power_button);
    this->fill(draw_list, button, art.ink);
    this->fill(draw_list, RectangleF(button.x + pen, button.y + pen,
        button.width - pen * 2.0f, button.height - pen * 2.0f),
        look.bezel_shades[this->persona_.shade % look.bezel_shades.size()]);
    this->fill(draw_list, RectangleF(button.x + pen * 2.0f, button.y + pen,
        button.width - pen * 4.0f, pen), art.highlight);
    // A crude power glyph is a U with a short straight switch stroke.
    const float glyph = std::min(button.width, button.height) * 0.4f;
    const Vector2F centre = button.center();
    this->fill(draw_list, RectangleF(centre.x - glyph / 2.0f, centre.y,
        pen, glyph / 2.0f), art.ink);
    this->fill(draw_list, RectangleF(centre.x + glyph / 2.0f - pen, centre.y,
        pen, glyph / 2.0f), art.ink);
    this->fill(draw_list, RectangleF(centre.x - glyph / 2.0f,
        centre.y + glyph / 2.0f - pen, glyph, pen), art.ink);
    this->fill(draw_list, RectangleF(centre.x - pen / 2.0f,
        centre.y - glyph / 2.0f, pen, glyph / 2.0f), art.ink);
    const RectangleF indicator = on_screen(screen, art.power_indicator);
    this->fill(draw_list, indicator, art.ink);
    this->fill(draw_list, RectangleF(indicator.x + pen, indicator.y + pen,
        indicator.width - pen * 2.0f, indicator.height - pen * 2.0f), art.power_light);

    this->draw_decorations(draw_list, screen);
}

void Monitor::draw_decorations(DrawList& draw_list, const RectangleF& screen) const
{
    const OpeningPresentation& look = *this->look_;
    const float height = screen.height;
    const float pen = look.monitor_art.pen_width * height;

    if (this->persona_.mug)
    {
        const Vector2F centre = on_screen(screen, this->persona_.coffee_ring);
        this->ring(draw_list, centre, coffee_radius * height, 0.008f * height,
            premultiplied(Colour(look.coffee.r, look.coffee.g, look.coffee.b, 0.55f)));
    }
    for (size_t index = 1; index < this->persona_.crack.size(); ++index)
    {
        this->line(draw_list, on_screen(screen, this->persona_.crack[index - 1]),
            on_screen(screen, this->persona_.crack[index]), pen, look.monitor_art.ink);
        this->line(draw_list, on_screen(screen, this->persona_.crack[index - 1]),
            on_screen(screen, this->persona_.crack[index]), pen * 0.45f, look.crack);
    }
    for (const MonitorPersona::Decoration& sticker : this->persona_.stickers)
    {
        const float radius = sticker_size * height;
        this->blob(draw_list, on_screen(screen, sticker.at), Vector2F(radius),
            radius / 3.0f, look.monitor_art.ink);
        this->blob(draw_list, on_screen(screen, sticker.at), Vector2F(radius - pen),
            radius / 3.0f, look.stickers[sticker.colour % look.stickers.size()]);
    }
    for (const MonitorPersona::Decoration& note : this->persona_.sticky_notes)
    {
        // Square, stuck on a little crooked: the tilt shears the scribbles.
        const float size = note_size * height;
        const RectangleF paper(on_screen(screen, note.at), Vector2F(size));
        this->fill(draw_list, RectangleF(paper.position() + Vector2F(size * 0.06f),
            paper.size()), premultiplied(look.bezel_shadow));
        this->fill(draw_list, paper, look.monitor_art.ink);
        this->fill(draw_list, RectangleF(paper.x + pen, paper.y + pen,
            size - pen * 2.0f, size - pen * 2.0f),
            look.sticky_notes[note.colour % look.sticky_notes.size()]);
        this->fill(draw_list, RectangleF(paper.x + pen, paper.y + pen,
            size - pen * 2.0f, size * 0.14f),
            premultiplied(Colour(0.0f, 0.0f, 0.0f, 0.12f)));
        this->fill(draw_list, RectangleF(paper.x + pen * 2.0f,
            paper.bottom() - pen * 2.0f, size - pen * 4.0f, pen), look.monitor_art.highlight);
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
    const float pad = a.width * picker_padding;
    const RectangleF panel(a.x - pad, a.y - pad, b.right() - a.x + pad * 2.0f, a.height + pad * 2.0f);
    this->fill(draw_list, panel, look.glass_colour);
    this->frame_rectangle(draw_list, panel, std::max(1.0f, pad * 0.25f), look.slot_outline);
    this->fill(draw_list, a, first);
    this->fill(draw_list, b, second);
}

void Monitor::draw_saved(DrawList& draw_list, const RectangleF& screen,
    const Colour& colour) const
{
    const RectangleF slot = this->empty_slot(screen);
    this->fill(draw_list, slot, colour);
    this->frame_rectangle(draw_list, slot, std::max(1.0f, slot.height * 0.08f),
        this->look_->slot_outline);
}

void Monitor::draw_close_button(DrawList& draw_list, const RectangleF& screen) const
{
    const OpeningPresentation& look = *this->look_;
    const RectangleF button = this->close_button(screen);
    const float pen = std::max(1.0f, button.width * 0.1f);
    this->fill(draw_list, button, look.glass_colour);
    this->frame_rectangle(draw_list, button, pen, look.slot_outline);
    const float in = button.width * 0.28f;
    this->line(draw_list, Vector2F(button.x + in, button.y + in),
        Vector2F(button.right() - in, button.bottom() - in), pen, look.slot_outline);
    this->line(draw_list, Vector2F(button.right() - in, button.y + in),
        Vector2F(button.x + in, button.bottom() - in), pen, look.slot_outline);
}

void Monitor::draw_closed(DrawList& draw_list, const RectangleF& screen) const
{
    this->fill(draw_list, this->program(screen), this->look_->desktop_colour);
}

void Monitor::draw_preview(DrawList& draw_list, const RectangleF& area,
    const Colour& colour) const
{
    this->fill(draw_list, area, premultiplied(Colour(colour.r, colour.g, colour.b,
        this->look_->preview_strength)));
}
