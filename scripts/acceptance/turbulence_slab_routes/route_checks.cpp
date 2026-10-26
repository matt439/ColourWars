#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_routes/fixtures.h"
#include "game/objects/level_builder.h"
#include "editor/level_document.h"
#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace turbulence_slab_routes_acceptance
{
    namespace
    {
        using namespace mattmath;
        using namespace labrador;
        using namespace colourwars_editor;
        using moving_structure_acceptance::World;
        constexpr float tolerance = 0.06f;
        constexpr int maximum_scene_updates = 1000;
        constexpr int maximum_matrix_updates = maximum_scene_updates * 8;
        constexpr unsigned int seed = 260026u;

        std::string read(const fs::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            require(file.good(), "Cannot read public Roller-route input: " + path.string());
            std::string value{ std::istreambuf_iterator<char>(file), {} };
            require(!file.bad(), "Cannot finish public Roller-route input: " + path.string());
            return value;
        }

        rapidjson::Document parse(const std::string& text)
        {
            rapidjson::Document result; result.Parse(text.c_str());
            require(!result.HasParseError() && result.IsObject(), "Cannot parse public Roller-route source");
            return result;
        }

        struct SourceGuard
        {
            std::vector<std::pair<fs::path, std::string>> sources;
            SourceGuard(const fs::path& content, const fs::path& slab, const fs::path& shoulder)
            {
                for (const char* file : { "manifest.json", "tuning.json", "presentation.json" })
                { sources.emplace_back(content / file, read(content / file)); }
                for (const auto& file : fs::directory_iterator(content / "levels"))
                {
                    if (file.is_regular_file() && file.path().extension() == ".json")
                    { sources.emplace_back(file.path(), read(file.path())); }
                }
                require(sources.size() == 11, "Roller-route guard must preserve eight source arenas");
                sources.emplace_back(slab, read(slab)); sources.emplace_back(shoulder, read(shoulder));
            }
            void check() const
            {
                for (const auto& [path, before] : sources)
                { require(read(path) == before, "Roller route changed source content: " + path.string()); }
            }
        };

        RectangleF rectangle(const rapidjson::Value& object)
        {
            const auto& r = object["rectangle"];
            return { r["x"].GetFloat(), r["y"].GetFloat(), r["width"].GetFloat(), r["height"].GetFloat() };
        }

        float mirror(bool side_b, float x) { return side_b ? 4000.0f - x : x; }
        bool supported(const Player& player)
        {
            const auto state = player.move_state();
            return state == PlayerMoveState::on_ground || state == PlayerMoveState::on_drop_down_ground ||
                state == PlayerMoveState::on_ramp_left || state == PlayerMoveState::on_ramp_right;
        }

        struct Geometry
        {
            const char* label;
            fs::path stage;
            bool slab;
            int original_ramps = 0;
        };

        struct Roles
        {
            std::size_t receiver = 0, upper = 0, crossover = 0;
            RectangleF upper_body, crossover_body;
        };

        Roles roles(const rapidjson::Document& document, const Geometry& geometry, bool side_b)
        {
            const std::string suffix = side_b ? "_b" : "_a";
            const std::string receiver = (geometry.slab ? "fixed_slab" : "shoulder_shoe") + suffix;
            const std::array<std::string, 3> names{ receiver, "upper_approach" + suffix, "crossover" + suffix };
            std::array<bool, 3> found{};
            Roles result;
            std::size_t ordinal = 0;
            int ramps = 0, slabs = 0;
            for (const auto& object : document["collision_objects"].GetArray())
            {
                const std::string type = object["type"].GetString();
                ramps += type == "StructureRamp" ? 1 : 0; slabs += type == "FixedSlabPaintable" ? 1 : 0;
                if (type != "StructurePaintable" && type != "FixedSlabPaintable") { continue; }
                const std::string name = object["name"].GetString();
                for (std::size_t group = 0; group < names.size(); ++group)
                {
                    if (name != names[group]) { continue; }
                    require(!found[group], "Duplicate public Roller-route role"); found[group] = true;
                    if (group == 0) { result.receiver = ordinal; }
                    if (group == 1) { result.upper = ordinal; result.upper_body = rectangle(object); }
                    if (group == 2) { result.crossover = ordinal; result.crossover_body = rectangle(object); }
                }
                ++ordinal;
            }
            require(std::all_of(found.begin(), found.end(), [](bool value) { return value; }), "Missing public Roller-route paint role");
            require(slabs == (geometry.slab ? 2 : 0) &&
                ramps == geometry.original_ramps + (geometry.slab ? 0 : 2),
                "Comparison must retain scored slabs versus two existing unscored ramps plus flat shoes");
            return result;
        }

        fs::path station(const fs::path& content, const Geometry& geometry, const fs::path& evidence,
            const std::string& label, bool side_b, bool uphill, const Vector2F& player_size)
        {
            LevelDocument document(content / "tuning.json"); document.load(geometry.stage);
            const float centre = mirror(side_b, uphill ? 1160.0f : 1540.0f);
            const float feet = uphill ? 880.0f : 800.0f;
            document.set_rectangle({ side_b ? EntryKind::spawn_b : EntryKind::spawn_a, 0, "" },
                { centre - player_size.x / 2.0f, feet - player_size.y, player_size.x, player_size.y });
            const auto output = evidence / (label + ".json");
            require(!fs::exists(output), "Refusing previous public Roller station"); document.save(output);
            LevelDocument reopened(content / "tuning.json"); reopened.load(output);
            require(reopened.serialize() == document.serialize() && reopened.validate().empty(),
                "Public Roller station changed during actual editor save/reopen");
            const auto before = parse(read(geometry.stage)), after = parse(read(output));
            require(after["collision_objects"] == before["collision_objects"] &&
                after["non_collision_objects"] == before["non_collision_objects"],
                "Public Roller station changed trial geometry");
            for (auto field = before.MemberBegin(); field != before.MemberEnd(); ++field)
            {
                if (std::string(field->name.GetString()) == "spawn_locations") { continue; }
                require(after.HasMember(field->name) && after[field->name] == field->value,
                    "Public Roller station changed non-spawn metadata");
            }
            const char* own = side_b ? "team_b" : "team_a";
            const char* other = side_b ? "team_a" : "team_b";
            require(after["spawn_locations"][other] == before["spawn_locations"][other], "Public Roller station changed the other team's spawns");
            for (rapidjson::SizeType index = 1; index < before["spawn_locations"][own].Size(); ++index)
            { require(after["spawn_locations"][own][index] == before["spawn_locations"][own][index], "Public Roller station changed an unused own-team spawn"); }
            return output;
        }

        struct Paint
        {
            std::array<double, 4> receiver{}, upper{}, crossover{};
            double total_owned = 0.0, total_scored = 0.0;
        };

        struct Trial
        {
            World& world;
            Registry<LevelLoadedInfo> infos{ "Public driven slab Roller route" };
            ResolutionManager resolution;
            ViewportManager viewports{ &resolution };
            Mouse mouse;
            Partitioner partitioner;
            std::unique_ptr<Level> level;
            RectangleF bounds;
            int updates = 0, paid = 0, held = 0, airborne = 0, slope = 0;
            float paid_cost = 0.0f;
            int owner = -1;
            Roles map;

            Trial(World& with, const fs::path& stage, const Roles& groups, bool side_b) : world(with), map(groups)
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(stage).c_str()); info->validate();
                const LevelObjectBuilder objects(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
                info->validate_content(objects, world.render, world.audio); bounds = info->out_of_bounds_rectangle();
                infos.add("turbulence_roller_route", std::move(info));
                MenuLevelSettings settings;
                settings.set_stage("turbulence_roller_route"); settings.set_player_count(1);
                settings.set_screen_layout(ScreenLayout::one_player); settings.begin_input_claims(input_slots::keyboard_and_mouse);
                settings.set_player_team(0, side_b ? PlayerTeam::b : PlayerTeam::a); settings.set_player_weapon(0, WeaponType::roller);
                viewports.set_layout(settings.screen_layout());
                LevelBuilder builder(&viewports, &world.render, &world.audio, &world.preferences, &infos,
                    &world.tuning, &world.presentation, &resolution, nullptr, &partitioner, &mouse);
                std::srand(seed); level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
                owner = paint_owners::index(player().player_num(), 0);
                require(owner == 4 && player().primary_weapon().type() == WeaponType::roller,
                    "Public Roller route lost actual device-less owner4 or tool");
                check();
            }

            const Player& player() const { return level->player(0); }
            std::string pose() const
            {
                std::ostringstream out; out << " update=" << updates << " centre=" << player().center().x <<
                    " feet=" << player().rectangle().bottom() << " vx=" << player().velocity().x <<
                    " state=" << static_cast<int>(player().move_state()); return out.str();
            }
            void check() const
            {
                const auto& body = player().rectangle();
                require(player().state() == PlayerState::alive && player().health() == 1.0f,
                    "Public Roller route died or lost health" + pose());
                require(std::isfinite(body.x) && std::isfinite(body.y) && std::isfinite(player().velocity().x) &&
                    std::isfinite(player().velocity().y) && body.left() >= bounds.left() && body.right() <= bounds.right() &&
                    body.top() >= bounds.top() && body.bottom() <= bounds.bottom(), "Public Roller route left finite whole-body bounds" + pose());
                require(player().carrier_id() == 0, "Fixed shoulder Roller route unexpectedly borrowed mover carry" + pose());
                require(std::isfinite(player().weapon_ammo()) && player().weapon_ammo() >= 0.0f && player().weapon_ammo() <= 1.0f,
                    "Public Roller route has invalid ammunition" + pose());
                for (const auto& effect : player().movement_feedback().effects())
                { require(effect.kind != MovementFeedbackKind::takeoff || effect.age != 0.0f, "Jump-free public Roller route created a takeoff" + pose()); }
            }
            void tick(float movement = 0.0f, bool fire = false, bool observe_route = false)
            {
                require(updates < maximum_scene_updates, "Public Roller route exceeded its finite update budget" + pose());
                const bool paid_before = fire && player().primary_weapon().ready_to_shoot();
                const float ammo_before = player().weapon_ammo();
                class Input final : public SeatInputFilter
                {
                public:
                    Input(float movement, bool fire) : movement_(movement), fire_(fire) {}
                    void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
                    {
                        require(seats.size() == 1, "Public Roller route unexpectedly gained a seat");
                        seats[0] = {}; seats[0].connected = true;
                        seats[0].x_movement = movement_; seats[0].primary_shoot = fire_;
                    }
                private: float movement_; bool fire_;
                } input(movement, fire);
                level->update({}, moving_structure_acceptance::dt, &input); ++updates;
                if (fire) { ++held; }
                if (paid_before)
                {
                    const float cost = world.tuning.weapon(WeaponType::roller).details.ammo_usage;
                    const float actual_cost = ammo_before - player().weapon_ammo();
                    require(cost > 0.0f && std::abs(actual_cost - cost) <= 0.000003f,
                        "Actual held Roller pulse did not pay its source-tuned cost" + pose());
                    ++paid; paid_cost += actual_cost;
                }
                else if (fire)
                { require(player().weapon_ammo() >= ammo_before, "Roller ammunition fell outside an actual ready trigger update" + pose()); }
                if (observe_route)
                {
                    airborne += !supported(player()) ? 1 : 0;
                    slope += player().move_state() == PlayerMoveState::on_ramp_left ||
                        player().move_state() == PlayerMoveState::on_ramp_right ? 1 : 0;
                }
                check();
            }
            Paint paint() const
            {
                Paint result;
                std::array<bool, 3> found{};
                for (const auto& sample : level->paint_surfaces())
                {
                    result.total_scored += sample.scored_area;
                    found[0] = found[0] || sample.source_index == map.receiver;
                    found[1] = found[1] || sample.source_index == map.upper;
                    found[2] = found[2] || sample.source_index == map.crossover;
                    if (sample.team == PlayerTeam::none) { continue; }
                    require(sample.team == player().team() && sample.paint_owner == owner,
                        "Solo public Roller paint lost actual team/raw-owner credit");
                    result.total_owned += sample.scored_area;
                    const std::size_t face = static_cast<std::size_t>(sample.face);
                    if (sample.source_index == map.receiver) { result.receiver[face] += sample.scored_area; }
                    if (sample.source_index == map.upper) { result.upper[face] += sample.scored_area; }
                    if (sample.source_index == map.crossover) { result.crossover[face] += sample.scored_area; }
                }
                require(std::all_of(found.begin(), found.end(), [](bool value) { return value; }),
                    "Public collector lost a Roller receiver or original fixed-floor source");
                return result;
            }
        };

        void flat_stop(const Trial& trial, const RectangleF& floor, const char* label)
        {
            const auto& body = trial.player().rectangle();
            require(supported(trial.player()) && std::abs(body.bottom() - floor.top()) < tolerance &&
                body.left() >= floor.left() - tolerance && body.right() <= floor.right() + tolerance &&
                trial.player().velocity().x == 0.0f, std::string(label) + trial.pose());
        }

        struct Result
        {
            RectangleF initial, final;
            float initial_ammo = 0.0f, arrival_ammo = 0.0f, final_ammo = 0.0f;
            float initial_health = 0.0f, final_health = 0.0f, paid_cost = 0.0f;
            int updates = 0, drive_updates = 0, braking_updates = 0, held = 0, paid = 0, airborne = 0, slope = 0;
            int first_top = -1;
            Paint final_paint;
        };

        void trace_row(std::ofstream& trace, const std::string& label, const char* phase, const Trial& trial)
        {
            const Paint paint = trial.paint();
            trace << label << ',' << phase << ',' << trial.updates << ',' << trial.player().center().x << ',' <<
                trial.player().rectangle().bottom() << ',' << trial.player().velocity().x << ',' << trial.player().velocity().y << ',' <<
                static_cast<int>(trial.player().move_state()) << ',' << supported(trial.player()) << ',' << trial.player().health() << ',' <<
                trial.player().weapon_ammo() << ',' << trial.held << ',' << trial.paid << ',' << trial.paid_cost << ',' <<
                paint.receiver[0] << ',' << paint.receiver[1] << ',' << paint.receiver[2] << ',' << paint.receiver[3] << ',' <<
                paint.upper[0] << ',' << paint.crossover[0] << ',' << paint.total_owned << '\n';
            require(trace.good(), "Cannot write actual public Roller-route trace");
        }

        Result measure(World& world, const fs::path& content, const Geometry& geometry, const fs::path& evidence,
            std::ofstream& trace, bool side_b, bool uphill)
        {
            const std::string label = std::string(geometry.label) + (side_b ? "-b-" : "-a-") + (uphill ? "uphill" : "downhill");
            const auto path = station(content, geometry, evidence, label, side_b, uphill, world.tuning.player().size);
            const auto groups = roles(parse(read(path)), geometry, side_b);
            Trial trial(world, path, groups, side_b);
            int count_in = 0;
            while (!trial.level->match_started() && count_in < 240) { trial.tick(); ++count_in; }
            require(trial.level->match_started() && count_in == 181, "Public Roller route changed source-tuned181-update count-in");
            for (int tick = 0; tick < 60; ++tick) { trial.tick(); }
            flat_stop(trial, uphill ? groups.upper_body : groups.crossover_body, "Public Roller station did not settle at its original flat source");
            require(trial.player().primary_weapon().ready_to_shoot() && trial.player().weapon_ammo() == 1.0f &&
                trial.paint().total_owned == 0.0, "Public Roller route did not begin neutral, ready and fully funded");
            Result result;
            result.initial = trial.player().rectangle(); result.initial_ammo = trial.player().weapon_ammo();
            result.initial_health = trial.player().health();
            trace_row(trace, label, "initial-ready", trial);
            const float target = mirror(side_b, uphill ? 1500.0f : 1160.0f);
            const float direction = target > trial.player().center().x ? 1.0f : -1.0f;
            bool braking = false;
            for (int step = 0; step < 480; ++step)
            {
                const float error = std::abs(target - trial.player().center().x);
                const float speed = std::abs(trial.player().velocity().x);
                const float stop = speed * speed / (2.0f * world.tuning.player().x_deacceleration) +
                    speed * moving_structure_acceptance::dt + 4.0f;
                if (!braking && speed > 0.0f && error <= stop) { braking = true; }
                trial.tick(braking ? 0.0f : direction, !braking, true);
                result.drive_updates += !braking ? 1 : 0; result.braking_updates += braking ? 1 : 0;
                if (result.first_top < 0 && trial.paint().receiver[0] > 0.0)
                { result.first_top = step + 1; }
                trace_row(trace, label, braking ? "release-braking" : "held-roller-walk", trial);
                if (braking && trial.player().velocity().x == 0.0f && supported(trial.player())) { break; }
            }
            require(braking, "Public Roller route never reached its bounded release/braking point" + trial.pose());
            flat_stop(trial, uphill ? groups.crossover_body : groups.upper_body,
                "Public Roller walk did not finish fully stopped on its fixed receiving floor");
            require(trial.held > 0 && trial.paid > 0, "Public held Roller route produced no actual paid trigger updates");
            result.arrival_ammo = trial.player().weapon_ammo();
            trace_row(trace, label, "receiver-stopped", trial);
            for (int tick = 0; tick < 180; ++tick) { trial.tick(); }
            flat_stop(trial, uphill ? groups.crossover_body : groups.upper_body,
                "Public Roller route lost receiving support during quiet observation");
            result.final = trial.player().rectangle(); result.final_ammo = trial.player().weapon_ammo();
            result.final_health = trial.player().health(); result.paid_cost = trial.paid_cost;
            result.updates = trial.updates; result.held = trial.held; result.paid = trial.paid;
            result.airborne = trial.airborne; result.slope = trial.slope; result.final_paint = trial.paint();
            trace_row(trace, label, "final-quiet", trial);
            std::cout << "MEASURE public Roller " << label << " updates=" << result.updates << " paid=" << result.paid <<
                " receiver_top=" << result.final_paint.receiver[0] << " receiver_bottom=" << result.final_paint.receiver[1] <<
                " total_owned=" << result.final_paint.total_owned << " airborne=" << result.airborne << '\n';
            return result;
        }
    }

    void run(const fs::path& content, const fs::path& slab, const fs::path& shoulder, const fs::path& evidence)
    {
        SourceGuard originals(content, slab, shoulder);
        const auto source = parse(read(content / "levels/turbulence_mover_trial.json"));
        require(source["collision_objects"].Size() == 22, "Public Roller comparison lost its qualified22-object baseline");
        int original_ramps = 0;
        for (const auto& object : source["collision_objects"].GetArray())
        { original_ramps += std::string(object["type"].GetString()) == "StructureRamp" ? 1 : 0; }
        const std::array<Geometry, 2> geometries{ Geometry{ "slab", slab, true, original_ramps },
            Geometry{ "composite", shoulder, false, original_ramps } };
        for (const auto& geometry : geometries)
        {
            const auto trial = parse(read(geometry.stage));
            require(trial["collision_objects"].Size() == (geometry.slab ? 24u : 26u), "Public Roller comparison has the wrong geometry inventory");
            for (rapidjson::SizeType index = 0; index < 22; ++index)
            { require(trial["collision_objects"][index] == source["collision_objects"][index], "Public Roller comparison changed an original object"); }
        }
        std::ofstream csv(evidence / "roller-route-comparison.csv"), trace(evidence / "roller-route-trace.csv");
        require(csv.good() && trace.good(), "Cannot create public Roller-route evidence");
        csv << std::setprecision(12);
        trace << std::setprecision(12);
        csv << "geometry,side,direction,count_in_updates,neutral_readiness_updates,drive_updates,release_braking_updates,quiet_updates,total_updates,"
            "initial_centre_x,initial_feet_y,final_centre_x,final_feet_y,final_supported,initial_health,final_health,initial_ammo,arrival_ammo,final_ammo,"
            "held_trigger_updates,actual_paid_updates,actual_total_paid_cost,airborne_walk_updates,sloped_support_updates,first_receiver_top_paint_walk_update,"
            "receiver_top_owner_area,receiver_bottom_owner_area,receiver_left_owner_area,receiver_right_owner_area,"
            "original_upper_top_area,original_upper_bottom_area,original_upper_left_area,original_upper_right_area,"
            "original_crossover_top_area,original_crossover_bottom_area,original_crossover_left_area,original_crossover_right_area,"
            "total_owned_area,total_scored_area,raw_paint_owner,scope\n";
        trace << "trial,phase,total_updates,centre_x,feet_y,vx,vy,move_state,supported,health,ammo,held_trigger_updates,paid_updates,total_paid_cost,"
            "receiver_top,receiver_bottom,receiver_left,receiver_right,original_upper_top,original_crossover_top,total_owned\n";
        World world(content);
        int scenes = 0, updates = 0, paid = 0, top_scenes = 0, airborne = 0;
        for (const bool side_b : { false, true })
        {
            for (const bool uphill : { true, false })
            {
                std::optional<Result> matched;
                for (const auto& geometry : geometries)
                {
                    const auto result = measure(world, content, geometry, evidence, trace, side_b, uphill);
                    if (matched)
                    {
                        require(result.initial == matched->initial && result.initial_ammo == matched->initial_ammo &&
                            result.initial_health == matched->initial_health, "Matched public Roller geometry pair changed its initial pose/health/reserve");
                    }
                    else { matched = result; }
                    csv << geometry.label << ',' << (side_b ? 'b' : 'a') << ',' << (uphill ? "uphill" : "downhill") <<
                        ",181,60," << result.drive_updates << ',' << result.braking_updates << ",180," << result.updates << ',' <<
                        result.initial.center().x << ',' << result.initial.bottom() << ',' << result.final.center().x << ',' << result.final.bottom() <<
                        ",1," << result.initial_health << ',' << result.final_health << ',' << result.initial_ammo << ',' << result.arrival_ammo << ',' <<
                        result.final_ammo << ',' << result.held << ',' << result.paid << ',' << result.paid_cost << ',' << result.airborne << ',' << result.slope << ',' <<
                        result.first_top << ',' << result.final_paint.receiver[0] << ',' << result.final_paint.receiver[1] << ',' <<
                        result.final_paint.receiver[2] << ',' << result.final_paint.receiver[3] << ',' <<
                        result.final_paint.upper[0] << ',' << result.final_paint.upper[1] << ',' << result.final_paint.upper[2] << ',' << result.final_paint.upper[3] << ',' <<
                        result.final_paint.crossover[0] << ',' << result.final_paint.crossover[1] << ',' << result.final_paint.crossover[2] << ',' << result.final_paint.crossover[3] << ',' <<
                        result.final_paint.total_owned << ',' << result.final_paint.total_scored <<
                        ",4,actual-public-input-driven-paid-Roller-route\n";
                    ++scenes; updates += result.updates; paid += result.paid; airborne += result.airborne;
                    top_scenes += result.final_paint.receiver[0] > 0.0 ? 1 : 0;
                    require(updates <= maximum_matrix_updates, "Public Roller route matrix exceeded its finite update budget");
                    csv.flush(); trace.flush(); require(csv.good() && trace.good(), "Cannot flush public Roller measurements");
                }
            }
        }
        originals.check();
        require(scenes == 8, "Public Roller route inventory differs from eight fresh geometry/side/direction scenes");
        std::ofstream summary(evidence / "roller-route-summary.json");
        require(summary.good(), "Cannot create public Roller-route summary");
        summary << "{\"outcome\":\"passed\",\"scope\":\"public-input-driven-paid-Roller-route\",\"seed\":" << seed <<
            ",\"fresh_public_levels\":" << scenes << ",\"total_updates\":" << updates << ",\"maximum_scene_updates\":" << maximum_scene_updates <<
            ",\"maximum_matrix_updates\":" << maximum_matrix_updates << ",\"actual_paid_updates\":" << paid <<
            ",\"receiver_top_positive_scenes\":" << top_scenes << ",\"airborne_walk_updates\":" << airborne <<
            ",\"post_construction_placements\":0,\"jump_inputs\":0,\"deaths\":0,\"original_objects_preserved\":22,"
            "\"unscored_composite_ramp_preserved\":true,\"registered\":false,\"opposed_pressure_measured\":false,\"human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot finish public Roller-route summary");
    }
}
