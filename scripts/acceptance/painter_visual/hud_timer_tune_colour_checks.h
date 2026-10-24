// Path 46: normal clock foreground only. Source-derived font metrics and the
// fixed pure-paper ROI are hypotheses until the native qualification runs.
enum class HudTimerReference { correct, straight, squared };
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
        { "normal", 123.25f, L"2:04", 130.0f, true, false, true },
        { "above-ten", 10.01f, L"0:11", 120.0f, true, false, true },
        { "exact-ten", 10.0f, L"0:10", 130.0f, true, false, false },
        { "fractional-closing", 9.25f, L"0:10", 130.0f, true, false, false },
        { "zero", 0.0f, L"0:00", 130.0f, true, false, true },
        { "stopped-short", 4.25f, L"0:05", 128.0f, false, false, true },
        { "reduced-normal", 123.25f, L"2:04", 130.0f, true, true, true },
        { "reduced-closing", 10.0f, L"0:10", 130.0f, true, true, false },
    } };
    return cases;
}
std::vector<float> hud_timer_fixed_style(const HudPresentation& look)
{
    auto copy = look;
    copy.timer_colour = Colour(20, 20, 20);
    auto values = hud_track_fixed_style(copy);
    // The track helper omits its own selected tint, so add that unselected
    // field explicitly here. Timer shadow and all closing colours are included.
    const auto& track = look.meter_track_colour;
    values.insert(values.end(), { track.r, track.g, track.b, track.a });
    return values;
}

class HudTimerColourProbe
{
public:
    HudTimerColourProbe(Application& app, const Presentation& presentation, const std::filesystem::path& output) :
        app_(app), presentation_(presentation), look_(presentation.hud()),
        identity_(presentation.player_identity()), expected_(look_), expected_identity_(identity_),
        hud_(app.render_resources(), &look_, &identity_),
        copied_(app.render_resources(), &expected_, &expected_identity_),
        canvas_(app.render_resources(), Colour(32, 64, 96)),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))),
        timer_font_(app.render_resources()->resolve_sprite_font(look_.timer_font)),
        font_(app.render_resources()->font(timer_font_)),
        fixed_(hud_timer_fixed_style(look_)), identity_fixed_(hud_track_identity_style(identity_)),
        names_(hud_track_resource_names(look_)), palette_(presentation.team_palette()), output_(output)
    {
        require(look_.timer_colour == Colour(20, 20, 20) && look_.timer_shadow_colour == Colour(0, 0, 0, 34) &&
            look_.timer_font == "gill_sans_mt_bold_48" && look_.timer_offset == Vector2F(0, .05f) &&
            look_.timer_shadow_offset == Vector2F(2, 2) && look_.timer_reference_width == 1920 &&
            look_.player_identity_reference_width == 1920 && look_.reference_height == 1080 &&
            look_.player_identity_padding == Vector2F(16, 10) && look_.paper.keyline_width == 2 &&
            look_.paper.paper == Colour(255, 245, 223) && look_.sprite_sheet == "sprite_sheet_1" &&
            look_.fill_frame == "pixel" && pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) &&
            pixel_.origin() == Vector2F::ZERO && font_->line_spacing() == 74.21875f,
            "Timer fixture requires authored normal-clock geometry, paper, shadow and font");
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
            hud_timer_fixed_style(look_) == fixed_ && hud_timer_fixed_style(expected_) == fixed_ &&
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
        require(timer_font_ == fonts_[1] && font_ == font_pointers_[1] && font_->line_spacing() == 74.21875f,
            "Timer font pointer or source line spacing changed");
        struct GlyphPin { char32_t character; RectangleI rectangle; float x, y, advance; };
        const std::array<GlyphPin, 6> glyphs{ {
            { U'0', { 311, 57, 31, 47 }, 12, 21, -8 }, { U'1', { 140, 112, 11, 45 }, 22, 22, 2 },
            { U'2', { 262, 109, 30, 46 }, 13, 21, -8 }, { U'4', { 223, 204, 31, 45 }, 12, 22, -8 },
            { U'5', { 32, 112, 27, 46 }, 14, 22, -6 }, { U':', { 322, 154, 12, 33 }, 13, 35, -8 },
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
            require(resources->measure_text(timer_font_, item.text) == Vector2F(item.width, 74.21875f),
                "Timer literal clock text measurement changed");
            const Vector2F position((1920.0f - item.width) / 2.0f, 54.0f);
            float pen_x = 0;
            for (const wchar_t character : std::wstring_view(item.text))
            {
                const Glyph* glyph = font_->find(static_cast<char32_t>(character));
                require(glyph != nullptr, "Timer literal text requires an exact glyph, not a fallback");
                pen_x = std::max(0.0f, pen_x + glyph->x_offset);
                const float x = position.x + pen_x, y = position.y + glyph->y_offset;
                require(x - 1 >= 906 && y - 1 >= 74 && x + static_cast<float>(glyph->subrect.width) + 3 <= 1028 &&
                    y + static_cast<float>(glyph->subrect.height) + 3 <= 125,
                    "Timer glyph and two-pixel shadow quads escaped the declared one-pixel-expanded ROI");
                pen_x += static_cast<float>(glyph->subrect.width) + glyph->x_advance;
            }
            // Inside the fixed ROI, stepped corners, right/bottom shade bands
            // and every other HUD component are absent. Its opaque paper is
            // one effective backdrop; changing the Canvas would add no case.
            const float inside_left = position.x - 14, inside_top = 46;
            const float right_shade = position.x + item.width + 8;
            const float bottom_shade = 54 + 74.21875f + 2;
            require(906 >= inside_left + 4 && 74 >= inside_top + 4 && 1028 <= right_shade &&
                125 <= bottom_shade, "Timer ROI no longer stays entirely on unshaded opaque paper");
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
        expected_.timer_colour = colour;
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
        require(item.normal, "Timer raw oracle covers only the normal foreground branch");
        return render([&](DrawList& list)
        {
            // Independent normal text oracle: literal pure paper, separate
            // unchanged shadow and explicitly multiplied straight colour.
            rectangle(list, roi(), Colour(255, 245, 223));
            list.set_filter(TextureFilter::linear);
            const Vector2F position((1920.0f - item.width) / 2.0f, 54.0f);
            list.draw_text(timer_font_, item.text, position + Vector2F(2, 2), Colour(0, 0, 0, 34),
                1.0f, 0.0f, Vector2F::ZERO, 0.0f);
            const Colour tint = mode == HudTimerReference::straight ? colour : mode == HudTimerReference::squared ?
                Colour(colour.r * colour.a * colour.a, colour.g * colour.a * colour.a,
                    colour.b * colour.a * colour.a, colour.a) :
                Colour(colour.r * colour.a, colour.g * colour.a, colour.b * colour.a, colour.a);
            list.draw_text(timer_font_, item.text, position, tint, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
        });
    }
    Frame normal(size_t context, const Colour& colour, bool copied_adoption)
    {
        require(look_.timer_colour == colour, "Timer observation selected channels differ");
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
    size_t control(size_t context, const Colour& colour, HudTimerReference mode)
    {
        require(normal_context_ == context && normal_colour_ == colour && !normal_reference_.pixels.empty(),
            "Timer negative reference requires its retained independently checked normal reference");
        const Frame wrong = raw(context, colour, mode);
        try { return check_control(normal_reference_, wrong); }
        catch (...) { failure(context, "negative-control", normal_reference_, wrong); throw; }
    }
    static void check_roi(const Frame& actual_frame, const Frame& raw_frame)
    {
        size_t checked = 0;
        for (int y = 74; y < 125; ++y) { for (int x = 906; x < 1028; ++x)
        {
            const size_t at = offset(x, y);
            require(same_pixel(actual_frame, raw_frame, at) && actual_frame.pixels[at + 3] == 255,
                "Timer raw glyph/shadow/paper RGBA reference differs"); ++checked;
        } }
        require(checked == 6222, "Timer independent ROI pixel count differs");
    }
    static size_t check_control(const Frame& correct, const Frame& wrong)
    {
        size_t changed = 0;
        for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
        {
            const bool equal = same_pixel(correct, wrong, offset(x, y));
            if (x < 906 || x >= 1028 || y < 74 || y >= 125)
            { require(equal, "Timer wrong tint changed the raw reference outside the declared glyph ROI"); }
            else if (!equal) { ++changed; }
        } }
        require(changed > 0 && changed <= 6222, "Timer straight/double-alpha reference failed to discriminate");
        return changed;
    }
    static void locality(const Frame& current, const Frame& baseline, bool changed)
    {
        size_t differences = 0;
        for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
        {
            const bool equal = same_pixel(current, baseline, offset(x, y));
            if (x < 906 || x >= 1028 || y < 74 || y >= 125)
            { require(equal, "Timer selected tint changed paper/shadow or another HUD component outside the glyph ROI"); }
            else if (!equal) { ++differences; }
        } }
        require(changed ? differences > 0 : differences == 0, "Timer selected normal glyph response differs");
    }
    static RectangleI roi() { return { 906, 74, 122, 51 }; }
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
        const std::string stem = std::string("hud-timer-failure-") + hud_timer_cases().at(context).name + '-' + scope;
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
            << "\",\"frame_width\":1920,\"frame_height\":1080,\"roi_x\":906,\"roi_y\":74,\"roi_width\":122,\"roi_height\":51,"
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
        unchanged(); require(readbacks_ < 55, "Timer probe exceeded its declared per-entrypoint readback budget");
        const Colour before = look_.timer_colour, expected_before = expected_.timer_colour;
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
        require(std::rand() == next && look_.timer_colour == before && expected_.timer_colour == expected_before,
            "Timer drawing consumed RNG or changed selected live/copied colours");
        unchanged(); return result;
    }
    Application& app_; const Presentation& presentation_; const HudPresentation& look_;
    const PlayerIdentityPresentation& identity_; HudPresentation expected_; PlayerIdentityPresentation expected_identity_;
    InterfaceGameplay hud_, copied_; Canvas canvas_; const SpriteSheet* sheet_; const SpriteFrame& pixel_;
    FontHandle timer_font_; const Font* font_; std::array<FontHandle, 3> fonts_{};
    std::array<const Font*, 3> font_pointers_{};
    std::vector<float> fixed_, identity_fixed_; std::array<std::string, 5> names_;
    TeamPalettePresentation palette_; const std::filesystem::path output_;
    Frame normal_reference_{}; size_t normal_context_ = 0; Colour normal_colour_{}; size_t readbacks_ = 0;
};

void hud_timer_colour_consumer_checks(Application& app, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    Presentation local = presentation; auto& look = const_cast<HudPresentation&>(local.hud());
    HudTimerColourProbe probe(app, local, output);
    const std::array<Colour, 4> colours{ Colour(20, 20, 20), Colour(53, 188, 225), Colour(32, 96, 192, 128), Colour(165, 50, 214, 0) };
    constexpr std::array<const char*, 4> names{ "default", "rgb", "rgba128", "alpha0" };
    std::array<Frame, 4> main{}; size_t controls = 0, control_pixels = 0, rows = 0, crops = 0;
    std::ofstream csv(output / "hud-timer-colour-main.csv");
    csv << "state,raw_rgba_match,adoption_match,repeat_match,locality_match,frozen_style_resources_rng,controls,discriminating_pixels,scope\n";
    for (size_t state = 0; state < colours.size(); ++state)
    {
        look.timer_colour = colours[state]; main[state] = probe.normal(0, colours[state], true);
        try { HudTimerColourProbe::locality(main[state], main[0], state != 0); }
        catch (...) { probe.preserve_failure(0, "main-locality", main[state], main[0]); throw; }
        size_t case_controls = 0, case_pixels = 0;
        const auto control = [&](HudTimerReference mode)
        {
            case_pixels += probe.control(0, colours[state], mode); ++case_controls;
        };
        if (state == 2) { control(HudTimerReference::straight); control(HudTimerReference::squared); }
        else if (state == 3) { control(HudTimerReference::straight); }
        csv << names[state] << ",true,true,true,true,true," << case_controls << ',' << case_pixels << ",normal-timer-glyph-roi\n";
        save(HudTimerColourProbe::compact(main[state]), output / (std::string("hud-timer-colour-") + names[state] + ".ppm"));
        ++rows; ++crops; controls += case_controls; control_pixels += case_pixels;
    }
    require(csv.good(), "Cannot retain timer main CSV");
    std::array<std::array<Frame, 8>, 2> branch{};
    std::ofstream branches(output / "hud-timer-colour-branches.csv");
    branches << "state,context,normal_branch,raw_rgba_match,repeat_match,selection_guard,frozen_style_resources_rng,scope\n";
    for (size_t state = 0; state < 2; ++state)
    {
        look.timer_colour = colours[state == 0 ? 0 : 2];
        for (size_t context = 1; context < hud_timer_cases().size(); ++context)
        {
            const auto& item = hud_timer_cases()[context];
            branch[state][context] = item.normal ? probe.normal(context, look.timer_colour, false) : probe.closing(context);
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
                << (state == 0 ? "baseline" : item.normal ? "normal-locality" : "closing-exact") << ",true,public-hud-branch-counterexample\n";
            ++rows;
        }
        require(branch[state][6].pixels == main[state == 0 ? 0 : 2].pixels,
            "Reduce Effects changed the ordinary normal timer frame");
        require(branch[state][3].pixels == branch[state][7].pixels,
            "Settled fractional closing clock differs from reduced-effects closing clock");
        require(branch[state][2].pixels != branch[state][7].pixels,
            "Closing beat counterexample failed to distinguish its optional tick accents");
    }
    require(branches.good() && probe.readbacks() == 55 && rows == 18 && crops == 4 && controls == 3 && control_pixels > 0 && control_pixels <= 18666,
        "Timer shared exact readback/row/crop/control budget differs");
    std::ofstream summary(output / "hud-timer-colour-consumer-summary.json");
    summary << "{\"scope\":\"normal-timer-glyph-roi-and-branch-invariance\",\"readbacks\":55,\"csv_rows\":18,\"main_rows\":4,\"branch_rows\":14,"
        "\"crops\":4,\"crop_width\":122,\"crop_height\":51,\"crop_pixels\":24888,\"ppm_bytes\":74720,\"roi_pixels\":6222,"
        "\"independent_roi_observations\":12,\"roi_pixel_checks\":74664,\"repeat_checks\":18,\"copied_adoption_checks\":4,"
        "\"closing_colour_invariance_checks\":3,\"reduce_effects_checks\":6,\"controls\":3,\"control_discriminating_pixels\":" << control_pixels << ',' <<
        "\"calibration_pixel_checks\":1760,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,"
        "\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain timer shared summary");
    std::cout << "PASS normal HUD timer: 55 readbacks, 18 rows, 4 native 122 by 51 crops; independent font/paper/shadow RGBA, "
        "3 discriminating tint controls and exact closing/Reduce Effects counterexamples. No updates or input polls.\n";
}

#if COLOURWARS_DEV_TOOLS
void hud_timer_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto root = output / "hud-timer-tune-scratch", source = root / "source", staged = root / "staged";
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
    Save preferences; const auto original_preferences = preferences.save_data();
    const auto preference_values = [&]()
    {
        auto value = limb_preference_values(preferences.play_settings());
        value.insert(value.end(), { static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), static_cast<int>(preferences.input_bindings().keyboard_jump) });
        return value;
    };
    const auto original_values = preference_values(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    {
        const auto* info = levels.get(name); require(info != nullptr, "Timer fixture requires shipped stage metadata");
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "hud-timer-tune-commands.log").string()), "Cannot retain timer command transcript");
    HudTimerColourProbe probe(app, presentation, output); JsonSpanDocument initial(bytes[1], "Timer private source");
    const auto* token = initial.find("hud.timer.colour"); require(token != nullptr, "Timer existing nested token is absent");
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
                log.back().text.starts_with("tune: presentation.hud.timer.colour ") && log.back().text.ends_with("(live)"),
                "Timer command lacks a fresh live-adoption success reply");
        }
    };
    std::ofstream csv(output / "hud-timer-tune.csv");
    csv << "state,raw_rgba_match,adoption_match,repeat_match,private_bytes,frozen_style_resources_rng,token,scope\n";
    const auto check = [&](const char* state, const Colour& colour, const char* word)
    {
        unchanged(); require(presentation.hud().timer_colour == colour, "Timer command parsed incorrect foreground channels");
        const auto* document = impl.tune_document("presentation"); require(document != nullptr, "Timer current document disappeared");
        const auto* leaf = document->current().find("hud.timer.colour");
        require(leaf != nullptr && document->current().token(*leaf) == std::string("\"") + word + '"', "Timer current exact quoted token differs");
        const Frame frame = probe.normal(0, colour, true);
        if (std::string_view(state) == "baseline") { baseline = frame; }
        if (std::string_view(state) == "reset-original") { require(frame.pixels == baseline.pixels, "Timer original reset lost complete HUD RGBA"); }
        if (std::string_view(state) == "partial") { saved = frame; }
        if (std::string_view(state) == "reset-saved" || std::string_view(state) == "final-saved")
        { require(frame.pixels == saved.pixels, "Timer saved reset lost complete HUD RGBA"); }
        if (std::string_view(state) == "named")
        {
            require(document->edits().size() == 1 && document->edits().contains("hud.timer.colour") &&
                document->edits().at("hud.timer.colour").current_token == "\"RED\"", "Timer named RED token was not retained");
        }
        csv << state << ",true,true,true,true,true," << word << ",debug-live-normal-timer\n"; ++rows; unchanged();
    };
    std::ofstream branch_csv(output / "hud-timer-tune-branches.csv");
    branch_csv << "state,context,closing_unchanged,frozen_style_resources_rng,scope\n";
    check("baseline", Colour(20, 20, 20), "141414"); closing_before = probe.actual(2);
    branch_csv << "baseline,exact-ten,baseline,true,debug-closing-counterexample\n"; ++branch_rows;
    command("tune presentation.hud.timer.colour RED"); check("named", Colour(255, 0, 0), "RED");
    command("tune.reset presentation.hud.timer.colour"); check("reset-original", Colour(20, 20, 20), "141414");
    command("tune presentation.hud.timer.colour 2060C080"); check("partial", Colour(32, 96, 192, 128), "2060C080");
    require(probe.actual(2).pixels == closing_before.pixels, "Actual timer tune command changed a closing-clock frame");
    branch_csv << "partial,exact-ten,true,true,debug-closing-counterexample\n"; ++branch_rows;
    command("tune.save presentation"); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "Timer one-token Save did not rebase");
    command("tune presentation.hud.timer.colour A532D600"); check("zero", Colour(165, 50, 214, 0), "A532D600");
    command("tune.reset presentation.hud.timer.colour"); check("reset-saved", Colour(32, 96, 192, 128), "2060C080");
    command("tune presentation.hud.timer.colour 35BCE1"); check("rgb", Colour(53, 188, 225), "35BCE1");
    command("tune.reset presentation.hud.timer.colour"); check("final-saved", Colour(32, 96, 192, 128), "2060C080");
    require(csv.good() && branch_csv.good() && commands == 8 && rows == 8 && branch_rows == 2 && probe.readbacks() == 34 &&
        impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Timer Debug exact command/state/branch/readback budget or saved final ledger differs");
    std::ofstream summary(output / "hud-timer-tune-summary.json");
    summary << "{\"scope\":\"debug-live-normal-timer-and-closing-invariance\",\"commands\":8,\"states\":8,\"readbacks\":34,\"csv_rows\":10,"
        "\"normal_rows\":8,\"branch_rows\":2,\"crops\":0,\"saved_tokens\":1,\"presentation_save_commands\":1,\"private_game_saves\":0,"
        "\"roi_pixel_checks\":49776,\"repeat_checks\":8,\"copied_adoption_checks\":8,\"closing_colour_invariance_checks\":1,\"calibration_pixel_checks\":1088,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,"
        "\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain timer Debug summary");
    std::cout << "PASS timer tune: 8 real commands, 8 normal states and 2 closing observations; 34 additional readbacks, "
        "one-token private Save/rebase and exact original/saved resets. No game Saves, updates or input polls.\n";
}
#endif
