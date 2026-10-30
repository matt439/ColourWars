// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// Idle drip and quiet-character matrices.

    // The decoration is a separate Scene object, so its falling footprint must
    // be checked separately from the player's culling box. Keep an additional
    // combined crop for judging the bead at its real nozzle and playing size.
    bool raster_adjacent(const Frame& decoration, const Frame& painter,
        const Frame& blank, const Viewport& viewport)
    {
        const RectangleI pane = viewport.pixel_rect();
        const auto ink = [&](const Frame& image, int x, int y)
        {
            const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                static_cast<size_t>(x)) * 4;
            return image.pixels[offset] != blank.pixels[offset] ||
                image.pixels[offset + 1] != blank.pixels[offset + 1] ||
                image.pixels[offset + 2] != blank.pixels[offset + 2];
        };
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                if (!ink(decoration, x, y)) { continue; }
                for (int neighbour_y = std::max(pane.y, y - 1);
                    neighbour_y <= std::min(pane.bottom() - 1, y + 1); ++neighbour_y)
                {
                    for (int neighbour_x = std::max(pane.x, x - 1);
                        neighbour_x <= std::min(pane.right() - 1, x + 1); ++neighbour_x)
                    {
                        if (ink(painter, neighbour_x, neighbour_y)) { return true; }
                    }
                }
            }
        }
        return false;
    }

    void quiet_capture(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output,
        const std::string& stem, std::ofstream& records, std::ofstream& quiet_records)
    {
        capture(app, level, canvas, output, stem, records);
        const auto before = observation(level);
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            auto& look = const_cast<IdleDripPresentation&>(presentation.idle_drip(player.primary_weapon().type()));
            const RestoreFlag restore{ look.enabled, look.enabled };
            const IdleDrip* drip = level.idle_drip(view);
            require(drip != nullptr, "wet tool builder omitted its Scene-owned idle drip");
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const Frame blank = isolated(nullptr, player, viewport, *app.renderer(), canvas);
            look.enabled = true;
            const Frame image = isolated(nullptr, player, viewport, *app.renderer(), canvas, drip);
            require(image.pixels == isolated(nullptr, player, viewport, *app.renderer(), canvas, drip).pixels,
                "frozen idle drip changed during repeated drawing");
            Frame crop{};
            const size_t pixels = check_projected_bounds(image, blank,
                player.camera().calculate_view_rectangle(drip->cull_bounds(1.0f / player.camera().scale)),
                viewport, crop);
            require((pixels > 0) == (drip->attached() || !drip->drops().empty()),
                "idle drip raster visibility did not match its bead/drop state");
            if (drip->attached() && drip->bead_age() >= look.grow_time)
            {
                const Frame painter = isolated(&player, player, viewport, *app.renderer(), canvas);
                require(raster_adjacent(image, painter, blank, viewport),
                    "mature attached idle bead did not meet the rendered wet lip within one pixel");
            }
            look.enabled = false;
            require(blank.pixels == isolated(nullptr, player, viewport, *app.renderer(), canvas, drip).pixels,
                "disabled idle drip still drew pixels");
            look.enabled = true;
            const Frame combined = isolated(&player, player, viewport, *app.renderer(), canvas, drip);
            RectangleF bounds = player.cull_bounds(1.0f / player.camera().scale);
            if (pixels > 0)
            {
                bounds = RectangleF::union_of(bounds, drip->cull_bounds(1.0f / player.camera().scale));
            }
            require(check_projected_bounds(combined, blank,
                player.camera().calculate_view_rectangle(RectangleF(bounds)), viewport, crop) > 0,
                "quiet detail crop omitted its painter");
            save(crop, output / (stem + "-detail-player" + std::to_string(view + 1) + ".ppm"));
            quiet_records << stem << ',' << view + 1 << ',' << drip->attached() << ','
                << drip->bead_age() << ',' << drip->interval_remaining() << ','
                << drip->drops().size() << ',' << pixels << ','
                << player.quiet_character().state().breath << ','
                << player.quiet_character().state().lean << ','
                << player.quiet_character().state().blinking << '\n';
        }
        require(before == observation(level), "quiet detail drawing changed observed simulation or cosmetic state");
    }

    void compare_quiet(Application& app, Level& level, const Canvas& canvas,
        Presentation& presentation, bool blinking)
    {
        const auto before = observation(level);
        auto& enabled = const_cast<bool&>(presentation.figure().quiet.enabled);
        const RestoreFlag restore{ enabled, enabled };
        bool changed = false;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            enabled = true;
            const Frame active = isolated(&player, player, viewport, *app.renderer(), canvas);
            enabled = false;
            changed |= active.pixels != isolated(&player, player, viewport, *app.renderer(), canvas).pixels;
        }
        require(changed, "quiet breathing/weight shift produced no visible playing-size raster change");
        enabled = true;
        if (blinking)
        {
            const Player& player = level.player(0);
            auto& blink = const_cast<bool&>(player.quiet_character().state().blinking);
            const RestoreFlag restore_blink{ blink, blink };
            require(blink, "quiet blink fixture missed its closed-eye phase");
            const Viewport& viewport = app.viewport_manager()->player_viewport(0);
            const Frame closed = isolated(&player, player, viewport, *app.renderer(), canvas);
            blink = false;
            require(closed.pixels != isolated(&player, player, viewport, *app.renderer(), canvas).pixels,
                "quiet blink did not change the eyes at playing size");
        }
        require(before == observation(level), "quiet on/off comparison changed observed simulation or cosmetic state");
    }

    void quiet_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output, std::ofstream& records)
    {
        std::ofstream quiet_records(output / "quiet-detail.csv");
        quiet_records << "capture,view,attached,bead_age,interval_remaining,drops,drip_pixels,breath,lean,blinking\n";
        const auto& look = presentation.idle_drip();
        for (const int count : { 1, 4 })
        {
            for (const TeamColourPair palette : { TeamColourPair::blue_orange,
                TeamColourPair::pink_green, TeamColourPair::blue_yellow })
            {
                auto level = build(app, builder, count, palette, false, false, WeaponType::sprayer);
                auto inputs = neutral_inputs();
                settle(*level, inputs);
                resize(app, *level, { 1280, 720 });
                for (int view = 0; view < count; ++view)
                {
                    require(level->place_player(view, Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                        "could not place the quiet fixture on the recovery floor");
                }
                for (int tick = 0; tick < 60; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
                const std::string prefix = std::to_string(count) + "p-quiet-" + TeamColourTools::palette_key(palette);
                const IdleDrip* drip = level->idle_drip(0);
                require(drip != nullptr, "quiet fixture has no sprayer drip");
                const auto advance_until = [&](const auto& condition, const char* message)
                {
                    for (int tick = 0; tick < 900 && !condition(); ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
                    require(condition(), message);
                    require(level->state() == LevelState::active, "quiet fixture exhausted its real match clock");
                };
                const auto phase = [&](const std::string& name, bool extents = false)
                {
                    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                    {
                        if (size.x != 1280 && (!extents || palette != TeamColourPair::blue_orange)) { continue; }
                        resize(app, *level, size);
                        quiet_capture(app, *level, canvas, presentation, output,
                            prefix + '-' + name + '-' + std::to_string(size.x), records, quiet_records);
                    }
                    resize(app, *level, { 1280, 720 });
                };
                advance_until([&] { return level->player(0).quiet_character().state().breath >=
                    presentation.figure().quiet.breath_height * 0.9f; },
                    "settled character never reached its breathing crest");
                compare_quiet(app, *level, canvas, presentation, false);
                if (count == 4)
                {
                    require(level->player(0).quiet_character().state() != level->player(1).quiet_character().state(),
                        "separate roster members shared one quiet character phase");
                }
                phase("breath");
                advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time * 0.5f &&
                    drip->bead_age() < look.grow_time; }, "settled sprayer never grew a muzzle bead");
                phase("grow");
                advance_until([&] { return drip->attached() &&
                    drip->bead_age() >= look.grow_time + look.stretch_time * 0.5f; },
                    "settled sprayer never stretched its muzzle bead");
                phase("stretch", true);
                advance_until([&] { return !drip->drops().empty() && drip->drops()[0].age >= dt * 2.0f; },
                    "settled sprayer never released a falling cosmetic drop");
                phase("fall");
                const float drop_y = drip->drops()[0].position.y;
                level->update(acceptance::slot_seats(*level, inputs), dt);
                require(drip->drops().empty() || drip->drops()[0].position.y > drop_y,
                    "released idle drop did not fall downwards");
                advance_until([&] { return level->player(0).quiet_character().state().blinking; },
                    "settled character never blinked");
                compare_quiet(app, *level, canvas, presentation, true);
                phase("blink");
                advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time * 0.5f; },
                    "idle bead did not return for the interruption check");
                for (auto& input : inputs) { input.x_movement = -1.0f; }
                level->update(acceptance::slot_seats(*level, inputs), dt);
                require(!drip->attached(), "starting movement did not remove the attached idle bead");
                phase("interrupted");
                for (int tick = 0; tick < 6; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
                phase("lean-left");
            }
        }
        require(quiet_records.good(), "quiet detail capture record write failed");
        std::cout << "PASS actual settled 1P/4P sprayer grow/stretch/fall/interruption and quiet breath/blink/lean; "
            "three 720p palettes, representative 1080p/QHD; frozen redraw, on/off rasters and Scene culling bounds\n";
    }

    void wet_tool_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, PlaySettings& preferences,
        const std::filesystem::path& output, std::ofstream& records)
    {
        const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
        preferences.reduce_effects = false;
        std::ofstream details(output / "quiet-tools-detail.csv");
        details << "capture,view,attached,bead_age,interval_remaining,drops,drip_pixels,breath,lean,blinking\n";
        for (const auto& tool : std::array<std::pair<WeaponType, const char*>, 2>{{
            { WeaponType::sniper, "sniper" }, { WeaponType::bucket, "bucket" } }})
        {
            const auto& look = presentation.idle_drip(tool.first);
            for (const int count : { 1, 4 })
            {
                for (const TeamColourPair palette : { TeamColourPair::blue_orange,
                    TeamColourPair::pink_green, TeamColourPair::blue_yellow })
                {
                    auto level = build(app, builder, count, palette, false, false, tool.first);
                    auto inputs = neutral_inputs();
                    settle(*level, inputs);
                    resize(app, *level, { 1280, 720 });
                    for (int view = 0; view < count; ++view)
                    {
                        require(level->place_player(view,
                            Vector2F(1200.0f + 200.0f * static_cast<float>(view), 1275)),
                            "could not place the wet-tool fixture on the recovery floor");
                    }
                    for (int tick = 0; tick < 60; ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
                    const IdleDrip* drip = level->idle_drip(0);
                    require(drip != nullptr, "wet-tool fixture omitted its Scene-owned drip");
                    const std::string prefix = std::to_string(count) + "p-quiet-" + tool.second + '-' +
                        TeamColourTools::palette_key(palette);
                    const auto advance_until = [&](const auto& condition, const char* message)
                    {
                        for (int tick = 0; tick < 900 && !condition(); ++tick) { level->update(acceptance::slot_seats(*level, inputs), dt); }
                        require(condition(), message);
                        require(level->state() == LevelState::active, "wet-tool fixture exhausted its real match clock");
                    };
                    const auto phase = [&](const std::string& name, bool extents = false)
                    {
                        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                        {
                            if (size.x != 1280 && (!extents || palette != TeamColourPair::blue_orange)) { continue; }
                            resize(app, *level, size);
                            quiet_capture(app, *level, canvas, presentation, output,
                                prefix + '-' + name + '-' + std::to_string(size.x), records, details);
                        }
                        resize(app, *level, { 1280, 720 });
                    };
                    const auto reduced = [&](const char* name)
                    {
                        const auto before = observation(*level);
#if COLOURWARS_DEV_TOOLS
                        const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 0);
#endif
                        for (int view = 0; view < count; ++view)
                        {
                            const Player& player = level->player(view);
                            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                            const IdleDrip* decoration = level->idle_drip(view);
                            const Frame blank = isolated(nullptr, player, viewport, *app.renderer(), canvas);
                            const Frame normal = isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration);
                            if (view == 0) { require(normal.pixels != blank.pixels, "wet-tool reduction fixture has no visible liquid"); }
                            preferences.reduce_effects = true;
                            const Frame hidden = isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration);
                            require(hidden.pixels == blank.pixels && hidden.pixels ==
                                isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration).pixels,
                                "reduce effects did not hide the frozen wet-tool bead/drop");
                            preferences.reduce_effects = false;
                            require(normal.pixels == isolated(nullptr, player, viewport, *app.renderer(), canvas, decoration).pixels,
                                "normal wet-tool liquid did not return after reducing effects");
                        }
                        preferences.reduce_effects = true;
                        capture(app, *level, canvas, output, prefix + '-' + name + "-reduced-1280", records);
                        preferences.reduce_effects = false;
                        require(before == observation(*level), "wet-tool reduced draws changed simulation/cosmetic state");
#if COLOURWARS_DEV_TOOLS
                        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
                            "wet-tool reduced draws changed the replay fingerprint");
#endif
                    };
                    advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time * 0.5f &&
                        drip->bead_age() < look.grow_time; }, "settled wet tool never grew a bead");
                    phase("grow");
                    advance_until([&] { return drip->attached() &&
                        drip->bead_age() >= look.grow_time + look.stretch_time * 0.5f; },
                        "settled wet tool never stretched its bead");
                    phase("stretch", true);
                    reduced("stretch");
                    // Change the real stick aim while this same mature bead is
                    // still attached. The art's local wet point is measured
                    // from the left-centre draw pivot, independently of the
                    // projectile launch point used by the runtime attachment.
                    int quadrant = 0;
                    for (const float angle : { PI / 4.0f, 3.0f * PI / 4.0f, -3.0f * PI / 4.0f, -PI / 4.0f })
                    {
                        const float age = drip->bead_age();
                        for (auto& input : inputs)
                        {
                            input.shoot_angle = angle;
                            input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
                        }
                        level->update(acceptance::slot_seats(*level, inputs), dt);
                        require(drip->attached() && drip->bead_age() > age,
                            "continuous wet-tool aim replaced or released the attached bead");
                        for (int view = 0; view < count; ++view)
                        {
                            const IdleDrip* attached = level->idle_drip(view);
                            if (!attached->attached()) { continue; }
                            const Weapon& weapon = level->player(view).primary_weapon();
                            const float side = std::abs(angle) > PI / 2.0f ? -1.0f : 1.0f;
                            const Vector2F local = tool.first == WeaponType::sniper ? Vector2F(99, 3) : Vector2F(46, 8);
                            const Vector2F expected = weapon.draw_pos() + Vector2F::rotate_vector(
                                Vector2F(local.x, side * local.y), weapon.rotation());
                            const RectangleF bead = attached->bead_rectangle();
                            require(std::abs(bead.center().x - expected.x) < 0.01f && std::abs(bead.y - expected.y) < 0.01f,
                                "wet bead missed the authored muzzle lip/bucket runoff stripe after aiming/flipping");
                        }
                        phase("aim" + std::to_string(++quadrant));
                    }
                    advance_until([&] { return !drip->drops().empty() && drip->drops()[0].age >= dt * 2.0f; },
                        "settled wet tool never released a falling cosmetic drop");
                    phase("fall");
                    reduced("fall");
                    const float drop_y = drip->drops()[0].position.y;
                    level->update(acceptance::slot_seats(*level, inputs), dt);
                    require(drip->drops().empty() || drip->drops()[0].position.y > drop_y,
                        "released wet-tool drop did not fall downwards");
                    for (auto& input : inputs) { input.x_movement = -1.0f; }
                    level->update(acceptance::slot_seats(*level, inputs), dt);
                    require(!drip->attached(), "wet-tool movement did not interrupt its bead");
                    for (auto& input : inputs)
                    {
                        input.x_movement = 0;
                        input.shoot_angle = PI;
                        input.shoot_direction = Vector2F(-1, 0);
                    }
                    advance_until([&] { return drip->attached() && drip->bead_age() >= look.grow_time; },
                        "left-facing settled wet tool never regrew a mature bead");
                    require(!level->player(0).facing_right(), "wet-tool left-facing fixture stayed right-facing");
                    phase("left-attached", true);
                }
            }
        }
        require(details.good(), "wet-tool detail capture record write failed");
        std::cout << "PASS actual 1P/4P sniper residual bead and bucket rim runoff: grow/stretch/fall, "
            "four continuous aim quadrants, flipped lip/stripe attachment and both body facings; "
            "all 720p palettes, representative 1080p/QHD; frozen redraw, bounds and reduced-effects suppression\n";
    }
