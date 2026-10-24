// Fields56-58: one isolated shared HUD paper family. Existing fixtures stay
// byte-identical; the independent oracle is bounded to35 material sites.
enum class HudPaperReference { correct, wrong_ink, omitted_ink, wrong_paper, omitted_paper,
    straight, squared, black_band, no_scale, missing_bottom, missing_right };
using HudPaperTuple = std::array<Colour, 3>;
HudPaperTuple hud_paper_values(const HudPresentation& look) { return { look.paper.ink, look.paper.paper, look.paper.shade }; }
void set_hud_paper_values(HudPresentation& look, const HudPaperTuple& value)
{ look.paper.ink = value[0]; look.paper.paper = value[1]; look.paper.shade = value[2]; }
const std::array<const char*, 3>& hud_paper_keys()
{ static const std::array<const char*, 3> keys{ "hud.paper.ink", "hud.paper.paper", "hud.paper.shade" }; return keys; }
struct HudPaperSite { const char* name; RectangleI box; size_t panel, material, layers; };
const std::array<HudPaperSite, 35>& hud_paper_sites()
{
    static const std::array<HudPaperSite, 35> sites{ {
        { "identity-ink-first", { 400, 44, 8, 2 }, 0, 0, 1 },
        { "identity-ink-second", { 41, 60, 2, 8 }, 0, 0, 1 },
        { "identity-paper-first-flat", { 400, 46, 8, 2 }, 0, 1, 1 },
        { "identity-shade-base-first", { 400, 182, 8, 2 }, 0, 2, 1 },
        { "identity-shade-base-second", { 527, 80, 2, 8 }, 0, 2, 1 },
        { "identity-shade-bottom-one", { 400, 173, 8, 2 }, 0, 3, 1 },
        { "identity-shade-bottom-two", { 400, 175, 8, 2 }, 0, 3, 2 },
        { "identity-shade-bottom-three", { 400, 177, 8, 2 }, 0, 3, 3 },
        { "identity-shade-right-one", { 517, 80, 2, 8 }, 0, 3, 1 },
        { "identity-shade-right-two", { 519, 80, 2, 8 }, 0, 3, 2 },
        { "identity-shade-right-three", { 521, 80, 2, 8 }, 0, 3, 3 },
        { "meters-ink-first", { 1580, 44, 8, 2 }, 1, 0, 1 },
        { "meters-ink-second", { 1539, 60, 2, 8 }, 1, 0, 1 },
        { "meters-paper-first-flat", { 1580, 46, 8, 2 }, 1, 1, 1 },
        { "meters-shade-base-first", { 1580, 218, 8, 2 }, 1, 2, 1 },
        { "meters-shade-base-second", { 1860, 80, 2, 8 }, 1, 2, 1 },
        { "meters-shade-bottom-one", { 1580, 209, 8, 2 }, 1, 3, 1 },
        { "meters-shade-bottom-two", { 1580, 211, 8, 2 }, 1, 3, 2 },
        { "meters-shade-bottom-three", { 1580, 213, 8, 2 }, 1, 3, 3 },
        { "meters-shade-right-one", { 1851, 80, 2, 8 }, 1, 3, 1 },
        { "meters-shade-right-two", { 1853, 80, 2, 8 }, 1, 3, 2 },
        { "meters-shade-right-three", { 1855, 80, 2, 8 }, 1, 3, 3 },
        { "timer-ink-first", { 900, 44, 8, 2 }, 2, 0, 1 },
        { "timer-ink-second", { 879, 60, 2, 8 }, 2, 0, 1 },
        { "timer-paper-first-flat", { 900, 46, 8, 2 }, 2, 1, 1 },
        { "timer-shade-base-first", { 900, 139, 8, 2 }, 2, 2, 1 },
        { "timer-shade-base-second", { 1042, 80, 2, 8 }, 2, 2, 1 },
        { "timer-shade-bottom-one", { 900, 130, 8, 2 }, 2, 3, 1 },
        { "timer-shade-bottom-two", { 900, 132, 8, 2 }, 2, 3, 2 },
        { "timer-shade-bottom-three", { 900, 134, 8, 2 }, 2, 3, 3 },
        { "timer-shade-right-one", { 1033, 80, 2, 8 }, 2, 3, 1 },
        { "timer-shade-right-two", { 1035, 80, 2, 8 }, 2, 3, 2 },
        { "timer-shade-right-three", { 1037, 80, 2, 8 }, 2, 3, 3 },
        { "meters-health-direct-ink-border", { 1580, 95, 8, 2 }, 1, 0, 1 },
        { "meters-paint-direct-ink-border", { 1580, 181, 8, 2 }, 1, 0, 1 },
    } }; return sites;
}
const std::array<HudPaperTuple, 6>& hud_paper_palettes()
{
    static const std::array<HudPaperTuple, 6> values{ {
        { Colour(20, 20, 20), Colour(255, 245, 223), Colour(0, 0, 0, 85) },
        { Colour(255, 0, 0), Colour(255, 245, 223), Colour(0, 0, 0, 85) },
        { Colour(20, 20, 20), Colour(53, 188, 225), Colour(0, 0, 0, 85) },
        { Colour(20, 20, 20), Colour(255, 245, 223), Colour(255, 0, 0) },
        { Colour(20, 20, 20), Colour(255, 245, 223), Colour(32, 96, 192, 128) },
        { Colour(255, 0, 0), Colour(53, 188, 225), Colour(32, 96, 192, 128) },
    } }; return values;
}
const std::array<std::vector<Glyph>, 3>& hud_paper_literal_glyphs()
{
    static const std::array<std::vector<Glyph>, 3> glyphs{ {
        std::vector<Glyph>{
            { static_cast<char32_t>(65), { 145, 31, 25, 23 }, 5.0f, 11.0f, -5.0f },
            { static_cast<char32_t>(69), { 49, 83, 17, 23 }, 7.0f, 11.0f, -4.0f },
            { static_cast<char32_t>(72), { 226, 51, 23, 23 }, 7.0f, 11.0f, -3.0f },
            { static_cast<char32_t>(73), { 211, 126, 6, 23 }, 7.0f, 11.0f, -2.0f },
            { static_cast<char32_t>(76), { 164, 80, 18, 23 }, 7.0f, 11.0f, -5.0f },
            { static_cast<char32_t>(78), { 172, 55, 23, 23 }, 7.0f, 11.0f, -3.0f },
            { static_cast<char32_t>(80), { 123, 81, 18, 23 }, 7.0f, 11.0f, -4.0f },
            { static_cast<char32_t>(84), { 99, 58, 22, 23 }, 6.0f, 11.0f, -5.0f },
        },
        std::vector<Glyph>{
            { static_cast<char32_t>(48), { 311, 57, 31, 47 }, 12.0f, 21.0f, -8.0f },
            { static_cast<char32_t>(49), { 140, 112, 11, 45 }, 22.0f, 22.0f, 2.0f },
            { static_cast<char32_t>(50), { 262, 109, 30, 46 }, 13.0f, 21.0f, -8.0f },
            { static_cast<char32_t>(52), { 223, 204, 31, 45 }, 12.0f, 22.0f, -8.0f },
            { static_cast<char32_t>(58), { 322, 154, 12, 33 }, 13.0f, 35.0f, -8.0f },
        },
        std::vector<Glyph>{
            { static_cast<char32_t>(32), { 505, 1, 1, 1 }, 31.0f, 92.0f, -19.0f },
            { static_cast<char32_t>(47), { 331, 110, 13, 34 }, 8.0f, 17.0f, -8.0f },
            { static_cast<char32_t>(49), { 193, 116, 8, 34 }, 17.0f, 17.0f, 1.0f },
            { static_cast<char32_t>(65), { 406, 37, 38, 34 }, 8.0f, 17.0f, -9.0f },
            { static_cast<char32_t>(69), { 486, 37, 25, 34 }, 11.0f, 17.0f, -6.0f },
            { static_cast<char32_t>(75), { 237, 74, 32, 34 }, 11.0f, 17.0f, -8.0f },
            { static_cast<char32_t>(77), { 266, 38, 36, 34 }, 11.0f, 17.0f, -5.0f },
            { static_cast<char32_t>(80), { 194, 80, 26, 34 }, 12.0f, 17.0f, -7.0f },
            { static_cast<char32_t>(84), { 413, 73, 33, 34 }, 9.0f, 17.0f, -7.0f },
            { static_cast<char32_t>(97), { 396, 145, 25, 24 }, 9.0f, 27.0f, -9.0f },
            { static_cast<char32_t>(98), { 128, 84, 25, 34 }, 10.0f, 17.0f, -7.0f },
            { static_cast<char32_t>(100), { 155, 84, 25, 34 }, 9.0f, 17.0f, -6.0f },
            { static_cast<char32_t>(101), { 450, 145, 24, 24 }, 9.0f, 27.0f, -7.0f },
            { static_cast<char32_t>(109), { 143, 120, 41, 24 }, 10.0f, 27.0f, -5.0f },
            { static_cast<char32_t>(111), { 368, 145, 26, 24 }, 9.0f, 27.0f, -6.0f },
            { static_cast<char32_t>(114), { 237, 146, 19, 24 }, 11.0f, 27.0f, -8.0f },
            { static_cast<char32_t>(115), { 258, 146, 19, 24 }, 9.0f, 27.0f, -8.0f },
            { static_cast<char32_t>(117), { 143, 146, 24, 24 }, 10.0f, 27.0f, -6.0f },
            { static_cast<char32_t>(121), { 379, 1, 25, 35 }, 8.0f, 27.0f, -9.0f },
        },
    } }; return glyphs;
}
std::vector<float> hud_paper_fixed_style(const HudPresentation& look)
{
    auto copy = look; set_hud_paper_values(copy, hud_paper_palettes()[0]);
    auto values = hud_track_fixed_style(copy); const auto& track = look.meter_track_colour;
    values.insert(values.end(), { track.r, track.g, track.b, track.a });
    return values; // Only selected shared-paper3 is masked; all55 styles remain.
}
using HudPaperFrames = std::array<std::array<Frame, 2>, 6>;
class HudPaperColourProbe
{
public:
    HudPaperColourProbe(Application& app, const Presentation& presentation, const std::filesystem::path& output, size_t budget) :
        app_(app), presentation_(presentation), look_(presentation.hud()), identity_(presentation.player_identity()),
        expected_(look_), expected_identity_(identity_), hud_(app.render_resources(), &look_, &identity_), copied_(app.render_resources(), &expected_, &expected_identity_),
        black_(app.render_resources(), Colour::black), nonblack_(app.render_resources(), Colour(32, 64, 96)),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)), pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))),
        fixed_(hud_paper_fixed_style(look_)), identity_fixed_(hud_track_identity_style(identity_)), names_(hud_track_resource_names(look_)),
        palette_(presentation.team_palette()), output_(output), budget_(budget)
    {
        require(hud_paper_values(look_) == hud_paper_palettes()[0] && look_.paper.keyline_width == 2 && look_.paper.shadow_offset == 4 &&
            look_.player_identity_reference_width == 1920 && look_.reference_height == 1080 && look_.timer_reference_width == 1920 &&
            look_.player_identity_offset == Vector2F(.03f, .05f) && look_.player_identity_padding == Vector2F(16, 10) &&
            look_.player_identity_shadow_offset == Vector2F(2, 2) && look_.timer_offset == Vector2F(0, .05f) && look_.timer_shadow_offset == Vector2F(2, 2) &&
            look_.right_margin == .19f && look_.health_top_margin == .09f && look_.ammo_top_margin == .17f && look_.box_size == Vector2F(.15f, .022f) &&
            look_.meter_label_gap == 6 && look_.box_fill_border == .002f && identity_.enabled && identity_.ui_size == Vector2F(54, 42) && identity_.ui_gap == 12 &&
            identity_.background_colour == Colour::white && identity_.ink_colour == Colour::black &&
            look_.health_colour == Colour(118, 232, 181) && look_.meter_track_colour == Colour(184, 184, 184) &&
            look_.meter_label_font == "gill_sans_mt_bold_24" && look_.timer_font == "gill_sans_mt_bold_48" && look_.player_identity_font == "gill_sans_mt_bold_36" &&
            look_.sprite_sheet == "sprite_sheet_1" && look_.fill_frame == "pixel",
            "HUD paper source defaults, exact geometry, opaque overlays or resource names changed");
        const std::array<std::string, 3> names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        const std::array<std::wstring, 3> words{ L"HEALTHPAINT", L"2:040:10", L"P1  /  TEAM A\nKeyboard/mouse" };
        for (size_t i = 0; i < names.size(); ++i)
        {
            fonts_[i] = app_.render_resources()->resolve_sprite_font(names[i]); font_pointers_[i] = app_.render_resources()->font(fonts_[i]);
            glyphs_[i] = hud_paper_literal_glyphs()[i];
            require(font_pointers_[i]->first_unrenderable(words[i]) == std::wstring_view::npos, "HUD paper literal text requires exact ordinal glyph coverage");
        }
        unchanged(); geometry();
    }
    void unchanged() const
    {
        auto* resources = app_.render_resources();
        require(&look_ == &presentation_.hud() && &identity_ == &presentation_.player_identity() &&
            hud_paper_fixed_style(look_) == fixed_ && hud_paper_fixed_style(expected_) == fixed_ &&
            hud_track_identity_style(identity_) == identity_fixed_ && hud_track_identity_style(expected_identity_) == identity_fixed_ &&
            hud_track_resource_names(look_) == names_ && hud_track_resource_names(expected_) == names_ &&
            hud_track_same_palette(presentation_.team_palette(), palette_) && resources->sprite_sheet(look_.sprite_sheet) == sheet_ &&
            &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) == &pixel_ && pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) &&
            pixel_.origin() == Vector2F::ZERO && app_.viewport_manager()->player_viewport(0).pixel_rect() == RectangleI(0, 0, 1920, 1080),
            "HUD paper edit changed unselected style including closing55, identity, palette, resource, geometry or viewport");
        const std::array<std::string, 3> names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        for (size_t i = 0; i < names.size(); ++i)
        {
            require(fonts_[i] == resources->resolve_sprite_font(names[i]) && font_pointers_[i] == resources->font(fonts_[i]), "HUD paper font resource changed");
            for (const auto& held : glyphs_[i])
            {
                const auto* value = font_pointers_[i]->find(held.character);
                require(value && value->subrect == held.subrect && value->x_offset == held.x_offset && value->y_offset == held.y_offset && value->x_advance == held.x_advance,
                    "HUD paper ordinal glyph source metadata changed");
            }
        }
        require(font_pointers_[0]->line_spacing() == 37.109375f && font_pointers_[1]->line_spacing() == 74.21875f && font_pointers_[2]->line_spacing() == 55.6640625f, "HUD paper source font line spacing changed");
        require(font_pointers_[0]->measure(L"HEALTH") == Vector2F(145, 37.109375f) && font_pointers_[0]->measure(L"PAINT") == Vector2F(112, 37.109375f) &&
            font_pointers_[1]->measure(L"2:04") == Vector2F(130, 74.21875f) && font_pointers_[1]->measure(L"0:10") == Vector2F(130, 74.21875f) &&
            font_pointers_[2]->measure(L"P1  /  TEAM A\nKeyboard/mouse") == Vector2F(386, 117.6640625f), "HUD paper source font measures changed");
        for (const auto& tuple : { hud_paper_values(look_), hud_paper_values(expected_) })
        { require(tuple[0].a == 1.0f && tuple[1].a == 1.0f && tuple[0] != tuple[1] && tuple[2].a > 0.0f, "HUD paper existing distinct-opaque/positive-shade domain changed"); }
    }
    Frame actual(size_t context, size_t background, bool copied = false)
    {
        if (copied) { set_hud_paper_values(expected_, hud_paper_values(look_)); }
        const auto& item = hud_timer_cases().at(context);
        return render(context, background, [&](DrawList& list)
        { (copied ? copied_ : hud_).draw_gameplay_interface_prepared(list, { 1920, 1080 }, 0, L"P1  /  TEAM A\nKeyboard/mouse",
            .5f, .5f, item.seconds, Colour(6, 52, 204), item.reduced, item.running); });
    }
    Frame raw(size_t background, const HudPaperTuple& value, HudPaperReference mode = HudPaperReference::correct)
    {
        return render(0, background, [&](DrawList& list)
        {
            for (size_t site = 0; site < 35; ++site)
            {
                const auto& patch = hud_paper_sites()[site];
                if (patch.material == 0)
                {
                    if (mode == HudPaperReference::omitted_ink)
                    { if (site >= 33) { rectangle(list, patch.box, value[1]); } }
                    else { rectangle(list, patch.box, mode == HudPaperReference::wrong_ink ? Colour(255, 0, 0) : value[0]); }
                    continue;
                }
                const Colour paper = mode == HudPaperReference::wrong_paper ? Colour(53, 188, 225) : mode == HudPaperReference::omitted_paper ? value[0] : value[1];
                if (patch.material == 1) { rectangle(list, patch.box, paper); continue; }
                const bool band = patch.material == 3;
                if (band) { rectangle(list, patch.box, paper); }
                const auto& shade = value[2]; const float alpha = shade.a * (band && mode != HudPaperReference::no_scale ? .12f : 1.0f);
                const Colour tint = mode == HudPaperReference::straight ? Colour(shade.r, shade.g, shade.b, alpha) :
                    mode == HudPaperReference::squared ? Colour(shade.r * alpha * alpha, shade.g * alpha * alpha, shade.b * alpha * alpha, alpha) :
                    band && mode == HudPaperReference::black_band ? Colour(0.0f, 0.0f, 0.0f, alpha) :
                    Colour(shade.r * alpha, shade.g * alpha, shade.b * alpha, alpha);
                for (size_t layer = 0; layer < patch.layers; ++layer)
                {
                    const size_t local = site % 11; const bool missing = layer == 1 &&
                        ((mode == HudPaperReference::missing_bottom && local >= 5 && local <= 7) || (mode == HudPaperReference::missing_right && local >= 8 && local <= 10));
                    if (!missing) { rectangle(list, patch.box, tint); }
                }
            }
        });
    }
    Frame observe(size_t context, size_t background, bool copied)
    {
        const Frame frame = actual(context, background), repeated = actual(context, background); equality("repeat", context, frame, repeated);
        if (copied) { const Frame adopted = actual(context, background, true); equality("copied-adoption", context, frame, adopted); } return frame;
    }
    void equality(const char* stage, size_t context, const Frame& frame, const Frame& reference, bool equal = true) const
    {
        try { require((frame.pixels == reference.pixels) == equal, "HUD paper complete-frame relation differs"); }
        catch (...) { failure(stage, context, frame, reference); throw; }
    }
    void patches(const char* stage, size_t context, const Frame& frame, const Frame& reference) const
    {
        try
        {
            size_t checked = 0; const bool closing = !hud_timer_cases().at(context).normal;
            for (const auto& patch : hud_paper_sites())
            {
                if (closing && patch.panel == 2) { continue; }
                for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
                { const size_t at = offset(x, y); require(same(frame, reference, at) && frame.pixels[at + 3] == 255, "HUD paper independent literal material RGBA differs"); ++checked; } }
            }
            require(checked == (closing ? 384U : 560U), "HUD paper applicable material pixel budget differs");
        }
        catch (...) { failure(stage, context, frame, reference); throw; }
    }
    void locality(size_t context, const Frame& frame, const Frame& reference, bool changed) const
    {
        try
        {
            size_t differences = 0; const size_t panels = hud_timer_cases()[context].normal ? 3U : 2U;
            for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
            {
                bool inside = false; for (size_t panel = 0; panel < panels; ++panel) { inside = inside || contains(crop_box(panel), x, y); }
                const bool equal = same(frame, reference, offset(x, y));
                if (!inside) { require(equal, "HUD paper edit escaped applicable panel envelopes"); } else if (!equal) { ++differences; }
            } }
            require(changed ? differences > 0 : differences == 0, "HUD paper active local response differs");
        }
        catch (...) { failure("locality", context, frame, reference); throw; }
    }
    void interiors(const Frame& frame, const Frame& reference) const
    {
        try
        {
            for (const auto& box : { RectangleI(58, 92, 52, 40), RectangleI(1580, 100, 8, 2), RectangleI(1750, 100, 8, 2), RectangleI(1580, 188, 8, 2), RectangleI(1750, 188, 8, 2) })
            { for (int y = box.y; y < box.bottom(); ++y) { for (int x = box.x; x < box.right(); ++x)
                { require(same(frame, reference, offset(x, y)), "HUD paper edit changed an unselected opaque badge/fill/track interior"); } } }
        }
        catch (...) { failure("unselected-interiors", 0, frame, reference); throw; }
    }
    void canvas_relation(const Frame& black, const Frame& nonblack, bool opaque) const
    {
        try
        {
            for (const auto& patch : hud_paper_sites())
            {
                if (patch.material < 2) { continue; } size_t different = 0;
                for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
                { if (!same(black, nonblack, offset(x, y))) { ++different; } } }
                require(patch.material == 3 || opaque ? different == 0 : different > 0, "HUD paper base/band Canvas relation differs");
            }
        }
        catch (...) { failure("canvas-relation", 0, black, nonblack); throw; }
    }
    void clock_region(size_t context, const Frame& frame, const Frame& reference) const
    {
        try { const auto box = crop_box(2); for (int y = box.y; y < box.bottom(); ++y) { for (int x = box.x; x < box.right(); ++x)
            { require(same(frame, reference, offset(x, y)), "Shared HUD paper changed the separate closing-clock region"); } } }
        catch (...) { failure("closing-region", context, frame, reference); throw; }
    }
    size_t control(size_t background, const HudPaperTuple& value, HudPaperReference mode, const Frame& reference)
    {
        const Frame wrong = raw(background, value, mode); size_t pixels = 0, sites = 0;
        try
        {
            for (size_t site = 0; site < 35; ++site)
            {
                const auto& patch = hud_paper_sites()[site]; size_t support = 0;
                for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
                { if (!same(reference, wrong, offset(x, y))) { ++support; } } }
                require(affected(site, mode) ? support > 0 : support == 0, "HUD paper negative reference lost a per-site discriminator or changed unselected material");
                if (affected(site, mode)) { ++sites; } pixels += support;
            }
            auto exterior = wrong.pixels;
            for (size_t site = 0; site < 35; ++site)
            {
                if (!affected(site, mode)) { continue; } const auto& box = hud_paper_sites()[site].box;
                for (int y = box.y; y < box.bottom(); ++y) { for (int x = box.x; x < box.right(); ++x)
                { const size_t at = offset(x, y); std::copy(reference.pixels.begin() + at, reference.pixels.begin() + at + 4, exterior.begin() + at); } }
            }
            require(exterior == reference.pixels, "HUD paper wrong reference changed complete raw exterior of the fixed source mask");
            const size_t expected = mode == HudPaperReference::wrong_ink || mode == HudPaperReference::omitted_ink ? 8U :
                mode == HudPaperReference::wrong_paper || mode == HudPaperReference::omitted_paper ? 21U :
                mode == HudPaperReference::straight || mode == HudPaperReference::squared ? 24U :
                mode == HudPaperReference::black_band || mode == HudPaperReference::no_scale ? 18U : 6U;
            require(sites == expected && pixels > 0 && pixels <= expected * 16, "HUD paper negative source support count differs");
        }
        catch (...) { failure("negative-control", 0, reference, wrong); throw; } return pixels;
    }
    static RectangleI crop_box(size_t panel)
    { static const std::array<RectangleI, 3> boxes{ RectangleI(40, 43, 491, 144), RectangleI(1538, 43, 327, 180), RectangleI(862, 43, 202, 100) }; return boxes.at(panel); }
    static Frame compact(const Frame& frame, size_t panel) { return foreground_crop(frame, crop_box(panel), 0); }
    size_t readbacks() const { return readbacks_; }
    void failure(const char* stage, size_t context, const Frame& frame, const Frame& reference, bool actual_available = true) const
    {
        const std::string stem = std::string("hud-paper-tune-failure-") + hud_timer_cases().at(context).name + '-' + stage + '-' + std::to_string(readbacks_);
        for (size_t panel = 0; panel < 3; ++panel)
        { save(compact(frame, panel), output_ / (stem + '-' + std::to_string(panel) + (actual_available ? "-actual.ppm" : "-held.ppm"))); save(compact(reference, panel), output_ / (stem + '-' + std::to_string(panel) + "-reference.ppm")); }
        for (const auto& item : { std::pair<const Frame*, const char*>(&frame, actual_available ? "-actual.rgba" : "-held.rgba"), { &reference, "-reference.rgba" } })
        { std::ofstream file(output_ / (stem + item.second), std::ios::binary); file.write(reinterpret_cast<const char*>(item.first->pixels.data()), static_cast<std::streamsize>(item.first->pixels.size())); require(file.good(), "Cannot preserve HUD paper failure RGBA"); }
        std::ofstream record(output_ / (stem + ".json")); record << "{\"scope\":\"" << stage << "\",\"context\":\"" << hud_timer_cases().at(context).name << "\",\"frame_width\":1920,\"frame_height\":1080,\"actual_frame_available\":" << (actual_available ? "true" : "false") << ",\"readbacks\":" << readbacks_ << ",\"diagnostic\":\"source-hypothesis-failure; no fitting or tolerance\"}\n";
        require(record.good(), "Cannot retain HUD paper source-hypothesis record");
    }
private:
    static size_t offset(int x, int y) { return (static_cast<size_t>(y) * 1920 + static_cast<size_t>(x)) * 4; }
    static bool same(const Frame& a, const Frame& b, size_t at) { return std::equal(a.pixels.begin() + at, a.pixels.begin() + at + 4, b.pixels.begin() + at); }
    static bool contains(const RectangleI& box, int x, int y) { return x >= box.x && x < box.right() && y >= box.y && y < box.bottom(); }
    static bool affected(size_t site, HudPaperReference mode)
    {
        const auto& patch = hud_paper_sites()[site];
        if (mode == HudPaperReference::wrong_ink || mode == HudPaperReference::omitted_ink) { return patch.material == 0; }
        if (mode == HudPaperReference::wrong_paper || mode == HudPaperReference::omitted_paper) { return patch.material == 1 || patch.material == 3; }
        if (mode == HudPaperReference::straight || mode == HudPaperReference::squared) { return patch.material >= 2; }
        if (mode == HudPaperReference::black_band || mode == HudPaperReference::no_scale) { return patch.material == 3; }
        if (site >= 33) { return false; } const size_t local = site % 11;
        return mode == HudPaperReference::missing_bottom ? local == 6 || local == 7 : local == 9 || local == 10;
    }
    void rectangle(DrawList& list, const RectangleI& box, const Colour& colour) const
    {
        list.set_filter(TextureFilter::point); list.draw_sprite(sheet_->texture(), pixel_.source_rectangle(),
            RectangleF(static_cast<float>(box.x), static_cast<float>(box.y), static_cast<float>(box.width), static_cast<float>(box.height)), colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
    }
    void geometry() const;
    Frame render(size_t context, size_t background, const std::function<void(DrawList&)>& draw)
    {
        unchanged(); require(background < 2 && readbacks_ < budget_, "HUD paper exceeded frozen context/readback budget");
        const std::array<HudPaperTuple, 2> held{ hud_paper_values(look_), hud_paper_values(expected_) };
        std::srand(565817); const int next = std::rand(); std::srand(565817);
        Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->player_viewport(0)); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        (background == 0 ? black_ : nonblack_).draw(list, { 0, 0, 1920, 1080 }); draw(list);
        rectangle(list, { 20, 20, 4, 4 }, Colour::white); rectangle(list, { 25, 20, 4, 4 }, Colour::black);
        Frame frame = read(renderer); ++readbacks_;
        require(frame.width == 1920 && frame.height == 1080 && frame.pixels.size() == 1920 * 1080 * 4, "HUD paper requires native1920x1080 RGBA");
        for (int y = 20; y < 24; ++y) { for (int x = 20; x < 29; ++x)
        { if (x == 24) { continue; } const size_t at = offset(x, y); const unsigned char c = x < 24 ? 255 : 0;
            require(frame.pixels[at] == c && frame.pixels[at + 1] == c && frame.pixels[at + 2] == c && frame.pixels[at + 3] == 255, "HUD paper WHITE/BLACK calibration differs"); } }
        require(std::rand() == next && held == std::array<HudPaperTuple, 2>{ hud_paper_values(look_), hud_paper_values(expected_) }, "HUD paper draw consumed RNG or changed selected tuples");
        unchanged(); (void)context; return frame;
    }
    Application& app_; const Presentation& presentation_; const HudPresentation& look_; const PlayerIdentityPresentation& identity_;
    HudPresentation expected_; PlayerIdentityPresentation expected_identity_; InterfaceGameplay hud_, copied_; Canvas black_, nonblack_;
    const SpriteSheet* sheet_; const SpriteFrame& pixel_; std::array<FontHandle, 3> fonts_{}; std::array<const Font*, 3> font_pointers_{}; std::array<std::vector<Glyph>, 3> glyphs_;
    std::vector<float> fixed_, identity_fixed_; std::array<std::string, 5> names_; TeamPalettePresentation palette_;
    const std::filesystem::path output_; size_t budget_, readbacks_ = 0;
};
void HudPaperColourProbe::geometry() const
{
    const Vector2F resolution(1920, 1080); auto* resources = app_.render_resources();
    const float scale = std::min(resolution.x / look_.player_identity_reference_width, resolution.y / look_.reference_height);
    const Vector2F padding = look_.player_identity_padding * scale;
    const Vector2F identity_pos(resolution.x * look_.player_identity_offset.x, resolution.y * look_.player_identity_offset.y);
    const Vector2F identity_size = resources->measure_text(fonts_[2], L"P1  /  TEAM A\nKeyboard/mouse") * scale;
    const Vector2F badge_size = identity_.ui_size * scale;
    const float badge_width = identity_.enabled ? badge_size.x + identity_.ui_gap * scale : 0.0f;
    const Vector2F label_pos = identity_pos + Vector2F(badge_width, 0.0f);
    const RectangleF identity_face(identity_pos - padding, Vector2F(identity_size.x + badge_width,
        std::max(identity_size.y, identity_.enabled ? badge_size.y : 0.0f)) + padding * 2.0f);
    const float x_pos = resolution.x - resolution.x * look_.right_margin;
    const Vector2F health_pos(x_pos, resolution.y * look_.health_top_margin), ammo_pos(x_pos, resolution.y * look_.ammo_top_margin);
    const float label_height = resources->measure_text(fonts_[0], L"HEALTH").y * scale;
    const float panel_top = std::min(ammo_pos.y, health_pos.y) - label_height - look_.meter_label_gap * scale;
    const float panel_bottom = std::max(ammo_pos.y, health_pos.y) + resolution.y * look_.box_size.y;
    const RectangleF meters_face(Vector2F(x_pos, panel_top) - padding,
        Vector2F(resolution.x * look_.box_size.x, panel_bottom - panel_top) + padding * 2.0f);
    const float timer_scale = scale * look_.player_identity_reference_width / look_.timer_reference_width;
    const Vector2F timer_size = resources->measure_text(fonts_[1], L"2:04") * timer_scale;
    const Vector2F timer_pos((resolution.x - timer_size.x) / 2.0f - resolution.x * look_.timer_offset.x, resolution.y * look_.timer_offset.y);
    const std::array<RectangleF, 3> faces{ identity_face, meters_face, RectangleF(timer_pos - padding, timer_size + padding * 2.0f) };
    require(scale == 1.0f && label_pos == Vector2F(123.599998474121094f, 54) &&
        identity_face.bottom() == 181.6640625f && meters_face.bottom() == 217.360015869140625f && faces[2].bottom() == 138.21875f &&
        timer_pos == Vector2F(895, 54), "HUD paper source float32 operation order or geometry changed; decimal rounding is not permitted");
    const float pen = look_.paper.keyline_width * scale, step = pen * 2.0f, shadow = look_.paper.shadow_offset * scale;
    const auto raster = [](const RectangleF& box)
    { const int x = static_cast<int>(box.x), y = static_cast<int>(box.y); return RectangleI(x, y, static_cast<int>(box.right()) - x, static_cast<int>(box.bottom()) - y); };
    const auto cross = [&](const RectangleF& box)
    { return std::array<RectangleI, 2>{ raster(RectangleF(box.x + step, box.y, box.width - step * 2.0f, box.height)),
        raster(RectangleF(box.x, box.y + step, box.width, box.height - step * 2.0f)) }; };
    const auto overlaps = [](const RectangleI& a, const RectangleI& b)
    { return a.x < b.right() && a.right() > b.x && a.y < b.bottom() && a.bottom() > b.y; };
    const Vector2F meter_size(resolution.x * look_.box_size.x, resolution.y * look_.box_size.y);
    const RectangleI health_track = raster(RectangleF(health_pos, meter_size)), ammo_track = raster(RectangleF(ammo_pos, meter_size));
    const std::array<RectangleI, 2> meter_borders{ raster(RectangleF(health_pos - Vector2F(pen), meter_size + Vector2F(pen * 2.0f))),
        raster(RectangleF(ammo_pos - Vector2F(pen), meter_size + Vector2F(pen * 2.0f))) };
    const RectangleI stripe = raster(RectangleF(identity_face.position() + Vector2F(pen, pen * 3.0f),
        Vector2F(look_.box_fill_border * look_.player_identity_reference_width * scale, identity_face.height - pen * 6.0f)));
    const RectangleI badge = raster(RectangleF(identity_pos + Vector2F(0, (identity_size.y - badge_size.y) / 2.0f), badge_size));
    for (size_t site = 0; site < 35; ++site)
    {
        const auto& patch = hud_paper_sites()[site]; require(patch.box.width * patch.box.height == 16 && patch.box.x >= 0 && patch.box.y >= 0 &&
            patch.box.right() <= 1920 && patch.box.bottom() <= 1080, "HUD paper source patch escaped its declared area/viewport");
        for (size_t old = 0; old < site; ++old) { require(!overlaps(patch.box, hud_paper_sites()[old].box), "HUD paper source sites overlap"); }
        require(!overlaps(patch.box, badge) && !overlaps(patch.box, stripe) && !overlaps(patch.box, health_track) && !overlaps(patch.box, ammo_track),
            "HUD paper source site intersects a later opaque overlay");
        if (site < 33) { require(!overlaps(patch.box, meter_borders[0]) && !overlaps(patch.box, meter_borders[1]), "HUD paper helper witness intersects a later direct ink border"); }
        const auto& face = faces[patch.panel]; const RectangleF inside(face.x + pen, face.y + pen, face.width - pen * 2.0f, face.height - pen * 2.0f);
        const auto base = cross(RectangleF(face.position() + Vector2F(shadow), face.size())), ink = cross(face), paper = cross(inside);
        std::array<RectangleI, 3> bottom{}, right{};
        for (size_t pass = 0; pass < 3; ++pass)
        { const float width = pen * static_cast<float>(3 - pass);
            bottom[pass] = raster(RectangleF(inside.x + step, inside.bottom() - width, inside.width - step * 2.0f, width));
            right[pass] = raster(RectangleF(inside.right() - width, inside.y + step, width, inside.height - step * 2.0f)); }
        for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
        {
            const bool b0 = contains(base[0], x, y), b1 = contains(base[1], x, y), i0 = contains(ink[0], x, y), i1 = contains(ink[1], x, y),
                p0 = contains(paper[0], x, y), p1 = contains(paper[1], x, y);
            size_t bottoms = 0, rights = 0; for (size_t pass = 0; pass < 3; ++pass)
            { bottoms += contains(bottom[pass], x, y) ? 1U : 0U; rights += contains(right[pass], x, y) ? 1U : 0U; }
            if (site >= 33) { require(contains(meter_borders[site - 33], x, y), "HUD paper direct meter ink source coverage changed"); continue; }
            const size_t local = site % 11;
            if (local < 2) { require((local == 0 ? i0 && !i1 : i1 && !i0) && !p0 && !p1 && bottoms + rights == 0 && !b0 && !b1, "HUD paper exclusive ink/Canvas witness changed"); }
            else if (local == 2) { require(p0 && !p1 && bottoms + rights == 0, "HUD paper flat first-cross witness changed"); }
            else if (local < 5) { require((local == 3 ? b0 && !b1 : b1 && !b0) && !i0 && !i1 && !p0 && !p1 && bottoms + rights == 0, "HUD paper exclusive base-shade witness changed"); }
            else
            {
                require((p0 || p1) && (local < 8 ? bottoms == patch.layers && rights == 0 : rights == patch.layers && bottoms == 0), "HUD paper selected-paper/band depth source coverage changed");
                if (local >= 9) { require(!p0 && p1, "HUD paper composed second-cross underlay lost exclusive source support"); }
            }
        } }
    }
    const auto glyph_clear = [&](size_t font, std::wstring_view text, const Vector2F& position)
    {
        require(font_pointers_[font]->first_unrenderable(text) == std::wstring_view::npos, "HUD paper actual caller text lost ordinal coverage");
        font_pointers_[font]->for_each_glyph(text, [&](const Glyph& glyph, const Vector2F& pen_position)
        {
            const float left = position.x + pen_position.x - 1.0f, top = position.y + pen_position.y + glyph.y_offset - 1.0f;
            const float right = position.x + pen_position.x + static_cast<float>(glyph.subrect.width) + 1.0f;
            const float bottom = position.y + pen_position.y + glyph.y_offset + static_cast<float>(glyph.subrect.height) + 1.0f;
            for (const auto& patch : hud_paper_sites())
            { require(!(left < static_cast<float>(patch.box.right()) && right > static_cast<float>(patch.box.x) && top < static_cast<float>(patch.box.bottom()) && bottom > static_cast<float>(patch.box.y)),
                "HUD paper source site intersects a conservative literal glyph footprint"); }
        });
    };
    glyph_clear(2, L"P1  /  TEAM A\nKeyboard/mouse", label_pos); glyph_clear(2, L"P1  /  TEAM A\nKeyboard/mouse", label_pos + look_.player_identity_shadow_offset * scale);
    glyph_clear(0, L"HEALTH", health_pos - Vector2F(0, resources->measure_text(fonts_[0], L"HEALTH").y * scale + look_.meter_label_gap * scale));
    glyph_clear(0, L"PAINT", ammo_pos - Vector2F(0, resources->measure_text(fonts_[0], L"PAINT").y * scale + look_.meter_label_gap * scale));
    glyph_clear(1, L"2:04", timer_pos); glyph_clear(1, L"2:04", timer_pos + look_.timer_shadow_offset * timer_scale); glyph_clear(1, L"0:10", timer_pos);
}
HudPaperFrames hud_paper_consumer_checks(HudPaperColourProbe& probe, HudPresentation& look, const std::filesystem::path& output)
{
    HudPaperFrames main{}, references{};
    constexpr std::array<const char*, 6> states{ "default", "ink", "paper", "shade-opaque", "shade-partial", "joint" };
    constexpr std::array<const char*, 2> backgrounds{ "black", "nonblack" };
    constexpr std::array<const char*, 3> panels{ "identity", "meters", "timer" };
    std::ofstream csv(output / "hud-paper-main.csv"), branches(output / "hud-paper-branches.csv"), controls(output / "hud-paper-controls.csv");
    csv << "state,background,ink_sites,flat_paper_sites,shade_base_sites,shade_band_sites,composed_second_paper_sites,raw_rgba_match,adoption_match,repeat_match,locality_match,unselected_interior_match,frozen_style_resources_rng,scope\n";
    branches << "state,context,applicable_sites,raw_rgba_match,repeat_match,selection_guard,clock_region_match,frozen_style_resources_rng,scope\n";
    controls << "material,control,background,affected_sites,discriminating_pixels,unselected_exterior_match,scope\n";
    size_t rows = 0, crops = 0, control_pixels = 0;
    for (size_t state = 0; state < 6; ++state)
    {
        set_hud_paper_values(look, hud_paper_palettes()[state]); const bool both = state == 0 || state == 3 || state == 4;
        for (size_t background = both ? 0U : 1U; background < 2; ++background)
        {
            main[state][background] = probe.observe(0, background, true); references[state][background] = probe.raw(background, hud_paper_palettes()[state]);
            probe.patches("main-reference", 0, main[state][background], references[state][background]);
            probe.locality(0, main[state][background], main[0][background], state != 0);
            if (state != 0) { probe.interiors(main[state][background], main[0][background]); }
            csv << states[state] << ',' << backgrounds[background] << ",8,3,6,18,6,true,true,true,true," << (state == 0 ? "not-run-baseline" : "true") << ",true,hud-paper-material-patches\n"; ++rows;
            if (background == 1) { for (size_t panel = 0; panel < 3; ++panel)
            { save(HudPaperColourProbe::compact(main[state][background], panel), output / (std::string("hud-paper-colour-") + states[state] + '-' + panels[panel] + ".ppm")); ++crops; } }
        }
        if (both) { probe.canvas_relation(main[state][0], main[state][1], state == 3); }
    }
    const std::array<HudPaperReference, 10> modes{ HudPaperReference::wrong_ink, HudPaperReference::omitted_ink,
        HudPaperReference::wrong_paper, HudPaperReference::omitted_paper, HudPaperReference::straight, HudPaperReference::squared,
        HudPaperReference::black_band, HudPaperReference::no_scale, HudPaperReference::missing_bottom, HudPaperReference::missing_right };
    constexpr std::array<const char*, 10> mode_names{ "wrong-opaque", "omitted", "wrong-opaque", "omitted", "straight", "double-submitted-alpha",
        "forced-black-band", "omitted-band-scale", "omitted-middle-bottom", "omitted-middle-right" };
    for (size_t mode = 0; mode < modes.size(); ++mode)
    {
        const size_t palette = mode < 4 ? 0 : 4; set_hud_paper_values(look, hud_paper_palettes()[palette]);
        for (size_t background = mode == 4 || mode == 5 ? 0U : 1U; background < 2; ++background)
        {
            const size_t pixels = probe.control(background, hud_paper_palettes()[palette], modes[mode], references[palette][background]); control_pixels += pixels;
            const size_t sites = mode < 2 ? 8U : mode < 4 ? 21U : mode < 6 ? 24U : mode < 8 ? 18U : 6U;
            controls << (mode < 2 ? "ink" : mode < 4 ? "paper" : "shade") << ',' << mode_names[mode] << ',' << backgrounds[background] << ',' << sites << ',' << pixels << ",true,hud-paper-material-counterexample\n"; ++rows;
        }
    }
    std::array<std::array<Frame, 8>, 2> branch{};
    for (const size_t context : { 6U, 2U, 3U, 7U })
    {
        for (size_t state = 0; state < 2; ++state)
        {
            const size_t palette = state == 0 ? 0 : 5; const auto& item = hud_timer_cases()[context]; set_hud_paper_values(look, hud_paper_palettes()[palette]);
            branch[state][context] = probe.observe(context, 1, false); probe.patches("branch-cached-reference", context, branch[state][context], references[palette][1]);
            if (item.normal) { probe.equality("normal-reduced", context, branch[state][context], main[palette][1]); }
            else if (state != 0)
            { probe.clock_region(context, branch[state][context], branch[0][context]); probe.locality(context, branch[state][context], branch[0][context], true); }
            branches << states[palette] << ',' << item.name << ',' << (item.normal ? 35 : 24) << ",true,true," <<
                (item.normal ? "normal-reduced-equal" : state == 0 ? "baseline" : "closing-panels-localized") << ',' <<
                (item.normal ? "not-run-normal-whole-frame-relation" : state == 0 ? "baseline" : "true") << ",true,hud-paper-applicability\n"; ++rows;
        }
    }
    for (size_t state = 0; state < 2; ++state)
    { probe.equality("settled-reduced", 7, branch[state][7], branch[state][3]); probe.equality("tick-counterexample", 7, branch[state][7], branch[state][2], false); }
    require(csv.good() && branches.good() && controls.good() && probe.readbacks() == 64 && rows == 29 && crops == 18 && control_pixels > 0 && control_pixels <= 3232,
        "HUD paper shared frozen reads/rows/crops/control counters differ");
    std::ofstream summary(output / "hud-paper-consumer-summary.json"); summary << "{\"scope\":\"hud-paper-material-patches-and-branch-isolation\",\"main_observations\":9,\"main_reads\":36,\"raw_references\":9,\"control_reads\":12,\"controls\":12,\"control_site_discriminations\":202,\"control_pixel_upper_bound\":3232,\"branch_observations\":8,\"branch_reads\":16,\"readbacks\":64,\"main_rows\":9,\"control_rows\":12,\"branch_rows\":8,\"csv_rows\":29,\"repeat_checks\":17,\"copied_adoption_checks\":9,\"independent_observations\":17,\"patch_pixel_checks\":8464,\"normal_reduced_equalities\":2,\"closing_region_invariance_checks\":3,\"closing_region_pixels\":60600,\"closing_full_hud_positive_changes\":3,\"settled_reduced_equalities\":2,\"tick_counterexamples\":2,\"unselected_region_checks\":35,\"unselected_pixel_checks\":15008,\"band_canvas_relations\":3,\"band_canvas_pixels\":864,\"base_canvas_site_relations\":18,\"base_canvas_pixels\":288,\"crops\":18,\"crop_pixels\":898584,\"ppm_bytes\":2696022,\"calibration_pixel_checks\":2048,\"patches\":35,\"patch_pixels\":560,\"closing_patch_sites\":24,\"closing_patch_pixels\":384,\"flat_first_paper_sites\":3,\"composed_unique_second_paper_sites\":6,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0,\"control_discriminating_pixels\":" << control_pixels << "}\n";
    require(summary.good(), "Cannot retain HUD paper shared summary"); set_hud_paper_values(look, hud_paper_palettes()[0]);
    std::cout << "PASS HUD paper: 64 reads, 29 rows, 18 native crops; 35 material sites, 202 per-site control discriminations and separate closing-clock isolation. No updates, input polls or game Saves.\n";
    return references;
}
#if COLOURWARS_DEV_TOOLS
void hud_paper_tune_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output, HudPaperColourProbe& probe, const HudPaperFrames& references)
{
    const auto root = output / "hud-paper-tune-scratch", source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "HUD paper private scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> bytes;
    for (size_t i = 0; i < files.size(); ++i)
    { bytes[i] = tune_colour_bytes(content / files[i]); tune_colour_write(source / files[i], bytes[i]); tune_colour_write(staged / files[i], bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp"); Save preferences;
    const auto preferences_now = [&]()
    {
        auto values = limb_preference_values(preferences.play_settings()); values.insert(values.end(), { static_cast<int>(preferences.resolution()),
            preferences.fullscreen() ? 1 : 0, static_cast<int>(preferences.team_palette()), static_cast<int>(preferences.input_bindings().keyboard_jump) }); return values;
    };
    const auto original_preferences = preferences_now(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    { const auto* info = levels.get(name); require(info != nullptr, "HUD paper requires shipped stage metadata"); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "hud-paper-tune-commands.log").string()), "Cannot retain HUD paper actual command transcript");
    JsonSpanDocument original(bytes[1], "HUD paper exact original source"); std::string saved_bytes = bytes[1];
    const std::array<const char*, 3> saved_words{ "RED", "35BCE1", "2060C080" };
    for (size_t i = 0; i < 3; ++i)
    { JsonSpanDocument current(saved_bytes, "HUD paper exact three-token replacement"); const auto* token = current.find(hud_paper_keys()[i]);
        require(token != nullptr && original.find(hud_paper_keys()[i]), "HUD paper exact existing token is absent"); saved_bytes = current.replaced(*token, std::string("\"") + saved_words[i] + '"'); }
    bool persisted = false; size_t commands = 0, accepted = 0, queries = 0, tunes = 0, resets = 0, saves = 0, refused = 0, rows = 0, new_raw = 0;
    const size_t initial_reads = probe.readbacks(); Frame baseline{}, pair{}, joint{};
    const auto unchanged = [&]()
    {
        probe.unchanged(); require(preferences_now() == original_preferences && !std::filesystem::exists(root / "profile/ColourWars/save_data.json") &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty() && impl.total_updates() == 0 && impl.level_updates() == 0,
            "HUD paper command changed preferences, saved a game, queued gameplay or advanced updates");
        for (size_t i = 0; i < files.size(); ++i)
        { const auto& expected = i == 1 && persisted ? saved_bytes : bytes[i];
            require(tune_colour_bytes(content / files[i]) == bytes[i] && tune_colour_bytes(source / files[i]) == expected && tune_colour_bytes(staged / files[i]) == expected,
                "HUD paper private bytes escaped the exact three saved spans"); }
    };
    const auto document_state = [&]()
    {
        std::vector<std::string> state;
        for (const char* name : { "presentation", "tuning" })
        { const auto* document = impl.tune_document(name); require(document != nullptr, "HUD paper document disappeared");
            state.insert(state.end(), { name, document->baseline().text(), document->current().text(), std::to_string(document->revision()) });
            for (const auto& [key, edit] : document->edits()) { state.insert(state.end(), { key, edit.baseline_token, edit.current_token }); } }
        for (const char* name : files)
        { state.push_back(tune_colour_bytes(content / name)); state.push_back(tune_colour_bytes(source / name)); state.push_back(tune_colour_bytes(staged / name)); }
        return state;
    };
    const auto command = [&](const std::string& line, bool should_accept, bool query = false)
    {
        unchanged(); const size_t before = impl.log().lines().size(); std::srand(565823); const int next = std::rand(); std::srand(565823);
        const auto status = impl.execute(line); ++commands; require(status.ok == should_accept && std::rand() == next, "HUD paper actual command acceptance/RNG differs");
        const auto& log = impl.log().lines(); require(log.size() > before, "HUD paper actual command lacks a fresh reply");
        if (!should_accept) { ++refused; require(log.back().text.find("distinct opaque ink/paper and visible shade required") != std::string::npos, "HUD paper refusal lacks actual coupled reader diagnostic"); return; }
        ++accepted;
        if (query) { ++queries; require(log.back().severity == DebugSeverity::info && log.back().text.starts_with(line.substr(5) + " = ") && log.back().text.ends_with("[live]"), "HUD paper query lacks fresh exact live reply"); }
        else if (line.starts_with("tune ")) { ++tunes; require(log.back().severity == DebugSeverity::info && log.back().text.starts_with("tune: presentation.hud.paper.") && log.back().text.ends_with("(live)"), "HUD paper tune lacks fresh live adoption reply"); }
        else if (line.starts_with("tune.reset ")) { ++resets; require(log.back().severity == DebugSeverity::info && log.back().text.starts_with("tune.reset: presentation.hud.paper.") && log.back().text.ends_with("(live)"), "HUD paper reset lacks fresh live adoption reply"); }
        else if (line == "tune.save presentation") { ++saves; require(log.back().severity == DebugSeverity::info && log.back().text.starts_with("tune.save: 3 value(s) written to "), "HUD paper Save lacks exact three-value reply"); }
    };
    std::ofstream csv(output / "hud-paper-tune.csv"), refusals(output / "hud-paper-tune-refusals.json");
    csv << "event,state,ink_token,paper_token,shade_token,raw_reference_source,raw_rgba_match,adoption_match,repeat_match,unchanged_frame,private_bytes,siblings_preserved,frozen_style_resources_rng,scope\n";
    refusals << "{\"scope\":\"atomic-hud-paper-policy-refusals-with-held-frame-reads\",\"records\":["; size_t refusal_records = 0;
    const auto check = [&](const char* state, const HudPaperTuple& tuple, const std::array<const char*, 3>& words, const char* reference_source, int cache, size_t pending)
    {
        unchanged(); require(hud_paper_values(presentation.hud()) == tuple, "HUD paper actual selected/sibling RGBA differ from literal expectation");
        const auto* document = impl.tune_document("presentation"); require(document && document->edits().size() == pending && impl.tune_document("tuning")->edits().empty(), "HUD paper pending sibling count changed");
        for (size_t i = 0; i < 3; ++i)
        {
            const auto* token = document->current().find(hud_paper_keys()[i]); require(token && document->current().token(*token) == std::string("\"") + words[i] + '"', "HUD paper exact selected/sibling token differs");
            const auto* baseline_token = document->baseline().find(hud_paper_keys()[i]); require(baseline_token != nullptr, "HUD paper baseline key disappeared");
            const auto baseline_word = document->baseline().token(*baseline_token), current_word = document->current().token(*token);
            if (baseline_word != current_word)
            { require(document->edits().contains(hud_paper_keys()[i]) && document->edits().at(hud_paper_keys()[i]).baseline_token == baseline_word && document->edits().at(hud_paper_keys()[i]).current_token == current_word, "HUD paper pending sibling span tokens changed"); }
            else { require(!document->edits().contains(hud_paper_keys()[i]), "HUD paper reset left an unchanged sibling in the ledger"); }
        }
        const Frame frame = probe.observe(0, 1, true); Frame raw;
        if (cache < 0) { raw = probe.raw(1, tuple); ++new_raw; }
        probe.patches("debug-independent-reference", 0, frame, cache < 0 ? raw : references[static_cast<size_t>(cache)][1]);
        if (std::string_view(state) == "baseline") { baseline = frame; }
        if (std::string_view(state) == "reset-original") { probe.equality("reset-original", 0, frame, baseline); }
        if (std::string_view(state) == "pair-pending") { pair = frame; }
        if (std::string_view(state) == "joint-pending") { joint = frame; }
        if (std::string_view(state) == "reset-shade") { probe.equality("reset-saved", 0, frame, joint); }
        csv << "accepted," << state << ',' << words[0] << ',' << words[1] << ',' << words[2] << ',' << reference_source << ",true,true,true,not-run-accepted-state,true,true,true,debug-live-hud-paper-family\n"; ++rows; unchanged();
    };
    const auto refuse = [&](const char* phase, const char* state, size_t selected, const char* word, const char* policy, size_t pending, const Frame& held_frame,
        const std::array<const char*, 3>& words)
    {
        Frame observed; bool actual_available = false;
        try
        {
            unchanged(); const auto before = document_state(); const auto tuple = hud_paper_values(presentation.hud());
            const auto* presentation_document = impl.tune_document("presentation"); const auto* tuning_document = impl.tune_document("tuning");
            require(presentation_document && presentation_document->edits().size() == pending, "HUD paper refusal lost expected held pending spans");
            command(std::string("tune presentation.") + hud_paper_keys()[selected] + ' ' + word, false);
            require(document_state() == before && hud_paper_values(presentation.hud()) == tuple && impl.tune_document("presentation") == presentation_document && impl.tune_document("tuning") == tuning_document,
                "HUD paper refusal changed live tuple, complete documents/revisions/ledgers/identity or source/staged bytes");
            unchanged(); observed = probe.actual(0, 1); actual_available = true; require(observed.pixels == held_frame.pixels, "HUD paper refusal changed the held valid full HUD frame");
            csv << "refused," << state << ',' << words[0] << ',' << words[1] << ',' << words[2] << ",not-run-policy-refusal,not-run-refusal-held-frame-only,not-run-refusal-held-frame-only,not-run-refusal-held-frame-only,true,true,true,true,debug-atomic-hud-paper-policy-refusal\n"; ++rows;
            refusals << (refusal_records == 0 ? "" : ",") << "{\"phase\":\"" << phase << "\",\"path\":\"presentation." << hud_paper_keys()[selected] << "\",\"input\":\"" << word << "\",\"policy\":\"" << policy << "\",\"pending_tokens\":" << pending << ",\"accepted\":false,\"atomic_preserved\":true,\"live_tuple_preserved\":true,\"documents_preserved\":true,\"bytes_preserved\":true,\"resources_rng_preserved\":true,\"raster_checked\":true,\"raster_reads\":1}"; ++refusal_records; unchanged();
        }
        catch (...)
        { probe.failure("policy-refusal", 0, actual_available ? observed : held_frame, held_frame, actual_available); throw; }
    };
    const Colour ink(20, 20, 20), paper(255, 245, 223), shade(0, 0, 0, 85), red(255, 0, 0), cyan(53, 188, 225), blue(32, 96, 192), partial(32, 96, 192, 128);
    check("baseline", { ink, paper, shade }, { "141414", "FFF5DF", "00000055" }, "shared-default", 0, 0);
    for (const char* key : hud_paper_keys())
    { const auto held = document_state(); const auto tuple = hud_paper_values(presentation.hud()); command(std::string("tune presentation.") + key, true, true);
        require(document_state() == held && hud_paper_values(presentation.hud()) == tuple, "HUD paper value-less query mutated held metadata"); }
    command("tune presentation.hud.paper.ink RED", true); check("ink-edit", { red, paper, shade }, { "RED", "FFF5DF", "00000055" }, "shared-ink", 1, 1);
    command("tune.reset presentation.hud.paper.ink", true); check("reset-original", { ink, paper, shade }, { "141414", "FFF5DF", "00000055" }, "shared-default", 0, 0);
    command("tune presentation.hud.paper.ink RED", true); command("tune presentation.hud.paper.paper 35BCE1", true);
    check("pair-pending", { red, cyan, shade }, { "RED", "35BCE1", "00000055" }, "new-raw", -1, 2);
    refuse("pair-pending", "equal-ink", 0, "35BCE1", "equal-opaque-siblings", 2, pair, { "RED", "35BCE1", "00000055" });
    refuse("pair-pending", "equal-paper", 1, "RED", "equal-opaque-siblings", 2, pair, { "RED", "35BCE1", "00000055" });
    refuse("pair-pending", "partial-ink", 0, "2060C080", "opaque-ink-required", 2, pair, { "RED", "35BCE1", "00000055" });
    refuse("pair-pending", "partial-paper", 1, "35BCE180", "opaque-paper-required", 2, pair, { "RED", "35BCE1", "00000055" });
    command("tune presentation.hud.paper.shade 2060C080", true);
    check("joint-pending", { red, cyan, partial }, { "RED", "35BCE1", "2060C080" }, "shared-joint", 5, 3);
    refuse("joint-pending", "zero-shade", 2, "A532D600", "positive-shade-alpha-required", 3, joint, { "RED", "35BCE1", "2060C080" });
    const auto held_tuple = hud_paper_values(presentation.hud()); command("tune.save presentation", true); persisted = true; unchanged();
    require(impl.tune_document("presentation")->baseline().text() == saved_bytes && impl.tune_document("presentation")->current().text() == saved_bytes &&
        impl.tune_document("presentation")->edits().empty() && hud_paper_values(presentation.hud()) == held_tuple, "HUD paper three-span Save failed rebase or changed held live tuple");
    command("tune presentation.hud.paper.ink 2060C0", true); command("tune presentation.hud.paper.paper FFF5DF", true); command("tune presentation.hud.paper.shade FF0000", true);
    check("all-overwrite", { blue, paper, red }, { "2060C0", "FFF5DF", "FF0000" }, "new-raw", -1, 3);
    command("tune.reset presentation.hud.paper.ink", true); check("reset-ink", { red, paper, red }, { "RED", "FFF5DF", "FF0000" }, "new-raw", -1, 2);
    command("tune.reset presentation.hud.paper.paper", true); check("reset-paper", { red, cyan, red }, { "RED", "35BCE1", "FF0000" }, "new-raw", -1, 1);
    command("tune.reset presentation.hud.paper.shade", true); check("reset-shade", { red, cyan, partial }, { "RED", "35BCE1", "2060C080" }, "shared-joint", 5, 0);
    refusals << "]}\n";
    require(csv.good() && refusals.good() && commands == 20 && accepted == 15 && queries == 3 && tunes == 7 && resets == 4 && saves == 1 && refused == 5 &&
        rows == 14 && refusal_records == 5 && new_raw == 4 && probe.readbacks() - initial_reads == 36 &&
        impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(), "HUD paper Debug frozen command/state/refusal/read counts or final ledgers differ");
    std::ofstream summary(output / "hud-paper-tune-summary.json"); summary << "{\"scope\":\"debug-live-hud-paper-family-and-policy-refusal\",\"accepted_rendered_states\":9,\"accepted_reads\":27,\"new_raw_reads\":4,\"cached_reference_observations\":5,\"refusal_reads\":5,\"readbacks\":36,\"accepted_rows\":9,\"refusal_rows\":5,\"csv_rows\":14,\"repeat_checks\":9,\"copied_adoption_checks\":9,\"independent_observations\":9,\"patch_pixel_checks\":5040,\"saved_tokens\":3,\"original_reset_frame_equalities\":1,\"saved_reset_frame_equalities\":1,\"sibling_pending_reset_checks\":3,\"refusal_json_records\":5,\"crops\":0,\"calibration_pixel_checks\":1152,\"commands\":20,\"accepted_commands\":15,\"query_commands\":3,\"tune_commands\":7,\"reset_commands\":4,\"presentation_save_commands\":1,\"refused_commands\":5,\"refusal_raster_reads\":5,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}" << '\n';
    require(summary.good(), "Cannot retain HUD paper Debug summary");
    std::cout << "PASS HUD paper tune: 20 actual statements, 9 accepted rendered states, 5 typed atomic refusals and held-frame reads; 36 additional reads, exact3-token Save/rebase and sibling-preserving resets. No game Saves, updates or input polls.\n";
}
#endif
void hud_paper_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    struct RestoreHudPaper { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestoreHudPaper() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
#if COLOURWARS_DEV_TOOLS
    constexpr size_t budget = 100;
#else
    constexpr size_t budget = 64; (void)levels; (void)content;
#endif
    HudPaperColourProbe probe(app, presentation, output, budget); auto& look = const_cast<HudPresentation&>(presentation.hud());
    const HudPaperFrames references = hud_paper_consumer_checks(probe, look, output);
#if COLOURWARS_DEV_TOOLS
    hud_paper_tune_checks(app, levels, tuning, presentation, content, output, probe, references);
#else
    (void)references; std::cout << "PASS hud-paper-tune-colours-only Release consumer scope; no developer commands executed\n";
#endif
    require(probe.readbacks() == budget, "HUD paper family combined readback budget differs");
}
