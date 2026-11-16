// Local research rehearsal: real production menus/matches, synthetic device snapshots.
void duel_flow(Application& app, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& content, const std::filesystem::path& output,
    const crt_trial::Runtime& crt_runtime)
{
    const auto catalog=acceptance::manifest_catalog(content);
    StateContext& stack=app;
    for(bool keyboard_host:{true,false})
    {
        stack.clear();
        const std::string prefix=keyboard_host ? "duel-keyboard-pad4" : "duel-pad3-keyboard";
        const acceptance::IsolatedSave isolated(output/(prefix+"-profile"));
        Save preferences; SettingsSession session(preferences); session.load_save_file();
        PlaySettings prefs=session.play_settings(); prefs.music_volume=prefs.sfx_volume=0; prefs.reduce_effects=true; prefs.crt_monitor=crt_runtime.enabled();
        require(session.set_play_settings_and_save(prefs),"Cannot seed private duel preferences");
        GameData data; data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&session);
        data.set_tuning(&tuning); data.set_presentation(&presentation); data.set_stage_catalog(&catalog); data.set_crt_runtime(&crt_runtime);
        const int pad=keyboard_host ? 3 : 2;
        const int host=keyboard_host ? input_slots::keyboard_and_mouse : pad;
        const int guest=keyboard_host ? pad : input_slots::keyboard_and_mouse;
        app.viewport_manager()->set_layout(ScreenLayout::one_player);
        app.resolution_manager()->set_resolution_exactly({1280,720});
        std::ignore=app.renderer()->window_size_changed(1280,720);
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        painter_scripted_pads::states={}; painter_scripted_pads::states[pad].connected=true;
        for(Key k:{Key::enter,Key::escape,Key::left,Key::right,Key::up,Key::down,Key::d}) app.keyboard()->on_key_up(k);
        const auto tick=[&]{app.gamepads()->poll();app.keyboard()->poll();app.mouse()->poll();stack.update(1.0f/60);};
        int confirms=0,directions=0;
        std::ofstream log(output/(prefix+"-actions.csv"));log<<"phase,confirmations,directions\n";
        const auto snap=[&](const std::string& phase){save(stack_frame(app),output/(prefix+"-"+phase+".ppm"));log<<phase<<','<<confirms<<','<<directions<<'\n';log.flush();};
        const auto tap=[&](int slot,GamepadButton button){
            Key k=button==GamepadButton::a ? Key::enter : button==GamepadButton::b ? Key::escape :
                button==GamepadButton::dpad_down ? Key::down : button==GamepadButton::dpad_up ? Key::up :
                button==GamepadButton::dpad_left ? Key::left : Key::right;
            if(button==GamepadButton::a)++confirms;else if(button!=GamepadButton::b)++directions;
            if(input_slots::is_pad(slot))painter_scripted_pads::states[slot].buttons=static_cast<uint16_t>(1u<<static_cast<unsigned>(button));
            else app.keyboard()->on_key_down(k);
            tick();painter_scripted_pads::states[pad].buttons=0;app.keyboard()->on_key_up(k);tick();tick();
        };
        const auto a=[&](int slot,const char* phase){tap(slot,GamepadButton::a);snap(phase);};
        tick();tick();stack.push(std::make_unique<GameMenu>(&data));tick();tick();snap("title");
        a(host,"home");a(host,"mode");a(host,"count");tap(host,GamepadButton::dpad_down);a(host,"join");
        a(guest,"guest-joined");a(host,"teams");
        tap(host,GamepadButton::dpad_left);a(host,"host-team-ready");
        tap(guest,GamepadButton::dpad_right);a(guest,"tools");
        a(host,"host-tool");a(host,"host-sub");a(guest,"guest-tool");a(guest,"stage");
        int movement=-1;for(size_t i=0;i<catalog.stages().size();++i)if(catalog.stages()[i].name=="crossfeed")movement=static_cast<int>(i);
        require(movement>=0,"Crossfeed missing");
        for(int i=0;i<=movement;++i)tap(host,GamepadButton::dpad_right);
        a(host,"arena-locked");a(host,"launch");
        for(int i=0;i<600&&!preferences.last_match_setup();++i)tick();
        require(preferences.last_match_setup()&&preferences.last_match_setup()->players.size()==2,"Duel did not launch");
        const auto& chosen=*preferences.last_match_setup();
        require(chosen.players[0].controller==PlayerController::human&&chosen.players[1].controller==PlayerController::human&&
            chosen.players[0].team==PlayerTeam::a&&chosen.players[1].team==PlayerTeam::b,"Duel roster changed");
        const auto launched=confirms;const auto launch_directions=directions;
        a(guest,"story-skip");for(int i=0;i<240;++i)tick();snap("playing");
        // Both real human input channels participate; no bot or developer finish.
        painter_scripted_pads::states[pad].left_stick.x=keyboard_host ? -1.0f : 1.0f;
        app.keyboard()->on_key_down(Key::d);
        for(int i=0;i<90;++i)tick();
        painter_scripted_pads::states[pad].left_stick.x=0;app.keyboard()->on_key_up(Key::d);
        painter_scripted_pads::states[pad].right_stick=Vector2F(keyboard_host ? -1.0f : 1.0f,-0.5f);
        painter_scripted_pads::states[pad].right_trigger=1.0f;
        for(int i=0;i<600;++i)tick();
        painter_scripted_pads::states[pad].right_trigger=0;
        painter_scripted_pads::states[pad].right_stick=Vector2F::zero;
        for(int i=0;i<18000&&stack.depth()==2;++i)tick();
        require(stack.depth()==3,"Duel did not naturally finish");
        const auto& look=presentation.results_menu();
        for(int i=0;i<static_cast<int>((look.team_fill_delay+look.team_fill_time+look.show_results_delay+2)*60);++i)tick();
        snap("results");
        require(preferences.records().find("crossfeed")&&preferences.records().find("crossfeed")->versus_played==1,"Duel record count wrong");
        a(guest,"end");a(guest,"rematch");a(host,"rematch-story-skip");for(int i=0;i<240;++i)tick();
        require(stack.depth()==2&&preferences.records().find("crossfeed")->versus_played==1,"Rematch failed or double-counted");
        snap("rematch-playing");
        const int to_rematch=confirms;
        painter_scripted_pads::states[pad].connected=false;tick();tick();
        require(stack.depth()==3,"Disconnected human did not pause rematch");snap("rematch-pad-missing");
        tap(input_slots::keyboard_and_mouse,GamepadButton::a);
        require(stack.depth()==3,"Keyboard bypassed missing human pad");
        painter_scripted_pads::states[pad].connected=true;tick();tick();tap(pad,GamepadButton::a);
        require(stack.depth()==2,"Reconnected original pad did not resume rematch");snap("rematch-reconnected");
        for(int slot:{host,guest}) {
            if(input_slots::is_pad(slot))painter_scripted_pads::states[slot].buttons=static_cast<uint16_t>(1u<<static_cast<unsigned>(GamepadButton::start));
            else app.keyboard()->on_key_down(Key::escape);
            tick();painter_scripted_pads::states[pad].buttons=0;app.keyboard()->on_key_up(Key::escape);tick();
            require(stack.depth()==3,"Human input channel lost rematch pause control");
            tap(slot,GamepadButton::b);require(stack.depth()==2,"Human could not resume own rematch pause");
        }
        snap("rematch-both-controls-proved");
        std::cout<<"PASS "<<prefix<<": launch="<<launched<<" confirms / "<<launch_directions<<" directions; through rematch="<<to_rematch<<"; synthetic devices only\n";
        stack.clear();painter_scripted_pads::states={};
    }
}
