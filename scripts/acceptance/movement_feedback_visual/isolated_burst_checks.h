// Included inside movement_feedback_visual.cpp's private namespace, after grounded(); not self-contained.

    struct FeedbackCanvas : TextureObject
    {
        explicit FeedbackCanvas(RenderResources* resources) :
            TextureObject("sprite_sheet_1", "pixel", resources, Colour(115, 122, 133)) {}
        using TextureObject::draw;
    };

    using BurstMatrix = std::array<std::unique_ptr<MovementFeedback>, 4>;

    Frame render_bursts(Application& app, const FeedbackCanvas& canvas,
        const BurstMatrix& effects, const std::array<Camera, 4>& cameras, bool draw_effects = true)
    {
        Renderer& renderer = *app.renderer();
        renderer.begin_frame();
        renderer.set_view_count(4);
        for (int view = 0; view < 4; ++view)
        {
            const auto index = static_cast<size_t>(view);
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            DrawList list = renderer.view(view);
            list.set_viewport(viewport);
            list.set_camera(cameras[index]);
            list.set_filter(TextureFilter::point);
            canvas.draw(list, cameras[index].visible_rectangle(viewport));
            if (draw_effects) { effects[index]->draw(list); }
        }
        renderer.submit();
        const Vector2F size = renderer.back_buffer_size();
        Frame frame{ static_cast<int>(size.x), static_cast<int>(size.y), {} };
        renderer.read_back_buffer(frame.pixels);
        require(frame.pixels.size() == static_cast<size_t>(frame.width) *
            static_cast<size_t>(frame.height) * 4, "burst readback size mismatch");
        return frame;
    }

    std::vector<float> burst_state(const BurstMatrix& effects)
    {
        std::vector<float> result;
        for (const auto& feedback : effects)
        {
            result.push_back(static_cast<float>(feedback->effects().size()));
            for (const auto& effect : feedback->effects())
            {
                result.insert(result.end(), { static_cast<float>(effect.kind),
                    effect.position.x, effect.position.y, effect.age, effect.strength,
                    effect.direction, effect.paint_colour.has_value() ? 1.0f : 0.0f });
            }
        }
        return result;
    }

    void check_burst_pixels(const Frame& enabled, const Frame& disabled,
        const MovementFeedback& effect, const Camera& camera, const Viewport& viewport,
        const MovementFeedbackPresentation& look, bool birth, bool strict_bounds,
        std::ofstream& records, const std::string& stem, int view)
    {
        const RectangleI pane = viewport.pixel_rect();
        // A tiny actor at the contact point prevents unrelated player geometry
        // from concealing a feedback-bounds regression.
        RectangleF bounds = camera.calculate_view_rectangle(
            effect.bounds(RectangleF(-0.5f, -0.5f, 1, 1)));
        bounds.x += static_cast<float>(pane.x);
        bounds.y += static_cast<float>(pane.y);
        // Round the enclosing edges for rasterization; the tiny inspection
        // fixture adds no pixel slack to conceal a sub-world-unit escape.
        const float slack = strict_bounds ? 0.0f : 1.0f;
        const float right = std::ceil(bounds.right()) + slack;
        const float bottom = std::ceil(bounds.bottom()) + slack;
        bounds.x = std::floor(bounds.x) - slack;
        bounds.y = std::floor(bounds.y) - slack;
        bounds.width = right - bounds.x;
        bounds.height = bottom - bounds.y;
        require(effect.effects().size() == 1, "isolated matrix did not contain exactly one burst");
        const auto& burst = effect.effects().front();
        const float duration = burst.kind == MovementFeedbackKind::takeoff ?
            look.takeoff.duration : look.landing.duration;
        const float fade = 1.0f - burst.age / duration;
        size_t changed = 0;
        size_t neutral_fill = 0;
        size_t black_keyline = 0;
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) *
                    static_cast<size_t>(enabled.width) + static_cast<size_t>(x)) * 4;
                if (std::equal(enabled.pixels.begin() + offset,
                    enabled.pixels.begin() + offset + 3, disabled.pixels.begin() + offset))
                {
                    continue;
                }
                ++changed;
                require(static_cast<float>(x) >= bounds.x &&
                    static_cast<float>(x) <= bounds.right() &&
                    static_cast<float>(y) >= bounds.y &&
                    static_cast<float>(y) <= bounds.bottom(),
                    "isolated movement puff escaped its culling bounds");
                const int low = std::min({ enabled.pixels[offset],
                    enabled.pixels[offset + 1], enabled.pixels[offset + 2] });
                const int high = std::max({ enabled.pixels[offset],
                    enabled.pixels[offset + 1], enabled.pixels[offset + 2] });
                if (high <= 8) { ++black_keyline; }
                if (low >= 150 && high <= 240 && high - low <= 12) { ++neutral_fill; }
                if (!birth)
                {
                    // Each covered pixel has one material blended once over
                    // the canvas. Overlapping translucent lobes or a fill over
                    // a translucent full border create extra, unintended shades.
                    bool single_blend = false;
                    for (const Colour& colour : { look.keyline, look.colour, look.highlight })
                    {
                        const std::array<float, 3> channels{ colour.r, colour.g, colour.b };
                        bool match = true;
                        for (size_t channel = 0; channel < channels.size(); ++channel)
                        {
                            const float expected = channels[channel] * fade * 255.0f +
                                static_cast<float>(disabled.pixels[offset + channel]) *
                                    (1.0f - colour.a * fade);
                            match &= std::abs(static_cast<float>(enabled.pixels[offset + channel]) -
                                expected) <= 2.0f;
                        }
                        single_blend |= match;
                    }
                    require(single_blend, "fading movement puff contains an overlapping compositing seam");
                }
            }
        }
        require(changed > 0, "isolated movement puff is invisible at gameplay scale");
        if (birth)
        {
            require(neutral_fill > 0, "movement puff lost its neutral grey fill");
            require(black_keyline > 0, "movement puff lost its black keyline");
        }
        records << stem << ',' << view + 1 << ',' << camera.scale << ',' << changed
            << ',' << neutral_fill << ',' << black_keyline << '\n';
    }

    // Keep the existing physical jump routes as the production-path proof. This
    // matrix isolates the cosmetic art, including the minimum accepted impact
    // which a full-height course jump does not exercise. Pane four deliberately
    // gives landing the takeoff's numeric style: its silhouette must still differ.
    void isolated_bursts(Application& app, Level& level, const Presentation& presentation,
        const std::filesystem::path& output)
    {
        FeedbackCanvas canvas(app.render_resources());
        std::ofstream records(output / "isolated-bursts.csv");
        records << "capture,view,camera_scale,changed_pixels,neutral_fill_pixels,black_keyline_pixels\n";
        for (const int fixture : { 0, 1, 2 })
        {
            const bool wide_keyline = fixture == 1;
            const bool tiny = fixture == 2;
            for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
                Vector2I(2560, 1440) })
            {
                if (fixture != 0 && size != Vector2I(1280, 720)) { continue; }
                resize(app, level, size);
                MovementFeedbackPresentation look = presentation.movement_feedback();
                if (wide_keyline)
                {
                    look.keyline_width = look.takeoff.particle_size * 3.0f;
                    // Valid authored data must leave a contact-fleck interior.
                    // These unrelated styles do not draw in this burst matrix.
                    look.step.particle_size = look.skid.particle_size = look.keyline_width * 3.0f;
                }
                if (tiny)
                {
                    look.pixel_size = 2.0f;
                    look.takeoff.particle_size = look.landing.particle_size = 3.0f;
                    look.takeoff.spread = look.landing.spread = 0.01f;
                    look.takeoff.lift = look.landing.lift = 0.01f;
                    look.keyline_width = 1.0f;
                }
                MovementFeedbackPresentation same_style = look;
                same_style.landing = look.takeoff;
                std::array<Camera, 4> cameras;
                for (int view = 0; view < 4; ++view)
                {
                    const auto index = static_cast<size_t>(view);
                    const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                    // The production matrix keeps actual gameplay scales. The
                    // tiny valid-style inspection magnifies snapping/bounds
                    // errors that one native pixel of slack could conceal.
                    const float scale = tiny ? 16.0f : level.players_[index]->camera().scale;
                    cameras[index] = Camera(-viewport.size() / (2 * scale), scale);
                }
                for (const float progress : { 0.0f, 0.5f, 0.9f })
                {
                    BurstMatrix effects;
                    for (size_t index = 0; index < effects.size(); ++index)
                    {
                        effects[index] = std::make_unique<MovementFeedback>(
                            index == 3 ? &same_style : &look, app.render_resources());
                    }
                    effects[0]->takeoff(Vector2F::ZERO);
                    effects[1]->land(Vector2F::ZERO, look.landing_min_speed);
                    effects[2]->land(Vector2F::ZERO, look.full_impact_speed);
                    effects[3]->land(Vector2F::ZERO, same_style.full_impact_speed);
                    require(effects[1]->effects().size() == 1 &&
                        effects[1]->effects()[0].strength == look.minimum_strength &&
                        effects[2]->effects().size() == 1 &&
                        effects[2]->effects()[0].strength == 1.0f,
                        "isolated landing matrix did not cover minimum and full impact strength");
                    for (size_t index = 0; index < effects.size(); ++index)
                    {
                        const float duration = index == 0 || index == 3 ?
                            look.takeoff.duration : look.landing.duration;
                        effects[index]->update(duration * progress);
                    }
                    const auto before = burst_state(effects);
                    const Frame enabled = render_bursts(app, canvas, effects, cameras);
                    require(enabled.pixels == render_bursts(app, canvas, effects, cameras).pixels,
                        "isolated movement puff changed during a frozen redraw");
                    look.enabled = same_style.enabled = false;
                    const Frame disabled = render_bursts(app, canvas, effects, cameras);
                    require(disabled.pixels == render_bursts(app, canvas, effects, cameras, false).pixels,
                        "disabled movement feedback still drew cosmetic pixels");
                    look.enabled = same_style.enabled = true;
                    require(enabled.pixels == render_bursts(app, canvas, effects, cameras).pixels &&
                        before == burst_state(effects),
                        "movement puff redraw or enabled toggle changed cosmetic state");
                    const std::string stem = std::string("isolated-bursts-") +
                        (wide_keyline ? "wide-keyline-" : tiny ? "tiny-inspection16x-" : "") +
                        std::to_string(size.x) + "x" +
                        std::to_string(size.y) + "-" + std::to_string(static_cast<int>(progress * 100));
                    save(enabled, output / (stem + ".ppm"));
                    for (int view = 0; view < 4; ++view)
                    {
                        check_burst_pixels(enabled, disabled, *effects[static_cast<size_t>(view)],
                            cameras[static_cast<size_t>(view)],
                            app.viewport_manager()->player_viewport(view),
                            view == 3 ? same_style : look,
                            progress == 0 && fixture == 0, tiny, records, stem, view);
                    }
                    // Equal settings and age isolate shape rather than duration,
                    // size, colour or fade. Compare corresponding pane pixels.
                    const RectangleI first = app.viewport_manager()->player_viewport(0).pixel_rect();
                    const RectangleI last = app.viewport_manager()->player_viewport(3).pixel_rect();
                    require(first.size() == last.size(), "isolated burst panes have unequal sizes");
                    bool different = false;
                    for (int y = 0; y < first.height && !different; ++y)
                    {
                        for (int x = 0; x < first.width; ++x)
                        {
                            const size_t a = (static_cast<size_t>(first.y + y) *
                                static_cast<size_t>(enabled.width) + static_cast<size_t>(first.x + x)) * 4;
                            const size_t b = (static_cast<size_t>(last.y + y) *
                                static_cast<size_t>(enabled.width) + static_cast<size_t>(last.x + x)) * 4;
                            if (!std::equal(enabled.pixels.begin() + a, enabled.pixels.begin() + a + 3,
                                enabled.pixels.begin() + b)) { different = true; break; }
                        }
                    }
                    require(different, "takeoff and landing lost their distinct puff silhouettes");
                }
            }
        }
        resize(app, level, { 1280, 720 });
        require(records.good(), "isolated burst record write failed");
        std::cout << "PASS isolated neutral keylined takeoff/minimum/full landing puffs; "
            "4P gameplay scales at 720p/1080p/QHD; distinct silhouettes with equal styles; "
            "birth/mid/late, single-blend fade colours, disabled/frozen redraw, "
            "wide-keyline bounds and tiny valid-style bounds under 16x inspection\n";
    }
