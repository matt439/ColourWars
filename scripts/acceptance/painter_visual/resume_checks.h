// Actual GameLevel/StateContext flow on the probe's hidden hardware shell.
// Public input messages drive the same pause pages as a player. Extra seats
// are idle dummies (Debug gives them idle bots), never claimed physical pads.
void resume_matrix(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    // Finished matches write stage records into this matrix's own profile.
    const NestedSaveProfile profile(output / "resume-profile");
    Save preferences;
    // Keep this fixture's preferences in memory; only stage records are saved.
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0;
    play.aim_marker = false;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true } };
    GameData data;
    data.set_application(&app);
    data.set_save(&preferences);
    data.set_level_infos(&levels);
    data.set_stages(&stages);
    data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    DebugDirector director(&tuning, &presentation);
    data.set_debug(&director);
#if COLOURWARS_DEV_TOOLS
    director.impl()->set_content_directories(content, content);
#else
    static_cast<void>(content);
#endif
    director.attach(&app, &data);
#if COLOURWARS_DEV_TOOLS
    director.run_launch_arguments({ L"+intro", L"off", L"+bot", L"all", L"idle" });
#endif
    StateContext& stack = app;
    struct ClearStates
    {
        StateContext& stack;
        ~ClearStates() { this->stack.clear(); }
    } clear{ stack };
    const float duration = presentation.pause_menu().resume_countdown;
    require(duration > 0.5f, "Resume hardware fixture needs a visible authored count-in");
    std::ofstream evidence(output / "resume.csv");
    evidence << "players,phase,remaining,projectiles,health,ammo,respawn\n";

    app.keyboard()->set_focused(true);
    app.mouse()->set_focused(true);
    app.keyboard()->poll(); app.keyboard()->poll();
    app.mouse()->poll(); app.mouse()->poll();
    for (const int count : { 1, 2, 4 })
    {
        stack.clear();
        stack.notify_activation(true);
        app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
        std::ignore = app.renderer()->window_size_changed(1280, 720);
        stack.push(std::make_unique<ResumeProbeRoot>());
        MenuLevelSettings settings;
        settings.set_stage("movement_course");
        settings.set_duration(60.0f);
        settings.set_player_count(count);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
            count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        for (int view = 0; view < count; ++view)
        {
            settings.set_player_team(view, view == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(view, WeaponType::sprayer);
        }
        auto owned = std::make_unique<GameLevel>(&data, settings);
        GameLevel* game = owned.get();
        bool ended = false;
        stack.push<EndMenuAction>(std::move(owned), [&](const EndMenuAction& result)
        {
            require(result == EndMenuAction::exit, "Resume fixture left with the wrong end action");
            ended = true;
        });
        const auto tick = [&](float elapsed = dt)
        {
            app.keyboard()->poll();
            app.mouse()->poll();
            stack.update(elapsed);
        };
        const auto key = [&](Key value, bool held)
        {
            if (held) { app.keyboard()->on_key_down(value); }
            else { app.keyboard()->on_key_up(value); }
            tick(0.0f);
        };
        const auto tap = [&](Key value)
        {
            key(value, true);
            key(value, false);
        };
        const auto enter_live = [&]
        {
            for (int step = 0; step < 2400 && game->level().state() != LevelState::active; ++step) { tick(); }
            require(stack.depth() == 2 && game->level().state() == LevelState::active,
                "Actual GameLevel did not enter live play");
            // Let the opening's GO tail expire before comparing frozen HUD pixels.
            for (int step = 0; step < 70; ++step) { tick(); }
        };
        const auto pause = [&]
        {
            tap(Key::escape);
            require(stack.depth() == 3, "A fresh Escape did not open the real pause page");
            tick(0.0f);
#if COLOURWARS_DEV_TOOLS
            require(!data.debug().impl()->execute("perf.capture 1 pause-rejected").ok &&
                !data.debug().impl()->perf().capture_active(),
                "A covered pause page armed a live-match pacing capture");
#endif
        };
        const auto resume = [&]
        {
            tap(Key::enter);
            require(stack.depth() == 2 && game->resume_remaining() == duration,
                "Actual pause-page Resume did not start one full count-in");
#if COLOURWARS_DEV_TOOLS
            require(!data.debug().impl()->execute("perf.capture 1 countin-rejected").ok &&
                !data.debug().impl()->perf().capture_active(),
                "A frozen resume count-in armed a live-match pacing capture");
#endif
        };
        const auto record = [&](const char* phase)
        {
            const Level& level = game->level();
            const Player& player = level.player(0);
            evidence << count << ',' << phase << ',' << game->resume_remaining() << ','
                << level.projectile_count() << ',' << player.health() << ','
                << player.weapon_ammo() << ',' << player.respawn_timer() << '\n';
        };
        enter_live();
        Level& level = const_cast<Level&>(game->level());
        require(level.place_player(0, { 559.0f, 715.0f }), "Cannot place resume fixture over the drop platform");
        for (int step = 0; step < 30; ++step) { tick(); }
        Player& player = const_cast<Player&>(level.player(0));
        const ProjectileSpray hit(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
            PlayerTeam::b, 0, Colour::white, app.render_resources(), resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()), &tuning.projectile(ProjectileType::spray));
        player.on_contact(hit, Vector2F::ZERO, 0.0f);
        require(player.health() < tuning.player().starting_health, "Resume fixture did not receive a nonfatal hit");
        // Put health close to regeneration, then pay a fresh shot so ammo,
        // cooldown, moving paint and cosmetic pulses all have work pending.
        const int warm = static_cast<int>((tuning.player().health_regen_delay - 0.2f) / dt);
        for (int step = 0; step < warm; ++step) { tick(); }
        app.mouse()->on_move({ 640, 0 });
        app.mouse()->on_button_down(MouseButton::left);
        for (int step = 0; step < 12 && !player.primary_weapon().shooting_this_update(); ++step) { tick(); }
        require(player.weapon_ammo() < tuning.starting_ammo() &&
            level.projectile_count() > 0, "Resume fixture did not retain a paid shot in flight");
        app.mouse()->on_button_up(MouseButton::left);
        pause();
        resume();
        const auto frozen = resume_observation(level);
        const Frame frozen_world = render(level, *app.renderer());
        record("begin");
        tick(0.0f); tick(-1.0f);
        require(game->resume_remaining() == duration && frozen == resume_observation(level),
            "Nonpositive GameLevel time advanced the count-in or world");
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
        {
            resize(app, level, size);
            resume_capture(app, *game, presentation, output, "resume-" + std::to_string(count) + "p-" +
                std::to_string(size.x) + 'x' + std::to_string(size.y));
        }
        resize(app, level, { 1280, 720 });
        require(frozen_world.pixels == render(level, *app.renderer()).pixels,
            "Frozen resume resize round-trip changed the underlying world/HUD");
        if (count == 4)
        {
            // Capture the actual second displayed number as well as the
            // initial beat, with the same frozen world under Reduce effects.
            tick(duration - 1.0f);
            require(std::ceil(game->resume_remaining()) == 1.0f,
                "Resume native one-second capture missed its displayed beat");
            resume_capture(app, *game, presentation, output, "resume-4p-1280x720-one");
            play.reduce_effects = true;
            resume_capture(app, *game, presentation, output, "resume-4p-1280x720-one-reduced");
            play.reduce_effects = false;
        }
        tick(game->resume_remaining() * 0.4f);
        require(game->resume_remaining() < duration && frozen == resume_observation(level),
            "Actual GameLevel advanced match/player/cosmetic state while counting");
        // A fresh pause interrupts immediately, and selecting Resume starts a
        // new full gate rather than continuing the old partial phrase.
        pause(); resume();
        require(frozen == resume_observation(level), "Re-pausing the count-in changed the frozen world");
        tick(duration * 0.25f);
        stack.notify_activation(false);
        const float unfocused = game->resume_remaining();
#if COLOURWARS_DEV_TOOLS
        require(!data.debug().impl()->execute("perf.capture 1 focus-rejected").ok &&
            !data.debug().impl()->perf().capture_active(),
            "An unfocused match armed a live-match pacing capture");
#endif
        tick(duration + 8.0f);
        require(game->resume_remaining() == unfocused && frozen == resume_observation(level),
            "An unfocused GameLevel advanced its resume clock or world");
        stack.notify_activation(true);
        require(game->resume_remaining() == duration, "Focus return did not restart the interrupted count-in");
        // An arbitrary covering state has no pause-menu result callback. A
        // focus interruption under it still requires a fresh full count when
        // the match is uncovered, rather than resuming the partial old count.
        tick(duration * 0.25f);
        const float covered_remaining = game->resume_remaining();
        stack.push(std::make_unique<ResumeProbeRoot>());
        stack.notify_activation(false);
        tick(duration + 1.0f);
        stack.notify_activation(true);
        require(stack.depth() == 3 && game->resume_remaining() == covered_remaining,
            "Focus return started a hidden count-in under an unrelated covering state");
        stack.pop();
        require(stack.depth() == 2 && game->resume_remaining() == duration &&
            frozen == resume_observation(level),
            "Uncovering a focus-interrupted match did not restart its full count-in");
        // Held action requests cross the exact GO boundary. Even an oversized
        // final tick is entirely count-in time and leaves no catch-up update.
        app.keyboard()->on_key_down(Key::space);
        app.keyboard()->on_key_down(Key::s);
        app.mouse()->on_button_down(MouseButton::left);
        tick(duration + 5.0f);
        require(game->resume_remaining() == 0.0f && frozen == resume_observation(level) &&
            frozen_world.pixels == render(level, *app.renderer()).pixels,
            "Completing GameLevel count-in advanced the clock, projectiles, regen, cooldowns or cosmetics");
        record("complete-frozen");
        tick();
        require(!player.input_jump_pressed() && !player.input_primary_shoot() && !player.input().drop_through,
            "Actions held through the count-in leaked into the first live update");
        app.keyboard()->on_key_up(Key::space);
        app.keyboard()->on_key_up(Key::s);
        app.mouse()->on_button_up(MouseButton::left);
        tick();
        app.keyboard()->on_key_down(Key::space);
        app.mouse()->on_button_down(MouseButton::left);
        tick();
        require(player.input_jump_pressed() && player.input_primary_shoot() && player.velocity().y < 0.0f,
            "A deliberate fresh jump/fire after neutral was swallowed");
        app.keyboard()->on_key_up(Key::space);
        app.mouse()->on_button_up(MouseButton::left);
        tick();
        app.keyboard()->on_key_down(Key::s);
        tick();
        require(player.input().drop_through, "A deliberate fresh drop after neutral was swallowed");
        app.keyboard()->on_key_up(Key::s);
        tick();

        // Dead-player redraw/respawn shares the same complete freeze, including
        // solo practice where there is no second living player.
        require(level.kill_player(0), "Cannot kill resume fixture player");
        tick();
        require(player.state() == PlayerState::dead, "Resume respawn fixture stayed alive");
        pause(); resume();
        const auto dead = resume_observation(level);
        const float respawn = player.respawn_timer();
        tick(duration + tuning.player().respawn_delay + 1.0f);
        require(dead == resume_observation(level) && player.respawn_timer() == respawn,
            "Resume count-in advanced a dead player's respawn/redraw");
        record("dead-frozen");
        tick();
        require(player.respawn_timer() < respawn, "Live simulation did not continue after the resume gate");

        // Foreground return while a pause page covers the match must still
        // wait for its Resume result; page coverage and focus are independent.
        pause();
        stack.notify_activation(false);
        tick(duration + 1.0f);
        stack.notify_activation(true);
        require(stack.depth() == 3 && game->resume_remaining() == 0.0f,
            "Focus return under pause started a hidden count-in");
        resume();
        tick(duration + 1.0f);
        pause();
        // Resume, Restart, Options, Quit: Restart is the next row. The
        // confirmation defaults to No, so Up deliberately selects Yes.
        tap(Key::down); tap(Key::enter); tick(0.0f);
        tap(Key::up); tap(Key::enter);
        require(stack.depth() == 2 && game->resume_remaining() == 0.0f &&
            game->level().state() != LevelState::active,
            "Actual Restart kept a resume gate or skipped the new round opening");
        enter_live();
        pause();
        tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::enter); tick(0.0f);
        tap(Key::up); tap(Key::enter);
        require(ended && stack.depth() == 1, "Actual Quit did not leave the match immediately");
        std::cout << "PASS actual " << count << "P GameLevel pause/resume input, freeze, focus, restart and quit\n";
    }
    menu_music_flow_matrix(app, data, presentation, output);
    arena_music_flow_matrix(app, data, tuning, presentation, output);
    results_music_flow_matrix(app, data, presentation, output);
    debug_frame_text_matrix(app, data, presentation, output);
    require(evidence.good(), "Resume evidence write failed");
}
