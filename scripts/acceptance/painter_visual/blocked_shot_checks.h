// Included inside painter_visual.cpp's private namespace, after painter_fixtures.h.
// Detached blocked-shot contact matrix.

    Frame blocked_context(const Player& player, const BlockedShotFeedback& feedback,
        const Camera& camera, const Viewport& viewport, const Vector2F& contact,
        const Vector2F& normal, Renderer& renderer, const Canvas& canvas, const Canvas& cover)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(camera);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport));
        player.draw(list);
        // The solid begins at the resolved face and overlaps the actual tool
        // nozzle. Drawing feedback last matches Level's world-overlay order.
        const Vector2F size = std::abs(normal.x) > 0.5f ? Vector2F(28, 56) : Vector2F(56, 28);
        const Vector2F center = contact - normal * 14.0f;
        cover.draw(list, RectangleF(center - size * 0.5f, size));
        feedback.draw(list);
        return read(renderer);
    }

    // Retain surrounding cover and the tool at native playing size. The
    // difference crop alone would lose the overlap that this cue must explain.
    Frame blocked_context_crop(const Frame& frame, const Camera& camera,
        const Viewport& viewport, const Vector2F& contact)
    {
        const RectangleI pane = viewport.pixel_rect();
        const Vector2F projected = camera.calculate_view_position(contact);
        const int center_x = pane.x + static_cast<int>(projected.x);
        const int center_y = pane.y + static_cast<int>(projected.y);
        const int left = std::max(pane.x, center_x - 90);
        const int top = std::max(pane.y, center_y - 70);
        const int right = std::min(pane.right(), center_x + 90);
        const int bottom = std::min(pane.bottom(), center_y + 70);
        Frame crop{ right - left, bottom - top, {} };
        for (int y = top; y < bottom; ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(left)) * 4;
            crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
                frame.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
        }
        return crop;
    }

    void blocked_level_capture(Application& app, Level& level, Presentation& presentation,
        const std::array<Vector2F, 4>& normals, const std::filesystem::path& output,
        const std::string& stem)
    {
        auto& look = const_cast<BlockedShotPresentation&>(presentation.blocked_shot());
        auto& feedback = const_cast<BlockedShotFeedback&>(level.blocked_shot_feedback());
        feedback.clear();
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            feedback.emit({ player.primary_weapon().nozzle_pos(), normals.at(static_cast<size_t>(view)),
                player.team_colour(), player.player_num() });
        }
        require(feedback.cues().size() == 4, "Level blocked-shot fixture did not retain four contacts");
        const RectangleF bounds = feedback.bounds();
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        const Frame enabled = render(level, *app.renderer());
        require(enabled.pixels == render(level, *app.renderer()).pixels,
            "Frozen Level blocked-shot redraw changed pixels");
        look.enabled = false;
        const Frame disabled = render(level, *app.renderer());
        require(disabled.pixels == render(level, *app.renderer()).pixels,
            "Disabled Level blocked-shot redraw changed pixels");
        look.enabled = true;
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            Frame crop{};
            require(check_projected_bounds(enabled, disabled,
                level.view_camera(view).calculate_view_rectangle(bounds), viewport, crop) > 0,
                "Level did not draw detached blocked-shot contacts in a live world pane");
            save(crop, output / (stem + "-player" + std::to_string(view + 1) + ".ppm"));
        }
        require(before == observation(level), "Level blocked-shot drawing changed simulation or cue timers");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "Level blocked-shot drawing changed replay fingerprint");
#endif
        feedback.clear();
    }

    // CPU acceptance exercises real blocked launches and resolved contacts.
    // Here the event age and cover orientation are controlled so hardware
    // rasters can establish visibility, projection and draw-only invariance.
    void blocked_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
        Presentation& presentation, const std::filesystem::path& output)
    {
        auto& look = const_cast<BlockedShotPresentation&>(presentation.blocked_shot());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = true;
        const Canvas cover(app.render_resources(), Colour(0.26f, 0.29f, 0.32f));
        const std::array<Vector2F, 4> normals{ Vector2F(-1, 0), Vector2F(1, 0),
            Vector2F(0, -1), Vector2F(0, 1) };
        const std::array<const char*, 4> normal_names{ "left", "right", "up", "down" };
        std::ofstream records(output / "blocked-shots.csv");
        records << "capture,view,input_slot,normal_x,normal_y,age,changed_pixels,bounds_x,bounds_y,bounds_width,bounds_height\n";
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            auto level = build(app, builder, 4, palette);
            const auto inputs = neutral_inputs();
            settle(*level, inputs);
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
            {
                if (size.x != 1280 && palette != TeamColourPair::blue_orange) { continue; }
                resize(app, *level, size);
                const std::string prefix = std::string("blocked-4p-") + TeamColourTools::palette_key(palette) +
                    '-' + std::to_string(size.x);
                blocked_level_capture(app, *level, presentation, normals, output, prefix + "-level");
                const auto level_before = observation(*level);
                for (int view = 0; view < level->player_count(); ++view)
                {
                    const Player& player = level->player(view);
                    const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                    const RectangleI pane = viewport.pixel_rect();
                    const Vector2F point = player.primary_weapon().nozzle_pos();
                    const Vector2F normal = normals.at(static_cast<size_t>(view));
                    // Keep the actual gameplay scale while framing the fixed
                    // contact identically; no screenshot crop is enlarged.
                    const float scale = player.camera().scale;
                    const Camera camera(point - Vector2F(static_cast<float>(pane.width) / (2.0f * scale),
                        static_cast<float>(pane.height) / (2.0f * scale)), scale);
                    BlockedShotFeedback feedback(&look, app.render_resources());
                    Frame initial{};
                    for (int phase = 0; phase < 3; ++phase)
                    {
                        const float age = look.duration * static_cast<float>(phase) * 0.5f;
                        feedback.clear();
                        feedback.emit({ point, normal, player.team_colour(), player.player_num() });
                        require(feedback.cues().size() == 1, "Controlled blocked-shot contact was not retained");
                        const RectangleF cue_bounds = feedback.cue_bounds(feedback.cues().front());
                        require(feedback.bounds() == cue_bounds, "Single blocked-shot aggregate omitted its cue bounds");
                        feedback.update(age);
                        const bool active = phase < 2;
                        require(feedback.cues().empty() != active, "Blocked-shot expiry did not match its authored duration");
                        const auto before = blocked_observation(feedback);
                        const auto draw = [&]()
                        {
                            return blocked_context(player, feedback, camera, viewport, point, normal,
                                *app.renderer(), canvas, cover);
                        };
                        const Frame enabled = draw();
                        for (int repeat = 0; repeat < 3; ++repeat)
                        {
                            require(enabled.pixels == draw().pixels,
                                "Frozen blocked-shot context redraw changed pixels");
                        }
                        look.enabled = false;
                        const Frame disabled = draw();
                        require(disabled.pixels == draw().pixels,
                            "Disabled blocked-shot context redraw changed pixels");
                        look.enabled = true;
                        Frame crop{};
                        const size_t changed = check_projected_bounds(enabled, disabled,
                            camera.calculate_view_rectangle(cue_bounds), viewport, crop);
                        require(active ? changed > 0 : changed == 0,
                            "Blocked-shot cover/nozzle overlap visibility did not match its lifetime");
                        const char* phase_name = phase == 0 ? "initial" : phase == 1 ? "middle" : "expired";
                        const std::string stem = prefix + '-' + normal_names.at(static_cast<size_t>(view)) + '-' + phase_name;
                        const std::string suffix = "-player" + std::to_string(view + 1) + ".ppm";
                        if (active) { save(crop, output / (stem + suffix)); }
                        save(blocked_context_crop(enabled, camera, viewport, point), output / (stem + "-context" + suffix));
                        if (phase == 0) { initial = enabled; }
                        if (phase == 1)
                        {
                            require(initial.pixels != enabled.pixels,
                                "Blocked-shot contact did not visibly rebound/fade at midlife");
                        }
                        records << stem << ',' << view + 1 << ',' << player.player_num() << ',' << normal.x << ','
                            << normal.y << ',' << age << ',' << changed << ',' << cue_bounds.x << ',' << cue_bounds.y << ','
                            << cue_bounds.width << ',' << cue_bounds.height << '\n';
                        require(before == blocked_observation(feedback),
                            "Blocked-shot drawing/toggling changed contact snapshot or timers");
                    }
                }
                require(level_before == observation(*level),
                    "Controlled blocked-shot raster fixture changed its real player or Level state");
            }
        }
        require(records.good(), "Blocked-shot capture record write failed");
        std::cout << "PASS detached blocked-shot Level on/off rasters, native 4P cover/nozzle overlap; "
            "four surface normals, three palettes at 720p and representative 1080p/QHD; "
            "initial/middle/expired, projected cue/aggregate bounds, frozen redraw/contact ages and replay fingerprint\n";
    }
