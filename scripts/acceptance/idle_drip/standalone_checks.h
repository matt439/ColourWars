#pragma once

// The checks that build an IdleDrip directly from presentation data, with
// no Level. Included by idle_drip.cpp after its shared fixtures.
namespace
{
    void check_profiles(World& world, const std::filesystem::path& content)
    {
        const auto& sprayer = world.presentation.idle_drip();
        const auto& sniper = world.presentation.idle_drip(WeaponType::sniper);
        const auto& bucket = world.presentation.idle_drip(WeaponType::bucket);
        require(sprayer.enabled && sprayer.interval_min == 2.8f && sprayer.interval_max == 6.2f &&
            sprayer.size == 6.0f && sprayer.stretch == 5.0f && sprayer.nozzle_inset == 8.5f &&
            sprayer.nozzle_underside == 3.0f,
            "Sprayer residual-paint profile changed while adding other tools");
        require(sniper.enabled && sniper.interval_min > sprayer.interval_min &&
            sniper.interval_max > sprayer.interval_max && sniper.size < sprayer.size &&
            sniper.stretch < sprayer.stretch && bucket.enabled &&
            bucket.interval_min > sprayer.interval_min && bucket.interval_max > sprayer.interval_max &&
            bucket.size < sprayer.size && bucket.stretch < sprayer.stretch,
            "Sniper or bucket lost its sparse, tool-specific residual-paint profile");
        require(!world.presentation.idle_drip(WeaponType::roller).enabled &&
            !world.presentation.idle_drip(WeaponType::mister).enabled,
            "An unsupported tool inherited an enabled residual-paint profile");
        for (const WeaponType type : supported_tools)
        {
            const auto& look = world.presentation.idle_drip(type);
            require(look.grow_time == sprayer.grow_time && look.stretch_time == sprayer.stretch_time &&
                look.lifetime == sprayer.lifetime && look.gravity == sprayer.gravity &&
                look.sprite_sheet == sprayer.sprite_sheet && look.fill_frame == sprayer.fill_frame &&
                look.keyline == sprayer.keyline && look.keyline_width == sprayer.keyline_width,
                "Tool residual paint did not inherit the shared liquid presentation");
        }

        ContentVariant invalid(content / "presentation.json");
        const auto reject = [&](const char* tool, const char* field, float value)
        {
            const auto& path = invalid.write([&](rapidjson::Document& json)
            {
                json["idle_drip"]["tools"][tool][field].SetFloat(value);
            });
            bool rejected = false;
            try { static_cast<void>(Presentation(path.string().c_str())); }
            catch (const std::exception& error)
            {
                const std::string message(error.what());
                rejected = message.find(path.string()) != std::string::npos &&
                    message.find(tool) != std::string::npos;
            }
            require(rejected, "Malformed drip profile did not fail with its source path and tool name");
        };
        reject("sniper", "interval_max", sniper.interval_min - 0.1f);
        reject("bucket", "size", bucket.keyline_width * 2.0f);
        reject("bucket", "nozzle_underside", 0.0f);
        std::cout << "PASS idle drip profiles: preserved sprayer, sparse sniper/bucket, unsupported tools and contextual content errors\n";
    }

    void check_quiet_intervals(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        IdleDrip first(&look, &world.render, Colour::white, 0);
        const float initial = first.interval_remaining();
        for (int tick = 0; tick < 1200; ++tick)
        {
            first.advance(false, Vector2F::ZERO, 0.0f, dt, {});
            require(!first.attached() && first.drops().empty() && first.interval_remaining() == initial,
                "Unsettled time consumed the next quiet interval or created a drip");
        }
        first.advance(true, Vector2F::ZERO, 0.0f, initial * 0.5f, {});
        require(!first.attached() && first.interval_remaining() < initial,
            "Settled sprayer skipped the wait or failed to age its quiet interval");
        first.advance(false, Vector2F::ZERO, 0.0f, dt, {});
        const float restarted = first.interval_remaining();
        require(restarted >= look.interval_min && restarted <= look.interval_max,
            "Activity did not schedule a complete new quiet interval");
        first.advance(true, Vector2F::ZERO, 0.0f, restarted * 0.5f, {});
        require(!first.attached(), "Interrupted quiet time leaked into the next idle interval");

        std::srand(8123);
        const int expected_random = std::rand();
        std::srand(8123);
        IdleDrip same_a(&look, &world.render, Colour::white, 3);
        IdleDrip same_b(&look, &world.render, Colour::white, 3);
        IdleDrip different(&look, &world.render, Colour::white, 4);
        bool varied = false;
        bool distinct = false;
        float previous = same_a.interval_remaining();
        for (int cycle = 0; cycle < 12; ++cycle)
        {
            require(observation(same_a) == observation(same_b),
                "Equal roster identities did not reproduce the local cosmetic sequence");
            distinct = distinct || same_a.interval_remaining() != different.interval_remaining();
            detach(same_a, look);
            detach(same_b, look);
            detach(different, look);
            const float interval = same_a.interval_remaining();
            require(interval >= look.interval_min && interval <= look.interval_max,
                "Local drip interval escaped its presentation range");
            varied = varied || interval != previous;
            previous = interval;
        }
        require(varied && distinct && std::rand() == expected_random,
            "Drip timing repeated uniformly, shared identities or consumed gameplay rand()");
        for (int cycle = 0; cycle < 6; ++cycle)
        {
            detach(same_a, look);
            std::ignore = std::rand();
            detach(same_b, look);
            require(observation(same_a) == observation(same_b),
                "Gameplay random draws changed the drip's private interval sequence");
        }
        std::cout << "PASS idle drip: settled waits, interrupted waits, varied per-seat intervals and isolated RNG\n";
    }

    void check_attachment_and_falling(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        const Vector2F nozzle(120.0f, 80.0f);
        IdleDrip right(&look, &world.render, Colour::white, 0);
        IdleDrip left(&look, &world.render, Colour::white, 0);
        attach(right, nozzle);
        attach(left, nozzle, PI);
        require(nearly_equal(right.bead_rectangle().center().x, nozzle.x - look.nozzle_inset) &&
            nearly_equal(left.bead_rectangle().center().x, nozzle.x + look.nozzle_inset) &&
            nearly_equal(right.bead_rectangle().y, nozzle.y + look.nozzle_underside) &&
            nearly_equal(left.bead_rectangle().y, right.bead_rectangle().y),
            "Facing flip moved the residual bead off the tool's nozzle underside");

        const RectangleF initial_bead = right.bead_rectangle();
        right.advance(true, nozzle, 0.0f, look.grow_time * 0.5f, {});
        const RectangleF growing = right.bead_rectangle();
        require(growing.width > initial_bead.width && nearly_equal(growing.height, growing.width) &&
            nearly_equal(growing.y, initial_bead.y), "Attached bead did not grow from its nozzle attachment");
        right.advance(true, nozzle, 0.0f, look.grow_time * 0.5f + look.stretch_time * 0.5f, {});
        const RectangleF stretched = right.bead_rectangle();
        require(nearly_equal(stretched.width, look.size) && stretched.height > stretched.width &&
            nearly_equal(stretched.y, initial_bead.y), "Grown bead did not stretch downwards before release");
        const Vector2F moved_nozzle(140.0f, 50.0f);
        const float angle = PI * 0.25f;
        left.advance(true, moved_nozzle, angle, look.grow_time * 0.25f, {});
        const Vector2F expected_anchor = moved_nozzle +
            Vector2F::rotate_vector(Vector2F(-look.nozzle_inset, look.nozzle_underside), angle);
        require(nearly_equal(left.bead_rectangle().center().x, expected_anchor.x) &&
            nearly_equal(left.bead_rectangle().y, expected_anchor.y),
            "Continuous aim or translation left an attached bead behind the nozzle");

        right.advance(true, nozzle, 0.0f, look.stretch_time, {});
        require(!right.attached() && right.drops().size() == 1, "Stretched bead failed to release");
        const Vector2F released = right.drops()[0].position;
        const float fall_step = std::min(0.05f, look.lifetime * 0.1f);
        right.advance(false, Vector2F(900, -900), PI, fall_step, {});
        require(right.drops().size() == 1 && right.drops()[0].position.x == released.x &&
            right.drops()[0].position.y > released.y,
            "Detached paint followed later aim or failed to fall under gravity");
        const float first_fall = right.drops()[0].position.y - released.y;
        const float before_second = right.drops()[0].position.y;
        right.advance(false, Vector2F(-900, 900), -PI * 0.5f, fall_step, {});
        require(right.drops()[0].position.y - before_second > first_fall,
            "Falling residual paint did not accelerate under gravity");

        left.advance(false, nozzle, 0.0f, dt, {});
        require(!left.attached() && left.drops().empty(), "Activity failed to cancel an attached bead");
        right.advance(false, nozzle, 0.0f, look.lifetime, {});
        require(right.drops().empty(), "Falling residual paint exceeded its bounded lifetime");
        std::cout << "PASS idle drip: both facings, aimed attachment, growth/stretch, independent gravity and cancellation\n";
    }

    void check_terrain(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        IdleDrip thin(&look, &world.render, Colour::white, 0);
        detach(thin, look);
        const float x = thin.drops()[0].position.x;
        const float top = thin.drops()[0].position.y + 20.0f;
        auto platform = world.structure(std::make_unique<RectangleF>(x - 20.0f, top, 40.0f, 0.1f),
            CollisionObjectType::structure_jump_through);
        const std::array<CollisionObject*, 1> platform_targets{ platform.get() };
        thin.advance(false, Vector2F::ZERO, 0.0f, 0.5f, platform_targets);
        require(thin.drops().empty(), "Residual paint tunnelled through a thin jump-through platform");

        IdleDrip ramp(&look, &world.render, Colour::white, 0);
        detach(ramp, look, Vector2F(10.0f + look.nozzle_inset, 0.0f));
        auto slope = world.structure(std::make_unique<Triangle>(0.0f, 100.0f, 100.0f, 0.0f, 100.0f, 100.0f),
            CollisionObjectType::structure_ramp_right);
        const std::array<CollisionObject*, 1> ramp_targets{ slope.get() };
        ramp.advance(false, Vector2F::ZERO, 0.0f, 0.35f, ramp_targets);
        require(ramp.drops().size() == 1,
            "Ramp bounding box retired a drop still above its actual sloping surface");
        ramp.advance(false, Vector2F::ZERO, 0.0f, 0.25f, ramp_targets);
        require(ramp.drops().empty(), "Residual paint passed through the ramp's actual surface");

        IdleDrip blocked(&look, &world.render, Colour::white, 0);
        auto wall = world.structure(std::make_unique<RectangleF>(-20.0f, 0.0f, 40.0f, 30.0f));
        const std::array<CollisionObject*, 1> wall_targets{ wall.get() };
        blocked.advance(true, Vector2F::ZERO, 0.0f, blocked.interval_remaining() + 0.001f, wall_targets);
        require(!blocked.attached() && blocked.drops().empty(),
            "Attached paint grew inside terrain instead of disappearing on contact");

        IdleDrip ignored(&look, &world.render, Colour::white, 0);
        auto projectile_tag = world.structure(std::make_unique<RectangleF>(-20.0f, 0.0f, 40.0f, 30.0f),
            CollisionObjectType::projectile_spray_team_a);
        const std::array<CollisionObject*, 1> ignored_targets{ projectile_tag.get() };
        ignored.advance(true, Vector2F::ZERO, 0.0f, ignored.interval_remaining() + 0.001f, ignored_targets);
        require(ignored.attached(), "Residual paint treated a non-terrain collision object as scenery");
        std::cout << "PASS idle drip: swept thin-platform contact, true ramp geometry, attached contact and terrain filtering\n";
    }

    void check_freeze_disable_and_capacity(World& world)
    {
        static_assert(!std::is_base_of_v<CollisionObject, IdleDrip>);
        static_assert(std::is_same_v<decltype(&IdleDrip::draw), void (IdleDrip::*)(DrawList&) const>);
        auto look = world.presentation.idle_drip();
        look.lifetime = 100.0f;
        IdleDrip drip(&look, &world.render, Colour::white, 0);
        detach(drip, look);
        attach(drip);
        const auto before = observation(drip);
        for (float elapsed : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            drip.advance(false, Vector2F(900, 900), PI, elapsed, {});
            drip.update(elapsed);
            require(observation(drip) == before, "Frozen or invalid delta advanced residual-paint state");
        }

        const auto falling = drip.drops()[0];
        drip.advance(false, Vector2F(900, 900), PI, dt, {});
        require(!drip.attached() && drip.drops().size() == 1 &&
            drip.drops()[0].position.x == falling.position.x &&
            drip.drops()[0].position.y > falling.position.y,
            "Activity cancelled an existing falling drop or retained its attached bead");
        attach(drip);
        const auto before_draw = observation(drip);

        look.enabled = false;
        DrawList unused;
        for (int view = 0; view < 4; ++view) { drip.draw(unused); }
        require(observation(drip) == before_draw, "Disabled draw fan-out mutated simulation state");
        drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        require(!drip.attached() && drip.drops().empty(), "Disabled presentation retained active residual paint");
        for (int tick = 0; tick < 600; ++tick) { drip.advance(true, Vector2F::ZERO, 0.0f, dt, {}); }
        require(!drip.attached() && drip.drops().empty(), "Disabled presentation emitted residual paint");

        look.enabled = true;
        look.interval_min = 0.001f;
        look.interval_max = 0.002f;
        look.grow_time = 0.001f;
        look.stretch_time = 0.001f;
        look.lifetime = 1000.0f;
        look.gravity = 0.0f;
        IdleDrip crowded(&look, &world.render, Colour::white, 0);
        for (int tick = 0; tick < 1000; ++tick)
        {
            crowded.advance(true, Vector2F::ZERO, 0.0f, dt, {});
            require(crowded.drops().size() <= 4, "Residual paint exceeded its fixed four-drop storage");
        }
        require(crowded.drops().size() == 4, "Capacity fixture never exercised full residual-paint storage");
        attach(crowded);
        crowded.clear();
        require(!crowded.attached() && crowded.drops().empty() && crowded.bounds() == RectangleF::ZERO,
            "Explicit scene-ending clear retained attached or falling residual paint");
        crowded.clear();
        require(!crowded.attached() && crowded.drops().empty(), "Residual-paint clear was not idempotent");
        std::cout << "PASS idle drip: frozen deltas, const/disabled draw, feature switch and fixed capacity\n";
    }

    void check_live_preference(World& world)
    {
        auto look = world.presentation.idle_drip();
        look.lifetime = 100.0f;
        PlaySettings preferences;
        IdleDrip drip(&look, &world.render, Colour::white, 0, &preferences);
        detach(drip, look);
        attach(drip);
        const auto before = observation(drip);
        preferences.reduce_effects = true;
        // A default list cannot accept sprites. Deliberate suppression must
        // return before touching it, including while the match is paused.
        DrawList suppressed;
        for (int view = 0; view < 4; ++view) { drip.draw(suppressed); }
        require(observation(drip) == before,
            "Reduced-effects draw mutated the frozen residual-paint state");
        drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        require(!drip.attached() && drip.drops().empty() && drip.bounds() == RectangleF::ZERO,
            "Reduced-effects update retained attached or detached residual paint");
        for (int tick = 0; tick < 600; ++tick)
        {
            drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        }
        require(!drip.attached() && drip.drops().empty(),
            "Reduced-effects sprayer generated new residual paint");
        preferences.reduce_effects = false;
        require(!drip.attached() && drip.drops().empty(),
            "Restoring effects resurrected an old attached or detached drip");
        drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        require(!drip.attached() && drip.drops().empty() &&
            drip.interval_remaining() >= look.interval_min - dt,
            "Restoring effects skipped the fresh residual-paint wait");
        attach(drip);
        require(drip.drops().empty(), "Fresh bead inherited previous detached drops");
        std::cout << "PASS idle drip preference: frozen draw suppression, active clear, no new drips and fresh restart\n";
    }
}
