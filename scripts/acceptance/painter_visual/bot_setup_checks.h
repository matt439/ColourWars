// Real Join/Team/Weapon pages with ordinary armed keyboard actions. Only the
// reader supplying the additional human pad is scripted.
void check_playable_bot_setup(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    const NestedSaveProfile profile(output / "bot-setup-profile");
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_level_infos(&levels); data.set_stages(&stages); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MenuLevelSettings settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    struct ClearBotSetup
    {
        StateContext& stack;
        ~ClearBotSetup() { this->stack.clear(); painter_scripted_pads::states = {}; }
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
    const auto pad_tap = [&](GamepadButton button)
    {
        auto& buttons = painter_scripted_pads::states[3].buttons;
        buttons = static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); tick();
        buttons = 0; tick(); tick();
    };
    const auto frame = [&]
    {
        app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto reference = [&](int page)
    {
        MenuLevelSettings copy = settings;
        MenuInput other_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext other_context(&data, &other_input, &copy);
        StateContext other;
        if (page == 0) { other.push(std::make_unique<MainMenuJoin>(&other_context)); }
        else if (page == 1) { other.push(std::make_unique<MainMenuTeamSelect>(&other_context)); }
        else if (page == 2) { other.push(std::make_unique<MainMenuWeaponSelect>(&other_context)); }
        else { other.push(std::make_unique<MainMenuStageSelect>(&other_context)); }
        app.renderer()->begin_frame(); other.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto expect_page = [&](int page, const char* message)
    {
        const Frame actual = frame(); const Frame expected = reference(page);
        if (actual.pixels != expected.pixels)
        {
            save(actual, output / "bot-setup-failure-actual.ppm");
            save(expected, output / "bot-setup-failure-expected.ppm");
        }
        require(actual.pixels == expected.pixels, message);
    };
    const auto open = [&](int count, bool claimed = false)
    {
        stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : keys) { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = claimed;
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll();
        settings = MenuLevelSettings(); settings.set_player_count(count);
        settings.begin_input_claims(input_slots::keyboard_and_mouse);
        if (claimed) { require(settings.claim_input_slot(3), "Cannot claim bot setup's sparse human seat"); }
        settings.set_stage("movement_course"); settings.set_duration(73.0f);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : count == 2 ? ScreenLayout::two_player :
            count == 3 ? ScreenLayout::three_player : ScreenLayout::four_player);
        for (int seat = 0; seat < count; ++seat)
        {
            settings.set_player_team(seat, PlayerTeam::a);
            settings.set_player_weapon(seat, WeaponType::sprayer);
        }
        input.reset_actions(); stack.push(std::make_unique<MainMenuJoin>(&context)); tick(); tick();
    };
    const auto bot = [&](int seat, BotDifficulty difficulty)
    {
        const auto& player = settings.player_settings()[static_cast<size_t>(seat)];
        require(player.controller == PlayerController::bot && player.difficulty == difficulty && player.num == -1,
            "Join did not retain the selected bot kind/difficulty without a device");
    };

    std::ofstream records(output / "bot-setup.csv");
    records << "check,width,height,passed\n";
    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        open(2); expect_page(0, "Neutral Join did not open for two participants");
        tap(Key::right); bot(1, BotDifficulty::normal);
        require(settings.human_count() == 1 && settings.screen_layout() == ScreenLayout::one_player,
            "One human and one bot did not choose one human pane");
        tap(Key::left); bot(1, BotDifficulty::easy);
        tap(Key::left);
        require(settings.bot_count() == 0 && settings.player_settings()[1].num == -1 &&
            settings.screen_layout() == ScreenLayout::two_player, "Bot to Human kept a device or bot layout");
        tap(Key::right); bot(1, BotDifficulty::normal);
        tap(Key::right); bot(1, BotDifficulty::hard);
        tap(Key::right); require(settings.bot_count() == 0, "Hard did not return to an unclaimed Human");
        require(settings.player_settings()[0].controller == PlayerController::human &&
            settings.player_settings()[0].num == input_slots::keyboard_and_mouse, "Join changed the human host");

        // Inactive navigation and an action held across activation are discarded.
        stack.notify_activation(false); tap(Key::right);
        require(settings.bot_count() == 0, "Inactive Join changed a bot seat");
        app.keyboard()->on_key_down(Key::enter); tick(); stack.notify_activation(true); tick();
        app.keyboard()->on_key_up(Key::enter); tick(); expect_page(0, "Focus return reused held Confirm");

        open(3); tap(Key::right); bot(1, BotDifficulty::normal);
        tap(Key::down); tap(Key::right); tap(Key::right); bot(2, BotDifficulty::hard);
        save(frame(), output / ("bot-setup-join-" + std::to_string(size.x) + "x" + std::to_string(size.y) + ".ppm"));
        tap(Key::enter); expect_page(1, "Completed human/bot Join did not open Team Select");
        tap(Key::enter); tap(Key::down); tap(Key::enter); tap(Key::down); tap(Key::enter);
        require(settings.player_team(2) == PlayerTeam::a, "Team choices committed despite leaving an empty team");
        tap(Key::right); tap(Key::right); tap(Key::enter);
        require(settings.player_team(0) == PlayerTeam::a && settings.player_team(1) == PlayerTeam::a &&
            settings.player_team(2) == PlayerTeam::b, "Host did not commit independently readied bot teams");
        expect_page(2, "All readied bot teams did not open Weapon Select");
        save(frame(), output / ("bot-setup-tools-" + std::to_string(size.x) + "x" + std::to_string(size.y) + ".ppm"));

        tap(Key::right); tap(Key::enter); // P1 Sniper, ready.
        tap(Key::down); tap(Key::right); tap(Key::right); tap(Key::enter); // P2 Roller, ready.
        tap(Key::down); tap(Key::right); tap(Key::right); tap(Key::right); // P3 Mister.
        tap(Key::up); tap(Key::escape); tap(Key::right); // P2 unready, change to Mister.
        tap(Key::down); tap(Key::enter);
        require(settings.player_weapon(0) == WeaponType::sprayer,
            "A bot Back did not require that row to be readied again");
        tap(Key::up); tap(Key::enter);
        require(settings.player_weapon(0) == WeaponType::sniper && settings.player_weapon(1) == WeaponType::mister &&
            settings.player_weapon(2) == WeaponType::mister, "Host tool changes affected the wrong participant");
        expect_page(3, "All readied human/bot tools did not open Arena Select");
        bot(1, BotDifficulty::normal); bot(2, BotDifficulty::hard);
        require(settings.screen_layout() == ScreenLayout::one_player && settings.duration() == std::optional<float>(73.0f),
            "Setup changed human views or the chosen duration");

        // A joined human keeps its own row, and P1's row movement skips it.
        open(4, true); tap(Key::right); bot(2, BotDifficulty::normal);
        tap(Key::down); tap(Key::right); bot(3, BotDifficulty::normal);
        tap(Key::up); tap(Key::right); bot(2, BotDifficulty::hard);
        require(settings.player_settings()[1].controller == PlayerController::human && settings.player_settings()[1].num == 3 &&
            settings.screen_layout() == ScreenLayout::two_player, "Host converted or reassigned a joined human");
        tap(Key::enter); expect_page(1, "Sparse two-human/two-bot roster did not leave Join");
        painter_scripted_pads::states[3].connected = false; tick();
        tap(Key::enter); tap(Key::down); tap(Key::right); tap(Key::enter);
        require(settings.player_team(2) == PlayerTeam::a, "Missing human allowed bot setup to commit");
        painter_scripted_pads::states[3].connected = true; tick(); tick();
        tap(Key::enter); // P1 ready.
        pad_tap(GamepadButton::dpad_right); pad_tap(GamepadButton::dpad_right); pad_tap(GamepadButton::a); // P2 B.
        tap(Key::down); tap(Key::enter); // P3 A.
        tap(Key::down); tap(Key::enter); // P4 A.
        expect_page(2, "Independent sparse human and host bot rows did not all ready");
        require(settings.player_team(1) == PlayerTeam::b && settings.player_team(2) == PlayerTeam::a,
            "Human input was routed into a bot's row");
        records << "setup-routing," << size.x << ',' << size.y << ",1\n";
    }
    open(1); tap(Key::right); tap(Key::down); tap(Key::left);
    require(settings.bot_count() == 0 && settings.human_count() == 1, "Solo practice gained a bot");
    tap(Key::enter); expect_page(2, "Bot-free solo practice did not retain its direct tool flow");
    std::cout << "PASS playable bot setup: native720/1080 Join Human/Normal/Easy/Hard, human layouts and host protection; "
        "P1 and bot team/tool readiness, empty-team refusal, row Back/reconfirm, sparse independent human input, "
        "inactive/disconnected gating, metadata continuity and bot-free solo practice\n";
}
