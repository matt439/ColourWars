#include "game/pch.h"
#include "scripts/acceptance/umbrella/fixtures.h"
#include "game/objects/player_builder.h"
#include "game/objects/projectile_rolling.h"

namespace umbrella_acceptance
{
    void check_sweeps(World& world)
    {
        auto& assets = world.assets;
        const auto settings = roster();
        world.viewports.set_layout(settings.screen_layout());
        PlayerBuilder builder(&assets.tuning, &assets.presentation);
        auto players = builder.build_players(settings, world.levels.get("crossfeed"), TeamColour{},
            &assets.render, &assets.audio, &assets.preferences, &world.viewports);
        Player& player = *players.front();
        player.place_at({ 400, 480 });
        player.set_player_input(hold());
        player.prepare_canopy(dt);
        require(player.canopy().has_value(), "An equipped living player did not offer a canopy");
        const auto face = player.canopy()->bounds;
        const std::array<Player*, 1> holders{ &player };
        const auto hits = [&] { return player.primary_weapon().canopy_state().hits_left; };
        auto cover = assets.surface(face, { true, true, true, true });
        const auto& sub = assets.tuning.sub(SubType::wall);
        const auto size = sub.body.details.col_rect_size;
        const Vector2F wall_centre(face.right() - size.x * 0.5f, face.center().y);
        ProjectileWall wall(RectangleF(wall_centre, size), {}, PlayerTeam::a, 0, Colour::white, &assets.render,
            resolve_projectile_strip(sub.body, assets.render), resolve_projectile_strip(sub.burst, assets.render), &sub);
        wall.on_structure_contact(*cover, {}, 0);
        require(wall.planted() && wall.bounds().right() == face.right(), "Planted wall and canopy lack the same entry face");
        const std::array<Projectile*, 1> planted{ &wall };
        const std::array<CollisionObject*, 1> covers{ cover.get() }, bodies{ &player };
        const Vector2F start(face.right() + 16, face.center().y), approach(-4800, 0);
        for (const ProjectileTargetFlags flags : { ProjectileTargetFlags{ false, false },
            ProjectileTargetFlags{ false, true }, ProjectileTargetFlags{ true, false } })
        {
            const auto sweep = [&](std::span<CollisionObject* const> targets,
                std::span<Projectile* const> subs, PlayerTeam team)
            {
                auto shot = straight_shot(world, start, approach, team);
                shot->update(dt);
                shot->resolve_movement_contacts(targets, flags, subs, holders);
                return shot;
            };
            const int before = hits(), wall_hits = wall.hits_left();
            require(sweep(covers, planted, PlayerTeam::b)->retire_cause() == ShotRetireCause::cover &&
                hits() == before && wall.hits_left() == wall_hits, "Cover lost a tied entry to a wall or canopy");
            require(sweep(bodies, planted, PlayerTeam::b)->retire_cause() == ShotRetireCause::breakable &&
                wall.hits_left() == wall_hits - 1 && hits() == before,
                "An exact tie must spend one planted-wall hit before the canopy");
            const float health = player.health();
            require(sweep(bodies, {}, PlayerTeam::b)->retire_cause() == ShotRetireCause::breakable &&
                player.health() == health && hits() == before - 1, "A swept shot hit the player behind the canopy");
            require(!sweep(bodies, planted, PlayerTeam::a)->for_deletion() && hits() == before - 1 &&
                wall.hits_left() == wall_hits - 1, "Friendly fire spent a shot or cost a block");
        }
        const int before = hits();
        auto beyond = straight_shot(world, player.center(), {}, PlayerTeam::b);
        beyond->resolve_launch_obstruction(start, bodies, std::nullopt, holders);
        require(beyond->retire_cause() == ShotRetireCause::breakable && hits() == before - 1,
            "An enemy muzzle reaching past the canopy bypassed its face");
        const auto& rolling = assets.tuning.projectile(ProjectileType::rolling);
        ProjectileRolling roller(RectangleF(face.center(), rolling.details.col_rect_size), {}, PlayerTeam::b, 1,
            Colour::white, &assets.render, resolve_projectile_strip(rolling, assets.render), &rolling);
        roller.resolve_launch_obstruction(start, {}, std::nullopt, holders);
        roller.update(dt);
        roller.resolve_movement_contacts({}, std::nullopt, {}, holders);
        require(!roller.for_deletion() && hits() == before - 1, "An unswept roller was consumed by the canopy");
    }
}
