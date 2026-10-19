// Included inside painter_visual.cpp's private namespace, before every other
// painter_visual/ header: the shared probe fixtures and raster helpers.

    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    struct Frame
    {
        int width;
        int height;
        std::vector<unsigned char> pixels;
    };

    struct RestoreFlag
    {
        bool& setting;
        bool value;
        ~RestoreFlag() { this->setting = this->value; }
    };

    Frame read(Renderer& renderer)
    {
        renderer.submit();
        const Vector2F size = renderer.back_buffer_size();
        Frame frame{ static_cast<int>(size.x), static_cast<int>(size.y), {} };
        renderer.read_back_buffer(frame.pixels);
        require(frame.pixels.size() == static_cast<size_t>(frame.width) *
            static_cast<size_t>(frame.height) * 4, "readback size mismatch");
        return frame;
    }

    Frame render(const Level& level, Renderer& renderer)
    {
        renderer.begin_frame();
        level.draw(renderer);
        return read(renderer);
    }

    // Points LOCALAPPDATA at a nested profile for one matrix. A matrix whose
    // matches finish writes stage records, and the probe's shared save
    // fixture must stay byte-identical.
    struct NestedSaveProfile
    {
        std::wstring previous;
        explicit NestedSaveProfile(const std::filesystem::path& path)
        {
            wchar_t* raw = nullptr; size_t length = 0;
            require(_wdupenv_s(&raw, &length, L"LOCALAPPDATA") == 0, "Cannot read the probe profile");
            if (raw) { this->previous = raw; std::free(raw); }
            std::filesystem::create_directories(path);
            require(_wputenv_s(L"LOCALAPPDATA", path.c_str()) == 0, "Cannot isolate a matrix's saves");
        }
        ~NestedSaveProfile() { _wputenv_s(L"LOCALAPPDATA", this->previous.c_str()); }
        NestedSaveProfile(const NestedSaveProfile&) = delete;
        NestedSaveProfile& operator=(const NestedSaveProfile&) = delete;
    };

    void save(const Frame& frame, const std::filesystem::path& path)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
        std::vector<unsigned char> rgb(frame.pixels.size() / 4 * 3);
        size_t target = 0;
        for (size_t source = 0; source < frame.pixels.size(); source += 4)
        {
            for (size_t channel = 0; channel < 3; ++channel) { rgb[target++] = frame.pixels[source + channel]; }
        }
        file.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
        require(file.good(), "capture write failed");
    }

    std::vector<float> blocked_observation(const BlockedShotFeedback& feedback)
    {
        std::vector<float> values{ static_cast<float>(feedback.cues().size()) };
        for (const auto& cue : feedback.cues())
        {
            const auto& contact = cue.contact;
            values.insert(values.end(), { contact.position.x, contact.position.y,
                contact.normal.x, contact.normal.y, contact.team_colour.r,
                contact.team_colour.g, contact.team_colour.b, contact.team_colour.a,
                static_cast<float>(contact.player_num), cue.age });
        }
        for (int slot = 0; slot < input_slots::count; ++slot)
        {
            values.push_back(feedback.repeat_remaining(slot));
        }
        return values;
    }

    // The same public simulation evidence used by the replay fingerprint, plus
    // aim, facing and movement effects. It is also available in Release.
    std::vector<float> observation(const Level& level)
    {
        const LevelEndInfo info = level.level_end_info();
        std::vector<float> values{ static_cast<float>(level.state()),
            info.team_a_area, info.team_b_area, info.total_area };
        for (const float area : info.player_areas) { values.push_back(area); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const RectangleF body = player.rectangle();
            const Weapon& tool = player.primary_weapon();
            const ToolMotionState& mechanism = tool.motion_state();
            require(tool.feedback() != nullptr, "real builder omitted weapon feedback");
            const WeaponFeedbackState& weapon_feedback = tool.feedback()->state();
            const CombatFeedback& combat = player.combat_feedback();
            const RectangleF death = combat.death_rectangle();
            const RectangleF identity = level.player_identity_bounds(view);
            const QuietCharacter::State& quiet = player.quiet_character().state();
            values.insert(values.end(), { body.x, body.y, body.width, body.height,
                player.velocity().x, player.velocity().y, player.health(), player.weapon_ammo(),
                player.respawn_timer(), static_cast<float>(player.state()),
                static_cast<float>(player.move_state()), player.facing_right() ? 1.0f : 0.0f,
                tool.rotation(), tool.nozzle_pos().x, tool.nozzle_pos().y,
                static_cast<float>(player.animation_state()), player.action_age(),
                static_cast<float>(player.action_frame()),
                static_cast<float>(tool.visual_pose()), static_cast<float>(tool.readiness()), mechanism.pressure,
                mechanism.valve, mechanism.travel_phase, mechanism.lift,
                mechanism.compression, mechanism.bucket_swing, mechanism.hose_sway,
                mechanism.mist_tail_remaining, weapon_feedback.muzzle_remaining,
                weapon_feedback.empty_remaining, weapon_feedback.recovered_remaining, combat.hurt_remaining(),
                combat.elimination_remaining(), death.x, death.y, death.width, death.height,
                quiet.breath, quiet.lean, quiet.blinking ? 1.0f : 0.0f });
            values.insert(values.end(), { level.player_identity_visible(view) ? 1.0f : 0.0f,
                level.player_identity_emphasis(view), identity.x, identity.y, identity.width, identity.height });
            const IdleDrip* drip = level.idle_drip(view);
            values.push_back(drip != nullptr ? 1.0f : 0.0f);
            if (drip != nullptr)
            {
                const RectangleF bead = drip->bead_rectangle();
                values.insert(values.end(), { drip->attached() ? 1.0f : 0.0f,
                    drip->bead_age(), drip->interval_remaining(), bead.x, bead.y,
                    bead.width, bead.height, static_cast<float>(drip->drops().size()) });
                for (const auto& drop : drip->drops())
                {
                    values.insert(values.end(), { drop.position.x, drop.position.y, drop.age });
                }
            }
            const auto effects = player.movement_feedback().effects();
            values.push_back(static_cast<float>(effects.size()));
            for (const auto& effect : effects)
            {
                const Colour paint = effect.paint_colour.value_or(Colour());
                values.insert(values.end(), { static_cast<float>(effect.kind),
                    effect.position.x, effect.position.y, effect.age, effect.strength,
                    effect.direction, effect.paint_colour.has_value() ? 1.0f : 0.0f,
                    paint.r, paint.g, paint.b, paint.a });
            }
        }
        const auto blocked = blocked_observation(level.blocked_shot_feedback());
        values.insert(values.end(), blocked.begin(), blocked.end());
        return values;
    }

    void resize(Application& app, Level& level, const Vector2I& size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        level.refresh_layout();
    }

    // A contrasting canvas makes black outlines observable too. The player is
    // still the real builder-owned object, drawn with its actual pane/camera.
    struct Canvas : TextureObject
    {
        explicit Canvas(RenderResources* resources, const Colour& colour = Colour(0.72f, 0.75f, 0.78f)) :
            TextureObject("sprite_sheet_1", "pixel", resources, colour) {}
        using TextureObject::draw;
    };

    Frame isolated(const Player* player, const Player& camera_player,
        const Viewport& viewport, Renderer& renderer, const Canvas& canvas,
        const IdleDrip* drip = nullptr)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(camera_player.camera());
        list.set_filter(TextureFilter::point);
        canvas.draw(list, camera_player.camera().visible_rectangle(viewport));
        if (player != nullptr) { player->draw(list); }
        if (drip != nullptr) { drip->draw(list); }
        return read(renderer);
    }

    // Pin only the tool's pivot in pane pixels, at the real gameplay zoom.
    // This removes body animation, camera follow and travelled distance from
    // the comparison, so a changed raster must come from the tool itself.
    Frame isolated_tool(const Player& player, const Viewport& viewport,
        Renderer& renderer, const Canvas& canvas, Presentation& presentation)
    {
        // A paid-shot pulse must not stand in for a changed mechanism pose.
        auto& look = const_cast<WeaponFeedbackPresentation&>(presentation.weapon_feedback());
        const RestoreFlag restore{ look.enabled, look.enabled };
        look.enabled = false;
        const Weapon& tool = player.primary_weapon();
        const float scale = player.camera().scale;
        const RectangleI pane = viewport.pixel_rect();
        const Camera camera(tool.draw_pos() - Vector2F(
            static_cast<float>(pane.width) / (2 * scale),
            static_cast<float>(pane.height) / (2 * scale)), scale);
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(camera);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, camera.visible_rectangle(viewport));
        tool.draw(list);
        return read(renderer);
    }

    Frame isolated_marker(const Player& player, const Viewport& viewport,
        Renderer& renderer, const Canvas& canvas, bool visible)
    {
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(viewport);
        list.set_camera(player.camera());
        list.set_filter(TextureFilter::point);
        canvas.draw(list, player.camera().visible_rectangle(viewport));
        if (visible) { player.primary_weapon().draw_aim_marker(list); }
        return read(renderer);
    }

    // This checks raster output, rather than reproducing the arms/sleeves
    // geometry in the test. Keep one pixel for rasterized partial edges.
    size_t check_projected_bounds(const Frame& image, const Frame& blank,
        RectangleF bounds, const Viewport& viewport, Frame& crop)
    {
        const RectangleI pane = viewport.pixel_rect();
        bounds.x += static_cast<float>(pane.x) - 1;
        bounds.y += static_cast<float>(pane.y) - 1;
        bounds.width += 2;
        bounds.height += 2;
        int left = pane.right(), top = pane.bottom(), right = pane.x, bottom = pane.y;
        size_t changed = 0;
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                    static_cast<size_t>(x)) * 4;
                if (image.pixels[offset] == blank.pixels[offset] &&
                    image.pixels[offset + 1] == blank.pixels[offset + 1] &&
                    image.pixels[offset + 2] == blank.pixels[offset + 2]) { continue; }
                require(static_cast<float>(x) >= bounds.x && static_cast<float>(x) <= bounds.right() &&
                    static_cast<float>(y) >= bounds.y && static_cast<float>(y) <= bounds.bottom(),
                    "rendered painter/tool pixels escaped projected Scene culling bounds");
                ++changed;
                left = std::min(left, x); top = std::min(top, y);
                right = std::max(right, x + 1); bottom = std::max(bottom, y + 1);
            }
        }
        if (changed == 0) { return 0; }
        left = std::max(pane.x, left - 8); top = std::max(pane.y, top - 8);
        right = std::min(pane.right(), right + 8); bottom = std::min(pane.bottom(), bottom + 8);
        crop = { right - left, bottom - top, {} };
        for (int y = top; y < bottom; ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                static_cast<size_t>(left)) * 4;
            crop.pixels.insert(crop.pixels.end(), image.pixels.begin() + start,
                image.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
        }
        return changed;
    }

    size_t check_bounds(const Frame& image, const Frame& blank, const Player& player,
        const Viewport& viewport, Frame& crop)
    {
        return check_projected_bounds(image, blank, player.camera().calculate_view_rectangle(
            player.cull_bounds(1.0f / player.camera().scale)), viewport, crop);
    }

    void capture(Application& app, Level& level, const Canvas& canvas,
        const std::filesystem::path& output, const std::string& stem,
        std::ofstream& records, bool expect_visible = true)
    {
        const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
        const Frame first = render(level, *app.renderer());
        for (int repeat = 0; repeat < 3; ++repeat)
        {
            require(first.pixels == render(level, *app.renderer()).pixels,
                "frozen redraw changed painter or another visible state");
        }
        save(first, output / (stem + ".ppm"));
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            const Frame blank = isolated(nullptr, player, viewport, *app.renderer(), canvas);
            const Frame image = isolated(&player, player, viewport, *app.renderer(), canvas);
            Frame crop{};
            const size_t changed = check_bounds(image, blank, player, viewport, crop);
            require(expect_visible ? changed > 0 : changed == 0,
                "painter visibility did not match death/redraw phase");
            if (changed > 0) { save(crop, output / (stem + "-player" + std::to_string(view + 1) + ".ppm")); }
            records << stem << ',' << view + 1 << ',' << player.player_num() << ',' << changed << ','
                << player.facing_right() << ',' << player.primary_weapon().rotation() << ','
                << player.respawn_progress() << ',' << static_cast<int>(player.animation_state()) << ','
                << player.action_age() << ',' << player.action_frame() << ','
                << player.primary_weapon().visual_pose() << ','
                << player.primary_weapon().motion_state().pressure << ','
                << player.primary_weapon().motion_state().valve << ','
                << player.primary_weapon().motion_state().travel_phase << ','
                << player.primary_weapon().motion_state().mist_tail_remaining << ','
                << player.primary_weapon().feedback()->state().muzzle_remaining << ','
                << player.primary_weapon().feedback()->state().empty_remaining << ','
                << player.primary_weapon().feedback()->state().recovered_remaining << ','
                << player.combat_feedback().hurt_remaining() << ','
                << player.combat_feedback().elimination_remaining() << ','
                << player.combat_feedback().death_rectangle().x << ','
                << player.combat_feedback().death_rectangle().y << ','
                << player.combat_feedback().death_rectangle().width << ','
                << player.combat_feedback().death_rectangle().height << ','
                << player.quiet_character().state().breath << ','
                << player.quiet_character().state().lean << ','
                << player.quiet_character().state().blinking << '\n';
        }
        require(before == observation(level), "drawing changed public simulation state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
            "drawing changed the replay fingerprint");
#endif
        std::cout << "PASS " << stem << " frozen redraw, fingerprint and raster bounds\n";
    }

    std::unique_ptr<Level> build(Application& app, LevelBuilder& builder,
        int count, TeamColourPair palette, bool roller = false, bool same_team = false,
        WeaponType tool = WeaponType::none, OpeningMode opening = OpeningMode::off)
    {
        MenuLevelSettings settings;
        settings.set_player_count(count);
        settings.set_stage("movement_course");
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
        constexpr int slots[] = { 3, 1, 2, 0 };
        constexpr WeaponType tools[] = { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket };
        settings.begin_input_claims(slots[0]);
        for (int view = 1; view < count; ++view)
        {
            require(settings.claim_input_slot(slots[view]), "input claim failed");
        }
        for (int view = 0; view < count; ++view)
        {
            settings.set_player_team(view, same_team || view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(view, tool != WeaponType::none ? tool :
                (roller ? WeaponType::roller : tools[view]));
        }
        app.viewport_manager()->set_layout(settings.screen_layout());
        auto level = builder.build_level(settings, palette, opening);
        require(level->player_count() == count, "builder changed the roster count");
        for (int view = 0; view < count; ++view)
        {
            require(level->player(view).player_num() == slots[view], "builder changed the sparse input roster");
        }
        return level;
    }

    std::vector<PlayerInputData> neutral_inputs()
    {
        std::vector<PlayerInputData> inputs(input_slots::count);
        for (auto& input : inputs)
        {
            input.connected = true;
            input.shoot_direction = Vector2F(1, 0);
            input.shoot_direction_requested = true;
        }
        return inputs;
    }

    void settle(Level& level, const std::vector<PlayerInputData>& inputs)
    {
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { level.update(inputs, dt); }
        require(level.state() == LevelState::active, "countdown did not reach active play");
        for (int tick = 0; tick < 60; ++tick) { level.update(inputs, dt); }
        for (int view = 0; view < level.player_count(); ++view)
        {
            require(level.player(view).state() == PlayerState::alive, "authored spawn did not settle alive");
        }
    }

    bool same_pixel(const Frame& first, const Frame& second, size_t offset)
    {
        return std::equal(first.pixels.begin() + offset, first.pixels.begin() + offset + 4,
            second.pixels.begin() + offset);
    }

    size_t pane_delta(const Frame& first, const Frame& second, const RectangleI& pane)
    {
        size_t changed = 0;
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(first.width) +
                    static_cast<size_t>(x)) * 4;
                if (!same_pixel(first, second, offset)) { ++changed; }
            }
        }
        return changed;
    }
