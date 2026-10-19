// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// Aim-marker on/off matrix.

    // On/off is rendered from exactly the same live Level. Every changed pixel
    // must belong to that pane's own current marker; another player's marker
    // in this pane cannot hide behind a whole-frame "changed" assertion.
    std::vector<Frame> marker_capture(Application& app, Level& level, PlaySettings& preferences,
        const Canvas& canvas, const std::filesystem::path& output,
        const std::string& stem, std::ofstream& records)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        preferences.aim_marker = true;
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Frozen marker Level redraw changed pixels");
        preferences.aim_marker = false;
        const Frame disabled = render(level, *app.renderer());
        require(disabled.pixels == render(level, *app.renderer()).pixels,
            "Disabled marker Level redraw changed pixels");
        preferences.aim_marker = true;
        save(enabled, output / (stem + ".ppm"));
        std::vector<Frame> marker_rasters;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const auto& player = level.player(view);
            const auto& tool = player.primary_weapon();
            const auto& viewport = app.viewport_manager()->player_viewport(view);
            Frame crop{};
            const size_t changed = check_projected_bounds(enabled, disabled,
                level.view_camera(view).calculate_view_rectangle(tool.aim_marker_bounds()), viewport, crop);
            require(changed > 0 && level.aim_marker_visible(view),
                "Active own-view marker produced no on/off raster delta");
            // The public marker bounds cover its independent world overlay,
            // even if its owner's body were culled by Scene.
            const Frame blank = isolated_marker(player, viewport, *app.renderer(), canvas, false);
            const Frame marker = isolated_marker(player, viewport, *app.renderer(), canvas, true);
            require(marker.pixels == isolated_marker(player, viewport, *app.renderer(), canvas, true).pixels,
                "Frozen isolated marker redraw changed pixels");
            Frame isolated_crop{};
            require(check_projected_bounds(marker, blank,
                player.camera().calculate_view_rectangle(tool.aim_marker_bounds()), viewport, isolated_crop) > 0,
                "Marker overlay omitted its independent raster");
            save(isolated_crop, output / (stem + "-player" + std::to_string(view + 1) + ".ppm"));
            marker_rasters.push_back(std::move(isolated_crop));
            records << stem << ',' << view + 1 << ',' << player.player_num() << ','
                << static_cast<int>(tool.type()) << ',' << static_cast<int>(tool.readiness()) << ','
                << tool.rotation() << ',' << changed << '\n';
        }
        require(before == observation(level), "Marker drawing/toggling changed simulation or feedback timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Marker drawing/toggling changed replay fingerprint");
#endif
        return marker_rasters;
    }

    void marker_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, PlaySettings& preferences, const std::filesystem::path& output)
    {
        const RestoreFlag restore{ preferences.aim_marker, preferences.aim_marker };
        preferences.aim_marker = true;
        std::ofstream records(output / "aim-marker.csv");
        records << "capture,view,input_slot,tool,readiness,angle,changed_pixels\n";
        for (const auto palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (int view = 0; view < 4; ++view)
            {
                require(level->place_player(view, Vector2F(1200.0f + 400.0f * static_cast<float>(view), 1275)),
                    "Could not place marker fixture on recovery floor");
            }
            for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
            for (int direction = 0; direction < 4; ++direction)
            {
                const float angle = -3.0f * PI / 4.0f + static_cast<float>(direction) * PI / 2.0f;
                std::array<std::vector<Frame>, 3> ready_rasters;
                std::array<std::vector<Frame>, 3> recovering_rasters;
                for (int phase = 0; phase < 3; ++phase)
                {
                    const auto readiness = phase == 0 ? WeaponReadiness::ready :
                        phase == 1 ? WeaponReadiness::recovering : WeaponReadiness::empty;
                    for (int view = 0; view < 4; ++view)
                    {
                        Player& player = const_cast<Player&>(level->player(view));
                        Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                        const float cooldown = tuning.weapon(tool.type()).details.shoot_interval + dt;
                        // Aim around the actual player's pivot, then keep that
                        // transform during the controlled firing-only updates.
                        tool.reset_ammo();
                        PlayerInputData aim;
                        aim.connected = true;
                        aim.shoot_direction_requested = true;
                        aim.shoot_angle = angle;
                        aim.shoot_direction = Vector2F::unit_vec_from_angle(angle);
                        std::ignore = tool.update_and_get_projectiles(aim, player.rectangle().center(),
                            player.velocity(), player.facing_right(), cooldown);
                        const auto fire = [&](bool held, float elapsed)
                        {
                            aim.primary_shoot = held;
                            return tool.update_and_get_projectiles(aim, player.rectangle().center(),
                                player.velocity(), player.facing_right(), elapsed);
                        };
                        if (phase == 1)
                        {
                            require(!fire(true, 0.0f).empty(), "Marker cooldown fixture did not pay a shot");
                        }
                        else if (phase == 2)
                        {
                            for (int shot = 0; shot < 1000 && tool.readiness() != WeaponReadiness::empty; ++shot)
                            {
                                require(!fire(true, 0.0f).empty(), "Marker depletion fixture refused an affordable shot");
                                if (tool.readiness() != WeaponReadiness::empty) { std::ignore = fire(false, cooldown); }
                            }
                        }
                        require(tool.readiness() == readiness, "Actual weapon did not enter requested marker readiness");
                    }
                    for (const auto size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                    {
                        if (size.x != 1280 && (palette != TeamColourPair::blue_orange || direction != 0)) { continue; }
                        resize(app, *level, size);
                        const char* state = phase == 0 ? "ready" : phase == 1 ? "recovering" : "empty";
                        const std::string stem = std::string("4p-aim-marker-") + TeamColourTools::palette_key(palette) +
                            "-q" + std::to_string(direction + 1) + "-" + state + "-" + std::to_string(size.x);
                        const auto rasters = marker_capture(app, *level, preferences, canvas, output, stem, records);
                        const size_t resolution = size.x == 1280 ? 0u : size.x == 1920 ? 1u : 2u;
                        if (phase == 0) { ready_rasters[resolution] = rasters; }
                        else
                        {
                            for (size_t view = 0; view < rasters.size(); ++view)
                            {
                                require(rasters[view].pixels != ready_rasters[resolution][view].pixels,
                                    "Ready and unavailable marker shapes produced identical playing-size rasters");
                                if (phase == 2)
                                {
                                    require(rasters[view].pixels != recovering_rasters[resolution][view].pixels,
                                        "Recovering and empty marker shapes produced identical playing-size rasters");
                                }
                            }
                            if (phase == 1) { recovering_rasters[resolution] = rasters; }
                        }
                    }
                    if (phase == 1)
                    {
                        const auto before = observation(*level);
                        const Frame paused = render(*level, *app.renderer());
                        level->suspend();
                        level->refresh_layout();
                        require(before == observation(*level) && paused.pixels == render(*level, *app.renderer()).pixels,
                            "Suspending the frozen match changed marker state or its raster");
                        level->resume();
                    }
                }
            }
            if (palette == TeamColourPair::blue_orange)
            {
                // Different states and directions in one frozen roster catch
                // a shared/global marker accidentally reused by another pane.
                for (int view = 0; view < 4; ++view)
                {
                    const auto& player = level->player(view);
                    auto& tool = const_cast<Weapon&>(player.primary_weapon());
                    const float cooldown = tuning.weapon(tool.type()).details.shoot_interval + dt;
                    PlayerInputData aim;
                    aim.connected = true;
                    aim.shoot_direction_requested = true;
                    aim.shoot_angle = -PI / 4.0f + static_cast<float>(view) * PI / 2.0f;
                    aim.shoot_direction = Vector2F::unit_vec_from_angle(aim.shoot_angle);
                    const auto fire = [&](bool held, float elapsed)
                    {
                        aim.primary_shoot = held;
                        return tool.update_and_get_projectiles(aim, player.rectangle().center(),
                            player.velocity(), player.facing_right(), elapsed);
                    };
                    tool.reset_ammo();
                    std::ignore = fire(false, cooldown);
                    if (view == 1) { require(!fire(true, 0.0f).empty(), "Mixed marker cooldown fixture did not fire"); }
                    if (view == 2)
                    {
                        for (int shot = 0; shot < 1000 && tool.readiness() != WeaponReadiness::empty; ++shot)
                        {
                            require(!fire(true, 0.0f).empty(), "Mixed marker empty fixture did not fire");
                            if (tool.readiness() != WeaponReadiness::empty) { std::ignore = fire(false, cooldown); }
                        }
                    }
                    require(tool.readiness() == (view == 1 ? WeaponReadiness::recovering :
                        view == 2 ? WeaponReadiness::empty : WeaponReadiness::ready),
                        "Mixed roster did not retain independent readiness");
                }
                resize(app, *level, { 1280, 720 });
                std::ignore = marker_capture(app, *level, preferences, canvas, output,
                    "4p-aim-marker-mixed-own-1280", records);
            }
        }
        require(records.good(), "Marker capture record write failed");
        std::cout << "PASS aim marker: actual ready/recovering/empty tools in all palettes and quadrants, four own panes at 720p, representative 1080p/QHD, independent marker raster bounds, pause, frozen redraw and unchanged fingerprint\n";
    }
