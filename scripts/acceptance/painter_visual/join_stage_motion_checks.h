// Actual Join/Stage pages on the existing scripted reader seam. Envelopes use
// authored resting layout, independently of the production ornament draw code.
void join_stage_motion_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& output)
{
    Save preferences;
    auto& play = const_cast<PlaySettings&>(preferences.play_settings());
    play.music_volume = play.sfx_volume = 0; play.reduce_effects = false;
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false },
        { "close_quarters", "Close Quarters", "stage_close_quarters", true } };
    const StageList original_stages = stages;
    GameData data; data.set_application(&app); data.set_save(&preferences); data.set_level_infos(&levels);
    data.set_stages(&stages); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning); MatchSetup settings;
    MenuContext context(&data, &input, &settings); StateContext& stack = app;
    MenuPage* page = nullptr;
    struct ClearJoinStageMotion
    {
        StateContext& stack; MenuContext& context;
        ~ClearJoinStageMotion()
        {
            this->context.cancel_music_exit(); script_menu_music(this->context, {});
            this->stack.clear(); painter_scripted_pads::states = {};
        }
    } clear{ stack, context };
    const std::array<int, 4> slots{ input_slots::keyboard_and_mouse, 3, 0, 2 };
    const auto& style = presentation.menu(); const auto& look = presentation.main_menu();
    const auto motion = style.motion;
    const float settled = std::max(motion.focus_duration, motion.press_duration) + 0.01f;
    const auto bit = [](GamepadButton button)
        { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto key = [](GamepadButton button)
    {
        return button == GamepadButton::a ? Key::enter : button == GamepadButton::b ? Key::escape :
            button == GamepadButton::dpad_left ? Key::left : Key::right;
    };
    const auto tick = [&](float elapsed = 0.0f)
        { app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed); };
    const auto hold = [&](int slot, GamepadButton button, bool down)
    {
        if (slot == input_slots::keyboard_and_mouse)
        {
            if (down) { app.keyboard()->on_key_down(key(button)); }
            else { app.keyboard()->on_key_up(key(button)); }
        }
        else
        {
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit(button)) :
                static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button)));
        }
    };
    const auto tap = [&](int slot, GamepadButton button)
        { hold(slot, button, true); tick(); hold(slot, button, false); tick(); };
    const auto together = [&](int first, GamepadButton a, int second, GamepadButton b)
    {
        hold(first, a, true); hold(second, b, true); tick();
        hold(first, a, false); hold(second, b, false); tick();
    };
    const auto frame = [&]
        { app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer()); };
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    const auto join_gutter = [&](int row)
    {
        const auto font = app.render_resources()->resolve_sprite_font(style.item_font);
        const Vector2F text = app.render_resources()->measure_text(font, L"P1");
        const Vector2F origin = style.layout.position + Vector2F(0.0f, look.player_count_spacing.y * static_cast<float>(row + 2));
        // The row shadow can extend the supplied height, never into its gutter.
        return RectangleF(origin.x - style.focus_padding.x - 2.0f, origin.y - 2.0f,
            style.focus_padding.x + 3.0f, text.y + std::abs(style.item_shadow_offset.y) + 4.0f);
    };
    const auto stage_accent = [&]
    {
        const auto& briefing = look.stage_select.briefing;
        return RectangleF(briefing.position.x - 2.0f, briefing.position.y - 2.0f,
            briefing.size.x + 4.0f, look.stage_select.briefing_padding.y + 4.0f);
    };
    const RectangleF join_detail(style.layout.position.x - style.focus_padding.x - 10.0f,
        style.layout.position.y + look.player_count_spacing.y * 2.0f - 10.0f,
        1000.0f, look.player_count_spacing.y * 4.0f);
    const auto same_plan = [](const MatchSetup& a, const MatchSetup& b, bool stage = true)
    {
        if (a.player_count() != b.player_count() || (stage && a.stage() != b.stage()) ||
            a.best_of() != b.best_of() || a.duration() != b.duration() || a.screen_layout() != b.screen_layout())
            { return false; }
        const auto first = a.player_settings(), second = b.player_settings();
        for (size_t i = 0; i < first.size(); ++i)
            { if (first[i].num != second[i].num || first[i].team != second[i].team || first[i].weapon != second[i].weapon) { return false; } }
        return true;
    };
    const auto frozen = [&](const Frame& image)
    {
        const MatchSetup before = settings;
        std::srand(1373); const int next = std::rand(); std::srand(1373);
        require(image.pixels == frame().pixels && image.pixels == frame().pixels && std::rand() == next &&
            same_plan(settings, before), "Frozen Join/Stage drawing changed pixels, plan or RNG");
    };
    std::ofstream records(output / "join-stage-motion.csv");
    records << "case,width,height,delta_x,delta_y,delta_width,delta_height,responding_ornaments\n";
    const auto changed = [&](const char* name, const Frame& moving, const Frame& rest,
        const std::vector<RectangleF>& envelopes)
    {
        const RectangleI delta = foreground_delta_bounds(moving, rest);
        std::vector<RectangleI> allowed; std::vector<size_t> counts(envelopes.size(), 0);
        for (const auto& box : envelopes) { allowed.push_back(project(box)); }
        bool outside = false;
        for (int y = delta.y; y < delta.bottom(); ++y)
        {
            for (int x = delta.x; x < delta.right(); ++x)
            {
                if (same_pixel(moving, rest, hud_pixel_offset(moving, x, y))) { continue; }
                bool found = false;
                for (size_t i = 0; i < allowed.size(); ++i)
                {
                    if (x >= allowed[i].x && x < allowed[i].right() && y >= allowed[i].y && y < allowed[i].bottom())
                        { found = true; ++counts[i]; }
                }
                outside = outside || !found;
            }
        }
        const bool visible = !counts.empty() && std::all_of(counts.begin(), counts.end(), [](size_t n) { return n > 0; });
        if (outside || !visible)
        {
            save(moving, output / (std::string("join-stage-motion-") + name + "-failure-moving.ppm"));
            save(rest, output / (std::string("join-stage-motion-") + name + "-failure-rest.ppm"));
            std::cerr << "JOIN/STAGE " << name << " delta=" << delta.x << ',' << delta.y << ',' << delta.width << ',' <<
                delta.height << " outside=" << outside << " counts=";
            for (size_t count : counts) { std::cerr << count << ','; } std::cerr << '\n';
        }
        require(visible && !outside, "Join/Stage motion was absent or changed fixed text, preview or neighbouring rows");
        records << name << ',' << moving.width << ',' << moving.height << ',' << delta.x << ',' << delta.y << ',' <<
            delta.width << ',' << delta.height << ',' << envelopes.size() << '\n';
    };
    const auto capture = [&](const char* name, const Frame& image, RectangleF detail)
    {
        const std::string stem = std::string("join-stage-motion-") + name + '-' +
            std::to_string(image.width) + 'x' + std::to_string(image.height);
        save(image, output / (stem + ".ppm")); save(foreground_crop(image, project(detail)), output / (stem + "-detail.ppm"));
    };
    const auto open = [&](bool stage, bool complete = false, int initiating_slot = input_slots::keyboard_and_mouse)
    {
        stack.clear(); context.cancel_music_exit(); script_menu_music(context, {}); stack.notify_activation(true);
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key value : { Key::enter, Key::escape, Key::left, Key::right }) { app.keyboard()->on_key_up(value); }
        painter_scripted_pads::states = {};
        for (auto& state : painter_scripted_pads::states) { state.connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        settings = MatchSetup(); settings.set_player_count(4); settings.begin_input_claims(initiating_slot);
        settings.set_stage("movement_course"); settings.set_best_of(3); settings.set_duration(73.0f);
        settings.set_screen_layout(ScreenLayout::four_player);
        if (complete || stage)
        {
            for (size_t i = 1; i < slots.size(); ++i)
                { require(settings.claim_input_slot(slots[i]), "Join/Stage fixture could not claim sparse roster"); }
        }
        for (int player = 0; player < 4; ++player)
        {
            settings.set_player_team(player, player % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(player, player == 0 ? WeaponType::bucket : WeaponType::sprayer);
        }
        stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        std::unique_ptr<MenuPage> opened;
        if (stage) { opened = std::make_unique<MainMenuStageSelect>(&context); }
        else { opened = std::make_unique<MainMenuJoin>(&context); }
        page = opened.get(); stack.push(std::move(opened)); tick(); tick();
        const Frame first = frame(); tick(settled);
        require(stack.depth() == 2 && first.pixels == frame().pixels, "Join/Stage animated its initial settled page");
    };
    const auto reduced_and_lifecycle = [&](bool stage)
    {
        const int slot = stage ? slots[0] : slots[1];
        const auto trigger = [&] { tap(slot, stage ? GamepadButton::dpad_right : GamepadButton::a); };
        // A reduced draw is the page at rest. Reduce effects also holds the
        // Stage preview's drifting backdrop clouds at their anchors (POL#9), so
        // there the current rest is drawn reduced too, and the normal rest
        // must still match it everywhere outside the preview image.
        const auto at_rest = [&](const Frame& reduced, const char* message)
        {
            const Frame normal = frame();
            if (!stage) { require(reduced.pixels == normal.pixels, message); return; }
            play.reduce_effects = true; const Frame reduced_rest = frame(); play.reduce_effects = false;
            const auto& stage_look = look.stage_select;
            const RectangleI preview = project(RectangleF(stage_look.preview_position, stage_look.preview_size));
            bool outside_same = normal.width == reduced_rest.width && normal.height == reduced_rest.height;
            for (int y = 0; outside_same && y < normal.height; ++y)
            {
                for (int x = 0; x < normal.width; ++x)
                {
                    if (x >= preview.x && x < preview.right() && y >= preview.y && y < preview.bottom()) { continue; }
                    if (!same_pixel(normal, reduced_rest, hud_pixel_offset(normal, x, y))) { outside_same = false; break; }
                }
            }
            require(reduced.pixels == reduced_rest.pixels && outside_same, message);
        };
        open(stage); trigger(); const Frame moving = frame(); frozen(moving);
        play.reduce_effects = true; const Frame reduced = frame(); play.reduce_effects = false;
        require(moving.pixels == frame().pixels, "Reduced Join/Stage drawing advanced its cosmetic clock");
        tick(settled); at_rest(reduced, "Applied Reduce effects did not settle Join/Stage immediately");
        open(stage); trigger(); stack.notify_activation(false); const Frame inactive = frame();
        stack.notify_activation(true); tick(settled);
        require(inactive.pixels == frame().pixels, "Activation retained or restarted Join/Stage motion");
        open(stage); trigger(); play.reduce_effects = true; const Frame covered = frame(); play.reduce_effects = false;
        stack.push(std::make_unique<MainMenuCredits>(&context)); stack.pop();
        at_rest(covered, "A test-pushed covering page retained Join/Stage motion");
        tick();
        open(stage); trigger(); play.reduce_effects = true; const Frame debug_rest = frame(); play.reduce_effects = false;
        page->debug_reset_input(slots[0]);
        at_rest(debug_rest, "Developer reset retained Join/Stage motion");
        tick(settled); at_rest(debug_rest, "Developer reset restarted Join/Stage motion");
        open(stage); play.reduce_effects = true; trigger(); const Frame quiet = frame(); tick(settled);
        require(quiet.pixels == frame().pixels, "Reduced-effects Join/Stage input animated"); play.reduce_effects = false;
        open(stage); trigger(); const int missing_slot = stage ? slots[2] : slots[1];
        painter_scripted_pads::states[static_cast<size_t>(missing_slot)].connected = false; tick();
        const Frame missing = frame(); tick(settled);
        require(missing.pixels == frame().pixels, "Missing-device refresh retained Join/Stage motion");
        painter_scripted_pads::states[static_cast<size_t>(missing_slot)].connected = true;
        hold(missing_slot, GamepadButton::a, true); tick(); tick(); hold(missing_slot, GamepadButton::a, false); tick(); tick();
        const Frame reconnected = frame(); tick(settled);
        require(reconnected.pixels == frame().pixels && stack.depth() == 2 && !context.music_exit_pending(),
            "Held reconnection replayed Join/Stage input or motion");
    };
    const auto destination = [&](bool team)
    {
        MatchSetup plan = settings; MenuInput reference_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext reference_context(&data, &reference_input, &plan); StateContext reference_stack;
        if (team) { reference_stack.push(std::make_unique<MainMenuTeamSelect>(&reference_context)); }
        else { reference_stack.push(std::make_unique<MainMenuPlayerCount>(&reference_context)); }
        app.renderer()->begin_frame(); reference_stack.draw(*app.renderer()); return read(*app.renderer());
    };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        open(false); tap(slots[1], GamepadButton::a); const Frame joined = frame(); frozen(joined);
        tick(0.0f); tick(-0.5f); require(joined.pixels == frame().pixels, "Nonpositive time advanced Join motion");
        tick(motion.press_duration * 0.4f); const Frame join_middle = frame(); tick(settled); const Frame join_rest = frame();
        changed("join-claim", joined, join_rest, { join_gutter(1) });
        changed("join-middle", join_middle, join_rest, { join_gutter(1) });
        require(joined.pixels != join_middle.pixels, "Join acknowledgement did not progress with positive time");
        tap(slots[1], GamepadButton::a); require(join_rest.pixels == frame().pixels, "Duplicate Join claim invented motion");
        tap(slots[2], GamepadButton::a); tick(settled);
        tap(slots[1], GamepadButton::b); const Frame released = frame(); tick(settled); const Frame release_rest = frame();
        changed("join-release-hole", released, release_rest, { join_gutter(1) });
        require(settings.player_index_for_slot(slots[1]) == -1 && settings.player_index_for_slot(slots[2]) == 2,
            "Join release compacted its stable roster rows");
        together(slots[1], GamepadButton::a, slots[3], GamepadButton::a); const Frame simultaneous = frame();
        tick(settled); const Frame simultaneous_rest = frame();
        changed("join-simultaneous", simultaneous, simultaneous_rest, { join_gutter(1), join_gutter(3) });
        require(settings.input_roster_complete() && settings.player_index_for_slot(slots[2]) == 2,
            "Simultaneous Join claims changed stable roster assignment");
        tap(1, GamepadButton::a); require(simultaneous_rest.pixels == frame().pixels, "A full-roster claim invented motion");
        // Slot zero is processed before slot one: release and replacement share
        // one middle row in the same update, while the other rows retain slots.
        together(0, GamepadButton::b, 1, GamepadButton::a); const Frame replacement = frame();
        tick(settled); changed("join-same-update-replacement", replacement, frame(), { join_gutter(2) });
        require(settings.input_roster_complete() && settings.player_index_for_slot(0) == -1 &&
            settings.player_index_for_slot(1) == 2 && settings.player_index_for_slot(3) == 3 &&
            settings.player_index_for_slot(2) == 1, "Same-update Join replacement moved an unrelated stable row");
        tap(1, GamepadButton::b); tap(1, GamepadButton::a); const Frame rapid = frame();
        tick(motion.press_duration * 0.4f); const Frame rapid_middle = frame(); tick(settled);
        changed("join-rapid-restart", rapid, frame(), { join_gutter(2) });
        changed("join-rapid-middle", rapid_middle, frame(), { join_gutter(2) });
        require(rapid.pixels != rapid_middle.pixels && settings.player_index_for_slot(1) == 2,
            "Rapid Join release/reclaim did not restart one bounded stable-row response");
        capture("join-claim", joined, join_detail); capture("join-middle", join_middle, join_detail);
        capture("join-rest", join_rest, join_detail); capture("join-release", released, join_detail);
        capture("join-simultaneous", simultaneous, join_detail);
        open(false); tap(slots[1], GamepadButton::a); tap(slots[2], GamepadButton::a); tick(settled);
        together(slots[0], GamepadButton::a, slots[3], GamepadButton::a);
        require(stack.depth() == 2 && settings.input_roster_complete(), "The final Join claim bypassed its snapshot barrier");
        const Frame final_claim = frame(); tick(settled); changed("join-final-barrier", final_claim, frame(), { join_gutter(3) });
        tap(slots[0], GamepadButton::a); const Frame team = frame(); tick(settled);
        require(stack.depth() == 2 && team.pixels == frame().pixels && team.pixels == destination(true).pixels,
            "Join continuation delayed or carried motion into Team Select");
        reduced_and_lifecycle(false);
        open(false); tap(slots[1], GamepadButton::a); require(settings.release_input_slot(slots[1]), "Cannot externally release Join seat");
        tick(); const Frame external_release = frame(); tick(settled);
        require(external_release.pixels == frame().pixels, "External roster replacement replayed accepted Join motion");
        open(false, false, 3); painter_scripted_pads::states[3].connected = false; tick(); const Frame missing_p1 = frame();
        tap(0, GamepadButton::a); require(settings.player_index_for_slot(0) == 1 && stack.depth() == 2,
            "Missing P1 prevented another device from claiming a stable Join row");
        tick(settled); tap(0, GamepadButton::b);
        require(stack.depth() == 2 && settings.player_index_for_slot(0) == 1 && frame().pixels != missing_p1.pixels &&
            frame().pixels == destination(false).pixels,
            "Missing-P1 rescue Back did not return to Player Count");

        open(true); const MatchSetup provisional = settings;
        const auto browse = [&](GamepadButton direction)
        {
            std::srand(1571); const int next = std::rand(); std::srand(1571); tap(slots[0], direction);
            require(std::rand() == next && same_plan(settings, provisional) && !context.music_exit_pending(),
                "Stage browsing changed the provisional plan or consumed RNG");
        };
        browse(GamepadButton::dpad_right); const Frame stage_arrival = frame(); frozen(stage_arrival);
        tick(0.0f); tick(-0.5f); require(stage_arrival.pixels == frame().pixels, "Nonpositive time advanced Stage motion");
        tick(motion.focus_duration * 0.4f); const Frame stage_middle = frame(); tick(settled); const Frame stage_rest = frame();
        changed("stage-browse", stage_arrival, stage_rest, { stage_accent() });
        changed("stage-middle", stage_middle, stage_rest, { stage_accent() });
        require(stage_arrival.pixels != stage_middle.pixels, "Stage accent did not progress with positive time");
        browse(GamepadButton::dpad_right); const Frame random_arrival = frame(); tick(settled); const Frame random_rest = frame();
        changed("stage-random", random_arrival, random_rest, { stage_accent() });
        browse(GamepadButton::dpad_right); tick(settled); browse(GamepadButton::dpad_left); const Frame wrapped = frame();
        tick(settled); require(random_rest.pixels == frame().pixels, "Stage wrapping failed to restore exact Random raster");
        changed("stage-random-wrap", wrapped, random_rest, { stage_accent() });
        tap(slots[0], GamepadButton::a); const Frame locked = frame(); tick(settled); const Frame locked_rest = frame();
        changed("stage-lock", locked, locked_rest, { stage_accent() });
        tap(slots[0], GamepadButton::dpad_right); require(locked_rest.pixels == frame().pixels,
            "Locked Stage arrows invented motion or changed selection");
        tap(slots[1], GamepadButton::b); const Frame unlocked = frame(); tick(settled);
        changed("stage-unlock", unlocked, frame(), { stage_accent() });
        require(random_rest.pixels == frame().pixels && same_plan(settings, provisional), "Stage unlock changed its provisional plan");
        capture("stage-arrival", stage_arrival, stage_accent()); capture("stage-middle", stage_middle, stage_accent());
        capture("stage-rest", stage_rest, stage_accent()); capture("stage-random", random_arrival, stage_accent());
        capture("stage-lock", locked, stage_accent()); capture("stage-unlock", unlocked, stage_accent());
        tap(slots[0], GamepadButton::a); tick(settled); const Frame outgoing_random = frame();
        hold(slots[0], GamepadButton::a, true); tick(); static_cast<WindowNotify&>(app).on_deactivated(); tick();
        static_cast<WindowNotify&>(app).on_activated(); tick();
        hold(slots[0], GamepadButton::a, false); tick(); tick();
        require(outgoing_random.pixels == frame().pixels && same_plan(settings, provisional) && !context.music_exit_pending(),
            "Interrupted Stage confirmation accepted its stale release");
        int music_updates = 0; script_menu_music(context, [&](float) { ++music_updates; });
        hold(slots[0], GamepadButton::a, true); tick();
        std::srand(4937); std::ignore = std::rand(); const int after_pick = std::rand(); std::srand(4937);
        hold(slots[0], GamepadButton::a, false); tick();
        require(std::rand() == after_pick && settings.stage() == "close_quarters" && same_plan(settings, provisional, false) &&
            context.music_exit_pending() && stack.depth() == 2 && music_updates == 1 && outgoing_random.pixels == frame().pixels,
            "Final Random start consumed wrong RNG, changed plan, refreshed its outgoing page or restarted motion");
        const MatchSetup committed = settings;
        tap(slots[0], GamepadButton::dpad_left); tap(slots[1], GamepadButton::b); tap(slots[0], GamepadButton::a); tick(settled);
        require(outgoing_random.pixels == frame().pixels && same_plan(settings, committed) && context.music_exit_pending(),
            "Pending Stage fade accepted input or refreshed Random visuals");
        capture("stage-pending-random", frame(), stage_accent()); context.complete_music_exit(); context.complete_music_exit();
        require(stack.depth() == 1 && !context.music_exit_pending(), "Completing Stage fade duplicated its state pop");
        // Final confirmation can arrive before the lock acknowledgement has
        // expired. Start discards that cosmetic response without refreshing
        // the outgoing Random widgets to the chosen concrete arena.
        open(true); tap(slots[0], GamepadButton::dpad_right); tap(slots[0], GamepadButton::dpad_right);
        tap(slots[0], GamepadButton::a); const Frame rapid_lock = frame();
        play.reduce_effects = true; const Frame rapid_lock_rest = frame(); play.reduce_effects = false;
        changed("stage-rapid-lock", rapid_lock, rapid_lock_rest, { stage_accent() });
        script_menu_music(context, [](float) {});
        std::srand(4937); std::ignore = std::rand(); const int after_rapid_pick = std::rand(); std::srand(4937);
        tap(slots[0], GamepadButton::a);
        require(std::rand() == after_rapid_pick && settings.stage() == "close_quarters" && context.music_exit_pending() &&
            rapid_lock_rest.pixels == frame().pixels, "Rapid final Stage start retained lock motion or refreshed outgoing Random");
        const MatchSetup rapid_committed = settings; const Frame rapid_pending = frame(); frozen(rapid_pending);
        static_cast<WindowNotify&>(app).on_deactivated(); tick(); static_cast<WindowNotify&>(app).on_activated(); tick();
        tap(slots[0], GamepadButton::dpad_right); tap(slots[0], GamepadButton::a); tap(slots[1], GamepadButton::b); tick(settled);
        require(rapid_pending.pixels == frame().pixels && same_plan(settings, rapid_committed) && context.music_exit_pending(),
            "Focus interruption or repeated input changed a rapid final Stage fade");
        capture("stage-rapid-start", rapid_pending, stage_accent()); context.complete_music_exit();
        require(stack.depth() == 1 && !context.music_exit_pending(), "Rapid Stage fade did not complete once");
        reduced_and_lifecycle(true);
        const Frame before_resize = frame();
        app.resolution_manager()->set_resolution_exactly({ 1600, 1200 });
        std::ignore = app.renderer()->window_size_changed(1600, 1200); std::ignore = frame();
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(before_resize.pixels == frame().pixels, "Settled Stage changed after resize round-trip");
        stages = { original_stages.front() }; open(true); const Frame only_stage = frame();
        tap(slots[0], GamepadButton::dpad_left); tap(slots[0], GamepadButton::dpad_right);
        require(only_stage.pixels == frame().pixels && settings.stage() == "movement_course",
            "A single excluded arena invented Random or a selection pulse");
        stages = original_stages;
        std::cout << "PASS native Join/Stage motion " << size.x << 'x' << size.y << '\n';
    }
    require(records.good(), "Join/Stage motion evidence write failed");
    require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE && GetForegroundWindow() != static_cast<HWND>(app.window()),
        "Join/Stage probes exposed or activated their hidden window");
    std::cout << "PASS actual Join/Stage motion: independent fixed-layout envelopes, stable holes and simultaneous claims, "
        "duplicate/full refusal and final snapshot barrier; real/Random wraps, lock/unlock and latched eligible Random start, "
        "nonpositive time, reduced effects, activation/cover/debug/missing/reconnect resets, frozen draw/RNG and resize; scripted reader only\n";
}
