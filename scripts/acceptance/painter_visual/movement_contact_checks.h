// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// Movement contact step/skid matrix and its resolved walk/reversal.

    void compare_movement(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool expect_delta)
    {
        auto& look = const_cast<MovementFeedbackPresentation&>(presentation.movement_feedback());
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
                "frozen movement feedback changed during repeated drawing");
            look.enabled = false;
            const Frame disabled = isolated(&player, player, viewport, *app.renderer(), canvas);
            require(disabled.pixels == isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "disabled movement feedback changed during repeated drawing");
            require((enabled.pixels != disabled.pixels) == expect_delta,
                "movement contact draw switch did not match the active/expired phase");
        }
        require(before == observation(level), "movement drawing changed direction, tint or simulation state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "movement drawing changed the replay fingerprint");
#endif
    }

    // A real walk and reversal exercise the completed Scene contact phase.
    // Use the broad, unpainted recovery floor rather than a spawn near a ledge.
    void resolved_contacts(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, const std::filesystem::path& output, std::ofstream& records)
    {
        auto level = build(app, builder, 4, TeamColourPair::blue_orange);
        auto inputs = neutral_inputs();
        settle(*level, inputs);
        resize(app, *level, { 1280, 720 });
        for (int view = 0; view < level->player_count(); ++view)
        {
            require(level->place_player(view, Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                "could not place the resolved-contact fixture on the recovery floor");
        }
        for (int tick = 0; tick < 30; ++tick) { level->update(inputs, dt); }
        for (int view = 0; view < level->player_count(); ++view)
        {
            inputs[static_cast<size_t>(level->player(view).player_num())].x_movement =
                view % 2 == 0 ? 1.0f : -1.0f;
        }
        const auto all_emitted = [&](MovementFeedbackKind kind)
        {
            for (int view = 0; view < level->player_count(); ++view)
            {
                const auto effects = level->player(view).movement_feedback().effects();
                if (!std::any_of(effects.begin(), effects.end(), [kind](const auto& effect)
                    { return effect.kind == kind && !effect.paint_colour.has_value(); })) { return false; }
            }
            return true;
        };
        for (int tick = 0; tick < 60 && !all_emitted(MovementFeedbackKind::step); ++tick)
        {
            level->update(inputs, dt);
        }
        require(all_emitted(MovementFeedbackKind::step), "resolved walking did not produce four dry contact particles");
        capture(app, *level, canvas, output, "4p-contact-resolved-dry-step", records);
        const auto all_at_skid_speed = [&]()
        {
            for (int view = 0; view < level->player_count(); ++view)
            {
                if (std::abs(level->player(view).velocity().x) < tuning.player().movement_sounds.skid_min_speed)
                {
                    return false;
                }
            }
            return true;
        };
        for (int tick = 0; tick < 60 && !all_at_skid_speed(); ++tick) { level->update(inputs, dt); }
        require(all_at_skid_speed(), "resolved walking never reached the skid threshold");
        for (auto& input : inputs) { input.x_movement = -input.x_movement; }
        level->update(inputs, dt);
        require(all_emitted(MovementFeedbackKind::skid), "resolved reversal did not produce four dry skid particles");
        capture(app, *level, canvas, output, "4p-contact-resolved-dry-skid", records);
    }

    // Only this geometry/timing matrix emits directly: the real walk/reversal
    // above proves the producer. Each pane retains its real player, camera,
    // tool and culling path; views 1/2 are dry right/left, 3/4 painted right/left.
    void contact_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        const GameTuning& tuning, Presentation& presentation,
        const std::filesystem::path& output, std::ofstream& records)
    {
        auto& look = const_cast<MovementFeedbackPresentation&>(presentation.movement_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = true;
        resolved_contacts(app, builder, canvas, tuning, output, records);
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (const MovementFeedbackKind kind : { MovementFeedbackKind::step, MovementFeedbackKind::skid })
            {
                const float duration = kind == MovementFeedbackKind::step ? look.step.duration : look.skid.duration;
                const auto emit = [&](int view, bool reverse, bool swap_paint, float age)
                {
                    const Player& player = level->player(view);
                    auto& feedback = const_cast<MovementFeedback&>(player.movement_feedback());
                    const RectangleF body = player.rectangle();
                    const float direction = (view % 2 == 0 ? 1.0f : -1.0f) * (reverse ? -1.0f : 1.0f);
                    const bool painted = (view >= 2) != swap_paint;
                    feedback.clear();
                    feedback.contact(kind, Vector2F(body.x + body.width * 0.5f, body.bottom()), direction,
                        painted ? std::optional<Colour>(player.team_colour()) : std::nullopt);
                    if (age > 0.0f) { feedback.update(age); }
                };
                for (int view = 0; view < level->player_count(); ++view) { emit(view, false, false, 0.0f); }
                const std::string prefix = std::string("4p-contact-") +
                    (kind == MovementFeedbackKind::step ? "step-" : "skid-") + TeamColourTools::palette_key(palette);
                const auto phase = [&](const char* name, bool active)
                {
                    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                    {
                        resize(app, *level, size);
                        compare_movement(app, *level, canvas, presentation, active);
                        capture(app, *level, canvas, output, prefix + '-' + name + '-' + std::to_string(size.x), records);
                    }
                };
                phase("initial", true);
                for (int view = 0; view < level->player_count(); ++view) { emit(view, false, false, duration * 0.5f); }
                phase("middle", true);
                // Compare at the exact same pose, tint, camera and age. A
                // different character, body frame or pane cannot supply either delta.
                for (int view = 0; view < level->player_count(); ++view)
                {
                    const Player& player = level->player(view);
                    const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                    const Frame normal = isolated(&player, player, viewport, *app.renderer(), canvas);
                    emit(view, true, false, duration * 0.5f);
                    const Frame reversed = isolated(&player, player, viewport, *app.renderer(), canvas);
                    require(normal.pixels != reversed.pixels, "contact particle direction produced no visible raster change");
                    emit(view, false, true, duration * 0.5f);
                    const Frame other_surface = isolated(&player, player, viewport, *app.renderer(), canvas);
                    require(normal.pixels != other_surface.pixels, "dry and painted contact particles were indistinguishable");
                    emit(view, false, false, duration);
                    require(player.movement_feedback().effects().empty(), "contact particles did not retire at their duration");
                }
                phase("expired", false);
            }
        }
        std::cout << "PASS resolved dry walk/reversal; controlled 4P step/skid on/off rasters at 720p/1080p/QHD; "
            "three palettes, dry/painted and left/right; initial/middle/expired; frozen draw and culling bounds\n";
    }
