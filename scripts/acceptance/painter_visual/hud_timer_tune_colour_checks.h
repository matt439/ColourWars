// Paths 46/47/48: independently selected normal foreground, shadow and opaque closing text. Font metrics and the
// fixed pure-paper ROI are hypotheses until the native qualification runs. Re-pinned to the OFL Lato Black
// atlas (lato_black_48, whole-pixel line spacing 77) after 95c1da3b replaced Gill Sans MT Bold.
enum class HudTimerSubject { foreground, shadow, closing_text };
enum class HudTimerReference { correct, straight, squared, reversed_order, wrong_opaque, added_normal_shadow };
Colour& hud_timer_selected(HudPresentation& look, HudTimerSubject subject)
{
    if (subject == HudTimerSubject::foreground) { return look.timer_colour; }
    return subject == HudTimerSubject::shadow ? look.timer_shadow_colour : look.closing_clock.text;
}
const Colour& hud_timer_selected(const HudPresentation& look, HudTimerSubject subject)
{
    if (subject == HudTimerSubject::foreground) { return look.timer_colour; }
    return subject == HudTimerSubject::shadow ? look.timer_shadow_colour : look.closing_clock.text;
}
constexpr float hud_timer_line_spacing = 77.0f;
struct HudTimerCase
{
    const char* name;
    float seconds;
    const wchar_t* text;
    float width;
    bool running, reduced, normal;
};
const std::array<HudTimerCase, 8>& hud_timer_cases()
{
    static const std::array<HudTimerCase, 8> cases{ {
        { "normal", 123.25f, L"2:04", 137.0f, true, false, true },
        { "above-ten", 10.01f, L"0:11", 135.0f, true, false, true },
        { "exact-ten", 10.0f, L"0:10", 137.0f, true, false, false },
        { "fractional-closing", 9.25f, L"0:10", 137.0f, true, false, false },
        { "zero", 0.0f, L"0:00", 137.0f, true, false, true },
        { "stopped-short", 4.25f, L"0:05", 135.0f, false, false, true },
        { "reduced-normal", 123.25f, L"2:04", 137.0f, true, true, true },
        { "reduced-closing", 10.0f, L"0:10", 137.0f, true, true, false },
    } };
    return cases;
}
std::vector<float> hud_timer_fixed_style(const HudPresentation& look, HudTimerSubject subject)
{
    auto copy = look;
    hud_timer_selected(copy, subject) = subject == HudTimerSubject::shadow ? Colour(0, 0, 0, 34) : Colour(20, 20, 20);
    auto values = hud_track_fixed_style(copy);
    // The track helper omits its own selected tint, so add that unselected
    // field explicitly here. The opposite timer colour and all closing colours remain guarded.
    const auto& track = look.meter_track_colour;
    values.insert(values.end(), { track.r, track.g, track.b, track.a });
    return values;
}

class HudTimerColourProbe
{
public:
    HudTimerColourProbe(Application& app, const Presentation& presentation, const std::filesystem::path& output,
        HudTimerSubject subject = HudTimerSubject::foreground) :
        subject_(subject), app_(app), presentation_(presentation), look_(presentation.hud()),
        identity_(presentation.player_identity()), expected_(look_), expected_identity_(identity_),
        hud_(app.render_resources(), &look_, &identity_),
        copied_(app.render_resources(), &expected_, &expected_identity_),
        canvas_(app.render_resources(), Colour(32, 64, 96)),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))),
        timer_font_(app.render_resources()->resolve_sprite_font(look_.timer_font)),
        font_(app.render_resources()->font(timer_font_)),
        fixed_(hud_timer_fixed_style(look_, subject)), identity_fixed_(hud_track_identity_style(identity_)),
        names_(hud_track_resource_names(look_)), palette_(presentation.team_palette()), output_(output)
    {
        require(look_.timer_colour == Colour(20, 20, 20) && look_.timer_shadow_colour == Colour(0, 0, 0, 34) &&
            look_.timer_font == "lato_black_48" && look_.timer_offset == Vector2F(0, .05f) &&
            look_.timer_shadow_offset == Vector2F(2, 2) && look_.timer_reference_width == 1920 &&
            look_.player_identity_reference_width == 1920 && look_.reference_height == 1080 &&
            look_.player_identity_padding == Vector2F(16, 10) && look_.paper.keyline_width == 2 &&
            look_.paper.paper == Colour(255, 245, 223) && look_.sprite_sheet == "sprite_sheet_1" &&
            look_.fill_frame == "pixel" && pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) &&
            pixel_.origin() == Vector2F::ZERO && font_->line_spacing() == hud_timer_line_spacing,
            "Timer fixture requires authored normal-clock geometry, paper, shadow and font");
        if (subject_ == HudTimerSubject::closing_text)
        {
            require(look_.closing_clock.text == Colour(20, 20, 20) && look_.closing_clock.paper == Colour(255, 245, 223) &&
                look_.closing_clock.keyline_width == 2 && look_.closing_clock.shadow_offset == 4 &&
                look_.closing_clock.tick_reach == 12,
                "Closing-text fixture requires its own authored opaque text, paper and panel geometry");
        }
        const std::array<std::string, 3> font_names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        for (size_t i = 0; i < font_names.size(); ++i)
        {
            fonts_[i] = app_.render_resources()->resolve_sprite_font(font_names[i]);
            font_pointers_[i] = app_.render_resources()->font(fonts_[i]);
        }
        unchanged();
    }

    void unchanged() const
    {
        auto* resources = app_.render_resources();
        require(&look_ == &presentation_.hud() && &identity_ == &presentation_.player_identity() &&
            hud_timer_fixed_style(look_, subject_) == fixed_ && hud_timer_fixed_style(expected_, subject_) == fixed_ &&
            hud_track_identity_style(identity_) == identity_fixed_ &&
            hud_track_identity_style(expected_identity_) == identity_fixed_ &&
            hud_track_resource_names(look_) == names_ && hud_track_resource_names(expected_) == names_ &&
            hud_track_same_palette(presentation_.team_palette(), palette_) &&
            resources->sprite_sheet(look_.sprite_sheet) == sheet_ &&
            &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) == &pixel_ &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO &&
            app_.viewport_manager()->player_viewport(0).pixel_rect() == RectangleI(0, 0, 1920, 1080),
            "Timer edit changed unselected HUD/shadow/closing/identity/palette, resources or viewport");
        const std::array<std::string, 3> font_names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        for (size_t i = 0; i < fonts_.size(); ++i)
        {
            require(fonts_[i] == resources->resolve_sprite_font(font_names[i]) &&
                font_pointers_[i] == resources->font(fonts_[i]), "Timer edit replaced a captured font handle/resource");
        }
        require(timer_font_ == fonts_[1] && font_ == font_pointers_[1] && font_->line_spacing() == hud_timer_line_spacing,
            "Timer font pointer or source line spacing changed");
        if (subject_ == HudTimerSubject::closing_text)
        {
            require(look_.closing_clock.text.a == 1.0f && expected_.closing_clock.text.a == 1.0f,
                "Closing text must retain its existing opaque-only reader policy");
        }
        struct GlyphPin { char32_t character; RectangleI rectangle; float x, y, advance; };
        const std::array<GlyphPin, 6> glyphs{ {
            { U'0', { 428, 152, 35, 48 }, 11, 15, -9 }, { U'1', { 330, 202, 31, 48 }, 13, 15, -7 },
            { U'2', { 219, 157, 33, 48 }, 12, 15, -8 }, { U'4', { 352, 152, 36, 48 }, 10, 15, -9 },
            { U'5', { 71, 165, 32, 48 }, 12, 15, -7 }, { U':', { 119, 115, 11, 33 }, 13, 30, -7 },
        } };
        for (const auto& pin : glyphs)
        {
            const Glyph* glyph = font_->find(pin.character);
            require(glyph != nullptr && glyph->subrect == pin.rectangle && glyph->x_offset == pin.x &&
                glyph->y_offset == pin.y && glyph->x_advance == pin.advance,
                "Timer source-derived glyph metrics changed");
        }
        for (const auto& item : hud_timer_cases())
        {
            require(resources->measure_text(timer_font_, item.text) == Vector2F(item.width, hud_timer_line_spacing),
                "Timer literal clock text measurement changed");
            const Vector2F position((1920.0f - item.width) / 2.0f, 54.0f);
            float pen_x = 0;
            for (const wchar_t character : std::wstring_view(item.text))
            {
                const Glyph* glyph = font_->find(static_cast<char32_t>(character));
                require(glyph != nullptr, "Timer literal text requires an exact glyph, not a fallback");
                pen_x = std::max(0.0f, pen_x + glyph->x_offset);
                const float x = position.x + pen_x, y = position.y + glyph->y_offset;
                require(x - 1 >= static_cast<float>(roi().x) && y - 1 >= static_cast<float>(roi().y) &&
                    x + static_cast<float>(glyph->subrect.width) + 3 <= static_cast<float>(roi().right()) &&
                    y + static_cast<float>(glyph->subrect.height) + 3 <= static_cast<float>(roi().bottom()),
                    "Timer glyph and two-pixel shadow quads escaped the declared one-pixel-expanded ROI");
                pen_x += static_cast<float>(glyph->subrect.width) + glyph->x_advance;
            }
            // Inside the fixed ROI, stepped corners, right/bottom shade bands
            // and every other HUD component are absent. Its opaque paper is
            // one effective backdrop; changing the Canvas would add no case.
            if (subject_ == HudTimerSubject::closing_text)
            {
                if (!item.normal)
                {
                    // Independently audited CLOSING panel: max((16,10),3*2)
                    // padding, own pen2/shade4, text0:10 at (891.5,54). Detached
                    // ticks are x859..872 / x1052..1065, outside this ROI.
                    const float pen = look_.closing_clock.keyline_width;
                    const Vector2F padding(std::max(look_.player_identity_padding.x, pen * 3),
                        std::max(look_.player_identity_padding.y, pen * 3));
                    const RectangleF face(position - padding, Vector2F(item.width, hud_timer_line_spacing) + padding * 2);
                    const RectangleF inside(face.x + pen, face.y + pen, face.width - 2 * pen, face.height - 2 * pen);
                    require(item.width == 137 && position == Vector2F(891.5f, 54) &&
                        look_.closing_clock.paper == Colour(255, 245, 223) &&
                        look_.closing_clock.keyline_width == 2 && look_.closing_clock.shadow_offset == 4 &&
                        padding == Vector2F(16, 10) && inside.right() - 3 * pen == 1036.5f &&
                        inside.bottom() - 3 * pen == 133 && static_cast<float>(roi().x) >= inside.x + 2 * pen &&
                        static_cast<float>(roi().y) >= inside.y + 2 * pen && static_cast<float>(roi().right()) <= inside.right() - 3 * pen &&
                        static_cast<float>(roi().bottom()) <= inside.bottom() - 3 * pen,
                        "Closing text ROI no longer has its own unshaded opaque paper backdrop");
                }
            }
            else
            {
                const float inside_left = position.x - 14, inside_top = 46;
                const float right_shade = position.x + item.width + 8;
                const float bottom_shade = 54 + hud_timer_line_spacing + 2;
                require(static_cast<float>(roi().x) >= inside_left + 4 && static_cast<float>(roi().y) >= inside_top + 4 &&
                    static_cast<float>(roi().right()) <= right_shade && static_cast<float>(roi().bottom()) <= bottom_shade,
                    "Timer ROI no longer stays entirely on unshaded opaque paper");
            }
        }
    }

    Frame actual(size_t context)
    {
        const auto& item = hud_timer_cases().at(context);
        return render([&](DrawList& list)
        {
            hud_.draw_gameplay_interface_prepared(list, { 1920, 1080 }, 0,
                L"P1  /  TEAM A\nKeyboard/mouse", .5f, .5f, item.seconds, Colour(6, 52, 204), item.reduced, item.running);
        });
    }
    Frame adoption(size_t context, const Colour& colour)
    {
        hud_timer_selected(expected_, subject_) = colour;
        const auto& item = hud_timer_cases().at(context);
        return render([&](DrawList& list)
        {
            copied_.draw_gameplay_interface_prepared(list, { 1920, 1080 }, 0,
                L"P1  /  TEAM A\nKeyboard/mouse", .5f, .5f, item.seconds, Colour(6, 52, 204), item.reduced, item.running);
        });
    }
    Frame raw(size_t context, const Colour& colour, HudTimerReference mode = HudTimerReference::correct)
    {
        const auto& item = hud_timer_cases().at(context);
        const bool closing_text = subject_ == HudTimerSubject::closing_text;
        require(closing_text ? !item.normal : item.normal, "Timer raw oracle requires an applicable selected-role branch");
        return render([&](DrawList& list)
        {
            // Independent ordered text oracle: literal pure paper, selected or
            // unchanged shadow FIRST, then selected or unchanged foreground.
            rectangle(list, roi(), Colour(255, 245, 223));
            list.set_filter(TextureFilter::linear);
            const Vector2F position((1920.0f - item.width) / 2.0f, 54.0f);
            if (closing_text)
            {
                require(colour.a == 1.0f && (mode == HudTimerReference::correct ||
                    mode == HudTimerReference::wrong_opaque || mode == HudTimerReference::added_normal_shadow),
                    "Closing text has no straight/double-alpha discriminator at alpha1");
                if (mode == HudTimerReference::added_normal_shadow)
                {
                    list.draw_text(timer_font_, item.text, position + Vector2F(2, 2), Colour(0, 0, 0, 34),
                        1.0f, 0.0f, Vector2F::ZERO, 0.0f);
                }
                list.draw_text(timer_font_, item.text, position,
                    mode == HudTimerReference::wrong_opaque ? Colour(255, 0, 0) : colour,
                    1.0f, 0.0f, Vector2F::ZERO, 0.0f);
                return; // The actual closing branch never calls normal shadow.
            }
            const Colour selected_tint = mode == HudTimerReference::straight ? colour : mode == HudTimerReference::squared ?
                Colour(colour.r * colour.a * colour.a, colour.g * colour.a * colour.a,
                    colour.b * colour.a * colour.a, colour.a) :
                Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a);
            const auto& foreground = look_.timer_colour;
            const auto& shadow = look_.timer_shadow_colour;
            const Colour foreground_tint = subject_ == HudTimerSubject::foreground ? selected_tint :
                Colour(foreground.r * foreground.a, foreground.g * foreground.a, foreground.b * foreground.a, foreground.a);
            const Colour shadow_tint = subject_ == HudTimerSubject::shadow ? selected_tint :
                Colour(shadow.r * shadow.a, shadow.g * shadow.a, shadow.b * shadow.a, shadow.a);
            const auto draw_foreground = [&]()
            { list.draw_text(timer_font_, item.text, position, foreground_tint, 1.0f, 0.0f, Vector2F::ZERO, 0.0f); };
            const auto draw_shadow = [&]()
            { list.draw_text(timer_font_, item.text, position + Vector2F(2, 2), shadow_tint, 1.0f, 0.0f, Vector2F::ZERO, 0.0f); };
            if (mode == HudTimerReference::reversed_order)
            {
                require(subject_ == HudTimerSubject::shadow, "Reversed timer order is a shadow overlap control only");
                draw_foreground(); draw_shadow();
            }
            else { draw_shadow(); draw_foreground(); }
        });
    }
    Frame normal(size_t context, const Colour& colour, bool copied_adoption)
    { return observe(context, colour, copied_adoption); }
    Frame observe(size_t context, const Colour& colour, bool copied_adoption)
    {
        require(hud_timer_selected(look_, subject_) == colour, "Timer observation selected channels differ");
        const Frame actual_frame = actual(context), repeated = actual(context);
        require(actual_frame.pixels == repeated.pixels, "Timer repeated HUD draw changed RGBA");
        if (copied_adoption)
        {
            require(actual_frame.pixels == adoption(context, colour).pixels,
                "Timer copied full-HUD adoption differs; this is not an independent art oracle");
        }
        normal_reference_ = raw(context, colour); normal_context_ = context; normal_colour_ = colour;
        try { check_roi(actual_frame, normal_reference_); }
        catch (...) { failure(context, "raw-roi", actual_frame, normal_reference_); throw; }
        return actual_frame;
    }
    Frame closing(size_t context)
    {
        require(!hud_timer_cases().at(context).normal, "Timer closing guard requires a closing context");
        const Frame first = actual(context), repeated = actual(context);
        require(first.pixels == repeated.pixels, "Timer closing repeated HUD draw changed RGBA");
        return first;
    }
    Frame unchanged_normal(size_t context)
    {
        require(subject_ == HudTimerSubject::closing_text && hud_timer_cases().at(context).normal,
            "Closing-text normal-branch control requires an inapplicable normal context");
        const Frame first = actual(context), repeated = actual(context);
        require(first.pixels == repeated.pixels, "Closing-text normal branch repeat changed RGBA");
        return first;
    }
    size_t control(size_t context, const Colour& colour, HudTimerReference mode)
    {
        require(normal_context_ == context && normal_colour_ == colour && !normal_reference_.pixels.empty(),
            "Timer negative reference requires its retained independently checked normal reference");
        const Frame wrong = raw(context, colour, mode);
        try
        {
            const size_t differences = check_control(normal_reference_, wrong);
            if (mode == HudTimerReference::reversed_order)
            {
                // Lato atlas audit: at world (911,79), the foreground glyph 2
                // and its (2,2)-shifted shadow are each fully opaque (the
                // authored frame is exact 141414 at (911,79) and (909,77)).
                // Authored opaque foreground hides the correct under-shadow;
                // drawing selected nonblack shadow last must change this site.
                const size_t at = offset(911, 79);
                require(normal_reference_.pixels[at] == 20 && normal_reference_.pixels[at + 1] == 20 &&
                    normal_reference_.pixels[at + 2] == 20 && normal_reference_.pixels[at + 3] == 255 &&
                    !same_pixel(normal_reference_, wrong, at),
                    "Timer reversed-order control lost its source-derived opaque overlap witness");
            }
            if (mode == HudTimerReference::added_normal_shadow)
            {
                // Glyph1 shifted by (2,2) covers (970,117) from its fully
                // opaque base row (968,115), while NO unshifted glyph quad
                // reaches row 117. Correct closing is paper.
                const size_t at = offset(970, 117);
                require(subject_ == HudTimerSubject::closing_text && normal_reference_.pixels[at] == 255 &&
                    normal_reference_.pixels[at + 1] == 245 && normal_reference_.pixels[at + 2] == 223 &&
                    normal_reference_.pixels[at + 3] == 255 && !same_pixel(normal_reference_, wrong, at),
                    "Closing added-shadow control lost its source-derived exposed paper witness");
            }
            return differences;
        }
        catch (...) { failure(context, "negative-control", normal_reference_, wrong); throw; }
    }
    static void check_roi(const Frame& actual_frame, const Frame& raw_frame)
    {
        size_t checked = 0;
        for (int y = roi().y; y < roi().bottom(); ++y) { for (int x = roi().x; x < roi().right(); ++x)
        {
            const size_t at = offset(x, y);
            require(same_pixel(actual_frame, raw_frame, at) && actual_frame.pixels[at + 3] == 255,
                "Timer raw glyph/shadow/paper RGBA reference differs"); ++checked;
        } }
        require(checked == 6812, "Timer independent ROI pixel count differs");
    }
    static size_t check_control(const Frame& correct, const Frame& wrong)
    {
        size_t changed = 0;
        for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
        {
            const bool equal = same_pixel(correct, wrong, offset(x, y));
            if (x < roi().x || x >= roi().right() || y < roi().y || y >= roi().bottom())
            { require(equal, "Timer wrong tint changed the raw reference outside the declared glyph ROI"); }
            else if (!equal) { ++changed; }
        } }
        require(changed > 0 && changed <= 6812, "Timer negative reference failed to discriminate");
        return changed;
    }
    static void locality(const Frame& current, const Frame& baseline, bool changed)
    {
        size_t differences = 0;
        for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
        {
            const bool equal = same_pixel(current, baseline, offset(x, y));
            if (x < roi().x || x >= roi().right() || y < roi().y || y >= roi().bottom())
            { require(equal, "Timer selected tint changed paper/shadow or another HUD component outside the glyph ROI"); }
            else if (!equal) { ++differences; }
        } }
        require(changed ? differences > 0 : differences == 0, "Timer selected normal glyph response differs");
    }
    static RectangleI roi() { return { 901, 68, 131, 52 }; }
    static Frame compact(const Frame& frame) { return foreground_crop(frame, roi(), 0); }
    size_t readbacks() const { return readbacks_; }
    void preserve_failure(size_t context, const char* scope, const Frame& actual_frame, const Frame& reference) const
    { failure(context, scope, actual_frame, reference); }
private:
    static size_t offset(int x, int y) { return (static_cast<size_t>(y) * 1920 + static_cast<size_t>(x)) * 4; }
    static bool same_pixel(const Frame& a, const Frame& b, size_t at)
    { return std::equal(a.pixels.begin() + at, a.pixels.begin() + at + 4, b.pixels.begin() + at); }
    void failure(size_t context, const char* scope, const Frame& actual_frame, const Frame& reference) const
    {
        const char* prefix = subject_ == HudTimerSubject::foreground ? "hud-timer-failure-" :
            subject_ == HudTimerSubject::shadow ? "hud-timer-shadow-failure-" : "hud-closing-text-failure-";
        const std::string stem = std::string(prefix) +
            hud_timer_cases().at(context).name + '-' + scope;
        save(compact(actual_frame), output_ / (stem + "-actual.ppm"));
        save(compact(reference), output_ / (stem + "-reference.ppm"));
        const auto rgba = [&](const Frame& frame, const char* suffix)
        {
            std::ofstream file(output_ / (stem + suffix), std::ios::binary);
            file.write(reinterpret_cast<const char*>(frame.pixels.data()), static_cast<std::streamsize>(frame.pixels.size()));
            require(file.good(), "Cannot preserve timer failure RGBA");
        };
        rgba(actual_frame, "-actual.rgba"); rgba(reference, "-reference.rgba");
        std::ofstream record(output_ / (stem + ".json"));
        record << "{\"scope\":\"" << scope << "\",\"context\":\"" << hud_timer_cases().at(context).name
            << "\",\"frame_width\":1920,\"frame_height\":1080,\"roi_x\":901,\"roi_y\":68,\"roi_width\":131,\"roi_height\":52,"
            "\"readbacks\":" << readbacks_ << ",\"diagnostic\":\"source-hypothesis-failure; no pixel fitting or tolerance\"}\n";
        require(record.good(), "Cannot preserve timer failure receipt");
    }
    void rectangle(DrawList& list, const RectangleI& box, const Colour& colour) const
    {
        list.set_filter(TextureFilter::point);
        list.draw_sprite(sheet_->texture(), pixel_.source_rectangle(),
            RectangleF(static_cast<float>(box.x), static_cast<float>(box.y), static_cast<float>(box.width), static_cast<float>(box.height)),
            colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
    }
    Frame render(const std::function<void(DrawList&)>& draw)
    {
        const size_t budget = subject_ == HudTimerSubject::foreground ? 55U : subject_ == HudTimerSubject::shadow ? 56U : 50U;
        unchanged(); require(readbacks_ < budget, "Timer probe exceeded its declared per-entrypoint readback budget");
        const std::array<Colour, 6> before{ look_.timer_colour, look_.timer_shadow_colour, look_.closing_clock.text,
            expected_.timer_colour, expected_.timer_shadow_colour, expected_.closing_clock.text };
        std::srand(46017); const int next = std::rand(); std::srand(46017);
        Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->player_viewport(0));
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas_.draw(list, { 0, 0, 1920, 1080 }); draw(list);
        rectangle(list, { 20, 20, 4, 4 }, Colour::white); rectangle(list, { 25, 20, 4, 4 }, Colour::black);
        Frame result = read(renderer); ++readbacks_;
        require(result.width == 1920 && result.height == 1080 && result.pixels.size() == 1920 * 1080 * 4,
            "Timer probe requires a native 1920 by 1080 RGBA backbuffer");
        for (int y = 20; y < 24; ++y) { for (int x = 20; x < 29; ++x)
        {
            if (x == 24) { continue; }
            const size_t at = offset(x, y); const unsigned char channel = x < 24 ? 255 : 0;
            require(result.pixels[at] == channel && result.pixels[at + 1] == channel &&
                result.pixels[at + 2] == channel && result.pixels[at + 3] == 255,
                "Timer actual WHITE/BLACK calibration differs");
        } }
        require(std::rand() == next && before == std::array<Colour, 6>{ look_.timer_colour, look_.timer_shadow_colour, look_.closing_clock.text,
            expected_.timer_colour, expected_.timer_shadow_colour, expected_.closing_clock.text },
            "Timer drawing consumed RNG or changed selected live/copied colours");
        unchanged(); return result;
    }
    HudTimerSubject subject_; Application& app_; const Presentation& presentation_; const HudPresentation& look_;
    const PlayerIdentityPresentation& identity_; HudPresentation expected_; PlayerIdentityPresentation expected_identity_;
    InterfaceGameplay hud_, copied_; Canvas canvas_; const SpriteSheet* sheet_; const SpriteFrame& pixel_;
    FontHandle timer_font_; const Font* font_; std::array<FontHandle, 3> fonts_{};
    std::array<const Font*, 3> font_pointers_{};
    std::vector<float> fixed_, identity_fixed_; std::array<std::string, 5> names_;
    TeamPalettePresentation palette_; const std::filesystem::path output_;
    Frame normal_reference_{}; size_t normal_context_ = 0; Colour normal_colour_{}; size_t readbacks_ = 0;
};

void hud_timer_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output, HudTimerSubject subject = HudTimerSubject::foreground)
{
    require(subject != HudTimerSubject::closing_text, "Opaque closing text uses its own bounded scenario");
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    Presentation local = presentation; auto& look = const_cast<HudPresentation&>(local.hud());
    HudTimerColourProbe probe(app, local, output, subject);
    const bool shadow = subject == HudTimerSubject::shadow;
    const char* prefix = shadow ? "hud-timer-shadow" : "hud-timer";
    const char* main_scope = shadow ? "normal-timer-shadow-glyph-roi" : "normal-timer-glyph-roi";
    const char* branch_scope = shadow ? "public-hud-shadow-branch-counterexample" : "public-hud-branch-counterexample";
    const char* summary_scope = shadow ? "normal-timer-shadow-glyph-roi-and-branch-invariance" : "normal-timer-glyph-roi-and-branch-invariance";
    const size_t expected_reads = shadow ? 56 : 55, expected_controls = shadow ? 4 : 3;
    Colour& selected = hud_timer_selected(look, subject);
    const std::array<Colour, 4> colours{ shadow ? Colour(0, 0, 0, 34) : Colour(20, 20, 20), Colour(53, 188, 225), Colour(32, 96, 192, 128), Colour(165, 50, 214, 0) };
    constexpr std::array<const char*, 4> names{ "default", "rgb", "rgba128", "alpha0" };
    std::array<Frame, 4> main{}; size_t controls = 0, control_pixels = 0, rows = 0, crops = 0;
    std::ofstream csv(output / (std::string(prefix) + "-colour-main.csv"));
    csv << "state,raw_rgba_match,adoption_match,repeat_match,locality_match,frozen_style_resources_rng,controls,discriminating_pixels,scope\n";
    for (size_t state = 0; state < colours.size(); ++state)
    {
        selected = colours[state]; main[state] = probe.normal(0, colours[state], true);
        try { HudTimerColourProbe::locality(main[state], main[0], state != 0); }
        catch (...) { probe.preserve_failure(0, "main-locality", main[state], main[0]); throw; }
        size_t case_controls = 0, case_pixels = 0;
        const auto control = [&](HudTimerReference mode)
        {
            case_pixels += probe.control(0, colours[state], mode); ++case_controls;
        };
        if (state == 2)
        {
            control(HudTimerReference::straight); control(HudTimerReference::squared);
            if (shadow) { control(HudTimerReference::reversed_order); }
        }
        else if (state == 3) { control(HudTimerReference::straight); }
        csv << names[state] << ",true,true,true,true,true," << case_controls << ',' << case_pixels << ',' << main_scope << '\n';
        save(HudTimerColourProbe::compact(main[state]), output / ((std::string(prefix) + "-colour-") + names[state] + ".ppm"));
        ++rows; ++crops; controls += case_controls; control_pixels += case_pixels;
    }
    require(csv.good(), "Cannot retain timer main CSV");
    std::array<std::array<Frame, 8>, 2> branch{};
    std::ofstream branches(output / (std::string(prefix) + "-colour-branches.csv"));
    branches << "state,context,normal_branch,raw_rgba_match,repeat_match,selection_guard,frozen_style_resources_rng,scope\n";
    for (size_t state = 0; state < 2; ++state)
    {
        selected = colours[state == 0 ? 0 : 2];
        for (size_t context = 1; context < hud_timer_cases().size(); ++context)
        {
            const auto& item = hud_timer_cases()[context];
            branch[state][context] = item.normal ? probe.normal(context, selected, false) : probe.closing(context);
            if (state != 0)
            {
                try
                {
                    if (item.normal) { HudTimerColourProbe::locality(branch[state][context], branch[0][context], true); }
                    else { require(branch[state][context].pixels == branch[0][context].pixels, "Normal timer edit changed a closing-clock frame"); }
                }
                catch (...) { probe.preserve_failure(context, "branch-selection", branch[state][context], branch[0][context]); throw; }
            }
            branches << (state == 0 ? "default" : "rgba128") << ',' << item.name << ',' << (item.normal ? "true" : "false")
                << ',' << (item.normal ? "true" : "not-run-closing-invariance-only") << ",true,"
                << (state == 0 ? "baseline" : item.normal ? "normal-locality" : "closing-exact") << ",true," << branch_scope << '\n';
            ++rows;
        }
        require(branch[state][6].pixels == main[state == 0 ? 0 : 2].pixels,
            "Reduce Effects changed the ordinary normal timer frame");
        require(branch[state][3].pixels == branch[state][7].pixels,
            "Settled fractional closing clock differs from reduced-effects closing clock");
        require(branch[state][2].pixels != branch[state][7].pixels,
            "Closing beat counterexample failed to distinguish its optional tick accents");
    }
    require(branches.good() && probe.readbacks() == expected_reads && rows == 18 && crops == 4 && controls == expected_controls && control_pixels > 0 && control_pixels <= expected_controls * 6812,
        "Timer shared exact readback/row/crop/control budget differs");
    std::ofstream summary(output / (std::string(prefix) + "-colour-consumer-summary.json"));
    summary << "{\"scope\":\"" << summary_scope << "\",\"readbacks\":" << expected_reads << ",\"csv_rows\":18,\"main_rows\":4,\"branch_rows\":14,"
        "\"crops\":4,\"crop_width\":131,\"crop_height\":52,\"crop_pixels\":27248,\"ppm_bytes\":81800,\"roi_pixels\":6812,"
        "\"independent_roi_observations\":12,\"roi_pixel_checks\":81744,\"repeat_checks\":18,\"copied_adoption_checks\":4,"
        "\"closing_colour_invariance_checks\":3,\"reduce_effects_checks\":6,\"controls\":" << expected_controls << ",\"control_discriminating_pixels\":" << control_pixels << ',' <<
        "\"calibration_pixel_checks\":" << expected_reads * 32 << ",\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,"
        "\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain timer shared summary");
    if (shadow) { std::cout << "PASS normal HUD timer shadow: 56 readbacks, 18 rows, 4 native 131 by 52 crops; ordered glyph/shadow RGBA, 4 tint/order controls and unchanged closing/foreground.\n"; }
    else { std::cout << "PASS normal HUD timer: 55 readbacks, 18 rows, 4 native 131 by 52 crops; independent font/paper/shadow RGBA, "
        "3 discriminating tint controls and exact closing/Reduce Effects counterexamples. No updates or input polls.\n"; }
}

#if COLOURWARS_DEV_TOOLS
void hud_timer_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output,
    HudTimerSubject subject = HudTimerSubject::foreground)
{
    require(subject != HudTimerSubject::closing_text, "Opaque closing text must not use normal alpha acceptance states");
    const bool shadow = subject == HudTimerSubject::shadow;
    const std::string prefix = shadow ? "hud-timer-shadow" : "hud-timer";
    const std::string path = shadow ? "hud.timer.shadow_colour" : "hud.timer.colour";
    const char* baseline_word = shadow ? "00000022" : "141414";
    const Colour baseline_colour = shadow ? Colour(0, 0, 0, 34) : Colour(20, 20, 20);
    const char* normal_scope = shadow ? "debug-live-normal-timer-shadow" : "debug-live-normal-timer";
    const char* branch_scope = shadow ? "debug-shadow-closing-counterexample" : "debug-closing-counterexample";
    const char* summary_scope = shadow ? "debug-live-normal-timer-shadow-and-closing-invariance" : "debug-live-normal-timer-and-closing-invariance";
    const auto root = output / (prefix + "-tune-scratch"), source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "Timer private scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> bytes;
    for (size_t i = 0; i < files.size(); ++i)
    {
        bytes[i] = tune_colour_bytes(content / files[i]);
        tune_colour_write(source / files[i], bytes[i]); tune_colour_write(staged / files[i], bytes[i]);
    }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreTimer { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestoreTimer() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
    Save preferences; SettingsSession preferences_session(preferences); const auto original_preferences = preferences.save_data();
    const auto preference_values = [&]()
    {
        auto value = limb_preference_values(preferences_session.play_settings());
        value.insert(value.end(), { static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), static_cast<int>(preferences_session.input_bindings().keyboard_jump) });
        return value;
    };
    const auto original_values = preference_values(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    {
        const auto* info = levels.get(name); require(info != nullptr, "Timer fixture requires shipped stage metadata");
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session); const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "crossfeed", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / (prefix + "-tune-commands.log")).string()), "Cannot retain timer command transcript");
    HudTimerColourProbe probe(app, presentation, output, subject); JsonSpanDocument initial(bytes[1], "Timer private source");
    const auto* token = initial.find(path); require(token != nullptr, "Timer existing nested token is absent");
    const std::string saved_bytes = initial.replaced(*token, "\"2060C080\""); bool persisted = false;
    size_t commands = 0, rows = 0, branch_rows = 0; Frame baseline{}, saved{}, closing_before{};
    const auto unchanged = [&]()
    {
        probe.unchanged(); require(preference_values() == original_values &&
            preferences.save_data().resolution == original_preferences.resolution &&
            !std::filesystem::exists(root / "profile/ColourWars/save_data.json") && !impl.in_match() && !impl.can_apply() && impl.request().empty(),
            "Timer command changed private defaults, wrote a game Save or queued gameplay");
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto& expected = i == 1 && persisted ? saved_bytes : bytes[i];
            require(tune_colour_bytes(content / files[i]) == bytes[i] && tune_colour_bytes(source / files[i]) == expected &&
                tune_colour_bytes(staged / files[i]) == expected, "Timer command changed source/staging beyond one exact saved token");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t before = impl.log().lines().size(); std::srand(46019); const int next = std::rand(); std::srand(46019);
        const auto result = impl.execute(line); ++commands;
        require(result.ok && std::rand() == next, "Actual timer command failed or consumed RNG");
        if (line.starts_with("tune "))
        {
            const auto& log = impl.log().lines(); require(log.size() > before && log.back().severity == DebugSeverity::info &&
                log.back().text.starts_with(("tune: presentation." + path + " ")) && log.back().text.ends_with("(live)"),
                "Timer command lacks a fresh live-adoption success reply");
        }
    };
    std::ofstream csv(output / (prefix + "-tune.csv"));
    csv << "state,raw_rgba_match,adoption_match,repeat_match,private_bytes,frozen_style_resources_rng,token,scope\n";
    const auto check = [&](const char* state, const Colour& colour, const char* word)
    {
        unchanged(); require(hud_timer_selected(presentation.hud(), subject) == colour, "Timer command parsed incorrect foreground channels");
        const auto* document = impl.tune_document("presentation"); require(document != nullptr, "Timer current document disappeared");
        const auto* leaf = document->current().find(path);
        require(leaf != nullptr && document->current().token(*leaf) == std::string("\"") + word + '"', "Timer current exact quoted token differs");
        const Frame frame = probe.normal(0, colour, true);
        if (std::string_view(state) == "baseline") { baseline = frame; }
        if (std::string_view(state) == "reset-original") { require(frame.pixels == baseline.pixels, "Timer original reset lost complete HUD RGBA"); }
        if (std::string_view(state) == "partial") { saved = frame; }
        if (std::string_view(state) == "reset-saved" || std::string_view(state) == "final-saved")
        { require(frame.pixels == saved.pixels, "Timer saved reset lost complete HUD RGBA"); }
        if (std::string_view(state) == "named")
        {
            require(document->edits().size() == 1 && document->edits().contains(path) &&
                document->edits().at(path).current_token == "\"RED\"", "Timer named RED token was not retained");
        }
        csv << state << ",true,true,true,true,true," << word << ',' << normal_scope << '\n'; ++rows; unchanged();
    };
    std::ofstream branch_csv(output / (prefix + "-tune-branches.csv"));
    branch_csv << "state,context,closing_unchanged,frozen_style_resources_rng,scope\n";
    check("baseline", baseline_colour, baseline_word); closing_before = probe.actual(2);
    branch_csv << "baseline,exact-ten,baseline,true," << branch_scope << '\n'; ++branch_rows;
    command("tune presentation." + path + " RED"); check("named", Colour(255, 0, 0), "RED");
    command("tune.reset presentation." + path); check("reset-original", baseline_colour, baseline_word);
    command("tune presentation." + path + " 2060C080"); check("partial", Colour(32, 96, 192, 128), "2060C080");
    require(probe.actual(2).pixels == closing_before.pixels, "Actual timer tune command changed a closing-clock frame");
    branch_csv << "partial,exact-ten,true,true," << branch_scope << '\n'; ++branch_rows;
    command("tune.save presentation"); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "Timer one-token Save did not rebase");
    command("tune presentation." + path + " A532D600"); check("zero", Colour(165, 50, 214, 0), "A532D600");
    command("tune.reset presentation." + path); check("reset-saved", Colour(32, 96, 192, 128), "2060C080");
    command("tune presentation." + path + " 35BCE1"); check("rgb", Colour(53, 188, 225), "35BCE1");
    command("tune.reset presentation." + path); check("final-saved", Colour(32, 96, 192, 128), "2060C080");
    require(csv.good() && branch_csv.good() && commands == 8 && rows == 8 && branch_rows == 2 && probe.readbacks() == 34 &&
        impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Timer Debug exact command/state/branch/readback budget or saved final ledger differs");
    std::ofstream summary(output / (prefix + "-tune-summary.json"));
    summary << "{\"scope\":\"" << summary_scope << "\",\"commands\":8,\"states\":8,\"readbacks\":34,\"csv_rows\":10,"
        "\"normal_rows\":8,\"branch_rows\":2,\"crops\":0,\"saved_tokens\":1,\"presentation_save_commands\":1,\"private_game_saves\":0,"
        "\"roi_pixel_checks\":54496,\"repeat_checks\":8,\"copied_adoption_checks\":8,\"closing_colour_invariance_checks\":1,\"calibration_pixel_checks\":1088,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,"
        "\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain timer Debug summary");
    if (shadow) { std::cout << "PASS timer shadow tune: 8 real commands, 8 normal states and 2 closing observations; 34 additional readbacks, one-token private Save/rebase and exact authored-default/saved resets. No game Saves, updates or input polls.\n"; }
    else { std::cout << "PASS timer tune: 8 real commands, 8 normal states and 2 closing observations; 34 additional readbacks, "
        "one-token private Save/rebase and exact original/saved resets. No game Saves, updates or input polls.\n"; }
}
#endif
