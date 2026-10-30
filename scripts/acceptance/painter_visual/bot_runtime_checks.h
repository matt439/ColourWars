// Actual GameLevel lifecycle with a bot between two sparse human seats.
void check_playable_bot_runtime(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    std::ignore = content;
    const NestedSaveProfile profile(output / "bot-players-runtime-profile");
    Save preferences; preferences.load_save_file();
    PlaySettings silent = preferences.play_settings(); silent.music_volume = silent.sfx_volume = 0;
    require(preferences.set_play_settings_and_save(silent), "Cannot silence private runtime fixture");
    StageList stages;
    GameData data; data.set_application(&app); data.set_save(&preferences);
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
        const Frame image = resume_stack_frame(app);
        require(image.pixels == resume_stack_frame(app).pixels && observation(game->level()) == before,
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
            save(resume_stack_frame(app), output / ("bot-players-" + name + "-" + std::to_string(size.y) + "-detail.ppm"));
        }
        app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
        std::ignore = app.renderer()->window_size_changed(1280, 720);
    };
    for (int round = 1; round <= 2; ++round)
    {
        if (game->level().intro_playing()) { tap(Key::enter); }
        for (int step = 0; step < 3000 && stack.depth() == 2; ++step) { tick(); }
        require(stack.depth() == 3 && game->level().state() == LevelState::finished,
            "Mixed match did not reach Results");
        mapping(round == 1 ? "round-one-results" : "round-two-results");
        for (int step = 0; step < 360; ++step) { tick(); }
        if (round == 1) { capture("results"); }
        tap(Key::enter); tick(0.0f); tap(Key::enter);
        for (int step = 0; step < 180 && stack.depth() == 3; ++step) { tick(); }
        require(stack.depth() == 2, "Mixed series did not continue from End");
        mapping(round == 1 ? "round-two-rebuild" : "round-three-rebuild");
    }
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
    std::cout << "PASS actual mixed GameLevel: sparse human panes, bot world labels at720/1080, Pause/Restart, focus count-in, human disconnect/reconnect and two Results/series continuations; one-pane bot match and Results captured\n";
}
