// Real Join/Team/Weapon pages with ordinary armed keyboard actions. Only the
// reader supplying the additional human pad is scripted.
void check_playable_bot_setup(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output)
{
    // ROAD34: drive Player Count's real host claim before the existing Join
    // matrix. The private profile and scripted reader never borrow a player save.
    {
        const auto profile_root = output / "host-profile-boost-profile";
        const NestedSaveProfile host_profile(profile_root);
        Save host_preferences;
        PlaySettings quiet;
        quiet.music_volume = quiet.sfx_volume = 0; quiet.reduce_effects = true;
        require(host_preferences.set_play_settings_and_save(quiet), "ROAD34 could not seed private quiet preferences");
        bool saved = false;
        const int host_id = host_preferences.add_profile_and_save("HOST", 0, saved);
        require(saved, "ROAD34 could not create its private host profile");
        const int guest_id = host_preferences.add_profile_and_save("GUEST", 3, saved);
        require(saved && host_preferences.set_profile_boost_and_save(guest_id, 2),
            "ROAD34 could not create its private guest profile");
        const auto save_path = profile_root / "ColourWars/save_data.json";
        const auto profile_bytes = [&]
        {
            std::ifstream stream(save_path, std::ios::binary);
            require(stream.good(), "ROAD34 private profile file disappeared");
            const std::string text{ std::istreambuf_iterator<char>(stream), {} };
            require(!stream.bad(), "ROAD34 could not read its complete private profile file");
            return text;
        };
        StageList host_stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
        GameData host_data; host_data.set_application(&app); host_data.set_save(&host_preferences);
        host_data.set_level_infos(&levels); host_data.set_stages(&host_stages);
        host_data.set_tuning(&tuning); host_data.set_presentation(&presentation);
        MenuInput host_input(app.gamepads(), app.keyboard(), &tuning);
        MatchSetup host_settings;
        MenuContext host_context(&host_data, &host_input, &host_settings);
        StateContext& host_stack = app;
        require(host_stack.depth() == 0, "ROAD34 needs the painter's empty owned state stack");
        const std::array<Key, 3> host_keys{ Key::enter, Key::escape, Key::r };
        struct ClearHostProfiles
        {
            Application& app;
            StateContext& stack;
            std::array<Key, 3> keys;
            bool active, keyboard_focus, mouse_focus;
            std::array<GamepadState, Gamepads::max_count> previous_pads;
            ~ClearHostProfiles()
            {
                stack.clear();
                painter_scripted_pads::states = previous_pads;
                for (const Key key : keys) app.keyboard()->on_key_up(key);
                app.keyboard()->set_focused(keyboard_focus); app.mouse()->set_focused(mouse_focus);
                app.gamepads()->poll(); app.gamepads()->poll();
                app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll();
                stack.notify_activation(active);
            }
        } clear_host{ app, host_stack, host_keys, host_stack.active(), app.keyboard()->focused(),
            app.mouse()->focused(), painter_scripted_pads::states };
        const auto host_tick = [&]
        {
            app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); host_stack.update(dt);
        };
        const auto host_tap = [&](int slot, GamepadButton button)
        {
            if (slot == input_slots::keyboard_and_mouse)
            {
                const Key key = button == GamepadButton::a ? Key::enter :
                    button == GamepadButton::b ? Key::escape : Key::r;
                app.keyboard()->on_key_down(key); host_tick(); app.keyboard()->on_key_up(key); host_tick();
            }
            else
            {
                auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
                buttons = static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); host_tick();
                buttons = 0; host_tick();
            }
            host_tick();
        };
        const auto host_frame = [&]
        {
            app.renderer()->begin_frame(); host_stack.draw(*app.renderer()); return read(*app.renderer());
        };
        std::ofstream host_records(output / "host-profile-boost.csv");
        require(host_records.good(), "ROAD34 could not create its private observations");
        host_records << "phase,host_slot,players,profile_boost,expected_host,actual_host,expected_guest,actual_guest,private_save_unchanged,passed\n";
        std::string before_host_ui;
        PlayerProfiles profiles_before_host_ui;
        int current_host = -1, current_guest = -1, current_profile_boost = -1;
        const auto host_source_unchanged = [&]
        {
            return profile_bytes() == before_host_ui && host_preferences.profiles() == profiles_before_host_ui &&
                host_preferences.play_settings().music_volume == 0 && host_preferences.play_settings().sfx_volume == 0 &&
                host_preferences.play_settings().reduce_effects;
        };
        const auto host_observe = [&](const char* phase, int expected_host, int expected_guest)
        {
            const int actual_host = host_settings.player_handicap(0);
            const int actual_guest = host_settings.player_count() == 2 ? host_settings.player_handicap(1) : -1;
            const bool source_unchanged = host_source_unchanged();
            const bool passed = actual_host == expected_host && actual_guest == expected_guest && source_unchanged;
            host_records << phase << ',' << current_host << ',' << host_settings.player_count() << ','
                << current_profile_boost << ',' << expected_host << ',' << actual_host << ',' << expected_guest << ','
                << actual_guest << ',' << source_unchanged << ',' << passed << '\n';
            host_records.flush();
            require(host_records.good(), "ROAD34 could not retain its complete observation");
            require(passed, "ROAD34 actual setup did not retain its literal profile Boost and private save");
        };
        const auto host_expect_page = [&](int page, int expected_host, int expected_guest)
        {
            MatchSetup expected = host_settings;
            expected.set_player_handicap(0, expected_host);
            if (expected.player_count() == 2) expected.set_player_handicap(1, expected_guest);
            MenuInput expected_input(app.gamepads(), app.keyboard(), &tuning);
            MenuContext expected_context(&host_data, &expected_input, &expected);
            StateContext expected_stack;
            if (page == 0) expected_stack.push(std::make_unique<MainMenuJoin>(&expected_context));
            else if (page == 1) expected_stack.push(std::make_unique<MainMenuTeamSelect>(&expected_context));
            else if (page == 2) expected_stack.push(std::make_unique<MainMenuWeaponSelect>(&expected_context));
            else expected_stack.push(std::make_unique<MainMenuStageSelect>(&expected_context));
            const Frame actual = host_frame();
            app.renderer()->begin_frame(); expected_stack.draw(*app.renderer()); const Frame wanted = read(*app.renderer());
            if (actual.pixels != wanted.pixels)
            {
                save(actual, output / "host-profile-failure-actual.ppm");
                save(wanted, output / "host-profile-failure-expected.ppm");
            }
            require(actual.pixels == wanted.pixels, "ROAD34 actual setup page differs from its literal Boost reference");
        };
        const auto host_open = [&](int slot, int count, int boost, int previous_boost, bool wearing, bool guest_wearing = false)
        {
            host_stack.clear(); host_stack.notify_activation(true);
            app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
            for (const Key key : host_keys) app.keyboard()->on_key_up(key);
            painter_scripted_pads::states = {};
            for (int pad = 0; pad < input_slots::pad_count; ++pad)
                require(host_preferences.wear_profile(pad, -1), "ROAD34 could not clear a private profile wearer");
            require(host_preferences.set_profile_boost_and_save(host_id, boost), "ROAD34 could not seed a private Boost");
            if (wearing) require(host_preferences.wear_profile(slot, host_id), "ROAD34 could not wear the host profile");
            else if (slot == input_slots::keyboard_and_mouse)
                require(host_preferences.wear_profile(0, host_id), "ROAD34 could not seed its unrelated-pad control");
            current_host = slot; current_guest = slot == 3 ? 0 : 3; current_profile_boost = boost;
            if (guest_wearing) require(host_preferences.wear_profile(current_guest, guest_id), "ROAD34 could not wear the guest profile");
            if (input_slots::is_pad(slot)) painter_scripted_pads::states[static_cast<size_t>(slot)].connected = true;
            if (count == 2) painter_scripted_pads::states[static_cast<size_t>(current_guest)].connected = true;
            app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
            app.mouse()->poll(); app.mouse()->poll();
            host_settings = MatchSetup(); host_settings.set_player_count(count);
            host_settings.set_player_handicap(0, previous_boost);
            host_settings.set_stage("movement_course"); host_settings.set_duration(73.0f);
            for (int seat = 0; seat < count; ++seat)
            {
                host_settings.set_player_team(seat, seat == 0 ? PlayerTeam::a : PlayerTeam::b);
                host_settings.set_player_weapon(seat, WeaponType::sprayer);
                if (seat == 1) host_settings.set_player_handicap(seat, 1);
            }
            require(host_settings.player_settings()[0].num == -1, "ROAD34 fixture claimed its host before Player Count");
            before_host_ui = profile_bytes(); profiles_before_host_ui = host_preferences.profiles();
            host_input.reset_actions(); host_stack.push(std::make_unique<MainMenuPlayerCount>(&host_context));
            host_tick(); host_tick(); host_tap(slot, GamepadButton::a);
            require(host_settings.player_settings()[0].num == slot && host_settings.player_count() == count,
                "ROAD34 actual Player Count action did not claim the initiating host");
            // This first observation precedes every reference page and later matrix case.
            host_observe("host-join", wearing ? boost : previous_boost, count == 2 ? 1 : -1);
            host_expect_page(0, wearing ? boost : previous_boost, count == 2 ? 1 : -1);
            if (count == 2)
            {
                host_tap(current_guest, GamepadButton::a);
                require(host_settings.player_settings()[1].num == current_guest, "ROAD34 guest did not claim its own device");
                host_observe("guest-join", wearing ? boost : previous_boost, guest_wearing ? 2 : 1);
            }
            host_tap(slot, GamepadButton::a);
            if (count == 2)
            {
                host_expect_page(1, wearing ? boost : previous_boost, guest_wearing ? 2 : 1);
                host_tap(slot, GamepadButton::a); host_tap(current_guest, GamepadButton::a);
            }
            host_observe("weapon", wearing ? boost : previous_boost, count == 2 ? (guest_wearing ? 2 : 1) : -1);
            host_expect_page(2, wearing ? boost : previous_boost, count == 2 ? (guest_wearing ? 2 : 1) : -1);
        };
        host_open(0, 1, 2, 0, true); // The original host preclaim defect fails here first.
        for (const int host : { 0, 3 })
            for (const int count : { 1, 2 })
                for (int boost = 0; boost <= 2; ++boost)
                    if (!(host == 0 && count == 1 && boost == 2))
                        host_open(host, count, boost, boost == 0 ? 2 : 0, true);
        host_open(0, 1, 0, 2, false);
        host_open(3, 2, 2, 1, false);
        host_open(input_slots::keyboard_and_mouse, 1, 2, 1, false);
        host_open(0, 2, 1, 0, true, true);

        // Commit a manual Boost by readying both weapon rows, then return via
        // Stage -> Weapon -> Team -> Weapon without beginning new device claims.
        host_tap(0, GamepadButton::x); host_tap(0, GamepadButton::a); host_tap(3, GamepadButton::a);
        host_observe("manual-commit", 2, 2); host_expect_page(3, 2, 2);
        host_tap(0, GamepadButton::b); host_expect_page(2, 2, 2);
        host_tap(0, GamepadButton::b); host_expect_page(1, 2, 2);
        host_tap(0, GamepadButton::a); host_tap(3, GamepadButton::a);
        host_observe("manual-back", 2, 2); host_expect_page(2, 2, 2);

        // Save API boundary only: no match is built or started by this matrix.
        PlayerProfiles expected_profiles = profiles_before_host_ui;
        expected_profiles.list[static_cast<size_t>(host_id)].boost = 2;
        const InputBindings before_bindings = host_preferences.input_bindings();
        std::array<int, input_slots::pad_count> carry{ -1, -1, -1, -1 };
        carry[0] = host_settings.player_handicap(0); carry[3] = host_settings.player_handicap(1);
        require(host_preferences.remember_profile_boosts_and_save(carry), "ROAD34 private Save API carry-back failed");
        require(host_preferences.profiles() == expected_profiles && profile_bytes() != before_host_ui,
            "ROAD34 Save API carry-back changed more than the selected profile Boost or did not persist it");
        Save reloaded; reloaded.load_save_file(); // The verified private file already exists; no legacy fallback is read.
        require(reloaded.profiles() == expected_profiles && reloaded.input_bindings() == before_bindings &&
            reloaded.play_settings().music_volume == 0 && reloaded.play_settings().sfx_volume == 0 &&
            reloaded.play_settings().reduce_effects, "ROAD34 private carry-back did not reopen with its original settings");
        for (int pad = 0; pad < input_slots::pad_count; ++pad)
            require(reloaded.worn_profile(pad) == -1, "ROAD34 runtime profile wearing leaked into the save");
        host_records << "save-api-carry-back,0,2,1,2,2,2,2,0,1\n";
        host_records.flush(); require(host_records.good(), "ROAD34 could not retain its Save API boundary observation");
        std::cout << "PASS host profile Boost: 16 actual Player Count/Join/Weapon paths, Pad0/Pad3 Off/+1/+2 and 1/2P, "
            "bare-pad/keyboard controls, guest inheritance, committed manual Back navigation and private Save API carry-back; "
            "no actual match-start or physical-pad acceptance\n";
    }

    const NestedSaveProfile profile(output / "bot-setup-profile");
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = true;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences);
    data.set_level_infos(&levels); data.set_stages(&stages); data.set_tuning(&tuning);
    data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MatchSetup settings;
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
        MatchSetup copy = settings;
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
        settings = MatchSetup(); settings.set_player_count(count);
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
        save(frame(), output / ("bot-setup-teams-" + std::to_string(size.x) + "x" + std::to_string(size.y) + ".ppm"));
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
