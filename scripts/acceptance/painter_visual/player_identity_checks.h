// Included inside painter_visual.cpp's private namespace, after the shared
// raster helpers and Debug tune-colour helpers. Player identity matrix.

    Frame identity_context_crop(const Frame& frame, const Level& level, int view,
        const Viewport& viewport)
    {
        const RectangleF body = level.player(view).rectangle();
        const RectangleF badge = level.player_identity_bounds(view);
        const float left = std::min(body.left(), badge.left());
        const float top = std::min(body.top(), badge.top());
        const float right = std::max(body.right(), badge.right());
        const float bottom = std::max(body.bottom(), badge.bottom());
        const RectangleF projected = level.view_camera(view).calculate_view_rectangle(
            RectangleF(left, top, right - left, bottom - top));
        const RectangleI pane = viewport.pixel_rect();
        const int x0 = std::max(pane.x, pane.x + static_cast<int>(std::floor(projected.x)) - 12);
        const int y0 = std::max(pane.y, pane.y + static_cast<int>(std::floor(projected.y)) - 12);
        const int x1 = std::min(pane.right(), pane.x + static_cast<int>(std::ceil(projected.right())) + 12);
        const int y1 = std::min(pane.bottom(), pane.y + static_cast<int>(std::ceil(projected.bottom())) + 12);
        require(x1 > x0 && y1 > y0, "Identity context did not intersect its own pane");
        Frame crop{ x1 - x0, y1 - y0, {} };
        for (int y = y0; y < y1; ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(x0)) * 4;
            crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
                frame.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
        }
        return crop;
    }

    void identity_capture(Application& app, Level& level, Presentation& presentation,
        const std::filesystem::path& output, const std::string& stem, std::ofstream& records)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Frozen identity Level redraw changed pixels");
        auto& look = const_cast<PlayerIdentityPresentation&>(presentation.player_identity());
        // Hide only the world badge at the identical state. The same shared
        // renderer still draws the pane's UI badge, so every delta below must
        // belong to the owner above the figure, not its HUD or another player.
        const Vector2F world_size = look.world_size;
        look.world_size = Vector2F::ZERO;
        const Frame disabled = render(level, *app.renderer());
        look.world_size = world_size;
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Restoring world identity size did not restore its raster");
        save(enabled, output / (stem + ".ppm"));
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const RectangleF bounds = level.player_identity_bounds(view);
            Frame crop{};
            const size_t changed = check_projected_bounds(enabled, disabled,
                level.view_camera(view).calculate_view_rectangle(bounds), viewport, crop);
            require(level.player_identity_visible(view) ? changed > 0 : changed == 0,
                "World identity visibility or own-pane raster bounds did not match lifecycle");
            if (changed > 0)
            {
                save(identity_context_crop(enabled, level, view, viewport), output /
                    (stem + "-context-player" + std::to_string(view + 1) + ".ppm"));
                save(crop, output / (stem + "-badge-player" + std::to_string(view + 1) + ".ppm"));
            }
            records << stem << ',' << view + 1 << ',' << level.player(view).player_num() << ','
                << level.player_identity_visible(view) << ',' << level.player_identity_emphasis(view) << ','
                << changed << ',' << bounds.x << ',' << bounds.y << ',' << bounds.width << ',' << bounds.height << '\n';
        }
        require(before == observation(level), "Identity draws changed simulation or cosmetic timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Identity draws changed replay fingerprint");
#endif
    }

    void identity_matrix(Application& app, LevelBuilder& builder, Registry<LevelLoadedInfo>& levels,
        GameTuning& tuning, Presentation& presentation, PlaySettings& preferences,
        const std::filesystem::path& content, const std::filesystem::path& output)
    {
        const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
        preferences.reduce_effects = false;
        std::ofstream records(output / "player-identity.csv");
        records << "capture,view,input_slot,visible,emphasis,changed_pixels,bounds_x,bounds_y,bounds_width,bounds_height\n";
        const auto inputs = neutral_inputs();
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette, false, true, WeaponType::sprayer);
            settle(*level, inputs);
            resize(app, *level, { 1280, 720 });
            // All four identical same-team tools overlap. Only roster identity
            // distinguishes the figures; teammates share the team colour.
            for (int view = 0; view < 4; ++view)
            {
                require(level->place_player(view, { 1400.0f + 12.0f * static_cast<float>(view), 1275.0f }),
                    "Could not overlap same-team identity fixtures");
            }
            for (int tick = 0; tick < 600 && level->player_identity_emphasis(0) > 0.0f; ++tick)
            {
                level->update(acceptance::slot_seats(*level, inputs), dt);
            }
            require(level->player_identity_emphasis(0) == 0.0f, "Identity emphasis did not settle");
            const std::string prefix = std::string("4p-identity-") + TeamColourTools::palette_key(palette);
            identity_capture(app, *level, presentation, output, prefix + "-overlap-quiet-1280", records);
            const RectangleF quiet = level->player_identity_bounds(0);
            if (palette == TeamColourPair::blue_orange)
            {
                identity_ink_consumer_checks(app, *level, presentation, output);
                identity_background_consumer_checks(app, *level, presentation, output);
#if COLOURWARS_DEV_TOOLS
                identity_ink_tune_checks(app, *level, levels, tuning, presentation, content, output);
                identity_background_tune_checks(app, *level, levels, tuning, presentation, content, output);
#else
                std::ignore = levels; std::ignore = tuning; std::ignore = content;
#endif
                // Pin the shared renderer at exactly one pane-local position
                // and native gameplay size. Team tint, bodies and subpixel
                // camera differences cannot make two roster digits differ.
                const Viewport& viewport = app.viewport_manager()->player_viewport(0);
                const RectangleF projected = level->view_camera(0).calculate_view_rectangle(quiet);
                const RectangleF box(Vector2F(24, 24), projected.size());
                const Canvas canvas(app.render_resources());
                const PlayerBadge badge(&presentation.player_identity(), app.render_resources());
                const auto glyph = [&](int roster)
                {
                    Renderer& renderer = *app.renderer();
                    renderer.begin_frame();
                    renderer.set_view_count(1);
                    DrawList list = renderer.view(0);
                    list.set_viewport(viewport);
                    list.set_camera(Camera::DEFAULT_CAMERA);
                    canvas.draw(list, RectangleF(Vector2F::ZERO, viewport.size()));
                    if (roster >= 0) { badge.draw(list, box, roster); }
                    return read(renderer);
                };
                const Frame blank = glyph(-1);
                std::vector<Frame> digits;
                for (int roster = 0; roster < 4; ++roster)
                {
                    const Frame image = glyph(roster);
                    Frame crop{};
                    require(check_projected_bounds(image, blank, box, viewport, crop) > 0,
                        "Shared roster badge produced no native-size glyph");
                    for (const Frame& earlier : digits)
                    {
                        require(crop.pixels != earlier.pixels,
                            "Two roster badge numbers have identical native-size rasters");
                    }
                    save(crop, output / ("4p-identity-glyph-quiet-1280-player" +
                        std::to_string(roster + 1) + ".ppm"));
                    digits.push_back(std::move(crop));
                }
            }
            level->emphasise_players();
            require(level->player_identity_emphasis(0) == 1.0f &&
                level->player_identity_bounds(0).width > quiet.width,
                "Requested identity emphasis did not expand the world badge");
            identity_capture(app, *level, presentation, output, prefix + "-overlap-emphasis-1280", records);
            preferences.reduce_effects = true;
            require(level->player_identity_visible(0) && level->player_identity_emphasis(0) == 0.0f &&
                level->player_identity_bounds(0).width == quiet.width,
                "Reduce effects hid identity or retained its animated emphasis");
            identity_capture(app, *level, presentation, output, prefix + "-overlap-reduced-1280", records);
            preferences.reduce_effects = false;
            if (palette != TeamColourPair::blue_orange) { continue; }
            for (const Vector2I size : { Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                resize(app, *level, size);
                identity_capture(app, *level, presentation, output,
                    prefix + "-overlap-emphasis-" + std::to_string(size.x), records);
            }
            resize(app, *level, { 1280, 720 });
            auto crossing = inputs;
            require(level->place_player(0, { 1400, 1275 }) && level->place_player(1, { 1512, 1275 }),
                "Could not place teammate crossing fixture");
            crossing[static_cast<size_t>(level->player(0).player_num())].x_movement = 1.0f;
            crossing[static_cast<size_t>(level->player(1).player_num())].x_movement = -1.0f;
            for (int tick = 0; tick < 60 &&
                level->player(0).rectangle().x <= level->player(1).rectangle().x; ++tick)
            {
                level->update(acceptance::slot_seats(*level, crossing), dt);
            }
            require(level->player(0).rectangle().x > level->player(1).rectangle().x,
                "Same-team scripted players did not cross");
            identity_capture(app, *level, presentation, output, prefix + "-crossed-1280", records);
            level->suspend();
            identity_capture(app, *level, presentation, output, prefix + "-paused-1280", records);
            level->resume();
            require(level->kill_player(0), "Could not kill identity fixture");
            level->update(acceptance::slot_seats(*level, inputs), dt);
            require(!level->player_identity_visible(0), "Dead player retained an owner badge");
            identity_capture(app, *level, presentation, output, prefix + "-dead-1280", records);
            for (int tick = 0; tick < 600 && level->player(0).state() != PlayerState::alive; ++tick)
            {
                level->update(acceptance::slot_seats(*level, inputs), dt);
            }
            require(level->player(0).state() == PlayerState::alive &&
                level->player_identity_visible(0) && level->player_identity_emphasis(0) > 0.0f,
                "Respawn did not restore and emphasise the owning identity");
            identity_capture(app, *level, presentation, output, prefix + "-respawn-1280", records);
        }
        auto sparse = build(app, builder, 2, TeamColourPair::blue_orange, false, true);
        settle(*sparse, inputs);
        resize(app, *sparse, { 1280, 720 });
        identity_capture(app, *sparse, presentation, output, "2p-identity-sparse-slots-1280", records);
        auto story = build(app, builder, 4, TeamColourPair::blue_orange,
            false, true, WeaponType::sprayer, OpeningMode::quick);
        resize(app, *story, { 1280, 720 });
        require(story->intro_playing() && !story->player_identity_visible(0),
            "Story fixture did not suppress world identity");
        identity_capture(app, *story, presentation, output, "4p-identity-story-hidden-1280", records);
        require(records.good(), "Identity record write failed");
        std::cout << "PASS player identity: three 4P720 palettes, same-team overlap/crossing, sparse/reordered input slots, quiet/emphasis/reduced effects, 1080p/QHD bounds, dead/respawn/story/pause, own-pane world-only raster deltas, frozen cosmetic state and fingerprint\n";
    }
