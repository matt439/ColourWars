#include "game/pch.h"
#include "game/play_settings.h"
#include "game/tuning.h"
#include "game/objects/paintable_object.h"
#include "game/objects/structure.h"
#include "game/objects/paint_tile.h"
#include "game/objects/team_colour.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/render/colour.h"

// Read the real scored tiles by face, as the Movement Course paint station
// does. Load dependencies normally before opening this one declaration.
#define private public
#include "game/objects/structure_paintable.h"
#undef private

#include "scripts/acceptance/crossfeed/fixtures.h"

namespace crossfeed_acceptance
{
    namespace
    {
        const char* tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed trajectory needs an aimed primary");
            }
        }

        std::array<float, 4> face_area(const Run& run, const std::string& name, PlayerTeam team)
        {
            std::array<float, 4> result{};
            const float thickness = run.services.tuning.paint().thickness;
            for (const auto& [surface, object] : run.structures)
            {
                if (surface->name != name) { continue; }
                const auto* scoring = dynamic_cast<const StructurePaintable*>(object);
                require(scoring != nullptr, "Crossfeed trajectory face target is not paintable");
                const RectangleF& r = surface->rectangle;
                for (const PaintTile& tile : scoring->paint_tiles_)
                {
                    if (tile.team() != team) { continue; }
                    const RectangleF b = tile.bounds();
                    if (approximately(b.height, thickness) && approximately(b.top(), r.top()))
                    { result[0] += tile.area(); }
                    else if (approximately(b.height, thickness) && approximately(b.bottom(), r.bottom()))
                    { result[1] += tile.area(); }
                    else if (approximately(b.width, thickness) && approximately(b.left(), r.left()))
                    { result[2] += tile.area(); }
                    else if (approximately(b.width, thickness) && approximately(b.right(), r.right()))
                    { result[3] += tile.area(); }
                    else { throw std::runtime_error("Crossfeed trajectory cannot classify an owned face tile"); }
                }
                return result;
            }
            throw std::runtime_error("Crossfeed trajectory face target is missing");
        }

        struct Flight
        {
            Projectile* object = nullptr;
            Vector2F origin;
            Vector2F previous;
            Vector2F integrated;
            Vector2F first_step;
            int launch_tick = 0;
            int first_step_tick = -1;
            int finish_tick = -1;
            bool retired_before_contacts = false;
            std::string outcome = "unresolved";
            std::string surface = "none";
            std::string face = "none";
            Vector2F contact;
            Vector2F finish;
            float travelled = 0.0f;
        };

        struct Touch
        {
            std::string surface;
            std::string face;
            Vector2F point;
        };

        std::optional<Touch> touching_surface(const Run& run, const Projectile& projectile)
        {
            // Swept contact leaves the projectile at the actual first impact,
            // with a small real penetration. Read that resolved geometry; an
            // integrated endpoint before the sweep would overstate distance.
            for (const auto& [surface, object] : run.structures)
            {
                if (surface->ramp()) { continue; }
                const auto overlap = narrow_phase(*object->shape(), *projectile.shape());
                if (!overlap) { continue; }
                const RectangleF& cover = surface->rectangle;
                const Vector2F centre = projectile.shape()->bounding_box().center();
                Vector2F point(std::clamp(centre.x, cover.left(), cover.right()),
                    std::clamp(centre.y, cover.top(), cover.bottom()));
                std::string face;
                if (std::fabs(overlap->normal.x) >= std::fabs(overlap->normal.y))
                {
                    face = overlap->normal.x > 0.0f ? "right" : "left";
                    point.x = overlap->normal.x > 0.0f ? cover.right() : cover.left();
                }
                else
                {
                    face = overlap->normal.y > 0.0f ? "bottom" : "top";
                    point.y = overlap->normal.y > 0.0f ? cover.bottom() : cover.top();
                }
                return Touch{ surface->name, face, point };
            }
            return std::nullopt;
        }

        void single_launch(Services& s, WeaponType type, PlayerTeam team, bool moving, bool underside)
        {
            const bool right = team == PlayerTeam::a;
            const float direction = right ? 1.0f : -1.0f;
            const RectangleF bounds = s.map.info.out_of_bounds_rectangle();
            const auto x = [&](float value) { return right ? value : bounds.left() + bounds.right() - value; };
            const Surface& support = s.map.named(underside ? "centre_plinth" :
                (right ? "floor_paint_a" : "floor_paint_b"));
            const float station = underside ? 1800.0f : 1250.0f;
            const Surface& target = s.map.named(underside ? "centre_mixer" : "centre_plinth");
            const Surface& start_floor = s.map.named(right ? "boundary_floor_a" : "boundary_floor_b");
            Run run(s, standing_on(s, moving ? start_floor : support, x(moving ? 100.0f : station)), team, type);
            run.settle();
            int approach_ticks = 0;
            if (moving)
            {
                while ((run.player->center().x - x(station)) * direction < 0.0f && approach_ticks < 600)
                {
                    run.tick(input(direction));
                    ++approach_ticks;
                }
                require(approach_ticks < 600 &&
                    approximately(run.player->velocity().x, direction * s.tuning.player().max_velocity.x),
                    "Crossfeed single-launch approach did not reach the station at full speed by input");
            }
            const float forward = type == WeaponType::bucket ? -0.4f :
                type == WeaponType::sprayer ? -0.25f : 0.0f;
            const float angle = underside ? -PI / 2.0f : (right ? forward : PI - forward);
            const float initial_ammo = run.player->weapon_ammo();
            std::vector<Flight> flights;
            int paid_tick = -1;
            float paid_ammo = -1.0f;
            run.on_weapon_launch = [&](std::span<const std::unique_ptr<Projectile>> emitted)
            {
                if (run.paid_events == 0 || paid_tick >= 0) { return; }
                paid_tick = run.ticks;
                paid_ammo = initial_ammo - run.player->weapon_ammo();
                for (const auto& object : emitted)
                {
                    Flight flight;
                    flight.object = object.get();
                    flight.origin = object->shape()->bounding_box().center();
                    flight.previous = flight.integrated = flight.finish = flight.origin;
                    flight.launch_tick = run.ticks;
                    flights.push_back(flight);
                }
            };
            const auto current = [&](const Projectile* projectile)
            {
                const auto objects = run.scene.collision_objects();
                return std::find(objects.begin(), objects.end(), projectile) != objects.end();
            };
            run.after_scene_update = [&]
            {
                for (Flight& flight : flights)
                {
                    if (flight.object == nullptr || !current(flight.object)) { continue; }
                    flight.integrated = flight.object->shape()->bounding_box().center();
                    flight.retired_before_contacts = flight.object->for_deletion();
                    if (flight.first_step_tick < 0)
                    {
                        flight.first_step_tick = run.ticks;
                        flight.first_step = (flight.integrated - flight.origin) / dt;
                    }
                }
            };
            run.after_scene_contacts = [&]
            {
                for (Flight& flight : flights)
                {
                    // Launch survivors are pending until the launch tick ends.
                    // Every later borrowed pointer must still be in the Scene
                    // before it is read; clear it before end_tick can delete it.
                    if (flight.object == nullptr || !current(flight.object)) { continue; }
                    const Vector2F resolved = flight.object->shape()->bounding_box().center();
                    flight.travelled += (resolved - flight.previous).length();
                    flight.previous = flight.finish = resolved;
                    const bool outside = !run.scene.in_bounds(*flight.object);
                    if (!flight.object->for_deletion() && !outside) { continue; }
                    flight.finish_tick = run.ticks;
                    if (outside) { flight.outcome = "world-bound-exit"; }
                    else if (flight.retired_before_contacts) { flight.outcome = "lifetime"; }
                    else if (const auto touch = touching_surface(run, *flight.object))
                    {
                        flight.outcome = "resolved-structure-contact";
                        flight.surface = touch->surface;
                        flight.face = touch->face;
                        flight.contact = touch->point;
                    }
                    else { flight.outcome = "retired-without-identified-structure"; }
                    flight.object = nullptr;
                }
            };

            for (int tick = 0; tick < 480; ++tick)
            {
                PlayerInputData actions = aimed(angle, input(paid_tick < 0 && moving ? direction : 0.0f));
                actions.primary_shoot = paid_tick < 0;
                run.tick(actions);
                const bool finished = paid_tick >= 0 && std::all_of(flights.begin(), flights.end(),
                    [](const Flight& flight) { return flight.object == nullptr; });
                if (finished) { break; }
            }
            run.on_weapon_launch = {};
            run.after_scene_update = {};
            run.after_scene_contacts = {};
            require(run.paid_events == 1 && paid_tick >= 0 && paid_ammo > 0.0f,
                "Crossfeed trajectory did not release after exactly one paid event");
            require(std::all_of(flights.begin(), flights.end(),
                [](const Flight& flight) { return flight.object == nullptr; }),
                "Crossfeed trajectory observation ended with an unresolved launched survivor");
            const auto faces = face_area(run, target.name, team);
            const char* label = underside ? "mixer-underside" : "lower-plinth-cover";
            std::cout << "MEASURE Crossfeed single_launch tool=" << tool_name(type) <<
                " team=" << (right ? 'a' : 'b') << " station=" << label <<
                " approach=" << (moving ? "actual-full-speed-input-then-release" : "stationary") <<
                " approach_ticks=" << approach_ticks << " aim=" << fixed(angle, 4) <<
                " first_paid_owner_vx=" << fixed(run.first_paid_velocity.x) <<
                " first_paid_owner_vy=" << fixed(run.first_paid_velocity.y) <<
                " first_paid_nozzle_x=" << fixed(run.first_paid_nozzle.x) <<
                " first_paid_nozzle_y=" << fixed(run.first_paid_nozzle.y) <<
                " paid_events=" << run.paid_events << " paid_ammo=" << fixed(paid_ammo, 4) <<
                " launch_survivors=" << flights.size() << " launch_retired_objects=" << run.launch_retired_objects <<
                " launch_paint=" << fixed(run.launch_paint) << " swept_paint=" << fixed(run.swept_paint) <<
                " target_top_area=" << fixed(faces[0]) << " target_bottom_area=" << fixed(faces[1]) <<
                " target_left_area=" << fixed(faces[2]) << " target_right_area=" << fixed(faces[3]) << '\n';
            for (size_t index = 0; index < flights.size(); ++index)
            {
                const Flight& flight = flights[index];
                std::cout << "MEASURE Crossfeed tracked_flight tool=" << tool_name(type) <<
                    " team=" << (right ? 'a' : 'b') << " station=" << label <<
                    " approach=" << (moving ? "actual-full-speed-input-then-release" : "stationary") <<
                    " launch_object=" << index << " flight_ticks=" << flight.finish_tick - flight.launch_tick <<
                    " flight_seconds=" << fixed(static_cast<float>(flight.finish_tick - flight.launch_tick) * dt, 4) <<
                    " first_step_vx=" << fixed(flight.first_step.x) << " first_step_vy=" << fixed(flight.first_step.y) <<
                    " origin_x=" << fixed(flight.origin.x) << " origin_y=" << fixed(flight.origin.y) <<
                    " finish_x=" << fixed(flight.finish.x) << " finish_y=" << fixed(flight.finish.y) <<
                    " net_distance=" << fixed((flight.finish - flight.origin).length()) <<
                    " path_distance=" << fixed(flight.travelled) << " outcome=" << flight.outcome <<
                    " contact_surface=" << flight.surface << " contact_face=" << flight.face <<
                    " contact_x=" << fixed(flight.contact.x) << " contact_y=" << fixed(flight.contact.y) <<
                    " scope=one-paid-launch-survivor-identity-through-actual-first-retirement\n";
            }
            if (underside)
            {
                require(faces[1] > 0.0f, "Crossfeed single paid aimed launch did not paint the mixer underside");
            }
            else
            {
                require(std::any_of(flights.begin(), flights.end(), [](const Flight& flight)
                    { return flight.surface == "centre_plinth"; }),
                    "Crossfeed single paid lower launch did not reach the actual solid plinth by flight");
            }
        }
    }

    void trajectory_checks(Services& s)
    {
        std::cout << "NOTE Crossfeed tracked trajectories: exactly one paid event per station; each returned "
            "launch survivor retains its own identity through first retirement. Launch-contact retirements are "
            "counted separately because Player removes them before returning survivors. Full speed is reached "
            "through real input before firing, then movement and fire release. Distances stop at authored cover, "
            "lifetime or world bounds; they are not unobstructed maximum ranges or human counterplay\n";
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::mister, WeaponType::bucket })
        {
            for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
            {
                for (const bool moving : { false, true })
                {
                    for (const bool underside : { false, true })
                    { single_launch(s, type, team, moving, underside); }
                }
            }
        }
        std::cout << "PASS Crossfeed tracked single-launch trajectories and actual mixer face paint, "
            "with mirrored stationary/full-speed launches and paid launch-retirement accounting\n";
    }
}
