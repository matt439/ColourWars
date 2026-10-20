#include "game/pch.h"
#include "scripts/acceptance/movement/fixtures.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/team_colour.h"
#include "engine/collision/narrow_phase.h"

#include <iomanip>
#include <sstream>

// Authored legacy climbs constrain a global jump change too. Each trial builds
// the complete collision scene and constructs one Player on its actual lower
// surface. No placement after construction, injected floor or reduced geometry.
namespace movement_acceptance
{
    namespace
    {
        constexpr float position_tolerance = 0.1f;

        struct Route
        {
            const char* stage;
            const char* lower;
            const char* target;
            float rise;
        };

        bool supported(const Player& player)
        {
            return player.move_state() == PlayerMoveState::on_ground ||
                player.move_state() == PlayerMoveState::on_drop_down_ground ||
                player.move_state() == PlayerMoveState::on_ramp_left ||
                player.move_state() == PlayerMoveState::on_ramp_right;
        }

        struct LegacyRun
        {
            struct NamedObject
            {
                std::string name;
                CollisionObject* object;
            };

            Scene scene{ nullptr, nullptr };
            std::vector<NamedObject> objects;
            Player* player = nullptr;
            size_t takeoffs = 0;
            int ticks = 0;
            std::string context;

            CollisionObject& named(const char* name) const
            {
                CollisionObject* found = nullptr;
                for (const NamedObject& entry : this->objects)
                {
                    if (entry.name != name) { continue; }
                    require(found == nullptr, (this->context + ": duplicate route object " + name).c_str());
                    found = entry.object;
                }
                require(found != nullptr, (this->context + ": missing route object " + name).c_str());
                return *found;
            }

            LegacyRun(const Suite& suite, const Route& route) :
                context(std::string(route.stage) + " " + route.lower + " -> " + route.target)
            {
                const LevelLoadedInfo level((suite.content / "levels" /
                    (std::string(route.stage) + ".json")).string().c_str());
                level.validate();
                this->scene.set_bounds(level.out_of_bounds_rectangle());
                const LevelObjectBuilder builder(&suite.render, &suite.audio, &suite.preferences,
                    &suite.tuning.paint());
                const TeamColour colours;
                const JsonValue authored = level.collision_objects_json();
                auto built = builder.build_collision_objects(authored, colours);
                require(built.size() == authored.size(), "Legacy builder changed the authored object count");
                for (size_t index = 0; index < built.size(); ++index)
                {
                    this->objects.push_back({ authored.at(index).string("name"),
                        this->scene.add(std::move(built[index])) });
                }
                const CollisionObject& lower_object = this->named(route.lower);
                const CollisionObject& target_object = this->named(route.target);
                const RectangleF lower = lower_object.shape()->bounding_box();
                const RectangleF target = target_object.shape()->bounding_box();
                require(to_collision_type(target_object.tag()) == CollisionObjectType::structure_jump_through,
                    (this->context + ": target is no longer an authored one-way platform").c_str());
                require(std::fabs(lower.top() - target.top() - route.rise) <= position_tolerance,
                    (this->context + ": authored rise changed").c_str());
                const float left = std::max(lower.left(), target.left());
                const float right = std::min(lower.right(), target.right());
                require(left <= right, (this->context + ": route footprints no longer meet").c_str());
                // Two upper Turbulence pairs share only an edge. Centring the
                // body on that edge leaves half supported and half below the
                // target; it is an actual authored overhang, not a new floor.
                const float centre = left + (right - left) / 2.0f;
                const Vector2F size = suite.tuning.player().size;
                const RectangleF body(centre - size.x / 2.0f, lower.top() - size.y, size.x, size.y);
                require(body.left() < lower.right() && body.right() > lower.left() &&
                    body.left() < target.right() && body.right() > target.left(),
                    (this->context + ": constructed body does not overlap both route footprints").c_str());
                auto weapon = WeaponBuilder::build_weapon(&suite.tuning, PlayerTeam::a, 0, 0,
                    Colour::white, WeaponType::sprayer, body.center(), &suite.render, &suite.audio,
                    &suite.preferences);
                this->player = this->scene.add(std::make_unique<Player>(body, &suite.tuning,
                    &suite.feedback, &suite.presentation.combat_feedback(),
                    &suite.presentation.respawn_redraw(), &suite.render, &suite.audio,
                    &suite.preferences, 0, PlayerTeam::a, WeaponType::sprayer, std::move(weapon),
                    WeaponType::none, Colour::white, Viewport{}, body.position()));
                this->scene.end_tick();
                for (int tick = 0; tick < 8; ++tick) { this->tick(); }
                require(supported(*this->player) && this->player->velocity() == Vector2F::ZERO &&
                    std::fabs(this->player->rectangle().bottom() - lower.top()) <= position_tolerance,
                    (this->context + ": player did not settle on its actual lower surface").c_str());
                require(this->takeoffs == 0, (this->context + ": settling produced a takeoff").c_str());
            }

            void tick(const PlayerInputData& actions = input())
            {
                ++this->ticks;
                this->player->set_player_input(actions);
                this->scene.update(dt);
                const auto shots = this->player->update_weapon_and_get_projectiles(dt,
                    this->scene.collision_objects());
                require(shots.empty(), (this->context + ": a non-firing route emitted a projectile").c_str());
                this->scene.resolve();
                this->player->end_contacts(this->scene.contacts());
                this->player->update_weapon_position();
                this->player->update_prev_rectangle();
                this->scene.end_tick();
                require(this->player->state() == PlayerState::alive && this->scene.in_bounds(*this->player),
                    (this->context + ": route died or left the authored bounds").c_str());
                const RectangleF& body = this->player->rectangle();
                require(std::isfinite(body.x) && std::isfinite(body.y),
                    (this->context + ": movement became non-finite").c_str());
                for (const NamedObject& entry : this->objects)
                {
                    if (to_collision_type(entry.object->tag()) == CollisionObjectType::structure_jump_through)
                    {
                        continue; // The real Player intentionally passes upward through these.
                    }
                    const auto overlap = narrow_phase(*this->player->shape(), *entry.object->shape());
                    require(!overlap || overlap->penetration <= position_tolerance,
                        (this->context + ": intersects authored solid " + entry.name +
                            " at tick " + std::to_string(this->ticks)).c_str());
                }
                for (const auto& effect : this->player->movement_feedback().effects())
                {
                    if (effect.age == 0.0f && effect.kind == MovementFeedbackKind::takeoff) { ++this->takeoffs; }
                }
            }

            std::string support_name() const
            {
                if (!supported(*this->player)) { return "none"; }
                const RectangleF& body = this->player->rectangle();
                for (const NamedObject& entry : this->objects)
                {
                    const RectangleF surface = entry.object->shape()->bounding_box();
                    if (body.left() < surface.right() && body.right() > surface.left() &&
                        std::fabs(body.bottom() - surface.top()) <= position_tolerance)
                    {
                        return entry.name;
                    }
                }
                return "unidentified";
            }
        };
    }

    void check_legacy_routes(const Suite& suite)
    {
        const Route routes[]{
            { "king_of_the_hill", "castle_floor_left", "castle_platform_left", 200.0f },
            { "king_of_the_hill", "castle_floor_right", "castle_platform_right", 200.0f },
            { "turbulence", "boundary_floor", "left_bottom_left_jump", 200.0f },
            { "turbulence", "boundary_floor", "left_bottom_right_jump", 200.0f },
            { "turbulence", "boundary_floor", "right_bottom_left_jump", 200.0f },
            { "turbulence", "boundary_floor", "right_bottom_right_jump", 200.0f },
            { "turbulence", "left_lower_paint", "left_mid_left_jump", 200.0f },
            { "turbulence", "left_lower_paint", "left_mid_right_jump", 200.0f },
            { "turbulence", "right_lower_paint", "right_mid_left_jump", 200.0f },
            { "turbulence", "right_lower_paint", "right_mid_right_jump", 200.0f },
            { "turbulence", "central_divider_ledge_paint", "upper_jump_mid", 200.0f },
            { "turbulence", "upper_left", "upper_jump_left", 200.0f },
            { "turbulence", "upper_right", "upper_jump_right", 200.0f },
            { "king_of_the_hill", "mountain_level_11", "castle_entrance", 170.0f },
            { "king_of_the_hill", "castle_platform_mid", "castle_room_entrance", 175.0f },
        };
        size_t failures = 0;
        size_t failed_200 = 0;
        std::string failed_routes;
        for (const Route& route : routes)
        {
            LegacyRun run(suite, route);
            const float start_feet = run.player->rectangle().bottom();
            float peak_rise = 0.0f;
            std::string first_landing = "none";
            int landing_tick = -1;
            for (int tick = 0; tick < 180; ++tick)
            {
                run.tick(input(tick == 0, true));
                peak_rise = std::max(peak_rise, start_feet - run.player->rectangle().bottom());
                if (tick == 0)
                {
                    require(run.player->move_state() == PlayerMoveState::jumping && run.takeoffs == 1,
                        (run.context + ": a held press did not launch once from the lower surface").c_str());
                }
                if (landing_tick < 0 && supported(*run.player))
                {
                    first_landing = run.support_name();
                    landing_tick = tick;
                }
            }
            require(run.takeoffs == 1, (run.context + ": held input produced another takeoff").c_str());
            const std::string final_support = run.support_name();
            const bool reached = first_landing == route.target && final_support == route.target &&
                run.player->move_state() == PlayerMoveState::on_drop_down_ground &&
                run.player->velocity() == Vector2F::ZERO;
            std::ostringstream report;
            report << std::fixed << std::setprecision(3) << "MEASURE legacy-route stage=" << route.stage
                << " lower=" << route.lower << " target=" << route.target << " required_rise=" << route.rise
                << " peak_rise=" << peak_rise << " first=" << first_landing << " final=" << final_support
                << " landing_tick=" << landing_tick << " takeoffs=" << run.takeoffs
                << " outcome=" << (reached ? "PASS" : "FAIL") << '\n';
            const std::string line = report.str();
            std::cout.write(line.data(), static_cast<std::streamsize>(line.size()));
            std::cout.flush();
            if (!reached)
            {
                ++failures;
                if (route.rise == 200.0f) { ++failed_200; }
                failed_routes += " [" + run.context + "]";
            }
        }
        require(failures == 0, ("legacy authored climbs failed=" + std::to_string(failures) +
            ", failed200=" + std::to_string(failed_200) + failed_routes).c_str());
        std::cout << "PASS legacy authored climbs: 13 authored 200-unit one-way rises plus 170/175, "
            "both map halves, complete built geometry, one held takeoff and target landing per pair\n";
    }
}
