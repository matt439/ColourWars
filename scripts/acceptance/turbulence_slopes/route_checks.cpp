#include "game/pch.h"
#include "scripts/acceptance/turbulence_slopes/fixtures.h"
#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace turbulence_slopes_acceptance
{
    namespace
    {
        using namespace mattmath;
        constexpr float tolerance = 0.06f;
        constexpr int maximum_trial_updates = 2400;

        std::string position(const Trial& trial)
        {
            std::ostringstream text;
            text << std::setprecision(std::numeric_limits<float>::max_digits10) << " updates=" << trial.updates <<
                " centre=" << trial.player().center().x << " feet=" << trial.player().rectangle().bottom() <<
                " vx=" << trial.player().velocity().x << " vy=" << trial.player().velocity().y <<
                " move_state=" << static_cast<int>(trial.player().move_state()) << " carrier=" << trial.player().carrier_id();
            return text.str();
        }

        void tick(Trial& trial, const PlayerInputData& action = input())
        {
            require(trial.updates < maximum_trial_updates, "Shoulder route exceeded its finite update budget" + position(trial));
            require(!action.jump_pressed && !action.jump_held, "Shoulder walk supplied an unintended jump input");
            trial.tick(action);
            for (const auto& effect : trial.player().movement_feedback().effects())
            {
                require(effect.age != 0.0f || effect.kind != MovementFeedbackKind::takeoff,
                    "Shoulder walk invented a takeoff" + position(trial));
            }
        }

        fs::path variant(Context& context, const std::string& label, bool side_b, float centre, float feet)
        {
            using namespace colourwars_editor;
            LevelDocument document(context.content / "tuning.json"); document.load(context.stage);
            const auto size = context.world.tuning.player().size;
            document.set_rectangle({ side_b ? EntryKind::spawn_b : EntryKind::spawn_a, 0, "" },
                { mirrored(side_b, centre) - size.x / 2.0f, feet - size.y, size.x, size.y });
            require(document.validate().empty(), "Editor rejected legal own-half shoulder route station");
            const auto output = context.evidence / (label + ".json");
            require(!fs::exists(output), "Refusing an earlier shoulder-route station"); document.save(output);
            LevelDocument reopened(context.content / "tuning.json"); reopened.load(output);
            require(reopened.serialize() == document.serialize() && reopened.validate().empty(),
                "Shoulder-route station changed during editor save/reopen");
            return output;
        }

        void on_flat(const Trial& trial, const RectangleF& surface, const std::string& label)
        {
            const auto& body = trial.player().rectangle();
            require(supported(trial.player()) && trial.player().carrier_id() == 0 &&
                std::fabs(body.bottom() - surface.top()) < tolerance && body.left() >= surface.left() - tolerance &&
                body.right() <= surface.right() + tolerance, label + position(trial));
        }

        void on_shoulder(const Trial& trial, bool side_b)
        {
            const auto& body = trial.player().rectangle();
            const float left = side_b ? 2600.0f : 1240.0f;
            const float right = left + 160.0f;
            const float support_x = side_b ? body.left() : body.right();
            const float support_y = side_b ? 800.0f + (support_x - left) * 0.5f : 880.0f - (support_x - left) * 0.5f;
            const auto expected = side_b ? PlayerMoveState::on_ramp_left : PlayerMoveState::on_ramp_right;
            require(trial.player().move_state() == expected && trial.player().carrier_id() == 0 &&
                body.left() >= left - tolerance && body.right() <= right + tolerance &&
                std::fabs(body.bottom() - support_y) < tolerance && trial.player().velocity().x == 0.0f,
                "Resting shoulder body did not retain its whole-body ramp footing" + position(trial));
        }

        void drive(Trial& trial, float centre, int limit = 600)
        {
            for (int step = 0; step < limit; ++step)
            {
                tick(trial, input(steering(trial, centre)));
                if (supported(trial.player()) && std::fabs(trial.player().center().x - centre) < 6.0f &&
                    trial.player().velocity().x == 0.0f) { return; }
            }
            throw std::runtime_error("Shoulder walk did not stop at its finite destination=" + std::to_string(centre) + position(trial));
        }

        void drop_to_ground(Trial& trial)
        {
            const auto ground = named_rectangle(trial.context.stage, "ground");
            for (int step = 0; step < 120; ++step)
            {
                tick(trial, input(0.0f, false, false, true));
                if (supported(trial.player()) && std::fabs(trial.player().rectangle().bottom() - ground.top()) < tolerance)
                { on_flat(trial, ground, "Home drop lost full-body ground support"); return; }
            }
            throw std::runtime_error("Home drop failed to rejoin the required lower route" + position(trial));
        }

        void recover(Trial& trial, bool side_b)
        {
            drive(trial, mirrored(side_b, 300.0f));
            drop_to_ground(trial);
            drive(trial, mirrored(side_b, 1420.0f));
            on_flat(trial, named_rectangle(trial.context.stage, side_b ? "lower_bed_b" : "lower_bed_a"),
                "Shoulder walk did not recover the original lower bed");
        }

        void receipt(std::ofstream& csv, const std::string& label, const char* event,
            const Trial& trial, bool side_b, float requested_speed)
        {
            const auto& body = trial.player().rectangle();
            const auto mover = trial.mover(side_b);
            csv << label << ',' << event << ',' << (side_b ? 'b' : 'a') << ',' << requested_speed << ',' << trial.updates << ',' <<
                body.x << ',' << body.y << ',' << body.width << ',' << body.height << ',' << trial.player().center().x << ',' << body.bottom() << ',' <<
                trial.player().velocity().x << ',' << trial.player().velocity().y << ',' << static_cast<int>(trial.player().move_state()) << ',' <<
                supported(trial.player()) << ',' << trial.player().carrier_id() << ',' << mover.motion.phase << ',' <<
                trial.player().health() << ',' << trial.player().weapon_ammo() << '\n';
            csv.flush(); require(csv.good(), "Cannot flush actual shoulder-route evidence");
        }

        int shoulder_route(Context& context, std::ofstream& csv, bool side_b, bool uphill, float requested_speed)
        {
            const std::string label = std::string(side_b ? "b-" : "a-") + (uphill ? "uphill-" : "downhill-") +
                "speed" + std::to_string(static_cast<int>(requested_speed));
            const auto path = variant(context, label, side_b, uphill ? 906.0f : 1860.0f, uphill ? 880.0f : 800.0f);
            Trial trial(context, path, settings(1, side_b ? PlayerTeam::b : PlayerTeam::a));
            trial.start();
            on_flat(trial, named_rectangle(context.stage, uphill ? (side_b ? "upper_approach_b" : "upper_approach_a") :
                (side_b ? "crossover_b" : "crossover_a")), "Authored shoulder start did not settle on its existing platform");
            receipt(csv, label, "settled-before-driving", trial, side_b, requested_speed);
            const float direction = (uphill != side_b) ? 1.0f : -1.0f;
            if (requested_speed > 0.0f)
            {
                bool reached = false;
                for (int step = 0; step < 90; ++step)
                {
                    tick(trial, input(direction));
                    if (std::fabs(trial.player().velocity().x) >= requested_speed - 0.03f)
                    { reached = true; break; }
                }
                require(reached && std::fabs(std::fabs(trial.player().velocity().x) - requested_speed) < 0.03f &&
                    trial.player().velocity().x * direction > 0.0f,
                    "Ordinary shoulder approach did not produce its measured personal-speed mark" + position(trial));
            }
            else { require(trial.player().velocity().x == 0.0f, "Rest shoulder trial did not begin at rest"); }
            // This is the actual driven mark, including its world position.
            // The uphill800 mark can occur on the ramp, not before entry.
            receipt(csv, label, "actual-driven-speed-mark", trial, side_b, requested_speed);
            if (requested_speed == 0.0f)
            {
                drive(trial, mirrored(side_b, 1320.0f)); on_shoulder(trial, side_b);
                receipt(csv, label, "mid-ramp-whole-body-stop", trial, side_b, requested_speed);
            }
            drive(trial, mirrored(side_b, uphill ? 1500.0f : 1060.0f));
            on_flat(trial, named_rectangle(context.stage, uphill ? (side_b ? "crossover_b" : "crossover_a") :
                (side_b ? "upper_approach_b" : "upper_approach_a")), "Shoulder receiving stop did not fit the complete player body");
            receipt(csv, label, "flat-receiver-stopped", trial, side_b, requested_speed);
            recover(trial, side_b);
            receipt(csv, label, "original-lower-bed-recovered", trial, side_b, requested_speed);
            return trial.updates;
        }

        int lower_route(Context& context, std::ofstream& csv, bool side_b)
        {
            const std::string label = side_b ? "b-original-lower" : "a-original-lower";
            const auto path = variant(context, label, side_b, 300.0f, 1120.0f);
            Trial trial(context, path, settings(1, side_b ? PlayerTeam::b : PlayerTeam::a)); trial.start();
            on_flat(trial, named_rectangle(context.stage, side_b ? "home_floor_b" : "home_floor_a"), "Original home did not settle");
            drop_to_ground(trial); receipt(csv, label, "home-drop-ground", trial, side_b, 0.0f);
            drive(trial, mirrored(side_b, 1420.0f));
            on_flat(trial, named_rectangle(context.stage, side_b ? "lower_bed_b" : "lower_bed_a"), "Original near bed route changed");
            receipt(csv, label, "near-bed-stopped", trial, side_b, 0.0f);
            drive(trial, 2000.0f); on_flat(trial, named_rectangle(context.stage, "ground"), "Original lower centre route changed");
            receipt(csv, label, "lower-centre-stopped", trial, side_b, 0.0f);
            drive(trial, mirrored(side_b, 3700.0f)); on_flat(trial, named_rectangle(context.stage, "ground"), "Original far lower home route changed");
            receipt(csv, label, "far-lower-home-stopped", trial, side_b, 0.0f);
            return trial.updates;
        }
    }

    void routes(Context& context)
    {
        std::ofstream csv(context.evidence / "shoulder-routes.csv");
        require(csv.good(), "Cannot open shoulder route receipts");
        csv << std::setprecision(std::numeric_limits<double>::max_digits10) <<
            "trial,event,side,requested_personal_speed,update,x,y,width,height,centre_x,feet_y,actual_personal_vx,personal_vy,"
            "move_state,supported,carrier_id,own_mover_phase,health,ammo\n";
        int route_count = 0, updates = 0;
        for (const bool side_b : { false, true })
        {
            for (const bool uphill : { true, false })
            {
                for (const float speed : { 0.0f, 415.0f, 800.0f })
                { updates += shoulder_route(context, csv, side_b, uphill, speed); ++route_count; }
            }
            updates += lower_route(context, csv, side_b); ++route_count;
        }
        csv.close(); require(csv.good() && route_count == 14 && updates <= maximum_trial_updates * 14,
            "Shoulder route receipt/update inventory differs");
        std::ofstream summary(context.evidence / "shoulder-routes-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"existing-ramp-flat-shoe-driven-routes\",\"shoulder_trials\":12,"
            "\"original_lower_trials\":2,\"maximum_trial_updates\":2400,\"total_updates\":" << updates <<
            ",\"jump_inputs\":0,\"post_construction_placements\":0,\"speed_marks_driven\":true,\"human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot finish shoulder route summary");
        std::cout << "PASS12 actual shoulder walks at driven0/415/800 personal-speed marks: mirrored uphill/downhill, "
            "whole-body mid-ramp/flat stops and ordinary lower recovery;2 original home/lower crossings, no jumps or post-build placements\n";
    }
}
