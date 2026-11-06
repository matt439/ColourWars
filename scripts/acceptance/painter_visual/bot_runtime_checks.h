// Actual GameLevel lifecycle with a bot between two sparse human seats.
void check_playable_bot_runtime(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& output)
{
    const acceptance::IsolatedSave profile(output / "bot-players-runtime-profile");
    Save preferences; SettingsSession preferences_session(preferences); preferences_session.load_save_file();
    PlaySettings silent = preferences_session.play_settings(); silent.music_volume = silent.sfx_volume = 0;
    require(preferences_session.set_play_settings_and_save(silent), "Cannot silence private runtime fixture");
    StageList stages;
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    data.set_tuning(&tuning); data.set_presentation(&presentation);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "crossfeed" }); data.set_stage_catalog(&stage_catalog);
    StateContext& stack = app;
    struct ClearBots { StateContext& stack; ~ClearBots() { this->stack.clear(); } } clear{ stack };
    stack.clear(); stack.notify_activation(true);
    app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    for (const Key key : { Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right })
    { app.keyboard()->on_key_up(key); }
    painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = true;
    app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();

    MatchSetup settings; settings.set_player_count(4);
    settings.set_player_bot(1, BotDifficulty::easy); settings.set_player_bot(3, BotDifficulty::hard);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    require(settings.claim_input_slot(3), "Cannot claim P3's sparse human device");
    settings.set_screen_layout(ScreenLayout::two_player); settings.set_stage("crossfeed");
    settings.set_duration(3.0f); settings.set_best_of(5);
    for (int participant = 0; participant < 4; ++participant)
    {
        settings.set_player_team(participant, participant < 2 ? PlayerTeam::a : PlayerTeam::b);
        settings.set_player_weapon(participant, participant == 3 ? WeaponType::roller : WeaponType::sprayer);
    }
    stack.push(std::make_unique<ResumeProbeRoot>());
    auto owned = std::make_unique<GameLevel>(&data, settings, MatchStart::menu);
    GameLevel* game = owned.get(); stack.push(std::move(owned));
    require(preferences.last_match_setup() &&
        remembered_setup_matches(*preferences.last_match_setup(), settings),
        "Normal confirmed bot match failed to save its controller/difficulty setup");
    const auto tick = [&](float elapsed = dt)
    { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(0.0f);
        app.keyboard()->on_key_up(key); tick(0.0f);
    };
    const auto simulation = [&]
    {
        const Level& level = game->level();
        const auto score = level.level_end_info();
        std::vector<float> state{ static_cast<float>(level.state()), score.team_a_area, score.team_b_area,
            score.total_area, static_cast<float>(level.projectile_count()) };
        for (int participant = 0; participant < level.player_count(); ++participant)
        {
            const Player& player = level.player(participant);
            const RectangleF body = player.rectangle();
            state.insert(state.end(), { body.x, body.y, body.width, body.height,
                player.velocity().x, player.velocity().y, player.health(), player.weapon_ammo(),
                player.respawn_timer(), static_cast<float>(player.state()), static_cast<float>(player.move_state()),
                static_cast<float>(player.primary_weapon().readiness()) });
        }
        return state;
    };
    std::ofstream evidence(output / "bot-players-runtime.csv");
    evidence << "check,participants,views,roster_identity,lifecycle\n";
    const auto mapping = [&](const char* phase)
    {
        const Level& level = game->level();
        require(level.player_count() == 4 && level.view_count() == 2 &&
            level.participant_for_view(0) == 0 && level.participant_for_view(1) == 2 &&
            level.view_for_participant(1) == -1 && level.view_for_participant(3) == -1 &&
            level.player(2).player_num() == 3 && app.viewport_manager()->layout() == ScreenLayout::two_player,
            "Actual GameLevel confused roster identity and human panes");
        require(level.bot_difficulty(1) == BotDifficulty::easy && level.bot_difficulty(3) == BotDifficulty::hard &&
            level.bot_identity(1) == L"P2 - Bot (Easy)" && level.bot_identity(3) == L"P4 - Bot (Hard)",
            "Actual match lost bot world identity/difficulty");
        for (int participant = 0; participant < 4; ++participant)
        {
            const auto& planned = settings.player_settings()[static_cast<size_t>(participant)];
            require(level.player(participant).team() == planned.team &&
                level.player(participant).primary_weapon().type() == planned.weapon,
                "Lifecycle changed bot/human team or weapon");
        }
        evidence << phase << ",4,2,stable,passed\n";
    };
    const auto live = [&]
    {
        if (game->level().intro_playing()) { tap(Key::enter); }
        for (int step = 0; step < 2400 && game->level().state() != LevelState::active; ++step) { tick(); }
        require(stack.depth() == 2 && game->level().state() == LevelState::active,
            "Actual mixed match did not enter live play");
    };
    mapping("initial"); live();
    for (int step = 0; step < 20; ++step) { tick(); }
    tap(Key::escape); tick(0.0f);
    require(stack.depth() == 3, "Mixed roster could not open Pause");
    const auto paused = observation(game->level());
    for (int step = 0; step < 10; ++step) { tick(); }
    require(observation(game->level()) == paused, "Bots advanced under Pause");
    mapping("pause-frozen");
    // A normal Restart rebuilds every private bot decision state.
    tap(Key::down); tap(Key::enter); tick(0.0f); tap(Key::up); tap(Key::enter);
    require(stack.depth() == 2 && game->level().state() != LevelState::active,
        "Mixed-roster Pause Restart did not rebuild the actual match");
    require(game->session().round() == 1 && game->session().team_a_wins() == 0 &&
        game->session().team_b_wins() == 0, "Pause Restart moved the round-one ledger");
    mapping("restart"); live();
    stack.notify_activation(false);
    const auto unfocused = simulation();
    for (int step = 0; step < 8; ++step) { tick(); }
    require(simulation() == unfocused, "Bots advanced while focus was lost");
    stack.notify_activation(true); tick(0.0f);
    require(game->resume_remaining() > 0, "Mixed match lost the focus resume count-in");
    for (int step = 0; step < 180 && game->resume_remaining() > 0; ++step) { tick(); }
    require(simulation() == unfocused, "Bots advanced during focus count-in");
    mapping("focus-count-in-frozen");

    painter_scripted_pads::states[3].connected = false; tick();
    require(stack.depth() == 3, "Missing participating human did not pause mixed match");
    const auto disconnected = observation(game->level());
    for (int step = 0; step < 8; ++step) { tick(); }
    require(observation(game->level()) == disconnected, "Bots advanced while a human was disconnected");
    painter_scripted_pads::states[3].connected = true; tick(); tick();
    require(stack.depth() == 3, "A returned human auto-resumed a mixed match");
    tap(Key::enter);
    require(stack.depth() == 3, "Returned pause owner failed to retain input ownership");
    const auto pad_tap = [&](GamepadButton button)
    {
        auto& buttons = painter_scripted_pads::states[3].buttons;
        buttons = static_cast<std::uint16_t>(1u << static_cast<unsigned int>(button)); tick(0.0f);
        buttons = 0; tick(0.0f);
    };
    pad_tap(GamepadButton::b);
    require(stack.depth() == 2 && game->resume_remaining() > 0, "Mixed match could not explicitly resume");
    for (int step = 0; step < 180 && game->resume_remaining() > 0; ++step) { tick(); }
    mapping("human-reconnected");

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        const auto before = observation(game->level());
        const Frame image = stack_frame(app);
        require(image.pixels == stack_frame(app).pixels && observation(game->level()) == before,
            "Frozen mixed-roster drawing advanced bots, identity or pixels");
        save(image, output / (std::string("bot-players-runtime-") + std::to_string(size.y) + "-detail.ppm"));
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    // Inspection captures only: drawing at each size, then back to 720p.
    const auto capture = [&](const std::string& name)
    {
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
        {
            app.resolution_manager()->set_resolution_exactly(size);
            std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            save(stack_frame(app), output / ("bot-players-" + name + "-" + std::to_string(size.y) + "-detail.ppm"));
        }
        app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
        std::ignore = app.renderer()->window_size_changed(1280, 720);
    };
    // C6 (DES#3): the GameLevel's ledger across its own continuations. Pause
    // Restart replays the round in progress on the same ledger; Next Round and
    // Replay Draw keep it; Play Series Again starts a fresh best-of-5.
    const auto ledger = [&](int round, int a_wins, int b_wins)
    {
        const Session& session = game->session();
        return session.round() == round && session.team_a_wins() == a_wins &&
            session.team_b_wins() == b_wins;
    };
    bool restarted_after_win = false, series_again = false;
    for (int round = 1; !series_again; ++round)
    {
        require(round <= 12, "Mixed best-of-5 did not complete within twelve rounds");
        const int a_before = game->session().team_a_wins(), b_before = game->session().team_b_wins();
        require(ledger(round, a_before, b_before) && !game->session().complete(),
            "Mixed series round number drifted from its continuations");
        if (!restarted_after_win && a_before + b_before > 0)
        {
            live();
            for (int step = 0; step < 20; ++step) { tick(); }
            tap(Key::escape); tick(0.0f);
            require(stack.depth() == 3, "Mixed series could not open Pause after a won round");
            tap(Key::down); tap(Key::enter); tick(0.0f); tap(Key::up); tap(Key::enter);
            require(stack.depth() == 2 && game->level().state() != LevelState::active,
                "Mid-series Pause Restart did not rebuild the actual match");
            require(ledger(round, a_before, b_before) && !game->session().complete(),
                "Pause Restart after a won round changed the round or the wins");
            mapping("series-restart");
            restarted_after_win = true;
        }
        if (game->level().intro_playing()) { tap(Key::enter); }
        for (int step = 0; step < 3000 && stack.depth() == 2; ++step) { tick(); }
        require(stack.depth() == 3 && game->level().state() == LevelState::finished,
            "Mixed match did not reach Results");
        const Session finished = game->session();
        require(finished.round() == round &&
            finished.team_a_wins() + finished.team_b_wins() - a_before - b_before ==
            (finished.round_winner() == PlayerTeam::none ? 0 : 1),
            "Results did not count the finished round exactly once");
        mapping(round == 1 ? "round-one-results" : "round-results");
        for (int step = 0; step < 360; ++step) { tick(); }
        if (round == 1) { capture("results"); }
        tap(Key::enter); tick(0.0f); tap(Key::enter);
        for (int step = 0; step < 180 && stack.depth() == 3; ++step) { tick(); }
        require(stack.depth() == 2, "Mixed series did not continue from End");
        if (finished.complete())
        {
            require(ledger(1, 0, 0) && game->session().is_series() && !game->session().complete() &&
                game->session().score_string(presentation.menu_text().results) ==
                Session(5).score_string(presentation.menu_text().results),
                "Play Series Again did not reset the GameLevel to a fresh best-of-5 at round 1, 0-0");
            mapping("series-again-rebuild");
            series_again = true;
        }
        else
        {
            require(ledger(round + 1, finished.team_a_wins(), finished.team_b_wins()),
                "Next Round did not advance the GameLevel's round with its wins kept");
            mapping("next-round-rebuild");
        }
    }
    require(restarted_after_win, "Mixed series never reached a Pause Restart after a won round");
    require(evidence.good(), "Bot runtime evidence failed to write");
    require(preferences.last_match_setup() &&
        remembered_setup_matches(*preferences.last_match_setup(), settings),
        "Restart/series changed remembered bot metadata");
    stack.clear();

    // One human pane beside two bots, captured live for label inspection.
    MatchSetup solo; solo.set_player_count(3);
    solo.set_player_bot(1, BotDifficulty::normal); solo.set_player_bot(2, BotDifficulty::hard);
    solo.begin_input_claims(input_slots::keyboard_and_mouse);
    solo.set_screen_layout(ScreenLayout::one_player); solo.set_stage("crossfeed"); solo.set_duration(60.0f);
    for (int participant = 0; participant < 3; ++participant)
    {
        solo.set_player_team(participant, participant == 2 ? PlayerTeam::b : PlayerTeam::a);
        solo.set_player_weapon(participant, static_cast<WeaponType>(participant));
    }
    stack.push(std::make_unique<ResumeProbeRoot>());
    auto solo_owned = std::make_unique<GameLevel>(&data, solo, MatchStart::menu);
    game = solo_owned.get(); stack.push(std::move(solo_owned));
    live();
    for (int step = 0; step < 150; ++step) { tick(); }
    require(game->level().view_count() == 1 && game->level().bot_identity(1) == L"P2 - Bot (Normal)" &&
        game->level().bot_identity(2) == L"P3 - Bot (Hard)", "One-pane bot match lost its pane or bot labels");
    capture("one-pane");
    stack.clear();
    std::cout << "PASS actual mixed GameLevel: sparse human panes, bot world labels at720/1080, Pause/Restart, focus count-in, human disconnect/reconnect, a best-of-5 played to Play Series Again with its ledger kept by Pause Restart and Next Round and reset by Play Series Again; one-pane bot match and Results captured\n";
}
