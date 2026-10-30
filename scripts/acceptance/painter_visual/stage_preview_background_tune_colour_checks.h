// Field51 is captured by the actual Stage Select UiTexture. These tests use
// public pages/input only; the small material oracle does not reconstruct art.
enum class PreviewBackingReference { correct, straight, squared };
struct PreviewBackingCounts
{
    size_t reads = 0, updates = 0, polls = 0, taps = 0, opens = 0, reopens = 0;
};
class PreviewBackingProbe
{
public:
    PreviewBackingProbe(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
        GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output,
        const char* phase, DebugDirector* debug = nullptr) : app_(app), presentation_(presentation),
        look_(presentation.hud()), canvas_(canvas), input_(app.gamepads(), app.keyboard(), &tuning),
        context_(&data_, &input_, &settings_), stack_(app),
        sheet_(app.render_resources()->sprite_sheet(presentation.menu().sprite_sheet)),
        pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(presentation.menu().fill_frame))),
        fixed_(fixed_style(presentation)), identity_fixed_(hud_track_identity_style(presentation.player_identity())),
        palette_(presentation.team_palette()), output_(output), phase_(phase),
        initial_keyboard_(app.keyboard()->state()), initial_previous_(app.keyboard()->previous_state()),
        initial_mouse_focus_(app.mouse()->focused()), initial_activation_(stack_.active()),
        initial_pads_(painter_scripted_pads::states)
    {
        require(stack_.depth() == 0 && initial_keyboard_.keys[0] == 0 && initial_keyboard_.keys[1] == 0 &&
            initial_previous_.keys[0] == 0 && initial_previous_.keys[1] == 0,
            "Preview backing fixture requires an empty stack and neutral private keyboard");
        const auto& menu = presentation.menu(); const auto& geometry = presentation.main_menu().stage_select;
        require(look_.player_identity_background_colour == Colour(18, 24, 39, 238) &&
            geometry.preview_position == Vector2F(150, 330) && geometry.preview_size == Vector2F(1010, 500) &&
            geometry.name_position == Vector2F(150, 245) && geometry.navigation_position == Vector2F(150, 865) &&
            menu.rule_height == 2 && menu.panel_edge_colour == Colour(68, 82, 104) &&
            geometry.selected == Colour(117, 237, 215) && menu.panel_edge_colour.a == 1 && geometry.selected.a == 1 &&
            menu.sprite_sheet == "sprite_sheet_1" && menu.fill_frame == "pixel" &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO &&
            app.viewport_manager()->fullscreen_viewport().pixel_rect() == RectangleI(0, 0, 1920, 1080),
            "Preview backing authored RGBA, underlay, layout or native viewport hypothesis changed");
        for (const char* name : { "crossfeed", "close_quarters" })
        {
            const auto* info = levels.get(name); require(info != nullptr, "Preview backing needs shipped arena metadata");
            stages_.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
        }
        // Accepted Crossfeed is now in Random. This fixture still needs one
        // excluded and one eligible record, so it holds Crossfeed's copy
        // excluded; the captured pages and pixels are those it always read.
        require(stages_[0].include_in_random, "Preview backing expected accepted Crossfeed to be eligible for Random");
        stages_[0].include_in_random = false;
        require(stages_[0].display_name == "Crossfeed" && !stages_[0].include_in_random && stages_[1].include_in_random &&
            random_stage_indices(stages_) == std::vector<size_t>{ 1 } &&
            levels.get("crossfeed")->zoom_out_finish_bounds_rectangle() == RectangleF(0, 500, 3600, 880),
            "Preview backing Crossfeed/eligible Random source geometry changed");
        auto& play = const_cast<PlaySettings&>(preferences_.play_settings());
        play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
        preferences_before_ = preference_values();
        data_.set_application(&app); data_.set_save(&preferences_); stage_catalog_ = acceptance::catalog_of(levels, stages_, { "crossfeed" }); data_.set_stage_catalog(&stage_catalog_);
        data_.set_tuning(&tuning); data_.set_presentation(&presentation); if (debug) { data_.set_debug(debug); }
        font_names_ = { menu.heading_font, menu.item_font, menu.detail_font, menu.brand_font,
            presentation.main_menu().announcement_font, geometry.record_font };
        for (size_t i = 0; i < font_names_.size(); ++i)
        { fonts_[i] = app.render_resources()->resolve_sprite_font(font_names_[i]); font_pointers_[i] = app.render_resources()->font(fonts_[i]); }
        const RectangleF preview(150, 330, 1010, 500), sample(180, 350, 64, 32);
        colourwars::UiTextDropShadow question("source-preview-question-bound", L"?", font_names_[4], Vector2F::ZERO,
            app.render_resources(), menu.highlight, menu.shadow_colour, menu.title_shadow_offset);
        question.set_position(preview.center() - question.bounds().center());
        require(preview.contains(sample) && !sample.intersects(question.bounds()), "Source backing ROI intersects the Random question");
        for (const wchar_t* text : { L"Crossfeed", L"Random" })
        {
            const colourwars::UiTextDropShadow name("source-preview-name-bound", text, menu.item_font, geometry.name_position,
                app.render_resources(), Colour::white, Colour::black, menu.item_shadow_offset);
            require(!sample.intersects(name.bounds()), "Source backing ROI intersects a stage name");
        }
        unchanged();
    }
    ~PreviewBackingProbe() { if (!closed_) { try { close(); } catch (...) {} } }
    GameData& data() { return data_; }
    const PreviewBackingCounts& counts() const { return counts_; }
    Colour captured() const { return captured_; }
    bool random() const { return slot_ == 2; }
    bool locked() const { return locked_; }
    void set_source(const Colour& colour) { const_cast<HudPresentation&>(presentation_.hud()).player_identity_background_colour = colour; }
    void unchanged() const
    {
        auto* resources = app_.render_resources();
        require(&look_ == &presentation_.hud() && fixed_style(presentation_) == fixed_ &&
            hud_track_identity_style(presentation_.player_identity()) == identity_fixed_ &&
            hud_track_same_palette(presentation_.team_palette(), palette_) && preference_values() == preferences_before_ &&
            resources->sprite_sheet(presentation_.menu().sprite_sheet) == sheet_ &&
            &sheet_->sprite_frame(sheet_->resolve_sprite_frame(presentation_.menu().fill_frame)) == &pixel_ &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO &&
            app_.viewport_manager()->fullscreen_viewport().pixel_rect() == RectangleI(0, 0, 1920, 1080),
            "Preview backing changed unselected HUD, identity, palette, preferences, relevant menu geometry or resources");
        for (size_t i = 0; i < fonts_.size(); ++i)
        { require(fonts_[i] == resources->resolve_sprite_font(font_names_[i]) && font_pointers_[i] == resources->font(fonts_[i]), "Preview backing replaced a captured font resource"); }
        for (const auto& pad : painter_scripted_pads::states)
        { require(!pad.connected && pad.buttons == 0 && pad.left_stick == Vector2F::ZERO && pad.right_stick == Vector2F::ZERO &&
            pad.left_trigger == 0 && pad.right_trigger == 0, "Preview backing altered disconnected scripted pad inputs"); }
        if (opened_ && !closed_)
        {
            require(stack_.depth() == 2 && !context_.music_exit_pending() && settings_.player_count() == 1 &&
                settings_.player_settings().size() == 1 && settings_.player_settings()[0].num == input_slots::keyboard_and_mouse &&
                settings_.player_team(0) == PlayerTeam::a && settings_.player_weapon(0) == WeaponType::sprayer &&
                settings_.stage() == "crossfeed" && settings_.duration() == 73.0f && settings_.best_of() == 1 &&
                settings_.screen_layout() == ScreenLayout::one_player,
                "Preview backing launched/queued a match or changed the fixed provisional roster/stage/tool plan");
        }
    }
    void open(bool random = false)
    {
        stack_.clear(); context_.cancel_music_exit(); script_menu_music(context_, {});
        stack_.notify_activation(true); app_.keyboard()->set_focused(true); app_.mouse()->set_focused(true);
        for (Key key : { Key::enter, Key::escape, Key::left, Key::right }) { app_.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; poll(); poll(); input_.reset_actions();
        settings_ = MatchSetup(); settings_.set_player_count(1); settings_.set_best_of(1); settings_.set_duration(73.0f);
        settings_.set_stage("crossfeed"); settings_.set_screen_layout(ScreenLayout::one_player);
        settings_.begin_input_claims(input_slots::keyboard_and_mouse); settings_.set_player_team(0, PlayerTeam::a);
        settings_.set_player_weapon(0, WeaponType::sprayer); slot_ = 0; locked_ = false; captured_ = look_.player_identity_background_colour;
        stack_.push(std::make_unique<PauseMenuProbeRoot>(app_, canvas_));
        std::srand(51023); const int next = std::rand(); std::srand(51023);
        stack_.push(std::make_unique<MainMenuStageSelect>(&context_)); ++counts_.opens; opened_ = true;
        require(std::rand() == next, "Preview backing page construction consumed RNG"); tick(); tick();
        if (random) { tap(Key::left); slot_ = 2; } unchanged();
    }
    void tick()
    {
        poll(); std::srand(51029); const int next = std::rand(); std::srand(51029); stack_.update(0.0f); ++counts_.updates;
        require(std::rand() == next, "Preview backing neutral/navigation menu update consumed RNG"); unchanged();
    }
    void browse(bool right)
    {
        tap(right ? Key::right : Key::left);
        if (!locked_) { slot_ = right ? (slot_ == 2 ? 0 : slot_ + 1) : (slot_ == 0 ? 2 : slot_ - 1); }
    }
    void ready(bool value)
    { require(locked_ != value, "Fixture must not send a second fresh locked Proceed"); tap(value ? Key::enter : Key::escape); locked_ = value; }
    void back_to_weapon()
    { require(!locked_, "Back reconstruction needs an unconfirmed page"); tap(Key::escape); }
    void proceed_from_weapon()
    {
        tap(Key::enter); captured_ = look_.player_identity_background_colour; slot_ = 0; locked_ = false;
        ++counts_.reopens; tick(); tick(); unchanged();
    }
    void reopen() { back_to_weapon(); proceed_from_weapon(); }
    Frame repeated(const char* tag)
    {
        const Frame actual = frame(), repeated_frame = frame();
        if (actual.pixels != repeated_frame.pixels) { failure(tag, actual, repeated_frame); throw std::runtime_error("Preview backing repeated complete-page draw changed RGBA"); }
        return actual;
    }
    Frame reference(const Colour& colour, bool locked, PreviewBackingReference mode = PreviewBackingReference::correct)
    {
        return draw(false, [&](DrawList& list)
        {
            rectangle(list, roi(), locked ? Colour(117, 237, 215) : Colour(68, 82, 104));
            const float factor = mode == PreviewBackingReference::straight ? 1.0f :
                mode == PreviewBackingReference::squared ? colour.a * colour.a : colour.a;
            rectangle(list, roi(), Colour(colour.r * factor, colour.g * factor, colour.b * factor, colour.a));
        });
    }
    void match_roi(const char* tag, const Frame& actual, const Frame& reference) const
    {
        try
        { for (int y = 350; y < 382; ++y) { for (int x = 180; x < 244; ++x)
            { const size_t at = offset(x, y); require(same(actual, reference, at) && actual.pixels[at + 3] == 255, "Actual preview backing differs from explicit opaque-underlay/once-tint RGBA reference"); } } }
        catch (...) { failure(tag, actual, reference); throw; }
    }
    void match_page(const char* tag, const Frame& actual, const Frame& expected) const
    { if (actual.pixels != expected.pixels) { failure(tag, actual, expected); throw std::runtime_error("Preview capture/reopen lost exact matching-state full-page RGBA"); } }
    void locality(const Frame& actual, const Frame& baseline, bool changed) const
    {
        size_t differences = 0;
        try
        { for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
            { const bool equal = same(actual, baseline, offset(x, y));
              if (x < 150 || x >= 1160 || y < 330 || y >= 830) { require(equal, "Selected preview tint changed another card, text, edge or UI region"); }
              else if (!equal) { ++differences; } } }
            require(changed ? differences > 0 : differences == 0, "Selected preview backing failed to respond inside its bounded pane"); }
        catch (...) { failure("selected-locality", actual, baseline); throw; }
    }
    size_t negative(const Frame& correct, const Frame& wrong) const
    {
        size_t changed = 0;
        try
        { for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
            { const bool equal = same(correct, wrong, offset(x, y));
              if (x < 180 || x >= 244 || y < 350 || y >= 382) { require(equal, "Preview negative reference changed its exterior"); }
              else if (!equal) { ++changed; } } }
            require(changed > 0 && changed <= 2048, "Preview default/partial/zero negative reference did not discriminate"); }
        catch (...) { failure("negative-reference", correct, wrong); throw; } return changed;
    }
    void close()
    {
        if (closed_) { return; } context_.cancel_music_exit(); script_menu_music(context_, {}); stack_.clear();
        for (Key key : { Key::enter, Key::escape, Key::left, Key::right }) { app_.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = initial_pads_; app_.keyboard()->set_focused(initial_keyboard_.focused);
        app_.mouse()->set_focused(initial_mouse_focus_); stack_.notify_activation(initial_activation_); poll(); poll(); closed_ = true;
        require(app_.keyboard()->state().keys[0] == initial_keyboard_.keys[0] && app_.keyboard()->state().keys[1] == initial_keyboard_.keys[1] &&
            app_.keyboard()->focused() == initial_keyboard_.focused && app_.mouse()->focused() == initial_mouse_focus_ &&
            stack_.depth() == 0 && stack_.active() == initial_activation_, "Preview fixture failed to restore neutral keys/focus/activation/stack");
    }
    static RectangleI roi() { return { 180, 350, 64, 32 }; }
    static Frame compact(const Frame& source) { return foreground_crop(source, roi(), 0); }
private:
    static std::vector<float> fixed_style(const Presentation& presentation)
    {
        auto hud = presentation.hud(); hud.player_identity_background_colour = Colour(18, 24, 39, 238);
        auto v = hud_track_fixed_style(hud); const auto& track = hud.meter_track_colour;
        v.insert(v.end(), { track.r, track.g, track.b, track.a });
        const auto& menu = presentation.menu(); const auto& p = presentation.main_menu().stage_select;
        for (const auto& point : { p.preview_position, p.preview_size, p.name_position, p.navigation_position }) { v.insert(v.end(), { point.x, point.y }); }
        for (const auto& colour : { menu.panel_edge_colour, p.selected, p.selected_ink, p.unselected }) { v.insert(v.end(), { colour.r, colour.g, colour.b, colour.a }); }
        v.push_back(menu.rule_height); return v;
    }
    std::vector<int> preference_values() const
    {
        auto values = limb_preference_values(preferences_.play_settings());
        values.insert(values.end(), { static_cast<int>(preferences_.resolution()), preferences_.fullscreen() ? 1 : 0,
            static_cast<int>(preferences_.team_palette()), static_cast<int>(preferences_.input_bindings().keyboard_jump) }); return values;
    }
    void poll() { app_.gamepads()->poll(); app_.keyboard()->poll(); app_.mouse()->poll(); ++counts_.polls; }
    void tap(Key key) { app_.keyboard()->on_key_down(key); tick(); app_.keyboard()->on_key_up(key); tick(); ++counts_.taps; }
    Frame frame() { return draw(true, [](DrawList&) {}); }
    static size_t offset(int x, int y) { return (static_cast<size_t>(y) * 1920 + static_cast<size_t>(x)) * 4; }
    static bool same(const Frame& a, const Frame& b, size_t at)
    { return std::equal(a.pixels.begin() + at, a.pixels.begin() + at + 4, b.pixels.begin() + at); }
    void rectangle(DrawList& list, const RectangleI& b, const Colour& colour) const
    { list.set_filter(TextureFilter::point); list.draw_sprite(sheet_->texture(), pixel_.source_rectangle(),
        RectangleF(static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.width), static_cast<float>(b.height)),
        colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f); }
    Frame draw(bool actual, const std::function<void(DrawList&)>& raw)
    {
        unchanged(); require(counts_.reads < (phase_ == "shared" ? 68U : 24U), "Preview backing exceeded its finite read budget");
        const Colour before = look_.player_identity_background_colour, held = captured_;
        const auto keys = app_.keyboard()->state(), previous = app_.keyboard()->previous_state();
        std::srand(51031); const int next = std::rand(); std::srand(51031);
        Renderer& renderer = *app_.renderer(); renderer.begin_frame();
        if (actual) { stack_.draw(renderer); } else { renderer.set_view_count(1); }
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        if (!actual) { canvas_.draw(list, { 0, 0, 1920, 1080 }); raw(list); }
        rectangle(list, { 20, 20, 4, 4 }, Colour::white); rectangle(list, { 25, 20, 4, 4 }, Colour::black);
        Frame frame = read(renderer); ++counts_.reads;
        require(frame.width == 1920 && frame.height == 1080 && frame.pixels.size() == 1920 * 1080 * 4, "Preview backing requires an unchanged native 1920 by 1080 RGBA backbuffer");
        for (int y = 20; y < 24; ++y) { for (int x = 20; x < 29; ++x)
        { if (x == 24) { continue; } const size_t at = offset(x, y); const unsigned char c = x < 24 ? 255 : 0;
          require(frame.pixels[at] == c && frame.pixels[at + 1] == c && frame.pixels[at + 2] == c && frame.pixels[at + 3] == 255, "Preview backing WHITE/BLACK RGBA calibration differs"); } }
        require(std::rand() == next && look_.player_identity_background_colour == before && captured_ == held &&
            app_.keyboard()->state().keys[0] == keys.keys[0] && app_.keyboard()->state().keys[1] == keys.keys[1] &&
            app_.keyboard()->previous_state().keys[0] == previous.keys[0] && app_.keyboard()->previous_state().keys[1] == previous.keys[1],
            "Preview draw consumed RNG or changed source/captured colour or keyboard snapshots"); unchanged(); return frame;
    }
    void failure(const char* tag, const Frame& actual, const Frame& expected) const
    {
        const std::string stem = std::string("stage-preview-background-failure-") + phase_ + '-' + tag;
        save(compact(actual), output_ / (stem + "-actual.ppm")); save(compact(expected), output_ / (stem + "-reference.ppm"));
        const auto rgba = [&](const Frame& frame, const char* suffix)
        { std::ofstream file(output_ / (stem + suffix), std::ios::binary); file.write(reinterpret_cast<const char*>(frame.pixels.data()), static_cast<std::streamsize>(frame.pixels.size())); require(file.good(), "Cannot preserve preview failure RGBA"); };
        rgba(actual, "-actual.rgba"); rgba(expected, "-reference.rgba");
        std::ofstream record(output_ / (stem + ".json")); record << "{\"scope\":\"source-hypothesis-failure-no-fitting\",\"roi\":[180,350,64,32],\"reads\":" << counts_.reads << ",\"local_menu_updates\":" << counts_.updates << "}\n";
        require(record.good(), "Cannot preserve preview failure receipt");
    }
    Application& app_; Presentation& presentation_; const HudPresentation& look_; const Canvas& canvas_;
    Save preferences_; StageList stages_; StageCatalog stage_catalog_; GameData data_; MenuInput input_; MatchSetup settings_; MenuContext context_; StateContext& stack_;
    const SpriteSheet* sheet_; const SpriteFrame& pixel_; std::vector<float> fixed_, identity_fixed_; TeamPalettePresentation palette_;
    std::vector<int> preferences_before_; std::array<std::string, 6> font_names_{}; std::array<FontHandle, 6> fonts_{}; std::array<const Font*, 6> font_pointers_{};
    const std::filesystem::path output_; const std::string phase_; KeyboardState initial_keyboard_, initial_previous_; bool initial_mouse_focus_, initial_activation_;
    decltype(painter_scripted_pads::states) initial_pads_; PreviewBackingCounts counts_; Colour captured_; int slot_ = 0; bool locked_ = false, opened_ = false, closed_ = false;
};

struct PreviewBackingEvidence
{
    const std::array<Colour, 4> colours{ Colour(18, 24, 39, 238), Colour(53, 188, 225), Colour(32, 96, 192, 128), Colour(165, 50, 214, 0) };
    std::array<std::array<Frame, 2>, 4> raw;
    std::array<std::array<std::array<Frame, 2>, 2>, 4> pages;
};
PreviewBackingEvidence stage_preview_background_consumer_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, const Presentation& original, const std::filesystem::path& output)
{
    Presentation local = original; PreviewBackingProbe probe(app, levels, canvas, tuning, local, output, "shared");
    PreviewBackingEvidence evidence;
    constexpr std::array<const char*, 4> states{ "default", "rgb", "partial", "zero" };
    constexpr std::array<const char*, 2> contexts{ "crossfeed", "random" }, readiness{ "browse", "locked" };
    std::array<std::array<size_t, 2>, 4> control_counts{}, control_pixels{};
    size_t rows = 0, crops = 0, controls = 0, changed_pixels = 0;
    std::ofstream csv(output / "stage-preview-background-colour-main.csv");
    csv << "state,context,readiness,raw_rgba_match,repeat_match,locality_match,frozen_resources_plan_rng,controls,control_pixels,scope\n";
    for (size_t state = 0; state < 4; ++state)
    {
        for (size_t context = 0; context < 2; ++context)
        {
            probe.set_source(evidence.colours[state]); probe.open(context == 1);
            if (state == 0 && context == 0)
            {
                for (size_t palette = 0; palette < 4; ++palette) { for (size_t locked = 0; locked < 2; ++locked)
                { evidence.raw[palette][locked] = probe.reference(evidence.colours[palette], locked != 0); } }
                for (size_t locked = 0; locked < 2; ++locked)
                {
                    const auto negative = [&](size_t palette, PreviewBackingReference mode)
                    {
                        const Frame wrong = probe.reference(evidence.colours[palette], locked != 0, mode);
                        control_pixels[palette][locked] += probe.negative(evidence.raw[palette][locked], wrong);
                        ++control_counts[palette][locked];
                    };
                    negative(0, PreviewBackingReference::straight); // Old submitted nonblack default RGB.
                    negative(2, PreviewBackingReference::straight); negative(2, PreviewBackingReference::squared);
                    negative(3, PreviewBackingReference::straight);
                }
            }
            for (size_t locked = 0; locked < 2; ++locked)
            {
                if (locked) { probe.ready(true); }
                const Frame frame = probe.repeated("primary"); evidence.pages[state][context][locked] = frame;
                probe.match_roi("primary-raw", frame, evidence.raw[state][locked]);
                probe.locality(frame, evidence.pages[0][context][locked], state != 0);
                const size_t count = context == 0 ? control_counts[state][locked] : 0;
                const size_t pixels = context == 0 ? control_pixels[state][locked] : 0;
                csv << states[state] << ',' << contexts[context] << ',' << readiness[locked]
                    << ",true,true,true,true," << count << ',' << pixels << ",actual-page-backing-underlay-roi\n";
                save(PreviewBackingProbe::compact(frame), output / (std::string("stage-preview-background-colour-") + states[state] + '-' + contexts[context] + '-' + readiness[locked] + ".ppm"));
                ++rows; ++crops; controls += count; changed_pixels += pixels;
            }
        }
        require(evidence.pages[state][0][0].pixels != evidence.pages[state][1][0].pixels &&
            evidence.pages[state][0][0].pixels != evidence.pages[state][0][1].pixels,
            "Actual Crossfeed/Random and browse/locked observations must be distinct pages/states");
    }
    std::ofstream life(output / "stage-preview-background-lifetime.csv");
    life << "state,source_token,captured_token,context,readiness,repeat_match,raw_rgba_match,full_page_guard,frozen_resources_plan_rng,scope\n";
    size_t life_rows = 0;
    const auto retained = [&](const char* state, const char* source, size_t context, size_t locked, size_t palette = 0)
    {
        const Frame frame = probe.repeated(state); probe.match_roi(state, frame, evidence.raw[palette][locked]);
        probe.match_page(state, frame, evidence.pages[palette][context][locked]);
        life << state << ',' << source << ',' << (palette == 0 ? "121827EE" : "2060C080") << ',' << contexts[context] << ',' << readiness[locked]
            << ",true,true," << (palette == 0 ? "exact-captured-authored" : "exact-fresh-partial") << ",true,captured-next-screen-lifetime\n";
        ++life_rows; return frame;
    };
    probe.set_source(evidence.colours[0]); probe.open(); retained("authored-open", "121827EE", 0, 0);
    probe.set_source(evidence.colours[2]); probe.tick(); retained("source-edited-retained", "2060C080", 0, 0);
    probe.browse(false); retained("cycled-random-retained", "2060C080", 1, 0);
    probe.ready(true); retained("random-locked-retained", "2060C080", 1, 1);
    probe.browse(true); retained("locked-arrow-ignored", "2060C080", 1, 1);
    probe.ready(false); retained("random-unlocked-retained", "2060C080", 1, 0);
    probe.browse(true); retained("cycled-crossfeed-retained", "2060C080", 0, 0);
    probe.ready(true); retained("crossfeed-locked-retained", "2060C080", 0, 1);
    probe.ready(false); probe.back_to_weapon();
    const Frame weapon = probe.repeated("back-to-weapon");
    require(weapon.pixels != evidence.pages[0][0][0].pixels, "Actual Back did not visibly leave the Stage page");
    life << "back-to-weapon,2060C080,not-stage-page,weapon,not-stage-page,true,not-run-weapon-transition,page-changed,true,captured-next-screen-lifetime\n"; ++life_rows;
    probe.proceed_from_weapon(); retained("weapon-proceed-reopened", "2060C080", 0, 0, 2);
    probe.close(); const auto counts = probe.counts();
    require(csv.good() && life.good() && rows == 16 && life_rows == 10 && crops == 16 && controls == 8 &&
        changed_pixels > 0 && changed_pixels <= 16384 && counts.reads == 68 && counts.updates == 63 &&
        counts.polls == 83 && counts.taps == 21 && counts.opens == 9 && counts.reopens == 1,
        "Preview backing shared finite read/row/crop/update/poll/lifetime budget differs");
    std::ofstream summary(output / "stage-preview-background-colour-consumer-summary.json");
    summary << "{\"scope\":\"captured-preview-backing-two-underlays\",\"readbacks\":68,\"csv_rows\":26,\"main_rows\":16,\"lifetime_rows\":10,\"crops\":16,\"crop_width\":64,\"crop_height\":32,"
        "\"crop_pixels\":32768,\"ppm_bytes\":98512,\"roi_pixels\":2048,\"roi_observations\":25,\"roi_pixel_checks\":51200,\"repeat_checks\":26,\"raw_references\":8,\"controls\":8,"
        "\"control_discriminating_pixels\":" << changed_pixels << ",\"full_page_lifetime_checks\":9,\"weapon_transition_nonoracle_checks\":1,\"explicit_stage_pushes\":9,\"back_weapon_proceed_routes\":1,"
        "\"tap_gestures\":21,\"local_menu_updates\":63,\"gamepad_polls\":83,\"keyboard_polls\":83,\"mouse_polls\":83,\"input_polls\":249,\"calibration_pixel_checks\":2176,"
        "\"positive_time_updates\":0,\"level_updates\":0,\"player_updates\":0,\"scene_updates\":0,\"gameplay_weapon_updates\":0,\"private_game_saves\":0}\n";
    require(summary.good(), "Cannot retain preview backing shared summary");
    std::cout << "PASS Stage preview backing: 68 reads, 26 rows, 16 native 64 by 32 crops; actual Random/Crossfeed browse/locked composites, "
        "eight default/partial/zero controls, captured source edits and real Back/Weapon/Proceed reopen. 63 zero-dt menu updates and 249 explicit device polls.\n";
    return evidence;
}

#if COLOURWARS_DEV_TOOLS
void stage_preview_background_command_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output,
    const PreviewBackingEvidence& shared)
{
    const auto root = output / "stage-preview-background-tune-scratch", source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "Preview backing private scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> bytes;
    for (size_t i = 0; i < files.size(); ++i)
    { bytes[i] = tune_colour_bytes(content / files[i]); tune_colour_write(source / files[i], bytes[i]); tune_colour_write(staged / files[i], bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestorePreview { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestorePreview() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
    const std::string path = "hud.player_identity.background_colour", full = "presentation." + path;
    JsonSpanDocument document(bytes[1], "Preview backing private source"); const auto* token = document.find(path);
    require(token && document.token(*token) == "\"121827EE\"", "Preview authored exact RGBA token must stay 121827EE");
    const std::string saved_bytes = document.replaced(*token, "\"2060C080\""); bool persisted = false;
    DebugDirector director(&tuning, &presentation); auto& impl = *director.impl();
    impl.set_content_directories(source, staged); impl.set_user_directory(root / "user"); impl.set_script_directories({ root / "scripts" });
    PreviewBackingProbe probe(app, levels, canvas, tuning, presentation, output, "debug", &director); director.attach(&app, &probe.data());
    impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "stage-preview-background-tune-commands.log").string()), "Cannot retain preview command transcript");
    size_t commands = 0, rows = 0;
    const auto unchanged = [&]()
    {
        probe.unchanged(); require(!impl.in_match() && !impl.can_apply() && impl.request().empty() &&
            impl.total_updates() == 0 && impl.level_updates() == 0 && !std::filesystem::exists(root / "profile/ColourWars/save_data.json"),
            "Preview commands launched/queued gameplay, wrote a game Save or advanced match hooks");
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto& wanted = i == 1 && persisted ? saved_bytes : bytes[i];
            require(tune_colour_bytes(content / files[i]) == bytes[i] && tune_colour_bytes(source / files[i]) == wanted &&
                tune_colour_bytes(staged / files[i]) == wanted, "Preview commands changed bytes beyond the one exact private saved token");
        }
    };
    const auto command = [&](const std::string& text, bool query = false)
    {
        const size_t before = impl.log().lines().size(); std::srand(51037); const int next = std::rand(); std::srand(51037);
        require(impl.execute(text).ok && std::rand() == next, "Preview real tune/query/reset/save failed or consumed RNG"); ++commands;
        if (text.starts_with("tune "))
        {
            const auto& log = impl.log().lines(); require(log.size() > before && log.back().severity == DebugSeverity::info &&
                (query ? log.back().text.find("[next screen]") != std::string::npos :
                    log.back().text.ends_with("(applies when the screen is next opened)")),
                "Preview tune/query lacks a fresh next-screen reply");
        }
    };
    std::ofstream csv(output / "stage-preview-background-tune.csv");
    csv << "state,source_token,captured_token,context,document_revision,ledger_entries,repeat_match,raw_rgba_match,full_page_match,private_bytes,frozen_resources_plan_rng,scope\n";
    const auto observe = [&](const char* state, const char* word, size_t captured, size_t context = 0)
    {
        unchanged(); const auto* current = impl.tune_document("presentation"); require(current != nullptr, "Preview tune document disappeared");
        const auto* leaf = current->current().find(path); require(leaf && current->current().token(*leaf) == std::string("\"") + word + '"', "Preview exact quoted source token differs");
        const std::string quoted = std::string("\"") + word + '"';
        const std::string baseline_word = persisted ? "\"2060C080\"" : "\"121827EE\"";
        require(current->baseline().text() == (persisted ? saved_bytes : bytes[1]) &&
            current->current().text() == document.replaced(*token, quoted), "Preview full current/baseline document changed beyond the selected token");
        const bool edited = quoted != baseline_word;
        require(current->edits().size() == (edited ? 1U : 0U) && (!edited ||
            (current->edits().contains(path) && current->edits().at(path).baseline_token == baseline_word &&
                current->edits().at(path).current_token == quoted)), "Preview exact baseline/current ledger token pair differs");
        const auto* tuning_document = impl.tune_document("tuning");
        require(tuning_document && tuning_document->baseline().text() == bytes[0] && tuning_document->current().text() == bytes[0] &&
            tuning_document->edits().empty(), "Preview command changed the tuning document");
        const int revision = current->revision(); const size_t ledger_entries = current->edits().size();
        const Colour wanted = std::string_view(word) == "121827EE" ? shared.colours[0] : std::string_view(word) == "35BCE1" ? shared.colours[1] : shared.colours[2];
        require(presentation.hud().player_identity_background_colour == wanted, "Preview source channels differ from the actual command");
        const Frame frame = probe.repeated(state); probe.match_roi(state, frame, shared.raw[captured][0]);
        probe.match_page(state, frame, shared.pages[captured][context][0]);
        require(current->revision() == revision && current->edits().size() == ledger_entries, "Preview drawing changed document revision/ledger");
        csv << state << ',' << word << ',' << (captured == 0 ? "121827EE" : "2060C080") << ',' << (context ? "random" : "crossfeed") << ',' << revision << ',' << ledger_entries
            << ",true,true,true,true,true,debug-captured-next-screen-preview\n"; ++rows; unchanged();
    };
    probe.open(); observe("baseline", "121827EE", 0);
    command("tune " + full, true); command("tune " + full + " 2060C080"); probe.tick(); observe("edit-retained", "2060C080", 0);
    probe.browse(false); observe("cycle-retained", "2060C080", 0, 1);
    probe.reopen(); observe("edit-reopened", "2060C080", 2);
    command("tune.reset " + full); probe.tick(); observe("reset-original-retained", "121827EE", 2);
    probe.reopen(); observe("reset-original-reopened", "121827EE", 0);
    command("tune " + full + " 2060C080"); probe.tick(); observe("save-pending-retained", "2060C080", 0);
    require(impl.tune_document("presentation")->edits().size() == 1 && impl.tune_document("presentation")->edits().at(path).current_token == "\"2060C080\"",
        "Preview pending exact token ledger differs");
    command("tune.save presentation"); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "Preview one-token save failed to rebase");
    probe.tick(); observe("saved-retained", "2060C080", 0);
    probe.reopen(); observe("saved-reopened", "2060C080", 2);
    command("tune " + full + " 35BCE1"); probe.tick(); observe("post-save-edit-retained", "35BCE1", 2);
    command("tune.reset " + full); probe.tick(); observe("reset-saved-retained", "2060C080", 2);
    probe.reopen(); observe("reset-saved-reopened", "2060C080", 2);
    probe.close(); unchanged(); const auto counts = probe.counts();
    require(csv.good() && commands == 7 && rows == 12 && counts.reads == 24 && counts.updates == 34 && counts.polls == 38 &&
        counts.taps == 9 && counts.opens == 1 && counts.reopens == 4 && impl.tune_document("presentation")->edits().empty() &&
        impl.tune_document("tuning")->edits().empty(), "Preview Debug finite command/read/row/update/poll/reopen budget or final ledger differs");
    std::ofstream summary(output / "stage-preview-background-tune-summary.json");
    summary << "{\"scope\":\"debug-captured-next-screen-preview\",\"commands\":7,\"query_commands\":1,\"tune_commands\":3,\"reset_commands\":2,\"presentation_save_commands\":1,\"saved_tokens\":1,"
        "\"states\":12,\"readbacks\":24,\"csv_rows\":12,\"crops\":0,\"roi_observations\":12,\"roi_pixel_checks\":24576,\"repeat_checks\":12,\"explicit_stage_pushes\":1,\"back_weapon_proceed_routes\":4,"
        "\"tap_gestures\":9,\"local_menu_updates\":34,\"gamepad_polls\":38,\"keyboard_polls\":38,\"mouse_polls\":38,\"input_polls\":114,\"calibration_pixel_checks\":768,"
        "\"positive_time_updates\":0,\"level_updates\":0,\"player_updates\":0,\"scene_updates\":0,\"gameplay_weapon_updates\":0,\"private_game_saves\":0}\n";
    require(summary.good(), "Cannot retain preview backing Debug summary");
    std::cout << "PASS Stage preview next-screen commands: 7 statements, 12 retained/fresh observations, 24 additional reads, one exact private token saved/rebased/reset. "
        "34 zero-dt menu updates and 114 explicit device polls; no gameplay simulation or game Save.\n";
}
#endif
