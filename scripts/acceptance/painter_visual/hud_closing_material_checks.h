// Paths52-54: isolated opaque closing-clock materials. Included after immutable
// hud_timer_tune_colour_checks.h; no existing timer subject or helper is changed.
using ClosingMaterialTuple = std::array<Colour, 3>;
ClosingMaterialTuple closing_materials(const HudPresentation& look)
{ return { look.closing_clock.ink, look.closing_clock.paper, look.closing_clock.accent }; }
void set_closing_materials(HudPresentation& look, const ClosingMaterialTuple& value)
{ look.closing_clock.ink = value[0]; look.closing_clock.paper = value[1]; look.closing_clock.accent = value[2]; }
const std::array<const char*, 3>& closing_material_keys()
{ static const std::array<const char*, 3> keys{ "hud.closing_clock.ink", "hud.closing_clock.paper", "hud.closing_clock.accent" }; return keys; }
struct ClosingMaterialPatch { const char* name; RectangleI box; size_t material; bool tick; };
const std::array<ClosingMaterialPatch, 12>& closing_material_patches()
{
    static const std::array<ClosingMaterialPatch, 12> patches{ {
        // Lato 0:10 (137 x 77) at (891.5,54): face (875.5,44,169,97), whole-pixel
        // floor raster; tick rows 75..78 / 105..108, accents rows 77 / 106.
        { "ink-body-first", { 900, 44, 16, 2 }, 0, false }, { "ink-body-second", { 875, 60, 2, 8 }, 0, false },
        { "paper-first", { 900, 46, 16, 4 }, 1, false }, { "paper-second", { 877, 56, 4, 12 }, 1, false },
        { "ink-tick-left-top", { 859, 75, 14, 1 }, 0, true }, { "ink-tick-left-bottom", { 859, 105, 14, 1 }, 0, true },
        { "ink-tick-right-top", { 1052, 75, 14, 1 }, 0, true }, { "ink-tick-right-bottom", { 1052, 105, 14, 1 }, 0, true },
        { "accent-left-top", { 860, 77, 12, 1 }, 2, true }, { "accent-left-bottom", { 860, 106, 12, 1 }, 2, true },
        { "accent-right-top", { 1053, 77, 12, 1 }, 2, true }, { "accent-right-bottom", { 1053, 106, 12, 1 }, 2, true },
    } }; return patches;
}
std::vector<float> closing_material_fixed_style(const HudPresentation& look)
{
    auto copy = look; set_closing_materials(copy, { Colour(20, 20, 20), Colour(255, 245, 223), Colour(255, 211, 77) });
    auto values = hud_track_fixed_style(copy); const auto& track = look.meter_track_colour;
    values.insert(values.end(), { track.r, track.g, track.b, track.a }); return values;
}
class HudClosingMaterialProbe
{
public:
    HudClosingMaterialProbe(Application& app, const Presentation& presentation, const std::filesystem::path& output, size_t budget) :
        app_(app), presentation_(presentation), look_(presentation.hud()), identity_(presentation.player_identity()),
        expected_(look_), expected_identity_(identity_), hud_(app.render_resources(), &look_, &identity_),
        copied_(app.render_resources(), &expected_, &expected_identity_), canvas_(app.render_resources(), Colour(32, 64, 96)),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)), pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))),
        fixed_(closing_material_fixed_style(look_)), identity_fixed_(hud_track_identity_style(identity_)), names_(hud_track_resource_names(look_)),
        palette_(presentation.team_palette()), output_(output), budget_(budget)
    {
        require(closing_materials(look_) == ClosingMaterialTuple{ Colour(20, 20, 20), Colour(255, 245, 223), Colour(255, 211, 77) } &&
            look_.closing_clock.text == Colour(20, 20, 20) && look_.closing_clock.shade == Colour(0, 0, 0, 85) &&
            look_.closing_clock.keyline_width == 2 && look_.closing_clock.shadow_offset == 4 && look_.closing_clock.tick_reach == 12 &&
            look_.closing_clock.beat_duration == .18f && look_.player_identity_padding == Vector2F(16, 10) &&
            look_.timer_offset == Vector2F(0, .05f) && look_.timer_reference_width == 1920 &&
            look_.player_identity_reference_width == 1920 && look_.reference_height == 1080 &&
            look_.timer_font == "lato_black_48" && look_.sprite_sheet == "sprite_sheet_1" && look_.fill_frame == "pixel",
            "Closing materials require the declared authored colours, clock geometry and source resources");
        const std::array<std::string, 3> names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        for (size_t i = 0; i < names.size(); ++i)
        { fonts_[i] = app_.render_resources()->resolve_sprite_font(names[i]); font_pointers_[i] = app_.render_resources()->font(fonts_[i]); }
        unchanged();
    }
    void unchanged() const
    {
        auto* resources = app_.render_resources();
        require(&look_ == &presentation_.hud() && &identity_ == &presentation_.player_identity() &&
            closing_material_fixed_style(look_) == fixed_ && closing_material_fixed_style(expected_) == fixed_ &&
            hud_track_identity_style(identity_) == identity_fixed_ && hud_track_identity_style(expected_identity_) == identity_fixed_ &&
            hud_track_resource_names(look_) == names_ && hud_track_resource_names(expected_) == names_ &&
            hud_track_same_palette(presentation_.team_palette(), palette_) && resources->sprite_sheet(look_.sprite_sheet) == sheet_ &&
            &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) == &pixel_ &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO &&
            app_.viewport_manager()->player_viewport(0).pixel_rect() == RectangleI(0, 0, 1920, 1080),
            "Closing material edit changed unselected HUD, identity, geometry, palette, captured sheet/frame or viewport");
        const std::array<std::string, 3> names{ look_.meter_label_font, look_.timer_font, look_.player_identity_font };
        for (size_t i = 0; i < names.size(); ++i)
        { require(fonts_[i] == resources->resolve_sprite_font(names[i]) && font_pointers_[i] == resources->font(fonts_[i]), "Closing material font handle/resource changed"); }
        require(font_pointers_[1]->line_spacing() == hud_timer_line_spacing, "Closing timer font source line spacing changed");
        for (const auto& item : hud_timer_cases())
        { require(resources->measure_text(fonts_[1], item.text) == Vector2F(item.width, hud_timer_line_spacing), "Closing literal context font metrics changed"); }
        for (const auto& value : { closing_materials(look_), closing_materials(expected_) })
        { for (const auto& colour : value) { require(colour.a == 1.0f, "Closing material existing alpha-one reader policy changed"); } }
    }
    Frame actual(size_t context, bool copied = false)
    {
        if (copied) { set_closing_materials(expected_, closing_materials(look_)); }
        const auto& item = hud_timer_cases().at(context);
        return render([&](DrawList& list) { (copied ? copied_ : hud_).draw_gameplay_interface_prepared(list, { 1920, 1080 }, 0,
            L"P1  /  TEAM A\nKeyboard/mouse", .5f, .5f, item.seconds, Colour(6, 52, 204), item.reduced, item.running); });
    }
    Frame raw(size_t context, const ClosingMaterialTuple& value, int selected = -1, bool omitted = false)
    {
        require(context == 2 || context == 3 || context == 7, "Closing patch reference requires the exact declared closing contexts");
        return render([&](DrawList& list)
        {
            for (const auto& patch : closing_material_patches())
            {
                // Literal opaque-channel patch oracle, bypassing production
                // fill/CrudeDrawing/HUD helpers. Inactive ticks expose Canvas.
                Colour colour = patch.tick && context != 2 ? Colour(32, 64, 96) : value[patch.material];
                if (selected >= 0 && static_cast<size_t>(selected) == patch.material)
                {
                    require(context == 2, "Wrong/omitted material controls are declared only for the authored active baseline");
                    colour = omitted ? (patch.material == 0 ? Colour(32, 64, 96) : value[0]) : Colour(3, 151, 227);
                }
                rectangle(list, patch.box, colour);
            }
        });
    }
    Frame observe(size_t context, bool copied)
    {
        const Frame first = actual(context), repeated = actual(context);
        try { require(first.pixels == repeated.pixels, "Closing material repeated draw changed complete RGBA"); }
        catch (...) { failure("repeat", context, first, repeated); throw; }
        if (copied)
        {
            const Frame adopted = actual(context, true);
            try { require(first.pixels == adopted.pixels, "Closing copied HUD adoption differs; it is not an independent art oracle"); }
            catch (...) { failure("copied-adoption", context, first, adopted); throw; }
        }
        reference_ = raw(context, closing_materials(look_));
        try { compare_patches(first, reference_); }
        catch (...) { failure("patch-reference", context, first, reference_); throw; }
        return first;
    }
    Frame invariant(size_t context)
    {
        const Frame first = actual(context), repeated = actual(context);
        try { require(first.pixels == repeated.pixels, "Closing normal repeat differs"); }
        catch (...) { failure("normal-repeat", context, first, repeated); throw; }
        return first;
    }
    size_t control(size_t material, bool omitted)
    {
        const Frame wrong = raw(2, closing_materials(look_), static_cast<int>(material), omitted);
        size_t changed = 0, discriminations = 0;
        try
        {
            for (const auto& patch : closing_material_patches())
            {
                size_t support = 0;
                for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
                { if (!same(reference_, wrong, offset(x, y))) { ++support; } } }
                if (patch.material == material) { require(support > 0, "Closing negative material reference lost an independently declared draw-site discriminator"); ++discriminations; }
                else { require(support == 0, "Closing wrong material changed an unselected reference patch"); }
                changed += support;
            }
            for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
            {
                bool selected = false;
                for (const auto& patch : closing_material_patches())
                { selected = selected || (patch.material == material && x >= patch.box.x && x < patch.box.right() && y >= patch.box.y && y < patch.box.bottom()); }
                if (!selected) { require(same(reference_, wrong, offset(x, y)), "Closing wrong reference changed its complete exterior"); }
            } }
            const size_t sites = material == 0 ? 6U : material == 1 ? 2U : 4U;
            require(discriminations == sites && changed > 0 && changed <= (material == 0 ? 104U : material == 1 ? 112U : 48U), "Closing material control source budget differs");
        }
        catch (...) { failure(omitted ? "omitted-material" : "wrong-opaque", 2, reference_, wrong); throw; }
        return changed;
    }
    void locality(const Frame& frame, const Frame& baseline, bool changed) const
    {
        size_t differences = 0; const RectangleI box = envelope();
        for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
        {
            const bool equal = same(frame, baseline, offset(x, y));
            if (x < box.x || x >= box.right() || y < box.y || y >= box.bottom()) { require(equal, "Closing material edit escaped the declared clock envelope"); }
            else if (!equal) { ++differences; }
        } }
        require(changed ? differences > 0 : differences == 0, "Closing active selected-material locality response differs");
    }
    static RectangleI envelope() { return { 858, 43, 209, 103 }; }
    static Frame compact(const Frame& frame) { return foreground_crop(frame, envelope(), 0); }
    void failure(const char* scope, size_t context, const Frame& actual_frame, const Frame& reference) const
    {
        const std::string stem = std::string("hud-closing-materials-failure-") + hud_timer_cases().at(context).name + '-' + scope;
        save(compact(actual_frame), output_ / (stem + "-actual.ppm")); save(compact(reference), output_ / (stem + "-reference.ppm"));
        for (const auto& pair : { std::pair<const Frame*, const char*>(&actual_frame, "-actual.rgba"), { &reference, "-reference.rgba" } })
        { std::ofstream file(output_ / (stem + pair.second), std::ios::binary); file.write(reinterpret_cast<const char*>(pair.first->pixels.data()), static_cast<std::streamsize>(pair.first->pixels.size())); require(file.good(), "Cannot retain closing material failure RGBA"); }
        std::ofstream record(output_ / (stem + ".json"));
        record << "{\"scope\":\"" << scope << "\",\"context\":\"" << hud_timer_cases().at(context).name << "\",\"frame_width\":1920,\"frame_height\":1080,\"crop_x\":858,\"crop_y\":43,\"crop_width\":209,\"crop_height\":103,\"readbacks\":" << readbacks_ << ",\"diagnostic\":\"source-hypothesis-failure; no fitting or tolerance\"}\n";
        require(record.good(), "Cannot retain closing material failure record");
    }
    size_t readbacks() const { return readbacks_; }
private:
    static size_t offset(int x, int y) { return (static_cast<size_t>(y) * 1920 + static_cast<size_t>(x)) * 4; }
    static bool same(const Frame& a, const Frame& b, size_t at)
    { return std::equal(a.pixels.begin() + at, a.pixels.begin() + at + 4, b.pixels.begin() + at); }
    static void compare_patches(const Frame& frame, const Frame& reference)
    {
        size_t checked = 0;
        for (const auto& patch : closing_material_patches()) { for (int y = patch.box.y; y < patch.box.bottom(); ++y) { for (int x = patch.box.x; x < patch.box.right(); ++x)
        { const size_t at = offset(x, y); require(same(frame, reference, at) && frame.pixels[at + 3] == 255, "Closing literal opaque patch RGBA reference differs"); ++checked; } } }
        require(checked == 264, "Closing patch pixel budget differs");
    }
    void rectangle(DrawList& list, const RectangleI& box, const Colour& colour) const
    {
        list.set_filter(TextureFilter::point); list.draw_sprite(sheet_->texture(), pixel_.source_rectangle(),
            RectangleF(static_cast<float>(box.x), static_cast<float>(box.y), static_cast<float>(box.width), static_cast<float>(box.height)),
            colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
    }
    Frame render(const std::function<void(DrawList&)>& draw)
    {
        unchanged(); require(readbacks_ < budget_, "Closing material probe exceeded its frozen readback budget");
        const std::array<ClosingMaterialTuple, 2> held{ closing_materials(look_), closing_materials(expected_) };
        std::srand(525417); const int next = std::rand(); std::srand(525417);
        Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->player_viewport(0)); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); canvas_.draw(list, { 0, 0, 1920, 1080 }); draw(list);
        rectangle(list, { 20, 20, 4, 4 }, Colour::white); rectangle(list, { 25, 20, 4, 4 }, Colour::black);
        Frame frame = read(renderer); ++readbacks_;
        require(frame.width == 1920 && frame.height == 1080 && frame.pixels.size() == 1920 * 1080 * 4, "Closing material fixture requires native 1920 by 1080 RGBA");
        for (int y = 20; y < 24; ++y) { for (int x = 20; x < 29; ++x)
        { if (x == 24) { continue; } const size_t at = offset(x, y); const unsigned char c = x < 24 ? 255 : 0;
            require(frame.pixels[at] == c && frame.pixels[at + 1] == c && frame.pixels[at + 2] == c && frame.pixels[at + 3] == 255, "Closing material WHITE/BLACK calibration differs"); } }
        require(std::rand() == next && held == std::array<ClosingMaterialTuple, 2>{ closing_materials(look_), closing_materials(expected_) }, "Closing material draw consumed RNG or changed selected tuples");
        unchanged(); return frame;
    }
    Application& app_; const Presentation& presentation_; const HudPresentation& look_; const PlayerIdentityPresentation& identity_;
    HudPresentation expected_; PlayerIdentityPresentation expected_identity_; InterfaceGameplay hud_, copied_; Canvas canvas_;
    const SpriteSheet* sheet_; const SpriteFrame& pixel_; std::array<FontHandle, 3> fonts_{}; std::array<const Font*, 3> font_pointers_{};
    std::vector<float> fixed_, identity_fixed_; std::array<std::string, 5> names_; TeamPalettePresentation palette_;
    const std::filesystem::path output_; size_t budget_, readbacks_ = 0; Frame reference_{};
};
void hud_closing_material_consumer_checks(Application& app, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player); Presentation local = presentation; auto& look = const_cast<HudPresentation&>(local.hud());
    HudClosingMaterialProbe probe(app, local, output, 64); const ClosingMaterialTuple original = closing_materials(look);
    const std::array<ClosingMaterialTuple, 5> tuples{ original,
        ClosingMaterialTuple{ Colour(40, 108, 180), original[1], original[2] },
        ClosingMaterialTuple{ original[0], Colour(227, 197, 138), original[2] },
        ClosingMaterialTuple{ original[0], original[1], Colour(184, 81, 198) },
        ClosingMaterialTuple{ Colour(40, 108, 180), Colour(227, 197, 138), Colour(184, 81, 198) } };
    constexpr std::array<const char*, 5> states{ "default", "ink", "paper", "accent", "all" };
    std::ofstream csv(output / "hud-closing-materials-main.csv"), branches(output / "hud-closing-materials-branches.csv"), controls(output / "hud-closing-materials-controls.csv");
    csv << "state,raw_rgba_match,adoption_match,repeat_match,locality_match,opaque_selected,frozen_style_resources_rng,scope\n";
    branches << "state,context,raw_rgba_match,repeat_match,selection_guard,opaque_selected,frozen_style_resources_rng,scope\n";
    controls << "material,control,patch_discriminations,discriminating_pixels,exterior_unchanged,scope\n";
    size_t rows = 0, crops = 0, control_pixels = 0; Frame baseline;
    for (size_t state = 0; state < tuples.size(); ++state)
    {
        set_closing_materials(look, tuples[state]); const Frame frame = probe.observe(2, true); if (state == 0) { baseline = frame; }
        try { probe.locality(frame, baseline, state != 0); } catch (...) { probe.failure("active-locality", 2, frame, baseline); throw; }
        if (state == 0) { for (size_t material = 0; material < 3; ++material) { for (const bool omitted : { false, true })
        {
            const size_t pixels = probe.control(material, omitted); control_pixels += pixels;
            controls << (material == 0 ? "ink" : material == 1 ? "paper" : "accent") << ',' << (omitted ? "omitted" : "wrong-opaque") << ','
                << (material == 0 ? 6 : material == 1 ? 2 : 4) << ',' << pixels << ",true,opaque-material-counterexample\n"; ++rows;
        } } }
        save(HudClosingMaterialProbe::compact(frame), output / (std::string("hud-closing-materials-") + states[state] + ".ppm")); ++crops;
        csv << states[state] << ",true,true,true,true,true,true,opaque-closing-material-patches\n"; ++rows;
    }
    for (const size_t context : { 0U, 1U, 4U, 5U, 6U })
    {
        Frame normal_baseline;
        for (size_t state = 0; state < 2; ++state)
        {
            set_closing_materials(look, tuples[state == 0 ? 0 : 4]); const Frame frame = probe.invariant(context);
            if (state == 0) { normal_baseline = frame; }
            else { try { require(frame.pixels == normal_baseline.pixels, "Closing material edit changed an inapplicable normal complete frame"); }
                catch (...) { probe.failure("normal-invariance", context, frame, normal_baseline); throw; } }
            branches << (state == 0 ? "default" : "all") << ',' << hud_timer_cases()[context].name << ",not-run-normal-invariance-only,true,"
                << (state == 0 ? "baseline" : "normal-exact") << ",true,true,closing-material-applicability\n"; ++rows;
        }
    }
    std::array<Frame, 3> fractional;
    for (const size_t context : { 3U, 7U })
    {
        Frame settled_baseline;
        for (size_t state = 0; state < 3; ++state)
        {
            const size_t tuple = state == 0 ? 0 : state == 1 ? 4 : 3; set_closing_materials(look, tuples[tuple]);
            const Frame frame = probe.observe(context, false); if (state == 0) { settled_baseline = frame; }
            try
            {
                if (state == 1) { probe.locality(frame, settled_baseline, true); }
                if (state == 2) { require(frame.pixels == settled_baseline.pixels, "Inactive closing accent changed settled/reduced complete RGBA"); }
                if (context == 3) { fractional[state] = frame; }
                else { require(frame.pixels == fractional[state].pixels, "Matching settled fractional and reduced closing complete RGBA differ"); }
            }
            catch (...) { probe.failure("settled-reduced", context, frame, settled_baseline); throw; }
            if (state != 0) { save(HudClosingMaterialProbe::compact(frame), output / (std::string("hud-closing-materials-") + hud_timer_cases()[context].name + '-' + states[tuple] + ".ppm")); ++crops; }
            branches << states[tuple] << ',' << hud_timer_cases()[context].name << ",true,true,"
                << (state == 0 ? "baseline" : state == 1 ? "body-locality" : "accent-exact") << ",true,true,closing-material-applicability\n"; ++rows;
        }
    }
    require(baseline.pixels != fractional[0].pixels, "Closing material beat counterexample failed to distinguish detached ticks");
    require(csv.good() && branches.good() && controls.good() && probe.readbacks() == 64 && rows == 27 && crops == 9 && control_pixels > 0 && control_pixels <= 528, "Closing material shared frozen counters differ");
    std::ofstream summary(output / "hud-closing-materials-consumer-summary.json");
    summary << "{\"scope\":\"opaque-closing-material-patches-and-applicability\",\"readbacks\":64,\"csv_rows\":27,\"main_rows\":5,\"branch_rows\":16,\"control_rows\":6,\"crops\":9,\"crop_width\":209,\"crop_height\":103,\"crop_pixels\":193743,\"ppm_bytes\":581364,\"patches\":12,\"patch_pixels\":264,\"independent_observations\":11,\"patch_pixel_checks\":2904,\"repeat_checks\":21,\"copied_adoption_checks\":5,\"normal_invariance_checks\":5,\"accent_invariance_checks\":2,\"settled_reduced_checks\":3,\"beat_counterexamples\":1,\"controls\":6,\"patch_control_discriminations\":24,\"control_discriminating_pixels\":" << control_pixels << ",\"calibration_pixel_checks\":2048,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain closing material shared summary");
    std::cout << "PASS closing materials: 64 reads, 27 rows and 9 native 209 by 103 crops; 12 opaque source patches, 24 per-site negative discriminations and normal/settled/reduced invariance.\n";
}
#if COLOURWARS_DEV_TOOLS
void hud_closing_material_tune_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    const auto root = output / "hud-closing-materials-tune-scratch", source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "Closing material scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> bytes;
    for (size_t i = 0; i < files.size(); ++i)
    { bytes[i] = tune_colour_bytes(content / files[i]); tune_colour_write(source / files[i], bytes[i]); tune_colour_write(staged / files[i], bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreMaterials { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestoreMaterials() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
    Save preferences;
    const auto preferences_now = [&]()
    {
        auto values = limb_preference_values(preferences.play_settings());
        values.insert(values.end(), { static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), static_cast<int>(preferences.input_bindings().keyboard_jump) }); return values;
    };
    const auto original_preferences = preferences_now(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    { const auto* info = levels.get(name); require(info != nullptr, "Closing materials require shipped stage metadata"); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels); data.set_stages(&stages);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "hud-closing-materials-tune-commands.log").string()), "Cannot retain closing material command transcript");
    HudClosingMaterialProbe probe(app, presentation, output, 40); bool persisted = false;
    JsonSpanDocument initial(bytes[1], "Closing materials private source"); const auto* ink = initial.find(closing_material_keys()[0]);
    require(ink != nullptr, "Closing ink exact existing token is absent");
    JsonSpanDocument first(initial.replaced(*ink, "\"RED\""), "Closing ink exact replacement");
    const auto* paper = first.find(closing_material_keys()[1]); require(paper != nullptr, "Closing paper exact token is absent");
    const std::string saved_bytes = first.replaced(*paper, "\"35BCE1FF\"");
    size_t commands = 0, accepted = 0, refused = 0, queries = 0, rows = 0, crops = 0; Frame saved;
    const auto unchanged = [&]()
    {
        probe.unchanged(); require(preferences_now() == original_preferences && !std::filesystem::exists(root / "profile/ColourWars/save_data.json") &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty() && impl.total_updates() == 0 && impl.level_updates() == 0,
            "Closing material commands changed preferences, saved a game, requested gameplay or advanced updates");
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto& expected = i == 1 && persisted ? saved_bytes : bytes[i];
            require(tune_colour_bytes(content / files[i]) == bytes[i] && tune_colour_bytes(source / files[i]) == expected &&
                tune_colour_bytes(staged / files[i]) == expected, "Closing material private bytes escaped the exact two-token Save");
        }
    };
    const auto document_state = [&]()
    {
        std::vector<std::string> state;
        for (const char* name : { "presentation", "tuning" })
        {
            const auto* document = impl.tune_document(name); require(document != nullptr, "Closing material document disappeared");
            state.insert(state.end(), { name, document->baseline().text(), document->current().text(), std::to_string(document->revision()) });
            for (const auto& [key, edit] : document->edits()) { state.insert(state.end(), { key, edit.baseline_token, edit.current_token }); }
        }
        for (const char* name : files)
        { state.push_back(tune_colour_bytes(content / name)); state.push_back(tune_colour_bytes(source / name)); state.push_back(tune_colour_bytes(staged / name)); }
        return state;
    };
    const auto command = [&](const std::string& line, bool should_accept, bool query = false)
    {
        unchanged(); const size_t before = impl.log().lines().size(); std::srand(525423); const int next = std::rand(); std::srand(525423);
        const auto status = impl.execute(line); ++commands; require(status.ok == should_accept && std::rand() == next, "Closing material actual command acceptance/RNG differs");
        const auto& log = impl.log().lines(); require(log.size() > before, "Closing material actual command lacks a fresh reply");
        if (should_accept)
        {
            ++accepted;
            if (query)
            { ++queries; require(log.back().severity == DebugSeverity::info && log.back().text.starts_with(line.substr(5) + " = ") && log.back().text.ends_with("[live]"), "Closing material value-less tune query lacks exact live reply"); }
            else if (line.starts_with("tune "))
            { require(log.back().severity == DebugSeverity::info && log.back().text.starts_with("tune: presentation.hud.closing_clock.") && log.back().text.ends_with("(live)"), "Closing material edit lacks fresh live-adoption reply"); }
        }
        else { ++refused; require(log.back().text.find("opaque ink/paper/text/accent") != std::string::npos, "Closing partial alpha refusal lacks actual reader diagnostic"); }
    };
    std::ofstream csv(output / "hud-closing-materials-tune.csv");
    csv << "state,ink_token,paper_token,accent_token,raw_rgba_match,adoption_match,repeat_match,siblings_preserved,private_bytes,opaque_selected,frozen_style_resources_rng,scope\n";
    const auto check = [&](const char* state, const ClosingMaterialTuple& tuple, const std::array<const char*, 3>& words, bool crop)
    {
        unchanged(); require(closing_materials(presentation.hud()) == tuple, "Closing material selected/sibling channels differ");
        const auto* document = impl.tune_document("presentation"); require(document != nullptr, "Closing material document disappeared");
        for (size_t i = 0; i < words.size(); ++i)
        { const auto* token = document->current().find(closing_material_keys()[i]); require(token && document->current().token(*token) == std::string("\"") + words[i] + '"', "Closing material exact selected/sibling token differs"); }
        const Frame frame = probe.observe(2, true);
        if (std::string_view(state) == "paired-pending")
        {
            saved = frame; require(document->edits().size() == 2 && document->edits().at(closing_material_keys()[0]).current_token == "\"RED\"" &&
                document->edits().at(closing_material_keys()[1]).current_token == "\"35BCE1FF\"", "Closing two-token pending ledger differs");
        }
        if (std::string_view(state) == "paired-saved" || std::string_view(state) == "reset-accent" || std::string_view(state) == "final-saved")
        { require(frame.pixels == saved.pixels, "Closing selected reset or paired Save lost complete saved HUD RGBA"); }
        if (crop) { save(HudClosingMaterialProbe::compact(frame), output / (std::string("hud-closing-materials-tune-") + state + ".ppm")); ++crops; }
        csv << state << ',' << words[0] << ',' << words[1] << ',' << words[2] << ",true,true,true,true,true,true,true,debug-live-opaque-closing-materials\n";
        ++rows; unchanged();
    };
    const Colour original_ink(20, 20, 20), original_paper(255, 245, 223), original_accent(255, 211, 77), red(255, 0, 0), cyan(53, 188, 225), blue(32, 96, 192), orange(225, 124, 53), purple(165, 50, 214);
    check("baseline", { original_ink, original_paper, original_accent }, { "141414", "FFF5DF", "FFD34D" }, true);
    for (const char* key : closing_material_keys())
    {
        const auto before = document_state(); const auto colours = closing_materials(presentation.hud());
        command(std::string("tune presentation.") + key, true, true);
        require(document_state() == before && closing_materials(presentation.hud()) == colours, "Closing value-less tune query mutated documents or materials");
    }
    command("tune presentation.hud.closing_clock.ink RED", true);
    check("ink-edit", { red, original_paper, original_accent }, { "RED", "FFF5DF", "FFD34D" }, false);
    command("tune presentation.hud.closing_clock.paper 35BCE1FF", true);
    check("paired-pending", { red, cyan, original_accent }, { "RED", "35BCE1FF", "FFD34D" }, true);
    command("tune.save presentation", true); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty() && impl.tune_document("presentation")->baseline().text() == saved_bytes,
        "Closing exact two-token Save did not rebase the private baseline");
    check("paired-saved", { red, cyan, original_accent }, { "RED", "35BCE1FF", "FFD34D" }, false);
    command("tune presentation.hud.closing_clock.ink 2060C0FF", true); command("tune presentation.hud.closing_clock.accent E17C35", true);
    check("ink-accent-overwrite", { blue, cyan, orange }, { "2060C0FF", "35BCE1FF", "E17C35" }, true);
    command("tune.reset presentation.hud.closing_clock.ink", true);
    check("reset-ink", { red, cyan, orange }, { "RED", "35BCE1FF", "E17C35" }, false);
    command("tune.reset presentation.hud.closing_clock.accent", true);
    check("reset-accent", { red, cyan, original_accent }, { "RED", "35BCE1FF", "FFD34D" }, false);
    command("tune presentation.hud.closing_clock.paper A532D6FF", true); command("tune presentation.hud.closing_clock.accent 35BCE1FF", true);
    check("paper-accent-overwrite", { red, purple, cyan }, { "RED", "A532D6FF", "35BCE1FF" }, false);
    command("tune.reset presentation.hud.closing_clock.paper", true);
    // Equal opaque paper/accent is reader-valid; this is not a distinctness rule.
    check("reset-paper", { red, cyan, cyan }, { "RED", "35BCE1FF", "35BCE1FF" }, false);
    command("tune.reset presentation.hud.closing_clock.accent", true);
    check("final-saved", { red, cyan, original_accent }, { "RED", "35BCE1FF", "FFD34D" }, true);
    std::ofstream refusals(output / "hud-closing-materials-tune-refusals.json");
    refusals << "{\"scope\":\"atomic-opaque-reader-refusals-no-raster-read\",\"records\":[";
    for (size_t i = 0; i < closing_material_keys().size(); ++i)
    {
        unchanged(); const auto before = document_state(); const auto tuple = closing_materials(presentation.hud());
        const auto* presentation_document = impl.tune_document("presentation"); const auto* tuning_document = impl.tune_document("tuning");
        command(std::string("tune presentation.") + closing_material_keys()[i] + " 2060C080", false);
        require(document_state() == before && closing_materials(presentation.hud()) == tuple && impl.tune_document("presentation") == presentation_document &&
            impl.tune_document("tuning") == tuning_document, "Opaque material refusal changed runtime, document identity/current/baseline/revision/ledger or private bytes");
        refusals << (i == 0 ? "" : ",") << "{\"path\":\"presentation." << closing_material_keys()[i] << "\",\"input\":\"2060C080\",\"accepted\":false,\"atomic_preserved\":true,\"opaque_selected\":true,\"documents_preserved\":true,\"bytes_preserved\":true,\"resources_rng_preserved\":true,\"raster_checked\":false}";
        unchanged();
    }
    refusals << "]}\n";
    require(csv.good() && refusals.good() && commands == 17 && accepted == 14 && queries == 3 && refused == 3 && rows == 10 && crops == 4 && probe.readbacks() == 40 &&
        impl.tune_document("presentation")->edits().empty() && impl.tune_document("tuning")->edits().empty(), "Closing material Debug frozen command/row/crop/readback counters or final ledger differ");
    std::ofstream summary(output / "hud-closing-materials-tune-summary.json");
    summary << "{\"scope\":\"debug-live-opaque-closing-materials\",\"commands\":17,\"accepted_commands\":14,\"queries\":3,\"refused_commands\":3,\"states\":10,\"readbacks\":40,\"csv_rows\":10,\"refusal_json_records\":3,\"refusal_raster_reads\":0,\"crops\":4,\"crop_pixels\":86108,\"ppm_bytes\":258384,\"saved_tokens\":2,\"presentation_save_commands\":1,\"patch_pixel_checks\":2640,\"repeat_checks\":10,\"copied_adoption_checks\":10,\"sibling_checks\":10,\"saved_frame_equalities\":3,\"calibration_pixel_checks\":1280,\"private_game_saves\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot retain closing material Debug summary");
    std::cout << "PASS closing material commands: 17 statements (3 value-less live queries, 11 mutations/Save, 3 atomic alpha refusals), 10 rendered states, 40 extra reads, 4 crops and one exact two-token private Save. Refusals have no raster reads.\n";
}
#endif