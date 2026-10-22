// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// Local view-shake matrix and its HUD/capture helpers.

    // Draw the actual HUD over two contrasting backgrounds. Pixels independent
    // of both backgrounds are opaque HUD ink; full-match samples that match that
    // ink must stay fixed while the scene behind them moves. Translucent panel
    // pixels intentionally retain the world beneath them and are not compared.
    Frame shake_hud(Application& app, const Level& level, const GameTuning& tuning,
        const Presentation& presentation, const Colour& background)
    {
        const InterfaceGameplay hud(app.render_resources(), &presentation.hud(), &presentation.player_identity());
        const Canvas canvas(app.render_resources(), background);
        Renderer& renderer = *app.renderer();
        renderer.begin_frame();
        renderer.set_view_count(level.player_count());
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            DrawList list = renderer.view(view);
            list.set_viewport(viewport);
            list.set_camera(Camera::DEFAULT_CAMERA);
            list.set_filter(TextureFilter::point);
            canvas.draw(list, RectangleF(Vector2F::ZERO, viewport.size()));
            // Identity and meters use the real frozen values. Any timer ink
            // selected below must also match the full frame's current clock.
            hud.draw_gameplay_interface(list, viewport.size(), view, player.player_num(),
                player.team(), player.health() / tuning.player().starting_health,
                player.weapon_ammo() / tuning.starting_ammo(), 100.0f, player.team_colour());
        }
        return read(renderer);
    }

    void shake_capture(Application& app, Level& level, PlaySettings& preferences,
        const Frame& dark_hud, const Frame& light_hud, const std::filesystem::path& output,
        const std::string& stem, std::ofstream& records, bool active)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        std::vector<Camera> cameras;
        std::vector<Vector2F> aims;
        std::vector<Vector2F> offsets;
        std::vector<float> remaining;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            cameras.push_back(player.camera());
            const RectangleI pane = viewport.pixel_rect();
            const Vector2F cursor(static_cast<float>(pane.x) + viewport.width * 0.7f,
                static_cast<float>(pane.y) + viewport.height * 0.3f);
            aims.push_back(PlayerCamera::mouse_direction(player.center(), cursor, viewport, player.camera()));
            offsets.push_back(player.view_shake().offset());
            remaining.push_back(player.view_shake().remaining());
            require((view == 0 && active) == (player.view_shake().remaining() > 0.0f),
                "view shake was absent from the affected player or leaked into another seat");
        }

        preferences.view_shake = false;
        level.refresh_layout();
        for (int view = 0; view < level.player_count(); ++view)
        {
            require(level.view_camera(view) == cameras[static_cast<size_t>(view)],
                "disabled view shake displaced a render camera");
        }
        const Frame disabled = render(level, *app.renderer());
        preferences.view_shake = true;
        level.refresh_layout();
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "frozen redraw aged view shake or changed the match raster");
        preferences.view_shake = false;
        level.refresh_layout();
        require(disabled.pixels == render(level, *app.renderer()).pixels,
            "repeated frozen setting toggle changed the disabled match raster");
        preferences.view_shake = true;
        level.refresh_layout();

        save(enabled, output / (stem + ".ppm"));
        if (active && enabled.width == 1280) { save(disabled, output / (stem + "-off.ppm")); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            const size_t index = static_cast<size_t>(view);
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const RectangleI pane = viewport.pixel_rect();
            const size_t changed = pane_delta(enabled, disabled, pane);
            require(view == 0 && active ? changed > 0 : changed == 0,
                "view shake failed to move its pane or changed an unaffected pane");
            require((level.view_camera(view) != cameras[index]) == (view == 0 && active),
                "render camera displacement did not stay local to the affected view");
            size_t hud_pixels = 0;
            for (int y = pane.y; y < pane.bottom(); ++y)
            {
                for (int x = pane.x; x < pane.right(); ++x)
                {
                    const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(enabled.width) +
                        static_cast<size_t>(x)) * 4;
                    if (same_pixel(dark_hud, light_hud, offset) && same_pixel(dark_hud, disabled, offset))
                    {
                        require(same_pixel(enabled, disabled, offset), "view shake moved opaque HUD pixels");
                        ++hud_pixels;
                    }
                }
            }
            require(hud_pixels > 100, "HUD shake comparison did not find enough real opaque HUD pixels");
            const Vector2F cursor(static_cast<float>(pane.x) + viewport.width * 0.7f,
                static_cast<float>(pane.y) + viewport.height * 0.3f);
            require(player.camera() == cameras[index] &&
                PlayerCamera::mouse_direction(player.center(), cursor, viewport, player.camera()) == aims[index],
                "view shake setting changed the base follow camera or mouse aim");
            require(player.view_shake().offset() == offsets[index] &&
                player.view_shake().remaining() == remaining[index],
                "drawing or changing the view shake preference advanced cosmetic state");
            records << stem << ',' << view + 1 << ',' << player.player_num() << ',' << changed << ','
                << hud_pixels << ',' << offsets[index].x << ',' << offsets[index].y << ','
                << remaining[index] << '\n';
        }
        require(before == observation(level), "view shake draw/toggle changed observed match state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "view shake draw/toggle changed the replay fingerprint");
#endif
        std::cout << "PASS " << stem << " local world displacement, fixed HUD/aim and frozen redraw\n";
    }

    void shake_matrix(Application& app, LevelBuilder& builder, const GameTuning& tuning,
        const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output)
    {
        const RestoreFlag restore{ preferences.view_shake, preferences.view_shake };
        preferences.view_shake = true;
        std::ofstream records(output / "shake-detail.csv");
        records << "capture,view,input_slot,changed_pixels,fixed_hud_pixels,offset_x,offset_y,remaining\n";
        for (const int count : { 1, 4 })
        {
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                auto level = build(app, builder, count, TeamColourPair::blue_orange);
                auto inputs = neutral_inputs();
                settle(*level, inputs);
                resize(app, *level, size);
                for (int view = 0; view < count; ++view)
                {
                    require(level->place_player(view, Vector2F(1200.0f + 400.0f * static_cast<float>(view), 1275)),
                        "could not place the view shake fixture on the recovery floor");
                }
                for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
                const Frame dark_hud = shake_hud(app, *level, tuning, presentation, Colour::black);
                const Frame light_hud = shake_hud(app, *level, tuning, presentation, Colour::white);
                const auto phase = [&](const char* name, bool active)
                {
                    shake_capture(app, *level, preferences, dark_hud, light_hud, output,
                        std::to_string(count) + "p-shake-" + name + '-' + std::to_string(size.x), records, active);
                };
                const auto expire = [&]()
                {
                    for (int tick = 0; tick < 120 && level->player(0).view_shake().remaining() > 0.0f; ++tick)
                    {
                        level->update(inputs, dt);
                    }
                    require(level->player(0).view_shake().remaining() == 0.0f,
                        "real match view shake did not expire within 120 ticks");
                };
                phase("before", false);
                Player& player = const_cast<Player&>(level->player(0));
                const ProjectileSpray hit(player.rectangle(), Vector2F::ZERO, PlayerTeam::b,
                    0, Colour::white, app.render_resources(), resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()), &tuning.projectile(ProjectileType::spray));
                const float health = player.health();
                player.on_contact(hit, Vector2F::ZERO, 0);
                require(player.health() < health && player.state() == PlayerState::alive,
                    "view shake fixture did not take a real nonfatal enemy projectile hit");
                phase("hurt", true);
                level->suspend();
                level->refresh_layout();
                for (int view = 0; view < count; ++view)
                {
                    require(level->player(view).view_shake().remaining() == 0.0f &&
                        level->view_camera(view) == level->player(view).camera(),
                        "suspending a hurt match retained a displaced render camera");
                }
                const Frame suspended = render(*level, *app.renderer());
                preferences.view_shake = false;
                level->refresh_layout();
                require(suspended.pixels == render(*level, *app.renderer()).pixels,
                    "suspended match still rendered a view shake accent");
                preferences.view_shake = true;
                level->resume();
                level->refresh_layout();
                require(player.view_shake().remaining() == 0.0f &&
                    suspended.pixels == render(*level, *app.renderer()).pixels,
                    "resuming a suspended match restored a stale view shake accent");

                // Establish real support before walking off the runway. A
                // direct airborne placement is spawn settling and must stay
                // silent. Stop in the gap before its x2700 recovery platforms.
                require(level->place_player(0, Vector2F(2380, 875)), "could not place the strong-landing fixture");
                for (int tick = 0; tick < 30; ++tick) { level->update(inputs, dt); }
                require(player.move_state() == PlayerMoveState::on_ground,
                    "strong-landing fixture did not establish runway support");
                PlayerInputData& input = inputs[static_cast<size_t>(player.player_num())];
                input.x_movement = 1.0f;
                for (int tick = 0; tick < 120 && player.rectangle().x < 2482.0f; ++tick)
                {
                    level->update(inputs, dt);
                }
                input.x_movement = 0.0f;
                require(player.rectangle().x >= 2482.0f && player.rectangle().right() < 2700.0f,
                    "strong-landing fixture did not leave the runway into the open gap");
                for (int tick = 0; tick < 180 && player.view_shake().remaining() == 0.0f; ++tick)
                {
                    level->update(inputs, dt);
                }
                require(player.state() == PlayerState::alive && player.move_state() == PlayerMoveState::on_ground &&
                    player.view_shake().remaining() > 0.0f, "resolved strong landing did not start local view shake");
                phase("landing", true);
                expire();
                require(level->kill_player(0), "could not request local view shake elimination");
                level->update(inputs, dt);
                require(player.state() == PlayerState::dead, "view shake elimination fixture stayed alive");
                phase("elimination", true);
                expire();
                phase("expired", false);
            }
        }
        require(records.good(), "view shake capture record write failed");
        std::cout << "PASS real enemy projectile hurt, resolved strong landing and elimination; "
            "local 1P/4P world shake at 720p/1080p/QHD, unchanged other panes and opaque HUD ink; "
            "same-state on/off, frozen redraw, suspend/resume reset, expiry, stable follow camera/mouse aim and replay fingerprint\n";
    }
