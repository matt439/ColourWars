#include "game/pch.h"
#include "scripts/acceptance/paint_underfoot/fixtures.h"
#include "game/objects/projectile_spray.h"
#include <iostream>

namespace paint_underfoot_acceptance
{
    namespace
    {
        // One enemy hit on a player standing on paint of `floor_team`, then
        // the health after the regeneration delay and a further second.
        float regenerated(World& world, PlayerTeam floor_team)
        {
            Course course(world);
            auto& floor = course.surface();
            world.paint(floor, floor.shape()->bounding_box(), floor_team, -1);
            course.settle();
            const auto& spray = world.tuning.projectile(ProjectileType::spray);
            ProjectileSpray shot(mattmath::RectangleF(0, 0, 200, 200), mattmath::Vector2F::ZERO, PlayerTeam::b,
                paint_owners::index(-1, 1), labrador::Colour::white, &world.render,
                resolve_projectile_strip(spray, world.render), &spray);
            course.player->on_contact(shot, mattmath::Vector2F::ZERO, 0.0f);
            const float hurt = course.player->health();
            require(hurt < world.tuning.player().starting_health, "The enemy hit did no damage");
            const int ticks = static_cast<int>(std::ceil(world.tuning.player().health_regen_delay / dt)) + 60;
            for (int tick = 0; tick < ticks; ++tick) { course.tick(); }
            return course.player->health() - hurt;
        }
    }
    void check_health_regen(World& world)
    {
        require(!world.tuning.player().paint_underfoot.enemy_blocks_health_regen,
            "The health-regen block must ship disabled");
        world.configure(true);
        require(regenerated(world, PlayerTeam::b) > 0.0f, "Enemy paint blocked health regeneration while its toggle was off");
        world.configure(false, 1.10f, 1.25f, 0.85f, 0.0f, true);
        require(regenerated(world, PlayerTeam::b) > 0.0f, "The health-regen block acted while paint underfoot was off");
        world.configure(true, 1.10f, 1.25f, 0.85f, 0.0f, true);
        require(regenerated(world, PlayerTeam::b) == 0.0f, "Enemy paint did not block health regeneration");
        require(regenerated(world, PlayerTeam::a) > 0.0f, "Own paint blocked health regeneration");
        const auto missing = world.variant([](rapidjson::Document& value)
        { value["player"]["paint_underfoot"].RemoveMember("enemy_blocks_health_regen"); });
        require(!missing.player().paint_underfoot.enemy_blocks_health_regen, "A missing health-regen toggle did not read as off");
        world.configure(false);
        std::cout << "PASS underfoot health: enemy paint pauses regeneration only when both toggles are on\n";
    }
}
