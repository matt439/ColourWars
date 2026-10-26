#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/tool_fixtures.h"

namespace turbulence_blockout_acceptance
{
    namespace
    {
        struct FirstContact
        {
            int ticks = 0;
            int paid = 0;
            int launches = 0;
            float before = 0.0f;
            float after = 0.0f;
        };

        float reflect_angle(bool right, float angle) { return right ? angle : PI - angle; }

        float receiver_area(const Run& run, PlayerTeam team, const std::string& owner, float left, float right)
        {
            float area = 0.0f;
            for (const auto& [surface, object] : run.structures)
            {
                if (surface->name != owner) { continue; }
                const auto* scoring = dynamic_cast<const PaintableObject*>(object);
                require(scoring != nullptr, "Intended receiver has lost its scoring role");
                std::vector<PaintSurfaceSample> samples;
                scoring->append_paint_surfaces(samples);
                for (const auto& tile : samples)
                {
                    if (tile.team != team || tile.face != PaintSurfaceFace::top) { continue; }
                    const float centre = (tile.corners[0].x + tile.corners[2].x) / 2.0f;
                    if (centre >= left && centre < right) { area += tile.scored_area; }
                }
            }
            return area;
        }

        FirstContact first_contact(Services& services, WeaponType tool, bool right, bool upper)
        {
            const PlayerTeam team = right ? PlayerTeam::a : PlayerTeam::b;
            const PlayerTeam enemy = right ? PlayerTeam::b : PlayerTeam::a;
            const std::string source = std::string(upper ? "crossover" : "lower_bed") + (right ? "_a" : "_b");
            const std::string receiver = upper ? source : "ground";
            const float start = upper ? 1670.0f : 1420.0f, target_x = upper ? 1840.0f : 1780.0f;
            Run run(services, standing(services, services.named(source), reflected(right, start)), team, 0.0f, tool);
            Player& target = run.add_target(standing(services, services.named(receiver), reflected(right, target_x)), enemy);
            run.settle();
            require(supported(target) && target.state() == PlayerState::alive && target.health() > 0.0f,
                "Real receiver target did not settle on its authored support");
            const RectangleF initial_body = target.rectangle();
            const float initial_health = target.health();
            const int first = run.ticks;
            float angle = 0.0f;
            if (tool == WeaponType::sniper) { angle = (target.center() - run.player->center()).angle(); }
            else if (tool == WeaponType::bucket) { angle = reflect_angle(right, -PI / 6.0f); }
            else { angle = reflect_angle(right, 0.0f); }
            const float direction = right ? 1.0f : -1.0f;
            for (int tick = 0; tick < 180 && target.health() == initial_health; ++tick)
            {
                // Contact is the Roller's role; it closes through ordinary
                // movement rather than borrowing the ranged tools' reach.
                run.tick(firing(angle, tool == WeaponType::roller ? direction : 0.0f));
                require(target.health() > 0.0f && target.state() == PlayerState::alive,
                    "First-contact fixture continued through an elimination");
            }
            require(target.health() < initial_health && target.health() > 0.0f && run.paid_events > 0,
                "Useful receiver contact failed: " + std::string(tool_name(tool)) + " from " + source);
            require(target.rectangle() == initial_body,
                "Real receiver target moved after construction; first-contact trial must retain its initial position");
            // End this independent trial at the first damage observation. Do
            // not delete/teleport a victim into a subsequent recovery story.
            return { run.ticks - first, run.paid_events, run.emitted_objects, initial_health, target.health() };
        }
    }

    int useful_stations(Services& services, const std::filesystem::path& output)
    {
        std::ofstream csv(output / "turbulence-useful-stations.csv");
        require(csv.good(), "Cannot create useful receiver station receipt");
        csv << std::setprecision(std::numeric_limits<float>::max_digits10);
        csv << "team,tool,source,receiver,receiver_left,receiver_right,contact_ticks,contact_paid,contact_launches,"
            "target_health_before,target_health_after,paint_paid,paint_launches,receiver_area_at_release,receiver_area_at_retreat,"
            "receiver_area_settled,retreat_ticks,retreat_support,arrival_ammo,recovered_ammo,shot_free_ticks,paint_updates,scope\n";
        int updates = 0, trials = 0;
        for (const WeaponType tool : concrete_tools)
        {
            for (const bool right : { true, false })
            {
                const PlayerTeam team = right ? PlayerTeam::a : PlayerTeam::b;
                for (const bool upper : { false, true })
                {
                    const FirstContact contact = first_contact(services, tool, right, upper);
                    const std::string source = std::string(upper ? "crossover" : "lower_bed") + (right ? "_a" : "_b");
                    const std::string receiver = upper ? source : "ground";
                    const float left_a = upper ? 1780.0f : 1740.0f, right_a = upper ? 1940.0f : 2260.0f;
                    const float left = right ? left_a : 4000.0f - right_a, edge = right ? right_a : 4000.0f - left_a;
                    Run run(services, standing(services, services.named(source), reflected(right, upper ? 1670.0f : 1420.0f)),
                        team, 0.0f, tool);
                    run.settle();
                    const float full = run.player->weapon_ammo();
                    float angle = 0.0f;
                    if (tool == WeaponType::sniper)
                    {
                        const Vector2F point(reflected(right, upper ? 1840.0f : 1900.0f), upper ? 800.0f : 1280.0f);
                        angle = (point - run.player->center()).angle();
                    }
                    else if (tool == WeaponType::bucket) { angle = reflect_angle(right, upper ? 0.0f : -PI / 6.0f); }
                    else if (tool == WeaponType::sprayer) { angle = reflect_angle(right, upper ? PI / 18.0f : 0.0f); }
                    else { angle = reflect_angle(right, 0.0f); }
                    const float roller_goal = reflected(right, upper ? 1840.0f : 1780.0f);
                    require(receiver_area(run, team, receiver, left, edge) == 0.0f, "Useful receiver starts prepainted");
                    for (int tick = 0; tick < 90; ++tick)
                    { run.tick(firing(angle, tool == WeaponType::roller ? steering(run, roller_goal) : 0.0f)); }
                    const float release_area = receiver_area(run, team, receiver, left, edge);
                    const int paid = run.paid_events, last_shot = run.last_paid_tick, departure = run.ticks;
                    require(paid > 0 && run.minimum_ammo < full,
                        "Useful receiver burst lacked actual paid fire");
                    drive(run, reflected(right, upper ? 1180.0f : 1020.0f));
                    const int retreat_ticks = run.ticks - departure;
                    const std::string retreat_support = run.support();
                    const float retreat_area = receiver_area(run, team, receiver, left, edge), arrival_ammo = run.player->weapon_ammo();
                    for (int tick = 0; tick < 180; ++tick) { run.tick(); }
                    const float final_area = receiver_area(run, team, receiver, left, edge);
                    require(final_area > 0.0f && final_area >= release_area && final_area >= retreat_area,
                        "Actual burst never painted its intended receiver top: " + std::string(tool_name(tool)) + " from " + source);
                    require(run.paid_events == paid && run.last_paid_tick == last_shot && !flying(run) &&
                        std::fabs(run.player->weapon_ammo() - full) < 0.0001f && supported(*run.player),
                        "Useful-role ordinary shot-free retreat/recovery did not finish with support/full reserve");
                    require(run.ticks <= 1400 && contact.ticks <= 180, "Useful-role fixture exceeded its finite step budget");
                    csv << (right ? 'a' : 'b') << ',' << tool_name(tool) << ',' << source << ',' << receiver << ',' << left << ',' << edge << ',' <<
                        contact.ticks << ',' << contact.paid << ',' << contact.launches << ',' << contact.before << ',' << contact.after << ',' <<
                        paid << ',' << run.emitted_objects << ',' << release_area << ',' << retreat_area << ',' << final_area << ',' <<
                        retreat_ticks << ',' << retreat_support << ',' << arrival_ammo << ',' << run.player->weapon_ammo() << ',' <<
                        run.ticks - last_shot << ',' << run.ticks << ",paired-first-contact-and-unoccupied-receiver-paint-retreat\n";
                    updates += run.ticks + 45 + contact.ticks; ++trials;
                    std::cout << "MEASURE Turbulence useful tool=" << tool_name(tool) << " team=" << (right ? 'a' : 'b') <<
                        " source=" << source << " first_health=" << contact.before << "->" << contact.after << " contact_ticks=" <<
                        contact.ticks << " receiver_area=" << final_area << " retreat_ticks=" << retreat_ticks << '\n';
                }
            }
        }
        csv.close(); require(csv.good() && trials == 20, "Useful receiver trial/receipt count differs");
        std::cout << "PASS 20 paired actual first-health-loss and intended-receiver-top painting/retreat/recovery controls; "
            "target trials end before elimination and are separate from unoccupied painting controls\n";
        return updates;
    }
}
