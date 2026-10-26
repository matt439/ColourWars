// Debug-only supplement to RumbleOnly. Native window observations and
// synthetic StateContext activation are deliberately separate witnesses.
#if COLOURWARS_DEV_TOOLS && defined(COLOURWARS_PAINTER_SCRIPTED_PADS)
namespace debug_background_acceptance
{
    struct WindowMessages
    {
        int custom = 0;
        int power = 0;
        int destroyed = 0;
        int size = 0;
        int show = 0;
        WPARAM last_size = 0;
        WPARAM last_power = 0;
        WPARAM last_wparam = 0;
        LPARAM last_lparam = 0;
    };

    LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        auto* messages = reinterpret_cast<WindowMessages*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            messages = static_cast<WindowMessages*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(messages));
        }
        if (messages != nullptr)
        {
            if (message == WM_APP + 17)
            {
                ++messages->custom; messages->last_wparam = wparam; messages->last_lparam = lparam;
                return 0x4567;
            }
            if (message == WM_SIZE) { ++messages->size; messages->last_size = wparam; }
            if (message == WM_SHOWWINDOW) { ++messages->show; }
            if (message == WM_POWERBROADCAST)
            { ++messages->power; messages->last_power = wparam; return TRUE; }
            if (message == WM_NCDESTROY) { ++messages->destroyed; }
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    void window_checks()
    {
        DebugBackgroundWindow observer;
        require(!observer.attach(nullptr, {}) && !background_window_allows(observer.state()),
            "Missing native background observer did not fail closed");
        const wchar_t* name = L"ColourWarsPrivateBackgroundAcceptance";
        WNDCLASSW definition{}; definition.lpfnWndProc = &window_proc;
        definition.hInstance = GetModuleHandleW(nullptr); definition.lpszClassName = name;
        require(RegisterClassW(&definition) != 0, "Cannot register private background window");
        WindowMessages messages;
        HWND window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, name, name,
            WS_OVERLAPPEDWINDOW, -20000, -20000, 160, 120, nullptr, nullptr, definition.hInstance, &messages);
        int changes = 0;
        struct Cleanup
        {
            HWND& window; const wchar_t* name; HINSTANCE instance;
            ~Cleanup()
            {
                if (this->window != nullptr) { DestroyWindow(this->window); }
                UnregisterClassW(this->name, this->instance);
            }
        } cleanup{ window, name, definition.hInstance };
        require(window != nullptr, "Cannot create private background window");
        require(observer.attach(window, [&] { ++changes; }), "Cannot attach actual native background observer");
        require(!IsWindowVisible(window) && !observer.state().visible,
            "Private hidden HWND was reported visible");
        require(SendMessageW(window, WM_APP + 17, 73, 91) == 0x4567 && messages.custom == 1 &&
            messages.last_wparam == 73 && messages.last_lparam == 91,
            "Background subclass changed a forwarded message or its result");
        ShowWindow(window, SW_SHOWNOACTIVATE);
        require(IsWindowVisible(window) && observer.state().visible && background_window_allows(observer.state()) && messages.show > 0,
            "Native IsWindowVisible fact did not open the ordinary visible-window gate");
        // A second independent observer must not replace the first observer's
        // subclass, and its destruction must not remove that first observer.
        {
            DebugBackgroundWindow second;
            require(second.attach(window, {}) && second.state().visible,
                "Cannot attach an independent native observer to the private HWND");
        }
        const int sizes_before = messages.size;
        ShowWindow(window, SW_MINIMIZE);
        require(IsIconic(window) && observer.state().minimized && !background_window_allows(observer.state()) &&
            messages.size > sizes_before && messages.last_size == SIZE_MINIMIZED,
            "Native minimization did not close the background gate");
        ShowWindow(window, SW_SHOWNOACTIVATE);
        require(!IsIconic(window) && !observer.state().minimized && background_window_allows(observer.state()),
            "Native nonactivating restore did not restore window facts");
        require(SendMessageW(window, WM_POWERBROADCAST, PBT_APMSUSPEND, 0) == TRUE &&
            observer.state().power_suspended && !background_window_allows(observer.state()) && messages.last_power == PBT_APMSUSPEND,
            "Injected power-suspend message was not forwarded or gated");
        require(SendMessageW(window, WM_POWERBROADCAST, PBT_APMRESUMEAUTOMATIC, 0) == TRUE &&
            !observer.state().power_suspended && messages.power == 2 && messages.last_power == PBT_APMRESUMEAUTOMATIC,
            "Injected power-resume message was not forwarded or cleared");
        ShowWindow(window, SW_HIDE);
        require(!IsWindowVisible(window) && !observer.state().visible && changes >= 6,
            "Native hide did not close the background gate or publish fact changes");
        require(DestroyWindow(window) && messages.destroyed == 1, "Subclass swallowed native NCDESTROY");
        window = nullptr;
        require(!observer.state().visible && !background_window_allows(observer.state()),
            "Destroyed HWND retained usable background facts");
    }

    bool neutral(const PlayerInputData& input)
    {
        return input.x_movement == 0.0f && !input.jump_pressed && !input.jump_held &&
            input.shoot_angle == 0.0f && input.shoot_direction == Vector2F::ZERO &&
            !input.shoot_direction_requested && !input.primary_shoot && !input.drop_through &&
            !input.toggle_debug && !input.toggle_pause_menu && !input.pause_held && !input.skip_intro &&
            input.left_analog_stick == Vector2F::ZERO && input.right_analog_stick == Vector2F::ZERO;
    }
}

void debug_background_checks(Application& app, Registry<LevelLoadedInfo>& levels,
    GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    using namespace debug_background_acceptance;
    window_checks();
    const NestedSaveProfile profile(output / "debug-background-profile");
    Save preferences;
    PlaySettings initial; initial.music_volume = 17; initial.sfx_volume = 23;
    require(preferences.set_play_settings_and_save(initial), "Cannot seed private background preferences");
    const auto save_path = output / "debug-background-profile" / "ColourWars" / "save_data.json";
    const auto save_bytes = [&]
    {
        std::ifstream file(save_path, std::ios::binary);
        require(file.good(), "Cannot read private background save witness");
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    };
    const std::string saved = save_bytes();
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    DebugBackgroundState facts{ true, false, false };
    DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    DebugDirectorImpl& impl = *director.impl();
    impl.set_content_directories(content, content);
    impl.set_user_directory(output / "debug-background-profile" / "debug");
    impl.log().set_mirror_to_debugger(false);
    director.attach(&app, &data);
    require(impl.execute("intro off").ok, "Cannot suppress background fixture intro");
    painter_rumble::Recording motors;
    painter_rumble::RecordScope recording(motors);
    StateContext& stack = app;
    const bool was_active = stack.active();
    const bool keyboard_focused = app.keyboard()->focused();
    const bool mouse_focused = app.mouse()->focused();
    const auto previous_pads = painter_scripted_pads::states;
    auto* audio = app.audio_resources();
    const auto music = audio->resolve_sound_group("music");
    const auto sfx = audio->resolve_sound_group("sfx");
    const float old_music = audio->group_volume(music), old_sfx = audio->group_volume(sfx);
    struct Cleanup
    {
        StateContext& stack; Application& app; DebugDirectorImpl& impl;
        bool active, keyboard_focused, mouse_focused;
        std::array<GamepadState, Gamepads::max_count> pads;
        AudioResources::SoundGroupHandle music, sfx;
        float old_music, old_sfx;
        ~Cleanup()
        {
            this->stack.clear(); this->impl.set_background_probe({});
            this->stack.notify_activation(this->active);
            for (Key key : { Key::d, Key::space, Key::escape, Key::f10 }) { this->app.keyboard()->on_key_up(key); }
            this->app.keyboard()->set_focused(this->keyboard_focused);
            this->app.mouse()->set_focused(this->mouse_focused);
            painter_scripted_pads::states = this->pads;
            this->app.gamepads()->poll(); this->app.gamepads()->poll();
            this->app.keyboard()->poll(); this->app.keyboard()->poll();
            this->app.mouse()->poll(); this->app.mouse()->poll();
            this->app.audio_resources()->set_group_volume(this->music, this->old_music);
            this->app.audio_resources()->set_group_volume(this->sfx, this->old_sfx);
        }
    } cleanup{ stack, app, impl, was_active, keyboard_focused, mouse_focused, previous_pads,
        music, sfx, old_music, old_sfx };
    std::ofstream cases(output / "debug-background.csv");
    cases << "case,level_updates,stack_active,stack_depth,music,sfx,motor_requests\n";
    int case_count = 0;
    const auto note = [&](const char* name)
    {
        cases << name << ',' << impl.level_updates() << ',' << (stack.active() ? "true" : "false") << ',' <<
            stack.depth() << ',' << audio->group_volume(music) << ',' << audio->group_volume(sfx) << ',' << motors.count << '\n';
        ++case_count;
        require(save_bytes() == saved, "Background policy wrote the private Save");
    };
    const auto tick = [&](float elapsed = dt)
    {
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto muted = [&]
    {
        return audio->group_volume(music) == 0.0f && audio->group_volume(sfx) == 0.0f;
    };
    const auto no_motors = [&]
    {
        require(!motors.invalid, "Background motor recorder overflowed or rejected an intent");
        for (const auto& state : motors.current)
        {
            require(state.low_frequency == 0.0f && state.high_frequency == 0.0f,
                "Inactive GameLevel retained a motor intent");
        }
    };
    const auto no_motor_calls_since = [&](size_t begin)
    {
        for (size_t index = begin; index < motors.count; ++index)
        {
            require(motors.calls[index].motors.low_frequency == 0.0f && motors.calls[index].motors.high_frequency == 0.0f,
                "Inactive actual gameplay briefly emitted a nonzero motor intent");
        }
        no_motors();
    };
    const auto probe = [&] { impl.set_background_probe([&facts] { return facts; }); };
    const auto command = [&](const char* text)
    {
        const std::string message = std::string("Background fixture command failed: ") + text;
        require(impl.execute(text).ok, message.c_str());
    };
    const auto pads_neutral = [&]
    {
        painter_scripted_pads::states = {};
        painter_scripted_pads::states[3].connected = true;
        painter_scripted_pads::states[1].connected = true;
        for (Key key : { Key::d, Key::space, Key::escape, Key::f10 }) { app.keyboard()->on_key_up(key); }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
    };
    const auto pad = [&](GamepadButton button, int slot = 3)
    {
        painter_scripted_pads::states[static_cast<size_t>(slot)].buttons = static_cast<std::uint16_t>(1u << static_cast<unsigned int>(button));
        tick(0.0f); painter_scripted_pads::states[static_cast<size_t>(slot)].buttons = 0; tick(0.0f); tick(0.0f);
    };
    GameLevel* game = nullptr;
    const auto launch = [&]
    {
        stack.clear(); pads_neutral();
        MenuLevelSettings settings; settings.set_stage("movement_course"); settings.set_duration(60.0f);
        settings.set_player_count(3); settings.begin_input_claims(3);
        require(settings.claim_input_slot(1) && settings.claim_input_slot(input_slots::keyboard_and_mouse),
            "Cannot form sparse background roster");
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
        for (int count = 0; count < 2400 && game->level().state() != LevelState::active; ++count) { tick(); }
        require(stack.depth() == 2 && game->level().state() == LevelState::active,
            "Background fixture did not reach actual live play");
    };
    const auto frozen = [&](const char* reason)
    {
        const int updates = impl.level_updates();
        const auto before = resume_observation(game->level());
        for (int count = 0; count < 5; ++count) { tick(); }
        require(impl.level_updates() == updates && before == resume_observation(game->level()), reason);
    };
    const auto finish_count = [&]
    {
        require(game->resume_remaining() > 0.0f, "Ordinary activation failed to start resume count-in");
        const int updates = impl.level_updates();
        tick(game->resume_remaining() + dt);
        require(impl.level_updates() == updates && game->resume_remaining() == 0.0f,
            "Count-in final update leaked simulation or failed to finish");
        tick();
    };

    stack.clear(); stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
    sound_groups::apply(*audio, preferences.play_settings());
    launch(); live();
    require(!director.background_scripts(), "Background mode was not default Off");
    // Deliberately leave Mouse::focused() true across synthetic StateContext
    // deactivation. Level resolves mouse aim before its seat filter; the
    // inactive physical baseline must remain neutral after that resolution.
    command("timescale 0.25");
    painter_scripted_pads::states[3].buttons = static_cast<std::uint16_t>(1u << static_cast<unsigned int>(GamepadButton::a));
    const int edge_begin = impl.level_updates(); tick();
    require(impl.level_updates() == edge_begin, "Foreground edge control did not take a zero-run clock tick");
    command("timescale 1");
    stack.notify_activation(false);
    frozen("Default-Off actual inactive GameLevel advanced"); note("default-off-inactive");
    command("script.background 1");
    require(!director.background_updates_allowed(), "Actual hidden painter window bypassed IsWindowVisible");
    frozen("Opt-in hidden native Application advanced"); note("actual-hidden-hard-gate");
    probe();
    require(!stack.active() && director.background_updates_allowed() && muted(),
        "Opt-in failed to reconcile synthetic inactive facts or mute before first update");
    painter_scripted_pads::states[3].left_stick = Vector2F(1, 0);
    painter_scripted_pads::states[3].right_stick = Vector2F(1, 0);
    painter_scripted_pads::states[3].right_trigger = 1.0f;
    painter_scripted_pads::states[3].buttons = static_cast<std::uint16_t>(
        (1u << static_cast<unsigned int>(GamepadButton::a)) | (1u << static_cast<unsigned int>(GamepadButton::start)));
    app.keyboard()->on_key_down(Key::d); app.keyboard()->on_key_down(Key::space);
    const int neutral_begin = impl.level_updates(); tick();
    require(impl.level_updates() == neutral_begin + 1 && stack.depth() == 2 && !stack.active(),
        "Opt-in actual Level did not advance without changing synthetic activation");
    for (int view = 0; view < 3; ++view)
    {
        require(neutral(game->level().player(view).input()) && game->level().player(view).input().connected,
            "Inactive physical actions leaked or an occupied present seat became disconnected");
    }
    require(!app.gamepads()->connected(0) && !app.gamepads()->connected(2),
        "Background policy fabricated an absent raw pad");
    no_motors(); note("synthetic-inactive-neutral-presence-muted-before-step");
    require(!game->level().player(0).input_jump_pressed(), "Inactive step resurrected a queued foreground Jump edge");
    note("foreground-zero-run-edge-discarded"); pads_neutral();
    const size_t damage_begin = motors.count;
    Player& victim = const_cast<Player&>(game->level().player(1));
    victim.force_respawn();
    const auto& damage_values = tuning.projectile(ProjectileType::spray);
    const ProjectileSpray damage(RectangleF(0, 0, 10, 10), Vector2F::ZERO, PlayerTeam::a,
        3, Colour::white, app.render_resources(), resolve_projectile_strip(damage_values, *app.render_resources()), &damage_values);
    const float health_before = victim.health();
    victim.on_contact(damage, Vector2F::ZERO, 0.0f); tick();
    require(victim.health() < health_before, "Inactive motor control did not produce actual damage");
    no_motor_calls_since(damage_begin); note("inactive-actual-damage-motors-zero");

    DebugScriptRunner::Frame script;
    script.name = "background-bounded.cfg";
    script.statements = { { script.name, 1, "wait 4" }, { script.name, 2, "echo background-wait-complete" } };
    require(impl.scripts().push(std::move(script)).ok, "Cannot queue bounded background script");
    const int wait_begin = impl.level_updates();
    for (int count = 0; count < 6 && impl.scripts().running(); ++count) { tick(); }
    const bool script_echoed = std::any_of(impl.log().lines().begin(), impl.log().lines().end(),
        [](const DebugLogLine& line) { return line.text == "background-wait-complete"; });
    require(!impl.scripts().running() && script_echoed && impl.level_updates() >= wait_begin + 4,
        "Actual inactive updates did not complete the script wait"); note("inactive-script-wait");
    command("pause on"); frozen("Manual Debug pause was bypassed in background");
    command("pause off");
    DebugScriptRunner::Frame barrier;
    barrier.name = "background-disable-barrier.cfg";
    barrier.statements = { { barrier.name, 1, "script.background 0" },
        { barrier.name, 2, "echo background-held-sentinel" }, { barrier.name, 3, "restart" } };
    require(impl.scripts().push(std::move(barrier)).ok, "Cannot queue background self-disable barrier");
    const int held_updates = impl.level_updates(); tick();
    const auto sentinel_seen = [&]
    {
        return std::any_of(impl.log().lines().begin(), impl.log().lines().end(),
            [](const DebugLogLine& line) { return line.text == "background-held-sentinel"; });
    };
    require(!director.background_scripts() && impl.scripts().running() && !sentinel_seen() &&
        impl.level_updates() == held_updates && stack.depth() == 2,
        "Self-disable ran another inactive statement, update or queued restart");
    frozen("Self-disabled script progressed before activation");
    require(!sentinel_seen() && impl.scripts().running(), "Self-disable discarded or ran the held script");
    stack.notify_activation(true); tick(0.0f);
    require(sentinel_seen() && !impl.scripts().running() && impl.level_updates() == 0 && stack.depth() == 2,
        "Actual activation did not resume held statements and the requested restart");
    live(); command("script.background 1"); stack.notify_activation(false);
    note("manual-debug-pause-and-script-self-disable-barrier");
    stack.push(std::make_unique<ResumeProbeRoot>());
    command("script.background 0"); command("script.background 1");
    frozen("Opt-in toggle bypassed actual stack coverage");
    stack.pop(); tick(); note("covered-toggle-hard-gate");
    for (int gate = 0; gate < 3; ++gate)
    {
        facts = { gate != 0, gate == 1, gate == 2 }; probe();
        frozen("Injected hidden/minimized/power facts did not hard-freeze actual GameLevel");
        require(muted(), "Hard window gate restored inactive sound groups"); no_motors();
        note(gate == 0 ? "injected-hidden" : gate == 1 ? "injected-minimized" : "injected-power-suspended");
        facts = { true, false, false }; probe(); tick();
    }
    command("script.background 0");
    frozen("Turning background mode off did not freeze immediately");
    require(muted(), "Turning background mode off restored inactive audio"); note("off-inactive-freezes");
    // A current in-memory preference change is an independent restoration
    // witness. This fixture deliberately does not call a Save writer here.
    auto& current = const_cast<PlaySettings&>(preferences.play_settings());
    current.music_volume = 31; current.sfx_volume = 47;
    painter_scripted_pads::states[3].buttons = static_cast<std::uint16_t>(1u << static_cast<unsigned int>(GamepadButton::a));
    app.gamepads()->poll();
    stack.notify_activation(true);
    require(audio->group_volume(music) == current.music_gain() && audio->group_volume(sfx) == current.sfx_gain(),
        "Activation restored stale rather than current preferences");
    finish_count();
    require(!game->level().player(0).input_jump_pressed(), "Activation treated held Jump as a fresh press");
    painter_scripted_pads::states[3].buttons = 0; tick();
    painter_scripted_pads::states[3].buttons = static_cast<std::uint16_t>(1u << static_cast<unsigned int>(GamepadButton::a)); tick();
    require(game->level().player(0).input_jump_pressed(), "Activation failed to rearm ordinary Jump after release");
    painter_scripted_pads::states[3].buttons = 0; tick();
    note("off-then-activation-ordinary-countin-current-preferences");

    command("script.background 1");
    pad(GamepadButton::start);
    require(stack.depth() == 3, "Cannot open actual gameplay Pause menu");
    stack.notify_activation(false); command("script.background 0"); command("script.background 1");
    frozen("Background mode bypassed actual gameplay Pause"); note("actual-pause-menu-hard-gate");
    stack.notify_activation(true); pad(GamepadButton::a);
    require(stack.depth() == 2 && game->resume_remaining() > 0.0f, "Pause Resume did not start real count-in");
    stack.notify_activation(false);
    const float remaining = game->resume_remaining();
    frozen("Inactive background mode bypassed actual resume count-in");
    require(game->resume_remaining() == remaining, "Inactive resume count-in aged"); note("resume-countin-hard-gate");
    stack.notify_activation(true); finish_count();
    stack.notify_activation(false);
    painter_scripted_pads::states[1].connected = false; tick();
    require(stack.depth() == 3 && !app.gamepads()->connected(1),
        "Absent occupied pad did not retain truthful presence and pause actual GameLevel");
    frozen("Missing occupied pad lost match time in background"); note("missing-pad-real-pause");
    painter_scripted_pads::states[1].connected = true; tick(0.0f);
    stack.notify_activation(true); pad(GamepadButton::a, 1);
    require(stack.depth() == 2 && game->resume_remaining() > 0.0f,
        "Reconnected pause owner did not resume its actual menu");
    finish_count();

    // Seeded tape uses real inactive Level updates. Only seat 1 is a bot;
    // the two other seats remain neutral despite deliberately noisy readers.
    stack.notify_activation(false);
    painter_scripted_pads::states[1].left_stick = Vector2F(1, 0);
    painter_scripted_pads::states[1].right_stick = Vector2F(1, 0);
    painter_scripted_pads::states[1].right_trigger = 1.0f;
    app.keyboard()->on_key_down(Key::d); app.keyboard()->on_key_down(Key::space);
    command("bot 1 paint"); command("seed 197"); command("record background-bounded 420"); tick(0.0f);
    require(impl.recording() && muted(), "Inactive record rebuild did not mute before its first Level step");
    const size_t motor_begin = motors.count;
    bool paid_shot = false;
    for (int count = 0; count < 600 && impl.recording(); ++count)
    {
        require(muted(), "Background Music/SFX groups were not zero before a new update/shot");
        const float ammo = game->level().player(0).weapon_ammo();
        tick(); paid_shot = paid_shot || game->level().projectile_count() > 0 ||
            game->level().player(0).weapon_ammo() < ammo;
    }
    require(!impl.recording() && impl.level_updates() == 420 && paid_shot,
        "Bounded inactive bot recording did not complete actual paid shots/420 updates");
    const ReplayFile tape = ReplayFile::read(output / "debug-background-profile" / "ColourWars" / "replays" / "background-bounded.json");
    require(tape.updates == 420 && tape.inputs.size() == 3, "Inactive tape did not retain actual roster/update count");
    for (size_t view = 1; view < tape.inputs.size(); ++view)
    {
        for (const PlayerInputData& input : tape.inputs[view])
        {
            require(neutral(input) && input.connected, "Inactive recording retained physical actions or false occupied presence");
        }
    }
    no_motor_calls_since(motor_begin); note("inactive-bot-paid-shot-record");
    const size_t replay_motor_begin = motors.count;
    command("play background-bounded"); tick(0.0f);
    require(impl.playing() && !stack.active(), "Cannot arm actual inactive replay");
    for (int count = 0; count < 600 && impl.playing() && !impl.replay_finished(); ++count)
    { require(muted(), "Replay unmuted inactive groups"); tick(); }
    require(impl.replay_finished() && impl.level_updates() == tape.updates && impl.fingerprint().has_value() &&
        MatchFingerprint::difference(tape.final_print, *impl.fingerprint()).empty(),
        "Actual inactive replay did not reach its recorded final fingerprint");
    no_motor_calls_since(replay_motor_begin); note("inactive-replay-final-fingerprint");
    command("play stop"); command("pause off"); command("bot off");

    launch();
    require(!stack.active() && muted() && impl.level_updates() == 0,
        "New inactive GameLevel was not muted before its first update");
    tick(); require(impl.level_updates() == 1, "New inactive GameLevel did not inherit enabled policy");
    note("new-inactive-level-enabled");
    command("script.background 0"); launch();
    frozen("New inactive GameLevel bypassed default/disabled policy"); note("new-inactive-level-disabled");
    stack.clear();
    require(motors.factories == motors.destructions && save_bytes() == saved && cases.good(),
        "Background fixture leaked an owner, changed Save or could not write cases");
    std::ofstream summary(output / "debug-background-summary.json");
    summary << "{\"outcome\":\"passed\",\"cases\":" << case_count <<
        ",\"recorded_updates\":420,\"native_motor_calls\":0,\"save_unchanged\":true,"
        "\"limits\":\"Private HWND native visibility/minimize and forwarded synthetic power messages; "
        "actual GameLevel on hidden hardware shell with injected window facts and synthetic StateContext activation; "
        "scripted readers and recording motors only; no real OS focus/power-sleep, physical pad, listening, CI1 or performance acceptance\"}\n";
    require(summary.good(), "Cannot write background supplement summary");
    std::cout << "PASS Debug background supplement: actual GameLevel, neutral physical baseline, script/bot/replay, "
        "hard gates, immediate toggle, current-preference restoration and unchanged Save; synthetic activation only.\n";
}
#endif
