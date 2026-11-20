#pragma once

#include "scripts/acceptance/bot_navigation/fixtures.h"
#include "game/objects/projectile_ball.h"
#include "game/objects/projectile_wall.h"

namespace umbrella_acceptance
{
    using namespace bot_navigation_acceptance;

    inline PlayerInputData hold(float radians = 0.0f)
    {
        PlayerInputData input;
        input.connected = true;
        input.primary_shoot = true;
        input.shoot_direction_requested = true;
        input.shoot_angle = radians;
        input.shoot_direction = Vector2F::unit_vec_from_angle(radians);
        return input;
    }

    inline MatchSetup roster()
    {
        MatchSetup settings;
        settings.set_player_count(2);
        settings.set_screen_layout(ScreenLayout::two_player);
        settings.set_stage("crossfeed");
        settings.set_duration(120.0f);
        settings.begin_input_claims(0);
        require(settings.claim_input_slot(1), "Cannot claim second Umbrella seat");
        settings.set_player_team(0, PlayerTeam::a);
        settings.set_player_team(1, PlayerTeam::b);
        settings.set_player_weapon(0, WeaponType::umbrella);
        settings.set_player_weapon(1, WeaponType::roller);
        return settings;
    }

    inline std::unique_ptr<ProjectileBall> straight_shot(World& world, Vector2F centre, Vector2F velocity,
        PlayerTeam team)
    {
        const auto& jet = world.assets.tuning.projectile(ProjectileType::jet);
        return std::make_unique<ProjectileBall>(RectangleF(centre, jet.details.col_rect_size),
            velocity, team, 1, Colour::white, &world.assets.render, resolve_projectile_strip(jet, world.assets.render), &jet);
    }

    void check_weapon(World& world);
    void check_match(World& world);
    void check_sweeps(World& world);
}
