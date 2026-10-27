// Between the rounds of an unfinished series, the actual End menu reopens the
// real Weapon and Stage Select pages in its own frame (ROAD#20). Only ordinary
// armed keyboard actions drive them (no scripted pads, so the end-menu leaf
// builds it); P2 is a bot, so the host readies its row.
void series_repick_checks(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const NestedSaveProfile profile(output / "series-repick-profile");
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    StageList stages;
    for (const char* name : { "movement_course", "king_of_the_hill" })
    {
        const auto* info = levels.get(name);
        require(info != nullptr, "Series re-pick needs both fixture stages");
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_level_infos(&levels); data.set_stages(&stages); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearRepick
    {
        StateContext& stack;
        ~ClearRepick() { this->stack.clear(); }
    } clear{ stack };
    const std::array<Key, 7> keys{ Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right, Key::r };
    const auto tick = [&]
    {
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(dt);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(); app.keyboard()->on_key_up(key); tick();
        tick();
    };

    LevelEndInfo victory;
    victory.total_area = 100.0f; victory.team_a_area = 60.0f; victory.team_b_area = 40.0f;
    Session series(3); series.finish_round(victory);
    require(series.is_series() && !series.complete(), "Series re-pick fixture is not between rounds");

    std::optional<EndMenuAction> choice;
    int delivered = 0;
    const auto open = [&]
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : keys) { app.keyboard()->on_key_up(key); }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll();
        input.reset_actions(); choice.reset(); delivered = 0;
        stack.push(std::make_unique<EndMenuProbeRoot>(app, canvas));
        stack.push<EndMenuAction>(std::make_unique<EndMenuInitial>(&context, series),
            [&](const EndMenuAction& result) { choice = result; ++delivered; });
        tick(); tick();
        require(stack.depth() == 2 && !choice, "Series End menu did not open on neutral input");
    };
    const auto still_open = [&](const char* message)
    {
        require(stack.depth() == 2 && !choice && delivered == 0 && !context.music_exit_pending(), message);
    };
    const auto next_round = [&](const char* message)
    {
        require(choice == EndMenuAction::restart && delivered == 1 && stack.depth() == 1, message);
    };
    const auto evidence = [&](const std::string& name)
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer());
        save(read(*app.renderer()), output / ("series-repick-" + name + ".ppm"));
    };

    // A 1v1 series: P1 on the keyboard, P2 a Normal bot.
    settings.set_player_count(2);
    settings.begin_input_claims(input_slots::keyboard_and_mouse);
    settings.set_player_bot(1, BotDifficulty::normal);
    settings.set_stage("movement_course"); settings.set_duration(73.0f); settings.set_best_of(3);
    settings.set_screen_layout(ScreenLayout::one_player);
    settings.set_player_team(0, PlayerTeam::a); settings.set_player_team(1, PlayerTeam::b);
    settings.set_player_weapon(0, WeaponType::sprayer); settings.set_player_weapon(1, WeaponType::sprayer);
    const auto roster_kept = [&](const char* message)
    {
        const auto seats = settings.player_settings();
        require(settings.player_count() == 2 && settings.best_of() == 3 &&
            settings.duration() == std::optional<float>(73.0f) &&
            seats[0].controller == PlayerController::human && seats[0].num == input_slots::keyboard_and_mouse &&
            seats[1].controller == PlayerController::bot && seats[1].difficulty == BotDifficulty::normal &&
            settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::b, message);
    };

    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);

    // Change Weapons, one row under Next Round, opens the tools with the
    // series' current picks; their confirmation goes on to the stage, whose
    // confirmation plays the next round through the End menu's own result.
    open(); evidence("end");
    tap(Key::down); tap(Key::enter);
    still_open("Change Weapons between rounds closed the End frame instead of opening the tools");
    evidence("tools");
    tap(Key::right); tap(Key::enter); // P1 Sniper, ready.
    tap(Key::down); tap(Key::right); tap(Key::right); tap(Key::r); tap(Key::enter); // Bot Roller, +1, ready.
    still_open("Readied tools between rounds skipped the stage page");
    require(settings.player_weapon(0) == WeaponType::sniper && settings.player_weapon(1) == WeaponType::roller &&
        settings.player_handicap(0) == 0 && settings.player_handicap(1) == 1,
        "Re-picked tools or Boost did not reach the series' settings");
    evidence("tools-stage");
    tap(Key::enter); still_open("Locking the stage between rounds started the round early");
    tap(Key::enter);
    next_round("The stage confirmation after re-picked tools did not play the next round");
    require(settings.stage() == "movement_course", "Passing through the stage page changed the stage");
    roster_kept("Re-picking tools changed the series roster, teams or bot");

    // Change Level skips the tools and plays the newly chosen stage.
    open(); tap(Key::down); tap(Key::down); tap(Key::enter);
    still_open("Change Level between rounds closed the End frame instead of opening the stages");
    evidence("stage");
    tap(Key::right); tap(Key::enter); tap(Key::enter);
    next_round("The re-picked stage did not play the next round");
    require(settings.stage() == "king_of_the_hill" && settings.player_weapon(0) == WeaponType::sniper &&
        settings.player_weapon(1) == WeaponType::roller, "Re-picking the stage changed the stage or tools wrongly");
    roster_kept("Re-picking the stage changed the series roster, teams or bot");

    // Back from either page returns to the same End menu, which still plays on.
    open(); tap(Key::down); tap(Key::enter); tap(Key::left); tap(Key::escape);
    still_open("Back from the between-rounds tools left the End frame");
    tap(Key::enter);
    next_round("Back from the tools did not return to the End menu's Next Round");
    require(settings.player_weapon(0) == WeaponType::sniper,
        "Backing out of the tools committed an unconfirmed tool");
    open(); tap(Key::down); tap(Key::down); tap(Key::enter); tap(Key::left); tap(Key::escape);
    still_open("Back from the between-rounds stages left the End frame");
    tap(Key::enter);
    next_round("Back from the stages did not return to the End menu's Next Round");
    require(settings.stage() == "king_of_the_hill", "Backing out of the stages committed an unlocked stage");

    // Exit is the last row; Change Teams is not reachable between rounds.
    open(); tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::enter);
    require(choice == EndMenuAction::exit && delivered == 1 && stack.depth() == 1,
        "Exit was not the row under Change Level between rounds");
    roster_kept("Series re-pick navigation changed the roster");
    std::cout << "PASS series re-pick: actual End menu between rounds opens Weapon then Stage Select in its frame; "
        "re-picked tools and Boost, then the stage, play the next round; Back returns to the End menu without "
        "committing; teams, roster, bot and best-of stay; Exit follows Change Level\n";
}
