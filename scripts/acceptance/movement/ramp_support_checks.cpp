#include "game/pch.h"
#include "scripts/acceptance/movement/fixtures.h"
#include "engine/collision/narrow_phase.h"

// Real input over a short ramp/deck/ramp seam. The fixture has no jump in its
// walking routes, and never changes the player's position after a route starts.
namespace movement_acceptance
{
    namespace
    {
        constexpr float ramp_left = 200.0f;
        constexpr float crest_left = 520.0f;
        constexpr float crest_right = 880.0f;
        constexpr float ramp_right = 1200.0f;
        constexpr float crest_y = -160.0f;
        constexpr float reflection_x = ramp_left + ramp_right;
        constexpr float tolerance = 0.1f;

        void check(bool condition, const std::string& message)
        {
            if (!condition) { throw std::runtime_error("ramp support: " + message); }
        }

        bool supported(const Player& player)
        {
            return player.move_state() == PlayerMoveState::on_ground ||
                player.move_state() == PlayerMoveState::on_ramp_left ||
                player.move_state() == PlayerMoveState::on_ramp_right;
        }

        float height_at(float x)
        {
            if (x <= ramp_left || x >= ramp_right) { return 0.0f; }
            if (x < crest_left) { return -(x - ramp_left) * 0.5f; }
            if (x > crest_right) { return -(ramp_right - x) * 0.5f; }
            return crest_y;
        }

        float support_height(const RectangleF& body)
        {
            return std::min(height_at(body.left()), height_at(body.right()));
        }

        std::unique_ptr<Course> bump(const Suite& suite, bool solid_first,
            bool taller_deck = false, bool low_ceiling = false, bool slope_roof = false)
        {
            auto course = suite.course();
            const auto solids = [&]
            {
                course->floor();
                const float top = taller_deck ? -200.0f : crest_y;
                course->surface({ crest_left, top, crest_right - crest_left, -top });
                if (low_ceiling)
                {
                    // An 80-unit doorway above the deck cannot admit the
                    // current 120-unit body. Its side must block the approach.
                    course->surface({ crest_left, -300.0f, crest_right - crest_left, 60.0f });
                }
                if (slope_roof)
                {
                    course->surface({ 350.0f, -300.0f, 170.0f, 70.0f });
                    course->surface({ reflection_x - 520.0f, -300.0f, 170.0f, 70.0f });
                }
            };
            const auto ramps = [&]
            {
                course->ramp(TriangleRightAxisAligned(ramp_left, 0.0f,
                    crest_left, crest_y, crest_left, 0.0f),
                    CollisionObjectType::structure_ramp_right);
                course->ramp(TriangleRightAxisAligned(crest_right, crest_y,
                    crest_right, 0.0f, ramp_right, 0.0f),
                    CollisionObjectType::structure_ramp_left);
            };
            if (solid_first) { solids(); ramps(); }
            else { ramps(); solids(); }
            return course;
        }

        void clear(const Course& course, const std::string& context)
        {
            const RectangleF& body = course.player->rectangle();
            check(std::isfinite(body.x) && std::isfinite(body.y) &&
                std::isfinite(course.player->velocity().x) && std::isfinite(course.player->velocity().y),
                context + " produced non-finite movement");
            for (const CollisionObject* object : course.scene.collision_objects())
            {
                if (object == course.player) { continue; }
                const auto overlap = narrow_phase(*course.player->shape(), *object->shape());
                const RectangleF obstacle = object->shape()->bounding_box();
                check(!overlap || overlap->penetration <= tolerance,
                    context + " penetrated a structure at tick " + std::to_string(course.tick_count) +
                    " body=(" + std::to_string(body.x) + "," + std::to_string(body.y) + ") obstacle=(" +
                    std::to_string(obstacle.x) + "," + std::to_string(obstacle.y) + "," +
                    std::to_string(obstacle.width) + "," + std::to_string(obstacle.height) + ")");
            }
        }

        void place_on_approach(Course& course, const Suite& suite, bool right, float corner,
            const std::string& context)
        {
            const float left = right ? corner - suite.tuning.player().size.x : reflection_x - corner;
            course.player->place_at({ left, height_at(corner) - suite.tuning.player().size.y });
            for (int tick = 0; tick < 8; ++tick) { course.tick(); clear(course, context); }
            check(supported(*course.player) && course.player->velocity().x == 0.0f &&
                std::fabs(course.player->rectangle().bottom() - height_at(corner)) <= tolerance,
                context + " did not settle at rest on its authored support");
        }

        struct Travel
        {
            bool left_slope = false;
            bool right_slope = false;
            bool deck = false;
            int airborne_ticks = 0;
        };

        void observe_walk(const Course& course, Travel& travel, const std::string& context)
        {
            clear(course, context);
            const Player& player = *course.player;
            const RectangleF& body = player.rectangle();
            check(player.state() == PlayerState::alive, context + " died during an ordinary walk");
            check(supported(player) || player.move_state() == PlayerMoveState::in_air,
                context + " entered a jump, drop or ceiling state without that input");
            check(body.bottom() >= crest_y - tolerance && body.bottom() <= tolerance,
                context + " rose above the crest or fell through the floor without jumping");
            if (supported(player))
            {
                check(std::fabs(body.bottom() - support_height(body)) <= tolerance,
                    context + " retained support at the wrong height");
            }
            else
            {
                // A flat-to-downhill transition first falls onto the next
                // ramp. Only an already resolved ramp supplies continuity.
                ++travel.airborne_ticks;
            }
            travel.left_slope = travel.left_slope || player.move_state() == PlayerMoveState::on_ramp_right;
            travel.right_slope = travel.right_slope || player.move_state() == PlayerMoveState::on_ramp_left;
            travel.deck = travel.deck || (player.move_state() == PlayerMoveState::on_ground &&
                body.left() >= crest_left && body.right() <= crest_right &&
                std::fabs(body.bottom() - crest_y) <= tolerance);
        }

        void walk_across(Course& course, bool right, float strength, Travel& travel,
            const std::string& context, bool hold_down = false)
        {
            bool arrived = false;
            for (int tick = 0; tick < 600; ++tick)
            {
                course.tick(input(false, false, right ? strength : -strength, hold_down));
                observe_walk(course, travel, context);
                const RectangleF& body = course.player->rectangle();
                if (right ? body.left() >= ramp_right + 40.0f : body.right() <= ramp_left - 40.0f)
                {
                    arrived = true;
                    break;
                }
            }
            check(arrived, context + " stalled before crossing the ramp/deck/ramp route");
            for (int tick = 0; tick < 30; ++tick)
            {
                course.tick();
                observe_walk(course, travel, context);
            }
            check(course.player->move_state() == PlayerMoveState::on_ground &&
                course.player->velocity() == Vector2F::ZERO &&
                std::fabs(course.player->rectangle().bottom()) <= tolerance,
                context + " did not recover to rest on the far floor");
        }

        std::string context(bool right, bool solid_first, float strength)
        {
            return std::string(right ? "right" : "left") +
                (solid_first ? " solid-first " : " ramps-first ") + std::to_string(strength);
        }
    }

    void check_ramp_roof_clearance(const Suite& suite)
    {
        size_t approaches = 0;
        for (const bool right : { false, true })
        {
            for (const bool solid_first : { false, true })
            {
                for (const float strength : { 0.25f, 0.5f, 1.0f })
                {
                    const std::string label = context(right, solid_first, strength) + " slope roof";
                    auto course = bump(suite, solid_first, false, false, true);
                    place_on_approach(*course, suite, right, 400.0f, label);
                    float furthest_corner = 400.0f;
                    for (int tick = 0; tick < 180; ++tick)
                    {
                        course->tick(input(false, false, right ? strength : -strength));
                        clear(*course, label);
                        const RectangleF& body = course->player->rectangle();
                        const float corner = right ? body.right() : reflection_x - body.left();
                        furthest_corner = std::max(furthest_corner, corner);
                        check(corner <= 420.0f + tolerance,
                            label + " passed the point where the body no longer fits below the roof");
                        check(supported(*course->player), label + " lost its grounded support");
                    }
                    check(furthest_corner > 410.0f &&
                        course->player->velocity() == Vector2F::ZERO,
                        label + " did not advance into a blocked uphill step");
                    // Reversing away must remain a walk, rather than requiring
                    // a jump or placement reset to escape a ceiling/ramp wedge.
                    for (int tick = 0; tick < 90; ++tick)
                    {
                        course->tick(input(false, false, right ? -strength : strength));
                        clear(*course, label + " escape");
                    }
                    const RectangleF& escaped = course->player->rectangle();
                    const float escaped_corner = right ? escaped.right() : reflection_x - escaped.left();
                    check(escaped_corner < 350.0f && course->player->state() == PlayerState::alive &&
                        course->feedback_count(MovementFeedbackKind::takeoff) == 0,
                        label + " could not walk back out below the roof");
                    ++approaches;
                }
            }
        }
        std::cout << "PASS ramp slope-roof clearance: " << approaches <<
            " approaches and walking escapes, mirrored slopes, both solid/ramp orders, quarter/half/full input\n";
    }

    void check_ramp_support(const Suite& suite)
    {
        check_ramp_roof_clearance(suite);
        size_t walking_routes = 0;
        int downhill_airborne_ticks = 0;
        for (const bool right : { false, true })
        {
            for (const bool solid_first : { false, true })
            {
                for (const float strength : { 0.25f, 0.5f, 1.0f })
                {
                    for (const bool mid_ramp : { false, true })
                    {
                        const std::string label = context(right, solid_first, strength) +
                            (mid_ramp ? " mid-ramp restart" : " toe start");
                        auto course = bump(suite, solid_first);
                        place_on_approach(*course, suite, right, mid_ramp ? 400.0f : ramp_left, label);
                        Travel travel;
                        walk_across(*course, right, strength, travel, label);
                        walk_across(*course, !right, strength, travel, label + " return");
                        check(travel.left_slope && travel.right_slope && travel.deck,
                            label + " missed a resolved slope or the solid deck");
                        check(course->feedback_count(MovementFeedbackKind::takeoff) == 0,
                            label + " required an unintended takeoff");
                        downhill_airborne_ticks += travel.airborne_ticks;
                        walking_routes += 2;
                    }

                    // The ramp cache is support, not permission to step onto a
                    // taller wall or into a doorway smaller than the body.
                    for (const bool ceiling : { false, true })
                    {
                        const std::string label = context(right, solid_first, strength) +
                            (ceiling ? " low doorway" : " taller solid wall");
                        auto blocked = bump(suite, solid_first, !ceiling, ceiling);
                        place_on_approach(*blocked, suite, right, 400.0f, label);
                        const float initial_x = blocked->player->rectangle().x;
                        for (int tick = 0; tick < 180; ++tick)
                        {
                            blocked->tick(input(false, false, right ? strength : -strength));
                            clear(*blocked, label);
                            const RectangleF& body = blocked->player->rectangle();
                            check(body.bottom() >= crest_y - tolerance,
                                label + " stepped above the ramp's actual crest");
                            check(right ? body.right() <= crest_left + tolerance :
                                body.left() >= crest_right - tolerance,
                                label + " passed through its obstruction");
                        }
                        check(std::fabs(blocked->player->rectangle().x - initial_x) > 20.0f &&
                            blocked->player->state() == PlayerState::alive,
                            label + " never exercised a live uphill approach");
                    }
                }

                const std::string down_label = context(right, solid_first, 0.25f) + " Down held";
                auto down = bump(suite, solid_first);
                place_on_approach(*down, suite, right, 400.0f, down_label);
                Travel down_travel;
                walk_across(*down, right, 0.25f, down_travel, down_label, true);
                walk_across(*down, !right, 0.25f, down_travel, down_label + " return", true);
                check(down_travel.left_slope && down_travel.right_slope && down_travel.deck &&
                    down->feedback_count(MovementFeedbackKind::takeoff) == 0,
                    down_label + " lost an ordinary solid-ramp walking route");
                downhill_airborne_ticks += down_travel.airborne_ticks;
                walking_routes += 2;

                const std::string label = context(right, solid_first, 1.0f);
                auto jumping = bump(suite, solid_first);
                place_on_approach(*jumping, suite, right, 400.0f, label + " jump");
                jumping->tick(input(true, true, right ? 1.0f : -1.0f));
                check(jumping->player->velocity().y < 0.0f &&
                    !supported(*jumping->player) &&
                    jumping->feedback_count(MovementFeedbackKind::takeoff) == 1,
                    label + " ramp support suppressed an ordinary jump");
                float greatest_clearance = 0.0f;
                for (int tick = 0; tick < 12; ++tick)
                {
                    jumping->tick(input(false, true));
                    clear(*jumping, label + " jump");
                    greatest_clearance = std::max(greatest_clearance,
                        support_height(jumping->player->rectangle()) - jumping->player->rectangle().bottom());
                }
                check(greatest_clearance > suite.tuning.player().size.y * 0.1f,
                    label + " ramp continuity pulled a jump back onto the slope");

                auto reset = bump(suite, solid_first);
                place_on_approach(*reset, suite, right, 400.0f, label + " placement reset");
                reset->player->place_at({ 0.0f, -suite.tuning.player().size.y });
                check(reset->player->move_state() == PlayerMoveState::in_air &&
                    reset->player->velocity() == Vector2F::ZERO,
                    label + " placement retained ramp movement state");
                reset->tick(input(false, false, 1.0f));
                clear(*reset, label + " placement reset");
                check(reset->player->move_state() == PlayerMoveState::on_ground &&
                    std::fabs(reset->player->rectangle().bottom()) <= tolerance,
                    label + " old ramp support moved a newly placed floor player");

                place_on_approach(*reset, suite, right, 400.0f, label + " death reset");
                reset->player->kill();
                const RectangleF death = reset->player->rectangle();
                reset->tick(input(true, true, right ? 1.0f : -1.0f));
                check(reset->player->state() == PlayerState::dead &&
                    reset->player->rectangle() == death,
                    label + " fatal ramp contact allowed another movement tick");
                reset->player->force_respawn();
                check(reset->player->state() == PlayerState::alive &&
                    reset->player->move_state() == PlayerMoveState::in_air &&
                    reset->player->rectangle() == reset->player->spawn_rectangle() &&
                    reset->player->velocity() == Vector2F::ZERO,
                    label + " forced respawn retained prior ramp support");
                reset->tick();
                clear(*reset, label + " respawn reset");
                check(reset->player->move_state() == PlayerMoveState::on_ground &&
                    std::fabs(reset->player->rectangle().bottom()) <= tolerance,
                    label + " respawn reused a stale ramp instead of its floor");
            }
        }
        std::cout << "PASS ramp support: " << walking_routes <<
            " real-input crossings/returns, mirrored slopes, both solid/ramp orders, quarter/half/full input, "
            "toe and mid-ramp restarts, quarter-input Down held; taller walls/low doorways, jump-off and placement/death/respawn resets; "
            "flat-to-downhill airborne ticks=" << downhill_airborne_ticks << '\n';
    }
}
