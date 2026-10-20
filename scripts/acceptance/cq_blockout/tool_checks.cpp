#include "game/pch.h"
#include "scripts/acceptance/cq_blockout/fixtures.h"

namespace cq_blockout_acceptance
{
    void tool_checks(Services& s)
    {
        const Surface& main = s.map.named("lower_paint");
        const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::roller, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket })
        {
            const char* name = type == WeaponType::sprayer ? "sprayer" : type == WeaponType::roller ? "roller" :
                type == WeaponType::sniper ? "sniper" : type == WeaponType::mister ? "mister" : "bucket";
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const bool right = team == PlayerTeam::a;
                const float centre = right ? 1750.0f : bounds.left() + bounds.right() - 1750.0f;
                Run run(s, standing_on(s, main, centre), team, type);
                run.settle();
                const float initial_ammo = run.player->weapon_ammo();
                const float angle = right ? PI / 3.0f : 2.0f * PI / 3.0f;
                // A short paid painting push uses the actual main scoring
                // surface. Flying shots are measured separately from launch
                // obstruction/contact paint; neither is fabricated contact.
                for (int tick = 0; tick < 240; ++tick)
                {
                    PlayerInputData actions = aimed(angle,
                        input(tick < 45 ? (right ? 1.0f : -1.0f) : 0.0f));
                    actions.primary_shoot = tick < 90;
                    run.tick(actions);
                }
                const float area = run.team_area(team, main.name);
                require(run.minimum_ammo < initial_ammo, std::string("CQ ") + name + " station never paid for fire");
                require(area > 0.0f, std::string("CQ ") + name + " did not paint the main deck from its connected route");
                require(run.team_area(right ? PlayerTeam::b : PlayerTeam::a) == 0.0f,
                    "CQ tool station credited the wrong team");
                std::cout << "MEASURE CQ tool=" << name << " team=" << (right ? 'a' : 'b') <<
                    " main_face_area=" << fixed(area) << " maximum_net_ammo_drop=" << fixed(initial_ammo - run.minimum_ammo) <<
                    " emitted_objects=" << run.shots << " swept_area=" << fixed(run.swept_paint) <<
                    " support=" << run.support() << '\n';
            }
        }
        const Surface& cover = s.map.named("centre_cover");
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket })
        {
            const char* name = type == WeaponType::sprayer ? "sprayer" : type == WeaponType::sniper ? "sniper" :
                type == WeaponType::mister ? "mister" : "bucket";
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const bool right = team == PlayerTeam::a;
                const float centre = right ? 2600.0f : bounds.left() + bounds.right() - 2600.0f;
                Run run(s, standing_on(s, main, centre), team, type);
                run.settle();
                run.swept_surface = cover.name;
                // The bucket needs a shallow upward throw to keep its broad
                // falling blob off the floor before it reaches the baffle.
                // The three other tools use the level firing line.
                const float forward_angle = type == WeaponType::bucket ? -0.2f : 0.0f;
                const float angle = right ? forward_angle : PI - forward_angle;
                run.tick(aimed(angle));
                const Vector2F nozzle = run.player->primary_weapon().nozzle_pos();
                const Vector2F half = s.tuning.projectile(s.tuning.weapon(type).details.proj_type).
                    details.col_rect_size / 2.0f;
                const float gap = right ? cover.rectangle.left() - nozzle.x - half.x :
                    nozzle.x - half.x - cover.rectangle.right();
                require(gap > tolerance && nozzle.y + half.y < main.rectangle.top(),
                    std::string("CQ ") + name + " flight station starts inside cover or the floor");
                const float initial_ammo = run.player->weapon_ammo();
                for (int tick = 0; tick < 240; ++tick)
                {
                    PlayerInputData actions = aimed(angle);
                    actions.primary_shoot = tick < 90;
                    run.tick(actions);
                }
                const float cover_area = run.team_area(team, cover.name);
                require(run.minimum_ammo < initial_ammo && run.shots > 0 && run.swept_paint > 0.0f &&
                    cover_area > 0.0f, std::string("CQ ") + name +
                    " paid flying paint did not reach the contested baffle from outside contact range");
                require(run.team_area(right ? PlayerTeam::b : PlayerTeam::a) == 0.0f,
                    "CQ flying tool station credited the wrong team");
                std::cout << "MEASURE CQ flight tool=" << name << " team=" << (right ? 'a' : 'b') <<
                    " initial_gap=" << fixed(gap) << " aim=" << fixed(angle, 3) <<
                    " cover_area=" << fixed(cover_area) << " maximum_net_ammo_drop=" << fixed(initial_ammo - run.minimum_ammo) <<
                    " emitted_objects=" << run.shots << " swept_cover_area=" << fixed(run.swept_paint) << '\n';
            }
        }
        std::cout << "PASS CQ all five current tools pay ammo and paint the main connected scoring surface from both sides; "
            "four aimed tools also reach the contested baffle by paid swept flight from mirrored firing lines. "
            "Measurements establish access, not balance or human weapon-role acceptance\n";
    }
}
