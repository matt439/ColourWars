#pragma once

#include "game/objects/crude_drawing.h"
#include "game/config/presentation/presentation_match.h"
#include "game/ai/bot_random.h"

#include <vector>

// Whose computer this match is on: its body shade, what is stuck to it, and the
// person who uses it. One is rolled per match, so a rematch is somewhere else
// (docs/backstory.md, "Rematch"). Positions are fractions of the screen.
struct MonitorPersona
{
    struct Decoration
    {
        labrador::Vector2F at;
        size_t colour = 0;
        float tilt = 0.0f;
    };

    size_t shade = 0;
    // A small silhouette, more stickers; an adult may have a mug, and so a
    // coffee ring on the bezel. The bezel and the silhouette come from this
    // one kit, so they always agree.
    bool child = false;
    bool mug = false;
    std::vector<Decoration> sticky_notes;
    std::vector<Decoration> stickers;
    labrador::Vector2F coffee_ring;
    std::vector<labrador::Vector2F> crack;

    // Draws from the round's persona stream (MatchRandom::persona), so the
    // same round seed rolls the same person whatever else the build drew.
    static MonitorPersona roll(const OpeningPresentation& look, BotRandom& random);
};

// The person's monitor, the paint program on it and its palette: one full
// row of the level kit's swatches and the one empty slot the match is about.
// The picture itself is the match's world, drawn by the scene into picture().
//
// Every rectangle is laid out in `screen`, the rectangle the whole screen is
// mapped to: the window at rest, and a larger one while the view pushes
// through the glass, so the bezel grows past the edges with the picture.
class Monitor final : public CrudeDrawing
{
public:
    // The presentation is borrowed and outlives the level.
    Monitor(const OpeningPresentation* look, labrador::RenderResources* render_resources,
        MonitorPersona persona);

    const MonitorPersona& persona() const;

    // The screen rectangle that puts the glass exactly over `target`.
    labrador::RectangleF screen_for_glass(const labrador::RectangleF& target) const;

    labrador::RectangleF glass(const labrador::RectangleF& screen) const;
    // Where the picture is shown, above the palette.
    labrador::RectangleF picture(const labrador::RectangleF& screen) const;
    // The two colours being tried, in a little picker over the empty slot.
    labrador::RectangleF swatch(const labrador::RectangleF& screen, int which) const;
    labrador::RectangleF empty_slot(const labrador::RectangleF& screen) const;
    // The program's close button, in the top corner of the glass.
    labrador::RectangleF close_button(const labrador::RectangleF& screen) const;

    // The bezel, the glass, the program and its palette, behind the world.
    void draw(labrador::DrawList& draw_list, const labrador::RectangleF& screen) const;
    // Physical keylines, controls and decorations, above the world/preview but
    // below its cursor and the person. Notes may overlap the glass's edge.
    void draw_front_details(labrador::DrawList& draw_list,
        const labrador::RectangleF& screen) const;
    void draw_picker(labrador::DrawList& draw_list, const labrador::RectangleF& screen,
        const labrador::Colour& first, const labrador::Colour& second) const;
    // The empty slot, filled: the colour has been saved.
    void draw_saved(labrador::DrawList& draw_list, const labrador::RectangleF& screen,
        const labrador::Colour& colour) const;
    void draw_close_button(labrador::DrawList& draw_list, const labrador::RectangleF& screen) const;
    // The program closed without saving: the glass shows the desktop.
    void draw_closed(labrador::DrawList& draw_list, const labrador::RectangleF& screen) const;
    // Shows `colour` over part of the picture, in whatever space the list is in.
    void draw_preview(labrador::DrawList& draw_list, const labrador::RectangleF& area,
        const labrador::Colour& colour) const;

private:
    // TextureObject keeps positioned drawing protected. These swatches are
    // fixed atlas frames with WHITE tint, resolved once for this monitor.
    class PaletteSwatch final : public labrador::TextureObject
    {
    public:
        using TextureObject::TextureObject;
        using TextureObject::draw;
    };

    const OpeningPresentation* look_;
    MonitorPersona persona_;
    std::vector<PaletteSwatch> palette_swatches_;

    labrador::RectangleF program(const labrador::RectangleF& screen) const;
    labrador::RectangleF palette(const labrador::RectangleF& screen) const;
    labrador::RectangleF palette_swatch(const labrador::RectangleF& screen, size_t index) const;
    void draw_bezel_shading(labrador::DrawList& draw_list,
        const labrador::RectangleF& screen) const;
    void draw_decorations(labrador::DrawList& draw_list, const labrador::RectangleF& screen) const;
};
