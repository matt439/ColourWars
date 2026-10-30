// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// The tool's paid-shot pulse and mechanism-pose matrices.

    // Compare only the new draw switch, with the actual tool and player still
    // supplying the pose, tint, pivot and nozzle. Existing action/mechanism
    // animation cannot supply this raster delta.
    void compare_weapon_feedback(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool active, bool muzzle)
    {
        auto& look = const_cast<WeaponFeedbackPresentation&>(presentation.weapon_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Weapon& tool = player.primary_weapon();
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            look.enabled = true;
            const Frame enabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(enabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "frozen weapon feedback changed during repeated drawing");
            look.enabled = false;
            const Frame disabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(disabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "disabled weapon feedback changed during repeated drawing");
            const bool visible = active && (!muzzle || tool.type() != WeaponType::roller);
            require((enabled.pixels != disabled.pixels) == visible,
                "weapon feedback draw switch did not match active/expired or roller state");
            if (visible && muzzle)
            {
                const RectangleI pane = viewport.pixel_rect();
                const Vector2F nozzle = player.camera().calculate_view_position(tool.nozzle_pos()) +
                    Vector2F(static_cast<float>(pane.x), static_cast<float>(pane.y));
                // The pulse is brief and local to the current continuous
                // nozzle, including when aim has changed since its paid shot.
                const float reach = look.muzzle_sizes.at(static_cast<size_t>(tool.type())).length() *
                    player.camera().scale * 2.0f + look.keyline_width * player.camera().scale * 2.0f + 2.0f;
                for (int y = pane.y; y < pane.bottom(); ++y)
                {
                    for (int x = pane.x; x < pane.right(); ++x)
                    {
                        const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(enabled.width) +
                            static_cast<size_t>(x)) * 4;
                        if (enabled.pixels[offset] == disabled.pixels[offset] &&
                            enabled.pixels[offset + 1] == disabled.pixels[offset + 1] &&
                            enabled.pixels[offset + 2] == disabled.pixels[offset + 2]) { continue; }
                        require(std::abs(static_cast<float>(x) - nozzle.x) <= reach &&
                            std::abs(static_cast<float>(y) - nozzle.y) <= reach,
                            "paid-shot raster did not follow the current actual nozzle");
                    }
                }
            }
        }
        require(before == observation(level), "weapon drawing changed feedback timers or simulation state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "weapon feedback drawing changed the replay fingerprint");
#endif
    }

    // Paid shots prove the producer for every tool. Turning on the following
    // tick must move the still-live pulse with continuous aim and sprite flip.
    void weapon_fire_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output,
        std::ofstream& records, int count, TeamColourPair palette,
        WeaponType selected, const char* name)
    {
        auto level = build(app, builder, count, palette, false, false, selected);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (int quadrant = 0; quadrant < 4; ++quadrant)
        {
            for (int view = 0; view < count; ++view)
            {
                auto& input = inputs[static_cast<size_t>(level->player(view).player_num())];
                input.primary_shoot = false;
                input.shoot_angle = -3.0f * PI / 4.0f + PI * 0.5f *
                    static_cast<float>((quadrant + view) % 4);
                input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
            }
            const auto ready = [&]()
            {
                for (int view = 0; view < count; ++view)
                {
                    const Weapon& tool = level->player(view).primary_weapon();
                    if (!tool.ready_to_shoot() || tool.feedback()->state().muzzle_remaining > 0.0f)
                    {
                        return false;
                    }
                }
                return true;
            };
            for (int tick = 0; tick < 600 && !ready(); ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
            require(ready(), "weapon visual fixture did not become ready for its paid shot");
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                auto& input = inputs[static_cast<size_t>(player.player_num())];
                input.primary_shoot = true;
                const float ammo = tool.ammo();
                // Exercise the actual shared firing workflow but retain its
                // projectiles locally: later quadrants cannot shoot another
                // fixture player or paint the floor under its camera.
                const auto shots = tool.update_and_get_projectiles(input,
                    player.rectangle().center(), player.velocity(), player.facing_right(), dt);
                require(!shots.empty() && tool.ammo() < ammo && tool.shooting_this_update(),
                    "weapon visual fixture did not pay its shot");
                require((tool.feedback()->state().muzzle_remaining > 0.0f) ==
                    (tool.type() != WeaponType::roller), "paid-shot muzzle timer did not match the tool");
            }
            compare_weapon_feedback(app, *level, canvas, presentation, true, true);
            const std::string prefix = std::to_string(count) + "p-weapon-" + name + '-' +
                TeamColourTools::palette_key(palette) + "-q" + std::to_string(quadrant + 1);
            capture(app, *level, canvas, output, prefix + "-paid-shot", records);
            for (auto& input : inputs)
            {
                input.primary_shoot = false;
                input.shoot_angle = std::atan2(std::sin(input.shoot_angle + PI),
                    std::cos(input.shoot_angle + PI));
                input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
            }
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                const auto shots = tool.update_and_get_projectiles(
                    inputs[static_cast<size_t>(player.player_num())], player.rectangle().center(),
                    player.velocity(), player.facing_right(), dt);
                require(shots.empty(), "released visual fixture unexpectedly launched a projectile");
            }
            compare_weapon_feedback(app, *level, canvas, presentation, true, true);
            capture(app, *level, canvas, output, prefix + "-aim-follow", records);
        }
    }

    // The paid-shot matrix above proves live producers; this fixture controls
    // only the cosmetic event age so all views/resolutions see identical
    // initial, fading and expired pulses. Empty/recovery gameplay edges have
    // their own CPU acceptance checks.
    void weapon_feedback_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output, std::ofstream& records)
    {
        auto& look = const_cast<WeaponFeedbackPresentation&>(presentation.weapon_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = true;
        for (const auto& tool : std::array<std::pair<WeaponType, const char*>, 5>{{
            { WeaponType::sprayer, "sprayer" }, { WeaponType::sniper, "sniper" },
            { WeaponType::roller, "roller" }, { WeaponType::mister, "mister" },
            { WeaponType::bucket, "bucket" } }})
        {
            weapon_fire_matrix(app, builder, canvas, presentation, output, records,
                1, TeamColourPair::blue_orange, tool.first, tool.second);
        }
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            weapon_fire_matrix(app, builder, canvas, presentation, output, records,
                4, palette, WeaponType::none, "mixed");
        }
        auto level = build(app, builder, 4, TeamColourPair::blue_orange);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        for (int view = 0; view < level->player_count(); ++view)
        {
            auto& input = inputs[static_cast<size_t>(level->player(view).player_num())];
            input.shoot_angle = -3.0f * PI / 4.0f + PI * 0.5f * static_cast<float>(view);
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
        }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        for (int event = 0; event < 3; ++event)
        {
            const char* name = event == 0 ? "muzzle" : event == 1 ? "empty" : "recovered";
            const float duration = event == 0 ? look.muzzle_duration :
                event == 1 ? look.empty_duration : look.recovered_duration;
            const auto emit = [&](float age)
            {
                for (int view = 0; view < level->player_count(); ++view)
                {
                    const Weapon& tool = level->player(view).primary_weapon();
                    auto& feedback = const_cast<WeaponFeedback&>(*tool.feedback());
                    feedback.clear();
                    if (event == 0) { feedback.shot(tool.type()); }
                    else if (event == 1) { feedback.empty(); }
                    else { feedback.recovered(); }
                    feedback.update(age);
                }
            };
            for (int phase = 0; phase < 3; ++phase)
            {
                emit(duration * static_cast<float>(phase) * 0.5f);
                const bool active = phase < 2;
                const char* phase_name = phase == 0 ? "initial" : phase == 1 ? "middle" : "expired";
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                {
                    resize(app, *level, size);
                    compare_weapon_feedback(app, *level, canvas, presentation, active, event == 0);
                    const std::string prefix = std::string("4p-weapon-controlled-") + name + '-' + phase_name +
                        '-' + std::to_string(size.x);
                    capture(app, *level, canvas, output, prefix + "-on", records);
                    look.enabled = false;
                    capture(app, *level, canvas, output, prefix + "-off", records);
                    look.enabled = true;
                }
            }
        }
        std::cout << "PASS paid-shot 1P five tools and 4P three palettes; four aim quadrants and live aim/flip follow; "
            "controlled 4P muzzle/empty/recovered initial/middle/expired on/off rasters at 720p/1080p/QHD; "
            "frozen feedback timers, redraw and culling bounds\n";
    }

    // Real paid shots and resolved movement select the tool poses. Capture the
    // release and airborne phrases too: additional render views must not age
    // either one, or leave cosmetic pixels outside the scene's cull bounds.
    void tool_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const std::filesystem::path& output, std::ofstream& records, int count,
        TeamColourPair palette, bool right, WeaponType tool, const std::string& name,
        Presentation& presentation)
    {
        auto level = build(app, builder, count, palette, false, false, tool);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (auto& input : inputs)
        {
            input.x_movement = right ? 1.0f : -1.0f;
            input.shoot_angle = right ? -PI / 4.0f : -3.0f * PI / 4.0f;
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
        }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        for (auto& input : inputs) { input.x_movement = 0; }
        for (int tick = 0; tick < 25; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        const std::string prefix = std::to_string(count) + "p-tool-" + name + "-" +
            TeamColourTools::palette_key(palette) + (right ? "-right-" : "-left-");
        std::vector<Frame> idle_tools;
        for (int view = 0; view < count; ++view)
        {
            idle_tools.push_back(isolated_tool(level->player(view),
                app.viewport_manager()->player_viewport(view), *app.renderer(), canvas, presentation));
        }
        const auto check_mechanisms = [&]()
        {
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                if (player.primary_weapon().visual_pose() == 0) { continue; }
                const Frame image = isolated_tool(player,
                    app.viewport_manager()->player_viewport(view), *app.renderer(), canvas, presentation);
                require(image.pixels != idle_tools[static_cast<size_t>(view)].pixels,
                    "tool mechanism pose is indistinguishable from idle at gameplay size");
            }
        };
        capture(app, *level, canvas, output, prefix + "idle", records);
        for (auto& input : inputs) { input.primary_shoot = true; }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        check_mechanisms();
        capture(app, *level, canvas, output, prefix + "paid-shot", records);
        for (int tick = 0; tick < 5; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        check_mechanisms();
        capture(app, *level, canvas, output, prefix + "held-fire", records);
        for (auto& input : inputs) { input.primary_shoot = false; }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        capture(app, *level, canvas, output, prefix + "release", records);
        level->suspend();
        capture(app, *level, canvas, output, prefix + "paused", records);
        level->resume();
        for (int tick = 0; tick < 8; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        check_mechanisms();
        capture(app, *level, canvas, output, prefix + "recovery", records);
        if (tool == WeaponType::sniper || tool == WeaponType::none)
        {
            for (int tick = 0; tick < 30; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
            check_mechanisms();
            capture(app, *level, canvas, output, prefix + "gauge-recovery", records);
        }
        if (tool == WeaponType::roller || tool == WeaponType::sprayer ||
            tool == WeaponType::bucket || tool == WeaponType::none)
        {
            for (auto& input : inputs) { input.x_movement = right ? 1.0f : -1.0f; }
            for (int tick = 0; tick < 12; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
            check_mechanisms();
            capture(app, *level, canvas, output, prefix + "travel", records);
        }
        if (tool == WeaponType::roller)
        {
            for (auto& input : inputs) { input.jump_pressed = true; }
            level->update(acceptance::slot_seats(*level, inputs), dt);
            for (auto& input : inputs) { input.jump_pressed = false; input.x_movement = 0; }
            for (int tick = 0; tick < 3; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
            check_mechanisms();
            capture(app, *level, canvas, output, prefix + "lift", records);
        }
        if (count == 4)
        {
            for (const Vector2I size : { Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                resize(app, *level, size);
                capture(app, *level, canvas, output,
                    prefix + "extent-" + std::to_string(size.x), records);
            }
        }
    }
