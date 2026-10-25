// Field55: positive-alpha closing-clock shade. This isolated leaf preserves
// the opaque52-54 fixtures and their authored fixed-shade guards unchanged.
// Independent scope: eight literal material patches (128 pixels), not all art.
enum class ClosingShadeReference { correct, straight, squared, black_band, no_scale, missing_bottom, missing_right };
struct ClosingShadePatch { const char* name; RectangleI box; size_t layers; };
const std::array<ClosingShadePatch, 8>& closing_shade_patches()
{
    static const std::array<ClosingShadePatch, 8> patches{ {
        { "base-first", { 900, 139, 8, 2 }, 1 }, { "base-second", { 1042, 80, 2, 8 }, 1 },
        { "bottom-one", { 900, 130, 8, 2 }, 1 }, { "bottom-two", { 900, 132, 8, 2 }, 2 },
        { "bottom-three", { 900, 134, 8, 2 }, 3 }, { "right-one", { 1033, 80, 2, 8 }, 1 },
        { "right-two", { 1035, 80, 2, 8 }, 2 }, { "right-three", { 1037, 80, 2, 8 }, 3 },
    } }; return patches;
}
const std::array<Colour, 4>& closing_shade_values()
{
    static const std::array<Colour, 4> values{ Colour(0, 0, 0, 85), Colour(255, 0, 0),
        Colour(32, 96, 192, 128), Colour(165, 50, 214, 1) }; return values;
}
std::vector<float> closing_shade_fixed_style(const HudPresentation& look)
{
    auto copy = look; copy.closing_clock.shade = Colour(0, 0, 0, 85);
    auto values = hud_track_fixed_style(copy); const auto& track = look.meter_track_colour;
    values.insert(values.end(), { track.r, track.g, track.b, track.a });
    return values; // Only closing shade is masked: ink/paper/accent/text remain guarded.
}
using ClosingShadeFrames = std::array<std::array<Frame, 2>, 4>;
class HudClosingShadeProbe
{
public:
    HudClosingShadeProbe(Application& app, const Presentation& presentation, const std::filesystem::path& output, size_t budget) :
        app_(app), presentation_(presentation), look_(presentation.hud()), identity_(presentation.player_identity()),
        expected_(look_), expected_identity_(identity_), hud_(app.render_resources(), &look_, &identity_),
        copied_(app.render_resources(), &expected_, &expected_identity_), black_(app.render_resources(), Colour::black),
        nonblack_(app.render_resources(), Colour(32, 64, 96)),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)), pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))),
        fixed_(closing_shade_fixed_style(look_)), identity_fixed_(hud_track_identity_style(identity_)), names_(hud_track_resource_names(look_)),
        palette_(presentation.team_palette()), output_(output), budget_(budget)
    {
        require(look_.closing_clock.shade == Colour(0, 0, 0, 85) && look_.closing_clock.ink == Colour(20, 20, 20) &&
            look_.closing_clock.paper == Colour(255, 245, 223) && look_.closing_clock.text == Colour(20, 20, 20) &&
            look_.closing_clock.accent == Colour(255, 211, 77) && look_.closing_clock.keyline_width == 2 &&
            look_.closing_clock.shadow_offset == 4 && look_.closing_clock.tick_reach == 12 && look_.closing_clock.beat_duration == .18f &&
            look_.player_identity_padding == Vector2F(16, 10) && look_.timer_offset == Vector2F(0, .05f) &&
            look_.timer_reference_width == 1920 && look_.player_identity_reference_width == 1920 && look_.reference_height == 1080 &&
            look_.timer_font == "lato_black_48" && look_.sprite_sheet == "sprite_sheet_1" && look_.fill_frame == "pixel",
            "Closing shade source materials, geometry or resources changed");
        const std::array<std::string, 3> names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        for (size_t i = 0; i < names.size(); ++i)
        { fonts_[i] = app_.render_resources()->resolve_sprite_font(names[i]); font_pointers_[i] = app_.render_resources()->font(fonts_[i]); }
        unchanged();
    }
    void unchanged() const
    {
        auto* resources = app_.render_resources();
        require(&look_ == &presentation_.hud() && &identity_ == &presentation_.player_identity() &&
            closing_shade_fixed_style(look_) == fixed_ && closing_shade_fixed_style(expected_) == fixed_ &&
            hud_track_identity_style(identity_) == identity_fixed_ && hud_track_identity_style(expected_identity_) == identity_fixed_ &&
            hud_track_resource_names(look_) == names_ && hud_track_resource_names(expected_) == names_ &&
            hud_track_same_palette(presentation_.team_palette(), palette_) && resources->sprite_sheet(look_.sprite_sheet) == sheet_ &&
            &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) == &pixel_ &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO &&
            app_.viewport_manager()->player_viewport(0).pixel_rect() == RectangleI(0, 0, 1920, 1080) &&
            look_.closing_clock.shade.a > 0.0f && expected_.closing_clock.shade.a > 0.0f,
            "Shade edit changed unselected HUD/closing materials, identity, palette, geometry, resources or positive-alpha domain");
        const std::array<std::string, 3> names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        for (size_t i = 0; i < names.size(); ++i)
        { require(fonts_[i] == resources->resolve_sprite_font(names[i]) && font_pointers_[i] == resources->font(fonts_[i]), "Closing shade font resource changed"); }
        require(font_pointers_[1]->line_spacing() == 74.21875f, "Closing shade timer font line spacing changed");
        for (const auto& item : hud_timer_cases())
        { require(resources->measure_text(fonts_[1], item.text) == Vector2F(item.width, 74.21875f), "Closing shade literal timer context metrics changed"); }
    }
    Frame actual(size_t context, size_t background, bool copied = false)
    {
        if (copied) { expected_.closing_clock.shade = look_.closing_clock.shade; }
        const auto& item = hud_timer_cases().at(context);
        return render(context, background, [&](DrawList& list)
        { (copied ? copied_ : hud_).draw_gameplay_interface_prepared(list, { 1920, 1080 }, 0,
            L"P1  /  TEAM A\nKeyboard/mouse", .5f, .5f, item.seconds, Colour(6, 52, 204), item.reduced, item.running); });
    }
    Frame raw(size_t background, const Colour& colour, ClosingShadeReference mode = ClosingShadeReference::correct)
    {
        return render(2, background, [&](DrawList& list)
        {
            for (size_t site = 0; site < closing_shade_patches().size(); ++site)
            {
                const auto& patch = closing_shade_patches()[site]; const bool band = site >= 2;
                if (band) { rectangle(list, patch.box, Colour(255, 245, 223)); }
                const float alpha = colour.a * (band && mode != ClosingShadeReference::no_scale ? .12f : 1.0f);
                const Colour tint = mode == ClosingShadeReference::straight ? Colour(colour.r, colour.g, colour.b, alpha) :
                    mode == ClosingShadeReference::squared ? Colour(colour.r * alpha * alpha, colour.g * alpha * alpha, colour.b * alpha * alpha, alpha) :
                    band && mode == ClosingShadeReference::black_band ? Colour(0.0f, 0.0f, 0.0f, alpha) :
                    Colour(colour.r * alpha, colour.g * alpha, colour.b * alpha, alpha);
                for (size_t layer = 0; layer < patch.layers; ++layer)
                {
                    // Source passes are 3,2,1. At depth2/3, the middle pass is
                    // layer1; the one-pass outer band has no middle layer.
                    const bool missing = layer == 1 && ((mode == ClosingShadeReference::missing_bottom && site >= 2 && site <= 4) ||
                        (mode == ClosingShadeReference::missing_right && site >= 5));
                    if (!missing) { rectangle(list, patch.box, tint); }
                }
            }
        });
    }
    Frame observe(size_t context, size_t background, bool copied)
    {
        const Frame frame = actual(context, background), repeated = actual(context, background);
        equality("repeat", context, frame, repeated);
        if (copied) { const Frame adopted = actual(context, background, true); equality("copied-adoption", context, frame, adopted); }
        return frame;
    }
    void equality(const char* stage, size_t context, const Frame& frame, const Frame& reference, bool equal = true) const
    {
        try { require((frame.pixels == reference.pixels) == equal, "Closing shade complete-frame relation differs"); }
        catch (...) { failure(stage, context, frame, reference); throw; }
    }
    void patches(const char* stage, size_t context, const Frame& frame, const Frame& reference, size_t first = 0, size_t last = 8, bool equal = true) const
    {
        try
        {
            require(first < last && last <= 8, "Closing shade patch range differs");
            for (size_t site = first; site < last; ++site)
            {
                const auto& patch = closing_shade_patches()[site]; size_t different = 0;
                for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
                { const size_t at = offset(x, y); require(frame.pixels[at + 3] == 255 && reference.pixels[at + 3] == 255, "Closing shade patch lost opaque output alpha"); if (!same(frame, reference, at)) { ++different; } } }
                require(equal ? different == 0 : different > 0, "Closing shade independent per-site RGBA relation differs");
            }
        }
        catch (...) { failure(stage, context, frame, reference); throw; }
    }
    void locality(size_t context, const Frame& frame, const Frame& baseline, bool changed) const
    {
        try
        {
            size_t differences = 0; const auto box = envelope();
            for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
            {
                const bool equal = same(frame, baseline, offset(x, y));
                if (x < box.x || x >= box.right() || y < box.y || y >= box.bottom())
                { require(equal, "Closing shade edit escaped the source clock envelope"); }
                else if (!equal) { ++differences; }
            } }
            require(changed ? differences > 0 : differences == 0, "Closing shade selected complete-frame response differs");
        }
        catch (...) { failure("locality", context, frame, baseline); throw; }
    }
    size_t control(size_t background, ClosingShadeReference mode, const Frame& reference)
    {
        const Frame wrong = raw(background, closing_shade_values()[2], mode); size_t changed = 0, sites = 0;
        const auto selected = [&](size_t site)
        {
            if (mode == ClosingShadeReference::straight || mode == ClosingShadeReference::squared) { return true; }
            if (mode == ClosingShadeReference::black_band || mode == ClosingShadeReference::no_scale) { return site >= 2; }
            return mode == ClosingShadeReference::missing_bottom ? site == 3 || site == 4 : site == 6 || site == 7;
        };
        try
        {
            for (size_t site = 0; site < 8; ++site)
            {
                const auto& patch = closing_shade_patches()[site]; size_t support = 0;
                for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
                { if (!same(reference, wrong, offset(x, y))) { ++support; } } }
                require(selected(site) ? support > 0 : support == 0, "Closing shade negative control lost a per-site discriminator or changed unselected material");
                if (selected(site)) { ++sites; } changed += support;
            }
            for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
            {
                bool inside = false;
                for (size_t site = 0; site < 8; ++site) { const auto& box = closing_shade_patches()[site].box;
                    inside = inside || (selected(site) && x >= box.x && x < box.right() && y >= box.y && y < box.bottom()); }
                if (!inside) { require(same(reference, wrong, offset(x, y)), "Closing shade negative control changed complete raw exterior"); }
            } }
            const size_t expected = mode == ClosingShadeReference::straight || mode == ClosingShadeReference::squared ? 8U :
                mode == ClosingShadeReference::black_band || mode == ClosingShadeReference::no_scale ? 6U : 2U;
            require(sites == expected && changed > 0 && changed <= expected * 16, "Closing shade control source support budget differs");
        }
        catch (...) { failure("negative-control", 2, reference, wrong); throw; }
        return changed;
    }
    static RectangleI envelope() { return { 862, 43, 202, 100 }; }
    static Frame compact(const Frame& frame) { return foreground_crop(frame, envelope(), 0); }
    size_t readbacks() const { return readbacks_; }
    void failure(const char* stage, size_t context, const Frame& frame, const Frame& reference) const
    {
        const std::string stem = std::string("closing-shade-failure-") + hud_timer_cases().at(context).name + '-' + stage + '-' + std::to_string(readbacks_);
        save(compact(frame), output_ / (stem + "-actual.ppm")); save(compact(reference), output_ / (stem + "-reference.ppm"));
        for (const auto& item : { std::pair<const Frame*, const char*>(&frame, "-actual.rgba"), { &reference, "-reference.rgba" } })
        { std::ofstream file(output_ / (stem + item.second), std::ios::binary); file.write(reinterpret_cast<const char*>(item.first->pixels.data()), static_cast<std::streamsize>(item.first->pixels.size())); require(file.good(), "Cannot preserve closing shade failure RGBA"); }
        std::ofstream record(output_ / (stem + ".json"));
        record << "{\"scope\":\"" << stage << "\",\"context\":\"" << hud_timer_cases().at(context).name << "\",\"frame_width\":1920,\"frame_height\":1080,\"crop_x\":862,\"crop_y\":43,\"crop_width\":202,\"crop_height\":100,\"readbacks\":" << readbacks_ << ",\"diagnostic\":\"source-hypothesis-failure; no fitting or tolerance\"}\n";
        require(record.good(), "Cannot retain closing shade failure record");
    }
private:
    static size_t offset(int x, int y) { return (static_cast<size_t>(y) * 1920 + static_cast<size_t>(x)) * 4; }
    static bool same(const Frame& a, const Frame& b, size_t at)
    { return std::equal(a.pixels.begin() + at, a.pixels.begin() + at + 4, b.pixels.begin() + at); }
    void rectangle(DrawList& list, const RectangleI& box, const Colour& colour) const
    {
        list.set_filter(TextureFilter::point); list.draw_sprite(sheet_->texture(), pixel_.source_rectangle(),
            RectangleF(static_cast<float>(box.x), static_cast<float>(box.y), static_cast<float>(box.width), static_cast<float>(box.height)),
            colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
    }
    Frame render(size_t context, size_t background, const std::function<void(DrawList&)>& draw)
    {
        unchanged(); require(background < 2 && context < 8 && readbacks_ < budget_, "Closing shade read budget or declared context differs");
        const std::array<Colour, 2> selected{ look_.closing_clock.shade, expected_.closing_clock.shade };
        std::srand(55017); const int next = std::rand(); std::srand(55017);
        Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->player_viewport(0)); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); (background == 0 ? black_ : nonblack_).draw(list, { 0, 0, 1920, 1080 }); draw(list);
        rectangle(list, { 20, 20, 4, 4 }, Colour::white); rectangle(list, { 25, 20, 4, 4 }, Colour::black);
        Frame frame = read(renderer); ++readbacks_;
        require(frame.width == 1920 && frame.height == 1080 && frame.pixels.size() == 1920 * 1080 * 4, "Closing shade requires native 1920 by 1080 RGBA");
        for (int y = 20; y < 24; ++y) { for (int x = 20; x < 29; ++x)
        { if (x == 24) { continue; } const size_t at = offset(x, y); const unsigned char c = x < 24 ? 255 : 0;
            require(frame.pixels[at] == c && frame.pixels[at + 1] == c && frame.pixels[at + 2] == c && frame.pixels[at + 3] == 255, "Closing shade WHITE/BLACK RGBA calibration differs"); } }
        require(std::rand() == next && selected == std::array<Colour, 2>{ look_.closing_clock.shade, expected_.closing_clock.shade }, "Closing shade draw consumed RNG or mutated selected channels");
        unchanged(); return frame;
    }
    Application& app_; const Presentation& presentation_; const HudPresentation& look_; const PlayerIdentityPresentation& identity_;
    HudPresentation expected_; PlayerIdentityPresentation expected_identity_; InterfaceGameplay hud_, copied_; Canvas black_, nonblack_;
    const SpriteSheet* sheet_; const SpriteFrame& pixel_; std::array<FontHandle, 3> fonts_{}; std::array<const Font*, 3> font_pointers_{};
    std::vector<float> fixed_, identity_fixed_; std::array<std::string, 5> names_; TeamPalettePresentation palette_;
    const std::filesystem::path output_; size_t budget_, readbacks_ = 0;
};
ClosingShadeFrames hud_closing_shade_consumer_checks(HudClosingShadeProbe& probe, HudPresentation& look, const std::filesystem::path& output)
{
    ClosingShadeFrames references{}, main{};
    constexpr std::array<const char*, 4> states{ "default", "opaque-red", "partial", "low-alpha" };
    constexpr std::array<const char*, 2> backgrounds{ "black", "nonblack" };
    std::ofstream csv(output / "closing-shade-main.csv"), branches(output / "closing-shade-branches.csv"), controls(output / "closing-shade-controls.csv");
    csv << "state,background,raw_rgba_match,adoption_match,repeat_match,locality_match,positive_alpha,frozen_style_resources_rng,scope\n";
    branches << "state,context,raw_rgba_match,repeat_match,selection_guard,positive_alpha,frozen_style_resources_rng,scope\n";
    controls << "control,background,patch_discriminations,discriminating_pixels,exterior_unchanged,scope\n";
    size_t rows = 0, crops = 0, control_pixels = 0;
    for (size_t state = 0; state < 4; ++state)
    {
        look.closing_clock.shade = closing_shade_values()[state];
        for (size_t background = 0; background < 2; ++background)
        {
            main[state][background] = probe.observe(2, background, true);
            references[state][background] = probe.raw(background, closing_shade_values()[state]);
            probe.patches("main-reference", 2, main[state][background], references[state][background]);
            probe.locality(2, main[state][background], main[0][background], state != 0);
            csv << states[state] << ',' << backgrounds[background] << ",true,true,true,true,true,true,closing-shade-material-patches\n"; ++rows;
            save(HudClosingShadeProbe::compact(main[state][background]), output / (std::string("closing-shade-colour-") + states[state] + '-' + backgrounds[background] + ".ppm")); ++crops;
        }
        // Opaque input makes the exposed BASE independent of Canvas. Derived
        // bands still have alpha .12, but their opaque paper underlay is fixed.
        probe.patches("band-canvas-invariance", 2, main[state][0], main[state][1], 2, 8);
        probe.patches("base-canvas-relation", 2, main[state][0], main[state][1], 0, 2, state == 1);
    }
    const std::array<ClosingShadeReference, 6> modes{ ClosingShadeReference::straight, ClosingShadeReference::squared,
        ClosingShadeReference::black_band, ClosingShadeReference::no_scale, ClosingShadeReference::missing_bottom, ClosingShadeReference::missing_right };
    constexpr std::array<const char*, 6> mode_names{ "straight", "double-submitted-alpha", "forced-black-band", "omitted-band-scale", "omitted-middle-bottom", "omitted-middle-right" };
    look.closing_clock.shade = closing_shade_values()[2];
    for (size_t mode = 0; mode < modes.size(); ++mode)
    {
        for (size_t background = mode < 2 ? 0U : 1U; background < 2; ++background)
        {
            const size_t pixels = probe.control(background, modes[mode], references[2][background]); control_pixels += pixels;
            controls << mode_names[mode] << ',' << backgrounds[background] << ',' << (mode < 2 ? 8 : mode < 4 ? 6 : 2) << ','
                << pixels << ",true,closing-shade-material-counterexample\n"; ++rows;
        }
    }
    std::array<std::array<Frame, 8>, 2> branch{};
    for (const size_t context : { 0U, 1U, 3U, 4U, 5U, 6U, 7U })
    {
        for (size_t state = 0; state < 2; ++state)
        {
            const size_t palette = state == 0 ? 0 : 2; const auto& item = hud_timer_cases()[context];
            look.closing_clock.shade = closing_shade_values()[palette];
            branch[state][context] = probe.observe(context, 1, false);
            if (!item.normal)
            {
                probe.patches("branch-reference", context, branch[state][context], references[palette][1]);
                probe.locality(context, branch[state][context], branch[0][context], state != 0);
                save(HudClosingShadeProbe::compact(branch[state][context]), output / (std::string("closing-shade-colour-") + item.name + '-' + states[palette] + ".ppm")); ++crops;
            }
            else if (state != 0) { probe.equality("normal-invariance", context, branch[state][context], branch[0][context]); }
            branches << states[palette] << ',' << item.name << ',' << (item.normal ? "not-run-normal-invariance-only" : "true")
                << ",true," << (state == 0 ? "baseline" : item.normal ? "normal-exact" : "closing-locality")
                << ",true,true,closing-shade-applicability\n"; ++rows;
        }
    }
    for (size_t state = 0; state < 2; ++state)
    {
        probe.equality("normal-reduced", 6, branch[state][6], branch[state][0]);
        probe.equality("settled-reduced", 7, branch[state][7], branch[state][3]);
        probe.equality("beat-counterexample", 7, branch[state][7], main[state == 0 ? 0 : 2][1], false);
    }
    require(csv.good() && branches.good() && controls.good() && probe.readbacks() == 68 && rows == 30 && crops == 12 &&
        control_pixels > 0 && control_pixels <= 768, "Closing shade shared frozen counters differ");
    std::ofstream summary(output / "closing-shade-consumer-summary.json");
    summary << "{\"scope\":\"closing-shade-material-patches-and-applicability\",\"main_observations\":8,\"main_reads\":32,\"branch_observations\":14,\"branch_reads\":28,\"raw_references\":8,\"controls\":8,\"control_reads\":8,\"control_site_discriminations\":48,\"control_pixel_upper_bound\":768,\"readbacks\":68,\"main_rows\":8,\"branch_rows\":14,\"control_rows\":8,\"csv_rows\":30,\"repeat_checks\":22,\"copied_adoption_checks\":8,\"independent_patch_observations\":12,\"patch_pixel_checks\":1536,\"normal_invariance_checks\":5,\"normal_reduced_relations\":2,\"settled_reduced_relations\":2,\"beat_counterexamples\":2,\"crops\":12,\"calibration_pixel_checks\":2176,\"band_canvas_invariance_checks\":4,\"band_canvas_pixel_checks\":384,\"base_canvas_site_relations\":8,\"base_canvas_relation_pixels\":128,\"patches\":8,\"patch_pixels\":128,\"crop_width\":202,\"crop_height\":100,\"crop_pixels\":242400,\"ppm_bytes\":727380,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0,\"control_discriminating_pixels\":" << control_pixels << "}\n";
    require(summary.good(), "Cannot retain closing shade shared summary");
    look.closing_clock.shade = closing_shade_values()[0];
    std::cout << "PASS closing shade: 68 reads, 30 rows, 12 native crops; 128 literal RGBA patch pixels, 48 per-site negative discriminations and normal/settled/reduced applicability.\n";
    return references;
}
#if COLOURWARS_DEV_TOOLS
void hud_closing_shade_tune_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output, HudClosingShadeProbe& probe, const ClosingShadeFrames& references)
{
    const auto root = output / "closing-shade-tune-scratch", source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "Closing shade private scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> bytes;
    for (size_t i = 0; i < files.size(); ++i)
    { bytes[i] = tune_colour_bytes(content / files[i]); tune_colour_write(source / files[i], bytes[i]); tune_colour_write(staged / files[i], bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    Save preferences;
    const auto preferences_now = [&]()
    {
        auto values = limb_preference_values(preferences.play_settings());
        values.insert(values.end(), { static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), static_cast<int>(preferences.input_bindings().keyboard_jump) }); return values;
    };
    const auto original_preferences = preferences_now(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    { const auto* info = levels.get(name); require(info != nullptr, "Closing shade requires shipped stage metadata"); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "closing-shade-tune-commands.log").string()), "Cannot retain closing shade command transcript");
    constexpr const char* path = "hud.closing_clock.shade";
    JsonSpanDocument initial(bytes[1], "Closing shade private source"); const auto* token = initial.find(path);
    require(token != nullptr, "Closing shade exact existing token is absent");
    const std::string saved_bytes = initial.replaced(*token, "\"2060C080\""); bool persisted = false;
    size_t commands = 0, accepted = 0, queries = 0, tunes = 0, resets = 0, saves = 0, refused = 0, rows = 0;
    const size_t initial_reads = probe.readbacks(); Frame baseline{}, saved{};
    const auto unchanged = [&]()
    {
        probe.unchanged(); require(preferences_now() == original_preferences && !std::filesystem::exists(root / "profile/ColourWars/save_data.json") &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty() && impl.total_updates() == 0 && impl.level_updates() == 0,
            "Closing shade commands changed preferences, saved a game, requested gameplay or advanced updates");
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto& expected = i == 1 && persisted ? saved_bytes : bytes[i];
            require(tune_colour_bytes(content / files[i]) == bytes[i] && tune_colour_bytes(source / files[i]) == expected &&
                tune_colour_bytes(staged / files[i]) == expected, "Closing shade private bytes escaped one exact saved token");
        }
    };
    const auto document_state = [&]()
    {
        std::vector<std::string> state;
        for (const char* name : { "presentation", "tuning" })
        {
            const auto* document = impl.tune_document(name); require(document != nullptr, "Closing shade document disappeared");
            state.insert(state.end(), { name, document->baseline().text(), document->current().text(), std::to_string(document->revision()) });
            for (const auto& [key, edit] : document->edits()) { state.insert(state.end(), { key, edit.baseline_token, edit.current_token }); }
        }
        for (const char* name : files)
        { state.push_back(tune_colour_bytes(content / name)); state.push_back(tune_colour_bytes(source / name)); state.push_back(tune_colour_bytes(staged / name)); }
        return state;
    };
    const auto command = [&](const std::string& line, bool should_accept, bool query = false)
    {
        unchanged(); const size_t before = impl.log().lines().size(); std::srand(55023); const int next = std::rand(); std::srand(55023);
        const auto status = impl.execute(line); ++commands;
        require(status.ok == should_accept && std::rand() == next, "Closing shade actual command acceptance/RNG differs");
        const auto& log = impl.log().lines(); require(log.size() > before, "Closing shade actual command lacks a fresh reply");
        if (!should_accept)
        { ++refused; require(log.back().text.find("visible shade required") != std::string::npos, "Closing zero-alpha refusal lacks actual positive-alpha reader diagnostic"); return; }
        ++accepted;
        if (query)
        { ++queries; require(log.back().severity == DebugSeverity::info && log.back().text.starts_with("presentation.hud.closing_clock.shade = ") && log.back().text.ends_with("[live]"), "Closing shade value-less query lacks fresh live reply"); }
        else if (line.starts_with("tune "))
        { ++tunes; require(log.back().severity == DebugSeverity::info && log.back().text.starts_with("tune: presentation.hud.closing_clock.shade ") && log.back().text.ends_with("(live)"), "Closing shade edit lacks fresh live-adoption reply"); }
        else if (line.starts_with("tune.reset ")) { ++resets; }
        else if (line == "tune.save presentation") { ++saves; }
    };
    std::ofstream csv(output / "closing-shade-tune.csv");
    csv << "state,token,raw_rgba_match,adoption_match,repeat_match,unchanged_frame,private_bytes,positive_alpha,frozen_style_resources_rng,scope\n";
    const auto check = [&](const char* state, const char* word, size_t palette)
    {
        unchanged(); require(presentation.hud().closing_clock.shade == closing_shade_values()[palette], "Closing shade selected live channels differ");
        const auto* document = impl.tune_document("presentation"); require(document != nullptr, "Closing shade document disappeared");
        const auto* leaf = document->current().find(path);
        require(leaf && document->current().token(*leaf) == std::string("\"") + word + '"', "Closing shade exact current quoted token differs");
        const Frame frame = probe.observe(2, 1, true); probe.patches("debug-cached-reference", 2, frame, references[palette][1]);
        if (std::string_view(state) == "baseline") { baseline = frame; }
        if (std::string_view(state) == "reset-original") { probe.equality("reset-original", 2, frame, baseline); }
        if (std::string_view(state) == "restored-partial") { saved = frame; }
        if (std::string_view(state) == "reset-saved") { probe.equality("reset-saved", 2, frame, saved); }
        if (std::string_view(state) == "named")
        { require(document->edits().size() == 1 && document->edits().at(path).current_token == "\"RED\"", "Closing shade named RED exact pending token differs"); }
        csv << state << ',' << word << ",true,true,true,not-run-accepted-state,true,true,true,debug-live-closing-shade\n"; ++rows; unchanged();
    };
    check("baseline", "00000055", 0);
    const auto before_query = document_state(); const Colour query_colour = presentation.hud().closing_clock.shade;
    command("tune presentation.hud.closing_clock.shade", true, true);
    require(document_state() == before_query && presentation.hud().closing_clock.shade == query_colour, "Closing shade query changed documents/bytes/live channels");
    command("tune presentation.hud.closing_clock.shade RED", true); check("named", "RED", 1);
    command("tune.reset presentation.hud.closing_clock.shade", true); check("reset-original", "00000055", 0);
    command("tune presentation.hud.closing_clock.shade 2060C080", true); check("partial", "2060C080", 2);
    command("tune presentation.hud.closing_clock.shade A532D601", true); check("low-alpha", "A532D601", 3);
    command("tune presentation.hud.closing_clock.shade 2060C080", true); check("restored-partial", "2060C080", 2);
    unchanged(); const auto before_refusal = document_state(); const Colour held = presentation.hud().closing_clock.shade;
    const auto* presentation_document = impl.tune_document("presentation"); const auto* tuning_document = impl.tune_document("tuning");
    command("tune presentation.hud.closing_clock.shade A532D600", false);
    require(document_state() == before_refusal && presentation.hud().closing_clock.shade == held &&
        impl.tune_document("presentation") == presentation_document && impl.tune_document("tuning") == tuning_document,
        "Closing zero-alpha refusal changed live channels, document identity/baseline/current/revision/ledgers or source/staging bytes");
    unchanged(); const Frame refused_frame = probe.actual(2, 1); probe.equality("zero-refusal-held-frame", 2, refused_frame, saved);
    csv << "zero-refused,2060C080,not-run-refusal-unchanged-frame-only,not-run-refusal-unchanged-frame-only,not-run-refusal-unchanged-frame-only,true,true,true,true,debug-closing-shade-zero-refusal\n"; ++rows;
    std::ofstream refusal(output / "closing-shade-tune-refusal.json");
    refusal << "{\"scope\":\"atomic-positive-alpha-reader-refusal-with-one-held-frame-read\",\"path\":\"presentation.hud.closing_clock.shade\",\"input\":\"A532D600\",\"accepted\":false,\"atomic_preserved\":true,\"positive_alpha\":true,\"documents_preserved\":true,\"bytes_preserved\":true,\"resources_rng_preserved\":true,\"raster_checked\":true,\"raster_reads\":1}\n";
    command("tune.save presentation", true); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("presentation")->baseline().text() == saved_bytes &&
        impl.tune_document("presentation")->current().text() == saved_bytes && presentation.hud().closing_clock.shade == held,
        "Closing shade one-token Save did not rebase baseline/current/ledger or preserve held live channels");
    command("tune presentation.hud.closing_clock.shade FF0000", true); check("rgb", "FF0000", 1);
    command("tune.reset presentation.hud.closing_clock.shade", true); check("reset-saved", "2060C080", 2);
    require(csv.good() && refusal.good() && commands == 10 && accepted == 9 && queries == 1 && tunes == 5 && resets == 2 && saves == 1 && refused == 1 &&
        rows == 9 && probe.readbacks() - initial_reads == 25 && impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(),
        "Closing shade Debug frozen counters or final ledgers differ");
    std::ofstream summary(output / "closing-shade-tune-summary.json"); summary << "{\"scope\":\"debug-live-positive-alpha-closing-shade\",\"commands\":10,\"query_commands\":1,\"tune_commands\":5,\"reset_commands\":2,\"presentation_save_commands\":1,\"refused_commands\":1,\"accepted_rendered_states\":8,\"accepted_state_reads\":24,\"refusal_reads\":1,\"readbacks\":25,\"accepted_rows\":8,\"refusal_rows\":1,\"csv_rows\":9,\"repeat_checks\":8,\"copied_adoption_checks\":8,\"raw_gpu_reads\":0,\"cached_patch_observations\":8,\"patch_pixel_checks\":1024,\"saved_tokens\":1,\"crops\":0,\"calibration_pixel_checks\":800,\"refusal_json_records\":1,\"refusal_raster_reads\":1,\"saved_frame_equalities\":2,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}" << '\n';
    require(summary.good(), "Cannot retain closing shade Debug summary");
    std::cout << "PASS closing shade tune: 10 actual statements, 8 accepted rendered states and 1 atomic AA00 refusal with a held-frame read; 25 additional reads, one-token Save/rebase and original/saved reset. No game Saves or updates.\n";
}
#endif
void hud_closing_shade_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    struct RestoreShade { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestoreShade() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
#if COLOURWARS_DEV_TOOLS
    constexpr size_t budget = 93;
#else
    constexpr size_t budget = 68; (void)levels; (void)content;
#endif
    HudClosingShadeProbe probe(app, presentation, output, budget);
    auto& look = const_cast<HudPresentation&>(presentation.hud());
    const ClosingShadeFrames references = hud_closing_shade_consumer_checks(probe, look, output);
#if COLOURWARS_DEV_TOOLS
    hud_closing_shade_tune_checks(app, levels, tuning, presentation, content, output, probe, references);
#else
    (void)references; std::cout << "PASS hud-closing-shade-only Release consumer scope; no developer commands executed\n";
#endif
    require(probe.readbacks() == budget, "Closing shade isolated leaf combined readback budget differs");
}
