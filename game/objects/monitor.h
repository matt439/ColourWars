#pragma once

#include "game/objects/crude_drawing.h"
#include "game/presentation.h"

#include <vector>

// Whose computer this match is on: a beige, what is stuck to it, and the
// person who uses it. One is rolled per match, so a rematch is somewhere else
// (docs/backstory.md, "Rematch"). Positions are fractions of the screen.
struct MonitorPersona
{
    struct Decoration
    {
        mattmath::Vector2F at;
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
    mattmath::Vector2F coffee_ring;
    std::vector<mattmath::Vector2F> crack;

    // Draws from rand(), after the build's own draws, so `seed` repeats it.
    static MonitorPersona roll(const OpeningPresentation& look);
};

// The person's monitor, the paint program on it and its palette: one full
// row of greys and the one empty slot the whole match is about. The picture
// itself is the match's world, drawn by the scene into picture().
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
    mattmath::RectangleF screen_for_glass(const mattmath::RectangleF& target) const;

    mattmath::RectangleF glass(const mattmath::RectangleF& screen) const;
    // Where the picture is shown, above the palette.
    mattmath::RectangleF picture(const mattmath::RectangleF& screen) const;
    // The two colours being tried, in a little picker over the empty slot.
    mattmath::RectangleF swatch(const mattmath::RectangleF& screen, int which) const;
    mattmath::RectangleF empty_slot(const mattmath::RectangleF& screen) const;

    // The bezel, the glass, the program and its palette, and what is stuck on.
    void draw(labrador::DrawList& draw_list, const mattmath::RectangleF& screen) const;
    void draw_picker(labrador::DrawList& draw_list, const mattmath::RectangleF& screen,
        const labrador::Colour& first, const labrador::Colour& second) const;
    // The empty slot, filled: the colour has been saved.
    void draw_saved(labrador::DrawList& draw_list, const mattmath::RectangleF& screen,
        const labrador::Colour& colour) const;
    // Shows `colour` over part of the picture, in whatever space the list is in.
    void draw_preview(labrador::DrawList& draw_list, const mattmath::RectangleF& area,
        const labrador::Colour& colour) const;

private:
    const OpeningPresentation* look_;
    MonitorPersona persona_;

    mattmath::RectangleF program(const mattmath::RectangleF& screen) const;
    mattmath::RectangleF palette(const mattmath::RectangleF& screen) const;
    mattmath::RectangleF palette_swatch(const mattmath::RectangleF& screen, size_t index) const;
    void draw_decorations(labrador::DrawList& draw_list, const mattmath::RectangleF& screen) const;
};
