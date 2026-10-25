#include "game/pch.h"
#include "game/objects/collision_object_type.h"
#include "scripts/acceptance/press_tower/fixtures.h"
#include <algorithm>
#include <limits>
#include <unordered_map>

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
            default: throw std::runtime_error("Press Tower live fire needs a concrete primary");
            }
        }

        // The whole home span the four spawn bodies of one team occupy.
        RectangleF spawn_span(const Services& s, bool team_a)
        {
            const auto points = team_a ? s.map.info.team_a_spawns() : s.map.info.team_b_spawns();
            const Vector2F size = s.tuning.player().size;
            float left = points.front().x, top = points.front().y, right = left + size.x, bottom = top + size.y;
            for (const Vector2F& point : points)
            {
                left = std::min(left, point.x);
                top = std::min(top, point.y);
                right = std::max(right, point.x + size.x);
                bottom = std::max(bottom, point.y + size.y);
            }
            return RectangleF::from_top_left_bottom_right({ left, top }, { right, bottom });
        }

        float gap_between(const RectangleF& a, const RectangleF& b)
        {
            const float dx = std::max({ 0.0f, b.left() - a.right(), a.left() - b.right() });
            const float dy = std::max({ 0.0f, b.top() - a.bottom(), a.top() - b.bottom() });
            return std::sqrt(dx * dx + dy * dy);
        }

        struct Flight
        {
            Vector2F centre;
            Vector2F half;
        };

        struct Volley
        {
            int launches = 0;
            // Launch centres with an unobstructed straight point line to the
            // span: what a fixed-muzzle ray model would call a sightline.
            int point_lines = 0;
            int reached = 0;
            float closest = std::numeric_limits<float>::max();
            std::string first_reach;
        };

        // One standing holder sweeps its aim through every direction that
        // faces the home it overlooks, one paid shot per aim, and every real
        // projectile is followed tick by tick until the Scene retires it. A
        // flight reaches the span when its swept box touches it, launch
        // segment included; nozzle, box size, gravity and drag are the game's.
        Volley sweep(Services& s, WeaponType type, bool team_a, const Surface& support, float centre,
            const RectangleF& span, const std::vector<RectangleF>& blockers, float step_degrees)
        {
            const PlayerTeam team = team_a ? PlayerTeam::a : PlayerTeam::b;
            Run run(s, standing_on(s, support, centre), team, type);
            run.settle();
            run.player->set_infinite_ammo(true);
            Volley volley;
            std::unordered_map<const CollisionObject*, Flight> flights;
            std::vector<Vector2F> span_points;
            for (float x = span.left(); x <= span.right() + 0.001f; x += 8.0f)
            {
                for (float y = span.top() + 2.0f; y <= span.bottom() - 2.0f + 0.001f; y += 12.0f) { span_points.emplace_back(x, y); }
            }
            const auto clear = [&](const Vector2F& from, const Vector2F& to)
            {
                return std::none_of(blockers.begin(), blockers.end(),
                    [&](const RectangleF& r) { return segment_hits(from, to, r); });
            };
            run.on_weapon_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted)
            {
                for (const auto& object : emitted)
                {
                    const RectangleF box = object->shape()->bounding_box();
                    flights[object.get()] = { box.center(), box.size() / 2.0f };
                    ++volley.launches;
                    volley.point_lines += std::any_of(span_points.begin(), span_points.end(),
                        [&](const Vector2F& point) { return clear(box.center(), point); }) ? 1 : 0;
                }
            };
            run.after_scene_contacts = [&]
            {
                for (CollisionObject* object : run.scene.collision_objects())
                {
                    if (!is_projectile(to_collision_type(object->tag()))) { continue; }
                    const auto found = flights.find(object);
                    require(found != flights.end(), "Press Tower live fire lost a projectile's launch");
                    const RectangleF box = object->shape()->bounding_box();
                    const Vector2F half(std::max(found->second.half.x, box.width / 2.0f),
                        std::max(found->second.half.y, box.height / 2.0f));
                    const RectangleF reach(span.left() - half.x, span.top() - half.y,
                        span.width + 2.0f * half.x, span.height + 2.0f * half.y);
                    if (segment_hits(found->second.centre, box.center(), reach) ||
                        narrow_phase(box, span).has_value())
                    {
                        if (volley.reached++ == 0)
                        {
                            volley.first_reach = '(' + fixed(found->second.centre.x) + ',' + fixed(found->second.centre.y) +
                                ")->(" + fixed(box.center().x) + ',' + fixed(box.center().y) + ')';
                        }
                    }
                    volley.closest = std::min(volley.closest, gap_between(box, span));
                    found->second = { box.center(), box.size() / 2.0f };
                }
            };
            const float interval = s.tuning.weapon(type).details.shoot_interval;
            const int hold = std::max(1, static_cast<int>(std::ceil(interval / dt)) + 1);
            // Team A overlooks the west home: down, through west, to up.
            const float base = team_a ? PI / 2.0f : -PI / 2.0f;
            const int steps = static_cast<int>(std::lround(180.0f / step_degrees));
            for (int step = 0; step <= steps; ++step)
            {
                const float angle = base + (team_a ? 1.0f : -1.0f) * step * step_degrees * PI / 180.0f;
                for (int tick = 0; tick < hold; ++tick)
                {
                    PlayerInputData actions = aimed(angle);
                    actions.primary_shoot = true;
                    run.tick(actions);
                }
            }
            const auto airborne = [&]
            {
                const auto objects = run.scene.collision_objects();
                return std::any_of(objects.begin(), objects.end(),
                    [](const CollisionObject* object) { return is_projectile(to_collision_type(object->tag())); });
            };
            for (int tick = 0; tick < 600 && airborne(); ++tick) { run.tick(); }
            require(!airborne(), "Press Tower live fire left a projectile in flight after ten seconds");
            return volley;
        }
    }

    void live_fire_checks(Services& s)
    {
        std::vector<RectangleF> blockers;
        for (const Surface& surface : s.map.surfaces)
        {
            if (!surface.ramp()) { blockers.push_back(surface.rectangle); }
        }
        const Surface& head = s.map.named("press_head");
        const Surface& crown = s.map.named("press_crown");
        int volleys = 0;
        for (const bool team_a : { true, false })
        {
            const auto x = [&](float value) { return reflected_x(s, team_a, value); };
            const auto side = [&](const std::string& base) -> const Surface& { return s.map.named(base + (team_a ? "_a" : "_b")); };
            const RectangleF span = spawn_span(s, team_a);
            // Both ends and the middle of every high standing span; the
            // summit's far half too, since it overlooks the same home.
            struct Holder { std::string name; const Surface& support; std::vector<float> centres; };
            const std::vector<Holder> holders{
                { "stair_3", side("stair_3"), { x(1346.0f), x(1400.0f), x(1454.0f) } },
                { "shoulder", side("shoulder"), { x(1506.0f), x(1560.0f), x(1614.0f) } },
                { "summit_near", head, { x(1666.0f), x(1710.0f), x(1754.0f) } },
                { "crown", crown, { 1800.0f } },
                { "summit_far", head, { x(1846.0f), x(1890.0f), x(1934.0f) } },
            };
            for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
                WeaponType::mister, WeaponType::bucket })
            {
                // One degree keeps consecutive jet boxes overlapping across
                // the whole stage; the slower tools are swept at two.
                const float step = type == WeaponType::sniper || type == WeaponType::sprayer ? 1.0f : 2.0f;
                for (const Holder& holder : holders)
                {
                    Volley total;
                    for (const float centre : holder.centres)
                    {
                        const Volley volley = sweep(s, type, team_a, holder.support, centre, span, blockers, step);
                        require(volley.reached == 0, std::string("Press Tower ") + tool_name(type) + " fired from " +
                            holder.name + " at x=" + fixed(centre) + " reaches the team " + (team_a ? "A" : "B") +
                            " spawn span: " + volley.first_reach);
                        total.launches += volley.launches;
                        total.point_lines += volley.point_lines;
                        total.closest = std::min(total.closest, volley.closest);
                        ++volleys;
                    }
                    // A centre beside a riser can lose every throw to launch
                    // obstruction (the bucket's 100 box); the holder cannot.
                    require(total.launches > 0, std::string("Press Tower live fire never launched a ") +
                        tool_name(type) + " from " + holder.name);
                    std::cout << "MEASURE Press Tower live_fire team=" << (team_a ? 'a' : 'b') << " tool=" <<
                        tool_name(type) << " holder=" << holder.name << " launches=" << total.launches <<
                        " straight_point_lines=" << total.point_lines << " reached=0 closest_gap=" <<
                        fixed(total.closest) << '\n';
                }
            }
        }
        std::cout << "PASS Press Tower live fire: " << volleys << " standing aim sweeps of all five tools from "
            "stair_3, shoulder, both summit halves and the crown; no real projectile's swept box reaches its "
            "overlooked spawn span\n";
    }
}
