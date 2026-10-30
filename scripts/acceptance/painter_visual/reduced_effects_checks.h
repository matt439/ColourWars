// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// Saved reduce-effects preference matrix.

    Frame isolated_status(const Player& player, const Viewport& viewport,
        Renderer& renderer, const Canvas& canvas, bool reduced)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(player.camera());
        list.set_filter(TextureFilter::point);
        canvas.draw(list, player.camera().visible_rectangle(viewport));
        const Weapon& tool = player.primary_weapon();
        tool.feedback()->draw(list, tool.nozzle_pos(), tool.draw_pos(),
            tool.rotation(), tool.type(), player.team_colour(), reduced);
        return read(renderer);
    }

    // The live builder borrows this same saved preference object. Compare its
    // two draw paths without advancing a match, then restore the exact normal
    // raster; rendering a reduced frame must not consume a cosmetic event.
    void reduced_effects_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, PlaySettings& preferences,
        const std::filesystem::path& output, std::ofstream& records)
    {
        const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
        const RestoreFlag restore_marker{ preferences.aim_marker, preferences.aim_marker };
        preferences.reduce_effects = false;
        preferences.aim_marker = true;
        std::ofstream effects_records(output / "reduced-effects.csv");
        effects_records << "capture,view,changed_pixels\n";
        const auto compare = [&](Level& level, const std::string& stem, bool every_pane)
        {
            const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
            const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
            preferences.reduce_effects = false;
            capture(app, level, canvas, output, stem + "-normal", records);
            const Frame normal = render(level, *app.renderer());
            preferences.reduce_effects = true;
            capture(app, level, canvas, output, stem + "-reduced", records);
            const Frame reduced = render(level, *app.renderer());
            bool changed = false;
            for (int view = 0; view < level.player_count(); ++view)
            {
                const size_t pixels = pane_delta(normal, reduced,
                    app.viewport_manager()->player_viewport(view).pixel_rect());
                require(!every_pane || pixels > 0,
                    "reduce-effects preference did not remove the live muzzle flash in every pane");
                changed |= pixels > 0;
                effects_records << stem << ',' << view + 1 << ',' << pixels << '\n';
            }
            require(changed, "reduce-effects preference produced no visible playing-size difference");
            preferences.reduce_effects = false;
            require(normal.pixels == render(level, *app.renderer()).pixels,
                "normal raster did not return after the same-state reduce-effects toggle");
            require(before == observation(level),
                "reduce-effects drawing or preference toggles changed observed match state");
#if COLOURWARS_DEV_TOOLS
            require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
                "reduce-effects drawing or preference toggles changed the replay fingerprint");
#endif
        };
        const auto compare_markers = [&](Level& level, bool initially_ready)
        {
            const auto before = observation(level);
            bool saw_recovering = false;
            for (int view = 0; view < level.player_count(); ++view)
            {
                const Player& player = level.player(view);
                const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                const WeaponReadiness readiness = player.primary_weapon().readiness();
                require(!initially_ready || readiness == WeaponReadiness::ready,
                    "reduced readiness fixture was not initially ready");
                saw_recovering |= readiness == WeaponReadiness::recovering;
                const Frame blank = isolated_marker(player, viewport, *app.renderer(), canvas, false);
                preferences.reduce_effects = false;
                require(level.aim_marker_visible(view), "normal reduce-effects fixture lost its readiness marker");
                const Frame normal = isolated_marker(player, viewport, *app.renderer(), canvas, true);
                preferences.reduce_effects = true;
                require(level.aim_marker_visible(view) && normal.pixels != blank.pixels &&
                    normal.pixels == isolated_marker(player, viewport, *app.renderer(), canvas, true).pixels &&
                    player.primary_weapon().readiness() == readiness,
                    "reduce effects hid or changed the real aim/readiness marker raster");
            }
            preferences.reduce_effects = false;
            require(initially_ready || saw_recovering,
                "reduced readiness fixture did not exercise a recovering tool");
            require(before == observation(level), "reduced readiness comparison changed observed match state");
        };
        for (const int count : { 1, 4 })
        {
            auto level = build(app, builder, count, TeamColourPair::blue_orange,
                false, false, count == 1 ? WeaponType::mister : WeaponType::none);
            auto inputs = neutral_inputs();
            settle(*level, inputs);
            resize(app, *level, { 1280, 720 });
            compare_markers(*level, true);
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                Weapon& tool = const_cast<Weapon&>(player.primary_weapon());
                auto& input = inputs[static_cast<size_t>(player.player_num())];
                input.primary_shoot = true;
                const float ammo = tool.ammo();
                // Retain real paid projectiles locally, as in the weapon
                // matrix, so the four fixtures cannot hit each other.
                const auto shots = tool.update_and_get_projectiles(input,
                    player.rectangle().center(), player.velocity(), player.facing_right(), dt);
                require(!shots.empty() && tool.ammo() < ammo &&
                    tool.feedback()->state().muzzle_remaining > 0.0f,
                    "reduce-effects weapon fixture did not produce a real paid muzzle flash");
            }
            const std::string prefix = std::to_string(count) + "p-reduce-effects-720";
            compare(*level, prefix + "-paid-shot", true);
            compare_markers(*level, false);
            for (int view = 0; view < count; ++view)
            {
                const Player& player = level->player(view);
                const Weapon& tool = player.primary_weapon();
                auto& feedback = const_cast<WeaponFeedback&>(*tool.feedback());
                const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                const RectangleI pane = viewport.pixel_rect();
                const Frame blank = isolated_marker(player, viewport, *app.renderer(), canvas, false);
                for (const bool recovered : { false, true })
                {
                    feedback.clear();
                    if (recovered) { feedback.recovered(); }
                    else { feedback.empty(); }
                    const auto before = observation(*level);
                    const Frame normal = isolated_status(player, viewport, *app.renderer(), canvas, false);
                    const Frame reduced = isolated_status(player, viewport, *app.renderer(), canvas, true);
                    const size_t normal_pixels = pane_delta(normal, blank, pane);
                    const size_t reduced_pixels = pane_delta(reduced, blank, pane);
                    require(reduced.pixels == isolated_status(player, viewport, *app.renderer(), canvas, true).pixels,
                        "frozen reduced status cue changed during repeated drawing");
                    require(reduced_pixels > 0 && (recovered ? reduced_pixels < normal_pixels :
                        reduced.pixels == normal.pixels),
                        "reduced effects removed a useful dry/refill cue or retained the refill rays");
                    const std::string stem = prefix + (recovered ? "-refill" : "-dry");
                    for (const auto& sample : { std::pair{ &normal, "-normal" }, std::pair{ &reduced, "-reduced" } })
                    {
                        Frame crop{};
                        require(check_projected_bounds(*sample.first, blank,
                            player.camera().calculate_view_rectangle(tool.draw_bounds()), viewport, crop) > 0,
                            "reduced status crop omitted its dry/refill cue");
                        save(crop, output / (stem + sample.second + "-player" + std::to_string(view + 1) + ".ppm"));
                    }
                    effects_records << stem << ',' << view + 1 << ',' << pane_delta(normal, reduced, pane) << '\n';
                    require(before == observation(*level), "reduced status drawing changed observed match state");
                }
                feedback.clear();
                if (tool.type() == WeaponType::mister)
                {
                    Weapon& mister = const_cast<Weapon&>(tool);
                    auto& input = inputs[static_cast<size_t>(player.player_num())];
                    input.primary_shoot = false;
                    const auto shots = mister.update_and_get_projectiles(input,
                        player.rectangle().center(), player.velocity(), player.facing_right(), dt);
                    require(shots.empty() && mister.motion_state().mist_tail_remaining > 0.0f,
                        "reduce-effects mister fixture did not release a real cosmetic tail");
                    const auto before = observation(*level);
                    preferences.reduce_effects = false;
                    const Frame normal = isolated_tool(player, viewport, *app.renderer(), canvas, presentation);
                    preferences.reduce_effects = true;
                    const Frame reduced = isolated_tool(player, viewport, *app.renderer(), canvas, presentation);
                    require(reduced.pixels == isolated_tool(player, viewport, *app.renderer(), canvas, presentation).pixels,
                        "frozen reduced mister tool changed during repeated drawing");
                    preferences.reduce_effects = false;
                    auto& motion = const_cast<ToolMotionState&>(mister.motion_state());
                    const ToolMotionState released = motion;
                    motion.mist_tail_remaining = 0.0f;
                    const Frame tool_only = isolated_tool(player, viewport, *app.renderer(), canvas, presentation);
                    motion = released;
                    require(normal.pixels != reduced.pixels && reduced.pixels == tool_only.pixels,
                        "reduce effects failed to hide only the real mister release tail");
                    require(before == observation(*level), "reduced mist drawing changed observed match state");
                }
            }
            compare(*level, prefix + "-mist-release", false);

            auto quiet = build(app, builder, count, TeamColourPair::blue_orange,
                false, false, WeaponType::sprayer);
            inputs = neutral_inputs();
            settle(*quiet, inputs);
            resize(app, *quiet, { 1280, 720 });
            for (int view = 0; view < count; ++view)
            {
                require(quiet->place_player(view,
                    Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                    "could not place the reduce-effects idle fixture on the recovery floor");
            }
            for (int tick = 0; tick < 60; ++tick) { quiet->update(acceptance::slot_seats(*quiet, inputs), dt); }
            const IdleDrip* drip = quiet->idle_drip(0);
            require(drip != nullptr, "reduce-effects idle fixture omitted its sprayer drip");
            const auto visible_bead = [&] { return drip->attached() &&
                drip->bead_age() >= presentation.idle_drip().grow_time; };
            for (int tick = 0; tick < 900 && !visible_bead(); ++tick) { quiet->update(acceptance::slot_seats(*quiet, inputs), dt); }
            require(visible_bead(), "reduce-effects fixture never produced a mature idle bead");
            compare(*quiet, prefix + "-idle", false);
        }
        require(effects_records.good(), "reduce-effects capture record write failed");
        std::cout << "PASS saved reduce-effects preference at 1P/4P 720p; real paid muzzle flashes and idle detail; "
            "preserved dry/refill and ready/cooldown marker rasters, fewer refill rays and hidden mister release tail; "
            "normal/reduced/restored rasters, frozen redraw and unchanged match observation/replay fingerprint\n";
    }
