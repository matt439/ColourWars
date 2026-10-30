// Actual GameLevel ownership on the hidden shell. Scripted input and the
// recording default factory replace every physical pad and motor operation.
#if defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
void rumble_owner_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    static_cast<void>(canvas);
    const NestedSaveProfile profile(output / "rumble-owner-profile");
    Save preferences; SettingsSession preferences_session(preferences);
    PlaySettings quiet; quiet.music_volume = quiet.sfx_volume = 0;
    require(preferences_session.set_play_settings_and_save(quiet), "Cannot seed private rumble owner settings");
    GameTuning local = tuning;
    // A longer local hit keeps a positive witness active across each boundary;
    // these are fixture values, not a claim about the authored physical feel.
    const_cast<RumbleProfile&>(local.rumble())[static_cast<size_t>(RumbleEvent::hit)].duration_seconds = 0.5f;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "movement_course" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&local); data.set_presentation(&presentation);
    DebugDirector director(&local, &presentation); data.set_debug(&director);
#if COLOURWARS_DEV_TOOLS
    director.impl()->set_content_directories(content, content);
    director.impl()->set_user_directory(output / "rumble-owner-profile" / "debug");
#else
    static_cast<void>(content);
#endif
    director.attach(&app, &data);
#if COLOURWARS_DEV_TOOLS
    require(director.impl()->execute("intro off").ok, "Cannot suppress fixture intro");
#endif
    painter_rumble::Recording motors;
    painter_rumble::RecordScope recording(motors);
    StateContext& stack = app;
    struct ClearRumbleOwner
    {
        StateContext& stack; Application& app;
        ~ClearRumbleOwner()
        {
            this->stack.clear(); painter_scripted_pads::states = {};
            this->app.gamepads()->poll(); this->app.gamepads()->poll();
        }
    } clear{ stack, app };
    std::ofstream cases(output / "rumble-owner.csv");
    cases << "case,requests,factories,destroyed,all_zero\n";
    size_t case_count = 0;
    const auto zero = [&]
    {
        require(!motors.invalid, "Recording motor sink rejected an intent or overflowed");
        for (const RumbleMotors& state : motors.current)
        {
            if (state.low_frequency != 0.0f || state.high_frequency != 0.0f) { return false; }
        }
        return true;
    };
    const auto note = [&](const char* name)
    {
        cases << name << ',' << motors.count << ',' << motors.factories << ',' << motors.destructions << ',' <<
            (zero() ? "true" : "false") << '\n'; ++case_count;
    };
    const auto nonzero_since = [&](size_t first)
    {
        for (size_t index = first; index < motors.count; ++index)
        {
            const auto& state = motors.calls[index].motors;
            if (state.low_frequency != 0.0f || state.high_frequency != 0.0f) { return true; }
        }
        return false;
    };
    GameLevel* game = nullptr;
    const auto tick = [&](float elapsed = dt)
    {
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto pad = [&](GamepadButton button)
    {
        const auto bit = static_cast<std::uint16_t>(1u << static_cast<unsigned int>(button));
        painter_scripted_pads::states[3].buttons = bit; tick(0.0f);
        painter_scripted_pads::states[3].buttons = 0; tick(0.0f); tick(0.0f);
    };
    const auto launch = [&](float duration)
    {
        stack.clear(); stack.notify_activation(true);
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (Key key : { Key::escape, Key::enter, Key::f10, Key::up, Key::down }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
        painter_scripted_pads::states = {};
        painter_scripted_pads::states[3].connected = true;
        painter_scripted_pads::states[1].connected = true;
        app.gamepads()->poll(); app.gamepads()->poll();
        MatchSetup settings; settings.set_stage("movement_course"); settings.set_duration(duration);
        settings.set_player_count(3); settings.begin_input_claims(3);
        require(settings.claim_input_slot(1) && settings.claim_input_slot(input_slots::keyboard_and_mouse),
            "Cannot form the sparse rumble roster");
        settings.set_screen_layout(ScreenLayout::three_player);
        for (int view = 0; view < 3; ++view)
        {
            settings.set_player_team(view, view == 1 ? PlayerTeam::b : PlayerTeam::a);
            settings.set_player_weapon(view, WeaponType::sprayer);
        }
        stack.push(std::make_unique<ResumeProbeRoot>());
        auto owner = std::make_unique<GameLevel>(&data, settings); game = owner.get();
        stack.push<EndMenuAction>(std::move(owner), [](const EndMenuAction&) {});
    };
    const auto live = [&]
    {
        for (int tick_index = 0; tick_index < 2400 && game->level().state() != LevelState::active; ++tick_index) { tick(); }
        require(stack.depth() == 2 && game->level().state() == LevelState::active, "Rumble fixture did not enter real live play");
    };
    const auto hit = [&](int view)
    {
        Player& player = const_cast<Player&>(game->level().player(view));
        player.force_respawn();
        const PlayerTeam enemy = player.team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
        const auto& values = local.projectile(ProjectileType::spray);
        const ProjectileSpray projectile(RectangleF(0, 0, 10, 10), Vector2F::ZERO, enemy,
            view == 1 ? 3 : 1, Colour::white, app.render_resources(), resolve_projectile_strip(values, *app.render_resources()), &values);
        player.on_contact(projectile, Vector2F::ZERO, 0.0f);
        tick();
    };
    const auto positive = [&](int view, int slot)
    {
        hit(view);
        const RumblePulse& expected = local.rumble()[static_cast<size_t>(RumbleEvent::hit)];
        require(motors.current[static_cast<size_t>(slot)].low_frequency == expected.motors.low_frequency &&
            motors.current[static_cast<size_t>(slot)].high_frequency == expected.motors.high_frequency,
            "Actual damage did not reach the owning physical pad");
    };
    const auto finish_count = [&]
    {
        require(stack.depth() == 2 && game->resume_remaining() > 0.0f, "Real Resume did not start the count-in gate");
        tick(game->resume_remaining() + dt); tick(0.0f);
        require(zero(), "Resume revived stale rumble");
    };

    launch(60.0f); live(); positive(0, 3); note("actual-hit-pad3");
    // A sparse second physical slot is independent of both view0 and pad0.
    for (int count = 0; count < 40; ++count) { tick(); }
    positive(1, 1); note("actual-hit-pad1");
    for (int count = 0; count < 40; ++count) { tick(); }
    const size_t keyboard_begin = motors.count; hit(2);
    require(zero() && !nonzero_since(keyboard_begin), "Keyboard damage reached a motor"); note("keyboard-suppressed");
    positive(0, 3);
    PlaySettings disabled = preferences_session.play_settings(); disabled.pad_rumble[3] = false;
    require(preferences_session.set_play_settings_and_save(disabled), "Cannot disable private rumble slot"); tick(0.0f);
    require(zero(), "Live Off did not immediately stop its pad");
    const size_t off_begin = motors.count; hit(0);
    require(!nonzero_since(off_begin), "Disabled slot emitted a pulse");
    disabled.pad_rumble[3] = true; require(preferences_session.set_play_settings_and_save(disabled), "Cannot restore private rumble slot");
    tick(0.0f); require(zero(), "Re-enable resumed stale events"); note("live-off-on");

    positive(0, 3); pad(GamepadButton::start);
    require(stack.depth() == 3 && zero(), "Actual Pause did not stop rumble");
    pad(GamepadButton::a); finish_count(); positive(0, 3); note("pause-countin-fresh-event");
    stack.notify_activation(false); require(zero(), "Focus loss did not stop rumble");
    stack.notify_activation(true); finish_count(); positive(0, 3); note("focus-countin-fresh-event");
    stack.push(std::make_unique<ResumeProbeRoot>()); require(zero(), "Unrelated stack cover did not stop rumble");
    stack.pop(); tick(0.0f); require(zero(), "Uncover revived a stale pulse"); note("cover-uncover");
    positive(0, 3); painter_scripted_pads::states[3].connected = false; tick(0.0f);
    require(stack.depth() == 3 && zero(), "Disconnect did not stop and pause the real owner");
    painter_scripted_pads::states[3].connected = true; tick(0.0f); pad(GamepadButton::a); finish_count();
    positive(0, 3); note("disconnect-reconnect");

    const size_t factory_before_restart = motors.factories;
    pad(GamepadButton::start); pad(GamepadButton::dpad_down); pad(GamepadButton::a);
    pad(GamepadButton::dpad_up); pad(GamepadButton::a);
    require(stack.depth() == 2 && zero() && motors.factories == factory_before_restart,
        "Restart leaked motors or replaced the containing screen's sink");
    live(); positive(0, 3); note("restart-fresh-policy");
#if COLOURWARS_DEV_TOOLS
    require(director.impl()->execute("pause on").ok && zero(), "Debug pause did not immediately stop rumble");
    hit(0); require(zero(), "Paused Debug clock emitted rumble");
    require(director.impl()->execute("step 1").ok, "Cannot single-step rumble fixture"); tick();
    require(zero(), "A paused single-step emitted rumble");
    require(director.impl()->execute("pause off").ok, "Cannot resume Debug clock"); tick(0.0f);
    require(zero(), "Debug resume replayed a stale event"); note("debug-pause-step");
    positive(0, 3);
    require(director.impl()->execute("bot 1 idle").ok, "Cannot assign fixture bot"); tick(0.0f);
    require(data.debug().drives_slot(3) && zero(), "Bot gate did not stop the owned physical slot");
    const size_t bot_begin = motors.count; hit(0);
    require(!nonzero_since(bot_begin), "Bot-controlled seat emitted rumble");
    require(director.impl()->execute("bot off").ok, "Cannot clear fixture bot"); tick(0.0f);
    require(zero(), "Bot removal replayed stale input"); note("bot-gate");
    positive(0, 3); app.keyboard()->on_key_down(Key::f10); tick(0.0f);
    require(stack.depth() > 2 && zero(), "Debug overlay hook did not stop rumble in its opening update");
    app.keyboard()->on_key_up(Key::f10); tick(0.0f);
    app.keyboard()->on_key_down(Key::escape); tick(0.0f); app.keyboard()->on_key_up(Key::escape); tick(0.0f);
    require(stack.depth() == 2 && zero(), "Debug overlay close revived a pulse"); note("debug-overlay");
    require(director.impl()->execute("seed 271").ok &&
        director.impl()->execute("record rumble-owner 360").ok, "Cannot arm isolated owner recording");
    tick(0.0f);
    for (int count = 0; count < 370; ++count) { tick(); }
    require(game->level().state() == LevelState::active, "Bounded owner recording did not reach live play");
    positive(0, 3);
    require(director.impl()->execute("play rumble-owner").ok && data.debug().drives_slot(3),
        "Replay did not expose its armed physical-slot exclusion");
    tick(0.0f); require(zero(), "Armed replay rebuild retained an old physical pulse");
    live(); require(data.debug().drives_slot(3), "Owner replay ended before its active gate witness");
    const size_t replay_begin = motors.count; hit(0);
    require(!nonzero_since(replay_begin), "Replay-controlled actual damage reached a motor");
    // Deliberately injected damage is outside the tape. Stop before the next
    // checkpoint: this witnesses the output gate, not replay determinism.
    require(director.impl()->execute("play stop").ok, "Cannot stop the isolated replay gate witness");
    note("armed-and-running-replay-gate");
#endif
    stack.clear(); require(zero() && motors.factories == motors.destructions, "Owner destruction leaked a policy or sink");
    note("owner-destruction");
    launch(0.20f); live(); positive(0, 3);
    for (int count = 0; count < 30 && game->level().state() == LevelState::active; ++count) { tick(); }
    require(game->level().state() != LevelState::active && zero(), "Time-up left an unexpired motor pulse running");
    note("time-up"); stack.clear(); require(zero() && motors.factories == motors.destructions, "Final teardown leaked rumble ownership");
    require(cases.good(), "Cannot write rumble owner receipts");
    std::ofstream summary(output / "rumble-owner-summary.json");
    summary << "{\"outcome\":\"passed\",\"cases\":" << case_count << ",\"requests\":" << motors.count <<
        ",\"factories\":" << motors.factories << ",\"destroyed\":" << motors.destructions <<
        ",\"native_motor_calls\":0,\"limits\":\"actual hidden-shell GameLevel with scripted readers and recording factory; "
#if COLOURWARS_DEV_TOOLS
        "the replay gate witness injects damage then stops before its next checkpoint; "
#else
        "Debug pause, step, bot, overlay and replay gate checks are not built in Release; "
#endif
        "no new replay determinism or physical feel acceptance\"}\n";
    require(summary.good(), "Cannot write rumble owner summary");
    std::cout << "PASS actual GameLevel rumble owner: sparse actual damage, keyboard/Off/On, pause/count-in/focus/cover/connection, "
        "restart/time-up/destruction; Debug pause/step/bot/overlay where compiled. Recording factory only; no native motor calls.\n";
}
#endif
