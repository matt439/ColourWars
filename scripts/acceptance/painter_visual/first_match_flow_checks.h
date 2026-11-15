// Local first-run rehearsal. Only physical input snapshots and silent display
// preferences are supplied; production match duration, rules and roster flow stay intact.
void first_match_flow(Application& app, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto catalog = acceptance::manifest_catalog(content);
    StateContext& stack = app;
    struct Clear { StateContext& stack; ~Clear() { stack.clear(); painter_scripted_pads::states = {}; } } clear{stack};
    for (bool practice : {true, false})
    for (bool pad : {false, true})
    {
        stack.clear();
        const std::string prefix = std::string(pad ? "flow-pad" : "flow-keyboard") + (practice ? "-practice" : "-versus");
        const acceptance::IsolatedSave profile(output / (prefix + "-fresh-profile"));
        Save preferences; SettingsSession session(preferences); session.load_save_file();
        require(!preferences.last_match_setup(), "Flow profile was not fresh");
        PlaySettings quiet = session.play_settings(); quiet.music_volume = quiet.sfx_volume = 0; quiet.reduce_effects = true;
        require(session.set_play_settings_and_save(quiet), "Cannot isolate silent flow preferences");
        GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&session);
        data.set_tuning(&tuning); data.set_presentation(&presentation); data.set_stage_catalog(&catalog);
        app.viewport_manager()->set_layout(ScreenLayout::one_player);
        app.resolution_manager()->set_resolution_exactly({1280,720});
        std::ignore = app.renderer()->window_size_changed(1280,720);
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        painter_scripted_pads::states = {}; painter_scripted_pads::states[0].connected = pad;
        for (Key key : {Key::enter,Key::escape,Key::up,Key::down,Key::left,Key::right,Key::d}) app.keyboard()->on_key_up(key);
        const auto tick = [&](float elapsed = 1.0f/60.0f)
        { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
        tick(); tick();
        stack.push(std::make_unique<GameMenu>(&data)); tick(); tick();
        int confirmations = 0, directions = 0;
        std::ofstream log(output / (prefix + "-actions.csv"));
        log << "phase,confirmations,directions,stack_depth\n";
        const auto capture = [&](const std::string& phase)
        {
            save(stack_frame(app),output/(prefix+"-"+phase+".ppm"));
            log << phase << ',' << confirmations << ',' << directions << ',' << stack.depth() << '\n'; log.flush();
        };
        const auto tap = [&](GamepadButton button)
        {
            const Key key = button==GamepadButton::a ? Key::enter : (button==GamepadButton::b || button==GamepadButton::start) ? Key::escape :
                button==GamepadButton::dpad_down ? Key::down : button==GamepadButton::dpad_up ? Key::up :
                button==GamepadButton::dpad_left ? Key::left : Key::right;
            if(button==GamepadButton::a) ++confirmations;
            else if(button!=GamepadButton::b && button!=GamepadButton::start) ++directions;
            if(pad) painter_scripted_pads::states[0].buttons=static_cast<uint16_t>(1u<<static_cast<unsigned int>(button));
            else app.keyboard()->on_key_down(key);
            tick(); painter_scripted_pads::states[0].buttons=0; app.keyboard()->on_key_up(key); tick(); tick(0.51f);
        };
        const auto confirm = [&](const char* phase) { tap(GamepadButton::a); capture(phase); };
        capture("title"); confirm("home"); confirm("mode"); confirm("player-count");
        if(practice)
        {
            confirm("join-one"); confirm("tool"); confirm("sub"); confirm("stage");
        }
        else
        {
            tap(GamepadButton::dpad_down); confirm("join-two");
            tap(GamepadButton::dpad_right); capture("bot-filled"); confirm("teams");
            const Frame neutral_team = stack_frame(app);
            confirm("neutral-ready-refused");
            const Frame refused_team = stack_frame(app);
            require(neutral_team.pixels != refused_team.pixels, "Ready on an unchosen team was silently ignored");
            require(!preferences.last_match_setup(), "Rejected team Ready saved or started the match");
            confirm("neutral-ready-repeated");
            require(refused_team.pixels == stack_frame(app).pixels, "Repeated refused Ready changed the team or warning");
            tap(GamepadButton::dpad_left); confirm("host-team-ready");
            tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_right); confirm("tool");
            confirm("host-tool"); confirm("host-sub");
            tap(GamepadButton::dpad_down); confirm("bot-tool"); confirm("stage");
        }
        int movement = -1;
        for(size_t i=0;i<catalog.stages().size();++i) if(catalog.stages()[i].name=="movement_course") movement=static_cast<int>(i);
        require(movement>=0,"Movement Course absent from real manifest");
        for(int i=0;i<=movement;++i) tap(GamepadButton::dpad_right);
        capture("movement-selected"); confirm("stage-confirm"); confirm("launch");
        for(int i=0;i<600 && !preferences.last_match_setup();++i) tick();
        require(preferences.last_match_setup() && preferences.last_match_setup()->stage=="movement_course" &&
            preferences.last_match_setup()->players.size()==(practice ? 1u : 2u),"Actual fresh setup did not launch selected Movement Course roster");
        capture("match-start");
        // Skip only the normal narrative, using the same visible proceed control.
        confirm("story-skipped");
        for(int i=0;i<240;++i) tick();
        if(pad) painter_scripted_pads::states[0].left_stick.x=1.0f; else app.keyboard()->on_key_down(Key::d);
        for(int i=0;i<60;++i) tick();
        painter_scripted_pads::states[0].left_stick.x=0; app.keyboard()->on_key_up(Key::d); tick(); capture("playing");
        if(practice)
        {
            for(int i=0;i<15000;++i) tick();
            require(stack.depth()==2,"Untimed Movement Course unexpectedly ended");
            capture("untimed-after-250-seconds"); tap(GamepadButton::start); capture("practice-pause");
            require(stack.depth()==3,"Practice pause did not open");
            tap(GamepadButton::b); tick(); require(stack.depth()==2,"Practice Back did not resume");
            capture("practice-resumed");
            std::cout << "PASS fresh " << prefix << ": untimed Movement Course, pause/resume; confirmations=" << confirmations
                << " directions=" << directions << "; no physical-controller claim\n";
            stack.clear(); continue;
        }
        // Normal 240-second timer; no developer finish command or duration override.
        for(int i=0;i<18000 && stack.depth()==2;++i) tick();
        require(stack.depth()==3,"Natural match did not reach Results");
        const auto& look=presentation.results_menu();
        for(int i=0;i<static_cast<int>((look.team_fill_delay+look.team_fill_time+look.show_results_delay+2.0f)*60);++i) tick();
        capture("results"); confirm("end-menu");
        const auto* record=preferences.records().find("movement_course");
        require(record && record->versus_played==1,"First match did not record exactly once");
        confirm("rematch"); require(stack.depth()==2,"Rematch did not return to actual match");
        confirm("rematch-story-skipped"); for(int i=0;i<240;++i) tick(); capture("rematch-playing");
        require(preferences.records().find("movement_course")->versus_played==1,"Rematch prematurely recorded another result");
        std::cout << "PASS fresh " << prefix << ": local human+bot, default-duration natural results, Rematch; final confirmations="
            << confirmations << " directions=" << directions << "; no physical-controller claim\n";
        stack.clear();
    }
}
