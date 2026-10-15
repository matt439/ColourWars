// Real team/weapon pages and input arming, with only the GamepadReader backend
// scripted. No physical controller, native foreground or player save is used.
void team_setup_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearTeamSetup
    {
        StateContext& stack;
        ~ClearTeamSetup() { this->stack.clear(); }
    } clear{ stack };
    const std::array<int, 4> slots{ input_slots::keyboard_and_mouse, 3, 0, 2 };
    const std::array<WeaponType, 4> weapons{ WeaponType::sprayer, WeaponType::sniper,
        WeaponType::roller, WeaponType::bucket };
    const auto bit = [](GamepadButton button)
        { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto tick = [&]
    {
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(dt);
    };
    const auto key_for = [](GamepadButton button)
    {
        return button == GamepadButton::x ? Key::r : button == GamepadButton::a ? Key::enter :
            button == GamepadButton::b ? Key::escape : button == GamepadButton::dpad_left ? Key::left : Key::right;
    };
    const auto hold = [&](int player, GamepadButton button, bool down)
    {
        const int slot = slots[static_cast<size_t>(player)];
        if (slot == input_slots::keyboard_and_mouse)
        {
            if (down) { app.keyboard()->on_key_down(key_for(button)); }
            else { app.keyboard()->on_key_up(key_for(button)); }
        }
        else
        {
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit(button)) :
                static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button)));
        }
    };
    const auto tap = [&](int player, GamepadButton button)
    {
        hold(player, button, true); tick(); hold(player, button, false); tick();
    };
    const auto frame = [&]
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto resize_page = [&](const Vector2I& size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
    };
    const auto reference = [&](const MenuLevelSettings& plan, bool weapon = false)
    {
        MenuLevelSettings reference_settings = plan;
        MenuInput reference_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext reference_context(&data, &reference_input, &reference_settings);
        StateContext reference_stack;
        // Replace the real footer with an independently specified full legend.
        // This also verifies that X/R and its action fit at native resolution.
        const PromptBarPresentation original = presentation.prompt_bar();
        auto& prompt = const_cast<PromptBarPresentation&>(presentation.prompt_bar());
        struct RestorePrompt
        {
            PromptBarPresentation& look;
            PromptBarPresentation original;
            ~RestorePrompt() { this->look = this->original; }
        } restore{ prompt, original };
        if (!weapon) { prompt.colour = prompt.shadow_colour = Colour(0.0f, 0.0f, 0.0f, 0.0f); }
        if (weapon) { reference_stack.push(std::make_unique<MainMenuWeaponSelect>(&reference_context)); }
        else { reference_stack.push(std::make_unique<MainMenuTeamSelect>(&reference_context)); }
        prompt = original;
        reference_stack.update(dt);
        app.renderer()->begin_frame(); reference_stack.draw(*app.renderer());
        if (!weapon)
        {
            std::vector<PromptBar::Prompt> prompts{
                { L"Left/Right", L"Pick a team" }, { L"A/Enter", L"Confirm" },
                { L"B/Esc", L"Cancel" } };
            if (plan.player_count() >= 2) { prompts.push_back({ L"X/R", L"Random teams" }); }
            const PromptBar expected("expected-team-legend", prompts, original, app.render_resources());
            DrawList list = app.renderer()->view(app.renderer()->view_count() - 1);
            list.set_viewport(app.viewport_manager()->fullscreen_viewport());
            list.set_camera(Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
                app.viewport_manager()->fullscreen_viewport()));
            list.set_filter(TextureFilter::linear); expected.draw(list);
        }
        return read(*app.renderer());
    };
    const auto same_plan = [](const MenuLevelSettings& a, const MenuLevelSettings& b, bool teams = true)
    {
        if (a.player_count() != b.player_count() || a.stage() != b.stage() || a.duration() != b.duration() ||
            a.best_of() != b.best_of() || a.screen_layout() != b.screen_layout()) { return false; }
        const auto first = a.player_settings(); const auto second = b.player_settings();
        for (size_t i = 0; i < first.size(); ++i)
        {
            if (first[i].num != second[i].num || first[i].weapon != second[i].weapon ||
                (teams && first[i].team != second[i].team)) { return false; }
        }
        return true;
    };
    const auto verify_team_page = [&](const MenuLevelSettings& plan, const char* reason)
    {
        require(frame().pixels == reference(plan).pixels, reason);
    };
    const auto open = [&](int count)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true);
        for (const Key key : { Key::r, Key::enter, Key::escape, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {};
        for (int i = 1; i < count; ++i)
            { painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(i)])].connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        settings = MenuLevelSettings(); settings.set_player_count(count);
        settings.begin_input_claims(slots[0]);
        for (int i = 1; i < count; ++i) { require(settings.claim_input_slot(slots[static_cast<size_t>(i)]), "Scripted setup seat claim failed"); }
        settings.set_stage("movement_course"); settings.set_duration(123.0f); settings.set_best_of(5);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
            count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        for (int i = 0; i < count; ++i)
        {
            settings.set_player_team(i, i % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(i, weapons[static_cast<size_t>(i)]);
        }
        input.reset_actions(); stack.push(std::make_unique<MainMenuTeamSelect>(&context)); tick(); tick();
    };
    const auto& team_look = presentation.main_menu().team_select;
    const auto& style = presentation.menu();
    const Vector2F last_row(150.0f, 150.0f + team_look.spacing.y * 5.0f);
    const auto measure = [&](const std::string& font, const std::wstring& text)
        { return app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(font), text); };
    const Vector2F label = measure(style.detail_font, L"P4 (Pad 3)");
    require(last_row.y + std::max(label.y, team_look.size.y) + style.item_shadow_offset.y < style.footer_y,
        "Four team-selection rows overlap the shared footer");
    const auto& prompt = presentation.prompt_bar();
    const Vector2F legend = measure(prompt.font,
        L"Left/Right Pick a team    A/Enter Confirm    B/Esc Cancel    X/R Random teams");
    require(prompt.position.y >= style.footer_y &&
        RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION).contains(
            RectangleF(prompt.position, legend + prompt.shadow_offset)),
        "Complete Random teams control legend escapes the shared footer");
    std::ofstream evidence(output / "team-setup.csv");
    evidence << "players,seed,requester,team_a,team_b,all_readiness_cleared,settings_preserved\n";
    bool majority_a = false, majority_b = false;
    resize_page({ 1280, 720 });
    for (const int count : { 2, 3, 4 })
    {
        for (unsigned int seed = 1; seed <= 8; ++seed)
        {
            open(count);
            const MenuLevelSettings initial = settings;
            for (int player = 0; player < count - 1; ++player) { tap(player, GamepadButton::a); }
            const Frame partly_ready = frame();
            require(partly_ready.pixels != reference(initial).pixels, "Pre-confirmed roster did not show any readiness");
            MenuLevelSettings shuffled = initial;
            std::srand(seed); require(shuffled.shuffle_teams(), "Expected complete roster did not shuffle");
            std::srand(seed);
            const int requester = static_cast<int>((seed - 1) % static_cast<unsigned int>(count));
            const int concurrent = (requester + 1) % count;
            hold(requester, GamepadButton::x, true); hold(concurrent, GamepadButton::a, true); tick();
            hold(requester, GamepadButton::x, false); hold(concurrent, GamepadButton::a, false); tick();
            require(same_plan(settings, initial), "Provisional shuffle modified shared match settings before confirmation");
            verify_team_page(shuffled, "Shuffle did not show balanced expected teams with ALL readiness cleared and X/R legend");
            tick(); // reset navigation must observe neutral after the shuffle
            const Frame shuffled_frame = frame();
            require(shuffled_frame.pixels == frame().pixels, "Frozen shuffled team page changed its raster");
            int a = 0;
            for (int i = 0; i < count; ++i) { a += shuffled.player_team(i) == PlayerTeam::a ? 1 : 0; }
            require(a > 0 && a < count && std::abs(a - (count - a)) <= 1, "Actual seeded shuffle was not balanced");
            if (count == 3) { majority_a = majority_a || a == 2; majority_b = majority_b || a == 1; }
            if (seed == 1)
            {
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
                {
                    resize_page(size); verify_team_page(shuffled, "Native-size shuffled rows/legend changed team/readiness state");
                    const std::string stem = "team-setup-" + std::to_string(count) + "p-" +
                        std::to_string(size.x) + "x" + std::to_string(size.y);
                    const Frame capture = frame(); save(capture, output / (stem + ".ppm"));
                    const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
                        app.viewport_manager()->fullscreen_viewport());
                    save(foreground_crop(capture, countdown_crop_bounds(camera.calculate_view_rectangle(
                        RectangleF(100.0f, 350.0f, 1450.0f, 710.0f)))), output / (stem + "-detail.ppm"));
                }
                resize_page({ 1280, 720 });
            }
            for (int player = 0; player < count - 1; ++player) { tap(player, GamepadButton::a); }
            require(same_plan(settings, initial), "Team page advanced before every shuffled player reconfirmed");
            tap(count - 1, GamepadButton::a);
            require(same_plan(settings, shuffled) && frame().pixels == reference(shuffled, true).pixels,
                "Final reconfirm did not commit just shuffled teams and enter actual Weapon Select");
            tap(0, GamepadButton::b); tick();
            verify_team_page(shuffled, "Returning from Weapon Select lost teams or retained readiness");
            require(same_plan(settings, shuffled), "Weapon return changed weapons, slots, stage, duration or series length");
            evidence << count << ',' << seed << ',' << requester << ',' << a << ',' << count - a << ",true,true\n";
        }
        std::cout << "PASS actual " << count << "P seeded team shuffle, independent requests, readiness reset, reconfirm and Weapon return\n";
    }
    require(majority_a && majority_b, "Three-player seeded hardware cases did not exercise both majority sides");

    // Held actions from the old context, same-device conflicts, missing pads
    // and focus loss must not turn into a new shuffle or a confirmation.
    open(4); const MenuLevelSettings before = settings;
    const Frame unchanged = frame();
    for (const int player : { 0, 1 })
    {
        for (const GamepadButton button : { GamepadButton::x, GamepadButton::a })
        {
            hold(player, button, true); tick();
            stack.transition_to(std::make_unique<MainMenuTeamSelect>(&context));
            hold(player, button, false); tick(); tick();
            require(frame().pixels == unchanged.pixels, "Held R/X/A release acted in a newly entered Team Select");
        }
    }
    for (const int player : { 0, 1 })
    {
        hold(player, GamepadButton::x, true); hold(player, GamepadButton::a, true); tick();
        hold(player, GamepadButton::x, false); hold(player, GamepadButton::a, false); tick();
        require(frame().pixels == unchanged.pixels, "Conflicting shuffle/confirm releases chose an arbitrary action");
    }
    painter_scripted_pads::states[0].connected = false; tick();
    const Frame missing = frame(); tap(0, GamepadButton::x);
    require(frame().pixels == missing.pixels && same_plan(settings, before), "Missing-player gate allowed shuffle");
    painter_scripted_pads::states[0].connected = true;
    painter_scripted_pads::states[0].buttons = bit(GamepadButton::x); tick(); tick();
    painter_scripted_pads::states[0].buttons = 0; tick(); tick();
    require(frame().pixels == unchanged.pixels, "Reconnecting with X held shuffled on its release");
    static_cast<WindowNotify&>(app).on_deactivated(); tick(); tap(0, GamepadButton::x);
    require(frame().pixels == unchanged.pixels, "Inactive team page accepted a shuffle");
    hold(0, GamepadButton::x, true);
    static_cast<WindowNotify&>(app).on_activated(); tick();
    hold(0, GamepadButton::x, false); tick(); tick();
    require(frame().pixels == unchanged.pixels && same_plan(settings, before), "Focus-return R release shuffled or changed match settings");

    open(4);
    for (const int player : { 1, 3 })
        { tap(player, GamepadButton::dpad_left); tap(player, GamepadButton::dpad_left); }
    for (int player = 0; player < 4; ++player) { tap(player, GamepadButton::a); }
    MenuLevelSettings warning_recovery = settings;
    std::srand(57); require(warning_recovery.shuffle_teams(), "Warning-recovery setup did not shuffle");
    std::srand(57); tap(3, GamepadButton::x); tick();
    verify_team_page(warning_recovery, "Shuffle did not clear the empty-team warning and all readiness");

    // A manual edit following a shuffle still works, and no shuffled team is
    // committed early. Swap both sides for a balanced two-player roster.
    open(2); MenuLevelSettings manual = settings;
    std::srand(91); require(manual.shuffle_teams(), "Manual-edit setup did not shuffle");
    std::srand(91); tap(1, GamepadButton::x); tick();
    for (int player = 0; player < 2; ++player)
    {
        const GamepadButton direction = manual.player_team(player) == PlayerTeam::a ?
            GamepadButton::dpad_right : GamepadButton::dpad_left;
        tap(player, direction); tap(player, direction);
        manual.set_player_team(player, manual.player_team(player) == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a);
    }
    verify_team_page(manual, "Manual team edits stopped working after shuffle");
    tap(0, GamepadButton::a); tap(1, GamepadButton::a);
    require(same_plan(settings, manual), "Manual post-shuffle teams did not commit while preserving other match settings");
    open(1); const MenuLevelSettings solo = settings;
    verify_team_page(solo, "Solo team-selection return advertised Random teams");
    const Frame solo_frame = frame(); tap(0, GamepadButton::x);
    require(same_plan(settings, solo) && frame().pixels == solo_frame.pixels,
        "Solo team-selection accepted a shuffle");

    // One actual match owns this committed shuffled roster through a normal
    // pause restart and two series rounds. Idle seats produce draws: the End
    // page's Replay Draw uses the same Session::next_round continuation path.
    open(2);
    std::srand(113); tap(1, GamepadButton::x); tick();
    tap(0, GamepadButton::a); tap(1, GamepadButton::a);
    MenuLevelSettings series = settings;
    series.set_best_of(3); series.set_duration(0.25f);
    stack.clear(); stack.push(std::make_unique<ResumeProbeRoot>());
    auto owned_game = std::make_unique<GameLevel>(&data, series);
    GameLevel* game = owned_game.get();
    bool left_series = false;
    stack.push<EndMenuAction>(std::move(owned_game), [&](const EndMenuAction& action)
    {
        require(action == EndMenuAction::exit, "Fixed-team series left through an unexpected action");
        left_series = true;
    });
    const auto game_tick = [&](float elapsed = dt)
    {
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto game_tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); game_tick(0.0f);
        app.keyboard()->on_key_up(key); game_tick(0.0f);
    };
    std::ofstream series_evidence(output / "team-series.csv");
    series_evidence << "phase,round,view,input_slot,team,weapon\n";
    const auto fixed_roster = [&](const char* phase, int round)
    {
        require(game->level().player_count() == series.player_count() &&
            app.viewport_manager()->layout() == series.screen_layout(), "Series rebuild changed roster size/layout");
        const auto players = series.player_settings();
        for (int view = 0; view < series.player_count(); ++view)
        {
            const auto& planned = players[static_cast<size_t>(view)];
            const Player& player = game->level().player(view);
            require(player.team() == planned.team && player.player_num() == planned.num &&
                player.primary_weapon().type() == planned.weapon,
                "Actual restart/series round rerolled a committed team, device slot or weapon");
            series_evidence << phase << ',' << round << ',' << view << ',' << player.player_num() << ',' <<
                static_cast<int>(player.team()) << ',' << static_cast<int>(player.primary_weapon().type()) << '\n';
        }
    };
    const auto enter_live = [&]
    {
        if (game->level().intro_playing()) { game_tap(Key::enter); }
        for (int step = 0; step < 2400 && game->level().state() != LevelState::active; ++step) { game_tick(); }
        require(stack.depth() == 2 && game->level().state() == LevelState::active,
            "Fixed-team series did not enter the actual match");
    };
    fixed_roster("initial", 1); enter_live();
    game_tap(Key::escape); game_tick(0.0f);
    require(stack.depth() == 3, "Fixed-team series did not open the actual Pause page");
    game_tap(Key::down); game_tap(Key::enter); game_tick(0.0f);
    game_tap(Key::up); game_tap(Key::enter);
    require(stack.depth() == 2 && game->level().state() != LevelState::active,
        "Normal pause Restart did not rebuild the fixed-team series");
    fixed_roster("pause-restart", 1);
    Session expected_session(3);
    for (int round = 1; round <= 2; ++round)
    {
        if (game->level().intro_playing()) { game_tap(Key::enter); }
        for (int step = 0; step < 3000 && stack.depth() == 2; ++step) { game_tick(); }
        require(stack.depth() == 3 && game->level().state() == LevelState::finished,
            "Fixed-team series did not reach actual Results");
        fixed_roster("results", round);
        expected_session.finish_round(game->level().level_end_info());
        const auto& results = presentation.results_menu();
        const int ready_steps = static_cast<int>((results.team_fill_delay + results.team_fill_time +
            results.show_results_delay + 1.0f) / dt) + 4;
        for (int step = 0; step < ready_steps; ++step) { game_tick(); }
        game_tap(Key::enter);
        require(stack.depth() == 3, "Actual series Results did not continue to End");
        // Compare the opaque primary card against an independently constructed
        // End page with the expected ledger, including its series continuation.
        MenuInput end_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext end_context(&data, &end_input, &series);
        StateContext end_stack;
        end_stack.push(std::make_unique<EndMenuInitial>(&end_context, expected_session));
        app.renderer()->begin_frame(); end_stack.draw(*app.renderer());
        const Frame expected_end = read(*app.renderer());
        const auto& end = presentation.end_menu();
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        const RectangleI primary = countdown_crop_bounds(camera.calculate_view_rectangle(
            RectangleF(end.layout.position, measure(style.item_font, expected_session.continue_string()))));
        require(foreground_crop(frame(), primary, 0).pixels == foreground_crop(expected_end, primary, 0).pixels,
            "Series End page lost its expected ongoing-series continuation");
        if (round == 1)
        {
            game_tap(Key::enter);
            require(stack.depth() == 2 && game->level().state() != LevelState::finished,
                "Normal series continuation did not create its next round");
            expected_session.next_round(); fixed_roster("next-round", 2);
        }
        else
        {
            game_tick(0.0f); game_tap(Key::down); game_tap(Key::enter);
            require(left_series && stack.depth() == 1, "Actual series Exit did not leave its match");
        }
    }
    require(series_evidence.good(), "Fixed-team series evidence write failed");
    require(evidence.good(), "Team-setup evidence write failed");
    std::cout << "PASS team setup held-entry/conflict/missing-pad/focus gates, manual edits and solo no-op\n";
    std::cout << "PASS actual shuffled 2P roster preserved through Pause Restart, Results/End and two best-of-three rounds; no physical-pad claim\n";
}
