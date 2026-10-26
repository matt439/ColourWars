#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include <iostream>

#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"

namespace turbulence_movers_acceptance
{
    namespace
    {
        using moving_structure_acceptance::bits;

        // The source countdown returns before the production input filter for
        // its first 180 updates. Record its state separately, then record only
        // actual input-filter calls in the unchanged production replay format.
        std::vector<MatchFingerprint> prime(Trial& trial)
        {
            std::vector<MatchFingerprint> result;
            while (!trial.level->match_started() && trial.updates < 240)
            { trial.tick(); result.push_back(MatchFingerprint::of(*trial.level, trial.updates)); }
            require(trial.updates == 181 && trial.level->match_started(), "Replay changed the source 181-update count-in");
            for (int step = 0; step < 5; ++step)
            { trial.tick(); result.push_back(MatchFingerprint::of(*trial.level, trial.updates)); }
            require(result.size() == 186, "Replay changed the source settling/neutral input margin");
            return result;
        }

        bool on_static(const Trial& trial, const mattmath::RectangleF& support)
        {
            const auto& body = trial.player().rectangle();
            return supported(trial.player()) && trial.player().carrier_id() == 0 &&
                std::fabs(body.bottom() - support.top()) < 0.06f &&
                body.left() >= support.left() - 0.06f && body.right() <= support.right() + 0.06f;
        }

        double local_credit(const HorizontalStructureState& state, PlayerTeam team)
        {
            double area = 0.0;
            for (const auto& tile : state.local_paint)
            { if (tile.team == team && tile.paint_owner == input_slots::keyboard_and_mouse) { area += tile.scored_area; } }
            return area;
        }

        class RouteRecording final
        {
        public:
            RouteRecording(Trial& source, ReplayRecorder& recorder, bool side_b, std::ofstream& trace) :
                trial(source), recorder_(recorder), side_b_(side_b), trace_(trace) {}

            void tick(const PlayerInputData& action = input())
            {
                require(observations.size() < 1800, "Replay route exceeded its finite active-update budget");
                trial.tick(action, &recorder_);
                const int update = static_cast<int>(observations.size()) + 1;
                observations.push_back(MatchFingerprint::of(*trial.level, update));
                const auto& print = observations.back();
                const auto state = trial.mover(side_b_);
                trace_ << "record," << update << ',' << hex64(print.hash) << ',' << trial.player().carrier_id() << ',' <<
                    hex64(print.horizontal_motion->structures[side_b_ ? 1u : 0u].phase) << ',' <<
                    state.motion.direction << ',' << local_credit(state, side_b_ ? PlayerTeam::b : PlayerTeam::a) << '\n';
                if (update % ReplayFile::checkpoint_interval == 0) { recorder_.add_checkpoint(print); }
            }

            void require_carrier() const
            {
                const auto state = trial.mover(side_b_);
                const auto& body = trial.player().rectangle();
                require(trial.player().carrier_id() == state.id && supported(trial.player()) &&
                    std::fabs(body.bottom() - state.current_bounds.top()) < 0.06f &&
                    body.left() >= state.current_bounds.left() - 0.06f && body.right() <= state.current_bounds.right() + 0.06f,
                    "Recorded route lost whole-body moving top support");
            }

            void drive(float target)
            {
                for (int step = 0; step < 360; ++step)
                {
                    tick(input(steering(trial, target)));
                    if (supported(trial.player()) && std::fabs(trial.player().center().x - target) < 6.0f &&
                        std::fabs(trial.player().velocity().x) < 0.01f) { return; }
                }
                throw std::runtime_error("Recorded route did not stop at its bounded escape target");
            }

            Trial& trial;
            std::vector<MatchFingerprint> observations;
        private:
            ReplayRecorder& recorder_;
            bool side_b_;
            std::ofstream& trace_;
        };

        void route_policy(RouteRecording& recording, bool side_b)
        {
            auto& trial = recording.trial;
            require(on_static(trial, named_rectangle(trial.context.stage, side_b ? "upper_approach_b" : "upper_approach_a")),
                "Recorded source station did not begin on the unchanged upper approach");
            bool boarded = false;
            for (int step = 0; step < 180; ++step)
            {
                const auto state = trial.mover(side_b);
                recording.tick(input(steering(trial, state.current_bounds.center().x), step == 0, step < 18));
                if (trial.player().carrier_id() == state.id) { boarded = true; break; }
            }
            require(boarded, "Recorded held jump did not board the proposed mover");
            for (int step = 0; step < 90 && std::fabs(trial.player().velocity().x) > 0.01f; ++step)
            { recording.tick(); recording.require_carrier(); }
            recording.require_carrier();
            require(trial.player().velocity().x == 0.0f, "Recorded boarding did not brake personal velocity");
            const float local_x = trial.player().rectangle().x - trial.mover(side_b).current_bounds.x;
            const float full = trial.player().weapon_ammo();
            bool paid = false;
            int turns = 0, previous_direction = trial.mover(side_b).motion.direction;
            for (int step = 0; step < 244; ++step)
            {
                recording.tick(input(0.0f, false, false, false, step < 30));
                recording.require_carrier();
                const auto state = trial.mover(side_b);
                if (state.motion.direction != previous_direction) { ++turns; }
                previous_direction = state.motion.direction;
                paid = paid || trial.player().weapon_ammo() < full;
                require(std::fabs(trial.player().rectangle().x - state.current_bounds.x - local_x) < 0.06f &&
                    trial.player().velocity().x == 0.0f && trial.level->level_end_info().total_area == 165120.0f,
                    "Recorded idle carry changed local footing, personal speed or authoritative score");
            }
            require(turns >= 2 && paid && local_credit(trial.mover(side_b), side_b ? PlayerTeam::b : PlayerTeam::a) > 0.0,
                "Recorded route missed its paid raw-slot local paint or two carried reversals");
            if (!side_b)
            {
                bool ready = false;
                for (int step = 0; step < 360; ++step)
                {
                    const auto state = trial.mover(false);
                    recording.tick(input(steering(trial, state.current_bounds.x + 220.0f)));
                    recording.require_carrier();
                    const auto after = trial.mover(false);
                    if (after.current_bounds.right() >= 1230.0f &&
                        std::fabs(trial.player().center().x - after.current_bounds.x - 220.0f) < 8.0f &&
                        std::fabs(trial.player().velocity().x) < 1.0f) { ready = true; break; }
                }
                require(ready, "Recorded waiting bay did not offer the qualified inward jump");
                bool released = false, landed = false;
                const auto crossover = named_rectangle(trial.context.stage, "crossover_a");
                for (int step = 0; step < 180; ++step)
                {
                    recording.tick(input(steering(trial, 1500.0f), step == 0, step < 18));
                    if (step == 0) { released = trial.player().carrier_id() == 0; }
                    if (on_static(trial, crossover)) { landed = true; break; }
                }
                require(released && landed, "Recorded inward jump did not release and reach the static crossover");
            }
            else
            {
                recording.tick(input(0.0f, false, false, true));
                require(trial.player().carrier_id() == 0 && trial.player().rectangle().bottom() > 720.0f,
                    "Recorded one-way drop did not release its carrier");
            }
            recording.drive(mirrored(side_b, 300.0f));
            for (int step = 0; step < 120; ++step) { recording.tick(input(0.0f, false, false, true)); }
            require(on_static(trial, named_rectangle(trial.context.stage, "ground")), "Recorded home drop missed safe ground");
            recording.drive(mirrored(side_b, 1420.0f));
            require(on_static(trial, named_rectangle(trial.context.stage, side_b ? "lower_bed_b" : "lower_bed_a")) &&
                trial.player().weapon_ammo() >= full - 0.0001f,
                "Recorded optional route did not recover the original lower route and natural reserve");
        }

        void replay_side(Context& context, bool side_b)
        {
            const std::string name = side_b ? "replay-b-drop" : "replay-a-jump";
            const auto stage = station_variant(context, name, side_b ? 0.5f : 0.0f, side_b);
            const auto stage_bytes = read_text(stage);
            ReplayFile header;
            header.build = "turbulence-mover-public-route-after-source-prime-186";
            header.seed = side_b ? 440u : 439u; header.dt_bits = bits(dt);
            header.tuning_hash = fnv1a(read_text(context.content / "tuning.json"));
            header.presentation_hash = fnv1a(read_text(context.content / "presentation.json"));
            header.settings = settings(1, side_b ? PlayerTeam::b : PlayerTeam::a);
            header.settings.set_stage(stage.stem().string());
            auto original = std::make_unique<Trial>(context, stage, header.settings, *header.seed);
            const auto initial = MatchFingerprint::of(*original->level, 0);
            require(initial.horizontal_motion && initial.horizontal_motion->structures.size() == 2 &&
                initial.horizontal_motion->structures[0].id == 1 && initial.horizontal_motion->structures[1].id == 2,
                "Recorded public source lost canonical mover-only identities");
            const auto priming = prime(*original);
            const auto ready = MatchFingerprint::of(*original->level, 0);
            std::ofstream trace(context.evidence / (name + "-trace.csv"));
            require(trace.good(), "Cannot open mover replay trace");
            trace << "phase,update,hash,carrier,phase_bits,direction,own_raw_slot_local_area\n";
            trace << "initial,0," << hex64(initial.hash) << ",,,,\n";
            for (const auto& print : priming)
            { trace << "prime," << print.updates << ',' << hex64(print.hash) << ",,,,\n"; }
            trace << "ready,0," << hex64(ready.hash) << ",,,,\n";
            ReplayRecorder recorder(name, header, nullptr, 1800);
            RouteRecording recording(*original, recorder, side_b, trace);
            route_policy(recording, side_b);
            recorder.file().updates = static_cast<int>(recording.observations.size());
            recorder.file().final_print = recording.observations.back();
            const auto tape = context.evidence / (name + "-tape.json");
            require(!fs::exists(tape), "Refusing to overwrite an owned replay tape");
            recorder.file().write(tape);
            const unsigned int terminal_rng = static_cast<unsigned int>(std::rand());
            original.reset();

            const auto decoded = ReplayFile::read(tape);
            require(decoded.seed && decoded.dt_bits == bits(dt) && decoded.build == header.build &&
                decoded.tuning_hash == fnv1a(read_text(context.content / "tuning.json")) &&
                decoded.presentation_hash == fnv1a(read_text(context.content / "presentation.json")) &&
                !decoded.god && !decoded.infinite_ammo && decoded.inputs.size() == 1 &&
                decoded.inputs[0].size() == static_cast<size_t>(decoded.updates) &&
                decoded.updates == static_cast<int>(recording.observations.size()) &&
                decoded.checkpoints.size() == static_cast<size_t>(decoded.updates / ReplayFile::checkpoint_interval),
                "Mover replay lost source setup/hashes, real input-filter calls or checkpoints");
            // Both file selection and complete roster/seed/dt come from decoded
            // tape data. The constructor reloads the saved editor JSON and uses
            // the public builder; no scene pointer or state is copied forward.
            const auto rebuild_stage = context.evidence / (decoded.settings.stage() + ".json");
            auto playback = std::make_unique<Trial>(context, rebuild_stage, decoded.settings, *decoded.seed);
            require(MatchFingerprint::difference(initial, MatchFingerprint::of(*playback->level, 0)).empty(),
                "Fresh decoded-settings public build differs before source countdown");
            const auto replay_priming = prime(*playback);
            for (size_t step = 0; step < priming.size(); ++step)
            {
                const auto difference = MatchFingerprint::difference(priming[step], replay_priming[step]);
                require(difference.empty(), "Mover source countdown/settling diverged at " + std::to_string(step + 1) + ": " + difference);
                trace << "reprime," << replay_priming[step].updates << ',' << hex64(replay_priming[step].hash) << ",,,,\n";
            }
            require(MatchFingerprint::difference(ready, MatchFingerprint::of(*playback->level, 0)).empty(),
                "Fresh source-primed replay differs at the first recorded input");
            ReplayPlayer player(name, decoded);
            size_t checkpoint = 0;
            for (int step = 0; step < decoded.updates; ++step)
            {
                playback->tick(input(), &player, float_from_bits(decoded.dt_bits));
                const auto actual = MatchFingerprint::of(*playback->level, step + 1);
                const auto difference = MatchFingerprint::difference(recording.observations[static_cast<size_t>(step)], actual);
                require(difference.empty(), "Mover public playback diverged at " + std::to_string(step + 1) + ": " + difference);
                trace << "play," << step + 1 << ',' << hex64(actual.hash) << ',' << playback->player().carrier_id() << ',' <<
                    hex64(actual.horizontal_motion->structures[side_b ? 1u : 0u].phase) << ',' <<
                    playback->mover(side_b).motion.direction << ',' <<
                    local_credit(playback->mover(side_b), side_b ? PlayerTeam::b : PlayerTeam::a) << '\n';
                if ((step + 1) % ReplayFile::checkpoint_interval == 0)
                {
                    require(decoded.checkpoints.at(checkpoint).update == step + 1 &&
                        decoded.checkpoints.at(checkpoint).hash == actual.hash, "Mover replay checkpoint differs");
                    ++checkpoint;
                }
            }
            require(checkpoint == decoded.checkpoints.size() && MatchFingerprint::difference(decoded.final_print,
                MatchFingerprint::of(*playback->level, decoded.updates)).empty() &&
                static_cast<unsigned int>(std::rand()) == terminal_rng && read_text(rebuild_stage) == stage_bytes,
                "Mover replay final state, terminal RNG or editor source bytes differ");
            trace.close(); require(trace.good(), "Cannot finish mover replay trace");
            std::cout << "PASS " << name << ": fresh decoded-tape public disk rebuild, 186 exact source-priming updates, " <<
                decoded.updates << " exact active updates, " << checkpoint <<
                " checkpoints, carry/two reversals/paid local paint/release/lower-route recovery and terminal RNG\n";
        }
    }

    void replay(Context& context)
    {
        replay_side(context, false);
        replay_side(context, true);
    }
}
#else
namespace turbulence_movers_acceptance
{
    void replay(Context&)
    { std::cout << "LIMIT Turbulence mover seeded replay requires Debug dev tools; no Release replay qualification\n"; }
}
#endif
