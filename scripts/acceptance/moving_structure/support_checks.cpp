#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include <array>
#include <tuple>
#include "engine/collision/narrow_phase.h"

namespace moving_structure_acceptance
{
    namespace
    {
        using namespace mattmath;
        constexpr std::array<WeaponType, 5> tools{ WeaponType::sprayer, WeaponType::sniper,
            WeaponType::roller, WeaponType::mister, WeaponType::bucket };
        HorizontalMotionSpec ferry(float phase = 0.25f)
        { return { { 1000.0f, 800.0f, 320.0f, 10.0f }, 672.0f, 6.4f, phase }; }
        GameTuning quick_tuning(World& world, bool underfoot = false)
        {
            return world.variant([&](rapidjson::Document& document)
            {
                set_countdown(document, dt / 4.0f);
                document["player"]["paint_underfoot"]["enabled"].SetBool(underfoot);
            });
        }
        void begin_rider(Fixture& fixture, TranslatingStructurePaintable& platform,
            WeaponType tool = WeaponType::sprayer, PlayerTeam team = PlayerTeam::a)
        {
            fixture.add_player(platform.bounds().center().x, platform.bounds().top(), team, tool);
            fixture.build(); fixture.start(); fixture.settle();
            require(platform.motion_id() != 0 && fixture.players[0]->carrier_id() == platform.motion_id(),
                "Real admitted rider did not acquire the moving support");
        }
        std::vector<std::uint64_t> observed(const Fixture& fixture)
        {
            std::vector<std::uint64_t> out{ static_cast<std::uint64_t>(fixture.level->state()) };
            for (const Player* player : fixture.players)
            {
                rectangle_bits(out, player->rectangle());
                for (float value : { player->velocity().x, player->velocity().y, player->health(),
                    player->weapon_ammo(), player->respawn_timer(), player->primary_weapon().nozzle_pos().x,
                    player->primary_weapon().nozzle_pos().y }) { out.push_back(bits(value)); }
                out.push_back(static_cast<std::uint32_t>(player->move_state()));
                out.push_back(static_cast<std::uint32_t>(player->state())); out.push_back(player->carrier_id());
                const auto effect = player->paint_underfoot_state();
                out.push_back(effect.enabled); out.push_back(static_cast<std::uint32_t>(effect.support_team));
                out.push_back(static_cast<std::uint32_t>(effect.step_team));
            }
            for (const auto* platform : fixture.movers)
            {
                const auto state = motion_bits(platform->motion_state()); out.insert(out.end(), state.begin(), state.end());
                const auto tiles = sample_bits(snapshot(*platform)); out.insert(out.end(), tiles.begin(), tiles.end());
            }
            const LevelEndInfo paint = fixture.level->level_end_info();
            for (float value : { paint.total_area, paint.team_a_area, paint.team_b_area }) { out.push_back(bits(value)); }
            for (float value : paint.player_areas) { out.push_back(bits(value)); }
            return out;
        }
        class CountingFilter final : public SeatInputFilter
        {
        public:
            int calls = 0;
            void filter_seats(std::vector<PlayerInputData>&, const Level&) override
            { ++calls; std::ignore = std::rand(); }
        };
        void unchanged_refusal(Fixture& fixture, float seconds, const std::string& fragment)
        {
            const auto before = observed(fixture);
            CountingFilter filter;
            std::srand(7821); const int expected_next = std::rand(); std::srand(7821);
            throws_with([&] { fixture.tick(input(), seconds, &filter); }, fragment);
            require(filter.calls == 0, "Refused moving update entered its input/RNG filter");
            require(std::rand() == expected_next, "Refused moving update consumed gameplay RNG");
            require(observed(fixture) == before, "Refused moving update changed observed world/paint/player state");
        }
        void admission_controls(World& world)
        {
            const auto make = [&](const HorizontalMotionSpec& spec)
            {
                auto fixture = std::make_unique<Fixture>(world);
                fixture->tuning = quick_tuning(world);
                auto& platform = fixture->mover(spec);
                fixture->add_player(platform.bounds().center().x, platform.bounds().top());
                return fixture;
            };
            {
                auto spec = ferry(); spec.period_seconds = 2.0f * SECONDS_PER_STEP;
                auto fixture = make(spec);
                throws_with([&] { fixture->build(); }, "Horizontal motion relative horizontal displacement is unsafe");
            }
            {
                auto fixture = make(ferry());
                fixture->tuning = world.variant([](rapidjson::Document& d)
                { set_countdown(d, dt / 4.0f); d["player"]["jump_launch_velocity"].SetFloat(100000.0f); });
                throws_with([&] { fixture->build(); }, "Horizontal motion relative vertical displacement is unsafe");
            }
            {
                auto fixture = make(ferry());
                fixture->tuning = world.variant([](rapidjson::Document& d)
                { set_countdown(d, dt / 4.0f); d["player"]["jump_acceleration"].SetFloat(1000000.0f); });
                throws_with([&] { fixture->build(); }, "Horizontal motion relative vertical displacement is unsafe");
            }
            {
                Fixture fixture(world); fixture.tuning = quick_tuning(world);
                auto& platform = fixture.mover(ferry());
                fixture.add_player({ platform.bounds().center().x, 680.0f, 1.0f, 120.0f });
                throws_with([&] { fixture.build(); }, "Horizontal motion relative horizontal displacement is unsafe");
            }
            {
                Fixture fixture(world);
                fixture.tuning = world.variant([](rapidjson::Document& d)
                {
                    set_countdown(d, dt / 4.0f);
                    d["player"]["max_velocity"]["x"].SetFloat(50.0f);
                    d["player"]["x_initial_velocity"].SetFloat(50.0f);
                });
                auto spec = ferry(); spec.anchor.width = 16.0f;
                auto& platform = fixture.mover(spec);
                const auto size = fixture.tuning.player().size;
                fixture.add_player({ platform.bounds().center().x - size.x / 2.0f,
                    800.0f - size.y, size.x, size.y }, PlayerTeam::a, WeaponType::sprayer, -1, { 880.0f, 0.0f });
                // Tuning is now slow/off, but an actual previously boosted velocity remains.
                throws_with([&] { fixture.build(); }, "Horizontal motion relative horizontal displacement is unsafe");
            }
            {
                auto fixture = make(ferry());
                fixture->tuning = world.variant([](rapidjson::Document& d)
                {
                    set_countdown(d, dt / 4.0f);
                    d["player"]["paint_underfoot"]["enabled"].SetBool(true);
                    d["player"]["paint_underfoot"]["own_speed_multiplier"].SetFloat(4.0f);
                });
                throws_with([&] { fixture->build(); }, "Horizontal motion relative horizontal displacement is unsafe");
            }
            // Each obstruction is clear of the platform body but inside a legal
            // full-width rider overhang/corridor; half-width padding misses the first.
            for (const RectangleF obstruction : { RectangleF(2032.0f, 700.0f, 4.0f, 20.0f),
                RectangleF(2034.0f, 790.0f, 80.0f, 10.0f), RectangleF(1200.0f, 670.0f, 20.0f, 20.0f) })
            {
                auto fixture = make(ferry()); fixture->plain(obstruction);
                throws_with([&] { fixture->build(); }, "Horizontal motion standing envelope intersects blocking geometry");
            }
            {
                auto fixture = make(ferry()); fixture->plain({ 1200.0f, 805.0f, 20.0f, 4.0f });
                throws_with([&] { fixture->build(); }, "Horizontal motion platform envelope intersects blocking geometry");
            }
            {
                Fixture fixture(world, 1, { 900.0f, 700.0f, 1400.0f, 1000.0f });
                fixture.tuning = quick_tuning(world); auto& platform = fixture.mover(ferry());
                fixture.add_player(platform.bounds().center().x, 800.0f);
                throws_with([&] { fixture.build(); }, "Horizontal motion standing envelope leaves Scene bounds");
            }
            {
                Fixture fixture(world, 1, { 900.0f, 600.0f, 1400.0f, 205.0f });
                fixture.tuning = quick_tuning(world); auto& platform = fixture.mover(ferry());
                fixture.add_player(platform.bounds().center().x, 800.0f);
                throws_with([&] { fixture.build(); }, "Horizontal motion platform envelope leaves Scene bounds");
            }
        }
        void first_update_and_zero(World& world)
        {
            for (int kind = 0; kind < 3; ++kind)
            {
                Fixture fixture(world);
                fixture.tuning = world.variant([&](rapidjson::Document& d)
                { set_countdown(d, kind == 1 ? dt / 2.0f : dt / 4.0f); });
                auto& platform = fixture.mover(ferry()); fixture.add_player(platform.bounds().center().x, 800.0f);
                fixture.build(kind == 2 ? OpeningMode::full : OpeningMode::off);
                require(fixture.level->has_horizontal_motion(), "Opening admission failed to discover actual mover");
                for (float bad : { -dt, std::numeric_limits<float>::quiet_NaN(),
                    std::numeric_limits<float>::infinity(), std::nextafter(SECONDS_PER_STEP,
                        std::numeric_limits<float>::infinity()), 6.4f })
                { unchanged_refusal(fixture, bad, "Horizontal motion"); }
                if (kind != 2)
                {
                    CountingFilter filter;
                    const auto before = motion_bits(platform.motion_state());
                    fixture.tick(input(), dt, &filter);
                    require(fixture.level->match_started() && filter.calls == 1 &&
                        motion_bits(platform.motion_state()) != before,
                        "Short/countdown crossing did not commit exactly the prepared active frame");
                }
            }
            Fixture fixture(world); fixture.tuning = quick_tuning(world);
            auto& platform = fixture.mover(ferry()); begin_rider(fixture, platform);
            fixture.tick(); const auto before = observed(fixture);
            const auto history = motion_bits(platform.motion_state());
            for (int repeat = 0; repeat < 3; ++repeat)
            {
                fixture.tick(input(), 0.0f);
                require(observed(fixture) == before && motion_bits(platform.motion_state()) == history,
                    "Zero moving update replayed carry/paint or changed persisted history");
                const auto step = platform.collision_step();
                require(step.segment_count == 0 && bits(step.dt) == bits(0.0f), "Zero update retained positive segments");
                require(step.before == platform.bounds() && step.after == platform.bounds(),
                    "Zero update exposed stale before/after travel to sweep consumers");
            }
            fixture.tick(); require(motion_bits(platform.motion_state()) != history,
                "Positive update after zero steps did not resume motion");
            fixture.tuning = world.variant([](rapidjson::Document& d)
            {
                set_countdown(d, dt / 4.0f);
                d["player"]["paint_underfoot"]["enabled"].SetBool(true);
                d["player"]["paint_underfoot"]["own_speed_multiplier"].SetFloat(4.0f);
            });
            unchanged_refusal(fixture, dt, "Horizontal motion relative horizontal displacement is unsafe");
        }
        void unchanged_action_refusal(Fixture& fixture, const PlayerInputData& requested,
            const std::string& diagnostic)
        {
            const auto before = observed(fixture);
            std::vector<std::vector<std::uint64_t>> steps;
            for (const auto* deck : fixture.movers) { steps.push_back(step_bits(deck->collision_step())); }
            const auto camera = fixture.level->view_camera(0);
            const auto emphasis = bits(fixture.level->player_identity_emphasis(0));
            const auto prior_input = bits(fixture.players[0]->input_x_movement());
            CountingFilter filter;
            std::srand(7631); const int next = std::rand(); std::srand(7631);
            throws_with([&] { fixture.tick(requested, dt, &filter); }, diagnostic);
            require(filter.calls == 0 && std::rand() == next,
                "Numerical refusal reached the ordinary input/RNG filter");
            require(observed(fixture) == before && fixture.level->view_camera(0) == camera &&
                bits(fixture.level->player_identity_emphasis(0)) == emphasis &&
                bits(fixture.players[0]->input_x_movement()) == prior_input,
                "Numerical refusal changed observed state, camera, input or identity clock");
            for (std::size_t index = 0; index < fixture.movers.size(); ++index)
            {
                require(step_bits(fixture.movers[index]->collision_step()) == steps[index],
                    "Numerical refusal rewrote the previously published collision step");
            }
        }
        void finite_horizontal_refusals(World& world)
        {
            {
                Fixture f(world);
                f.tuning = world.variant([](rapidjson::Document& document)
                {
                    set_countdown(document, dt / 4.0f);
                    document["player"]["max_velocity"]["x"].SetFloat(16.0f);
                    document["player"]["x_initial_velocity"].SetFloat(16.0f);
                });
                auto& deck = f.mover(ferry());
                f.add_player(deck.bounds().center().x, deck.bounds().top(), PlayerTeam::a,
                    WeaponType::sprayer, -1, { 16.0f, 0.0f });
                f.build(); f.tick(input(1.0f));
                require(f.level->match_started() && f.players[0]->velocity().x == 16.0f &&
                    f.players[0]->carrier_id() == deck.motion_id(),
                    "Stop-time control did not establish an ordinary active vx16 rider");
                f.tuning = world.variant([](rapidjson::Document& document)
                {
                    set_countdown(document, dt / 4.0f);
                    auto& player = document["player"];
                    player["max_velocity"]["x"].SetFloat(16.0f);
                    player["x_initial_velocity"].SetFloat(16.0f);
                    player["x_acceleration"].SetFloat((std::numeric_limits<float>::min)());
                    player["x_deacceleration"].SetFloat((std::numeric_limits<float>::min)());
                    player["paint_underfoot"]["enabled"].SetBool(true);
                    player["paint_underfoot"]["own_speed_multiplier"].SetFloat(1.0f);
                    player["paint_underfoot"]["enemy_speed_multiplier"].SetFloat(1.0f);
                });
                // Both rates are normal positive FLT_MIN, not subnormal test
                // data. An actual opposing axis would overflow time_to_stop;
                // require refusal before that input or any simulation is applied.
                unchanged_action_refusal(f, input(-1.0f), "Horizontal motion horizontal stop-time division is unsafe");
            }
            {
                // Geometry is enormous but allocation is bounded: no painted
                // faces, no TerrainMaterial, one collider per participant. The
                // constructor has no extent-sized tile/material/grid build.
                constexpr float retained = 3.39e38f;
                Fixture f(world, 1, { -3.0e37f, -3.0e37f, 6.0e37f, 6.0e37f });
                f.tuning = quick_tuning(world);
                auto& deck = f.mover({ { 0.0f, 0.0f, 1.0e37f, 1.0e37f },
                    1.0e36f, 10.0f, 0.25f }, PaintableFaces{});
                require(deck.material() == nullptr && snapshot(deck).empty(),
                    "Pre-clamp control unexpectedly constructed extent-sized material or paint");
                Player& player = f.add_player({ deck.bounds().left(), -1.0e37f, 1.0e37f, 1.0e37f },
                    PlayerTeam::a, WeaponType::roller, -1, { retained, 0.0f });
                f.build();
                require(f.scene->collision_objects().size() == 2 && deck.motion_id() != 0 &&
                    player.velocity().x == retained,
                    "Pre-clamp control did not admit its bounded two-object retained-velocity setup");
                f.tuning = world.variant([](rapidjson::Document& document)
                {
                    set_countdown(document, dt / 4.0f);
                    document["player"]["x_acceleration"].SetFloat((std::numeric_limits<float>::max)() / 2.0f);
                });
                // Do not run even one huge-world physics step: the observable
                // contract is refusal before filters/layout/motion. A later cap
                // cannot excuse a nonfinite same-direction velocity addition.
                unchanged_action_refusal(f, input(1.0f), "Horizontal motion horizontal pre-clamp addition is unsafe");
            }
        }
        void conservative_numeric_boundaries(World& world)
        {
            const auto slow = [&]
            {
                return world.variant([](rapidjson::Document& document)
                {
                    set_countdown(document, dt / 4.0f);
                    document["player"]["max_velocity"]["x"].SetFloat(1.0f);
                    document["player"]["x_initial_velocity"].SetFloat(1.0f);
                    document["player"]["x_acceleration"].SetFloat(0.0f);
                    document["player"]["x_deacceleration"].SetFloat(1.0f);
                });
            };
            for (float seconds : { std::nextafter(SECONDS_PER_STEP, 0.0f), SECONDS_PER_STEP })
            {
                Fixture f(world); f.tuning = slow();
                auto& deck = f.mover({ { 0.0f, 800.0f, 320.0f, 10.0f },
                    8.0f, 2.0f * SECONDS_PER_STEP, 0.0f }, PaintableFaces{});
                f.add_player({ 144.0f, 680.0f, 32.0f, 120.0f }); f.build();
                const float before = deck.bounds().x; CountingFilter filter;
                f.tick(input(), seconds, &filter);
                require(filter.calls == 1 && f.level->match_started() && deck.bounds().x > before &&
                    f.players[0]->carrier_id() == deck.motion_id(),
                    "Safe below/equal maximum dynamic dt did not commit an actual supported step");
            }
            // At the minimum period, one maximum step spans the entire travel.
            // Travelling a whole actual body width is outside the strict safe
            // content policy. Its immediate neighbours also remain inside the
            // conservative refusal band; no exact internal cutoff is copied.
            for (float travel : { std::nextafter(32.0f, 0.0f), 32.0f,
                std::nextafter(32.0f, std::numeric_limits<float>::infinity()) })
            {
                Fixture f(world); f.tuning = slow();
                const HorizontalMotionSpec spec{ { 0.0f, 800.0f, 320.0f, 10.0f },
                    travel, 2.0f * SECONDS_PER_STEP, 0.0f };
                const auto prepared = HorizontalMotion(spec).prepare(SECONDS_PER_STEP);
                require(bits(prepared.collision.after.x) == bits(travel),
                    "Boundary control did not describe a represented whole-width motion step");
                auto& deck = f.mover(spec, PaintableFaces{});
                f.add_player({ deck.bounds().center().x - 16.0f, 680.0f, 32.0f, 120.0f });
                throws_with([&] { f.build(); }, "Horizontal motion relative horizontal displacement is unsafe");
            }
            for (float origin : { 65536.0f, 16777216.0f })
            {
                // Translate modest fixed-size geometry; neither face count nor
                // any allocation grows with this coordinate/binade difference.
                Fixture f(world, 1, { origin - 1024.0f, -1024.0f, 8192.0f, 4096.0f });
                f.tuning = quick_tuning(world);
                auto spec = ferry(); spec.anchor.x += origin;
                auto& deck = f.mover(spec, PaintableFaces{});
                f.add_player(deck.bounds().center().x, deck.bounds().top());
                require(snapshot(deck).empty() && deck.material() == nullptr,
                    "Large-coordinate control unexpectedly allocated painted/material geometry");
                if (origin == 65536.0f)
                {
                    f.build(); const float before = deck.bounds().x; f.tick();
                    require(deck.bounds().x != before && f.players[0]->carrier_id() == deck.motion_id(),
                        "Modest translated control failed to admit and step its actual rider");
                }
                else
                {
                    throws_with([&] { f.build(); }, "Horizontal motion relative horizontal displacement is unsafe");
                }
            }
        }
        void stationary_and_resources(World& world)
        {
            for (WeaponType tool : tools)
            {
                Fixture fixture(world); fixture.tuning = quick_tuning(world);
                auto& platform = fixture.mover(ferry()); begin_rider(fixture, platform, tool);
                const auto initial_tool_phase = bits(fixture.players[0]->primary_weapon().motion_state().travel_phase);
                const auto initial_camera = fixture.players[0]->camera();
                bool reversed = false; const int initial_direction = platform.motion_state().direction;
                float moved = 0.0f;
                for (int step = 0; step < 210; ++step)
                {
                    Player& player = *fixture.players[0];
                    const float before_x = player.rectangle().x, before_platform = platform.bounds().x;
                    const Vector2F before_nozzle = player.primary_weapon().nozzle_pos(); fixture.tick();
                    const float carry = platform.bounds().x - before_platform;
                    require_near(player.rectangle().x - before_x, carry);
                    require_near(player.primary_weapon().nozzle_pos().x - before_nozzle.x, carry);
                    require(bits(player.velocity().x) == bits(0.0f) && player.carrier_id() == platform.motion_id(),
                        "Stationary rider gained personal velocity or lost established carrier");
                    require(bits(player.primary_weapon().motion_state().travel_phase) == initial_tool_phase,
                        "Involuntary carry advanced weapon locomotion");
                    for (const auto& effect : player.movement_feedback().effects())
                    { require(effect.kind != MovementFeedbackKind::step && effect.kind != MovementFeedbackKind::skid,
                        "Involuntary carry emitted walking/skid feedback"); }
                    moved += std::fabs(carry); reversed |= platform.motion_state().direction != initial_direction;
                }
                require(moved > 500.0f && reversed && fixture.players[0]->camera() != initial_camera,
                    "Rider trace lacked actual travel/reversal/world camera follow");

                Fixture moving(world), stationary(world);
                moving.tuning = quick_tuning(world); stationary.tuning = quick_tuning(world);
                auto& deck = moving.mover({ { 1000.0f, 800.0f, 2000.0f, 10.0f }, 80.0f, 6.4f, 0.25f });
                moving.add_player(deck.bounds().center().x, 800.0f, PlayerTeam::a, tool);
                stationary.surface(deck.bounds(), { true, true, true, true });
                stationary.add_player(deck.bounds().center().x, 800.0f, PlayerTeam::a, tool);
                moving.build(); stationary.build(); moving.start(); stationary.start(); moving.settle(); stationary.settle();
                // Every tool starts its ordinary cooldown at construction.
                // In particular, eight initial ticks cannot fire a Sniper.
                for (int warmup = 0; warmup < 120 &&
                    (!moving.players[0]->primary_weapon().ready_to_shoot() ||
                        !stationary.players[0]->primary_weapon().ready_to_shoot()); ++warmup)
                { moving.tick(input()); stationary.tick(input()); }
                require(moving.players[0]->primary_weapon().ready_to_shoot() &&
                    stationary.players[0]->primary_weapon().ready_to_shoot(),
                    "Resource controls failed to reach actual tool readiness");
                bool paid = false;
                const float initial_ammo = moving.players[0]->weapon_ammo();
                for (int step = 0; step < 180; ++step)
                {
                    const auto action = input(0.0f, false, step < 8);
                    moving.tick(action); stationary.tick(action);
                    paid |= moving.players[0]->weapon_ammo() < initial_ammo;
                    require(bits(moving.players[0]->weapon_ammo()) == bits(stationary.players[0]->weapon_ammo()) &&
                        moving.players[0]->primary_weapon().ready_to_shoot() == stationary.players[0]->primary_weapon().ready_to_shoot() &&
                        bits(moving.players[0]->health()) == bits(stationary.players[0]->health()),
                        "Carry changed paid-shot/resource/health timing against static control");
                }
                require(paid, "Resource carry control did not consume actual paid ammunition");
            }
        }
        void personal_motion_and_underfoot(World& world)
        {
            Fixture moving(world), stationary(world);
            moving.tuning = quick_tuning(world); stationary.tuning = quick_tuning(world);
            auto& deck = moving.mover({ { 1000.0f, 800.0f, 2000.0f, 10.0f }, 80.0f, 6.4f, 0.25f });
            moving.add_player(deck.bounds().center().x, 800.0f, PlayerTeam::a, WeaponType::roller);
            stationary.surface(deck.bounds()); stationary.add_player(deck.bounds().center().x, 800.0f, PlayerTeam::a, WeaponType::roller);
            moving.build(); stationary.build(); moving.start(); stationary.start(); moving.settle(); stationary.settle();
            bool advanced_tool = false;
            for (int step = 0; step < 120; ++step)
            {
                const float x = step < 40 ? 1.0f : step < 60 ? 0.0f : step < 100 ? -1.0f : 0.0f;
                moving.tick(input(x)); stationary.tick(input(x));
                require(bits(moving.players[0]->velocity().x) == bits(stationary.players[0]->velocity().x),
                    "Carrier altered personal acceleration/braking/reversal velocity");
                const float phase_error = std::fabs(moving.players[0]->primary_weapon().motion_state().travel_phase -
                    stationary.players[0]->primary_weapon().motion_state().travel_phase);
                require(std::min(phase_error, std::fabs(1.0f - phase_error)) < 0.003f,
                    "Personal Roller travel changed beyond bounded world-rounding tolerance");
                advanced_tool |= moving.players[0]->primary_weapon().motion_state().travel_phase != 0.0f;
            }
            require(advanced_tool, "Personal movement control did not exercise actual Roller travel");

            Fixture colored(world); colored.tuning = quick_tuning(world, true);
            auto& paint_deck = colored.mover(ferry());
            const auto rectangle = paint_deck.bounds();
            world.paint(paint_deck, { rectangle.left(), rectangle.top() - 1.0f, rectangle.width, 3.0f }, PlayerTeam::a, 5);
            begin_rider(colored, paint_deck);
            require(colored.players[0]->paint_underfoot_state().step_team == PlayerTeam::a,
                "Confirmed moving top failed world-to-anchor own-team sampling");
            for (int step = 0; step < 8; ++step) { colored.tick(input(1.0f)); }
            require(colored.players[0]->paint_underfoot_state().step_team == PlayerTeam::a,
                "Moving carried contact lost its consumed owned step");
            const auto current = paint_deck.bounds();
            world.paint(paint_deck, { current.left(), current.top() - 1.0f, current.width, 3.0f }, PlayerTeam::b, 6);
            colored.tick(input());
            require(colored.players[0]->paint_underfoot_state().step_team == PlayerTeam::a &&
                colored.players[0]->paint_underfoot_state().support_team == PlayerTeam::b,
                "Moving repaint did not retain completed-contact latency");
            colored.tick(input());
            require(colored.players[0]->paint_underfoot_state().step_team == PlayerTeam::b &&
                colored.players[0]->carrier_id() == paint_deck.motion_id(),
                "Enemy paint incorrectly selected or removed the physical carrier");
        }
        void release_and_one_way(World& world)
        {
            {
                Fixture f(world); f.tuning = quick_tuning(world); auto& deck = f.mover(ferry()); begin_rider(f, deck);
                const float x = f.players[0]->rectangle().x; f.tick(input(0.0f, true));
                require(f.players[0]->carrier_id() == 0 && f.players[0]->rectangle().bottom() < 800.0f &&
                    bits(f.players[0]->rectangle().x) == bits(x), "Takeoff retained or applied same-step carry");
            }
            {
                Fixture f(world); f.tuning = quick_tuning(world); auto& deck = f.mover(ferry(),
                    { true, true, true, true }, CollisionObjectType::structure_jump_through); begin_rider(f, deck);
                const float x = f.players[0]->rectangle().x; f.tick(input(0.0f, false, false, true));
                require(f.players[0]->carrier_id() == 0 && f.players[0]->rectangle().bottom() > 800.0f &&
                    bits(f.players[0]->rectangle().x) == bits(x), "One-way drop retained or applied same-step carry");
            }
            {
                Fixture f(world); f.tuning = quick_tuning(world);
                auto spec = ferry(); spec.anchor.width = 80.0f; spec.travel_x = 40.0f;
                auto& deck = f.mover(spec); begin_rider(f, deck);
                bool left = false;
                for (int step = 0; step < 90 && !left; ++step)
                { f.tick(input(1.0f)); left = f.players[0]->carrier_id() == 0; }
                require(left && f.players[0]->move_state() == PlayerMoveState::in_air, "Edge walk-off kept stale carry");
            }
            {
                Fixture f(world); f.tuning = quick_tuning(world); auto& deck = f.mover(ferry()); begin_rider(f, deck);
                Player& player = *f.players[0]; player.kill();
                require(player.state() == PlayerState::alive && player.health() == 0.0f && player.carrier_id() == 0,
                    "Debug fatal health failed to release before dead-state transition");
                f.tick(); require(player.state() == PlayerState::dead && player.carrier_id() == 0, "Dead rider kept carry");
                bool respawned = false;
                for (int step = 0; step < 300 && !respawned; ++step)
                { f.tick(); respawned = player.state() == PlayerState::alive; }
                require(respawned && player.carrier_id() == 0, "Natural respawn inherited carrier identity");
                require(f.level->place_player(0, { deck.bounds().center().x - player.rectangle().width / 2.0f, 680.0f }),
                    "Existing rider view rejected lifecycle placement");
                f.settle(); require(player.carrier_id() == deck.motion_id(), "Lifecycle control failed to reacquire carrier");
                require(f.level->place_player(0, { 500.0f, 400.0f }), "Existing view rejected teleport");
                require(player.carrier_id() == 0, "Teleport retained carrier");
                require(f.level->respawn_player(0), "Existing view rejected forced respawn");
                require(player.carrier_id() == 0, "Forced respawn retained carrier");
            }
            {
                Fixture f(world); f.tuning = quick_tuning(world); auto& deck = f.mover(ferry()); begin_rider(f, deck);
                const auto& tuning = f.tuning.projectile(ProjectileType::jet);
                RectangleF launch = f.players[0]->rectangle(); const auto half = tuning.details.col_rect_size / 2.0f;
                launch.offset(half.x, half.y);
                ProjectileJet shot(launch, Vector2F::ZERO, PlayerTeam::b, 6, world.palette.team_b,
                    &world.render, resolve_projectile_strip(tuning, world.render), &tuning);
                // Typed actual damage callback tests immediate release specifically;
                // moving projectile sweep/provenance are separately tested in their module.
                for (int hit = 0; f.players[0]->health() > 0.0f && hit < 64; ++hit)
                { f.players[0]->on_contact(shot, Vector2F::ZERO, 0.0f); }
                require(f.players[0]->state() == PlayerState::alive && f.players[0]->health() <= 0.0f &&
                    f.players[0]->carrier_id() == 0, "Projectile-fatal health kept carrier before state transition");
            }
            {
                Fixture f(world); f.tuning = quick_tuning(world); auto& deck = f.mover(ferry()); begin_rider(f, deck);
                const auto before = motion_bits(deck.motion_state());
                f.level->suspend();
                require(f.players[0]->carrier_id() == 0 && motion_bits(deck.motion_state()) == before,
                    "Context reset kept carrier or advanced frozen motion");
                f.level->resume(); f.settle();
                require(f.players[0]->carrier_id() == deck.motion_id(), "Resume did not reacquire an actual top support");
            }
            for (bool underside : { false, true })
            {
                Fixture f(world); f.tuning = quick_tuning(world); auto spec = ferry(); spec.travel_x = 80.0f;
                auto& deck = f.mover(spec);
                if (underside) { f.add_player(deck.bounds().center().x, 927.0f); }
                else { f.add_player(deck.bounds().left() - 25.0f, 820.0f); }
                f.build(); f.start();
                if (underside)
                { require(f.players[0]->rectangle().top() >= deck.bounds().bottom() - 0.002f,
                    "Underside control did not resolve an actual overlapping body"); }
                else
                { require(f.players[0]->rectangle().right() <= deck.bounds().left() + 0.002f,
                    "Side control did not resolve an actual overlapping body"); }
                require(f.players[0]->carrier_id() == 0, "Side or underside contact granted top carry");
                f.settle(); require(f.players[0]->carrier_id() == 0, "Side/underside contact left stale carry");
            }
            {
                Fixture f(world); f.tuning = quick_tuning(world); auto spec = ferry(); spec.travel_x = 80.0f;
                auto& deck = f.mover(spec, { true, true, true, true }, CollisionObjectType::structure_jump_through);
                f.surface({ 500.0f, 1000.0f, 2000.0f, 80.0f }); f.add_player(deck.bounds().center().x, 1000.0f);
                f.build(); f.start(); f.settle();
                bool rose_through = false, was_above = false, landed = false;
                for (int step = 0; step < 240 && !landed; ++step)
                {
                    auto action = input(); action.jump_pressed = step == 0; action.jump_held = step < 16; f.tick(action);
                    const Player& player = *f.players[0];
                    if (player.velocity().y < 0.0f && player.rectangle().intersects(deck.bounds()))
                    { rose_through = true; require(player.carrier_id() == 0, "Rising one-way pass attached carrier"); }
                    was_above |= player.rectangle().bottom() < deck.bounds().top();
                    landed = was_above && player.carrier_id() == deck.motion_id();
                }
                require(rose_through && was_above && landed, "Actual held jump did not rise through then land on moving one-way deck");
            }
        }
        class EdgeLandingProbe final : public labrador::GameObject
        {
        public:
            EdgeLandingProbe(const Player& player, const TranslatingStructurePaintable& platform) :
                player_(player), platform_(platform) {}
            void update(float /*seconds*/) override
            {
                // Scene updates every collider before its ordinary GameObjects.
                // Observe actual pre-resolve shapes; do not move either object,
                // dispatch a contact, or add a production test seam.
                ++updates;
                body = this->player_.rectangle();
                surface = this->platform_.bounds();
                descending_speed = this->player_.velocity().y;
                manifold = labrador::narrow_phase(*this->player_.shape(), *this->platform_.shape());
            }
            void draw(labrador::DrawList&) const override {}
            RectangleF bounds() const override { return RectangleF::ZERO; }
            int updates = 0;
            RectangleF body = RectangleF::ZERO;
            RectangleF surface = RectangleF::ZERO;
            float descending_speed = 0.0f;
            std::optional<labrador::Manifold> manifold;
        private:
            const Player& player_;
            const TranslatingStructurePaintable& platform_;
        };
        void resolving_one_way_edge(World& world)
        {
            Fixture f(world); f.tuning = quick_tuning(world);
            // A slow left-moving deck begins horizontally clear of these feet.
            // Its first actual step leaves only 0.25 units of edge overlap,
            // less than the ordinary gravity penetration on that same step.
            auto spec = ferry(0.75f); spec.travel_x = 96.0f;
            auto& deck = f.mover(spec, PaintableFaces{ true, true, true, true }, CollisionObjectType::structure_jump_through);
            const auto prepared = HorizontalMotion(spec).prepare(dt);
            const auto size = f.tuning.player().size;
            const float player_right = prepared.collision.after.left() + 0.25f;
            Player& player = f.add_player({ player_right - size.x,
                deck.bounds().top() - size.y - 0.25f, size.x, size.y });
            const RectangleF initial_body = player.rectangle();
            require(initial_body.right() < deck.bounds().left() && initial_body.bottom() < deck.bounds().top(),
                "One-way edge control did not start clear and above the actual deck");
            auto* probe = f.scene->add(std::make_unique<EdgeLandingProbe>(player, deck));
            f.build();
            require(deck.motion_id() != 0 && player.carrier_id() == 0,
                "One-way edge control did not admit an initially unbound actual player/deck");
            f.tick(); // Short countdown crosses into exactly one ordinary active step.
            require(f.level->match_started() && probe->updates == 1,
                "One-way edge control did not observe exactly one actual Scene update");
            const float overlap = std::min(probe->body.right(), probe->surface.right()) -
                std::max(probe->body.left(), probe->surface.left());
            const float downward_penetration = probe->body.bottom() - probe->surface.top();
            require(overlap > 0.0f && overlap < downward_penetration && probe->descending_speed > 0.0f,
                "Actual one-way edge shapes did not have the required shallow horizontal penetration");
            require(probe->manifold && std::abs(probe->manifold->normal.x) >
                std::abs(probe->manifold->normal.y) && probe->manifold->normal.y == 0.0f,
                "Actual pre-resolve edge manifold was not horizontal");
            require_near(probe->manifold->penetration, overlap);
            require(player.state() == PlayerState::alive && player.health() > 0.0f &&
                player.move_state() == PlayerMoveState::on_drop_down_ground &&
                bits(player.rectangle().x) == bits(probe->body.x),
                "Actual one-way callback did not resolve the descending edge landing vertically");
            require_near(player.rectangle().bottom(), deck.bounds().top());
            require(player.carrier_id() == deck.motion_id(),
                "Successful resolving one-way edge support was lost because its measured normal was horizontal");
            const float player_before = player.rectangle().x;
            const float deck_before = deck.bounds().x;
            f.tick();
            const float deck_travel = deck.bounds().x - deck_before;
            require(deck_travel < 0.0f && player.carrier_id() == deck.motion_id() &&
                player.move_state() == PlayerMoveState::on_drop_down_ground && player.velocity().x == 0.0f,
                "Resolved edge rider did not remain an actual stationary carrier passenger");
            require_near(player.rectangle().x - player_before, deck_travel);
            require_near(player.rectangle().bottom(), deck.bounds().top());
        }
        void dock_controls(World& world)
        {
            for (float direction : { 1.0f, -1.0f })
            {
                Fixture f(world); f.tuning = quick_tuning(world);
                f.surface({ 640.0f, 960.0f, 640.0f, 10.0f }); f.surface({ 2320.0f, 960.0f, 640.0f, 10.0f });
                auto& deck = f.mover({ { 1304.0f, 960.0f, 320.0f, 10.0f }, 672.0f, 6.4f,
                    direction > 0.0f ? 0.88f : 0.38f });
                f.add_player(direction > 0.0f ? 1000.0f : 2600.0f, 960.0f); f.build(); f.start(); f.settle();
                require(f.players[0]->carrier_id() == 0, "Dock boarding did not start on static support");
                bool boarded = false;
                for (int step = 0; step < 90 && !boarded; ++step)
                { f.tick(input(direction)); boarded = f.players[0]->carrier_id() == deck.motion_id(); }
                require(boarded, "Directed actual 24-unit dock approach did not board ferry");
                f.settle(40); require(f.players[0]->carrier_id() == deck.motion_id(), "Boarded rider could not brake and remain aboard");
            }
            for (float direction : { 1.0f, -1.0f })
            {
                Fixture f(world); f.tuning = quick_tuning(world);
                const RectangleF dock = direction > 0.0f ? RectangleF(2320.0f, 960.0f, 640.0f, 10.0f) :
                    RectangleF(640.0f, 960.0f, 640.0f, 10.0f);
                f.surface(dock);
                auto& deck = f.mover({ { 1304.0f, 960.0f, 320.0f, 10.0f }, 672.0f, 6.4f,
                    direction > 0.0f ? 0.42f : 0.92f });
                begin_rider(f, deck);
                bool retained_over_static = false, transferred = false;
                for (int step = 0; step < 90 && !transferred; ++step)
                {
                    f.tick(input(direction)); const Player& player = *f.players[0]; const auto body = player.rectangle();
                    const float moving_overlap = std::min(body.right(), deck.bounds().right()) - std::max(body.left(), deck.bounds().left());
                    const float static_overlap = std::min(body.right(), dock.right()) - std::max(body.left(), dock.left());
                    retained_over_static |= moving_overlap > 0.0f && static_overlap > moving_overlap && player.carrier_id() == deck.motion_id();
                    transferred = static_overlap > 0.0f && moving_overlap <= 0.0f && player.carrier_id() == 0 &&
                        player.move_state() == PlayerMoveState::on_ground;
                }
                require(retained_over_static && transferred, "Actual dock exit did not retain valid carrier then hand off to static support");
            }
        }
        void selection_only_contacts(World& world)
        {
            // These real Scene colliders are NOT admitted into a Level. Motion IDs
            // remain unbound; only end_contacts' integration metadata is controlled.
            // This proves policy/order, not playable overlap/admission or carriage.
            for (bool reverse : { false, true })
            {
                Fixture f(world); auto spec = ferry(); spec.travel_x = 80.0f;
                auto& first = f.mover(spec); auto& second = f.mover(spec);
                Player& player = f.add_player(first.bounds().center().x, 800.0f);
                f.scene->end_tick(); player.set_player_input(input()); f.scene->update(dt); f.scene->resolve();
                std::vector<labrador::Contact> contacts(f.scene->contacts().begin(), f.scene->contacts().end());
                std::vector<HorizontalSupportRef> supports{ { &first, 7, 11 }, { &second, 3, 22 } };
                if (reverse) { std::reverse(contacts.begin(), contacts.end()); std::reverse(supports.begin(), supports.end()); }
                player.end_contacts(contacts, supports);
                require(first.motion_id() == 0 && second.motion_id() == 0 && player.carrier_id() == 22,
                    "Selection-only equal supports depended on callback/ref order rather than stable order metadata");
            }
            {
                Fixture f(world); auto spec = ferry(); spec.travel_x = 80.0f;
                auto& moving = f.mover(spec); auto& dry = f.plain(moving.bounds());
                Player& player = f.add_player(moving.bounds().center().x, 800.0f);
                f.scene->end_tick(); player.set_player_input(input()); f.scene->update(dt); f.scene->resolve();
                const std::array<HorizontalSupportRef, 2> supports{ HorizontalSupportRef{ &moving, 0, 11 }, HorizontalSupportRef{ &dry, 99, 0 } };
                player.end_contacts(f.scene->contacts(), supports);
                require(player.carrier_id() == 0, "Nonpaintable static equal support lost static-before-moving tie");
            }
            {
                Fixture f(world); auto spec = ferry(); spec.travel_x = 80.0f;
                auto& smaller = f.mover(spec); spec.anchor.x = 1260.0f; auto& greater = f.mover(spec);
                Player& player = f.add_player(1340.0f, 800.0f);
                f.scene->end_tick(); player.set_player_input(input()); f.scene->update(dt); f.scene->resolve();
                const std::array<HorizontalSupportRef, 2> supports{ HorizontalSupportRef{ &smaller, 0, 11 }, HorizontalSupportRef{ &greater, 1, 22 } };
                player.end_contacts(f.scene->contacts(), supports);
                require(player.carrier_id() == 22, "Larger confirmed footprint overlap lost to earlier metadata order");
            }
        }
    }
    void check_support(World& world)
    {
        admission_controls(world);
        first_update_and_zero(world);
        finite_horizontal_refusals(world);
        conservative_numeric_boundaries(world);
        stationary_and_resources(world);
        personal_motion_and_underfoot(world);
        release_and_one_way(world);
        resolving_one_way_edge(world);
        dock_controls(world);
        selection_only_contacts(world);
    }
}
