// Included inside painter_visual.cpp's private namespace, after resume_fixture.h.
// Menu, Results and arena music-ownership flows on the hardware shell.

#if COLOURWARS_DEV_TOOLS
void resume_native_tap(Application& app, WPARAM key)
{
    // Drive the native queue rather than only the Keyboard object, so the
    // window procedure's own translation is part of the path.
    const HWND window = static_cast<HWND>(app.window());
    // The window maps physical scan positions, not virtual-key characters.
    // Include the set-1 scan field that real keyboard messages carry.
    const unsigned int scan = key == VK_OEM_3 ? 0x29u : key == VK_ESCAPE ? 0x01u :
        key == VK_F10 ? 0x44u : 0u;
    require(scan != 0, "Unsupported music-flow developer native key");
    const LPARAM pressed = static_cast<LPARAM>(1u | (scan << 16));
    const auto message = [&](UINT kind, LPARAM flags)
    {
        require(PostMessageW(window, kind, key, flags) != 0,
            "Cannot queue a music-flow developer key message");
        MSG queued{};
        require(PeekMessageW(&queued, window, kind, kind, PM_REMOVE) != 0,
            "Cannot read a music-flow developer key message");
        DispatchMessageW(&queued);
        app.keyboard()->poll();
        app.mouse()->poll();
        static_cast<StateContext&>(app).update(0.0f);
    };
    message(WM_KEYDOWN, pressed);
    message(WM_KEYUP, pressed | static_cast<LPARAM>(0xc0000000u));
}
#endif

void menu_music_flow_matrix(Application& app, GameData& data, const Presentation& presentation,
    const std::filesystem::path& output)
{
    StateContext& stack = app;
    const SoundBank* bank = app.audio_resources()->sound_bank(presentation.menu().sound_bank);
    require(bank->audible(), "Menu-music hardware flow requires the accepted real sound bank");
    const auto music = bank->resolve_effect(presentation.main_menu().music.name);
    const auto arena = bank->resolve_effect("music.close_quarters");
    auto& play = const_cast<PlaySettings&>(data.save()->play_settings());
    struct RestoreVolume
    {
        PlaySettings& play;
        int original;
        ~RestoreVolume() { this->play.music_volume = this->original; }
    } restore_volume{ play, play.music_volume };
    // A finite 1% music setting exercises the real pending fade; SFX stay muted.
    play.music_volume = 1;
    const float fade = presentation.music_mix().fade_out;
    std::ofstream evidence(output / "menu-music.csv");
    evidence << "phase,depth,menu_state,menu_looping,arena_state\n";
    const auto tick = [&](float elapsed = dt)
    {
        app.keyboard()->poll();
        app.mouse()->poll();
        stack.update(elapsed);
    };
    const auto tap = [&](Key key)
    {
        app.keyboard()->on_key_down(key); tick(0.0f);
        app.keyboard()->on_key_up(key); tick(0.0f);
    };
    const auto record = [&](const char* phase)
    {
        evidence << phase << ',' << stack.depth() << ',' << static_cast<int>(bank->effect_state(music))
            << ',' << bank->is_effect_looping(music) << ',' << static_cast<int>(bank->effect_state(arena)) << '\n';
    };
    const auto playing = [&]
    {
        require(bank->effect_state(music) == SoundState::playing && bank->is_effect_looping(music),
            "The actual GameMenu owner lost its looping menu voice");
    };
    const auto page = [&](const char* phase, Key action, int depth)
    {
        const Frame before = resume_stack_frame(app);
        // A replacement page must borrow the existing paused voice. Calling
        // Play from its constructor/init would wake it even on these dt0 ticks.
        bank->pause_effect(music);
        tap(action);
        require(stack.depth() == depth && bank->effect_state(music) == SoundState::paused,
            "Main-menu page navigation replayed its shared voice or reached the wrong stack depth");
        require(before.pixels != resume_stack_frame(app).pixels,
            "Menu-music navigation failed to replace the expected visible page");
        tick(0.0f);
        require(bank->effect_state(music) == SoundState::paused,
            "A nonpositive page tick resumed the persistent menu voice");
        tick();
        playing();
        record(phase);
    };
    stack.clear();
    stack.notify_activation(true);
    stack.push(std::make_unique<GameMenu>(&data));
    require(stack.depth() == 2, "Actual GameMenu did not own its initial title page");
    playing();
    tick(presentation.music_mix().fade_in + dt);
    record("title");
    page("home", Key::enter, 2);
    tap(Key::down);
    page("options", Key::enter, 2);
    // Fullscreen disables the resolution row and initially focuses Fullscreen.
    for (int move = 0; move < (data.save()->fullscreen() ? 2 : 3); ++move) { tap(Key::down); }
    page("controls", Key::enter, 3);
    page("sticks", Key::enter, 4);
    const Frame settings = resume_stack_frame(app);
    stack.notify_activation(false);
    require(bank->effect_state(music) == SoundState::paused,
        "Focus loss under nested main-menu settings left music playing");
    tick(fade + 1.0f);
    stack.notify_activation(true);
    tick(0.0f);
    require(stack.depth() == 4 && bank->effect_state(music) == SoundState::paused &&
        settings.pixels == resume_stack_frame(app).pixels,
        "Focus return under nested settings resumed music before a positive page tick or changed the page");
    tick();
    playing();
    record("settings-focus-return");
#if COLOURWARS_DEV_TOOLS
    const auto developer_cover = [&](bool menu, int depth, const char* phase)
    {
        resume_native_tap(app, menu ? VK_F10 : VK_OEM_3);
        require(stack.depth() == depth + 1 && bank->effect_state(music) == SoundState::paused,
            "A developer overlay above the main menu failed to suspend its music");
        stack.notify_activation(false);
        tick();
        stack.notify_activation(true);
        tick();
        require(bank->effect_state(music) == SoundState::paused,
            "Focus return under a main-menu developer overlay woke its suspended music");
        resume_native_tap(app, menu ? VK_F10 : VK_ESCAPE);
        require(stack.depth() == depth && bank->effect_state(music) == SoundState::paused,
            "Closing a main-menu developer overlay did not retain the suspended voice until the page tick");
        tick();
        playing();
        record(phase);
    };
    developer_cover(true, 4, "settings-debug-menu-return");
#endif
    page("controls-return", Key::escape, 3);
#if COLOURWARS_DEV_TOOLS
    developer_cover(false, 3, "controls-console-return");
#endif
    page("options-return", Key::escape, 2);
    page("home-return", Key::escape, 2);
    tap(Key::down);
    page("credits", Key::enter, 2);
    page("credits-home-return", Key::escape, 2);
    tap(Key::up); tap(Key::up);
    page("mode", Key::enter, 2);
    page("player-count", Key::enter, 2);
    page("join", Key::enter, 2);
    page("weapon", Key::enter, 2);
    page("stage", Key::enter, 2);
    // The shared fixture lists Movement Course first and Close Quarters next.
    tap(Key::right);
    tap(Key::enter);
    tap(Key::enter);
    const Frame pending = resume_stack_frame(app);
    require(stack.depth() == 2 && bank->effect_state(music) == SoundState::playing &&
        bank->effect_state(arena) == SoundState::stopped,
        "Final Stage confirmation skipped the outgoing menu fade or began the arena early");
    tap(Key::enter); tap(Key::enter); tap(Key::escape); tap(Key::down);
    tick(0.0f); tick(-1.0f);
    require(pending.pixels == resume_stack_frame(app).pixels && stack.depth() == 2,
        "Input or nonpositive time changed the latched Stage exit");
    stack.notify_activation(false);
    tick(fade + 1.0f);
    require(stack.depth() == 2 && bank->effect_state(music) == SoundState::paused &&
        bank->effect_state(arena) == SoundState::stopped && pending.pixels == resume_stack_frame(app).pixels,
        "An unfocused outgoing menu fade dispatched its match or changed the pending page");
    stack.notify_activation(true);
    tick(fade * 0.25f);
    require(stack.depth() == 2 && bank->effect_state(music) == SoundState::playing &&
        bank->effect_state(arena) == SoundState::stopped && pending.pixels == resume_stack_frame(app).pixels,
        "The first partial outgoing menu tick completed the exit or lost its pending page");
    record("stage-exit-partial");
    tick(fade + dt);
    require(stack.depth() == 2 && bank->effect_state(music) == SoundState::stopped &&
        pending.pixels != resume_stack_frame(app).pixels,
        "The outgoing menu fade failed to stop its voice and dispatch exactly one match");
    record("stage-exit-complete");
    play.music_volume = 0;
    const int opening_steps = static_cast<int>((data.tuning()->match().countdown + 1.0f) / dt) + 4;
    for (int step = 0; step < opening_steps; ++step) { tick(); }
    require(bank->effect_state(arena) == SoundState::playing && bank->is_effect_looping(arena) &&
        bank->effect_state(music) == SoundState::stopped,
        "Menu handoff failed to leave only the actual arena loop playing");
    tap(Key::escape); tick(0.0f);
    require(stack.depth() == 3, "Menu-music return fixture did not reach actual Pause");
    tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::enter); tick(0.0f);
    tap(Key::up); tap(Key::enter);
    require(stack.depth() == 2 && bank->effect_state(arena) == SoundState::stopped,
        "Actual Quit did not return to the persistent GameMenu or stop arena ownership");
    tick();
    playing();
    record("quit-home");
    stack.clear();
    require(bank->effect_state(music) == SoundState::stopped,
        "Destroying the actual GameMenu retained its music voice");
    record("owner-destroyed");
    require(evidence.good(), "Menu-music flow evidence write failed");
    std::cout << "PASS actual GameMenu music ownership, page/settings continuity, focus, outgoing fade and match return\n";
}

void results_music_flow_matrix(Application& app, GameData& data, const Presentation& presentation,
    const std::filesystem::path& output)
{
    StateContext& stack = app;
    const SoundBank* bank = app.audio_resources()->sound_bank(presentation.menu().sound_bank);
    require(bank->audible(), "Results-music hardware flow requires the accepted real sound bank");
    const auto music = bank->resolve_effect(presentation.results_menu().music.name);
    auto& play = const_cast<PlaySettings&>(data.save()->play_settings());
    struct RestoreVolume
    {
        PlaySettings& play;
        int original;
        ~RestoreVolume() { this->play.music_volume = this->original; }
    } restore_volume{ play, play.music_volume };
    std::ofstream evidence(output / "results-music.csv");
    evidence << "players,round,phase,depth,sound_state\n";
    for (const int count : { 1, 2, 4 })
    {
        stack.clear();
        stack.notify_activation(true);
        stack.push(std::make_unique<ResumeProbeRoot>());
        MatchSetup settings;
        settings.set_stage("movement_course");
        settings.set_duration(0.25f);
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
        const EndMenuAction exit_action = count == 2 ? EndMenuAction::change_teams : EndMenuAction::exit;
        bool ended = false;
        stack.push<EndMenuAction>(std::move(owned), [&](const EndMenuAction& action)
        {
            if (action != exit_action)
            {
                throw std::runtime_error("Results-music fixture returned action " +
                    std::to_string(static_cast<int>(action)) + ", expected " +
                    std::to_string(static_cast<int>(exit_action)) + " for " + std::to_string(count) + "P");
            }
            ended = true;
        });
        const auto tick = [&](float elapsed = dt)
        {
            app.keyboard()->poll();
            app.mouse()->poll();
            stack.update(elapsed);
        };
        const auto tap = [&](Key key)
        {
            app.keyboard()->on_key_down(key); tick(0.0f);
            app.keyboard()->on_key_up(key); tick(0.0f);
        };
        const auto record = [&](int round, const char* phase)
        {
            evidence << count << ',' << round << ',' << phase << ',' << stack.depth() << ','
                << static_cast<int>(bank->effect_state(music)) << '\n';
        };
        const auto reach_results = [&]
        {
            // Release uses the ordinary story-skip key; Debug already has
            // intro off. Solo practice has no person at either end.
            if (game->level().intro_playing()) { tap(Key::enter); }
            for (int step = 0; step < 3000 && stack.depth() == 2; ++step)
            {
#if COLOURWARS_DEV_TOOLS
                // Replay completion can hold the final simulation frame before
                // GameLevel opens Results. Reproduce that clock state at the
                // same boundary without creating a separate recording.
                if (count == 1 && game->level().state() == LevelState::finished)
                {
                    require(data.debug().impl()->execute("pause on").ok,
                        "Cannot hold the final frame before opening Results");
                }
#endif
                tick();
            }
            require(stack.depth() == 3 && game->level().state() == LevelState::finished,
                "Actual short GameLevel did not reach its results page");
            SoundState expected = SoundState::playing;
#if COLOURWARS_DEV_TOOLS
            if (count == 1) { expected = SoundState::paused; }
#endif
            require(bank->effect_state(music) == expected && !bank->is_effect_looping(music),
                "Actual results page did not begin the owned nonlooping coda");
        };
        const auto results_ready = [&]
        {
            const auto& look = presentation.results_menu();
            const int steps = static_cast<int>((look.team_fill_delay + look.team_fill_time +
                look.show_results_delay + 1.0f) / dt) + 4;
            for (int step = 0; step < steps; ++step) { tick(); }
        };
        const auto choose_end = [&](int round)
        {
#if COLOURWARS_DEV_TOOLS
            if (count == 1)
            {
                require(data.debug().impl()->execute("pause on").ok &&
                    bank->effect_state(music) == SoundState::paused,
                    "Cannot suspend the muted End coda with the developer clock");
                tap(Key::enter);
                require(stack.depth() == (round == 1 ? 2 : 1) &&
                    bank->effect_state(music) == SoundState::stopped,
                    "A paused developer clock blocked an ordinary muted End Rematch/Exit action");
                record(round, "end-clock-paused-dispatch");
                require(data.debug().impl()->execute("pause off").ok,
                    "Cannot restore the clock after the muted End action");
                return;
            }
#endif
            if (count != 2) { tap(Key::enter); return; }
            // Retain the existing muted 1/4-seat immediate exits; the two-seat
            // rematch and change-teams routes exercise finite audible fades.
            play.music_volume = 1;
            const auto frozen = resume_observation(game->level());
            tap(Key::enter);
            const Frame pending = resume_stack_frame(app);
            require(stack.depth() == 3 && !ended && bank->effect_state(music) == SoundState::playing,
                "An audible End action skipped its pending coda fade");
            tap(Key::enter); tap(Key::enter); tap(Key::escape); tap(Key::up);
            tick(0.0f); tick(-1.0f);
            require(stack.depth() == 3 && !ended && pending.pixels == resume_stack_frame(app).pixels &&
                frozen == resume_observation(game->level()),
                "Repeated input or nonpositive time changed the latched End action");
            stack.notify_activation(false);
            // Focus loss clears the cosmetic paper press immediately. The
            // latched action, its page and the finished world remain frozen.
            const Frame interrupted = resume_stack_frame(app);
            tick(presentation.music_mix().fade_out + 1.0f);
            require(stack.depth() == 3 && !ended && bank->effect_state(music) == SoundState::paused &&
                frozen == resume_observation(game->level()),
                "An unfocused End fade dispatched its action or advanced the finished world");
            stack.notify_activation(true);
            tick(presentation.music_mix().fade_out * 0.25f);
            require(stack.depth() == 3 && !ended && bank->effect_state(music) == SoundState::playing &&
                interrupted.pixels == resume_stack_frame(app).pixels && frozen == resume_observation(game->level()),
                "A partial End fade dispatched early or changed its pending page/world");
            record(round, "end-exit-partial");
            tick(presentation.music_mix().fade_out + dt);
            play.music_volume = 0;
        };
        for (int round = 1; round <= 2; ++round)
        {
            reach_results();
            record(round, "results");
            stack.notify_activation(false);
            require(bank->effect_state(music) == SoundState::paused,
                "Focus loss under the actual results page did not pause the coda");
            record(round, "results-unfocused");
            stack.notify_activation(true);
#if COLOURWARS_DEV_TOOLS
            if (count == 1)
            {
                require(bank->effect_state(music) == SoundState::paused,
                    "Focus return woke a results coda created under the paused developer clock");
                record(round, "results-clock-paused-focus-return");
                require(data.debug().impl()->execute("pause off").ok,
                    "Cannot release the clock held before Results opened");
            }
#endif
            require(bank->effect_state(music) == SoundState::playing,
                "Focus return under the covering results page did not resume the coda");
#if COLOURWARS_DEV_TOOLS
            if (count == 2 && round == 1)
            {
                const auto frozen = resume_observation(game->level());
                resume_native_tap(app, VK_F10);
                require(stack.depth() == 4 && bank->effect_state(music) == SoundState::paused,
                    "The developer menu above Results did not pause its owned coda");
                stack.notify_activation(false);
                tick();
                stack.notify_activation(true);
                tick();
                require(stack.depth() == 4 && bank->effect_state(music) == SoundState::paused &&
                    frozen == resume_observation(game->level()),
                    "Focus return under the developer menu woke the results coda or changed the finished world");
                record(round, "results-debug-menu-focus-return");
                resume_native_tap(app, VK_F10);
                require(stack.depth() == 3 && bank->effect_state(music) == SoundState::paused,
                    "Closing the developer menu failed to return to the Results page");
                tick();
                require(bank->effect_state(music) == SoundState::playing &&
                    !bank->is_effect_looping(music) && frozen == resume_observation(game->level()),
                    "The uncovered Results page failed to resume its existing coda with the finished world frozen");
                record(round, "results-debug-menu-closed");
            }
#endif
            results_ready();
            // Keep the voice paused across the real continue callback. A
            // page-owned replacement would start a new playing coda here,
            // making a restart detectable without a production-only accessor.
            stack.notify_activation(false);
            require(bank->effect_state(music) == SoundState::paused,
                "Results coda ended before the page-continuity check");
            tap(Key::enter);
            require(stack.depth() == 3 && bank->effect_state(music) == SoundState::paused,
                "Results-to-End replaced, restarted or stopped its paused coda");
            record(round, "end-unfocused");
            stack.notify_activation(true);
            require(bank->effect_state(music) == SoundState::playing,
                "Focus return under the End page did not resume the same coda");
            record(round, "end");
            if (round == 1)
            {
                // Rematch is the initial action. Its real callback must stop
                // the previous coda before the new round can create another.
                choose_end(round);
                require(stack.depth() == 2 && bank->effect_state(music) == SoundState::stopped &&
                    game->level().state() != LevelState::finished,
                    "Actual Rematch retained the old coda or failed to rebuild the round");
                record(round, "rematch");
            }
            else
            {
                if (count == 4)
                {
                    // One bounded real-time check covers the asynchronous
                    // buffer drain that the recording backend cannot model.
                    const ULONGLONG deadline = GetTickCount64() + 8000;
                    while (bank->effect_state(music) == SoundState::playing && GetTickCount64() < deadline)
                    {
                        Sleep(10);
                    }
                    require(bank->effect_state(music) == SoundState::stopped,
                        "Accepted results coda did not finish naturally within eight seconds");
                    stack.notify_activation(false);
                    stack.notify_activation(true);
                    require(bank->effect_state(music) == SoundState::stopped,
                        "Focus return resurrected the naturally finished results coda");
                    record(round, "natural-end-focus-return");
                }
                // Cover both destruction routes: Change Teams for 2P, and
                // Exit to Main Menu for the solo and four-seat fixtures.
                // End resets navigation when it is created. Observe a neutral
                // sample before the first direction so the new page arms it.
                tick(0.0f);
                // Watch Replay (ROAD#19) is row 1 after any timed round with an
                // unspoiled tape, which each of these is; the rest sit below it.
                constexpr int replay_rows = 1;
                const int moves = replay_rows + (count == 2 ? 1 : 4);
                for (int move = 0; move < moves; ++move) { tap(Key::down); }
                choose_end(round);
                require(ended && stack.depth() == 1 && bank->effect_state(music) == SoundState::stopped,
                    "Leaving the actual End page did not destroy and stop its owned coda");
                record(round, count == 2 ? "change-teams" : "exit");
            }
        }
        std::cout << "PASS actual " << count
            << "P GameLevel results/end coda, covered focus, continuity, rematch and teardown\n";
    }
    require(evidence.good(), "Results-music flow evidence write failed");
}

void arena_music_flow_matrix(Application& app, GameData& data, const GameTuning& tuning,
    const Presentation& presentation, const std::filesystem::path& output)
{
    StateContext& stack = app;
    const SoundBank* bank = app.audio_resources()->sound_bank("paint_exe");
    require(bank->audible(), "Arena-music hardware flow requires the accepted real sound bank");
    const auto music = bank->resolve_effect("music.close_quarters");
    const auto weapon = bank->resolve_effect(
        tuning.weapon(WeaponType::sprayer).loop_sounds.sound_name(PlayerTeam::a, 0));
    const float duration = presentation.pause_menu().resume_countdown;
    std::ofstream evidence(output / "arena-music.csv");
    evidence << "players,phase,depth,music_state,music_looping,weapon_state\n";
    for (const int count : { 1, 2, 4 })
    {
        stack.clear();
        stack.notify_activation(true);
        stack.push(std::make_unique<ResumeProbeRoot>());
        MatchSetup settings;
        settings.set_stage("close_quarters");
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
        stack.push<EndMenuAction>(std::move(owned), [&](const EndMenuAction& action)
        {
            require(action == EndMenuAction::exit, "Arena-music fixture left with the wrong end action");
            ended = true;
        });
        const auto tick = [&](float elapsed = dt)
        {
            app.keyboard()->poll();
            app.mouse()->poll();
            stack.update(elapsed);
        };
        const auto tap = [&](Key value)
        {
            app.keyboard()->on_key_down(value); tick(0.0f);
            app.keyboard()->on_key_up(value); tick(0.0f);
        };
        const auto record = [&](const char* phase)
        {
            evidence << count << ',' << phase << ',' << stack.depth() << ','
                << static_cast<int>(bank->effect_state(music)) << ',' << bank->is_effect_looping(music)
                << ',' << static_cast<int>(bank->effect_state(weapon)) << '\n';
        };
        const auto enter_live = [&]
        {
            if (game->level().intro_playing()) { tap(Key::enter); }
            for (int step = 0; step < 2400 && game->level().state() != LevelState::active; ++step) { tick(); }
            require(stack.depth() == 2 && game->level().state() == LevelState::active &&
                bank->effect_state(music) == SoundState::playing && bank->is_effect_looping(music),
                "Actual arena GameLevel did not begin its looping stage track");
        };
        const auto pause = [&]
        {
            tap(Key::escape);
            require(stack.depth() == 3, "Arena-music fixture did not open the actual pause page");
            // New pages reset direction arming; give the page a neutral sample.
            tick(0.0f);
        };
        const auto quiet_music = [&]
        {
            require(bank->effect_state(music) == SoundState::playing && bank->is_effect_looping(music) &&
                bank->effect_state(weapon) == SoundState::paused,
                "Covered arena did not retain only its music while the fire loop stayed paused");
        };
        const auto covered_focus = [&](const char* phase, int depth)
        {
            quiet_music();
            const auto frozen = resume_observation(game->level());
            tick(presentation.music_mix().duck_time + 0.25f);
            require(frozen == resume_observation(game->level()),
                "Covered arena music tick advanced the simulation or cosmetics");
            stack.notify_activation(false);
            require(bank->effect_state(music) == SoundState::paused &&
                bank->effect_state(weapon) == SoundState::paused,
                "Focus loss under a pause/settings page left an arena or fire loop playing");
            tick(duration + 1.0f);
            require(frozen == resume_observation(game->level()),
                "Unfocused covered arena advanced its simulation or cosmetics");
            stack.notify_activation(true);
            require(stack.depth() == depth && game->resume_remaining() == 0.0f &&
                frozen == resume_observation(game->level()),
                "Focus return under a pause/settings page uncovered or advanced the arena");
            quiet_music();
            record(phase);
        };
#if COLOURWARS_DEV_TOOLS
        const auto native_tap = [&](WPARAM key)
        {
            resume_native_tap(app, key);
        };
        const auto developer_cover = [&](bool menu, int depth, const char* phase)
        {
            quiet_music();
            const auto frozen = resume_observation(game->level());
            native_tap(menu ? VK_F10 : VK_OEM_3);
            require(stack.depth() == depth + 1 && data.debug().match_audio_paused() &&
                bank->effect_state(music) == SoundState::paused && bank->effect_state(weapon) == SoundState::paused,
                "A developer page above Pause/Controls failed to hard-pause arena/fire loops immediately");
            stack.notify_activation(false);
            tick(duration + 1.0f);
            stack.notify_activation(true);
            tick(duration + 1.0f);
            require(stack.depth() == depth + 1 && game->resume_remaining() == 0.0f &&
                frozen == resume_observation(game->level()) && bank->effect_state(music) == SoundState::paused,
                "Focus return under a covering developer page woke arena music or advanced the world");
            record(phase);
            native_tap(menu ? VK_F10 : VK_ESCAPE);
            require(stack.depth() == depth && !data.debug().match_audio_paused() &&
                bank->effect_state(music) == SoundState::paused,
                "Closing a developer page failed to return to its still-paused underlying page");
            tick();
            quiet_music();
            require(frozen == resume_observation(game->level()),
                "Restoring quiet music after a developer page advanced the covered world");
        };
#endif

        enter_live();
#if COLOURWARS_DEV_TOOLS
        DebugDirectorImpl& debug = *data.debug().impl();
        require(debug.execute("console.live 1").ok, "Cannot enable the arena-music live console");
        const int before_live_console = debug.level_updates();
        native_tap(VK_OEM_3);
        tick();
        require(debug.live_console_open() && !data.debug().match_audio_paused() && stack.depth() == 2 &&
            debug.level_updates() > before_live_console && bank->effect_state(music) == SoundState::playing,
            "The live console suspended arena music or the active simulation");
        native_tap(VK_OEM_3);
        require(!debug.live_console_open() && debug.execute("console.live 0").ok,
            "Cannot close and reset the arena-music live console");
        record("debug-live-console");
#endif
        app.mouse()->on_move({ 640, 0 });
        app.mouse()->on_button_down(MouseButton::left);
        for (int step = 0; step < 12 && bank->effect_state(weapon) != SoundState::playing; ++step) { tick(); }
        require(bank->effect_state(weapon) == SoundState::playing && bank->is_effect_looping(weapon),
            "Arena-music fixture did not start the actual player's fire loop");
        record("live-fire");
#if COLOURWARS_DEV_TOOLS
        require(debug.execute("pause on").ok, "Cannot pause the arena-music developer clock");
        const auto clock_frozen = resume_observation(game->level());
        const int before_pause = debug.level_updates();
        tick(0.5f);
        require(debug.level_updates() == before_pause && clock_frozen == resume_observation(game->level()) &&
            bank->effect_state(music) == SoundState::paused && bank->effect_state(weapon) == SoundState::paused,
            "Developer pause advanced the arena or retained playing music/fire loops");
        record("debug-pause");
        require(debug.execute("step 1").ok, "Cannot step the arena-music developer clock");
        tick();
        require(debug.level_updates() == before_pause + 1 &&
            bank->effect_state(music) == SoundState::paused && bank->effect_state(weapon) == SoundState::paused,
            "One developer step failed to run once or leaked playing arena/fire loops");
        record("debug-step");
        require(debug.execute("pause off").ok && debug.execute("timescale 4").ok,
            "Cannot restore and fast-forward the arena-music developer clock");
        const int before_fast = debug.level_updates();
        tick();
        require(debug.level_updates() == before_fast + 4 &&
            bank->effect_state(music) == SoundState::playing && bank->is_effect_looping(music),
            "Developer fast-forward failed to run four simulation steps with the arena track playing");
        require(debug.execute("timescale 1").ok, "Cannot reset the arena-music developer timescale");
        record("debug-fast-forward");
#endif
        pause();
        app.mouse()->on_button_up(MouseButton::left);
        covered_focus("pause-focus-return", 3);
#if COLOURWARS_DEV_TOOLS
        developer_cover(true, 3, "pause-debug-menu-focus-return");
#endif
        tap(Key::down); tap(Key::down); tap(Key::enter); tick(0.0f);
        require(stack.depth() == 4, "Arena-music fixture did not enter actual Options");
        covered_focus("options-focus-return", 4);
        for (int row = data.save()->fullscreen() ? 1 : 0; row < 3; ++row) { tap(Key::down); }
        tap(Key::enter); tick(0.0f);
        require(stack.depth() == 5, "Arena-music fixture did not enter actual Controls / audio");
        covered_focus("controls-focus-return", 5);
#if COLOURWARS_DEV_TOOLS
        developer_cover(false, 5, "controls-blocking-console-focus-return");
#endif
        tap(Key::enter); tick(0.0f);
        require(stack.depth() == 6, "Arena-music fixture did not enter nested Pad sticks");
        covered_focus("sticks-focus-return", 6);
        tap(Key::escape); tick(0.0f);
        require(stack.depth() == 5, "Arena-music fixture did not return from Pad sticks");
        tap(Key::escape); tick(0.0f);
        require(stack.depth() == 4, "Arena-music fixture did not return from Controls / audio");
        tap(Key::escape); tick(0.0f);
        require(stack.depth() == 3, "Arena-music fixture did not return from Options");
        // Back on the pause page delivers the ordinary Resume result, even
        // though its selection remains on Options after its children.
        tap(Key::escape);
        require(stack.depth() == 2 && game->resume_remaining() == duration,
            "Actual Resume did not begin the arena-music count-in");
        quiet_music();
        const auto frozen = resume_observation(game->level());
        tick(duration * 0.25f);
        require(frozen == resume_observation(game->level()),
            "Arena-music count-in advanced simulation or cosmetics");
        stack.notify_activation(false);
        const float remaining = game->resume_remaining();
        require(bank->effect_state(music) == SoundState::paused,
            "Focus loss during the count-in left arena music playing");
        tick(duration + 1.0f);
        require(game->resume_remaining() == remaining && frozen == resume_observation(game->level()),
            "Unfocused arena-music count-in advanced its clock or world");
        stack.notify_activation(true);
        require(game->resume_remaining() == duration, "Arena-music focus return did not restart the count-in");
        quiet_music();
        record("count-in-focus-return");
        // A developer state covering the count-in is a hard suspension,
        // unlike the match's ordinary pause/settings pages. Activation still
        // reaches the covered GameLevel and must not wake its arena voice.
        tick(duration * 0.25f);
        const float covered_remaining = game->resume_remaining();
        stack.push(std::make_unique<ResumeProbeRoot>());
        require(stack.depth() == 3 && bank->effect_state(music) == SoundState::paused &&
            bank->effect_state(weapon) == SoundState::paused,
            "An unrelated state covering the count-in did not hard-pause arena/fire loops");
        stack.notify_activation(false);
        tick(duration + 1.0f);
        stack.notify_activation(true);
        tick(duration + 1.0f);
        require(stack.depth() == 3 && game->resume_remaining() == covered_remaining &&
            frozen == resume_observation(game->level()) &&
            bank->effect_state(music) == SoundState::paused && bank->effect_state(weapon) == SoundState::paused,
            "Focus return under an unrelated covering state woke arena music or advanced the count-in/world");
        record("count-in-unrelated-cover-focus-return");
        stack.pop();
        require(stack.depth() == 2 && game->resume_remaining() == duration &&
            frozen == resume_observation(game->level()),
            "Uncovering the focus-interrupted arena did not restart a frozen full count-in");
        quiet_music();
        record("count-in-uncovered");
        tick(duration + 1.0f);
        require(game->resume_remaining() == 0.0f && frozen == resume_observation(game->level()) &&
            bank->effect_state(music) == SoundState::playing && bank->is_effect_looping(music) &&
            bank->effect_state(weapon) == SoundState::stopped,
            "Count-in completion advanced the world, lost the stage loop or revived the old fire loop");
        record("count-in-complete");
        tick();
        app.mouse()->on_button_down(MouseButton::left);
        for (int step = 0; step < 12 && bank->effect_state(weapon) != SoundState::playing; ++step) { tick(); }
        require(bank->effect_state(music) == SoundState::playing && bank->is_effect_looping(music) &&
            bank->effect_state(weapon) == SoundState::playing,
            "Fresh gameplay failed to keep the stage loop and restart deliberate fire");
        record("restored-live-fire");
        pause();
        app.mouse()->on_button_up(MouseButton::left);
        tap(Key::down); tap(Key::enter); tick(0.0f);
        tap(Key::up); tap(Key::enter);
        require(stack.depth() == 2 && bank->effect_state(music) == SoundState::stopped &&
            bank->effect_state(weapon) == SoundState::stopped,
            "Actual Restart retained the old arena track or fire-loop ownership");
        record("restart");
        enter_live();
        pause();
        tap(Key::down); tap(Key::down); tap(Key::down); tap(Key::enter); tick(0.0f);
        tap(Key::up); tap(Key::enter);
        require(ended && stack.depth() == 1 && bank->effect_state(music) == SoundState::stopped &&
            bank->effect_state(weapon) == SoundState::stopped,
            "Actual Quit retained arena music or fire-loop ownership");
        record("quit");
        std::cout << "PASS actual " << count
            << "P arena music under pause/options/controls/sticks, focus, count-in, restart and quit\n";
    }
    require(evidence.good(), "Arena-music flow evidence write failed");
}
