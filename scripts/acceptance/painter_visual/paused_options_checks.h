// Settled independent Options frame. Call after motion has settled, or while
// Reduce effects suppresses it. This uses the independently implemented
// setup_paper_reference; it never draws production menu/paper/icon widgets.
// canvas_underlay=false matches a paused Options page with covers_screen()==true:
// begin_frame's clear remains in aspect-ratio margins beneath ONE menu scrim.
// true supplies the ordinary standalone probe Canvas before that same scrim.
Frame paused_options_reference(Application& app, const Canvas& canvas,
    const Presentation& captured, const MenuText& words, int selected,
    const std::array<std::wstring, 3>& values, bool resolution_disabled,
    const std::wstring& status_copy, bool status_warning,
    const std::wstring& expected_caption, const std::string& caption_font,
    const Vector2F& caption_position, bool canvas_underlay = false)
{
    require(selected >= 0 && selected < 7 && !(resolution_disabled && selected == 0),
        "Independent paused Options requires an enabled selected row");
    const auto& style = captured.menu(); const auto& look = captured.main_menu();
    auto art = look.art; art.focus_padding = look.setup_focus_padding;
    art.shadow_offset = look.setup_shadow_offset;
    constexpr std::array<MenuItem, 7> roles{ MenuItem::resolution, MenuItem::fullscreen,
        MenuItem::team_palette, MenuItem::controls_audio, MenuItem::restore_defaults,
        MenuItem::apply, MenuItem::back };
    constexpr std::array<const wchar_t*, 7> labels{ L"Resolution", L"Fullscreen",
        L"Team palette", L"Controls / audio", L"Restore defaults", L"Apply", L"Back" };
    require(words.heading(MenuHeading::options) == L"Options" &&
        !expected_caption.empty() && words.options_palette_timing == expected_caption,
        "Paused Options heading/caption drifted from the independent fixture copy");
    for (size_t index = 0; index < roles.size(); ++index)
    {
        require(words.item(roles[index]) == labels[index],
            "Paused Options action drifted from its independent literal");
    }

    const RectangleF design(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION);
    const RectangleF paper((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) -
        look.options_box_size) / 2.0f, look.options_box_size);
    const float row_height = look.options_spacing.y - art.focus_padding.y * 2.0f;
    const float value_width = paper.right() - art.shadow_offset - art.focus_padding.x - look.options_value_x;
    require(row_height > 0.0f && value_width > 0.0f,
        "Independent paused Options has no fitted value budget");
    std::vector<std::unique_ptr<UiText>> text;
    std::array<RectangleF, 7> bounds;
    const auto add = [&](const std::wstring& copy, const std::string& font,
        const Vector2F& position, const Colour& colour)
    {
        auto widget = std::make_unique<UiText>("independent-paused-options", copy, font,
            position, app.render_resources(), colour);
        auto* result = widget.get(); text.push_back(std::move(widget)); return result;
    };
    const auto* heading = add(L"Options", style.heading_font, look.options_heading_position, art.ink);
    const auto* caption = add(expected_caption, caption_font, caption_position, art.ink);
    require(paper.contains(heading->bounds()) && paper.contains(caption->bounds()) &&
        heading->bounds().right() <= caption->bounds().x,
        "Independent paused Options heading and palette timing caption overlap or escape paper");
    for (int index = 0; index < 7; ++index)
    {
        const Vector2F position = look.options_position +
            Vector2F(0.0f, look.options_spacing.y * static_cast<float>(index));
        auto* label = add(labels[static_cast<size_t>(index)], style.item_font, position,
            index == 0 && resolution_disabled ? art.disabled_ink :
                index == selected ? art.focus_ink : art.ink);
        if (index < 3)
        {
            const float label_width = look.options_value_x - position.x - art.focus_padding.x;
            require(label_width > 0.0f, "Independent paused Options has no label budget");
            const RectangleF raw_label = label->bounds();
            label->set_scale(std::min({ 1.0f, label_width / raw_label.width,
                row_height / raw_label.height }));
            auto* value = add(values[static_cast<size_t>(index)], style.item_font,
                Vector2F(look.options_value_x, position.y), art.ink);
            const RectangleF raw_value = value->bounds();
            require(raw_value.width > 0.0f && raw_value.height > 0.0f,
                "Independent paused Options needs a nonempty value");
            value->set_scale(std::min({ 1.0f, value_width / raw_value.width,
                row_height / raw_value.height }));
            bounds[static_cast<size_t>(index)] = RectangleF::union_of(label->bounds(), value->bounds());
        }
        else { bounds[static_cast<size_t>(index)] = label->bounds(); }
        RectangleF padded = bounds[static_cast<size_t>(index)]; padded.inflate(art.focus_padding);
        require(paper.contains(padded) &&
            (index == 0 || padded.y >= bounds[static_cast<size_t>(index - 1)].bottom() + art.focus_padding.y),
            "Independent paused Options row/card escapes paper or reaches the preceding row");
        if (index == 0)
        {
            require(heading->bounds().bottom() <= padded.y && caption->bounds().bottom() <= padded.y,
                "Independent paused Options heading/caption reaches its first card");
        }
    }
    auto* status = add(status_copy, captured.prompt_bar().font, look.options_status_position,
        status_warning ? look.options_warning_colour : art.focus_ink);
    const float status_width = paper.right() - art.shadow_offset - art.focus_padding.x -
        look.options_status_position.x;
    if (!status_copy.empty())
    {
        require(status_width > 0.0f, "Independent paused Options has no status width");
        if (status->bounds().width > status_width) { status->set_scale(status_width / status->bounds().width); }
        RectangleF back = bounds.back(); back.inflate(art.focus_padding);
        require(paper.contains(status->bounds()) && !back.intersects(status->bounds()),
            "Independent paused Options status escapes paper or overlaps Back");
    }

    Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
    const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
    const Camera camera = Camera::frame(design, viewport);
    DrawList list = renderer.view(0); list.set_viewport(viewport);
    list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
    if (canvas_underlay)
    {
        canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
    }
    list.set_camera(camera);
    const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
    const auto texel = sheet->resolve_sprite_frame(style.fill_frame);
    sheet->draw(list, texel, camera.visible_rectangle(viewport), style.scrim_colour);
    sheet->draw(list, texel, design, look.home_background_colour);
    setup_paper_reference(list, *app.render_resources(), style, art, paper,
        bounds[static_cast<size_t>(selected)]);
    sheet->draw(list, texel, RectangleF(0.0f, style.footer_y, menu_consts::DESIGN_RESOLUTION.x,
        menu_consts::DESIGN_RESOLUTION.y - style.footer_y), style.scrim_colour);
    sheet->draw(list, texel, RectangleF(style.brand_position.x, style.footer_y,
        menu_consts::DESIGN_RESOLUTION.x - style.brand_position.x * 2.0f, style.rule_height),
        style.panel_edge_colour);
    list.set_filter(TextureFilter::linear);
    for (const auto& widget : text) { widget->draw(list); }
    const auto& footer = captured.prompt_bar();
    const colourwars::UiTextDropShadow prompt("independent-paused-options-prompt",
        L"Left/Right Change    A/Enter Select    B/Esc Back", footer.font, footer.position,
        app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
    prompt.draw(list);
    return read(renderer);
}

// Expected ordinary paused call (use the captured MenuText, not the live source):
// paused_options_reference(app, canvas, captured, captured_words, row, values,
//     disabled_resolution, independent_status, warning, literal_caption,
//     captured.menu().detail_font,
//     Vector2F(captured.main_menu().options_value_x,
//         captured.main_menu().options_heading_position.y), false);

// Actual paused GameLevel owns the route; all writes and display changes stay
// inside this probe's nested profile and hidden, windowed shell.
void paused_options_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const Presentation original = presentation;
    struct Profile
    {
        std::wstring previous;
        explicit Profile(const std::filesystem::path& path)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0, "Cannot read paused Options profile");
            if (raw) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(path);
            require(_wputenv_s(L"LOCALAPPDATA", path.c_str()) == 0, "Cannot isolate paused Options saves");
        }
        ~Profile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } profile(output / "paused-options-profile");
    const auto save_path = output / "paused-options-profile" / "ColourWars" / "save_data.json";
    const std::filesystem::path blocked(save_path.wstring() + L".tmp");
    const HWND window = static_cast<HWND>(app.window());
    const SettingsProbeActivationGuard activation(window);
    Save preferences; PlaySettings expected; expected.aim_sensitivity = 5;
    expected.music_volume = expected.sfx_volume = 0; expected.view_shake = expected.aim_marker = false;
    for (size_t slot = 0; slot < expected.sticks.size(); ++slot)
    {
        expected.sticks[slot].movement_deadzone = 9 + static_cast<int>(slot) * 11;
        expected.sticks[slot].aim_deadzone = 21 + static_cast<int>(slot) * 13;
    }
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true } };
    GameData data; data.set_application(&app); data.set_save(&preferences);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "movement_course" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    StateContext& stack = app;
    struct Cleanup
    {
        StateContext& stack; Presentation& presentation; Presentation original; std::filesystem::path blocked;
        ~Cleanup() { this->stack.clear(); this->presentation = this->original; painter_scripted_pads::states = {};
            std::error_code ignored; std::filesystem::remove(this->blocked, ignored); }
    } cleanup{ stack, presentation, original, blocked };
    const int keyboard = input_slots::keyboard_and_mouse;
    int owner = keyboard, device = keyboard, row = 0, players = 1;
    GameLevel* game = nullptr; const Level* level_identity = nullptr;
    std::vector<float> frozen; Frame world;
#if COLOURWARS_DEV_TOOLS
    MatchFingerprint fingerprint;
#endif
    std::ofstream worlds(output / "paused-options-world.csv");
    worlds << "check,players,level_identity,observation,level_hud_pixels,roster,layout,resume_zero\n";
    int world_checks = 0; MatchSetup plan;
    Presentation captured = original;
    std::wstring resolution_value = L"1280x720", palette_value = L"A: Blue / B: Yellow", status;
    bool warning = false;
    const float settled = std::max(original.menu().motion.focus_duration, original.menu().motion.press_duration) + 0.01f;
    const auto bytes = [&] { std::ifstream stream(save_path, std::ios::binary); require(stream.good(), "Lost paused Options save");
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()); };
    const auto preserved = [&]
    {
        const auto& p = preferences.play_settings();
        require(p.aim_sensitivity == expected.aim_sensitivity && p.music_volume == expected.music_volume &&
            p.sfx_volume == expected.sfx_volume && p.view_shake == expected.view_shake && p.aim_marker == expected.aim_marker &&
            p.reduce_effects == expected.reduce_effects, "Paused Options replaced shared play preferences");
        for (size_t i = 0; i < p.sticks.size(); ++i)
        { require(p.sticks[i].movement_deadzone == expected.sticks[i].movement_deadzone &&
            p.sticks[i].aim_deadzone == expected.sticks[i].aim_deadzone, "Paused Options replaced a pad profile"); }
    };
    const auto hide = [&] { require(!app.fullscreen() && IsWindowVisible(window) == FALSE && GetForegroundWindow() != window,
            "Paused Options showed, activated or fullscreened its private window"); };
    const auto resize = [&](Vector2I size) { app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y); };
    const auto tick = [&](float elapsed = 0.0f) { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](int slot, GamepadButton button, bool down)
    {
        if (slot == keyboard)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b || button == GamepadButton::start ? Key::escape :
                button == GamepadButton::dpad_up ? Key::up : button == GamepadButton::dpad_down ? Key::down :
                button == GamepadButton::dpad_left ? Key::left : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else
        {
            const auto bit = static_cast<uint16_t>(1u << static_cast<unsigned int>(button));
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit) : static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit));
        }
    };
    const auto tap_slot = [&](int slot, GamepadButton button) { hold(slot, button, true); tick(); hold(slot, button, false); tick(); tick(settled); };
    const auto tap = [&](GamepadButton button) { tap_slot(device, button); };
    const auto go = [&](int wanted) { while (row != wanted) { const bool down = row < wanted;
        tap(down ? GamepadButton::dpad_down : GamepadButton::dpad_up); row += down ? 1 : -1; } };
    const auto check_world = [&]
    {
        resize({ 1280, 720 }); const Frame actual = render(game->level(), *app.renderer());
        require(&game->level() == level_identity && game->resume_remaining() == 0.0f &&
            app.viewport_manager()->layout() == plan.screen_layout() && frozen == resume_observation(game->level()) &&
            actual.pixels == world.pixels, "Paused Options advanced or replaced the retained match, paint, players, timer HUD or layout");
        for (int view = 0; view < players; ++view) { require(game->level().player(view).player_num() == plan.player_settings()[static_cast<size_t>(view)].num &&
            game->level().player(view).team() == plan.player_team(view), "Paused Options changed the sparse match roster"); }
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(game->level(), 0)).empty(), "Paused Options changed match fingerprint");
#endif
        worlds << ++world_checks << ',' << players << ",true,true,true,true,true,true\n";
        preserved(); hide();
    };
    const auto open_options = [&] { tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_down); tap(GamepadButton::a); tick();
        require(stack.depth() == 4, "Pause row2 did not push its owned Options child"); row = 0;
        status.clear(); warning = false; captured = presentation; resolution_value = L"1280x720"; palette_value = L"A: Blue / B: Yellow"; };
    const auto open_match = [&](int count)
    {
        stack.clear(); presentation = original; players = count; owner = device = count == 1 ? keyboard : 3;
        resize({ 1280, 720 }); app.set_fullscreen(false); hide();
        const_cast<PlaySettings&>(preferences.play_settings()) = expected;
        require(preferences.set_display_settings_and_save(ScreenResolution::s_1280_720, false, TeamColourPair::blue_yellow), "Cannot seed paused Options profile");
        for (Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::escape, Key::space }) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {};
        for (int slot : { 3, 0, 2, 1 }) { painter_scripted_pads::states[static_cast<size_t>(slot)].connected = count > 1; }
        tick(); tick(); stack.push(std::make_unique<ResumeProbeRoot>());
        plan = MatchSetup{}; plan.set_stage("close_quarters"); plan.set_duration(120.0f); plan.set_player_count(count);
        plan.begin_input_claims(owner); if (count > 1) { require(plan.claim_input_slot(0), "Cannot claim joined pad0"); }
        if (count == 4) { require(plan.claim_input_slot(2) && plan.claim_input_slot(1), "Cannot claim sparse four-pad roster"); }
        plan.set_screen_layout(count == 1 ? ScreenLayout::one_player : count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        for (int view = 0; view < count; ++view) { plan.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b); plan.set_player_weapon(view, WeaponType::sprayer); }
        auto page = std::make_unique<GameLevel>(&data, plan); game = page.get();
        stack.push<EndMenuAction>(std::move(page), [](const EndMenuAction&) { throw std::runtime_error("Paused Options unexpectedly left the match"); });
        tick(); tick(); if (game->level().intro_playing()) { tap(GamepadButton::a); }
        for (int i = 0; i < 2400 && game->level().state() != LevelState::active; ++i) { tick(dt); }
        require(stack.depth() == 2 && game->level().state() == LevelState::active, "Actual paused Options match did not become active");
        // Close Quarters spawn alcoves are deliberately not scoreable. Place
        // the real participant above lower_paint, settle, then fire toward it.
        require(const_cast<Level&>(game->level()).place_player(0, Vector2F(1800.0f, 4750.0f)), "Cannot place painted-match fixture above lower_paint");
        for (int i = 0; i < 60; ++i) { tick(dt); }
        if (owner == keyboard) { app.mouse()->on_move({ 1200, 700 }); app.mouse()->on_button_down(MouseButton::left); }
        else { auto& pad = painter_scripted_pads::states[static_cast<size_t>(owner)]; pad.right_stick = Vector2F(1.0f, 0.5f); pad.right_trigger = 1.0f; }
        for (int i = 0; i < 90; ++i) { tick(dt); }
        app.mouse()->on_button_up(MouseButton::left);
        if (owner != keyboard) { painter_scripted_pads::states[static_cast<size_t>(owner)].right_trigger = 0.0f; painter_scripted_pads::states[static_cast<size_t>(owner)].right_stick = Vector2F::ZERO; }
        tick(); tap(GamepadButton::start); require(stack.depth() == 3, "Participant did not open actual Pause"); tick(); open_options();
        level_identity = &game->level(); world = render(game->level(), *app.renderer()); frozen = resume_observation(game->level());
#if COLOURWARS_DEV_TOOLS
        fingerprint = MatchFingerprint::of(game->level(), 0);
#endif
        require(game->level().level_end_info().team_a_area > 0.0f, "Paused Options fixture needs actual preexisting painted surface");
    };
    std::ofstream records(output / "paused-options.csv");
    records << "case,width,height,players,owner,input_slot,row,depth,full_frame_matches,frozen_draw_rng,retained_level\n";
    const auto compare = [&](const std::string& name, bool capture = false)
    {
        require(stack.depth() == 4 && &game->level() == level_identity && game->resume_remaining() == 0.0f && app.viewport_manager()->layout() == plan.screen_layout(), "Paused Options reference lost its retained match or stack depth"); preserved();
        const Frame actual = resume_stack_frame(app);
        const Frame reference = paused_options_reference(app, canvas, captured, captured.menu_text(), row,
            { resolution_value, L"Off", palette_value }, false, status, warning,
            captured.menu_text().options_palette_timing, captured.menu().detail_font,
            Vector2F(captured.main_menu().options_value_x, captured.main_menu().options_heading_position.y), false);
        const std::string stem = "paused-options-" + name + "-" + std::to_string(players) + "p-" + std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        if (actual.pixels != reference.pixels) { save(actual, output / (stem + "-failure-actual.ppm")); save(reference, output / (stem + "-failure-expected.ppm"));
            const auto delta = foreground_delta_bounds(actual, reference); std::cerr << "PAUSED OPTIONS " << name << " row=" << row << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' << delta.height << '\n'; }
        require(actual.pixels == reference.pixels, "Paused Options differs from independent complete paper/caption/seven-row reference");
        std::srand(9137); const int next = std::rand(); std::srand(9137);
        require(actual.pixels == resume_stack_frame(app).pixels && actual.pixels == resume_stack_frame(app).pixels && std::rand() == next,
            "Frozen paused Options changed pixels or consumed draw RNG");
        if (capture) { save(actual, output / (stem + ".ppm")); const auto& look = captured.main_menu();
            const RectangleF paper((Vector2F(1920.0f, captured.menu().footer_y) - look.options_box_size) / 2.0f, look.options_box_size);
            const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
            save(foreground_crop(actual, countdown_crop_bounds(camera.calculate_view_rectangle(paper)), 0), output / (stem + "-detail.ppm")); }
        records << name << ',' << actual.width << ',' << actual.height << ',' << players << ',' << owner << ',' << device << ',' << row << ',' << stack.depth() << ",true,true,true\n";
        return actual;
    };
    // The one-player live fixture supplies the six-aspect complete raster matrix.
    open_match(1);
    require(original.menu_text().options_palette_timing == L"Palette applies next round or restart.", "Paused palette timing copy drifted");
    for (Vector2I size : { Vector2I(1280,720), Vector2I(1920,1080), Vector2I(2560,1440), Vector2I(1600,1200), Vector2I(1920,1200), Vector2I(2560,1080) })
    {
        resize(size); for (int focus = 0; focus < 7; ++focus) { go(focus); compare("focus" + std::to_string(focus), focus == 2); }
        go(0); tick(0.0f); tick(-0.5f); compare("nonpositive");
    }
    check_world();
    // An authored focus response moves artwork while text remains fixed; reset
    // and immediate applied reduction restore the independent resting frame.
    go(0); hold(device, GamepadButton::dpad_down, true); tick(); row = 1;
    const Frame moving = resume_stack_frame(app); hold(device, GamepadButton::dpad_down, false); tick();
    const Frame moving_again = resume_stack_frame(app); require(moving.pixels == moving_again.pixels, "Draw advanced paused Options focus motion");
    expected.reduce_effects = true; const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = true;
    compare("immediate-reduction"); expected.reduce_effects = false; const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = false;
    tick(settled); const Frame resting = compare("motion-settled");
    require(moving.pixels != resting.pixels, "Paused Options focus had no authored motion response");
    const auto delta = foreground_delta_bounds(moving, resting);
    const Camera motion_camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
    const RectangleF motion_paper((Vector2F(1920.0f, original.menu().footer_y) - original.main_menu().options_box_size) / 2.0f, original.main_menu().options_box_size);
    require(countdown_crop_bounds(motion_camera.calculate_view_rectangle(motion_paper)).contains(delta), "Paused Options motion escaped its paper");
    go(3); hold(device, GamepadButton::a, true); tick();
    static_cast<WindowNotify&>(app).on_deactivated(); tick();
    static_cast<WindowNotify&>(app).on_activated(); tick();
    hold(device, GamepadButton::a, false); tick(); tick();
    require(stack.depth() == 4, "Interrupted held Enter entered paused Controls"); compare("held-entry-interrupted");
    go(0);
    // Text/font/layout are captured by this actual child; reopening adopts them.
    const_cast<MenuText&>(presentation.menu_text()).options_palette_timing = L"Palette updates next build.";
    const_cast<MenuStyle&>(presentation.menu()).detail_font = original.prompt_bar().font;
    const_cast<MainMenuPresentation&>(presentation.main_menu()).options_heading_position += Vector2F(12.0f, 4.0f);
    const_cast<MainMenuPresentation&>(presentation.main_menu()).options_value_x += 12.0f;
    compare("captured-caption-font", true); tap(GamepadButton::b); require(stack.depth() == 3, "Options Back closed Pause");
    // Pause retains focus row2 when its child returns.
    tap(GamepadButton::a); row = 0; captured = presentation; tick(); compare("reopened-caption-font", true);
    tap(GamepadButton::b); presentation = original; tap(GamepadButton::a); row = 0; captured = original; tick(); compare("restored-caption"); check_world();
    // Staged defaults can be discarded by either own Back route.
    go(4); tap(GamepadButton::a); resolution_value = L"1920x1080"; palette_value = L"Random"; status = original.menu_text().status.options_pending;
    const auto before_discard = bytes(); compare("restore-staged", true); go(6); tap(GamepadButton::a);
    require(stack.depth() == 3 && bytes() == before_discard, "Paused Options Back saved or replaced Pause"); tap(GamepadButton::a); row = 0; status.clear(); resolution_value = L"1280x720"; palette_value = L"A: Blue / B: Yellow"; tick(); compare("back-discard");
    go(4); tap(GamepadButton::a); tap(GamepadButton::b); require(stack.depth() == 3 && bytes() == before_discard, "Paused Options Escape saved or resumed");
    tap(GamepadButton::a); row = 0; tick(); compare("escape-discard"); check_world();
    // A palette-only Apply saves now but preserves this built arena and paint.
    go(2); tap(GamepadButton::dpad_left); palette_value = L"A: Pink / B: Green"; status = original.menu_text().status.options_pending;
    compare("palette-pending"); go(5); tap(GamepadButton::a); status = original.menu_text().status.options_saved;
    compare("palette-saved", true); const auto new_palette = preferences.team_palette();
    require(new_palette != TeamColourPair::blue_yellow && stack.depth() == 4, "Palette-only Apply incorrectly opened a display trial"); check_world();
    tap(GamepadButton::b); tap(GamepadButton::dpad_up); tap(GamepadButton::a); require(stack.depth() == 3, "Restart did not retain typed Pause frame");
    tap(GamepadButton::dpad_up); tap(GamepadButton::a); require(stack.depth() == 2 && game->resume_remaining() == 0.0f, "Deliberate Restart did not rebuild");
    require(game->level().player(0).team_colour() == TeamColourTools::team_colours(new_palette, presentation.team_palette()).team_a, "Restart ignored newly saved palette");
    for (int count : { 2, 4 })
    {
        open_match(count); compare("sparse-owner", true);
        const Frame exclusive = resume_stack_frame(app); tap_slot(keyboard, GamepadButton::dpad_down); tap_slot(1, GamepadButton::b);
        require(stack.depth() == 4 && exclusive.pixels == resume_stack_frame(app).pixels, "A nonowner changed connected-owner Options");
        hold(owner, GamepadButton::dpad_down, true); hold(keyboard, GamepadButton::b, true); tick();
        hold(owner, GamepadButton::dpad_down, false); hold(keyboard, GamepadButton::b, false); tick(); tick(settled); row = 1;
        compare("owner-spectator-simultaneous"); go(0);
        if (count == 2)
        {
            painter_scripted_pads::states[3].connected = false; tick(); device = 0; tap(GamepadButton::dpad_down); row = 1; compare("joined-fallback");
            painter_scripted_pads::states[0].connected = false; tick(); device = keyboard; tap(GamepadButton::dpad_down); row = 2; compare("keyboard-recovery");
            go(3); hold(3, GamepadButton::a, true); painter_scripted_pads::states[3].connected = true; tick(); tick(); hold(3, GamepadButton::a, false); tick(); tick();
            painter_scripted_pads::states[0].connected = true; tick(); device = 3;
            compare("held-owner-reconnect"); const Frame restored = resume_stack_frame(app); tap_slot(keyboard, GamepadButton::dpad_down);
            require(restored.pixels == resume_stack_frame(app).pixels, "Owner reconnection failed to restore exclusivity");
        }
        // Own Controls and Pad Sticks keep the same input policy and merge saves.
        go(3); const Frame options = compare("before-controls"); tap(GamepadButton::a); require(stack.depth() == 5, "Paused Options did not own Controls");
        tick(); const Frame controls = resume_stack_frame(app); tap_slot(keyboard, GamepadButton::b);
        require(stack.depth() == 5 && controls.pixels == resume_stack_frame(app).pixels, "Spectator closed owned Controls");
        if (count == 2) { painter_scripted_pads::states[3].connected = false; tick(); device = 0;
            tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_up); painter_scripted_pads::states[3].connected = true; tick(); tick(); device = 3; }
        tap(GamepadButton::a); require(stack.depth() == 6, "Owned Controls did not push Pad Sticks"); tick();
        const Frame sticks = resume_stack_frame(app); tap_slot(keyboard, GamepadButton::b);
        require(stack.depth() == 6 && sticks.pixels == resume_stack_frame(app).pixels, "Spectator closed owned Pad Sticks");
        if (count == 2) { painter_scripted_pads::states[3].connected = false; tick(); device = 0;
            tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_up); painter_scripted_pads::states[3].connected = true; tick(); tick(); device = 3; }
        tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_right);
        for (int i = 1; i < 7; ++i) { tap(GamepadButton::dpad_down); } tap(GamepadButton::a); // Apply
        expected.sticks[3].movement_deadzone += 5; preserved(); tap(GamepadButton::b); require(stack.depth() == 5, "Sticks Back skipped Controls");
        for (int i = 0; i < 6; ++i) { tap(GamepadButton::dpad_down); } tap(expected.view_shake ? GamepadButton::dpad_left : GamepadButton::dpad_right);
        for (int i = 0; i < 3; ++i) { tap(GamepadButton::dpad_down); } tap(GamepadButton::a); expected.view_shake = !expected.view_shake; preserved();
        tap(GamepadButton::b); require(stack.depth() == 4 && options.pixels == resume_stack_frame(app).pixels, "Controls return lost retained Options focus");
        compare("controls-sticks-merged"); check_world();
        expected.reduce_effects = true; const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = true;
        go(2); const Frame quiet = compare("reduced", true); tick(0.0f); tick(-0.25f); tick(settled);
        require(quiet.pixels == resume_stack_frame(app).pixels, "Reduced Options retained motion");
        expected.reduce_effects = false; const_cast<PlaySettings&>(preferences.play_settings()).reduce_effects = false;
        stack.notify_activation(false); tick(2.0f); stack.notify_activation(true); tick(); compare("activation-return"); check_world();
        // Only the deliberate typed Pause Resume begins its countdown.
        tap(GamepadButton::b); require(stack.depth() == 3 && game->resume_remaining() == 0.0f, "Options Back resumed match");
        tap(GamepadButton::b); require(stack.depth() == 2 && game->resume_remaining() > 0.0f, "Pause Resume lost its authored count-in");
    }
    // Private windowed trial paths remain children of the retained Options.
    open_match(2);
    const auto begin_trial = [&] { go(4); tap(GamepadButton::a); resolution_value = L"1920x1080"; palette_value = L"Random";
        status = original.menu_text().status.options_pending; go(5); const auto before = bytes(); tap(GamepadButton::a); hide(); tick();
        require(stack.depth() == 5 && bytes() == before && preferences.resolution() == ScreenResolution::s_1280_720,
            "Paused display trial wrote before Keep or replaced its parent"); };
    const auto rolled_back = [&] { hide(); tick(); require(stack.depth() == 4 && app.resolution_manager()->resolution_ivec() == Vector2I(1280,720) &&
        preferences.resolution() == ScreenResolution::s_1280_720, "Paused trial rollback lost old preset or Options parent");
        status = original.menu_text().status.options_reverted; compare("trial-reverted"); check_world(); };
    for (int path = 0; path < 5; ++path)
    {
        const auto before = bytes(); begin_trial(); const int depth = stack.depth(); tap_slot(keyboard, GamepadButton::b);
        require(stack.depth() == depth, "Spectator cancelled owned display trial");
        if (path == 0) { painter_scripted_pads::states[3].connected = false; tick(); device = 0; tap(GamepadButton::a);
            painter_scripted_pads::states[3].connected = true; tick(); tick(); device = 3; }
        else if (path == 1) { tap(GamepadButton::b); }
        else if (path == 2) { tick(original.main_menu().display_trial_seconds + 0.1f); }
        else if (path == 3) { stack.notify_activation(false); tick(); stack.notify_activation(true); tick(); }
        else { stack.push(std::make_unique<ResumeProbeRoot>()); require(stack.depth() == 6 && app.resolution_manager()->resolution_ivec() == Vector2I(1280,720), "Covered paused trial failed immediate rollback");
            tick(1.0f); require(stack.depth() == 6, "Covered paused trial popped its cover"); stack.pop(); tick(); }
        rolled_back(); require(bytes() == before, "Reverted paused trial wrote preferences");
    }
    begin_trial(); require(std::filesystem::create_directory(blocked), "Cannot create controlled paused Keep refusal");
    const auto durable = bytes(); tap(GamepadButton::dpad_up); tap(GamepadButton::a); hide(); tick();
    require(stack.depth() == 4 && preferences.resolution() == ScreenResolution::s_1920_1080 && bytes() == durable,
        "Failed paused Keep lost active candidate or durable old preferences"); status = original.menu_text().status.options_save_failed; warning = true;
    resize({1280,720}); compare("keep-save-failed", true); check_world();
    require(std::filesystem::remove(blocked), "Cannot remove controlled paused Keep refusal"); tap(GamepadButton::a); status = original.menu_text().status.options_saved; warning = false;
    compare("keep-save-recovered", true); Save loaded; loaded.load_save_file();
    require(loaded.resolution() == ScreenResolution::s_1920_1080 && loaded.team_palette() == TeamColourPair::random, "Paused Keep recovery failed reload"); preserved(); check_world();
    // Public developer hooks reject match replacement and unwind only this child.
    tap(GamepadButton::b); MenuInput direct_input(app.gamepads(), app.keyboard(), &tuning); MenuContext direct_context(&data, &direct_input, &plan);
    auto direct = std::make_unique<MainMenuOptions>(&direct_context, owner); auto* raw = direct.get(); stack.push(std::move(direct)); tick();
    require(!raw->debug_start_match(plan) && stack.depth() == 4, "Paused Options developer start replaced its running match");
    raw->debug_reset_input(owner); require(raw->debug_dismiss(false) && stack.depth() == 3, "Paused Options developer dismissal skipped Pause"); check_world();
    bool invalid = false; try { MainMenuOptions rejected(&direct_context, keyboard); } catch (const std::invalid_argument&) { invalid = true; }
    require(invalid, "Paused Options accepted a spectator as its match owner");
    require(records.good() && worlds.good(), "Paused Options evidence write failed");
    std::cout << "PASS actual paused Options1/2/4 players; complete six-aspect references, sparse owner/recovery, nested Controls/Sticks merge, retained painted match/HUD, next-build palette, private windowed trial rollback/Keep/failure/recovery and safe developer unwind\n";
}
