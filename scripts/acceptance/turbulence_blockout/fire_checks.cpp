#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/tool_fixtures.h"
#include <optional>
#include <unordered_map>

namespace turbulence_blockout_acceptance
{
    namespace
    {
        struct Target { std::string name; RectangleF rectangle; };
        struct DirectedPulse
        {
            int paid_tick = 0;
            int launches = 0;
            float angle = 0.0f;
            std::array<int, 7> first_tick{ -1, -1, -1, -1, -1, -1, -1 };
        };
        struct Flight { RectangleF previous; std::size_t pulse = 0; };

        // Linear inequalities for the four actual rectangle edges. Unlike a
        // maximum-radius segment, this retains diffusion/growth between the
        // before/after boxes without inflating the whole preceding path.
        std::optional<double> enters(const RectangleF& before, const RectangleF& after, const RectangleF& target)
        {
            const std::array<std::pair<double, double>, 4> inequalities{ {
                { static_cast<double>(before.right()) - target.left(), static_cast<double>(after.right()) - target.left() },
                { static_cast<double>(target.right()) - before.left(), static_cast<double>(target.right()) - after.left() },
                { static_cast<double>(before.bottom()) - target.top(), static_cast<double>(after.bottom()) - target.top() },
                { static_cast<double>(target.bottom()) - before.top(), static_cast<double>(target.bottom()) - after.top() }
            } };
            double begin = 0.0, end = 1.0;
            for (const auto& [first, last] : inequalities)
            {
                const double delta = last - first;
                if (delta == 0.0) { if (first < 0.0) { return std::nullopt; } continue; }
                const double crossing = -first / delta;
                if (delta > 0.0) { begin = std::max(begin, crossing); }
                else { end = std::min(end, crossing); }
                if (begin > end) { return std::nullopt; }
            }
            return begin;
        }

        RectangleF mirrored(bool target_a, RectangleF rectangle)
        { if (!target_a) { rectangle.x = 4000.0f - rectangle.right(); } return rectangle; }

        std::array<Target, 7> targets(const Services& services, bool target_a)
        {
            std::array<Target, 7> result;
            const auto spawns = target_a ? services.info.team_a_spawns() : services.info.team_b_spawns();
            require(spawns.size() == 4, "Directed arrival matrix requires exactly four authored spawn bodies");
            for (std::size_t index = 0; index < spawns.size(); ++index)
            { result[index] = { "spawn_" + std::to_string(index + 1), RectangleF(spawns[index], services.tuning.player().size) }; }
            result[4] = { "lower_departure", mirrored(target_a, { 500, 1160, 100, 120 }) };
            result[5] = { "upper_departure", mirrored(target_a, { 650, 920, 100, 120 }) };
            const auto& receiver = services.named(target_a ? "upper_approach_b" : "upper_approach_a").rectangle;
            result[6] = { "opposite_receiver", { receiver.x, receiver.top() - services.tuning.player().size.y,
                receiver.width, services.tuning.player().size.y } };
            return result;
        }

        void check_sweep_oracle()
        {
            require(enters({ 0, 0, 10, 10 }, { 100, 0, 10, 10 }, { 50, 2, 2, 6 }).has_value(),
                "Moving-box observer missed an entered target");
            require(!enters({ 0, 0, 10, 10 }, { 100, 0, 10, 10 }, { 50, 11, 2, 6 }),
                "Moving-box observer invented vertical reach");
            const auto growth = enters({ 0, 0, 10, 10 }, { -10, -10, 30, 30 }, { 15, 1, 1, 1 });
            require(growth && std::fabs(*growth - 0.5) < 1e-12,
                "Growing-box observer disagrees with analytical edge entry");
            require(!enters({ 0, 0, 10, 10 }, { 40, 0, 10, 10 }, { 60, 0, 5, 10 }),
                "Clipped flight observer counted travel beyond its actual endpoint");
        }
    }

    ToolThreatSummary tool_threats(Services& services, const std::filesystem::path& output)
    {
        check_sweep_oracle();
        std::ofstream csv(output / "turbulence-directed-threats.csv"), holders(output / "turbulence-threat-holders.csv");
        require(csv.good() && holders.good(), "Cannot create directed threat receipts");
        csv << std::setprecision(std::numeric_limits<float>::max_digits10);
        csv << "target_team,attacker_team,tool,holder,holder_x,holder_feet,aim_target,angle,paid_events,"
            "surviving_launches,aimed_target_arrived,aimed_first_tick,spawn_arrival_mask,lower_departure_arrived,"
            "upper_departure_arrived,both_departures_same_pulse,opposite_receiver_arrived,scope\n";
        holders << "target_team,tool,holder,paid_pulses,surviving_launches,spawn_arrival_mask,lower_departure_arrived,"
            "upper_departure_arrived,both_departures_sampled,single_pulse_both_departures,opposite_receiver_arrived,updates,scope\n";
        int total_updates = 0, holder_count = 0, pulse_count = 0;
        ToolThreatSummary summary;
        std::array<int, 5> tool_launches{};
        for (std::size_t tool_index = 0; tool_index < concrete_tools.size(); ++tool_index)
        {
            const WeaponType tool = concrete_tools[tool_index];
            for (const bool target_a : { true, false })
            {
                const auto volumes = targets(services, target_a);
                const PlayerTeam attacker = target_a ? PlayerTeam::b : PlayerTeam::a;
                struct Holder { const char* name; float centre; };
                constexpr std::array<Holder, 4> places{ {
                    { "upper_approach", 1060 }, { "crossover", 1670 }, { "centre_baffle", 2000 }, { "lower_bed", 1420 }
                } };
                for (const Holder& holder : places)
                {
                    const std::string support = std::string(holder.name) + (std::string_view(holder.name) == "centre_baffle" ?
                        "" : target_a ? "_a" : "_b");
                    Run run(services, standing(services, services.named(support), reflected(target_a, holder.centre)),
                        attacker, 0.0f, tool);
                    run.settle();
                    std::array<DirectedPulse, 7> pulses;
                    std::unordered_map<const CollisionObject*, Flight> flights;
                    std::size_t current = 0;
                    const auto observe = [&](Flight& flight, const RectangleF& box)
                    {
                        DirectedPulse& pulse = pulses[flight.pulse];
                        for (std::size_t target = 0; target < volumes.size(); ++target)
                        {
                            if (pulse.first_tick[target] < 0 && enters(flight.previous, box, volumes[target].rectangle))
                            { pulse.first_tick[target] = run.ticks - pulse.paid_tick; }
                        }
                        flight.previous = box;
                    };
                    run.on_weapon_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted)
                    {
                        for (const auto& projectile : emitted)
                        {
                            const RectangleF box = projectile->shape()->bounding_box();
                            auto [where, inserted] = flights.insert_or_assign(projectile.get(), Flight{ box, current });
                            (void)inserted;
                            ++pulses[current].launches;
                            observe(where->second, box); // Actual surviving launch footprint, before the next Scene tick.
                        }
                    };
                    run.after_scene_contacts = [&]
                    {
                        for (CollisionObject* object : run.scene.collision_objects())
                        {
                            if (!is_projectile(to_collision_type(object->tag()))) { continue; }
                            const auto found = flights.find(object);
                            require(found != flights.end(), "Threat observer lost actual launch provenance");
                            observe(found->second, object->shape()->bounding_box());
                        }
                    };
                    for (; current < pulses.size(); ++current)
                    {
                        int ready_wait = 0;
                        while (run.player->primary_weapon().readiness() != WeaponReadiness::ready && ready_wait++ < 240)
                        { run.tick(); }
                        require(run.player->primary_weapon().readiness() == WeaponReadiness::ready,
                            "Directed pulse did not become affordable/cooldown ready");
                        const Vector2F direction = volumes[current].rectangle.center() - run.player->center();
                        pulses[current].angle = direction.angle();
                        pulses[current].paid_tick = run.ticks + 1;
                        const int paid = run.paid_events;
                        run.tick(firing(pulses[current].angle));
                        require(run.paid_events == paid + 1, "A single directed trigger frame did not pay exactly once");
                        run.tick(); // Release before another directed pulse, including zero-interval tools.
                    }
                    for (int tick = 0; flying(run) && tick < 600; ++tick) { run.tick(); }
                    require(!flying(run), "Directed threat flight exceeded its finite ten-second tail");
                    require(run.paid_events == 7 && run.ticks <= 1500, "Directed holder exceeded pulse/update budget");
                    int spawn_mask = 0, launches = 0, both_departure_pulses = 0;
                    std::array<bool, 7> union_arrivals{};
                    for (std::size_t aim = 0; aim < pulses.size(); ++aim)
                    {
                        const DirectedPulse& pulse = pulses[aim];
                        int mask = 0;
                        for (std::size_t target = 0; target < volumes.size(); ++target)
                        {
                            union_arrivals[target] = union_arrivals[target] || pulse.first_tick[target] >= 0;
                            if (target < 4 && pulse.first_tick[target] >= 0) { mask |= 1 << target; }
                        }
                        spawn_mask |= mask; launches += pulse.launches;
                        // One paid trigger event may emit several projectiles.
                        // This joins its arrivals, not their contact times or damage.
                        const bool both_same_pulse = pulse.first_tick[4] >= 0 && pulse.first_tick[5] >= 0;
                        both_departure_pulses += both_same_pulse ? 1 : 0;
                        csv << (target_a ? 'a' : 'b') << ',' << (target_a ? 'b' : 'a') << ',' << tool_name(tool) << ',' <<
                            support << ',' << run.player->center().x << ',' << run.player->rectangle().bottom() << ',' <<
                            volumes[aim].name << ',' << pulse.angle << ",1," << pulse.launches << ',' <<
                            (pulse.first_tick[aim] >= 0) << ',' << pulse.first_tick[aim] << ',' << mask << ',' <<
                            (pulse.first_tick[4] >= 0) << ',' << (pulse.first_tick[5] >= 0) << ',' << both_same_pulse << ',' <<
                            (pulse.first_tick[6] >= 0) <<
                            ",sampled-moving-box-arrival-no-victim-damage\n";
                        ++pulse_count;
                    }
                    holders << (target_a ? 'a' : 'b') << ',' << tool_name(tool) << ',' << support << ",7," << launches << ',' <<
                        spawn_mask << ',' << union_arrivals[4] << ',' << union_arrivals[5] << ',' <<
                        (union_arrivals[4] && union_arrivals[5]) << ',' << both_departure_pulses << ',' << union_arrivals[6] << ',' << run.ticks <<
                        ",seven-directed-aims-no-universal-safety-claim\n";
                    std::cout << "MEASURE Turbulence threat target_team=" << (target_a ? 'a' : 'b') << " tool=" <<
                        tool_name(tool) << " holder=" << support << " launches=" << launches << " spawn_mask=" << spawn_mask <<
                        " lower=" << union_arrivals[4] << " upper=" << union_arrivals[5] << " receiver=" << union_arrivals[6] << '\n';
                    tool_launches[tool_index] += launches;
                    summary.single_pulse_both_departures += both_departure_pulses;
                    summary.holder_union_both_departures += union_arrivals[4] && union_arrivals[5] ? 1 : 0;
                    total_updates += run.ticks; ++holder_count;
                }
            }
        }
        for (const int launches : tool_launches)
        { require(launches > 0, "A concrete tool produced no surviving launch across any sampled holder"); }
        csv.close(); holders.close();
        require(csv.good() && holders.good() && holder_count == 40 && pulse_count == 280,
            "Directed threat receipt/count inventory differs");
        std::cout << "PASS 280 actual paid directed pulses from 40 standing holders: launch/movement/contact boxes observed "
            "before retirement; arrival masks are sampled geometry threats, not victim damage or universal spawn safety\n";
        summary.updates = total_updates;
        return summary;
    }
}
