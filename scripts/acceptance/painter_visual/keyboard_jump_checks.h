// Included inside painter_visual.cpp's private namespace, after the settings
// fixtures. This is deliberately a small native UI/save slice. The CPU case
// owns exhaustive key-domain, gesture and font-validation coverage.
void keyboard_jump_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const NestedSaveProfile isolated(output / "keyboard-jump-profile");
    const auto save_path = output / "keyboard-jump-profile" / "ColourWars" / "save_data.json";
    const auto blocked_path = std::filesystem::path(save_path.wstring() + L".tmp");
    Save preferences;
    PlaySettings quiet;
    quiet.music_volume = quiet.sfx_volume = 0;
    require(preferences.set_play_settings_and_save(quiet), "Cannot seed the private Jump play preferences");
    require(preferences.set_display_settings_and_save(ScreenResolution::s_1920_1080,
        false, TeamColourPair::blue_yellow), "Cannot seed the private Jump display preferences");
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_tuning(&tuning);
    data.set_presentation(&presentation); data.set_level_infos(&levels); data.set_stages(&stages);
    DebugDirector director(&tuning, &presentation);
    data.set_debug(&director);
#if COLOURWARS_DEV_TOOLS
    director.impl()->set_content_directories(content, content);
    director.impl()->set_user_directory(output / "keyboard-jump-profile" / "debug");
#else
    static_cast<void>(content);
#endif
    director.attach(&app, &data);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    const Presentation original = presentation;
    struct RestoreKeyboardJump
    {
        StateContext& stack;
        Presentation& presentation;
        const Presentation& original;
        std::filesystem::path blocked;
        ~RestoreKeyboardJump()
        {
            this->stack.clear(); this->presentation = this->original;
            std::error_code error; std::filesystem::remove(this->blocked, error);
        }
    } restore{ stack, presentation, original, blocked_path };
    const auto bytes = [&]
    {
        std::ifstream file(save_path, std::ios::binary);
        require(file.good(), "Keyboard Jump private save disappeared");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    size_t updates = 0, copies = 0, reloads = 0, opening_captures = 0;
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); ++updates;
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
    };
    const auto move = [&](Key direction, int count)
        { for (int index = 0; index < count; ++index) { tap(direction); } };
    const auto neutral = [&]
    {
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (unsigned int key = 1; key < static_cast<unsigned int>(Key::count); ++key)
            { app.keyboard()->on_key_up(static_cast<Key>(key)); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions();
    };
    const auto size_to = [&](Vector2I size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto open_controls = [&]
    {
        stack.clear(); stack.notify_activation(true); neutral();
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        stack.push(std::make_unique<ControlsSettingsMenu>(&context)); tick();
        require(stack.depth() == 2, "Jump fixture did not open actual Controls");
    };
    const auto child = [&]
    {
        tap(Key::enter); tick();
        require(stack.depth() == 3, "Actual Controls Keyboard Jump entry did not push its child");
    };
    const auto substitute = [](std::wstring text, const wchar_t* token, std::wstring_view label)
    {
        const size_t at = text.find(token);
        require(at != std::wstring::npos, "Independent Jump copy fixture lost its token");
        text.replace(at, std::wstring_view(token).size(), label); return text;
    };
    const auto help_copy = [&](const KeyboardJumpMenuText& copy, Key current, Key pending, bool warning = false)
    {
        std::wstring result = substitute(copy.current, L"{key}", input_key_label(current)) + L'\n' +
            substitute(copy.pending, L"{key}", input_key_label(pending)) + L'\n' + copy.help;
        if (warning) { result += L'\n' + copy.debug_collision; }
        return result;
    };
    const auto camera = [&]
    {
        return Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
    };
    const auto project = [&](const RectangleF& bounds)
        { return countdown_crop_bounds(camera().calculate_view_rectangle(bounds)); };
    std::ofstream records(output / "keyboard-jump.csv");
    records << "case,width,height,current,pending,stack_depth,copy_matches,frozen_rng,updates\n";
    // Independently rebuild the actual help and status in the captured font,
    // measuring both dimensions. Comparisons cover every glyph, not just a
    // whole-frame difference when a key changes.
    const auto copy_check = [&](const char* name, const Presentation& snapshot, const std::wstring& help,
        const std::wstring& status, bool warning, Key current, Key pending, bool write = true)
    {
        const auto& look = snapshot.controls_settings(); const auto& style = snapshot.menu();
        const auto& art = look.art;
        UiText help_text("independent-jump-help", help, look.help_font, look.controls_position,
            app.render_resources(), art.ink);
        const RectangleF help_raw = help_text.bounds();
        help_text.set_scale(std::min({ 1.0f, look.controls_size.x / help_raw.width,
            look.controls_size.y / help_raw.height }));
        const RectangleF panel((Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) -
            look.box_size) / 2.0f, look.box_size);
        UiText status_text("independent-jump-status", status, look.help_font, look.status_position,
            app.render_resources(), warning ? snapshot.main_menu().options_warning_colour : art.focus_ink);
        const RectangleF status_raw = status_text.bounds();
        status_text.set_scale(std::min({ 1.0f,
            (panel.right() - art.shadow_offset - art.focus_padding.x - look.status_position.x) / status_raw.width,
            (style.footer_y - look.status_position.y) / status_raw.height }));
        const bool action_copy = std::string_view(name) == "entry" || std::string_view(name) == "held-entry";
        std::vector<std::unique_ptr<UiText>> actions;
        FocusGroup focus;
        focus.set_style({ art.focus_ink, art.ink, style.disabled });
        if (action_copy)
        {
            const auto& text = snapshot.menu_text();
            const std::array<std::wstring, 4> labels{ text.keyboard_jump.capture, text.keyboard_jump.reset,
                text.item(MenuItem::apply), text.item(MenuItem::back) };
            for (size_t row = 0; row < labels.size(); ++row)
            {
                const Vector2F position = look.layout.position + Vector2F(0.0f,
                    static_cast<float>(row) * look.layout.spacing.y);
                actions.push_back(std::make_unique<UiText>("independent-jump-action", labels[row], look.item_font,
                    position, app.render_resources(), art.ink));
                const RectangleF raw = actions.back()->bounds();
                actions.back()->set_scale(std::min({ 1.0f,
                    (look.controls_position.x - art.focus_padding.x * 2.0f - art.shadow_offset - position.x) / raw.width,
                    (look.layout.spacing.y - art.focus_padding.y * 2.0f) / raw.height }));
                focus.add(actions.back().get());
                if (std::string_view(name) == "held-entry") { focus.set_enabled(actions.back().get(), false); }
            }
        }
        require(RectangleF(look.controls_position, look.controls_size).contains(help_text.bounds()) &&
            panel.contains(status_text.bounds()), "Keyboard Jump fitted help/status escaped its paper");
        const Frame actual = main_menu_frame(app);
        std::srand(6353); const int next_random = std::rand(); std::srand(6353);
        require(actual.pixels == main_menu_frame(app).pixels && std::rand() == next_random,
            "Frozen Keyboard Jump draw changed pixels or consumed randomness");
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(camera()); list.set_filter(TextureFilter::point);
        Canvas paper(app.render_resources(), art.paper); paper.draw(list, camera().visible_rectangle(
            app.viewport_manager()->fullscreen_viewport()));
        const MenuPaperArtwork artwork(panel, app.render_resources(), &style, &art, &focus);
        // Taller status lines reach the paper's lower inset shading. Retain
        // that actual background for every state, even without action copy.
        artwork.draw(list);
        list.set_filter(TextureFilter::linear); help_text.draw(list); status_text.draw(list);
        for (const auto& action : actions) { action->draw(list); }
        const Frame expected = read(renderer);
        const std::string stem = std::string("keyboard-jump-") + name + '-' +
            std::to_string(actual.width) + 'x' + std::to_string(actual.height);
        for (const auto& entry : { std::pair{ &help_text, "help" }, std::pair{ &status_text, "status" } })
        {
            const RectangleI crop = project(entry.first->bounds());
            const Frame observed = foreground_crop(actual, crop, 0);
            const Frame wanted = foreground_crop(expected, crop, 0);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + '-' + entry.second + "-failure-actual.ppm"));
                save(wanted, output / (stem + '-' + entry.second + "-failure-expected.ppm"));
                throw std::runtime_error("Actual Keyboard Jump help/status differs from independently fitted linear copy");
            }
            if (write) { save(observed, output / (stem + '-' + entry.second + ".ppm")); }
        }
        for (size_t row = 0; row < actions.size(); ++row)
        {
            const RectangleI crop = project(actions[row]->bounds());
            const Frame observed = foreground_crop(actual, crop, 0);
            const Frame wanted = foreground_crop(expected, crop, 0);
            if (observed.pixels != wanted.pixels)
            {
                save(observed, output / (stem + "-action-failure-actual.ppm"));
                save(wanted, output / (stem + "-action-failure-expected.ppm"));
                throw std::runtime_error("Actual Jump action label/fitting/disabled ink differs from independent copy");
            }
        }
        if (action_copy)
        {
            RectangleF crop(look.layout.position, Vector2F(look.controls_position.x - look.layout.position.x,
                look.layout.spacing.y * 4.0f)); crop.inflate(art.focus_padding + Vector2F(art.shadow_offset));
            save(foreground_crop(actual, project(crop)), output / (stem + "-actions.ppm"));
        }
        if (write && (std::string_view(name) == "entry" ||
            std::string_view(name) == "applied-j" || std::string_view(name) == "parent-refreshed-j"))
        { save(actual, output / (stem + "-context.ppm")); }
        ++copies;
        records << name << ',' << actual.width << ',' << actual.height << ',' << input_key_name(current) << ',' <<
            input_key_name(pending) << ',' << stack.depth() << ",true,true," << updates << '\n';
    };
    const auto reload = [&](Key key, int music)
    {
        Save reloaded; reloaded.load_save_file(); ++reloads;
        require(reloaded.input_bindings().keyboard_jump == key && reloaded.play_settings().music_volume == music &&
            reloaded.resolution() == ScreenResolution::s_1920_1080 &&
            reloaded.team_palette() == TeamColourPair::blue_yellow,
            "Jump Apply/retry lost its key, unrelated preferences or private-save reload");
    };
    const auto& copy = original.menu_text().keyboard_jump;
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        require(preferences.set_input_bindings_and_save(InputBindings{}), "Cannot reset private Jump key");
        require(preferences.set_play_settings_and_save(quiet), "Cannot reset private Jump volumes");
        size_to(size); open_controls();
        // Parent audio is pending when its independent child is entered.
        move(Key::down, 3); tap(Key::right); move(Key::up, 2); child();
        const std::string unapplied = bytes();
        copy_check("entry", original, help_copy(copy, Key::space, Key::space), copy.pending_status,
            false, Key::space, Key::space);
        // J held on entry and the Enter opener must both return to neutral.
        app.keyboard()->on_key_down(Key::j); tick(); tap(Key::enter);
        copy_check("held-entry", original, help_copy(copy, Key::space, Key::space), copy.capture_wait_neutral,
            false, Key::space, Key::space);
        app.keyboard()->on_key_up(Key::j); tick(); tick();
        copy_check("ready", original, help_copy(copy, Key::space, Key::space), copy.capture_ready,
            false, Key::space, Key::space);
        app.keyboard()->on_key_down(Key::j); tick();
        copy_check("holding-j", original, help_copy(copy, Key::space, Key::space),
            substitute(copy.capture_release, L"{key}", L"J"), false, Key::space, Key::space);
        // Re-reading this input frame cannot complete a second gesture.
        stack.update(0.0f); ++updates;
        require(preferences.input_bindings().keyboard_jump == Key::space && bytes() == unapplied,
            "Held/repeated-frame capture activated or saved Jump");
        app.keyboard()->on_key_up(Key::j); tick(); tick();
        copy_check("pending-j", original, help_copy(copy, Key::space, Key::j), copy.pending_status,
            false, Key::space, Key::j);
        move(Key::down, 3); tap(Key::enter); tick();
        require(stack.depth() == 2 && bytes() == unapplied &&
            preferences.input_bindings().keyboard_jump == Key::space,
            "Keyboard Jump Back saved its pending key or closed its parent");
        child(); // Controls retained its Keyboard Jump focus and pending audio.
        tap(Key::enter); tick();
        app.keyboard()->on_key_down(Key::j); app.keyboard()->on_key_down(Key::k); tick();
        copy_check("chord", original, help_copy(copy, Key::space, Key::space), copy.capture_rejected,
            true, Key::space, Key::space);
        app.keyboard()->on_key_up(Key::j); app.keyboard()->on_key_up(Key::k); tick();
        tap(Key::a);
        copy_check("conflict-a", original, help_copy(copy, Key::space, Key::space), copy.capture_conflict,
            true, Key::space, Key::space);
        tick();
        app.keyboard()->on_key_down(Key::j); tick(); app.keyboard()->on_key_down(Key::k); tick();
        app.keyboard()->on_key_up(Key::j); tick(); app.keyboard()->on_key_up(Key::k); tick();
        copy_check("overlap", original, help_copy(copy, Key::space, Key::space), copy.capture_rejected,
            true, Key::space, Key::space);
        tick(); tap(Key::digit_1);
        copy_check("unsupported", original, help_copy(copy, Key::space, Key::space), copy.capture_rejected,
            true, Key::space, Key::space);
        tap(Key::down); tick();
        copy_check("navigation-owned", original, help_copy(copy, Key::space, Key::space), copy.capture_rejected,
            true, Key::space, Key::space, false);
        tap(Key::escape); tick();
        require(stack.depth() == 3 && bytes() == unapplied, "Escape capture cancel escaped/saved the child");
        copy_check("cancel", original, help_copy(copy, Key::space, Key::space), copy.capture_cancelled,
            false, Key::space, Key::space);
        tap(Key::enter); tick(); app.keyboard()->on_key_down(Key::j); tick();
        app.keyboard()->set_focused(false); stack.notify_activation(false); tick();
        app.keyboard()->set_focused(true); stack.notify_activation(true); tick(); tick();
        copy_check("focus-cancel", original, help_copy(copy, Key::space, Key::space), copy.capture_cancelled,
            true, Key::space, Key::space);
        tap(Key::j); tick();
        require(bytes() == unapplied && preferences.input_bindings().keyboard_jump == Key::space,
            "A new focus-safe capture gesture activated before Apply");
        // Reset is pending; it does not change the active key or disk.
        tap(Key::down); tap(Key::enter);
        copy_check("reset-pending", original, help_copy(copy, Key::space, Key::space), copy.pending_status,
            false, Key::space, Key::space);
        tap(Key::up); tap(Key::enter); tick(); tap(Key::j); tick();
        move(Key::down, 2); tap(Key::enter); tick();
        require(preferences.input_bindings().keyboard_jump == Key::j &&
            preferences.play_settings().music_volume == 0, "Child Apply changed pending parent audio");
        reload(Key::j, 0);
        copy_check("applied-j", original, help_copy(copy, Key::j, Key::j), copy.saved_status,
            false, Key::j, Key::j);
#if COLOURWARS_DEV_TOOLS
        require(director.impl()->execute("bind j \"overlay collision\"").ok &&
            director.keyboard_jump_collision(Key::j), "Cannot seed a private bare J developer-bind collision");
        tick();
        copy_check("debug-bind-warning-j", original, help_copy(copy, Key::j, Key::j, true), copy.saved_status,
            false, Key::j, Key::j);
        require(director.impl()->execute("unbind j").ok && !director.keyboard_jump_collision(Key::j),
            "Removing a bare developer bind left the visible Jump collision active");
        tick();
#endif
        // A live template replacement cannot rebase this existing child.
        const_cast<MenuText&>(presentation.menu_text()).keyboard_jump.help = L"Replacement help for a later page.";
        tick();
        copy_check("retained-child-copy", original, help_copy(copy, Key::j, Key::j), copy.saved_status,
            false, Key::j, Key::j, false);
        presentation = original;
        tap(Key::down); tap(Key::enter); tick();
        const std::wstring parent_help = substitute(original.menu_text().help.controls, L"{jump}", L"J");
        copy_check("parent-refreshed-j", original, parent_help, original.menu_text().status.controls_pending,
            false, Key::j, Key::j);
        move(Key::down, 7); tap(Key::enter); tick();
        require(preferences.play_settings().music_volume == 10 && preferences.input_bindings().keyboard_jump == Key::j,
            "Parent Apply lost its pending audio or independently applied Jump key");
        reload(Key::j, 10);
        move(Key::up, 7); child();
        // A refused write leaves the effective choice active and the old JSON
        // intact. The same real Apply widget retries after the refusal clears.
        tap(Key::enter); tick(); tap(Key::k); tick(); move(Key::down, 2);
        const std::string before_failure = bytes();
        require(std::filesystem::create_directory(blocked_path), "Cannot refuse private Jump temporary write");
        tap(Key::enter); tick();
        require(preferences.input_bindings().keyboard_jump == Key::k && bytes() == before_failure,
            "Failed Jump write lost the active choice or replaced the old JSON");
        reload(Key::j, 10);
        copy_check("save-failed-k", original, help_copy(copy, Key::k, Key::k),
            original.menu_text().status.active_save_failed, true, Key::k, Key::k);
        require(std::filesystem::remove(blocked_path), "Cannot clear private Jump temporary-write refusal");
        tap(Key::enter); tick(); reload(Key::k, 10);
        // Reset then Back discards its Space proposal while K stays effective.
        tap(Key::up); tap(Key::enter);
        copy_check("reset-active-k", original, help_copy(copy, Key::k, Key::space), copy.pending_status,
            false, Key::k, Key::space);
        move(Key::down, 2); tap(Key::enter); tick(); child();
        copy_check("reopened-k", original, help_copy(copy, Key::k, Key::k), copy.pending_status,
            false, Key::k, Key::k);
        // Capturing Space consumes its release. It cannot confirm Apply; a
        // separate fresh Space gesture still has the fixed menu Proceed role.
        tap(Key::enter); tick(); tap(Key::space); tick();
        const std::string before_space_apply = bytes();
        move(Key::down, 2); tick(); tick();
        require(preferences.input_bindings().keyboard_jump == Key::k && bytes() == before_space_apply,
            "Space capture leaked its release into Apply");
        copy_check("space-awaits-confirm", original, help_copy(copy, Key::k, Key::space), copy.pending_status,
            false, Key::k, Key::space);
        tap(Key::space); tick(); reload(Key::space, 10);
        copy_check("space-fresh-confirm", original, help_copy(copy, Key::space, Key::space), copy.saved_status,
            false, Key::space, Key::space);
        stack.clear();
    }

    // Actual GameLevel -> Pause -> Options -> Controls -> Jump, during the
    // initial countdown. The covered Level never receives a simulation tick.
    // GameLevel::draw must refresh the key even at the same resolution.
    require(preferences.set_play_settings_and_save(quiet), "Cannot quiet the private paused reminder match");
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        require(preferences.set_input_bindings_and_save(InputBindings{}), "Cannot seed paused Jump reminder");
        size_to(size); stack.clear(); stack.notify_activation(true); neutral();
        settings.set_stage("movement_course"); settings.set_duration(60.0f); settings.set_player_count(1);
        settings.begin_input_claims(input_slots::keyboard_and_mouse); settings.set_screen_layout(ScreenLayout::one_player);
        settings.set_player_team(0, PlayerTeam::a); settings.set_player_weapon(0, WeaponType::sprayer);
        stack.push(std::make_unique<ResumeProbeRoot>());
        auto owned = std::make_unique<GameLevel>(&data, settings); GameLevel* game = owned.get();
        stack.push<EndMenuAction>(std::move(owned), [](const EndMenuAction&)
            { throw std::runtime_error("Paused Jump reminder unexpectedly ended its match"); });
        require(game->level().state() == LevelState::start_countdown,
            "Keyboard-only actual match did not begin its countdown reminder");
        const auto draw_game = [&]
        {
            app.renderer()->begin_frame(); game->draw(*app.renderer()); return read(*app.renderer());
        };
        const Frame before = draw_game();
        const std::wstring space_reminder = L"A / D  Move    |    S  Drop through\nSpace  Jump    |    Left click  Shoot\nMouse  Aim    |    Esc  Pause";
        const std::wstring j_reminder = L"A / D  Move    |    S  Drop through\nJ  Jump    |    Left click  Shoot\nMouse  Aim    |    Esc  Pause";
        require(game->level().keyboard_controls_text() == space_reminder,
            "Built opening reminder did not show the effective default Jump key");
        tap(Key::escape); tick(); require(stack.depth() == 3, "Countdown Escape did not open actual Pause");
        move(Key::down, 2); tap(Key::enter); tick();
        require(stack.depth() == 4, "Paused Jump route did not open actual Options");
        move(Key::down, 3); tap(Key::enter); tick();
        require(stack.depth() == 5, "Paused Jump route did not open actual Controls");
        tap(Key::down); tap(Key::enter); tick();
        require(stack.depth() == 6, "Paused Controls did not open Keyboard Jump");
        const auto frozen = resume_observation(game->level());
#if COLOURWARS_DEV_TOOLS
        const auto fingerprint = MatchFingerprint::of(game->level(), 0);
#endif
        tap(Key::enter); tick(); tap(Key::j); tick(); move(Key::down, 2); tap(Key::enter); tick();
        // New live reminder geometry must not replace the built text snapshot.
        auto& live = const_cast<MatchPresentation&>(presentation.match());
        live.controls_padding *= 2.0f; live.controls_gap *= 2.0f;
        const Frame changed = draw_game();
        require(game->level().keyboard_controls_text() == j_reminder &&
            game->level().state() == LevelState::start_countdown && game->resume_remaining() == 0.0f &&
            frozen == resume_observation(game->level()),
            "Paused reminder missed the new key or changed public simulation/count-in state");
        const RectangleF bounds = game->level().keyboard_controls_text_bounds();
        const RectangleF card = game->level().keyboard_controls_card_bounds();
        require(bounds.width > 0.0f && bounds.height > 0.0f && card.contains(bounds),
            "Paused refreshed reminder lost fitted text/shadow inside its card");
        const RectangleI pixels = countdown_crop_bounds(card);
        require(foreground_crop(before, pixels, 0).pixels != foreground_crop(changed, pixels, 0).pixels,
            "Paused reminder key change produced no native ink delta");
        std::srand(6373); const int next_random = std::rand(); std::srand(6373);
        require(changed.pixels == draw_game().pixels && std::rand() == next_random,
            "Paused reminder draw changed frozen pixels or consumed randomness");
        const std::string stem = "keyboard-jump-paused-opening-" + std::to_string(size.x) + 'x' + std::to_string(size.y);
        save(foreground_crop(changed, pixels), output / (stem + ".ppm"));
        // Reverse only the key, still without Level::update; captured layout
        // restores the exact old reminder despite the live geometry edit.
        require(preferences.set_input_bindings_and_save(InputBindings{}), "Cannot reverse paused Jump key");
        const Frame reversed = draw_game();
        const RectangleI restored = countdown_crop_bounds(game->level().keyboard_controls_card_bounds());
        require(foreground_crop(before, restored, 0).pixels == foreground_crop(reversed, restored, 0).pixels &&
            frozen == resume_observation(game->level()),
            "Paused key reversal adopted unrelated live geometry or advanced simulation");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(game->level(), 0)).empty(),
            "Paused Jump UI/refresh changed replay fingerprint");
#endif
        records << "paused-opening," << size.x << ',' << size.y << ",j,j," << stack.depth() <<
            ",true,true," << updates << '\n'; ++opening_captures;
        stack.clear(); presentation = original;
    }
    constexpr size_t expected_copies = COLOURWARS_DEV_TOOLS ? 44 : 42;
    require(copies == expected_copies && reloads == 10 && opening_captures == 2 && records.good(),
        "Keyboard Jump native evidence did not complete its bounded copy/reload/opening cases");
    std::cout << "PASS Keyboard Jump native: " << copies << " independently fitted help/status pairs, " <<
        reloads << " private Save reloads, " << opening_captures << " actual paused countdown reminders; " <<
        updates << " locally fed menu updates at 720p/1080p; actual Controls/child Apply/Reset/Back, "
        "held entry, J release, conflict/chord/overlap/unsupported/cancel/focus, failed write/retry, "
        "independent parent audio and fresh Space Proceed; frozen RNG/public state/fingerprint where available. "
        "Scope excludes process relaunch, physical devices, exhaustive key domain and human readability.\n";
}
