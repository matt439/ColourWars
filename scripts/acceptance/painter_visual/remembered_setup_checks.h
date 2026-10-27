// One real GameMenu flow, then a separate process reads the same private save.
// Only input snapshots are scripted; no production state/test getter is added.
std::string remembered_setup_bytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "Cannot read remembered-setup evidence");
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

std::string remembered_setup_section(const std::filesystem::path& path)
{
    const std::string bytes = remembered_setup_bytes(path);
    rapidjson::Document json; json.Parse(bytes.c_str());
    require(!json.HasParseError() && json.IsObject() && json.HasMember("last_match"),
        "The private save has no valid last_match section");
    rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    require(json["last_match"].Accept(writer), "Cannot retain the remembered setup section");
    return { buffer.GetString(), buffer.GetSize() };
}

bool remembered_setup_matches(const LastMatchSetup& actual, const MenuLevelSettings& expected)
{
    if (actual.players.size() != static_cast<size_t>(expected.player_count()) ||
        actual.stage != expected.stage() || actual.layout != expected.screen_layout() ||
        actual.duration != expected.duration() || actual.best_of != expected.best_of()) { return false; }
    for (int seat = 0; seat < expected.player_count(); ++seat)
    {
        const auto& player = actual.players[static_cast<size_t>(seat)];
        const auto& wanted = expected.player_settings()[static_cast<size_t>(seat)];
        if (player.team != wanted.team || player.weapon != wanted.weapon ||
            player.controller != wanted.controller || player.difficulty != wanted.difficulty) { return false; }
    }
    return true;
}

void remembered_setup_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output, bool reader)
{
    const auto root = output / "remembered-setup-profile";
    const NestedSaveProfile profile(root);
    const auto save_path = root / "ColourWars/save_data.json";
    const auto completed = output / "remembered-setup-writer.complete";
    require(!reader || std::filesystem::is_regular_file(completed), "The separate reader needs a completed writer process");
    std::unique_ptr<Save> preferences = std::make_unique<Save>();
    preferences->load_save_file();
    if (!reader)
    {
        PlaySettings play = preferences->play_settings();
        play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
        require(preferences->set_play_settings_and_save(play), "Cannot seed private remembered-setup preferences");
    }
    StageList stages;
    for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course" })
    {
        const auto* info = levels.get(name);
        stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() });
    }
    GameData data; data.set_application(&app); data.set_save(preferences.get()); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
#if COLOURWARS_DEV_TOOLS
    DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(root / "unused-source", content);
    impl.set_user_directory(root / "debug"); impl.set_script_directories({ root / "debug" });
    director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / (reader ? "remembered-setup-reader-commands.log" :
        "remembered-setup-commands.log")).string()), "Cannot retain remembered-setup command transcript");
#else
    std::ignore = content;
#endif
    StateContext& stack = app;
    struct ClearRemembered { StateContext& stack; ~ClearRemembered() { this->stack.clear(); } } clear{ stack };
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const auto bit = [](GamepadButton button)
        { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto tick = [&](float elapsed = 0.0f)
        { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](int slot, GamepadButton button, bool down)
    {
        if (slot == input_slots::keyboard_and_mouse)
        {
            const Key key = button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape :
                button == GamepadButton::dpad_up ? Key::up : button == GamepadButton::dpad_down ? Key::down :
                button == GamepadButton::dpad_left ? Key::left : Key::right;
            if (down) { app.keyboard()->on_key_down(key); } else { app.keyboard()->on_key_up(key); }
        }
        else
        {
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit(button)) :
                static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button)));
        }
    };
    const auto tap = [&](int slot, GamepadButton button, bool settle = true)
    {
        hold(slot, button, true); tick(); hold(slot, button, false); tick();
        if (settle) { tick(0.51f); }
    };
    const auto key_tap = [&](Key key)
        { app.keyboard()->on_key_down(key); tick(dt); app.keyboard()->on_key_up(key); tick(dt); };
    const auto frame = [&] { return main_menu_frame(app); };
    const auto neutral = [&](bool pad_present)
    {
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::enter, Key::space, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {}; painter_scripted_pads::states[3].connected = pad_present;
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll();
    };
    const auto open = [&](bool pad_present)
    {
        stack.clear(); neutral(pad_present); stack.push(std::make_unique<GameMenu>(&data)); tick(); tick();
        require(stack.depth() == 2, "Actual GameMenu did not open its Title page");
    };
    // Reference pages have independently specified choices/claims. Their
    // constructors show the expected unconfirmed state, never the live page's.
    const auto reference = [&](int page, const MenuLevelSettings& choices)
    {
        MenuLevelSettings expected = choices; MenuInput input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext context(&data, &input, &expected); StateContext other;
        other.notify_activation(true);
        if (page == 0) { other.push(std::make_unique<MainMenuModeSelect>(&context)); }
        else if (page == 1) { other.push(std::make_unique<MainMenuPlayerCount>(&context)); }
        else if (page == 2) { other.push(std::make_unique<MainMenuJoin>(&context)); }
        else if (page == 3) { other.push(std::make_unique<MainMenuTeamSelect>(&context)); }
        else if (page == 4) { other.push(std::make_unique<MainMenuWeaponSelect>(&context)); }
        else { other.push(std::make_unique<MainMenuStageSelect>(&context)); }
        other.update(0.0f); app.renderer()->begin_frame(); other.draw(*app.renderer()); return read(*app.renderer());
    };
    std::ofstream evidence(output / (reader ? "remembered-setup-reader.csv" : "remembered-setup.csv"));
    evidence << "check,players,best_of,stage,first_slot,second_slot,save_preserved,outcome\n";
    const auto note = [&](const char* check, const MenuLevelSettings& choices, bool preserved)
    {
        const auto seats = choices.player_settings();
        evidence << check << ',' << choices.player_count() << ',' << choices.best_of() << ',' << choices.stage() << ',' <<
            seats[0].num << ',' << seats[1].num << ',' << (preserved ? "true" : "false") << ",passed\n";
    };
    const auto compare = [&](int page, const MenuLevelSettings& choices, const char* name, bool capture = false)
    {
        const Frame actual = frame(); const Frame wanted = reference(page, choices);
        if (actual.pixels != wanted.pixels)
        {
            save(actual, output / (std::string("remembered-setup-") + name + "-failure-actual.ppm"));
            save(wanted, output / (std::string("remembered-setup-") + name + "-failure-expected.ppm"));
            throw std::runtime_error(std::string("Remembered setup actual page differs from expected fresh choices: ") + name);
        }
        std::srand(6823); const int next = std::rand(); std::srand(6823);
        require(actual.pixels == frame().pixels && std::rand() == next, "Remembered setup frozen drawing changed pixels or RNG");
        if (capture)
        {
            save(foreground_crop(actual, RectangleI(80, 120, 1040, 480), 0), output /
                (std::string("remembered-setup-") + (reader ? "reader-" : "writer-") + name + "-detail.ppm"));
        }
    };
    MenuLevelSettings seed; seed.set_player_count(2); seed.begin_input_claims(3);
    require(seed.claim_input_slot(input_slots::keyboard_and_mouse), "Cannot seed sparse remembered-setup roster");
    seed.set_screen_layout(ScreenLayout::two_player); seed.set_best_of(5); seed.set_duration(0.25f);
    seed.set_stage("king_of_the_hill"); seed.set_player_team(0, PlayerTeam::b); seed.set_player_team(1, PlayerTeam::a);
    seed.set_player_weapon(0, WeaponType::sprayer); seed.set_player_weapon(1, WeaponType::mister);
    MenuLevelSettings chosen = seed; chosen.set_best_of(3); chosen.set_stage("movement_course");
    chosen.set_player_weapon(0, WeaponType::sniper); chosen.set_player_weapon(1, WeaponType::bucket);

    if (reader)
    {
        require(preferences->last_match_setup() && remembered_setup_matches(*preferences->last_match_setup(), chosen),
            "A separate process did not load the writer's resolved preferences");
        const std::string before = remembered_setup_bytes(save_path);
        MenuLevelSettings expected = preferences->last_match_setup()->restore(stages);
        for (const auto& player : expected.player_settings()) { require(player.num == -1, "Disk reload restored a device claim"); }
        open(false); tap(4, GamepadButton::a); tap(4, GamepadButton::a);
        compare(0, expected, "mode-restored"); tap(4, GamepadButton::a);
        compare(1, expected, "count-restored"); tap(4, GamepadButton::a);
        expected.begin_input_claims(4); compare(2, expected, "missing-pad-unclaimed", true);
        tap(4, GamepadButton::a); compare(2, expected, "join-still-waiting");
        painter_scripted_pads::states[3].connected = true; tick(); tick();
        compare(2, expected, "reconnected-still-unclaimed");
        tap(3, GamepadButton::a); require(expected.claim_input_slot(3), "Cannot specify the fresh sparse rejoin");
        compare(2, expected, "fresh-claims", true); tap(4, GamepadButton::a);
        compare(3, expected, "teams-unconfirmed", true);
        tap(4, GamepadButton::a);
        require(frame().pixels != reference(4, expected).pixels, "Remembered team readiness skipped the second player's confirmation");
        tap(3, GamepadButton::a); compare(4, expected, "weapons-unconfirmed");
        tap(4, GamepadButton::a);
        require(frame().pixels != reference(5, expected).pixels, "Remembered weapon readiness skipped the second player's confirmation");
        tap(3, GamepadButton::a); compare(5, expected, "arena-unconfirmed", true);
        tap(4, GamepadButton::a); tap(4, GamepadButton::b); compare(5, expected, "arena-unlocked");
        tap(4, GamepadButton::dpad_left); tap(4, GamepadButton::b);
        compare(4, expected, "arena-edit-cancelled");
        require(remembered_setup_bytes(save_path) == before, "Reloading, joining, reconfirming or cancelling wrote remembered preferences");
        note("separate-process-reload-fresh-claims-confirmations-cancel", expected, true);
        require(evidence.good(), "Remembered reader evidence write failed");
        std::cout << "PASS remembered setup separate-process disk reload, seeded Mode/Count/Team/Weapon/Stage, fresh sparse claims and confirmations, reconnect without autojoin and cancelled arena edit\n";
        return;
    }

    // Normal setup restores bots while requiring fresh human claims.
    {
        MenuLevelSettings mixed = seed; mixed.set_player_bot(1, BotDifficulty::hard);
        mixed.set_screen_layout(ScreenLayout::one_player);
        require(preferences->remember_match_setup_and_save(LastMatchSetup::capture(mixed)),
            "Cannot seed the private bot setup");
        const std::string bot_save = remembered_setup_bytes(save_path);
        MenuLevelSettings expected = preferences->last_match_setup()->restore(stages);
        open(false); tap(4, GamepadButton::a); tap(4, GamepadButton::a);
        compare(0, expected, "bot-restored-mode"); tap(4, GamepadButton::a);
        compare(1, expected, "bot-restored-count", true);
        tap(4, GamepadButton::a); expected.begin_input_claims(4);
        compare(2, expected, "bot-restored-join", true);
        require(expected.input_roster_complete() && expected.human_count() == 1 &&
            expected.bot_count() == 1, "Remembered bot needs a spare controller");
        tap(4, GamepadButton::a); compare(3, expected, "bot-restored-teams", true);
        Save retained; retained.load_save_file();
        require(remembered_setup_bytes(save_path) == bot_save && preferences->last_match_setup() &&
            remembered_setup_matches(*preferences->last_match_setup(), mixed) && retained.last_match_setup() &&
            remembered_setup_matches(*retained.last_match_setup(), mixed),
            "Restoring the bot setup changed saved controller/difficulty metadata");
        note("bot-metadata-restored-no-spare-controller", expected, true);
        std::cout << "PASS remembered bots restored through actual GameMenu Mode/Count/Join/Team with fresh human claim and unchanged saved metadata\n";
        stack.clear();
    }

    require(preferences->remember_match_setup_and_save(LastMatchSetup::capture(seed)), "Cannot seed the private prior match choices");
    const std::string prior = remembered_setup_bytes(save_path);
    const auto unchanged = [&] { require(remembered_setup_bytes(save_path) == prior, "Uncommitted normal menu choices changed the prior save"); };
    open(true); tap(3, GamepadButton::a); tap(3, GamepadButton::a); compare(0, seed, "seeded-mode");
    tap(3, GamepadButton::dpad_up); tap(3, GamepadButton::a);
    MenuLevelSettings expected = seed; expected.set_best_of(3); compare(1, expected, "seeded-count");
    tap(3, GamepadButton::a); expected.begin_input_claims(3); compare(2, expected, "explicit-join", true);
    tap(3, GamepadButton::a); compare(2, expected, "no-auto-claim");
    tap(4, GamepadButton::a); require(expected.claim_input_slot(4), "Cannot specify the second explicit claim");
    tap(3, GamepadButton::a); compare(3, expected, "fresh-team-confirmation", true);
    tap(3, GamepadButton::dpad_left); tap(3, GamepadButton::dpad_right); unchanged();
    tap(3, GamepadButton::a); tap(4, GamepadButton::a); compare(4, expected, "fresh-weapon-confirmation");
    tap(3, GamepadButton::dpad_right); tap(3, GamepadButton::b);
    compare(3, expected, "provisional-weapon-cancelled"); unchanged();
    tap(3, GamepadButton::a); tap(4, GamepadButton::a); compare(4, expected, "weapon-reconfirmation");
    tap(3, GamepadButton::dpad_right); tap(4, GamepadButton::dpad_right);
    tap(3, GamepadButton::a); tap(4, GamepadButton::a);
    expected.set_player_weapon(0, WeaponType::sniper); expected.set_player_weapon(1, WeaponType::bucket);
    compare(5, expected, "arena-browse"); tap(3, GamepadButton::dpad_right); tap(3, GamepadButton::b);
    compare(4, expected, "arena-cancel-preserves-tools"); unchanged();
    tap(3, GamepadButton::a); tap(4, GamepadButton::a); compare(5, expected, "arena-reopened");
    for (int step = 0; step < 3; ++step) { tap(3, GamepadButton::dpad_right); }
    expected.set_stage("movement_course"); compare(5, expected, "chosen-arena", true); unchanged();
    tap(3, GamepadButton::a); unchanged(); note("first-arena-confirm-does-not-save", expected, true);
    tap(3, GamepadButton::a, false);
    for (int step = 0; step < 600 && remembered_setup_bytes(save_path) == prior; ++step) { tick(dt); }
    require(preferences->last_match_setup() && remembered_setup_matches(*preferences->last_match_setup(), chosen) &&
        remembered_setup_bytes(save_path) != prior && app.viewport_manager()->layout() == ScreenLayout::two_player,
        "The actual GameMenu normal start did not save the final resolved choices");
#if COLOURWARS_DEV_TOOLS
    require(impl.in_match(), "The normal GameMenu callback did not create a real attached match");
#endif
    const std::string committed = remembered_setup_section(save_path);
    const auto preserved = [&]
    {
        require(remembered_setup_section(save_path) == committed && preferences->last_match_setup() &&
            remembered_setup_matches(*preferences->last_match_setup(), chosen), "Restart, series or developer work overwrote the remembered user setup");
    };
    note("actual-normal-menu-start-saved", chosen, false);
    // Escape can pause the split countdown after Enter skips the story; no
    // inaccessible live GameLevel pointer or private production seam is used.
    key_tap(Key::enter); key_tap(Key::escape); tick();
    require(stack.depth() == 3, "The actual normal match did not open Pause");
    key_tap(Key::down); key_tap(Key::enter); tick(); key_tap(Key::up); key_tap(Key::enter);
    require(stack.depth() == 2, "The actual Pause Restart did not return to its match"); preserved();
    note("pause-restart-preserves-setup", chosen, true);
    Session session(3); LevelEndInfo drawn;
    for (int round = 1; round <= 2; ++round)
    {
        key_tap(Key::enter);
        for (int step = 0; step < 3000 && stack.depth() == 2; ++step) { tick(dt); }
        require(stack.depth() == 3, "The remembered series did not reach Results"); preserved();
        const auto& look = presentation.results_menu();
        const int ready = static_cast<int>((look.team_fill_delay + look.team_fill_time + look.show_results_delay + 1.0f) / dt) + 4;
        for (int step = 0; step < ready; ++step) { tick(dt); }
        key_tap(Key::enter); require(stack.depth() == 3, "The remembered series did not reach End");
        session.finish_round(drawn);
        MenuInput end_input(app.gamepads(), app.keyboard(), &tuning); MenuContext end_context(&data, &end_input, &chosen);
        StateContext end_stack; end_stack.push(std::make_unique<EndMenuInitial>(&end_context, session));
        app.renderer()->begin_frame(); end_stack.draw(*app.renderer()); const Frame wanted = read(*app.renderer());
        const auto& end = presentation.end_menu();
        const Vector2F extent = app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(
            presentation.menu().item_font), session.continue_string(presentation.menu_text().end));
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        const RectangleI card = countdown_crop_bounds(camera.calculate_view_rectangle(RectangleF(end.layout.position, extent)));
        require(foreground_crop(frame(), card, 0).pixels == foreground_crop(wanted, card, 0).pixels,
            "The real series lost its expected ongoing Replay Draw card"); preserved();
        if (round == 1)
        {
            key_tap(Key::enter); require(stack.depth() == 2, "Replay Draw did not continue the actual series");
            session.next_round(); preserved(); note("series-continuation-preserves-setup", chosen, true);
        }
    }
    const StageRecord* record = preferences->records().find("movement_course");
    require(record && record->versus_played == 2 && record->draws == 2, "The real series did not finish and record exactly two drawn rounds");
    preserved(); note("series-records-independent", chosen, true); stack.clear();

#if COLOURWARS_DEV_TOOLS
    const auto command = [&](const std::string& text)
    {
        const DebugStatus status = impl.execute(text);
        if (!status.ok) { std::cerr << "Remembered setup command " << text << ": " << status.message << '\n'; }
        require(status.ok, "A remembered-setup developer command failed");
    };
    open(true);
    std::filesystem::create_directories(root / "debug");
    { std::ofstream script(root / "debug/remembered_start.cfg");
        script << "seed 7\nplayers 4 paint\nstage close_quarters\n"; require(script.good(), "Cannot write private developer launch script"); }
    const std::string before_developer = remembered_setup_bytes(save_path);
    command("exec remembered_start");
    for (int step = 0; step < 600 && !impl.in_match(); ++step) { tick(dt); }
    require(impl.in_match(), "The real developer script did not start through GameMenu");
    require(impl.bot_for_seat(2, -1) == "paint" && impl.bot_for_seat(3, -1) == "paint",
        "The sparse developer launch did not install bots on its device-less seats");
    preserved(); require(remembered_setup_bytes(save_path) == before_developer, "Developer launch wrote the private user save");
    command("record remembered_setup 4");
    const auto replay = root / "ColourWars/replays/remembered_setup.json";
    for (int step = 0; step < 1500 && (!std::filesystem::is_regular_file(replay) || impl.recording()); ++step) { tick(dt); }
    require(std::filesystem::is_regular_file(replay) && !impl.recording(), "The actual sparse developer recording did not complete");
    const ReplayFile file = ReplayFile::read(replay); const auto seats = file.settings.player_settings();
    require(file.updates == 4 && seats.size() == 4 && seats[0].num == 3 && seats[1].num == 4 &&
        seats[2].num == -1 && seats[3].num == -1, "The real developer replay did not retain sparse pad/keyboard/dummy seats");
    command("menu"); tick(); require(!impl.in_match(), "The real developer match did not return to the menu");
    command("play remembered_setup");
    for (int step = 0; step < 1500 && !impl.replay_finished(); ++step) { tick(dt); }
    require(impl.in_match() && impl.replay_finished(), "The actual replay did not relaunch from GameMenu and finish");
    const auto played = impl.fingerprint();
    require(played && MatchFingerprint::difference(file.final_print, *played).empty(),
        "The actual sparse replay did not reproduce its recorded final state");
    require(std::any_of(impl.log().lines().begin(), impl.log().lines().end(), [](const DebugLogLine& line)
        { return line.text.find("replay remembered_setup: MATCH (") != std::string::npos; }),
        "The actual sparse replay did not report a matched checksum");
    preserved(); require(remembered_setup_bytes(save_path) == before_developer, "Developer record/replay overwrote the private user save");
    note("developer-script-sparse-bots-record-play-excluded", chosen, true);
    command("play stop"); command("bot all off"); command("menu"); tick(); open(true); command("bot 1 paint");
    tap(3, GamepadButton::a); tap(3, GamepadButton::a); tap(3, GamepadButton::a); tap(3, GamepadButton::a);
    tap(4, GamepadButton::a); tap(3, GamepadButton::a); tap(3, GamepadButton::a); tap(4, GamepadButton::a);
    tap(3, GamepadButton::dpad_right); tap(3, GamepadButton::a); tap(4, GamepadButton::a);
    tap(3, GamepadButton::a); tap(3, GamepadButton::a, false);
    for (int step = 0; step < 600 && !impl.in_match(); ++step) { tick(dt); }
    require(impl.in_match() && impl.bot_for_seat(0, 3) == "paint", "The normal menu bot exclusion did not install the explicit first-seat bot");
    preserved(); require(remembered_setup_bytes(save_path) == before_developer,
        "A normal menu launch with a bot controlling an explicitly claimed seat overwrote user setup");
    note("normal-menu-explicit-bot-excluded", chosen, true); stack.clear();
#endif
    Save reloaded; reloaded.load_save_file();
    require(reloaded.last_match_setup() && remembered_setup_matches(*reloaded.last_match_setup(), chosen),
        "A fresh Save object did not reload the final user choices after all exclusions");
    { std::ofstream done(completed); done << committed << '\n'; require(done.good(), "Cannot retain writer completion evidence"); }
    require(evidence.good(), "Remembered setup writer evidence write failed");
    std::ignore = canvas;
    std::cout << "PASS remembered setup actual normal GameMenu start/save, editable/cancelled choices, sparse explicit joining, Pause Restart and two drawn series rounds; Debug script/bot/replay exclusions where compiled; separate reader required for process relaunch\n";
}
