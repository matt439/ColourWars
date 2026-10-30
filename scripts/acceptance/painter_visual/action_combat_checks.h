// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// Pinned body-frame and combat on/off matrices.

    // Compare the two actual body rasters at the same location and camera,
    // without a rotating tool or movement effects supplying a false change.
    // The normal captures still include each real tool, viewport and cull box.
    Frame pinned_body(Player& player, const RectangleF& anchor, const Camera& camera,
        const Viewport& viewport, Renderer& renderer, const Canvas& canvas,
        Presentation& presentation)
    {
        auto& look = const_cast<CombatFeedbackPresentation&>(presentation.combat_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = false;
        const Camera live_camera = player.camera();
        player.set_pose({ anchor.position() - player.rectangle().position(), 0, false, false });
        player.set_camera(camera);
        const Frame result = isolated(&player, player, viewport, renderer, canvas);
        player.set_camera(live_camera);
        player.set_pose({});
        return result;
    }

    // Compare only the draw switch at exactly the same simulation state, so
    // action frames, camera following and the respawn pen cannot supply the
    // visible difference. Restore the authored setting even on a failed check.
    void compare_combat(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool expect_delta)
    {
        auto& look = const_cast<CombatFeedbackPresentation&>(presentation.combat_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            look.enabled = true;
            const Frame enabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(enabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "frozen combat feedback changed during repeated drawing");
            look.enabled = false;
            const Frame disabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(disabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "disabled combat feedback changed during repeated drawing");
            require((enabled.pixels != disabled.pixels) == expect_delta,
                "combat feedback draw switch did not match active/expired or whole-story state");
        }
        require(before == observation(level), "combat feedback drawing changed observed simulation/timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "combat feedback drawing changed the replay fingerprint");
#endif
    }

    void action_frames(Application& app, Level& level, const Canvas& canvas,
        const std::filesystem::path& output, const std::string& stem,
        std::ofstream& records, const std::vector<PlayerInputData>& inputs,
        PlayerAnimationState expected, Presentation& presentation)
    {
        std::vector<RectangleF> anchors;
        std::vector<Camera> cameras;
        std::vector<Frame> first;
        for (int view = 0; view < level.player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level.player(view));
            require(player.animation_state() == expected && player.action_frame() == 0 &&
                player.action_age() == 0.0f, "action did not begin at its first frame on the event tick");
            anchors.push_back(player.rectangle());
            cameras.push_back(player.camera());
            first.push_back(pinned_body(player, anchors.back(), cameras.back(),
                app.viewport_manager()->player_viewport(view), *app.renderer(), canvas, presentation));
        }
        capture(app, level, canvas, output, stem + "-frame0", records);
        if (expected == PlayerAnimationState::hurt || expected == PlayerAnimationState::elimination)
        {
            compare_combat(app, level, canvas, presentation, true);
        }
        for (int tick = 0; tick < 30 && level.player(0).animation_state() == expected &&
            level.player(0).action_frame() == 0; ++tick) { level.update(acceptance::slot_seats(level, inputs), dt); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level.player(view));
            require(player.animation_state() == expected && player.action_frame() == 1,
                "action skipped its second frame");
            const Frame second = pinned_body(player, anchors[static_cast<size_t>(view)],
                cameras[static_cast<size_t>(view)], app.viewport_manager()->player_viewport(view),
                *app.renderer(), canvas, presentation);
            require(second.pixels != first[static_cast<size_t>(view)].pixels,
                "first and second action body frames produced identical playing-size raster output");
        }
        capture(app, level, canvas, output, stem + "-frame1", records);
    }

    void action_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, Presentation& presentation, const std::filesystem::path& output,
        std::ofstream& records, int count, TeamColourPair palette, bool right)
    {
        auto level = build(app, builder, count, palette);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (auto& input : inputs) { input.x_movement = right ? 1.0f : -1.0f; }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        for (auto& input : inputs) { input.x_movement = 0.0f; }
        for (int tick = 0; tick < 20; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        const std::string prefix = std::to_string(count) + "p-action-" +
            TeamColourTools::palette_key(palette) + (right ? "-right-" : "-left-");
        for (auto& input : inputs) { input.jump_pressed = true; }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        for (auto& input : inputs) { input.jump_pressed = false; }
        action_frames(app, *level, canvas, output, prefix + "takeoff", records, inputs,
            PlayerAnimationState::taking_off, presentation);
        for (int tick = 0; tick < 150 &&
            level->player(0).animation_state() != PlayerAnimationState::landing; ++tick)
        {
            level->update(acceptance::slot_seats(*level, inputs), dt);
        }
        action_frames(app, *level, canvas, output, prefix + "land", records, inputs,
            PlayerAnimationState::landing, presentation);
        for (int tick = 0; tick < 20; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        for (auto& input : inputs)
        {
            input.primary_shoot = true;
            input.shoot_angle = -PI / 2.0f;
            input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
        }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        for (auto& input : inputs) { input.primary_shoot = false; }
        action_frames(app, *level, canvas, output, prefix + "fire", records, inputs,
            PlayerAnimationState::firing, presentation);
        for (int view = 0; view < count; ++view)
        {
            Player& player = const_cast<Player&>(level->player(view));
            const ProjectileSpray hit(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
                player.team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a,
                0, Colour::white, app.render_resources(), resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()), &tuning.projectile(ProjectileType::spray));
            player.on_contact(hit, Vector2F::ZERO, 0);
        }
        action_frames(app, *level, canvas, output, prefix + "hurt", records, inputs,
            PlayerAnimationState::hurt, presentation);
        for (int view = 0; view < count; ++view)
        {
            require(level->kill_player(view), "could not request action-matrix elimination");
        }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        action_frames(app, *level, canvas, output, prefix + "elimination", records, inputs,
            PlayerAnimationState::elimination, presentation);
    }

    void combat_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, Presentation& presentation,
        const std::filesystem::path& output, std::ofstream& records)
    {
        auto level = build(app, builder, 4, TeamColourPair::blue_orange);
        const auto inputs = neutral_inputs();
        settle(*level, inputs);
        const auto phase = [&](const char* name, bool active)
        {
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                resize(app, *level, size);
                compare_combat(app, *level, canvas, presentation, active);
                capture(app, *level, canvas, output, std::string("4p-combat-") + name + "-" +
                    std::to_string(size.x), records,
                    active || level->player(0).state() == PlayerState::alive);
            }
        };
        for (int view = 0; view < level->player_count(); ++view)
        {
            Player& player = const_cast<Player&>(level->player(view));
            const ProjectileSpray hit(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
                player.team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a,
                0, Colour::white, app.render_resources(), resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()), &tuning.projectile(ProjectileType::spray));
            player.on_contact(hit, Vector2F::ZERO, 0);
        }
        phase("hurt-initial", true);
        while (level->player(0).combat_feedback().hurt_remaining() >
            presentation.combat_feedback().hurt_duration * 0.5f) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        require(level->player(0).combat_feedback().hurt_remaining() > 0.0f,
            "hurt recovery fixture expired before its comparison");
        phase("hurt-recovery", true);
        while (level->player(0).combat_feedback().hurt_remaining() > 0.0f) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        compare_combat(app, *level, canvas, presentation, false);

        for (int view = 0; view < level->player_count(); ++view)
        {
            require(level->kill_player(view), "could not request combat-matrix elimination");
        }
        level->update(acceptance::slot_seats(*level, inputs), dt);
        phase("elimination-initial", true);
        for (int view = 0; view < level->player_count(); ++view)
        {
            const_cast<Player&>(level->player(view)).set_pose({ Vector2F::ZERO, 0, false, true });
        }
        compare_combat(app, *level, canvas, presentation, false);
        for (int view = 0; view < level->player_count(); ++view)
        {
            const_cast<Player&>(level->player(view)).set_pose({});
        }
        while (level->player(0).combat_feedback().elimination_remaining() >
            presentation.combat_feedback().elimination_duration * 0.5f) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        phase("elimination-middle", true);
        while (level->player(0).combat_feedback().elimination_remaining() >
            presentation.combat_feedback().elimination_duration * 0.15f) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        require(level->player(0).combat_feedback().elimination_remaining() > 0.0f,
            "elimination late fixture expired before its comparison");
        phase("elimination-late", true);
        while (level->player(0).combat_feedback().elimination_remaining() > 0.0f) { level->update(acceptance::slot_seats(*level, inputs), dt); }
        phase("elimination-expired", false);
        std::cout << "PASS controlled 4P combat on/off rasters at 720p/1080p/QHD; "
            "hurt initial/recovery and expiry; elimination initial/middle/late and expiry; whole-story suppression\n";
    }
