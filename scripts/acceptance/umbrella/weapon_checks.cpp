#include "game/pch.h"
#include "scripts/acceptance/umbrella/fixtures.h"
#include "game/objects/weapon_builder.h"
#include <algorithm>

namespace umbrella_acceptance
{
    void check_weapon(World& world)
    {
        auto& assets = world.assets;
        const auto& record = assets.tuning.weapon(WeaponType::umbrella);
        const auto& pellet = assets.tuning.projectile(ProjectileType::pellet);
        const Vector2F centre(400, 320), running(230, 0);
        auto tool = WeaponBuilder::build_weapon(&assets.tuning, PlayerTeam::a, 2, 0, Colour::white,
            WeaponType::umbrella, centre, &assets.render, &assets.audio, &assets.preferences);
        require(!tool->canopy() && tool->canopy_state().hits_left == record.canopy.hits &&
            tool->type() == WeaponType::umbrella, "An equipped umbrella must start shut with full durability");
        const float start_ammo = tool->ammo();
        const auto aim = hold(0.27f);
        std::vector<std::unique_ptr<Projectile>> fan;
        for (int i = 0; i < 90 && fan.empty(); ++i)
        {
            fan = tool->update_and_get_projectiles(aim, centre, running, true, dt);
        }
        require(record.pellets >= 2 && fan.size() == static_cast<size_t>(record.pellets) &&
            std::abs(tool->ammo() - (start_ammo - record.details.ammo_usage)) < 0.0001f,
            "A paid umbrella shot must spend one charge and emit the complete fan");
        std::vector<float> angles;
        for (auto& drop : fan)
        {
            require(drop->projectile_type() == ProjectileType::pellet && drop->team() == PlayerTeam::a &&
                drop->player_num() == 2 && drop->player_damage() == pellet.details.player_damage,
                "The umbrella fan lost pellet identity, credit or damage");
            const auto start = drop->shape()->bounding_box().center();
            drop->update(dt);
            auto speed = (drop->shape()->bounding_box().center() - start) / dt;
            speed.x += pellet.details.wind_resistance * dt;
            speed.x -= running.x * record.relative.player_vel_amount;
            speed.y -= pellet.details.gravity * dt;
            require(std::abs(speed.length() - record.details.starting_vel_length) < 0.3f,
                "A pellet lost its relative launch speed");
            angles.push_back(std::atan2(speed.y, speed.x));
        }
        std::sort(angles.begin(), angles.end());
        for (size_t i = 0; i < angles.size(); ++i)
        {
            const float expected = aim.shoot_angle - record.spread / 2.0f +
                record.spread * static_cast<float>(i) / static_cast<float>(angles.size() - 1);
            require(std::abs(expected - angles[i]) < 0.001f, "Pellet fan lost its full spread or even spacing");
        }
        const auto right = tool->canopy();
        require(right && right->team == PlayerTeam::a && right->bounds.size() == record.canopy.size &&
            right->bounds.center() == centre + Vector2F(record.canopy.reach, 0),
            "Held aim did not put the upright canopy the tuned distance in front");
        const auto left = hold(3.14f);
        const Vector2F moved = centre + Vector2F(18, 40);
        tool->update_and_get_projectiles(left, moved, {}, false, dt);
        require(tool->canopy() && tool->canopy()->bounds.center() == moved - Vector2F(record.canopy.reach, 0),
            "Turning or moving left the canopy behind its holder");
        tool->take_canopy_hit();
        tool->update_and_get_projectiles({}, moved, {}, false, dt);
        require(!tool->canopy(), "Release did not shut the canopy");
        tool->update_and_get_projectiles(left, moved, {}, false, dt);
        require(tool->canopy() && tool->canopy_state().hits_left == record.canopy.hits - 1,
            "Tapping Shoot repaired a damaged canopy");
        const float before_hits = tool->ammo();
        for (int remaining = record.canopy.hits - 1; remaining > 0; --remaining)
        {
            require(tool->canopy() && tool->canopy_state().hits_left == remaining, "Canopy broke before its last hit");
            tool->take_canopy_hit();
        }
        require(!tool->canopy() && tool->canopy_state().hits_left == 0 &&
            std::abs(tool->canopy_state().recovery_remaining - record.canopy.recover) < 0.001f &&
            tool->ammo() == before_hits, "The last hit must break the canopy without spending ink");
        int paid_fans = 0;
        float elapsed = 0;
        for (; elapsed + dt < record.canopy.recover - 0.001f; elapsed += dt)
        {
            if (!tool->update_and_get_projectiles(left, moved, {}, false, dt).empty()) { ++paid_fans; }
            require(!tool->canopy(), "A held broken canopy reopened before recovery elapsed");
        }
        require(paid_fans > 0, "The umbrella stopped firing while its canopy was broken");
        for (int tick = 0; tick < 3 && !tool->canopy(); ++tick)
        { tool->update_and_get_projectiles(left, moved, {}, false, dt); }
        require(tool->canopy() && tool->canopy_state().hits_left == record.canopy.hits,
            "A recovered held canopy did not reopen at full durability");

        auto dry = WeaponBuilder::build_weapon(&assets.tuning, PlayerTeam::b, 1, 1, Colour::white,
            WeaponType::umbrella, centre, &assets.render, &assets.audio, &assets.preferences);
        require(dry->spend_paint(dry->ammo()), "Could not empty an umbrella tank");
        for (int i = 0; i < 75; ++i)
        {
            const auto shots = dry->update_and_get_projectiles(aim, centre, {}, true, dt, 0.0f);
            require(shots.empty() && dry->canopy() && dry->ammo() == 0.0f,
                "Holding a dry umbrella should defend without firing an unpaid pellet");
        }
    }
}
