#include "game/pch.h"
#include "game/objects/countdown_art.h"

#include <array>

using namespace labrador;
using namespace mattmath;

namespace
{
    using CountdownGlyph = std::array<std::string_view, 9>;
    // Editable mouse-drawn strokes on seven-by-nine coarse canvases. The
    // uneven shoulders and feet are intentional; this is not a font atlas.
    constexpr std::array<CountdownGlyph, 13> glyphs = {{
        { ".####..", "######.", "##..###", "##...##", "##...##", "##...##", "###..##", ".######", "..####." },
        { "...##..", "..###..", ".####..", "...##..", "...##..", "...##..", "...##..", ".#####.", ".#####." },
        { ".####..", "######.", "....###", "....###", "...###.", "..###..", ".###...", "#######", "#######" },
        { "#####..", "######.", "....###", "...###.", "..####.", "....###", "....###", "######.", "#####.." },
        { "....##.", "...###.", "..####.", ".##.##.", "##..##.", "#######", "#######", "....##.", "....##." },
        { "#######", "#######", "##.....", "######.", "#######", ".....##", ".....##", "######.", "#####.." },
        { "..####.", ".#####.", "###....", "##.....", "######.", "#######", "##...##", ".######", "..####." },
        { "#######", "#######", "....###", "....##.", "...###.", "...##..", "..###..", "..##...", "..##..." },
        { ".####..", "######.", "##..###", ".#####.", "..###..", ".#####.", "###..##", ".######", "..####." },
        { ".####..", "######.", "##...##", "#######", ".######", ".....##", "....###", ".#####.", ".####.." },
        { "..####.", ".#####.", "###....", "##.....", "##.####", "##.####", "###..##", ".######", "..####." },
        { ".####..", "######.", "##..###", "##...##", "##...##", "##...##", "###..##", ".######", "..####." },
        { "..###..", "..###..", "..###..", "..###..", "..###..", "...##..", ".......", "..###..", "..###.." }
    }};

    const CountdownGlyph* glyph(wchar_t letter)
    {
        if (letter >= L'0' && letter <= L'9') { return &glyphs[letter - L'0']; }
        if (letter == L'G') { return &glyphs[10]; }
        if (letter == L'O') { return &glyphs[11]; }
        if (letter == L'!') { return &glyphs[12]; }
        return nullptr;
    }
}

CountdownArt::CountdownArt(const MatchPresentation* look, const OpeningPresentation* source,
    RenderResources* render_resources) :
    CrudeDrawing(source->sprite_sheet, source->fill_frame, render_resources), look_(look)
{
}

void CountdownArt::draw_card(DrawList& list, const RectangleF& bounds, float scale) const
{
    const CountdownArtPresentation& art = this->look_->countdown_art;
    const float pen = art.keyline_width * scale;
    const float step = pen * 2.0f;
    const auto paper = [&](const RectangleF& box, const Colour& colour)
    {
        this->fill(list, RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height), colour);
        this->fill(list, RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f), colour);
    };
    const float shadow = art.shadow_offset * scale;
    // Reserve the lower/right edge inside the card's reported occupied bounds.
    const RectangleF face(bounds.x, bounds.y, bounds.width - shadow, bounds.height - shadow);
    paper(RectangleF(face.position() + Vector2F(shadow), face.size()), premultiplied(art.shade));
    paper(face, premultiplied(art.ink));
    const RectangleF inside(face.x + pen, face.y + pen, face.width - pen * 2.0f, face.height - pen * 2.0f);
    paper(inside, premultiplied(art.paper));
    // Quiet airbrush passes inside the lower/right paper edge.
    for (int pass = 3; pass > 0; --pass)
    {
        Colour shade = art.shade;
        shade.a *= 0.12f;
        const float width = pen * static_cast<float>(pass);
        this->fill(list, RectangleF(inside.x + step, inside.bottom() - width,
            inside.width - step * 2.0f, width), premultiplied(shade));
        this->fill(list, RectangleF(inside.right() - width, inside.y + step,
            width, inside.height - step * 2.0f), premultiplied(shade));
    }
}

void CountdownArt::draw(DrawList& list, const RectangleF& bounds,
    std::wstring_view text, float beat_age, bool reduced_effects) const
{
    if (text.empty() || text.size() > 12 || bounds.width <= 0.0f || bounds.height <= 0.0f) { return; }
    const CountdownArtPresentation& art = this->look_->countdown_art;
    const bool go = text == L"GO!";
    float beat = 0.0f;
    if (!reduced_effects && beat_age >= 0.0f && beat_age < art.beat_duration)
    {
        const float remaining = 1.0f - beat_age / art.beat_duration;
        beat = remaining * remaining;
    }
    // Four spare cells each side include outline, shade and the brief pen
    // ticks. Settled size reserves room for the maximum authored pulse.
    const float columns = static_cast<float>(text.size()) * 8.0f - 1.0f;
    const float cell = std::min(bounds.width / (columns + 8.0f), bounds.height / 17.0f) *
        (1.0f + art.pulse * beat) / (1.0f + art.pulse);
    const Vector2F origin = bounds.center() - Vector2F(columns * cell, 9.0f * cell) / 2.0f;
    const float outline = cell * 0.45f;
    const Colour ink = premultiplied(art.ink);
    const Colour fill_colour = premultiplied(go ? art.go : art.number);
    const Colour shade = premultiplied(art.shade);
    // Complete silhouette passes, rather than per-dab outlines that would
    // leave seams across the wide hand-drawn strokes.
    for (int pass = 0; pass < 3; ++pass)
    {
        float x = origin.x;
        for (const wchar_t letter : text)
        {
            const CountdownGlyph* cells = glyph(letter);
            if (!cells) { return; }
            for (size_t y = 0; y < cells->size(); ++y)
            {
                const std::string_view row = (*cells)[y];
                for (size_t column = 0; column < row.size();)
                {
                    const size_t start = column++;
                    while (column < row.size() && row[column] == row[start]) { ++column; }
                    if (row[start] != '#') { continue; }
                    RectangleF dab(x + static_cast<float>(start) * cell,
                        origin.y + static_cast<float>(y) * cell,
                        static_cast<float>(column - start) * cell, cell);
                    if (pass == 0)
                    {
                        dab = RectangleF(dab.x - outline + cell * 0.6f, dab.y - outline + cell * 0.6f,
                            dab.width + outline * 2.0f, dab.height + outline * 2.0f);
                    }
                    else if (pass == 1)
                    {
                        dab = RectangleF(dab.x - outline, dab.y - outline,
                            dab.width + outline * 2.0f, dab.height + outline * 2.0f);
                    }
                    this->fill(list, dab, pass == 0 ? shade : (pass == 1 ? ink : fill_colour));
                }
            }
            x += cell * 8.0f;
        }
    }
    if (beat > 0.0f)
    {
        // Detached rectangle-tool marks; no fullscreen flash or camera shake.
        const Vector2F centre = bounds.center();
        const float reach = (go ? 1.8f : 1.5f) * cell;
        for (const float side : { -1.0f, 1.0f })
        {
            const float x = centre.x + side * (columns * cell / 2.0f + cell * 1.8f);
            for (const float row : { -1.0f, 1.0f })
            {
                const Vector2F from(x, centre.y + row * cell * 2.8f);
                const Vector2F to = from + Vector2F(side * reach, row * cell * 0.5f) * beat;
                this->line(list, from, to, cell * 0.65f, ink);
                this->line(list, from, to, cell * 0.25f, fill_colour);
            }
        }
    }
}
