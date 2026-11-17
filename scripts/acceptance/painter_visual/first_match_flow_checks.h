// Local first-run rehearsal. Only synthetic input snapshots and silent display
// preferences are supplied; production match duration, rules and roster flow stay intact.
void first_match_flow(Application& app, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output, const crt_trial::Runtime& crt_runtime)
{
    const auto catalog = acceptance::manifest_catalog(content);
    StateContext& stack = app;
    struct Clear { StateContext& stack; ~Clear() { stack.clear(); painter_scripted_pads::states = {}; } } clear{stack};
    for (bool quick : {false, true})
    for (bool practice : {true, false})
    for (bool pad : {false, true})
    for (bool already_worn : {false, true})
    {
        if(quick && practice) continue;
        if(already_worn && !(quick && pad)) continue;
        stack.clear();
        const std::string prefix = std::string(pad ? "flow-pad" : "flow-keyboard") + (practice ? "-practice" : "-versus") + (quick ? "-quick" : "-full") + (already_worn ? "-worn" : "");
        const acceptance::IsolatedSave profile(output / (prefix + "-fresh-profile"));
        Save preferences; SettingsSession session(preferences); session.load_save_file();
        require(!preferences.last_match_setup(), "Flow profile was not fresh");
        PlaySettings quiet = session.play_settings(); quiet.music_volume = quiet.sfx_volume = 0; quiet.reduce_effects = true; quiet.crt_monitor=crt_runtime.enabled();
        require(session.set_play_settings_and_save(quiet), "Cannot isolate silent flow preferences");
        GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&session);
        data.set_tuning(&tuning); data.set_presentation(&presentation); data.set_stage_catalog(&catalog); data.set_crt_runtime(&crt_runtime);
        app.viewport_manager()->set_layout(ScreenLayout::one_player);
        app.resolution_manager()->set_resolution_exactly({1280,720});
        std::ignore = app.renderer()->window_size_changed(1280,720);
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        painter_scripted_pads::states = {}; painter_scripted_pads::states[0].connected = pad;
        for (Key key : {Key::enter,Key::escape,Key::up,Key::down,Key::left,Key::right,Key::d}) app.keyboard()->on_key_up(key);
        const auto tick = [&](float elapsed = 1.0f/60.0f)
        { poll_gamepads(app); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
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
        if(quick)
        {
            const InputBindings before = session.input_bindings();
            tap(GamepadButton::a); tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_right); tap(GamepadButton::a);
            capture("preflight-review");
            for(const Vector2I size : {Vector2I(1024,768),Vector2I(1280,720),Vector2I(1920,1080),Vector2I(2560,1080)})
            for(bool reduced : {false,true})
            {
                PlaySettings variant=quiet; variant.reduce_effects=reduced;
                require(session.set_play_settings_and_save(variant),"Cannot set isolated review effects");
                app.resolution_manager()->set_resolution_exactly(size);
                std::ignore=app.renderer()->window_size_changed(size.x,size.y);
                tick();
                capture("review-"+std::to_string(size.x)+"x"+std::to_string(size.y)+(reduced?"-reduced":"-motion"));
            }
            require(session.set_play_settings_and_save(quiet),"Cannot restore review effects");
            app.resolution_manager()->set_resolution_exactly({1280,720});
            std::ignore=app.renderer()->window_size_changed(1280,720);
            tap(GamepadButton::b);
            require(!preferences.last_match_setup() && session.input_bindings()==before,
                "Cancelling prototype changed saved setup/controls");
            tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_right); tap(GamepadButton::a);
            tap(GamepadButton::dpad_down); tap(GamepadButton::a); capture("preflight-customise");
            MatchSetup proposed; proposed.set_player_count(2); proposed.set_player_bot(1,BotDifficulty::normal);
            proposed.begin_input_claims(pad ? 0 : input_slots::keyboard_and_mouse);
            proposed.set_player_team(0,PlayerTeam::a); proposed.set_player_team(1,PlayerTeam::b);
            MenuInput reference_input(app.gamepads(),app.keyboard(),&tuning);
            MenuContext reference_context(&data,&reference_input,&proposed); StateContext reference_stack;
            reference_stack.push(std::make_unique<MainMenuModeSelect>(&reference_context));
            app.renderer()->begin_frame(); reference_stack.draw(*app.renderer()); const Frame reference=read(*app.renderer());
            require(reference.pixels==stack_frame(app).pixels,"Customise did not open the existing full Mode page");
            require(!preferences.last_match_setup() && session.input_bindings()==before,
                "Customise wrote a match before launch");
            reference_stack.clear(); stack.clear();
            // Direct real-page rehearsals isolate each interruption and compare
            // against an independently constructed full-route destination.
            for(int scenario=0;scenario<(pad ? 6 : 5);++scenario)
            {
                proposed=MatchSetup{}; proposed.set_player_count(2);
                proposed.set_player_bot(1,BotDifficulty::normal);
                proposed.begin_input_claims(pad ? 0 : input_slots::keyboard_and_mouse);
                proposed.set_player_team(0,PlayerTeam::a); proposed.set_player_team(1,PlayerTeam::b);
                reference_input.reset_actions();
                stack.push(std::make_unique<MainMenuJoin>(&reference_context,true)); tick(); tick();
                if(scenario==0) tap(GamepadButton::b); // Back retains the complete Count route.
                if(scenario==1) { tap(GamepadButton::dpad_right); tap(GamepadButton::a); } // Hard bot -> full Team.
                if(scenario==2) { tap(GamepadButton::dpad_right); tap(GamepadButton::dpad_right); tap(GamepadButton::a); } // Unclaimed human cannot continue.
                if(scenario==3) { tap(GamepadButton::a); tap(GamepadButton::b); } // Arena Back -> full tools.
                if(scenario==4) {
                    if(pad) painter_scripted_pads::states[0].buttons=static_cast<uint16_t>(1u<<static_cast<unsigned int>(GamepadButton::a));
                    else app.keyboard()->on_key_down(Key::enter);
                    for(int held=0;held<30;++held) tick();
                    painter_scripted_pads::states[0].buttons=0; app.keyboard()->on_key_up(Key::enter); tick(); tick();
                } // A held Join confirm must not also confirm/launch the arena.
                if(scenario==5) {
                    painter_scripted_pads::states[0].connected=false; tick(); tick();
                    const Frame waiting=stack_frame(app);
                    app.keyboard()->on_key_down(Key::enter); tick(); app.keyboard()->on_key_up(Key::enter); tick();
                    require(waiting.pixels==stack_frame(app).pixels,"Other device advanced disconnected host Join");
                    require(!preferences.last_match_setup(),"Disconnected host launched a match");
                    painter_scripted_pads::states[0].connected=true; tick(); tick(); tap(GamepadButton::a);
                }
                const Frame observed=stack_frame(app);
                StateContext destination;
                if(scenario==0) destination.push(std::make_unique<MainMenuPlayerCount>(&reference_context));
                if(scenario==1) destination.push(std::make_unique<MainMenuTeamSelect>(&reference_context));
                if(scenario==2) destination.push(std::make_unique<MainMenuJoin>(&reference_context,true));
                if(scenario==3) destination.push(std::make_unique<MainMenuWeaponSelect>(&reference_context));
                if(scenario==4 || scenario==5) destination.push(std::make_unique<MainMenuStageSelect>(&reference_context));
                app.renderer()->begin_frame(); destination.draw(*app.renderer()); const Frame expected_destination=read(*app.renderer());
                save(observed,output/(prefix+"-interruption-"+std::to_string(scenario)+".ppm"));
                require(observed.pixels==expected_destination.pixels,"Reviewed Join interruption differs from independent full-route destination");
                require(!preferences.last_match_setup(),"Join interruption committed an unstarted match");
                destination.clear(); stack.clear();
            }
            stack.push(std::make_unique<GameMenu>(&data)); tick(); tick();
            confirmations=0; directions=0; // timed/count comparison begins at fresh Title, after separate recovery rehearsal.
        }
        if(quick) {
            InputBindings remapped=session.input_bindings();
            set_keyboard_control(remapped,KeyboardAction::pause,DesktopControl{Key::p,MouseButton::none});
            remapped.pads[0][PadAction::pause]={PadControl::y,PadControl::none};
            require(session.set_input_bindings_and_save(remapped),"Cannot seed private remapped pause");
        }
        int test_profile=-1;
        if(quick && pad) {
            bool saved=false; test_profile=session.add_profile_and_save("QUICK",0,saved);
            require(saved && preferences.set_profile_boost_and_save(test_profile,2),"Cannot seed private optional profile");
            if(already_worn) require(session.wear_profile(0,test_profile),"Cannot wear private starter profile");
        }
        capture("title"); confirm("home");
        if(quick)
        {
            tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_right); confirm("review-preset");
            require(!preferences.last_match_setup(),"Review committed preset before acceptance");
            confirm("preset-join");
            if(pad && !already_worn) {
                painter_scripted_pads::states[0].buttons=static_cast<uint16_t>(1u<<static_cast<unsigned int>(GamepadButton::x));
                tick(); painter_scripted_pads::states[0].buttons=0; tick(); tick();
                require(session.worn_profile(0)==test_profile,"Existing Join did not wear chosen profile");
                capture("join-profile-chosen");
            }
            confirm("stage");
        }
        else
        {
        confirm("mode"); confirm("player-count");
        if(practice)
        {
            confirm("join-one"); confirm("tool"); confirm("sub"); confirm("stage");
        }
        else
        {
            tap(GamepadButton::dpad_down); confirm("join-two");
            tap(GamepadButton::dpad_right); capture("bot-filled"); confirm("teams");
            // Preserve #93's negative regression checks, separate from the
            // valid-path confirmation count being compared with the shortcut.
            const Frame neutral_team=stack_frame(app);
            tap(GamepadButton::a); --confirmations; capture("negative-neutral-ready-refused");
            const Frame refused_team=stack_frame(app);
            require(neutral_team.pixels!=refused_team.pixels,"Ready on an unchosen team was silently ignored");
            require(!preferences.last_match_setup(),"Rejected team Ready saved or started the match");
            tap(GamepadButton::a); --confirmations; capture("negative-neutral-ready-repeated");
            require(refused_team.pixels==stack_frame(app).pixels,"Repeated refused Ready changed team or warning");
            tap(GamepadButton::dpad_left); confirm("host-team-ready");
            tap(GamepadButton::dpad_down); tap(GamepadButton::dpad_right); confirm("tool");
            confirm("host-tool"); confirm("host-sub");
            tap(GamepadButton::dpad_down); confirm("bot-tool"); confirm("stage");
        }
        }
        int movement = -1;
        for(size_t i=0;i<catalog.stages().size();++i) if(catalog.stages()[i].name=="movement_course") movement=static_cast<int>(i);
        require(movement>=0,"Movement Course absent from real manifest");
        for(int i=0;i<=movement;++i) tap(GamepadButton::dpad_right);
        capture("movement-selected"); confirm("stage-confirm"); confirm("launch");
        for(int i=0;i<600 && !preferences.last_match_setup();++i) tick();
        require(preferences.last_match_setup() && preferences.last_match_setup()->stage=="movement_course" &&
            preferences.last_match_setup()->players.size()==(practice ? 1u : 2u),"Actual fresh setup did not launch selected Movement Course roster");
        if(quick)
        {
            const auto& chosen = *preferences.last_match_setup();
            require(chosen.players[0].team == PlayerTeam::a && chosen.players[1].team == PlayerTeam::b &&
                chosen.players[0].weapon == WeaponType::sprayer && chosen.players[1].weapon == WeaponType::sprayer &&
                chosen.players[1].controller == PlayerController::bot && chosen.players[1].difficulty == BotDifficulty::normal &&
                chosen.players[0].sub==SubType::grenade && chosen.players[1].sub==SubType::grenade &&
                chosen.players[0].handicap==(pad ? 2 : 0) && chosen.players[1].handicap==0 &&
                chosen.best_of==1 && !chosen.duration && chosen.variant==MatchVariant::standard,
                "Quick launch differs from disclosed preset");
            const auto& kept=session.play_settings();
            require(kept.reduce_effects==quiet.reduce_effects && kept.view_shake==quiet.view_shake &&
                kept.aim_marker==quiet.aim_marker && kept.aim_sensitivity==quiet.aim_sensitivity &&
                kept.crt_monitor==quiet.crt_monitor && kept.sticks==quiet.sticks &&
                kept.pad_comfort==quiet.pad_comfort && kept.pad_rumble==quiet.pad_rumble,
                "Quick launch changed comfort/graphics preferences");
        }
        capture("match-start");
        // Skip only the normal narrative, using the same visible proceed control.
        confirm("story-skipped");
        for(int i=0;i<240;++i) tick();
        if(quick) {
            if(pad) painter_scripted_pads::states[0].buttons=static_cast<uint16_t>(1u<<static_cast<unsigned int>(GamepadButton::y));
            else app.keyboard()->on_key_down(Key::p);
            tick(); painter_scripted_pads::states[0].buttons=0; app.keyboard()->on_key_up(Key::p); tick();
            require(stack.depth()==3,"Quick match did not use effective remapped pause");
            capture("remapped-pause"); tap(GamepadButton::b);
            require(stack.depth()==2,"Quick remapped pause Back did not resume");
        }
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
