#include "game/pch.h"
#include "scripts/acceptance/turbulence_movers/fixtures.h"
#include <iomanip>
#include <iostream>

namespace turbulence_movers_acceptance
{
    namespace
    {
        constexpr float tolerance = 0.06f;

        std::string position(const Trial& trial)
        {
            const auto& body = trial.player().rectangle();
            return " at update " + std::to_string(trial.updates) + " centre=" +
                std::to_string(trial.player().center().x) + " feet=" + std::to_string(body.bottom()) +
                " vx=" + std::to_string(trial.player().velocity().x) + " carrier=" +
                std::to_string(trial.player().carrier_id());
        }

        void on_static(const Trial& trial, const mattmath::RectangleF& support, const std::string& label)
        {
            const auto& body = trial.player().rectangle();
            require(supported(trial.player()) && trial.player().carrier_id() == 0 &&
                std::fabs(body.bottom() - support.top()) < tolerance &&
                body.left() >= support.left() - tolerance && body.right() <= support.right() + tolerance,
                label + position(trial));
        }

        void on_mover(const Trial& trial, bool side_b)
        {
            const auto state = trial.mover(side_b);
            const auto& body = trial.player().rectangle();
            require(supported(trial.player()) && trial.player().carrier_id() == state.id &&
                std::fabs(body.bottom() - state.current_bounds.top()) < tolerance &&
                body.left() >= state.current_bounds.left() - tolerance &&
                body.right() <= state.current_bounds.right() + tolerance,
                "Whole player body lost actual moving top support" + position(trial));
        }

        void drive(Trial& trial, float target, int limit = 360)
        {
            for (int step = 0; step < limit; ++step)
            {
                trial.tick(input(steering(trial, target)));
                if (supported(trial.player()) && std::fabs(trial.player().center().x - target) < 6.0f &&
                    std::fabs(trial.player().velocity().x) < 0.01f) { return; }
            }
            throw std::runtime_error("Driven mover escape did not stop at its finite target" + position(trial));
        }

        void recover(Trial& trial, bool side_b)
        {
            // Follow the existing longer home loop. No position/velocity setter,
            // cheat, reset or respawn can turn a failed optional bay into success.
            drive(trial, mirrored(side_b, 300.0f));
            for (int step = 0; step < 120; ++step) { trial.tick(input(0.0f, false, false, true)); }
            on_static(trial, named_rectangle(trial.context.stage, "ground"), "Home drop did not recover ground");
            drive(trial, mirrored(side_b, 1420.0f));
            on_static(trial, named_rectangle(trial.context.stage, side_b ? "lower_bed_b" : "lower_bed_a"),
                "Optional mover escape did not rejoin the original lower route");
        }

        double credited(const HorizontalStructureState& state, PlayerTeam team, int owner)
        {
            double result = 0.0;
            for (const auto& tile : state.local_paint)
            { if (tile.team == team && tile.paint_owner == owner) { result += tile.scored_area; } }
            return result;
        }

        void receipt(std::ofstream& csv, const std::string& label, const std::string& event,
            const Trial& trial, bool side_b)
        {
            const auto state = trial.mover(side_b);
            csv << label << ',' << event << ',' << trial.updates << ',' << state.motion.phase << ',' <<
                state.motion.direction << ',' << state.current_bounds.x << ',' << trial.player().center().x << ',' <<
                trial.player().rectangle().bottom() << ',' << trial.player().velocity().x << ',' <<
                trial.player().velocity().y << ',' << trial.player().carrier_id() << ',' << trial.player().weapon_ammo() << ',' <<
                credited(state, side_b ? PlayerTeam::b : PlayerTeam::a, input_slots::keyboard_and_mouse) << ',' <<
                trial.player().health() << '\n';
            csv.flush(); require(csv.good(), "Cannot write mover route evidence");
        }

        void ride(Context& context, std::ofstream& csv, bool side_b, float phase, int margin, bool jump_exit)
        {
            const std::string label = std::string(side_b ? "b" : "a") + "-phase" + std::to_string(phase) +
                "-margin" + std::to_string(margin);
            const auto path = station_variant(context, label, phase, side_b);
            Trial trial(context, path, settings(1, side_b ? PlayerTeam::b : PlayerTeam::a));
            trial.start();
            on_static(trial, named_rectangle(context.stage, side_b ? "upper_approach_b" : "upper_approach_a"),
                "Authored upper approach station did not settle");
            for (int step = 0; step < margin + 2; ++step) { trial.tick(); }
            receipt(csv, label, "board-takeoff", trial, side_b);
            bool boarded = false;
            for (int step = 0; step < 180; ++step)
            {
                const auto state = trial.mover(side_b);
                trial.tick(input(steering(trial, state.current_bounds.center().x), step == 0, step < 18));
                if (trial.player().carrier_id() == state.id) { boarded = true; break; }
            }
            require(boarded, "One held upper-approach jump did not board its optional mover: " + label + position(trial));
            receipt(csv, label, "board-contact", trial, side_b);
            // Release the personal input; the concrete Level carrier must supply
            // translation without inventing personal launch velocity.
            for (int step = 0; step < 90 && std::fabs(trial.player().velocity().x) > 0.01f; ++step)
            { trial.tick(); on_mover(trial, side_b); }
            on_mover(trial, side_b);
            require(trial.player().velocity().x == 0.0f, "Boarding personal speed did not brake" + position(trial));
            const float local_x = trial.player().rectangle().x - trial.mover(side_b).current_bounds.x;
            const float full = trial.player().weapon_ammo();
            bool paid = false;
            int reversals = 0, previous_direction = trial.mover(side_b).motion.direction;
            for (int step = 0; step < 244; ++step)
            {
                trial.tick(input(0.0f, false, false, false, step < 30));
                on_mover(trial, side_b);
                const auto state = trial.mover(side_b);
                if (state.motion.direction != previous_direction) { ++reversals; }
                previous_direction = state.motion.direction;
                paid = paid || trial.player().weapon_ammo() < full;
                require(std::fabs(trial.player().rectangle().x - state.current_bounds.x - local_x) < tolerance &&
                    trial.player().velocity().x == 0.0f && trial.level->level_end_info().total_area == 165120.0f,
                    "Idle carry changed local footing, personal speed or local score" + position(trial));
            }
            require(reversals >= 2 && paid && credited(trial.mover(side_b), side_b ? PlayerTeam::b : PlayerTeam::a,
                input_slots::keyboard_and_mouse) > 0.0,
                "Carried paid burst did not retain raw-slot local paint through two reversals: " + label);
            receipt(csv, label, "paid-carry-two-reversals", trial, side_b);
            if (jump_exit)
            {
                // Approach the inward part of the deck, then explicitly wait for
                // its inward endpoint. Carried speed is not counted as launch vx.
                bool launch_ready = false;
                for (int step = 0; step < 360; ++step)
                {
                    const auto state = trial.mover(side_b);
                    const float local_goal = side_b ? 100.0f : 220.0f;
                    trial.tick(input(steering(trial, state.current_bounds.x + local_goal)));
                    on_mover(trial, side_b);
                    const auto after = trial.mover(side_b);
                    if ((side_b ? after.current_bounds.left() <= 2770.0f : after.current_bounds.right() >= 1230.0f) &&
                        std::fabs(trial.player().center().x - after.current_bounds.x - local_goal) < 8.0f &&
                        std::fabs(trial.player().velocity().x) < 1.0f) { launch_ready = true; break; }
                }
                require(launch_ready, "Inward waiting bay never offered a controlled jump departure" + position(trial));
                receipt(csv, label, "inward-wait-finished", trial, side_b);
                bool landed = false, released = false;
                const auto crossover = named_rectangle(context.stage, side_b ? "crossover_b" : "crossover_a");
                for (int step = 0; step < 180; ++step)
                {
                    trial.tick(input(steering(trial, mirrored(side_b, 1500.0f)), step == 0, step < 18));
                    released = released || trial.player().carrier_id() == 0;
                    const auto& body = trial.player().rectangle();
                    if (supported(trial.player()) && trial.player().carrier_id() == 0 &&
                        std::fabs(body.bottom() - crossover.top()) < tolerance &&
                        body.left() >= crossover.left() && body.right() <= crossover.right()) { landed = true; break; }
                }
                require(released && landed, "Optional inward jump did not reach the existing crossover" + position(trial));
                receipt(csv, label, "jump-to-static-crossover", trial, side_b);
            }
            else
            {
                trial.tick(input(0.0f, false, false, true));
                require(trial.player().carrier_id() == 0 && trial.player().rectangle().bottom() > 720.0f,
                    "One-way drop did not release moving support" + position(trial));
                receipt(csv, label, "drop-release", trial, side_b);
            }
            recover(trial, side_b);
            require(trial.player().weapon_ammo() >= full - 0.0001f,
                "Ordinary escape did not permit natural paid-shot reserve recovery");
            receipt(csv, label, "original-lower-route-recovered", trial, side_b);
        }

        void missed_tap(Context& context, std::ofstream& csv, bool side_b)
        {
            const std::string label = side_b ? "b-tap-miss" : "a-tap-miss";
            Trial trial(context, station_variant(context, label, 0.0f, side_b),
                settings(1, side_b ? PlayerTeam::b : PlayerTeam::a));
            trial.start();
            bool carrier = false;
            for (int step = 0; step < 100; ++step)
            {
                trial.tick(input(0.0f, step == 0, step == 0));
                carrier = carrier || trial.player().carrier_id() != 0;
            }
            require(!carrier, "A tap unexpectedly boarded the 160-unit optional rise");
            recover(trial, side_b);
            receipt(csv, label, "tap-miss-original-route-recovered", trial, side_b);
        }
    }

    void routes(Context& context, bool full_matrix)
    {
        std::ofstream csv(context.evidence / "mover-routes.csv");
        require(csv.good(), "Cannot open actual mover route evidence");
        csv << std::setprecision(std::numeric_limits<double>::max_digits10);
        csv << "trial,event,update,actual_phase,outgoing_direction,mover_x,centre_x,feet_y,personal_vx,personal_vy,"
            "carrier_id,ammo,own_raw_slot_local_area,health\n";
        const std::vector<float> phases = full_matrix ? std::vector<float>{ 0.0f, 0.25f, 0.5f, 0.75f } :
            std::vector<float>{ 0.0f, 0.5f };
        const std::vector<int> margins = full_matrix ? std::vector<int>{ -2, 0, 2 } : std::vector<int>{ 0 };
        int trials = 0;
        for (const bool side_b : { false, true })
        {
            for (const float phase : phases)
            {
                for (const int margin : margins)
                { ride(context, csv, side_b, phase, margin, phase < 0.5f); ++trials; }
            }
            missed_tap(context, csv, side_b); ++trials;
        }
        csv.close(); require(csv.good(), "Cannot finish actual mover route evidence");
        std::cout << "PASS " << trials << " real Level mover route trials: one held boarding jump, paid local paint, "
            "two carried reversals, explicit inward wait/jump or drop, and unchanged lower-route recovery\n";
    }
}
