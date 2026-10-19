#pragma once

// The checks that build a real Level through LevelBuilder: gameplay
// invariance, held fire and round end. Included by idle_drip.cpp after
// its shared fixtures.
namespace
{
    PlayerInputData input()
    {
        PlayerInputData result;
        result.connected = true;
        result.shoot_direction_requested = true;
        result.shoot_direction = Vector2F(1, 0);
        return result;
    }

    void compare_gameplay(const Level& first, const Level& second)
    {
        require(first.state() == second.state() && first.projectile_count() == second.projectile_count() &&
            first.collision_object_count() == second.collision_object_count(),
            "Idle-drip feature switch changed level state or gameplay-object counts");
        for (int view = 0; view < first.player_count(); ++view)
        {
            const Player& a = first.player(view);
            const Player& b = second.player(view);
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                a.state() == b.state() && a.move_state() == b.move_state() && a.health() == b.health() &&
                a.weapon_ammo() == b.weapon_ammo() &&
                a.primary_weapon().ready_to_shoot() == b.primary_weapon().ready_to_shoot() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos(),
                "Idle-drip feature switch changed movement, damage, paint, shot readiness or muzzle");
        }
        const auto a = first.level_end_info();
        const auto b = second.level_end_info();
        require(a.player_areas == b.player_areas && a.team_a_area == b.team_a_area &&
            a.team_b_area == b.team_b_area && a.total_area == b.total_area,
            "Cosmetic idle drips captured a surface or changed the paint score");
    }

    void check_level_integration(World& enabled_world, const std::filesystem::path& content)
    {
        World disabled_world(content);
        for (const WeaponType type : supported_tools)
        {
            const_cast<IdleDripPresentation&>(disabled_world.presentation.idle_drip(type)).enabled = false;
        }
        std::srand(753);
        auto enabled = enabled_world.build(900.0f);
        std::srand(753);
        auto disabled = disabled_world.build(900.0f);
        require(enabled->idle_drip(0) != nullptr && enabled->idle_drip(1) != nullptr &&
            enabled->idle_drip(2) != nullptr && enabled->idle_drip(3) == nullptr,
            "Level did not attach residual paint to precisely the sprayer, sniper and bucket");
        {
            auto other = disabled_world.build(300.0f,
                { WeaponType::mister, WeaponType::sniper, WeaponType::bucket, WeaponType::sprayer });
            require(other->idle_drip(0) == nullptr && other->idle_drip(1) != nullptr &&
                other->idle_drip(2) != nullptr && other->idle_drip(3) != nullptr,
                "Mister inherited residual paint or supported tools depended on their roster position");
        }
        std::vector<PlayerInputData> inputs(input_slots::count, input());
        unsigned tick_number = 0;
        const auto tick = [&]
        {
            const unsigned seed = 753u + tick_number++;
            std::srand(seed);
            enabled->update(inputs, dt);
            std::srand(seed);
            disabled->update(inputs, dt);
            compare_gameplay(*enabled, *disabled);
            for (int view = 0; view < 3; ++view)
            {
                require(!disabled->idle_drip(view)->attached() && disabled->idle_drip(view)->drops().empty(),
                    "Disabled Level emitted a cosmetic drip");
            }
        };
        const auto wait_for_bead = [&](int view)
        {
            for (int step = 0; step < 1200 && (!enabled->idle_drip(view)->attached() ||
                enabled->idle_drip(view)->bead_age() != 0.0f); ++step) { tick(); }
            require(enabled->idle_drip(view)->attached() && enabled->idle_drip(view)->bead_age() == 0.0f,
                "Real Level never delivered settled player time to a fresh drip");
        };
        const auto untouched_score = enabled->level_end_info().player_areas;
        const std::array untouched_ammo{ enabled->player(0).weapon_ammo(),
            enabled->player(1).weapon_ammo(), enabled->player(2).weapon_ammo() };
        std::array<bool, 3> fell{};
        for (int step = 0; step < 1200; ++step)
        {
            tick();
            for (int view = 0; view < 3; ++view)
            {
                fell.at(static_cast<size_t>(view)) = fell.at(static_cast<size_t>(view)) ||
                    !enabled->idle_drip(view)->drops().empty();
                require(enabled->player(view).weapon_ammo() == untouched_ammo.at(static_cast<size_t>(view)),
                    "Real idle drip spent ammunition");
            }
        }
        require(std::all_of(fell.begin(), fell.end(), [](bool value) { return value; }) &&
            enabled->level_end_info().player_areas == untouched_score,
            "A real tool's idle drip failed to fall or captured a surface");

        for (int view = 0; view < 3; ++view)
        {
            wait_for_bead(view);
            const auto paused = observation(*enabled->idle_drip(view));
            for (int step = 0; step < 10; ++step) { enabled->update(inputs, 0.0f); }
            require(observation(*enabled->idle_drip(view)) == paused,
                "Frozen Level update advanced residual-paint state");

            // A bead follows the rendered wet lip, including the bucket's rim
            // well behind its projectile launch point. Use independent art
            // landmarks rather than deriving expectations from the drip code.
            for (const float angle : { 0.0f, PI, -PI * 0.25f, -PI * 0.75f, PI * 0.125f, PI * 0.875f })
            {
                auto& aimed = inputs.at(static_cast<size_t>(view));
                aimed.shoot_angle = angle;
                aimed.shoot_direction = Vector2F(std::cos(angle), std::sin(angle));
                tick();
                const Weapon& weapon = enabled->player(view).primary_weapon();
                const auto& look = enabled_world.presentation.idle_drip(weapon.type());
                Vector2F local = view == 1 ? Vector2F(99.0f, 3.0f) : Vector2F(46.0f, 8.0f);
                if (view == 0) { local = Vector2F(69.0f, 3.0f); }
                if (angle > PI * 0.5f || angle < -PI * 0.5f) { local.y = -local.y; }
                const Vector2F anchor = weapon.draw_pos() + Vector2F::rotate_vector(local, angle);
                const IdleDrip& drip = *enabled->idle_drip(view);
                require(weapon.rotation() == angle && drip.attached() &&
                    nearly_equal(drip.bead_rectangle().center().x, anchor.x) &&
                    nearly_equal(drip.bead_rectangle().y, anchor.y) &&
                    drip.bead_rectangle().width <= look.size,
                    "Real aimed/facing tool left its bead off the rendered wet lip");
            }
            inputs.at(static_cast<size_t>(view)) = input();
            inputs.at(static_cast<size_t>(view)).x_movement = 1.0f;
            tick();
            require(!enabled->idle_drip(view)->attached(), "Resolved walking failed to cancel the attached bead");
            inputs.at(static_cast<size_t>(view)) = input();
            wait_for_bead(view);
            const float ammo = enabled->player(view).weapon_ammo();
            inputs.at(static_cast<size_t>(view)).primary_shoot = true;
            tick();
            require(!enabled->idle_drip(view)->attached() && enabled->player(view).weapon_ammo() < ammo,
                "Paid firing failed to cancel the attached bead");
            inputs.at(static_cast<size_t>(view)) = input();
            wait_for_bead(view);
            inputs.at(static_cast<size_t>(view)).jump_pressed = true;
            inputs.at(static_cast<size_t>(view)).jump_held = true;
            tick();
            require(!enabled->idle_drip(view)->attached(), "Jumping failed to cancel the attached bead");
            inputs.at(static_cast<size_t>(view)) = input();
            wait_for_bead(view);
            require(enabled->kill_player(view) && disabled->kill_player(view), "Could not eliminate the drip fixture player");
            tick();
            require(!enabled->idle_drip(view)->attached(), "Elimination retained an attached residual bead");
            require(enabled->respawn_player(view) && disabled->respawn_player(view), "Could not respawn the drip fixture player");
            tick();
            require(!enabled->idle_drip(view)->attached(), "Respawn immediately re-used an old residual bead");

            wait_for_bead(view);
            enabled_world.preferences.reduce_effects = true;
            const auto before_draw = observation(*enabled->idle_drip(view));
            DrawList suppressed;
            enabled->idle_drip(view)->draw(suppressed);
            require(observation(*enabled->idle_drip(view)) == before_draw,
                "Live reduced-effects draw changed the frozen tool drip");
            for (int step = 0; step < 650; ++step)
            {
                tick();
                for (int supported = 0; supported < 3; ++supported)
                {
                    require(!enabled->idle_drip(supported)->attached() && enabled->idle_drip(supported)->drops().empty(),
                        "Live reduced-effects preference retained or restarted a tool drip");
                }
            }
            enabled_world.preferences.reduce_effects = false;
            tick();
            require(!enabled->idle_drip(view)->attached() && enabled->idle_drip(view)->drops().empty() &&
                enabled->idle_drip(view)->interval_remaining() >=
                    enabled_world.presentation.idle_drip(roster.at(static_cast<size_t>(view))).interval_min - dt,
                "Restoring real Level effects resurrected an old drip or skipped the fresh wait");
        }
        std::cout << "PASS idle drip: all three real tools, rendered lip transforms, pause/actions/death/respawn, live preference and gameplay equivalence\n";
    }

    void check_empty_held_fire(const std::filesystem::path& content)
    {
        ContentVariant empty_tank(content / "tuning.json");
        const auto& tuning = empty_tank.write([](rapidjson::Document& json)
        {
            json["weapons"]["starting_ammo"].SetFloat(0.001f);
            json["weapons"]["ammo_regen_delay"].SetFloat(1000.0f);
            json["weapons"]["ammo_regen_rate"].SetFloat(0.0f);
        });
        World world(content, tuning);
        auto level = world.build();
        std::vector<PlayerInputData> inputs(input_slots::count, input());
        for (int view = 0; view < 3; ++view)
        {
            for (int step = 0; step < 1200 && !level->idle_drip(view)->attached(); ++step)
            {
                level->update(inputs, dt);
            }
            require(level->idle_drip(view)->attached(), "Empty tool did not form a bead while genuinely idle");
            inputs.at(static_cast<size_t>(view)).primary_shoot = true;
            level->update(inputs, dt);
            require(!level->idle_drip(view)->attached() &&
                level->player(view).primary_weapon().readiness() == WeaponReadiness::empty &&
                !level->player(view).primary_weapon().shooting_this_update(),
                "An unaffordable held shot retained its bead or was mistaken for paid firing");
        }
        for (int step = 0; step < 1200; ++step)
        {
            level->update(inputs, dt);
            for (int view = 0; view < 3; ++view)
            {
                require(!level->idle_drip(view)->attached() && level->idle_drip(view)->drops().empty() &&
                    level->player(view).weapon_ammo() == 0.001f,
                    "Sustained empty held fire started a residual bead or changed ammunition");
            }
        }
        require(level->projectile_count() == 0, "Empty held-fire fixture emitted gameplay projectiles");
        std::cout << "PASS idle drip: empty held fire interrupts and suppresses all three tools beyond their quiet intervals\n";
    }

    void check_round_end(const std::filesystem::path& content)
    {
        World world(content);
        for (const WeaponType type : supported_tools)
        {
            auto& look = const_cast<IdleDripPresentation&>(world.presentation.idle_drip(type));
            look.interval_min = 0.01f;
            look.interval_max = 0.02f;
            look.grow_time = 0.01f;
            look.stretch_time = 0.01f;
            look.lifetime = 100.0f;
            look.gravity = 0.0f;
        }
        auto level = world.build(0.5f);
        const std::vector<PlayerInputData> inputs(input_slots::count, input());
        std::array<bool, 3> saw_attached{};
        std::array<bool, 3> saw_falling{};
        for (int step = 0; step < 600 && level->state() != LevelState::zoom_out; ++step)
        {
            level->update(inputs, dt);
            for (int view = 0; view < 3; ++view)
            {
                saw_attached.at(static_cast<size_t>(view)) = saw_attached.at(static_cast<size_t>(view)) ||
                    level->idle_drip(view)->attached();
                saw_falling.at(static_cast<size_t>(view)) = saw_falling.at(static_cast<size_t>(view)) ||
                    !level->idle_drip(view)->drops().empty();
            }
        }
        require(std::all_of(saw_attached.begin(), saw_attached.end(), [](bool value) { return value; }) &&
            std::all_of(saw_falling.begin(), saw_falling.end(), [](bool value) { return value; }) &&
            level->state() == LevelState::zoom_out,
            "Round-end fixture failed to exercise attached and falling paint before time-up");
        for (int view = 0; view < 3; ++view)
        {
            require(!level->idle_drip(view)->attached() && level->idle_drip(view)->drops().empty(),
                "Time-up froze residual paint on screen instead of clearing it with the tool");
        }
        for (int step = 0; step < 60; ++step) { level->update(inputs, dt); }
        for (int view = 0; view < 3; ++view)
        {
            require(!level->idle_drip(view)->attached() && level->idle_drip(view)->drops().empty(),
                "Round reveal restarted residual paint after its tool disappeared");
        }
        std::cout << "PASS idle drip: time-up clears and reveal cannot restart attached or falling paint\n";
    }
}
