// Included inside painter_visual.cpp's private namespace, before every other
// painter_visual/ header: the shared probe fixtures and raster helpers.

    using acceptance::require;

    const float dt = simulation::seconds_per_step;

    struct Frame
    {
        int width;
        int height;
        std::vector<unsigned char> pixels;
    };

    // The layout the game gives this many human panes: three players take
    // three_player, which numbers its quadrants differently from four_player.
    ScreenLayout players_layout(int count)
    {
        switch (count)
        {
        case 1: return ScreenLayout::one_player;
        case 2: return ScreenLayout::two_player;
        case 3: return ScreenLayout::three_player;
        default: return ScreenLayout::four_player;
        }
    }

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
        MatchSetup settings;
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
        auto level = builder.build_level(settings, palette, opening, MatchRandom());
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
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { level.update(acceptance::slot_seats(level, inputs), dt); }
        require(level.state() == LevelState::active, "countdown did not reach active play");
        for (int tick = 0; tick < 60; ++tick) { level.update(acceptance::slot_seats(level, inputs), dt); }
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

    // The pixels a rectangle touches: its edges floored and ceiled outward.
    RectangleI pixel_bounds(const RectangleF& area)
    {
        const int left = static_cast<int>(std::floor(area.x));
        const int top = static_cast<int>(std::floor(area.y));
        return RectangleI(left, top, static_cast<int>(std::ceil(area.right())) - left,
            static_cast<int>(std::ceil(area.bottom())) - top);
    }

    size_t pixel_offset(const Frame& frame, int x, int y)
    {
        return (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
            static_cast<size_t>(x)) * 4;
    }

    // Relative luminance of the sRGB pixel at `offset`, 0 for black to 1 for white.
    float srgb_luminance(const Frame& frame, size_t offset)
    {
        const auto linear = [](unsigned char value)
        {
            const float channel = static_cast<float>(value) / 255.0f;
            return channel <= 0.04045f ? channel / 12.92f :
                std::pow((channel + 0.055f) / 1.055f, 2.4f);
        };
        return 0.2126f * linear(frame.pixels[offset]) +
            0.7152f * linear(frame.pixels[offset + 1]) +
            0.0722f * linear(frame.pixels[offset + 2]);
    }

    // The root a menu probe pushes its real pages above: the probe Canvas
    // across the whole window, so a page that does not cover it shows it.
    class CanvasProbeRoot final : public State
    {
    public:
        CanvasProbeRoot(Application& app, const Canvas& canvas) : app_(app), canvas_(canvas) {}
        void init() override {}
        void update(float) override {}
        void draw(Renderer& renderer) const override
        {
            renderer.set_view_count(1);
            DrawList list = renderer.view(0);
            list.set_viewport(this->app_.viewport_manager()->fullscreen_viewport());
            list.set_camera(Camera::default_camera);
            list.set_filter(TextureFilter::point);
            this->canvas_.draw(list, RectangleF(Vector2F::zero,
                this->app_.resolution_manager()->resolution_vec()));
        }
    private:
        Application& app_;
        const Canvas& canvas_;
    };

    // The whole state stack as the game draws it, read back.
    Frame stack_frame(Application& app)
    {
        app.renderer()->begin_frame();
        static_cast<StateContext&>(app).draw(*app.renderer());
        return read(*app.renderer());
    }

    // A real spray shot with the given tuning's spray values and strip, for a
    // probe that lands one contact by hand instead of firing it.
    ProjectileSpray controlled_spray(Application& app, const GameTuning& tuning, const RectangleF& area,
        PlayerTeam team, int owner, const Colour& colour)
    {
        const ProjectileTuning& values = tuning.projectile(ProjectileType::spray);
        return ProjectileSpray(area, Vector2F::zero, team, owner, colour, app.render_resources(),
            resolve_projectile_strip(values, *app.render_resources()), &values);
    }

    // A copy of a loaded font for a retained-copy probe: every printable glyph
    // from ' ' to '}', so '~' is absent and an old copy that uses it cannot be
    // drawn with it, and the glyphs `shifted` picks set 1.25 px further right
    // with 0.75 px more advance, so a page that kept the old font shows.
    FontHandle add_probe_font(RenderResources& resources, const std::string& source_name,
        const std::string& name, const std::function<bool(char32_t)>& shifted = {})
    {
        const Font* source = resources.font(resources.resolve_sprite_font(source_name));
        std::vector<Glyph> glyphs;
        for (char32_t character = U' '; character < U'~'; ++character)
        {
            const Glyph* found = source->find(character);
            require(found != nullptr, "Probe font " + name + " lacks a printable glyph of " + source_name);
            Glyph glyph = *found;
            if (shifted && shifted(character)) { glyph.x_offset += 1.25f; glyph.x_advance += 0.75f; }
            glyphs.push_back(glyph);
        }
        resources.add_font(name, std::make_unique<Font>(source->atlas(), std::move(glyphs), source->line_spacing()));
        const FontHandle handle = resources.resolve_sprite_font(name);
        require(!resources.can_render(handle, L"~"), "Probe font " + name + " still draws '~'");
        return handle;
    }

    bool is_digit(char32_t character) { return character >= U'0' && character <= U'9'; }
    bool is_brace(char32_t character) { return character == U'{' || character == U'}'; }

    // A probe's memory-only play preferences. The Save holds the edit before a
    // session is built over it, so the session's later refreshes keep it.
    void seed_play_settings(Save& save, const std::function<void(PlaySettings&)>& edit)
    {
        PlaySettings play = save.saved_play_settings();
        edit(play);
        save.set_play_settings(play);
    }

    void quiet_play(PlaySettings& play) { play.music_volume = play.sfx_volume = 0; }

    // The run's effective play settings, changed in memory as no menu does,
    // for a leaf that flips one (Reduce effects, a volume) under an open page
    // or match. Seed starting values with seed_play_settings instead.
    PlaySettings& live_play_settings(SettingsSession& session)
    {
        return const_cast<PlaySettings&>(session.play_settings());
    }

    // Style fingerprints: every field of a look as numbers, so a probe can
    // show that drawing or a command left the rest of it alone. A probe that
    // edits some fields masks them on a copy first. field_count pins each
    // struct's shape, so a new field fails the build here until it is added.
    struct AnyField
    {
        template<class T> operator T() const;
    };

    template<class T, class... Fields>
    consteval size_t field_count()
    {
        if constexpr (requires { T{ Fields{}..., AnyField{} }; }) { return field_count<T, Fields..., AnyField>(); }
        else { return sizeof...(Fields); }
    }

    void add_style(std::vector<float>& values, float value) { values.push_back(value); }
    void add_style(std::vector<float>& values, int value) { values.push_back(static_cast<float>(value)); }
    void add_style(std::vector<float>& values, bool value) { values.push_back(value ? 1.0f : 0.0f); }
    void add_style(std::vector<float>& values, const Vector2F& value) { values.insert(values.end(), { value.x, value.y }); }
    void add_style(std::vector<float>& values, const RectangleF& value)
    { values.insert(values.end(), { value.x, value.y, value.width, value.height }); }
    void add_style(std::vector<float>& values, const Colour& value)
    { values.insert(values.end(), { value.r, value.g, value.b, value.a }); }
    template<class Text>
        requires std::same_as<Text, std::string> || std::same_as<Text, std::wstring>
    void add_style(std::vector<float>& values, const Text& value)
    {
        values.push_back(static_cast<float>(value.size()));
        for (const auto character : value) { values.push_back(static_cast<float>(character)); }
    }
    template<class Item>
    void add_style(std::vector<float>& values, const std::vector<Item>& items)
    {
        values.push_back(static_cast<float>(items.size()));
        for (const Item& item : items) { add_style(values, item); }
    }
    template<class... Fields>
    void add_styles(std::vector<float>& values, const Fields&... fields) { (add_style(values, fields), ...); }

    std::vector<float> opening_style(const OpeningPresentation& p)
    {
        static_assert(field_count<OpeningPresentation>() == 34, "opening_style: add the new OpeningPresentation field");
        static_assert(field_count<MonitorArtPresentation>() == 10, "opening_style: add the new MonitorArtPresentation field");
        static_assert(field_count<SilhouetteArtPresentation>() == 5, "opening_style: add the new SilhouetteArtPresentation field");
        static_assert(field_count<CursorArtPresentation>() == 7, "opening_style: add the new CursorArtPresentation field");
        static_assert(field_count<RevealArtPresentation>() == 8, "opening_style: add the new RevealArtPresentation field");
        std::vector<float> v;
        add_styles(v, p.sprite_sheet, p.fill_frame, p.bezel_shades, p.bezel_shadow, p.glass, p.glass_colour,
            p.program_colour, p.desktop_colour, p.palette_height, p.palette_sheet, p.palette_frames, p.slot_outline,
            p.sticky_notes, p.stickers, p.scribble, p.coffee, p.crack, p.silhouette, p.adult_height, p.child_height,
            p.cursor_size, p.preview_strength, p.figure_grey, p.percent_font, p.percent_height, p.face, p.sweat,
            p.hop, p.shake, p.slump);
        const auto& m = p.monitor_art;
        add_styles(v, m.ink, m.highlight, m.power_light, m.pen_width, m.corner_step, m.shadow_width, m.vent_area,
            m.vent_count, m.power_button, m.power_indicator);
        const auto& s = p.silhouette_art;
        add_styles(v, s.ink, s.rim, s.shade, s.pen_width, s.shade_width);
        const auto& c = p.cursor_art;
        add_styles(v, c.ink, c.fill, c.shade, c.shadow, c.shadow_offset, c.click_duration, c.click_reach);
        const auto& r = p.reveal_art;
        add_styles(v, r.ink, r.paper, r.shade, r.keyline_width, r.padding, r.shadow_offset, r.tick_length, r.beat_duration);
        return v;
    }

    std::vector<float> hud_style(const HudPresentation& p)
    {
        static_assert(field_count<HudPresentation>() == 32, "hud_style: add the new HudPresentation field");
        static_assert(field_count<HudPaperPresentation>() == 5, "hud_style: add the new HudPaperPresentation field");
        static_assert(field_count<ClosingClockPresentation>() == 9, "hud_style: add the new ClosingClockPresentation field");
        static_assert(field_count<HudLowPaintPresentation>() == 4, "hud_style: add the new HudLowPaintPresentation field");
        static_assert(field_count<HudSubMarkPresentation>() == 2, "hud_style: add the new HudSubMarkPresentation field");
        static_assert(field_count<CursorMarkerPresentation>() == 6, "hud_style: add the new CursorMarkerPresentation field");
        static_assert(field_count<TeammateMarkerPresentation>() == 9, "hud_style: add the new TeammateMarkerPresentation field");
        static_assert(field_count<HudLabels>() == 4, "hud_style: add the new HudLabels field");
        std::vector<float> v;
        add_styles(v, p.meter_label_font, p.meter_track_colour, p.meter_label_gap, p.reference_height, p.right_margin,
            p.ammo_top_margin, p.health_top_margin, p.box_size, p.box_fill_border, p.health_colour, p.sprite_sheet,
            p.fill_frame, p.timer_font, p.timer_offset, p.timer_shadow_offset, p.timer_colour, p.timer_shadow_colour,
            p.timer_reference_width, p.player_identity_font, p.player_identity_offset, p.player_identity_shadow_offset,
            p.player_identity_colour, p.player_identity_shadow_colour, p.player_identity_padding,
            p.player_identity_reference_width);
        add_styles(v, p.paper.ink, p.paper.paper, p.paper.shade, p.paper.keyline_width, p.paper.shadow_offset);
        const auto& k = p.closing_clock;
        add_styles(v, k.ink, k.paper, k.shade, k.text, k.accent, k.keyline_width, k.shadow_offset, k.tick_reach, k.beat_duration);
        add_styles(v, p.low_paint.tag, p.low_paint.text, p.low_paint.padding, p.low_paint.blink_period);
        add_styles(v, p.sub_mark.size, p.sub_mark.gap);
        const auto& cursor = p.cursor_marker;
        add_styles(v, cursor.enabled, cursor.ink, cursor.paper, cursor.size, cursor.ink_width, cursor.paper_width);
        const auto& t = p.teammate_marker;
        add_styles(v, t.enabled, t.ink, t.size, t.keyline_width, t.badge_inset, t.pointer_length, t.pointer_width,
            t.margin, t.fade_in);
        add_styles(v, p.labels.team_a, p.labels.team_b, p.labels.health, p.labels.paint);
        return v;
    }

    std::vector<float> player_identity_style(const PlayerIdentityPresentation& p)
    {
        static_assert(field_count<PlayerIdentityPresentation>() == 12,
            "player_identity_style: add the new PlayerIdentityPresentation field");
        std::vector<float> v;
        add_styles(v, p.enabled, p.world_size, p.world_offset, p.bot_label_size, p.bot_label_gap, p.emphasis_duration,
            p.emphasis_scale, p.ui_size, p.ui_gap, p.background_colour, p.ink_colour, p.border);
        return v;
    }

    bool same_team_palette(const TeamPalettePresentation& a, const TeamPalettePresentation& b)
    {
        if (a.colours.size() != b.colours.size() || a.pairs.size() != b.pairs.size()) { return false; }
        for (size_t i = 0; i < a.colours.size(); ++i)
        {
            if (a.colours[i].name != b.colours[i].name || a.colours[i].colour != b.colours[i].colour) { return false; }
        }
        for (size_t i = 0; i < a.pairs.size(); ++i)
        {
            if (a.pairs[i].key != b.pairs[i].key || a.pairs[i].label != b.pairs[i].label ||
                a.pairs[i].colours.team_a != b.pairs[i].colours.team_a ||
                a.pairs[i].colours.team_b != b.pairs[i].colours.team_b ||
                a.pairs[i].in_random != b.pairs[i].in_random) { return false; }
        }
        return true;
    }

    std::array<std::string, 5> hud_resource_names(const HudPresentation& p)
    { return { p.meter_label_font, p.sprite_sheet, p.fill_frame, p.timer_font, p.player_identity_font }; }

#if COLOURWARS_DEV_TOOLS
    // Points TEMP and TMP at a probe folder while it lives, so the director's
    // candidate files stay in the probe's scratch.
    class TuneColourTemp
    {
    public:
        explicit TuneColourTemp(const std::filesystem::path& directory)
        {
            std::filesystem::create_directories(directory);
            for (size_t index = 0; index < 2; ++index)
            {
                wchar_t* previous = nullptr; size_t length = 0;
                require(_wdupenv_s(&previous, &length, this->names_[index]) == 0,
                    "cannot read the tune probe temporary directory");
                if (previous != nullptr) { this->previous_[index] = previous; std::free(previous); }
                require(_wputenv_s(this->names_[index], directory.c_str()) == 0,
                    "cannot isolate tune candidate temporary files");
            }
        }
        TuneColourTemp(const TuneColourTemp&) = delete;
        TuneColourTemp& operator=(const TuneColourTemp&) = delete;
        ~TuneColourTemp()
        {
            for (size_t index = 0; index < 2; ++index)
            { _wputenv_s(this->names_[index], this->previous_[index].c_str()); }
        }
    private:
        const std::array<const wchar_t*, 2> names_{ L"TEMP", L"TMP" };
        std::array<std::wstring, 2> previous_;
    };

    std::string tune_colour_bytes(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary); require(file.good(), "cannot read tune probe content");
        return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    }

    void tune_colour_write(const std::filesystem::path& path, const std::string& bytes)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        require(file.good(), "cannot write tune probe scratch content");
    }

    // The two content files a tune command can edit, in real_bytes() order.
    constexpr std::array<const char*, 2> tune_colour_files{ "tuning.json", "presentation.json" };

    // How a tune-colour leaf sets itself up. The scratch folder and the command
    // transcript are named under the leaf's output folder.
    struct TuneColourSetup
    {
        std::string scratch;
        std::string transcript;
        // Refuse a scratch folder an earlier run left behind.
        bool fresh = false;
        // Put the live tuning and presentation back when the leaf ends.
        bool restore_content = true;
        // Memory-only play preferences, held by the Save before the session.
        std::function<void(PlaySettings&)> memory_play;
        // Preferences the leaf writes to its private save, after the session.
        std::function<void(Save&, SettingsSession&)> seed;
        // The stages the director judges content against; every manifest stage by default.
        std::function<StageCatalog()> catalog;
    };

    // A tune-colour leaf's private scratch: copies of tuning.json and
    // presentation.json as the director's source and staged content, an
    // isolated save profile and TEMP, and the live tuning and presentation
    // restored when it ends (after anything a derived fixture holds).
    class TuneColourScratch
    {
    public:
        TuneColourScratch(GameTuning& tuning, Presentation& presentation, const std::filesystem::path& content,
            const std::filesystem::path& output, const TuneColourSetup& setup) :
            root_(output / setup.scratch), source_(root_ / "source"), staged_(root_ / "staged"),
            real_bytes_(copy_content(content, root_, source_, staged_, setup.fresh)),
            profile_(root_ / "profile"), temporary_(root_ / "temp"),
            tuning_(tuning), presentation_(presentation), original_tuning_(tuning),
            original_presentation_(presentation), restore_content_(setup.restore_content) {}
        TuneColourScratch(const TuneColourScratch&) = delete;
        TuneColourScratch& operator=(const TuneColourScratch&) = delete;
        ~TuneColourScratch()
        {
            if (this->restore_content_)
            {
                this->tuning_ = this->original_tuning_;
                this->presentation_ = this->original_presentation_;
            }
        }

        const std::filesystem::path& root() const { return this->root_; }
        const std::filesystem::path& source() const { return this->source_; }
        const std::filesystem::path& staged() const { return this->staged_; }
        // The real content files' bytes, as copied: tuning.json, then presentation.json.
        const std::array<std::string, 2>& real_bytes() const { return this->real_bytes_; }
        std::filesystem::path save_path() const { return acceptance::save_path_in(this->root_ / "profile"); }
        const GameTuning& original_tuning() const { return this->original_tuning_; }
        const Presentation& original_presentation() const { return this->original_presentation_; }

    private:
        static std::array<std::string, 2> copy_content(const std::filesystem::path& content,
            const std::filesystem::path& root, const std::filesystem::path& source,
            const std::filesystem::path& staged, bool fresh)
        {
            require(!fresh || !std::filesystem::exists(root), "Tune scratch " + root.string() + " must be fresh");
            std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
            std::array<std::string, 2> bytes;
            for (size_t index = 0; index < tune_colour_files.size(); ++index)
            {
                bytes[index] = tune_colour_bytes(content / tune_colour_files[index]);
                tune_colour_write(source / tune_colour_files[index], bytes[index]);
                tune_colour_write(staged / tune_colour_files[index], bytes[index]);
            }
            return bytes;
        }

        std::filesystem::path root_;
        std::filesystem::path source_;
        std::filesystem::path staged_;
        std::array<std::string, 2> real_bytes_;
        acceptance::IsolatedSave profile_;
        TuneColourTemp temporary_;
        GameTuning& tuning_;
        Presentation& presentation_;
        GameTuning original_tuning_;
        Presentation original_presentation_;
        bool restore_content_;
    };

    // The scratch with a real DebugDirector attached over it: a private Save
    // and session, a GameData holding them, the live tuning and presentation
    // and the stage catalog, the director reading the scratch content, and its
    // log retained as the leaf's command transcript.
    class TuneColourSession : public TuneColourScratch
    {
    public:
        TuneColourSession(Application& app, GameTuning& tuning, Presentation& presentation,
            const std::filesystem::path& content, const std::filesystem::path& output, const TuneColourSetup& setup) :
            TuneColourScratch(tuning, presentation, content, output, setup),
            settings_(seeded(this->preferences_, setup)),
            catalog_(setup.catalog ? setup.catalog() : acceptance::manifest_catalog(content)),
            director_(&tuning, &presentation)
        {
            if (setup.seed)
            {
                setup.seed(this->preferences_, this->settings_);
                this->seeded_save_ = tune_colour_bytes(this->save_path());
            }
            this->data_.set_application(&app); this->data_.set_save(&this->preferences_);
            this->data_.set_settings_session(&this->settings_); this->data_.set_stage_catalog(&this->catalog_);
            this->data_.set_tuning(&tuning); this->data_.set_presentation(&presentation);
            this->data_.set_debug(&this->director_);
            DebugDirectorImpl& impl = this->impl();
            impl.set_content_directories(this->source(), this->staged());
            impl.set_user_directory(this->root() / "user");
            impl.set_script_directories({ this->root() / "scripts" });
            this->director_.attach(&app, &this->data_);
            impl.log().set_mirror_to_debugger(false);
            require(impl.log().set_transcript((output / setup.transcript).string()),
                "Cannot retain the " + setup.transcript + " command transcript");
        }

        Save& preferences() { return this->preferences_; }
        SettingsSession& settings() { return this->settings_; }
        GameData& data() { return this->data_; }
        DebugDirector& director() { return this->director_; }
        DebugDirectorImpl& impl() { return *this->director_.impl(); }
        // The private save's bytes just after the seed wrote it.
        const std::string& seeded_save() const
        {
            require(this->seeded_save_.has_value(), "Tune leaf read a seeded save it never wrote");
            return *this->seeded_save_;
        }
        // Every preference a tune command must leave alone, display and keys included.
        std::vector<int> preference_values() const
        {
            const PlaySettings& play = this->settings_.play_settings();
            std::vector<int> values{ static_cast<int>(this->preferences_.resolution()),
                this->preferences_.fullscreen() ? 1 : 0, static_cast<int>(this->preferences_.team_palette()),
                play.aim_sensitivity, play.music_volume, play.sfx_volume, play.view_shake ? 1 : 0,
                play.aim_marker ? 1 : 0, play.reduce_effects ? 1 : 0,
                static_cast<int>(this->settings_.input_bindings().keyboard_jump) };
            for (const StickSettings& stick : play.sticks)
            { values.push_back(stick.movement_deadzone); values.push_back(stick.aim_deadzone); }
            return values;
        }

    private:
        static Save& seeded(Save& save, const TuneColourSetup& setup)
        {
            if (setup.memory_play) { seed_play_settings(save, setup.memory_play); }
            return save;
        }

        Save preferences_;
        SettingsSession settings_;
        StageCatalog catalog_;
        GameData data_;
        DebugDirector director_;
        std::optional<std::string> seeded_save_;
    };

    // The non-default preferences most tune-colour leaves write first, so a
    // command that touched them would show.
    PlaySettings tune_colour_play()
    {
        PlaySettings play; play.music_volume = 17; play.sfx_volume = 83; play.aim_sensitivity = 4;
        play.view_shake = false; play.aim_marker = true; play.reduce_effects = true;
        play.sticks[2].movement_deadzone = 25; play.sticks[2].aim_deadzone = 70;
        return play;
    }

    // tune_colour_play with a 1280x720 windowed Blue/Orange display.
    void seed_tune_colour_display(Save& save, SettingsSession& settings)
    {
        require(settings.set_play_settings_and_save(tune_colour_play()) && save.set_display_settings_and_save(
            ScreenResolution::s_1280_720, false, TeamColourPair::blue_orange), "Cannot seed the private tune-colour display preferences");
    }

    // tune_colour_play with Jump on J.
    void seed_tune_colour_jump(Save&, SettingsSession& settings)
    {
        InputBindings bindings; bindings.keyboard_jump = Key::j;
        require(settings.set_play_settings_and_save(tune_colour_play()) && settings.set_input_bindings_and_save(bindings),
            "Cannot seed the private tune-colour Jump preferences");
    }
#endif
