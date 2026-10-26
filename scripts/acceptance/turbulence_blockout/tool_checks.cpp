#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/tool_fixtures.h"

namespace turbulence_blockout_acceptance
{
    namespace
    {
        struct HeldPaint
        {
            double total = 0.0;
            std::array<double, 4> faces{};
            std::array<double, 6> roles{};
        };

        std::size_t object_role(const Surface& surface, const PaintSurfaceSample& tile)
        {
            if (surface.name == "ground")
            {
                const float x = (tile.corners[0].x + tile.corners[2].x) / 2.0f;
                return x < 600 ? 0 : x < 1100 ? 2 : x < 2900 ? 4 : x < 3400 ? 3 : 1;
            }
            if (surface.name.starts_with("home_")) { return surface.name.ends_with("_a") ? 0 : 1; }
            if (surface.name.starts_with("lower_bed")) { return 4; }
            if (surface.name.starts_with("crossover") || surface.name == "centre_baffle") { return 5; }
            return surface.name.ends_with("_a") ? 2 : 3;
        }

        HeldPaint paint(const Run& run, PlayerTeam team)
        {
            HeldPaint result;
            double authoritative = 0.0;
            for (const auto& [surface, object] : run.structures)
            {
                const auto* scoring = dynamic_cast<const PaintableObject*>(object);
                if (scoring == nullptr) { continue; }
                const PaintTotal total = scoring->paint_total();
                authoritative += team == PlayerTeam::a ? total.team_a : total.team_b;
                require((team == PlayerTeam::a ? total.team_b : total.team_a) == 0.0f,
                    "Single-team tool station credited an absent opposing painter");
                std::vector<PaintSurfaceSample> samples;
                scoring->append_paint_surfaces(samples);
                for (const auto& tile : samples)
                {
                    if (tile.team != team) { continue; }
                    result.total += tile.scored_area;
                    result.faces[static_cast<std::size_t>(tile.face)] += tile.scored_area;
                    result.roles[object_role(*surface, tile)] += tile.scored_area;
                }
            }
            require(std::fabs(result.total - authoritative) < 0.01,
                "Tool station snapshot differs from runtime held paint");
            double role_sum = 0.0;
            for (const double value : result.roles) { role_sum += value; }
            require(role_sum == result.total, "Tool station object-role budget duplicates or loses paint");
            return result;
        }

        void record(std::ofstream& csv, const Run& run, PlayerTeam team, WeaponType tool,
            const std::string& station, const char* phase)
        {
            const auto held = paint(run, team);
            csv << (team == PlayerTeam::a ? 'a' : 'b') << ',' << tool_name(tool) << ',' << station << ',' << phase << ',' <<
                run.ticks << ',' << run.paid_events << ',' << run.emitted_objects << ',' << run.player->weapon_ammo() << ',' <<
                run.minimum_ammo << ',' << run.ticks - run.last_paid_tick << ',' << run.player->center().x << ',' <<
                run.player->rectangle().bottom() << ',' << run.support() << ',' << held.total;
            for (const auto value : held.faces) { csv << ',' << value; }
            for (const auto value : held.roles) { csv << ',' << value; }
            csv << ",object-role-not-geographic-x-band,local-floor-paint-baseline-not-useful-contest-claim\n";
        }
    }

    int tool_stations(Services& services, const std::filesystem::path& output)
    {
        std::ofstream csv(output / "turbulence-tool-stations.csv");
        require(csv.good(), "Cannot create tool station receipt");
        csv << std::setprecision(std::numeric_limits<float>::max_digits10);
        csv << "team,tool,station,phase,tick,paid_events,surviving_launches,ammo,minimum_ammo,shot_free_ticks,centre_x,feet_y,"
            "support,held_area,top_area,bottom_area,left_area,right_area,home_a,home_b,approach_a,approach_b,lower_contest,"
            "upper_crossing,partition,scope\n";
        int updates = 0, trials = 0;
        for (const WeaponType tool : concrete_tools)
        {
            for (const bool right : { true, false })
            {
                const PlayerTeam team = right ? PlayerTeam::a : PlayerTeam::b;
                const float direction = right ? 1.0f : -1.0f;
                const float angle = right ? PI / 3.0f : 2.0f * PI / 3.0f;
                for (const bool upper : { false, true })
                {
                    const std::string station = std::string(upper ? "crossover" : "lower_bed") + (right ? "_a" : "_b");
                    Run run(services, standing(services, services.named(station), reflected(right, upper ? 1670.0f : 1420.0f)),
                        team, 0.0f, tool);
                    run.settle();
                    const float full = run.player->weapon_ammo();
                    for (int tick = 0; tick < 90; ++tick)
                    { run.tick(firing(angle, tick < 12 ? direction : 0.0f)); }
                    const float spent = run.player->weapon_ammo();
                    require(run.paid_events > 0 && run.minimum_ammo < full && paint(run, team).total > 0.0,
                        "Tool station did not pay for actual painting: " + std::string(tool_name(tool)) + " on " + station);
                    record(csv, run, team, tool, station, "burst-end");
                    const int last_shot = run.last_paid_tick, paid = run.paid_events;
                    const int leave = run.ticks;
                    drive(run, reflected(right, upper ? 1180.0f : 1020.0f));
                    require(run.paid_events == paid && run.last_paid_tick == last_shot && supported(*run.player),
                        "Shot-free retreat fired or lost all supported recovery space");
                    if (!upper) { support_is(run, "ground"); }
                    record(csv, run, team, tool, station, "retreat-arrival");
                    const float arrival_ammo = run.player->weapon_ammo();
                    for (int tick = 0; tick < 180; ++tick) { run.tick(); }
                    require(run.paid_events == paid && run.player->weapon_ammo() >= arrival_ammo &&
                        run.player->weapon_ammo() >= spent && std::fabs(run.player->weapon_ammo() - full) < 0.0001f,
                        "Natural shot-free tool recovery failed to refill the original reserve");
                    require(!flying(run), "Station left projectiles alive past the bounded release/recovery window");
                    record(csv, run, team, tool, station, "recovered");
                    updates += run.ticks; ++trials;
                    require(run.ticks <= 1400, "Tool station exceeded its finite retreat/recovery budget");
                    std::cout << "MEASURE Turbulence tool=" << tool_name(tool) << " team=" << (right ? 'a' : 'b') <<
                        " station=" << station << " paid=" << paid << " launches=" << run.emitted_objects <<
                        " retreat_ticks=" << run.ticks - 180 - leave << " arrival_ammo=" << arrival_ammo <<
                        " final_ammo=" << run.player->weapon_ammo() << " support=" << run.support() << '\n';
                }
            }
        }
        csv.close(); require(csv.good() && trials == 20, "Tool station receipt/trial inventory differs");
        std::cout << "PASS 20 mirrored five-tool lower/upper local-floor painting, ordinary retreat and natural recovery baselines\n";
        return updates;
    }

    void tools(Services& services, const std::filesystem::path& output)
    {
        const int station_updates = tool_stations(services, output);
        const int useful_updates = useful_stations(services, output);
        const ToolThreatSummary threats = tool_threats(services, output);
        require(station_updates <= 28000 && useful_updates <= 36000 && threats.updates <= 60000,
            "Tool matrix exceeded its finite update budget");
        std::ofstream summary(output / "turbulence-tools-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"turbulence-explicit-tools\",\"station_trials\":20,"
            "\"standing_holders\":40,\"directed_pulses\":280,\"station_updates\":" << station_updates <<
            ",\"useful_role_trials\":20,\"useful_role_updates\":" << useful_updates << ",\"threat_updates\":" << threats.updates <<
            ",\"single_pulse_both_departures\":" << threats.single_pulse_both_departures <<
            ",\"holder_union_both_departures\":" << threats.holder_union_both_departures <<
            ",\"human_acceptance\":false,\"useful_target_first_damage_measured\":true,\"threat_volume_damage_measured\":false}\n";
        summary.close(); require(summary.good(), "Cannot flush bounded tool summary");
        std::cout << "PASS explicit Turbulence --tools: actual target first-health loss, intended receiver painting and "
            "station recovery; directed threat volumes remain arrival samples without victim damage/universal safety/balance acceptance\n";
    }
}
