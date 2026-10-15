// Actual GameLevel/StateContext flow on the probe's hidden hardware shell.
// Public input messages drive the same pause pages as a player. Extra seats
// are idle dummies (Debug gives them idle bots), never claimed physical pads.
class ResumeProbeRoot final : public State
{
public:
    void init() override {}
    void update(float) override {}
    void draw(Renderer&) const override {}
};

Frame resume_stack_frame(Application& app)
{
    app.renderer()->begin_frame();
    static_cast<StateContext&>(app).draw(*app.renderer());
    return read(*app.renderer());
}

std::vector<float> resume_observation(const Level& level)
{
    auto result = observation(level);
    result.push_back(static_cast<float>(level.projectile_count()));
    for (int view = 0; view < level.player_count(); ++view)
    {
        result.push_back(level.player(view).primary_weapon().ready_to_shoot() ? 1.0f : 0.0f);
    }
    return result;
}

void resume_capture(Application& app, GameLevel& game, Presentation& presentation,
    const std::filesystem::path& output, const std::string& stem)
{
    const Level& level = game.level();
    const auto before = resume_observation(level);
    const float remaining = game.resume_remaining();
    require(remaining > 0.0f, "Resume capture was taken outside its count-in");
    const Frame context = resume_stack_frame(app);
    require(context.pixels == resume_stack_frame(app).pixels,
        "Frozen GameLevel resume drawing changed pixels");
    save(context, output / (stem + "-context.ppm"));

    // Isolate the shared paper overlay from Debug's status strip and prove
    // every changed pixel stays in the authored shared card and its shadow.
    const Frame plain = render(level, *app.renderer());
    app.renderer()->begin_frame();
    level.draw(*app.renderer());
    level.draw_resume_countdown(*app.renderer(), remaining);
    const Frame overlay = read(*app.renderer());
    const Vector2F size = app.resolution_manager()->resolution_vec();
    const auto& look = presentation.match();
    const float scale = std::min(size.x / look.reference_size.x, size.y / look.reference_size.y);
    const Vector2F card_size = look.countdown_size * scale;
    const float shadow = look.countdown_art.shadow_offset * scale + 3.0f;
    const RectangleF card((size.x - card_size.x) / 2.0f - shadow,
        size.y * look.countdown_center_y - card_size.y / 2.0f - shadow,
        card_size.x + shadow * 2.0f, card_size.y + shadow * 2.0f);
    const RectangleI delta = foreground_delta_bounds(overlay, plain);
    require(delta.width > 0 && delta.height > 0 &&
        RectangleI(card).contains(delta), "Shared resume paper escaped its authored card bounds");
    save(foreground_crop(context, RectangleI(card)), output / (stem + "-detail.ppm"));
    require(before == resume_observation(level) && game.resume_remaining() == remaining,
        "Resume drawing advanced simulation, cosmetics or its real-time clock");
}

#if COLOURWARS_DEV_TOOLS
void resume_native_tap(Application& app, WPARAM key)
{
    // F10 is intercepted by the production WH_GETMESSAGE hook, so drive
    // its native queue rather than only the Keyboard object.
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
    tap(Key::down); tap(Key::down);
    page("credits", Key::enter, 2);
    page("credits-home-return", Key::escape, 2);
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
        MenuLevelSettings settings;
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
            tick(presentation.music_mix().fade_out + 1.0f);
            require(stack.depth() == 3 && !ended && bank->effect_state(music) == SoundState::paused &&
                frozen == resume_observation(game->level()),
                "An unfocused End fade dispatched its action or advanced the finished world");
            stack.notify_activation(true);
            tick(presentation.music_mix().fade_out * 0.25f);
            require(stack.depth() == 3 && !ended && bank->effect_state(music) == SoundState::playing &&
                pending.pixels == resume_stack_frame(app).pixels && frozen == resume_observation(game->level()),
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
                const int moves = count == 2 ? 1 : 4;
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
        MenuLevelSettings settings;
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
        require(stack.depth() == 4, "Arena-music fixture did not enter actual Controls / audio");
        covered_focus("controls-focus-return", 4);
#if COLOURWARS_DEV_TOOLS
        developer_cover(false, 4, "controls-blocking-console-focus-return");
#endif
        tap(Key::enter); tick(0.0f);
        require(stack.depth() == 5, "Arena-music fixture did not enter nested Pad sticks");
        covered_focus("sticks-focus-return", 5);
        tap(Key::escape); tick(0.0f);
        require(stack.depth() == 4, "Arena-music fixture did not return from Pad sticks");
        tap(Key::escape); tick(0.0f);
        require(stack.depth() == 3, "Arena-music fixture did not return from Controls / audio");
        // Back on the pause page delivers the ordinary Resume result, even
        // though its selection remains on Controls / audio after the child.
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
            << "P arena music under pause/controls/sticks, focus, count-in, restart and quit\n";
    }
    require(evidence.good(), "Arena-music flow evidence write failed");
}

void resume_matrix(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    Save preferences;
    // Keep this fixture's preferences in memory; no player/save file is written.
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
            PlayerTeam::b, 0, Colour::white, app.render_resources(), &tuning.projectile(ProjectileType::spray));
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
        // Resume, Restart, Controls, Quit: Restart is the next row. The
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
    require(evidence.good(), "Resume evidence write failed");
}
