// Production setup pages, with the same bounded scripted reader used by the
// team-setup fixture. Every pixel envelope below is built independently from
// the authored resting layout; no probe changes production widget geometry.
void setup_motion_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, GameTuning& tuning, Presentation& presentation,
    const std::filesystem::path& output)
{
    struct SetupMotionProfile
    {
        std::wstring previous;
        explicit SetupMotionProfile(const std::filesystem::path& root)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0,
                "Cannot read the isolated setup-motion profile");
            if (raw != nullptr) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(root);
            require(_wputenv_s(L"LOCALAPPDATA", root.c_str()) == 0,
                "Cannot isolate setup-motion preferences");
        }
        ~SetupMotionProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
    } isolated(output / "setup-motion-profile");
    Save preferences; SettingsSession preferences_session(preferences);
    PlaySettings initial; initial.music_volume = initial.sfx_volume = 0; initial.reduce_effects = false;
    require(preferences_session.set_play_settings_and_save(initial), "Cannot seed setup-motion preferences");
    auto& play = const_cast<PlaySettings&>(preferences_session.play_settings());
    StageList stages{ { "movement_course", "Movement Course", "stage_turbulence", false } };
    GameData data;
    data.set_application(&app); data.set_save(&preferences); data.set_settings_session(&preferences_session);
    const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "movement_course" }); data.set_stage_catalog(&stage_catalog); data.set_tuning(&tuning); data.set_presentation(&presentation);
    MenuInput input(app.gamepads(), app.keyboard(), &tuning);
    MatchSetup settings;
    MenuContext context(&data, &input, &settings);
    StateContext& stack = app;
    MenuPage* active_page = nullptr;
    struct ClearSetupMotion
    {
        StateContext& stack;
        ~ClearSetupMotion() { this->stack.clear(); painter_scripted_pads::states = {}; }
    } clear{ stack };
    const std::array<int, 4> slots{ input_slots::keyboard_and_mouse, 3, 0, 2 };
    const std::array<WeaponType, 4> weapons{ WeaponType::sprayer, WeaponType::sniper,
        WeaponType::roller, WeaponType::bucket };
    const MenuStyle& style = presentation.menu();
    const MainMenuPresentation& look = presentation.main_menu();
    const MenuFocusMotionPresentation motion = style.motion;
    const float settle_time = std::max(motion.focus_duration, motion.press_duration) + 0.01f;
    require(motion.arrow_travel > 0.0f && motion.press_fraction > 0.0f &&
        motion.selection_inset_fraction > 0.0f, "Shipped setup-motion responses must be visible");
    const auto tick = [&](float elapsed = 0.0f)
    {
        app.gamepads()->poll(); app.keyboard()->poll(); app.mouse()->poll(); stack.update(elapsed);
    };
    const auto bit = [](GamepadButton button)
        { return static_cast<uint16_t>(1u << static_cast<unsigned int>(button)); };
    const auto key_for = [](GamepadButton button)
    {
        return button == GamepadButton::x ? Key::r : button == GamepadButton::a ? Key::enter :
            button == GamepadButton::b ? Key::escape : button == GamepadButton::dpad_up ? Key::up :
            button == GamepadButton::dpad_down ? Key::down :
            button == GamepadButton::dpad_left ? Key::left : Key::right;
    };
    const auto hold = [&](int player, GamepadButton button, bool down)
    {
        const int slot = slots[static_cast<size_t>(player)];
        if (slot == input_slots::keyboard_and_mouse)
        {
            if (down) { app.keyboard()->on_key_down(key_for(button)); }
            else { app.keyboard()->on_key_up(key_for(button)); }
        }
        else
        {
            auto& buttons = painter_scripted_pads::states[static_cast<size_t>(slot)].buttons;
            buttons = down ? static_cast<uint16_t>(buttons | bit(button)) :
                static_cast<uint16_t>(buttons & static_cast<uint16_t>(~bit(button)));
        }
    };
    const auto tap = [&](int player, GamepadButton button)
        { hold(player, button, true); tick(); hold(player, button, false); tick(); };
    const auto together = [&](GamepadButton first, GamepadButton second)
    {
        hold(0, first, true); hold(1, second, true); tick();
        hold(0, first, false); hold(1, second, false); tick();
    };
    const auto frame = [&]
        { app.renderer()->begin_frame(); stack.draw(*app.renderer()); return read(*app.renderer()); };
    const auto project = [&](RectangleF bounds)
    {
        const Camera camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
            app.viewport_manager()->fullscreen_viewport());
        camera.calculate_view_rectangle(bounds); return countdown_crop_bounds(bounds);
    };
    const auto setup_bounds = [&](Vector2F spacing, int row, const std::wstring& words)
    {
        const Vector2F origin = style.layout.position + Vector2F(0.0f, spacing.y * static_cast<float>(row));
        const colourwars::UiTextDropShadow text("independent-setup-motion-text", words, style.item_font, origin,
            app.render_resources(), look.art.ink, Colour(0.0f, 0.0f, 0.0f, 0.0f), style.item_shadow_offset);
        return text.bounds();
    };
    const auto focus_arrow = [&](Vector2F spacing, int row, const std::wstring& words)
    {
        RectangleF bounds = setup_bounds(spacing, row, words); bounds.inflate(look.setup_focus_padding);
        const float pen = look.art.keyline_width;
        return RectangleF(bounds.x - pen * 8.0f - 2.0f, bounds.center().y - pen * 4.0f - 2.0f,
            pen * 10.0f + 4.0f, pen * 8.0f + 4.0f);
    };
    const auto duration_card = [&]
    {
        const Vector2F spacing = look.mode_select_spacing;
        std::wostringstream words; words << settings.duration().value_or(tuning.match().duration) / 60.0f << L" min";
        const std::wstring value = settings.duration() ? words.str() : L"Default (" + words.str() + L")";
        const colourwars::UiTextDropShadow text("independent-setup-motion-value", value, style.item_font,
            style.layout.position + Vector2F(spacing.x * 2.0f, spacing.y * 5.0f), app.render_resources(),
            look.art.ink, Colour(0.0f, 0.0f, 0.0f, 0.0f), style.item_shadow_offset);
        RectangleF bounds = RectangleF::union_of(setup_bounds(spacing, 5,
            presentation.menu_text().item(MenuItem::match_length)), text.bounds());
        bounds.inflate(look.setup_focus_padding + Vector2F(2.0f)); return bounds;
    };
    const auto team_box = [&](int player, PlayerTeam team)
    {
        const int choice = team == PlayerTeam::a ? 0 : team == PlayerTeam::b ? 2 : 1;
        RectangleF box(style.layout.position + Vector2F(look.team_select.spacing.x * 2.0f +
            look.team_select.size.x * static_cast<float>(choice),
            look.team_select.spacing.y * static_cast<float>(player + 2)), look.team_select.size);
        box.inflate(2.0f); return box;
    };
    const auto weapon_box = [&](int player, bool ornament)
    {
        RectangleF box(style.layout.position + Vector2F(look.weapon_select.spacing.x * 2.0f,
            look.weapon_select.spacing.y * static_cast<float>(player + 2)), look.weapon_select.size);
        if (ornament) { box.inflate(style.icon_padding + Vector2F(style.rule_height + 2.0f)); }
        else { box.inflate(-1.0f); }
        return box;
    };
    const auto frozen = [&](const Frame& captured)
    {
        std::srand(9239); const int next = std::rand(); std::srand(9239);
        require(captured.pixels == frame().pixels && captured.pixels == frame().pixels && std::rand() == next,
            "Frozen setup-motion drawing changed pixels or consumed RNG");
    };
    std::ofstream records(output / "setup-motion.csv");
    records << "case,width,height,delta_x,delta_y,delta_width,delta_height,responding_rows\n";
    const auto changed = [&](const char* name, const Frame& moving, const Frame& resting,
        const std::vector<RectangleF>& envelopes)
    {
        const RectangleI delta = foreground_delta_bounds(moving, resting);
        std::vector<RectangleI> allowed; std::vector<size_t> counts(envelopes.size(), 0);
        for (const auto& envelope : envelopes) { allowed.push_back(project(envelope)); }
        bool outside = false;
        for (int y = delta.y; y < delta.bottom(); ++y)
        {
            for (int x = delta.x; x < delta.right(); ++x)
            {
                if (same_pixel(moving, resting, hud_pixel_offset(moving, x, y))) { continue; }
                bool found = false;
                for (size_t row = 0; row < allowed.size(); ++row)
                {
                    const auto& box = allowed[row];
                    if (x >= box.x && x < box.right() && y >= box.y && y < box.bottom())
                        { ++counts[row]; found = true; }
                }
                outside = outside || !found;
            }
        }
        const bool visible = !counts.empty() && std::all_of(counts.begin(), counts.end(),
            [](size_t count) { return count > 0; });
        if (outside || !visible)
        {
            save(moving, output / (std::string("setup-motion-") + name + "-failure-moving.ppm"));
            save(resting, output / (std::string("setup-motion-") + name + "-failure-rest.ppm"));
            std::cerr << "SETUP MOTION " << name << " delta=" << delta.x << ',' << delta.y << ',' <<
                delta.width << ',' << delta.height << " outside=" << outside << " rows=";
            for (size_t count : counts) { std::cerr << count << ','; } std::cerr << '\n';
        }
        require(!outside && visible,
            "Setup motion was absent on an expected row or escaped its independent ornament envelope");
        records << name << ',' << moving.width << ',' << moving.height << ',' << delta.x << ',' <<
            delta.y << ',' << delta.width << ',' << delta.height << ',' << envelopes.size() << '\n';
    };
    const auto fixed_weapon_art = [&](const Frame& moving, const Frame& resting)
    {
        for (int player = 0; player < settings.player_count(); ++player)
        {
            const RectangleI box = project(weapon_box(player, false));
            if (foreground_crop(moving, box, 0).pixels != foreground_crop(resting, box, 0).pixels)
            {
                save(moving, output / "setup-motion-weapon-art-failure-moving.ppm");
                save(resting, output / "setup-motion-weapon-art-failure-rest.ppm");
                std::cerr << "SETUP WEAPON ART player=" << player << " crop=" << box.x << ',' <<
                    box.y << ',' << box.width << ',' << box.height << '\n';
            }
            require(foreground_crop(moving, box, 0).pixels == foreground_crop(resting, box, 0).pixels,
                "Weapon motion shifted its inner artwork, paint or supply mask");
        }
        // Names, descriptions, labels and badges are outside every allowed
        // tile envelope, so changed() also verifies their exact fixed raster.
    };
    const auto capture = [&](const char* name, const Frame& captured, RectangleF detail)
    {
        const std::string stem = std::string("setup-motion-") + name + '-' +
            std::to_string(captured.width) + 'x' + std::to_string(captured.height);
        save(captured, output / (stem + ".ppm"));
        save(foreground_crop(captured, project(detail)), output / (stem + "-detail.ppm"));
    };
    const auto open = [&](int page, int count = 4, int best_of = 1)
    {
        stack.clear(); stack.notify_activation(true); context.cancel_music_exit();
        app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::r, Key::enter, Key::escape, Key::up, Key::down, Key::left, Key::right })
            { app.keyboard()->on_key_up(key); }
        painter_scripted_pads::states = {};
        for (int player = 1; player < count; ++player)
            { painter_scripted_pads::states[static_cast<size_t>(slots[static_cast<size_t>(player)])].connected = true; }
        app.gamepads()->poll(); app.gamepads()->poll(); app.keyboard()->poll(); app.keyboard()->poll();
        app.mouse()->poll(); app.mouse()->poll();
        settings = MatchSetup(); settings.set_player_count(count); settings.set_best_of(best_of);
        settings.begin_input_claims(slots[0]);
        for (int player = 1; player < count; ++player)
            { require(settings.claim_input_slot(slots[static_cast<size_t>(player)]), "Setup-motion input claim failed"); }
        settings.set_stage("movement_course"); settings.set_duration(120.0f);
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
            count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        for (int player = 0; player < count; ++player)
        {
            settings.set_player_team(player, player % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(player, weapons[static_cast<size_t>(player)]);
        }
        input.reset_actions(); stack.push(std::make_unique<PauseMenuProbeRoot>(app, canvas));
        std::unique_ptr<MenuPage> opened;
        if (page == 0) { opened = std::make_unique<MainMenuModeSelect>(&context); }
        else if (page == 1) { opened = std::make_unique<MainMenuPlayerCount>(&context); }
        else if (page == 2) { opened = std::make_unique<MainMenuTeamSelect>(&context); }
        else { opened = std::make_unique<MainMenuWeaponSelect>(&context); }
        active_page = opened.get(); stack.push(std::move(opened));
        tick(); tick(); require(stack.depth() == 2, "Setup-motion production page did not open");
        const Frame first = frame(); tick(settle_time);
        require(first.pixels == frame().pixels, "A setup page animated its initial selection");
    };
    const auto expected_page = [&](int page)
    {
        MatchSetup plan = settings;
        MenuInput expected_input(app.gamepads(), app.keyboard(), &tuning);
        MenuContext expected_context(&data, &expected_input, &plan);
        StateContext expected_stack;
        if (page == 2) { expected_stack.push(std::make_unique<MainMenuJoin>(&expected_context)); }
        else if (page == 1) { expected_stack.push(std::make_unique<MainMenuStageSelect>(&expected_context)); }
        else { expected_stack.push(std::make_unique<MainMenuWeaponSelect>(&expected_context)); }
        app.renderer()->begin_frame(); expected_stack.draw(*app.renderer()); return read(*app.renderer());
    };
    const auto lifecycle = [&](int page)
    {
        tap(0, GamepadButton::dpad_right); const Frame active = frame(); frozen(active);
        play.reduce_effects = true; const Frame reduced = frame(); play.reduce_effects = false;
        require(active.pixels == frame().pixels, "Reduced setup drawing consumed its motion clock");
        tick(settle_time); require(reduced.pixels == frame().pixels,
            "Applied Reduce effects did not immediately suppress setup motion");
        tap(0, GamepadButton::dpad_left); stack.notify_activation(false);
        const Frame inactive = frame(); stack.notify_activation(true); tick(settle_time);
        require(inactive.pixels == frame().pixels, "Activation retained or restarted setup motion");
        tap(0, GamepadButton::dpad_right);
        painter_scripted_pads::states[0].connected = false; tick(); const Frame missing = frame();
        tick(settle_time); require(missing.pixels == frame().pixels,
            "Missing-player gate retained setup motion");
        painter_scripted_pads::states[0].connected = true;
        painter_scripted_pads::states[0].buttons = bit(GamepadButton::dpad_right); tick(); tick();
        painter_scripted_pads::states[0].buttons = 0; tick(); tick();
        const Frame reconnect = frame(); tick(settle_time);
        require(reconnect.pixels == frame().pixels, "Reconnection replayed setup motion or held input");
        tap(0, GamepadButton::dpad_left);
        play.reduce_effects = true; const Frame covered_rest = frame(); play.reduce_effects = false;
        // Setup has no user-facing child here. Push a real covering page only
        // to exercise StateContext's ordinary suspension/resumption mechanism.
        stack.push(std::make_unique<MainMenuCredits>(&context)); require(stack.depth() == 3,
            "Test-pushed covering page did not suspend setup"); stack.pop();
        require(stack.depth() == 2 && covered_rest.pixels == frame().pixels,
            "Returning from a test-pushed covering page retained setup motion");
        tick();
        play.reduce_effects = true; tap(0, GamepadButton::dpad_right); const Frame reduced_choice = frame();
        tick(settle_time); require(reduced_choice.pixels == frame().pixels,
            "Reduced-effects setup input restarted selection motion"); play.reduce_effects = false;
        tap(0, GamepadButton::dpad_left); play.reduce_effects = true;
        const Frame reset_rest = frame(); play.reduce_effects = false;
        active_page->debug_reset_input(slots[0]);
        require(reset_rest.pixels == frame().pixels, "Developer input reset retained per-player setup motion");
        tick(settle_time); require(reset_rest.pixels == frame().pixels,
            "Developer input reset restarted per-player setup motion");
        std::cout << "PASS setup motion lifecycle page " << page << '\n';
    };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        open(0); tap(0, GamepadButton::dpad_down);
        const Frame mode_arrival = frame(); frozen(mode_arrival); tick(0.0f); tick(-0.5f);
        require(mode_arrival.pixels == frame().pixels, "Nonpositive setup time advanced focus motion");
        tick(motion.focus_duration * 0.4f); const Frame mode_middle = frame(); tick(settle_time);
        const Frame mode_rest = frame();
        const RectangleF mode_stripe = focus_arrow(look.mode_select_spacing, 3, presentation.menu_text().item(MenuItem::best_of_three));
        changed("mode-arrival", mode_arrival, mode_rest, { mode_stripe });
        changed("mode-middle", mode_middle, mode_rest, { mode_stripe });
        require(mode_arrival.pixels != mode_middle.pixels, "Mode focus did not progress with positive time");
        tap(0, GamepadButton::dpad_left); require(mode_rest.pixels == frame().pixels,
            "A refused horizontal Mode action invented a focus press");
        tap(0, GamepadButton::dpad_down); tap(0, GamepadButton::dpad_down); tick(settle_time);
        tap(0, GamepadButton::dpad_right); const Frame duration_press = frame(); frozen(duration_press);
        tick(settle_time); const Frame duration_rest = frame();
        const RectangleF duration_stripe = duration_card();
        changed("duration-press", duration_press, duration_rest, { duration_stripe });
        tap(0, GamepadButton::a); const Frame duration_confirm = frame(); tick(settle_time);
        changed("duration-confirm", duration_confirm, frame(), { duration_stripe });
        capture("mode-arrival", mode_arrival, mode_stripe); capture("mode-middle", mode_middle, mode_stripe);
        capture("mode-rest", mode_rest, mode_stripe); capture("duration-press", duration_press, duration_stripe);
        capture("duration-rest", duration_rest, duration_stripe);

        // Count retains this fixture's four-player choice. Settle at Three,
        // then measure the same Two Players response and confirmation as before.
        open(1); tap(0, GamepadButton::dpad_up); tick(settle_time);
        tap(0, GamepadButton::dpad_up); const Frame count_arrival = frame();
        tick(motion.focus_duration * 0.4f); const Frame count_middle = frame(); tick(settle_time);
        const Frame count_rest = frame();
        const RectangleF count_stripe = focus_arrow(look.player_count_spacing, 3, presentation.menu_text().item(MenuItem::two_players));
        changed("count-arrival", count_arrival, count_rest, { count_stripe });
        changed("count-middle", count_middle, count_rest, { count_stripe });
        tap(0, GamepadButton::dpad_down); tap(0, GamepadButton::dpad_up); const Frame rapid_count = frame();
        tick(settle_time); changed("count-rapid", rapid_count, frame(), { count_stripe });
        capture("count-arrival", count_arrival, count_stripe); capture("count-rest", count_rest, count_stripe);
        tap(0, GamepadButton::a); require(settings.player_count() == 2 && stack.depth() == 2 &&
            frame().pixels == expected_page(2).pixels,
            "Count confirmation waited for cosmetic motion instead of starting Join");

        open(2); const MatchSetup initial_team = settings;
        together(GamepadButton::dpad_right, GamepadButton::dpad_left);
        const Frame team_arrival = frame(); frozen(team_arrival);
        tick(motion.focus_duration * 0.4f); const Frame team_middle = frame(); tick(settle_time);
        const Frame team_rest = frame();
        const std::vector<RectangleF> centers{ team_box(0, PlayerTeam::none), team_box(1, PlayerTeam::none) };
        changed("team-simultaneous", team_arrival, team_rest, centers);
        changed("team-middle", team_middle, team_rest, centers);
        require(team_arrival.pixels != team_middle.pixels, "Team response did not progress with positive time");
        tap(0, GamepadButton::dpad_left); tap(0, GamepadButton::dpad_right); tap(0, GamepadButton::dpad_left);
        const Frame rapid_team = frame(); tick(settle_time);
        changed("team-rapid", rapid_team, frame(), { team_box(0, PlayerTeam::a) });
        tap(0, GamepadButton::dpad_right); tick(settle_time); const Frame no_team = frame();
        tap(0, GamepadButton::a); require(no_team.pixels == frame().pixels,
            "Confirming no team invented a readiness response");
        tap(0, GamepadButton::dpad_left); tick(settle_time); const Frame endpoint = frame();
        tap(0, GamepadButton::dpad_left); require(endpoint.pixels == frame().pixels,
            "A Team endpoint invented a selection response");
        tap(0, GamepadButton::a); const Frame team_ready = frame(); tick(settle_time);
        const Frame team_ready_rest = frame(); changed("team-ready", team_ready, team_ready_rest, { team_box(0, PlayerTeam::a) });
        tap(0, GamepadButton::a); tap(0, GamepadButton::dpad_right);
        require(team_ready_rest.pixels == frame().pixels,
            "Already-ready Team input invented a response or changed the choice");
        tap(0, GamepadButton::b); const Frame team_unready = frame(); tick(settle_time);
        changed("team-unready", team_unready, frame(), { team_box(0, PlayerTeam::a) });
        capture("team-arrival", team_arrival, RectangleF(630.0f, 380.0f, 190.0f, 220.0f));
        capture("team-middle", team_middle, RectangleF(630.0f, 380.0f, 190.0f, 220.0f));
        capture("team-rest", team_rest, RectangleF(630.0f, 380.0f, 190.0f, 220.0f));
        capture("team-ready", team_ready, team_box(0, PlayerTeam::a));
        lifecycle(2);

        // Refused completion is a warning, not an accepted readiness press.
        open(2);
        for (int player : { 1, 3 }) { tap(player, GamepadButton::dpad_left); tap(player, GamepadButton::dpad_left); }
        for (int player = 0; player < 3; ++player) { tap(player, GamepadButton::a); }
        tick(settle_time); tap(3, GamepadButton::a); const Frame refused = frame(); tick(settle_time);
        require(refused.pixels == frame().pixels, "Refused empty-team completion invented a press");
        MatchSetup shuffled = initial_team;
        std::srand(67); require(shuffled.shuffle_teams(), "Expected setup shuffle failed"); std::srand(67);
        together(GamepadButton::x, GamepadButton::a); const Frame shuffle = frame(); frozen(shuffle);
        tick(settle_time); const Frame shuffle_rest = frame(); std::vector<RectangleF> shuffled_boxes;
        for (int player = 0; player < 4; ++player) { shuffled_boxes.push_back(team_box(player, shuffled.player_team(player))); }
        changed("team-shuffle", shuffle, shuffle_rest, shuffled_boxes);
        capture("team-shuffle", shuffle, RectangleF(630.0f, 380.0f, 190.0f, 460.0f));
        for (int player = 0; player < 4; ++player) { tap(player, GamepadButton::a); }
        for (int player = 0; player < 4; ++player)
            { require(settings.player_team(player) == shuffled.player_team(player), "Team confirmation did not immediately commit shuffled choices"); }
        require(frame().pixels == expected_page(0).pixels,
            "Final Team confirmation waited for motion or carried motion into Weapon Select");

        open(3); const MatchSetup initial_weapon = settings;
        together(GamepadButton::dpad_right, GamepadButton::dpad_right);
        const Frame weapon_arrival = frame(); frozen(weapon_arrival);
        tick(motion.focus_duration * 0.4f); const Frame weapon_middle = frame(); tick(settle_time);
        const Frame weapon_rest = frame();
        const std::vector<RectangleF> tiles{ weapon_box(0, true), weapon_box(1, true) };
        changed("weapon-simultaneous", weapon_arrival, weapon_rest, tiles);
        changed("weapon-middle", weapon_middle, weapon_rest, tiles);
        fixed_weapon_art(weapon_arrival, weapon_rest); fixed_weapon_art(weapon_middle, weapon_rest);
        require(weapon_arrival.pixels != weapon_middle.pixels, "Weapon tile response did not progress with positive time");
        tap(0, GamepadButton::a); const Frame weapon_ready = frame(); tick(settle_time);
        const Frame weapon_ready_rest = frame(); changed("weapon-ready", weapon_ready, weapon_ready_rest, { weapon_box(0, true) });
        fixed_weapon_art(weapon_ready, weapon_ready_rest);
        tap(0, GamepadButton::a); tap(0, GamepadButton::dpad_right);
        require(weapon_ready_rest.pixels == frame().pixels,
            "Already-ready Weapon input invented motion or changed the tool");
        tap(0, GamepadButton::b); const Frame weapon_unready = frame(); tick(settle_time);
        const Frame weapon_unready_rest = frame(); changed("weapon-unready", weapon_unready, weapon_unready_rest, { weapon_box(0, true) });
        fixed_weapon_art(weapon_unready, weapon_unready_rest);
        tap(0, GamepadButton::dpad_left); tap(0, GamepadButton::dpad_right); tap(0, GamepadButton::dpad_left);
        const Frame weapon_rapid = frame(); tick(settle_time); const Frame weapon_rapid_rest = frame();
        changed("weapon-rapid", weapon_rapid, weapon_rapid_rest, { weapon_box(0, true) });
        fixed_weapon_art(weapon_rapid, weapon_rapid_rest);
        capture("weapon-arrival", weapon_arrival, RectangleF(630.0f, 380.0f, 400.0f, 240.0f));
        capture("weapon-middle", weapon_middle, RectangleF(630.0f, 380.0f, 400.0f, 240.0f));
        capture("weapon-rest", weapon_rest, RectangleF(630.0f, 380.0f, 400.0f, 240.0f));
        capture("weapon-ready", weapon_ready, weapon_box(0, true));
        for (size_t choice = 0; choice < look.weapon_select_art.choices.size(); ++choice)
        {
            tap(0, GamepadButton::dpad_right); const Frame selected = frame(); tick(settle_time);
            const Frame selected_rest = frame(); changed("weapon-catalog-motion", selected, selected_rest, { weapon_box(0, true) });
            fixed_weapon_art(selected, selected_rest);
        }
        for (int player = 0; player < 4; ++player)
            { require(settings.player_weapon(player) == initial_weapon.player_weapon(player), "Weapon motion committed a provisional choice"); }
        lifecycle(3);
        const Frame before_resize = frame();
        app.resolution_manager()->set_resolution_exactly({ 1920, 1080 });
        std::ignore = app.renderer()->window_size_changed(1920, 1080); std::ignore = frame();
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        require(before_resize.pixels == frame().pixels, "Settled setup changed after resize round-trip");
        for (int player = 0; player < 4; ++player) { tap(player, GamepadButton::a); }
        require(frame().pixels == expected_page(1).pixels,
            "Final Weapon confirmation waited for motion or carried motion into Stage Select");
        std::cout << "PASS native setup motion " << size.x << 'x' << size.y << '\n';
    }
    require(records.good(), "Setup-motion evidence write failed");
    require(IsWindowVisible(static_cast<HWND>(app.window())) == FALSE &&
        GetForegroundWindow() != static_cast<HWND>(app.window()),
        "Setup-motion probes exposed or activated their hidden window");
    std::cout << "PASS actual setup motion: native 720p/1080p/QHD initial/middle/rest; independent rows, "
        "fixed weapon art/masks/text/badges, accepted and refused actions, simultaneous/rapid input, "
        "immediate final transitions, applied Reduce effects, activation/missing/reconnect/covering-page resets, "
        "frozen redraw/RNG and resize; scripted reader only\n";
}
