#include "game/pch.h"
#include "game/game_data.h"
#include "game/input_slots.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// Inspect the real Scene-owned roster without adding test-only game APIs. The
// match routes inject no state, position, velocity, contact or effect. A separate
// controlled matrix calls the public cosmetic producers to isolate their art.
#define private public
#include "game/objects/level_builder.h"
#undef private

using namespace labrador;
using namespace mattmath;

namespace
{
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

    Frame render(Level& level, Renderer& renderer)
    {
        renderer.begin_frame();
        level.draw(renderer);
        renderer.submit();
        const Vector2F size = renderer.back_buffer_size();
        Frame frame{ static_cast<int>(size.x), static_cast<int>(size.y), {} };
        renderer.read_back_buffer(frame.pixels);
        require(frame.pixels.size() ==
            static_cast<size_t>(frame.width) * static_cast<size_t>(frame.height) * 4,
            "readback size mismatch");
        return frame;
    }

    void save(const Frame& frame, const std::filesystem::path& path)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
        std::vector<unsigned char> rgb(frame.pixels.size() / 4 * 3);
        for (size_t source = 0, target = 0; source < frame.pixels.size(); source += 4)
        {
            for (size_t channel = 0; channel < 3; ++channel)
            {
                rgb[target++] = frame.pixels[source + channel];
            }
        }
        file.write(reinterpret_cast<const char*>(rgb.data()),
            static_cast<std::streamsize>(rgb.size()));
        require(file.good(), "capture write failed");
    }

    size_t differences(const Frame& enabled, const Frame& disabled,
        const Viewport& viewport, const Level& level, int view,
        size_t* readable_pixels = nullptr)
    {
        require(enabled.width == disabled.width && enabled.height == disabled.height,
            "comparison frame sizes differ");
        const RectangleI rectangle = viewport.pixel_rect();
        std::vector<RectangleF> effect_bounds;
        for (const Player* player : level.players_)
        {
            RectangleF projected = level.players_[view]->camera().calculate_view_rectangle(player->bounds());
            // Rasterization can cover the last partial pixel at an edge.
            projected.x += static_cast<float>(rectangle.x) - 1;
            projected.y += static_cast<float>(rectangle.y) - 1;
            projected.width += 2;
            projected.height += 2;
            effect_bounds.push_back(projected);
        }
        size_t changed = 0;
        if (readable_pixels != nullptr) { *readable_pixels = 0; }
        for (int y = rectangle.y; y < rectangle.bottom(); ++y)
        {
            for (int x = rectangle.x; x < rectangle.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) *
                    static_cast<size_t>(enabled.width) + static_cast<size_t>(x)) * 4;
                if (enabled.pixels[offset] != disabled.pixels[offset] ||
                    enabled.pixels[offset + 1] != disabled.pixels[offset + 1] ||
                    enabled.pixels[offset + 2] != disabled.pixels[offset + 2])
                {
                    ++changed;
                    // A one-level RGB change proves rendering, but not a
                    // useful movement cue at the smallest supported panes.
                    if (readable_pixels != nullptr)
                    {
                        int contrast = 0;
                        for (size_t channel = 0; channel < 3; ++channel)
                        {
                            contrast = std::max(contrast,
                                std::abs(static_cast<int>(enabled.pixels[offset + channel]) -
                                    static_cast<int>(disabled.pixels[offset + channel])));
                        }
                        if (contrast >= 32) { ++*readable_pixels; }
                    }
                    const Vector2F pixel(static_cast<float>(x), static_cast<float>(y));
                    require(std::any_of(effect_bounds.begin(), effect_bounds.end(),
                        [&](const RectangleF& bounds) {
                            return pixel.x >= bounds.x && pixel.x <= bounds.right() &&
                                pixel.y >= bounds.y && pixel.y <= bounds.bottom();
                        }),
                        "feedback pixels escaped the Scene culling bounds");
                }
            }
        }
        return changed;
    }

    std::vector<float> simulation_state(const Level& level)
    {
        std::vector<float> state{ level.timer_ };
        for (const Player* player : level.players_)
        {
            const RectangleF& rectangle = player->rectangle();
            state.insert(state.end(), { rectangle.x, rectangle.y, rectangle.width,
                rectangle.height, player->velocity().x, player->velocity().y });
            const auto effects = player->movement_feedback().effects();
            state.push_back(static_cast<float>(effects.size()));
            for (const auto& effect : effects)
            {
                state.insert(state.end(), { static_cast<float>(effect.kind),
                    effect.position.x, effect.position.y, effect.age, effect.strength });
            }
        }
        return state;
    }

    void resize(Application& app, Level& level, const Vector2I& size)
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        level.refresh_layout();
    }

    bool grounded(const Player& player)
    {
        return player.move_state() == PlayerMoveState::on_ground ||
            player.move_state() == PlayerMoveState::on_drop_down_ground ||
            player.move_state() == PlayerMoveState::on_ramp_left ||
            player.move_state() == PlayerMoveState::on_ramp_right;
    }

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

    // With `required_views`, only the listed views must show feedback; the
    // others still keep any difference inside the culling bounds and 1% cap.
    void capture_phase(Application& app, Level& level, Presentation& presentation,
        const std::filesystem::path& output, int count, const std::string& phase,
        int tick, std::ofstream& records, bool expect_difference = true,
        const std::vector<bool>& required_views = {})
    {
        auto& feedback = const_cast<MovementFeedbackPresentation&>(
            presentation.movement_feedback());
        const auto before = simulation_state(level);
        Vector2F reference_extent;
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080),
            Vector2I(2560, 1440) })
        {
            resize(app, level, size);
            const Vector2F extent = level.players_.front()->camera().visible_rectangle(
                app.viewport_manager()->player_viewport(0)).size();
            if (reference_extent == Vector2F::ZERO) { reference_extent = extent; }
            require((extent - reference_extent).length() < 0.01f,
                "world framing changed across resolutions");
            const std::string stem = std::to_string(count) + "p-" + phase + "-" +
                std::to_string(size.x) + "x" + std::to_string(size.y);

            // The same simulation snapshot is drawn twice. The presentation
            // switch only hides the existing effects; no intervening update.
            feedback.enabled = true;
            const Frame enabled = render(level, *app.renderer());
            feedback.enabled = false;
            const Frame disabled = render(level, *app.renderer());
            feedback.enabled = true;
            save(enabled, output / (stem + "-on.ppm"));
            save(disabled, output / (stem + "-off.ppm"));
            for (int view = 0; view < count; ++view)
            {
                size_t readable_pixels = 0;
                const size_t changed = differences(enabled, disabled,
                    app.viewport_manager()->player_viewport(view), level, view,
                    &readable_pixels);
                if (required_views.empty())
                {
                    require(expect_difference ? changed > 0 : changed == 0,
                        "movement feedback visibility did not match the expected phase");
                }
                else if (required_views.at(static_cast<size_t>(view)))
                {
                    require(changed > 0, "a route takeoff or landing showed no feedback in its own view");
                }
                const auto pane = app.viewport_manager()->player_viewport(view).pixel_rect();
                require(changed * 100 < static_cast<size_t>(pane.width) * static_cast<size_t>(pane.height),
                    "movement feedback changed more than one percent of a pane");
                if (count == 4 && size == Vector2I(1280, 720) &&
                    (phase == "takeoff" || phase == "takeoff-mid" ||
                        phase == "landing" || phase == "landing-mid"))
                {
                    // This settled course fixture gives every pane nearby
                    // takeoff/landing cues. Keep them visible into recovery;
                    // the upper bound above still protects the play area.
                    require(changed >= 128 && readable_pixels >= 64,
                        "four-player 720p contact cues are too small or faint");
                }
                records << count << ',' << phase << ',' << tick << ',' << tick * dt << ','
                    << size.x << ',' << size.y << ',' << view + 1 << ',' << changed
                    << ',' << readable_pixels << '\n';
            }
            std::cout << "PASS " << stem << " matched feedback on/off\n";
        }
        resize(app, level, { 1280, 720 });
        require(before == simulation_state(level),
            "on/off redraws or resize advanced movement, effects or the match clock");
    }

    void check_suspended_draws(Level& level, Renderer& renderer,
        const std::filesystem::path& output, int count)
    {
        level.suspend();
        const auto before = simulation_state(level);
        const Frame initial = render(level, renderer);
        for (int frame = 0; frame < 12; ++frame)
        {
            const Frame repeated = render(level, renderer);
            require(initial.pixels == repeated.pixels,
                "suspended redraw changed feedback or another visible state");
        }
        save(initial, output / (std::to_string(count) + "p-paused-landing.ppm"));
        require(before == simulation_state(level), "suspended drawing advanced simulation state");
        level.resume();
        std::cout << "PASS " << count << "p landing frozen across 12 suspended draws\n";
    }

    // Route targets are named authored objects, so a moved or renamed platform
    // fails here instead of silently walking the route off the course.
    RectangleF surface(const LevelLoadedInfo& info, const std::string& name)
    {
        const JsonValue objects = info.collision_objects_json();
        for (size_t index = 0; index < objects.size(); ++index)
        {
            const JsonValue object = objects.at(index);
            if (object.string("name") != name) { continue; }
            const JsonValue shape = object.object("rectangle");
            return RectangleF(shape.number("x"), shape.number("y"),
                shape.number("width"), shape.number("height"));
        }
        throw std::runtime_error("Movement Course object '" + name + "' is missing");
    }

    std::unique_ptr<Level> build_course(Application& app, LevelBuilder& builder,
        int count)
    {
        MenuLevelSettings settings;
        settings.set_player_count(count);
        settings.set_stage("movement_course");
        settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
        settings.begin_input_claims(0);
        for (int player = 1; player < count; ++player)
        {
            require(settings.claim_input_slot(player), "input claim failed");
        }
        for (int player = 0; player < count; ++player)
        {
            settings.set_player_team(player, player % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(player, player % 2 == 0 ? WeaponType::sprayer : WeaponType::roller);
        }
        app.viewport_manager()->set_layout(settings.screen_layout());
        return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
    }

    // One player on the moving route. Team A runs from rest and holds a full
    // jump over the 800-980 short gap, which carries the one-player camera past
    // its 1152 scroll border; Team B starts later, runs left along its runway
    // and jumps short of the return step, so all four panes pan.
    struct RouteRunner
    {
        float direction = 0;
        RectangleF landing;
        int jump_tick = -1;
        int takeoff_tick = -1;
        int landing_tick = -1;
        Vector2F camera_min;
        Vector2F camera_max;
    };

    constexpr int route_hold_ticks = 30;
    constexpr int route_team_b_delay = 30;
    constexpr int route_sequence_start = 45;
    constexpr int route_tick_limit = 400;
    constexpr float route_takeoff_margin = 15.0f;
    // Team A releases steering 170 past the far lip, still airborne; braking
    // stops it short of the long gap and past the one-player scroll border.
    constexpr float route_release_past_lip = 170.0f;
    // Team B presses this far in from its runway's left end, releasing at once.
    constexpr float route_team_b_jump_inset = 170.0f;
    constexpr float route_minimum_pan = 16.0f;

    void route_phase(Application& app, LevelBuilder& builder, Presentation& presentation,
        const LevelLoadedInfo& course, const std::filesystem::path& output, int count,
        int sequence_frames, bool save_sequence,
        std::ofstream& records, std::ofstream& route_records)
    {
        auto& feedback = const_cast<MovementFeedbackPresentation&>(
            presentation.movement_feedback());
        const RectangleF runway_a = surface(course, "spawn_a_runway");
        const RectangleF short_landing = surface(course, "short_landing_long_takeoff");
        const RectangleF runway_b = surface(course, "spawn_b_runway");

        auto level = build_course(app, builder, count);
        std::vector<PlayerInputData> inputs(input_slots::count);
        for (auto& input : inputs)
        {
            input.connected = true;
            input.shoot_direction = Vector2F(1, 0);
            input.shoot_direction_requested = true;
        }
        for (int tick = 0; tick < 600 && level->state() != LevelState::active; ++tick)
        {
            level->update(inputs, dt);
        }
        require(level->state() == LevelState::active, "route countdown did not reach active play");
        for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
        resize(app, *level, { 1280, 720 });

        std::vector<RouteRunner> runners;
        for (const Player* player : level->players_)
        {
            require(player->state() == PlayerState::alive && grounded(*player),
                "route spawn did not settle alive on support");
            RouteRunner runner;
            runner.direction = player->team() == PlayerTeam::a ? 1.0f : -1.0f;
            runner.landing = runner.direction > 0 ? short_landing : runway_b;
            runner.camera_min = runner.camera_max = player->camera().translation;
            runners.push_back(runner);
        }

        const std::string prefix = std::to_string(count) + "p-route";
        const std::filesystem::path on_directory = output / (prefix + "-60fps-on");
        const std::filesystem::path off_directory = output / (prefix + "-60fps-off");
        if (save_sequence && sequence_frames > 0)
        {
            std::filesystem::create_directories(on_directory);
            std::filesystem::create_directories(off_directory);
        }
        const int landing_expiry = static_cast<int>(std::ceil(
            presentation.movement_feedback().landing.duration / dt)) + 1;
        bool captured_takeoff = false;
        bool captured_landing = false;
        int last_landing = -1;
        int tick = 0;
        for (; tick < route_tick_limit; ++tick)
        {
            for (size_t index = 0; index < runners.size(); ++index)
            {
                RouteRunner& runner = runners[index];
                const Player& player = *level->players_[index];
                PlayerInputData& input = inputs.at(static_cast<size_t>(player.player_num()));
                // Sweep aim through a full turn every 90 ticks.
                input.shoot_angle = static_cast<float>(tick) * 2.0f * PI / 90.0f;
                if (index % 2 != 0) { input.shoot_angle = PI - input.shoot_angle; }
                input.shoot_angle = std::atan2(std::sin(input.shoot_angle), std::cos(input.shoot_angle));
                input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
                input.jump_pressed = false;
                input.jump_held = runner.jump_tick >= 0 && tick - runner.jump_tick < route_hold_ticks;
                input.x_movement = 0;
                if (runner.landing_tick >= 0) { continue; }
                if (runner.jump_tick < 0)
                {
                    if (runner.direction < 0 && tick < route_team_b_delay) { continue; }
                    input.x_movement = runner.direction;
                    const bool at_takeoff = runner.direction > 0 ?
                        player.rectangle().right() >= runway_a.right() - route_takeoff_margin :
                        player.center().x <= runway_b.left() + route_team_b_jump_inset;
                    if (at_takeoff && grounded(player))
                    {
                        runner.jump_tick = tick;
                        input.jump_pressed = true;
                        input.jump_held = true;
                    }
                }
                else if (runner.direction > 0 &&
                    player.center().x < short_landing.left() + route_release_past_lip)
                {
                    input.x_movement = runner.direction;
                }
            }
            level->update(inputs, dt);

            std::vector<bool> took_off(runners.size(), false);
            std::vector<bool> landed(runners.size(), false);
            for (size_t index = 0; index < runners.size(); ++index)
            {
                RouteRunner& runner = runners[index];
                const Player& player = *level->players_[index];
                require(player.state() == PlayerState::alive, "the route killed a player");
                if (runner.jump_tick >= 0 && runner.takeoff_tick < 0)
                {
                    require(!grounded(player) && player.velocity().y < 0,
                        "a route jump did not take off on its press");
                    runner.takeoff_tick = tick;
                    took_off[index] = true;
                }
                else if (runner.takeoff_tick >= 0 && runner.landing_tick < 0 && grounded(player))
                {
                    runner.landing_tick = tick;
                    last_landing = tick;
                    landed[index] = true;
                }
                if (runner.landing_tick >= 0)
                {
                    const RectangleF& body = player.rectangle();
                    require(grounded(player) &&
                        std::abs(body.bottom() - runner.landing.top()) <= 0.05f &&
                        body.left() >= runner.landing.left() && body.right() <= runner.landing.right(),
                        "a route landing is not supported on its authored platform");
                }
                const Vector2F translation = player.camera().translation;
                runner.camera_min = Vector2F(std::min(runner.camera_min.x, translation.x),
                    std::min(runner.camera_min.y, translation.y));
                runner.camera_max = Vector2F(std::max(runner.camera_max.x, translation.x),
                    std::max(runner.camera_max.y, translation.y));
            }

            // One update, two draws of the same state: only the switch differs.
            const auto before = simulation_state(*level);
            feedback.enabled = true;
            const Frame enabled = render(*level, *app.renderer());
            feedback.enabled = false;
            const Frame disabled = render(*level, *app.renderer());
            feedback.enabled = true;
            require(before == simulation_state(*level),
                "route on/off redraws advanced movement, effects or the match clock");
            for (int view = 0; view < count; ++view)
            {
                const size_t changed = differences(enabled, disabled,
                    app.viewport_manager()->player_viewport(view), *level, view);
                const auto pane = app.viewport_manager()->player_viewport(view).pixel_rect();
                require(changed * 100 < static_cast<size_t>(pane.width) * static_cast<size_t>(pane.height),
                    "route feedback changed more than one percent of a pane");
                const Player& player = *level->players_[static_cast<size_t>(view)];
                route_records << count << ',' << tick << ',' << view + 1
                    << ',' << player.center().x << ',' << player.rectangle().bottom()
                    << ',' << player.camera().translation.x << ',' << player.camera().translation.y
                    << ',' << (grounded(player) ? 1 : 0) << ',' << changed << '\n';
            }
            const int frame = tick - route_sequence_start;
            if (save_sequence && frame >= 0 && frame < sequence_frames)
            {
                std::ostringstream filename;
                filename << "frame-" << std::setw(3) << std::setfill('0') << frame << ".ppm";
                save(enabled, on_directory / filename.str());
                save(disabled, off_directory / filename.str());
            }

            const std::string phase = "route-";
            if (!captured_takeoff && std::find(took_off.begin(), took_off.end(), true) != took_off.end())
            {
                captured_takeoff = true;
                capture_phase(app, *level, presentation, output, count, phase + "takeoff",
                    tick, records, true, took_off);
            }
            if (!captured_landing && std::find(landed.begin(), landed.end(), true) != landed.end())
            {
                captured_landing = true;
                capture_phase(app, *level, presentation, output, count, phase + "landing",
                    tick, records, true, landed);
            }
            const bool all_landed = std::all_of(runners.begin(), runners.end(),
                [](const RouteRunner& runner) { return runner.landing_tick >= 0; });
            if (all_landed && tick >= last_landing + landing_expiry &&
                (!save_sequence || frame + 1 >= sequence_frames))
            {
                break;
            }
        }
        for (size_t index = 0; index < runners.size(); ++index)
        {
            const RouteRunner& runner = runners[index];
            require(runner.takeoff_tick >= 0, "a route player never took off");
            require(runner.landing_tick >= 0, "a route player never landed");
            const Vector2F pan = runner.camera_max - runner.camera_min;
            require(pan.x > route_minimum_pan, "a route view's camera did not pan horizontally");
            std::cout << "ROUTE " << prefix << " view=" << index + 1
                << " takeoff=" << runner.takeoff_tick << " landing=" << runner.landing_tick
                << " pan_x=" << pan.x << " pan_y=" << pan.y << '\n';
        }
        require(captured_takeoff && captured_landing, "route phases were not captured");
        std::cout << "PASS " << prefix << " moving aim-swept route; per-tick feedback on/off; "
            << tick + 1 << " ticks\n";
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 4 || argc == 5,
            "usage: movement_feedback_visual <staged-content> <output> <sequence-frames> "
            "[one-player-route-sequences 0|1]");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        const int sequence_frames = std::stoi(argv[3]);
        require(sequence_frames >= 0 && sequence_frames <= 180,
            "sequence frame count must be between 0 and 180");
        // Paired route sequences are four-player only unless asked: each one is
        // two 720p frames per tick for each selected player count.
        const bool one_player_route_sequences = argc == 5 && std::wstring(argv[4]) == L"1";
        std::filesystem::create_directories(output);
        std::ofstream records(output / "phases.csv");
        records << "players,phase,tick,seconds,width,height,view,changed_pixels,readable_pixels\n";
        std::ofstream route_records(output / "route.csv");
        route_records << "players,tick,view,centre_x,feet_y,camera_x,camera_y,grounded,changed_pixels\n";

        Registry<LevelLoadedInfo> levels("Level");
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        require(presentation.movement_feedback().enabled,
            "the visual probe requires feedback enabled in staged presentation.json");
        PlaySettings preferences;
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsMovementFeedbackProbe";
        options.window_title = L"ColourWars movement feedback validation";
        options.resolution = ScreenResolution::s_1280_720;
        options.fullscreen = false;
        options.max_threads = 4;
        options.visible = false;
        Application app(options);
        app.initialize();
        const auto& device = app.renderer()->device_info();
        std::cout << "DEVICE backend=" << device.backend << " api=" << device.api
            << " name=" << device.device_name << " vendor=" << device.vendor_id
            << " device=" << device.device_id << '\n';
        require(device.backend == "d3d11" && device.kind == RenderDeviceKind::hardware,
            "visual evidence requires a D3D11 hardware device");
        app.resource_loader()->register_kind("level", {
            [&](const std::string& directory, const std::string& name, bool) {
                auto info = std::make_unique<LevelLoadedInfo>((directory + name + ".json").c_str());
                info->validate();
                levels.add(name, std::move(info));
            }, nullptr });
        app.load_manifest((content / "manifest.json").string().c_str());
        presentation.validate_resources(*app.render_resources(), *app.audio_resources());
        LevelBuilder builder(app.viewport_manager(), app.render_resources(),
            app.audio_resources(), &preferences, &levels, &tuning, &presentation,
            app.resolution_manager(), app.thread_pool(), app.partitioner(), app.mouse());
        for (int count : { 1, 4 })
        {
            MenuLevelSettings settings;
            settings.set_player_count(count);
            settings.set_stage("movement_course");
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            settings.begin_input_claims(0);
            for (int player = 1; player < count; ++player)
            {
                require(settings.claim_input_slot(player), "input claim failed");
            }
            for (int player = 0; player < count; ++player)
            {
                settings.set_player_team(player, player % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(player, player % 2 == 0 ? WeaponType::sprayer : WeaponType::roller);
            }
            app.viewport_manager()->set_layout(settings.screen_layout());
            auto level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
            std::vector<PlayerInputData> inputs(input_slots::count);
            for (auto& input : inputs)
            {
                input.connected = true;
                input.shoot_direction = Vector2F(1, 0);
                input.shoot_direction_requested = true;
            }
            // Let the actual countdown, gravity and collision settle authored
            // spawn points. The limit deliberately refuses an unready scene.
            for (int tick = 0; tick < 600 && level->state() != LevelState::active; ++tick)
            {
                level->update(inputs, dt);
            }
            require(level->state() == LevelState::active, "countdown did not reach active play");
            for (int tick = 0; tick < 60; ++tick) { level->update(inputs, dt); }
            for (const Player* player : level->players_)
            {
                require(player->state() == PlayerState::alive && grounded(*player),
                    "authored spawn did not settle alive on support");
            }
            if (count == 4) { isolated_bursts(app, *level, presentation, output); }

            const auto sequence = output / (std::to_string(count) + "p-sequence-60fps");
            if (sequence_frames > 0) { std::filesystem::create_directories(sequence); }
            bool landed = false;
            int landing_tick = -1;
            const int takeoff_mid = static_cast<int>(std::ceil(
                presentation.movement_feedback().takeoff.duration / (2 * dt)));
            const int landing_mid = static_cast<int>(std::ceil(
                presentation.movement_feedback().landing.duration / (2 * dt)));
            const int landing_expiry = static_cast<int>(std::ceil(
                presentation.movement_feedback().landing.duration / dt)) + 1;
            bool expired = false;
            for (int tick = 0; tick < 180; ++tick)
            {
                for (auto& input : inputs)
                {
                    input.jump_pressed = tick == 0;
                    input.jump_held = tick < 30;
                }
                level->update(inputs, dt);
                for (const Player* player : level->players_)
                {
                    require(player->state() == PlayerState::alive, "scripted jump killed a player");
                }
                if (tick == 0)
                {
                    for (const Player* player : level->players_)
                    {
                        require(!grounded(*player) && player->velocity().y < 0,
                            "scripted jump did not take off");
                    }
                    capture_phase(app, *level, presentation, output, count, "takeoff", tick, records);
                }
                if (tick == takeoff_mid)
                {
                    capture_phase(app, *level, presentation, output, count, "takeoff-mid", tick, records);
                }
                if (!landed && tick > 0 && std::all_of(level->players_.begin(), level->players_.end(),
                    [](const Player* player) { return grounded(*player); }))
                {
                    landed = true;
                    landing_tick = tick;
                    capture_phase(app, *level, presentation, output, count, "landing", tick, records);
                    check_suspended_draws(*level, *app.renderer(), output, count);
                }
                if (landed && tick == landing_tick + landing_mid)
                {
                    capture_phase(app, *level, presentation, output, count, "landing-mid", tick, records);
                }
                if (landed && tick == landing_tick + landing_expiry)
                {
                    expired = true;
                    for (const Player* player : level->players_)
                    {
                        require(player->movement_feedback().effects().empty(),
                            "movement feedback did not expire");
                    }
                    capture_phase(app, *level, presentation, output, count, "expired", tick, records, false);
                }
                if (tick < sequence_frames)
                {
                    std::ostringstream filename;
                    filename << "frame-" << std::setw(3) << std::setfill('0') << tick << ".ppm";
                    save(render(*level, *app.renderer()), sequence / filename.str());
                }
                if (expired && tick + 1 >= sequence_frames) { break; }
            }
            require(landed, "scripted jump did not land within 180 ticks");
            require(expired, "movement feedback did not expire within 180 ticks");
        }
        const LevelLoadedInfo& course = *levels.get("movement_course");
        for (int count : { 1, 4 })
        {
            route_phase(app, builder, presentation, course, output, count,
                sequence_frames, count == 4 || one_player_route_sequences,
                records, route_records);
        }
        require(records.good() && route_records.good(), "phase record write failed");
        std::cout << "PASS real LevelBuilder/Scene takeoff and landing; 1/4 views; "
            "720p/1080p/QHD; feedback on/off; paused redraw; moving aim-swept route in "
            "the runtime presentation with camera pan; scripted inputs, no physical-device claim\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
