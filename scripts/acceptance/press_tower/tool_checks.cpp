#include "game/pch.h"
#include "scripts/acceptance/press_tower/fixtures.h"

namespace press_tower_acceptance
{
    namespace
    {
        const char* tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Press Tower station needs a concrete primary");
            }
        }

        // A short paid painting push from a resting station: aim forward and
        // down, move for 12 ticks, fire for 90, then let the paint land.
        void station(Services& s, WeaponType type, PlayerTeam team, const Surface& support, float centre)
        {
            const bool right = team == PlayerTeam::a;
            Run run(s, standing_on(s, support, centre), team, type);
            run.settle();
            const float initial = run.player->weapon_ammo();
            const float angle = right ? PI / 3.0f : 2.0f * PI / 3.0f;
            for (int tick = 0; tick < 240; ++tick)
            {
                PlayerInputData actions = aimed(angle, input(tick < 12 ? (right ? 1.0f : -1.0f) : 0.0f));
                actions.primary_shoot = tick < 90;
                run.tick(actions);
            }
            const float area = run.team_area(team, support.name);
            require(run.paid_events > 0 && run.minimum_ammo < initial,
                std::string("Press Tower ") + tool_name(type) + " never paid for fire on '" + support.name + "'");
            require(area > 0.0f, std::string("Press Tower ") + tool_name(type) + " did not paint '" + support.name +
                "' from its own station");
            require(run.team_area(right ? PlayerTeam::b : PlayerTeam::a) == 0.0f,
                "Press Tower tool station assigned paint to the wrong team");
            std::cout << "MEASURE Press Tower tool=" << tool_name(type) << " team=" << (right ? 'a' : 'b') <<
                " station=" << support.name << " area=" << fixed(area) << " all_team_area=" << fixed(run.team_area(team)) <<
                " minimum_ammo=" << fixed(run.minimum_ammo, 4) << " paid_events=" << run.paid_events <<
                " emitted_objects=" << run.emitted_objects << " support=" << run.support() << '\n';
        }
    }

    void tool_checks(Services& s)
    {
        const Surface& bed = s.map.named("press_bed");
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                const bool right = team == PlayerTeam::a;
                // Under the press head, 200 of headroom.
                station(s, type, team, bed, right ? 1700.0f : 1900.0f);
                // A stair tread open above, backed by the next riser.
                station(s, type, team, s.map.named(right ? "stair_2_a" : "stair_2_b"), right ? 1240.0f : 2360.0f);
            }
        }
        std::cout << "PASS Press Tower five tools pay ammo and paint the bed under the press head and a stair "
            "tread from both teams. Stations establish access, not balance or weapon-role acceptance\n";
    }
}
