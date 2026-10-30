#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_routes/fixtures.h"
#include "game/objects/level_builder.h"
#include "editor/level_document.h"
#include "game/debug/dev_tools.h"
#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <set>
#if COLOURWARS_DEV_TOOLS
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include <rapidjson/prettywriter.h>
#endif

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
            fs::path content;
            std::set<fs::path> levels;

            static std::set<fs::path> inventory(const fs::path& root)
            {
                const auto manifest = parse(read(root / "manifest.json"));
                std::set<fs::path> expected, actual;
                for (const auto& asset : manifest["assets"].GetArray())
                {
                    if (std::string(asset["kind"].GetString()) != "level") { continue; }
                    for (const auto& name : asset["names"].GetArray())
                    {
                        const auto path = (fs::path(asset["directory"].GetString()) /
                            (std::string(name.GetString()) + ".json")).lexically_normal();
                        require(expected.insert(path).second, "Roller-route guard found a duplicate manifest level");
                    }
                }
                for (const auto& file : fs::directory_iterator(root / "levels"))
                {
                    if (file.is_regular_file() && file.path().extension() == ".json")
                    { actual.insert(file.path().lexically_relative(root).lexically_normal()); }
                }
                require(actual == expected, "Roller-route guard needs exact manifest/filesystem level inventory");
                for (const char* name : { "king_of_the_hill", "turbulence", "close_quarters", "movement_course",
                    "crossfeed", "press_tower", "turbulence_blockout", "turbulence_mover_trial" })
                {
                    require(expected.contains(fs::path("levels") / (std::string(name) + ".json")),
                        "Roller-route guard lost an original arena");
                }
                return expected;
            }

            SourceGuard(const fs::path& root, const fs::path& slab, const fs::path& shoulder) :
                content(root.lexically_normal()), levels(inventory(content))
            {
                for (const char* file : { "manifest.json", "tuning.json", "presentation.json" })
                { sources.emplace_back(content / file, read(content / file)); }
                for (const auto& file : levels) { sources.emplace_back(content / file, read(content / file)); }
                require(sources.size() == levels.size() + 3, "Roller-route guard lost startup or registered-level byte snapshots");
                sources.emplace_back(slab, read(slab)); sources.emplace_back(shoulder, read(shoulder));
            }
            void check() const
            {
                for (const auto& [path, before] : sources)
                { require(read(path) == before, "Roller route changed source content: " + path.string()); }
                require(inventory(content) == levels, "Roller route changed the source level inventory");
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

            Trial(World& with, const fs::path& stage, const Roles& groups, bool side_b,
                const MatchSetup* decoded_settings = nullptr, const unsigned int* decoded_seed = nullptr) :
                world(with), map(groups)
            {
                auto info = std::make_unique<LevelLoadedInfo>(path_to_utf8(stage).c_str()); info->validate();
                const LevelObjectBuilder objects(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
                info->validate_content(objects, world.render, world.audio); bounds = info->out_of_bounds_rectangle();
                infos.add(decoded_settings ? decoded_settings->stage() : "turbulence_roller_route", std::move(info));
                MatchSetup settings;
                settings.set_stage("turbulence_roller_route"); settings.set_player_count(1);
                settings.set_screen_layout(ScreenLayout::one_player); settings.begin_input_claims(input_slots::keyboard_and_mouse);
                settings.set_player_team(0, side_b ? PlayerTeam::b : PlayerTeam::a); settings.set_player_weapon(0, WeaponType::roller);
                if (decoded_settings != nullptr) { settings = *decoded_settings; }
                viewports.set_layout(settings.screen_layout());
                LevelBuilder builder(&viewports, &world.render, &world.audio, &world.preferences, &infos,
                    &world.tuning, &world.presentation, &resolution, nullptr, &partitioner);
                std::srand(decoded_seed ? *decoded_seed : seed);
                level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
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
            void tick(float movement = 0.0f, bool fire = false, bool observe_route = false,
                SeatInputFilter* extra = nullptr, float seconds = moving_structure_acceptance::dt,
                PlayerInputData* effective_output = nullptr)
            {
                require(updates < maximum_scene_updates, "Public Roller route exceeded its finite update budget" + pose());
                const bool ready_before = player().primary_weapon().ready_to_shoot();
                const float ammo_before = player().weapon_ammo();
                class Input final : public SeatInputFilter
                {
                public:
                    Input(float movement, bool fire, SeatInputFilter* extra) : movement_(movement), fire_(fire), extra_(extra) {}
                    void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
                    {
                        require(seats.size() == 1, "Public Roller route unexpectedly gained a seat");
                        seats[0] = {}; seats[0].connected = true;
                        seats[0].x_movement = movement_; seats[0].primary_shoot = fire_;
                        if (extra_ != nullptr) { extra_->filter_seats(seats, level); }
                        effective = seats[0]; called = true;
                    }
                    PlayerInputData effective;
                    bool called = false;
                private: float movement_; bool fire_; SeatInputFilter* extra_;
                } input(movement, fire, extra);
                level->update({}, seconds, &input); ++updates;
                if (effective_output != nullptr && input.called) { *effective_output = input.effective; }
                const bool actual_fire = input.called && input.effective.primary_shoot;
                if (actual_fire) { ++held; }
                if (actual_fire && ready_before)
                {
                    const float cost = world.tuning.weapon(WeaponType::roller).details.ammo_usage;
                    const float actual_cost = ammo_before - player().weapon_ammo();
                    require(cost > 0.0f && std::abs(actual_cost - cost) <= 0.000003f,
                        "Actual held Roller pulse did not pay its source-tuned cost" + pose());
                    ++paid; paid_cost += actual_cost;
                }
                else if (actual_fire)
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

#if COLOURWARS_DEV_TOOLS
        std::uint64_t paint_witness(const std::vector<std::uint64_t>& paint);

        struct Observation
        {
            MatchFingerprint print;
            std::vector<std::uint64_t> paint;
            std::uint64_t paint_hash = 0;
        };

        Observation observe(const Trial& trial, int input_update)
        {
            Observation result{ MatchFingerprint::of(*trial.level, input_update),
                moving_structure_acceptance::sample_bits(trial.level->paint_surfaces()), 0 };
            result.paint_hash = paint_witness(result.paint);
            return result;
        }

        void same_observation(const Observation& expected, const Trial& actual, int input_update,
            const std::string& label)
        {
            const auto difference = MatchFingerprint::difference(expected.print,
                MatchFingerprint::of(*actual.level, input_update));
            require(difference.empty(), label + ": " + difference);
            require(expected.paint == moving_structure_acceptance::sample_bits(actual.level->paint_surfaces()),
                label + ": complete world paint quads/local areas/team/raw-owner/source ordering differ");
        }

        std::uint64_t paint_witness(const std::vector<std::uint64_t>& paint)
        {
            std::uint64_t hash = 0xcbf29ce484222325ull;
            for (const std::uint64_t value : paint)
            {
                for (unsigned int byte = 0; byte < 8; ++byte)
                { hash ^= (value >> (byte * 8)) & 255u; hash *= 0x100000001b3ull; }
            }
            return hash;
        }

        void replay_trace(std::ofstream& trace, const char* phase, int level_update, int input_update,
            const Observation& observation, const Trial& trial)
        {
            const auto paint = trial.paint();
            trace << phase << ',' << level_update << ',' << input_update << ',' << hex64(observation.print.hash) << ',' <<
                hex64(observation.paint_hash) << ',' << trial.player().center().x << ',' <<
                trial.player().rectangle().bottom() << ',' << trial.player().weapon_ammo() << ',' <<
                paint.receiver[0] << ',' << paint.total_owned << ',' << trial.player().carrier_id() << '\n';
            require(trace.good(), "Cannot write exact public slab replay observations");
        }

        void write_variant(const fs::path& path, const rapidjson::Document& document)
        {
            require(!fs::exists(path), "Refusing earlier slab replay control receipt");
            rapidjson::StringBuffer buffer;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot serialize slab replay control JSON");
            std::ofstream file(path, std::ios::binary); file << buffer.GetString(); file.close();
            require(file.good(), "Cannot write slab replay control receipt");
        }

        void check_codec_refusal(const fs::path& tape, const fs::path& output)
        {
            auto invalid = parse(read(tape));
            require(invalid["inputs"].Size() == 1 && !invalid["inputs"][0].Empty(),
                "Slab replay codec control needs an actual recorded input run");
            invalid["inputs"][0][0].SetString("1*not-an-encoded-input", invalid.GetAllocator());
            write_variant(output, invalid);
            bool refused = false;
            try { (void)ReplayFile::read(output); }
            catch (const std::runtime_error& error)
            { refused = std::string(error.what()).size() > 0; }
            require(refused, "Production replay codec accepted malformed recorded input");
        }

        struct ReplayCounts
        {
            int record_updates = 0, input_updates = 0, priming_updates = 0, checkpoints = 0;
            int paid = 0, held = 0, braking = 0, corruption_updates = 0;
            double receiver_top = 0.0;
        };

        ReplayCounts replay_route(World& world, const fs::path& content, const Geometry& geometry,
            const fs::path& evidence, bool side_b, bool uphill, bool corruption_probe)
        {
            const std::string name = std::string("replay-") + (side_b ? "b-" : "a-") + (uphill ? "uphill" : "downhill");
            const auto stage = station(content, geometry, evidence, name, side_b, uphill, world.tuning.player().size);
            const auto stage_bytes = read(stage);
            const auto groups = roles(parse(stage_bytes), geometry, side_b);
            ReplayFile header;
            header.build = "public-slab-Roller-after-180-unfiltered-countdown-updates";
            header.dt_bits = bits_from_float(moving_structure_acceptance::dt);
            header.seed = seed;
            header.tuning_hash = fnv1a(read(content / "tuning.json"));
            header.presentation_hash = fnv1a(read(content / "presentation.json"));
            MatchSetup roster;
            roster.set_stage(stage.stem().string()); roster.set_player_count(1);
            roster.set_screen_layout(ScreenLayout::one_player);
            roster.begin_input_claims(input_slots::keyboard_and_mouse);
            roster.set_player_team(0, side_b ? PlayerTeam::b : PlayerTeam::a);
            roster.set_player_weapon(0, WeaponType::roller);
            header.tape.start(0, roster);
            auto original = std::make_unique<Trial>(world, stage, groups, side_b, &header.tape.settings(), &*header.seed);
            const Observation initial = observe(*original, 0);
            ReplayRecorder recorder(name, header, nullptr, maximum_scene_updates);
            std::vector<Observation> priming, recorded;
            std::ofstream trace(evidence / (name + "-exact-trace.csv"));
            require(trace.good(), "Cannot create slab replay trace");
            trace << std::setprecision(12) << "phase,level_update,recorded_input_update,full_fingerprint_hash,paint_bits_witness,"
                "centre_x,feet_y,ammo,receiver_top_area,total_owned_area,carrier\n";
            replay_trace(trace, "initial-cold", 0, 0, initial, *original);
            const auto tick = [&](float movement = 0.0f, bool fire = false, bool route = false)
            {
                const auto before = static_cast<std::size_t>(recorder.file().tape.steps());
                original->tick(movement, fire, route, &recorder);
                const auto after = static_cast<std::size_t>(recorder.file().tape.steps());
                if (after == before)
                {
                    require(recorded.empty() && !original->level->match_started(),
                        "Production input filter skipped an active recorded slab update");
                    priming.push_back(observe(*original, original->updates));
                    replay_trace(trace, "unfiltered-cold-priming", original->updates, 0, priming.back(), *original);
                }
                else
                {
                    require(after == before + 1 && recorder.file().tape.seats(static_cast<int>(before)).size() == 1,
                        "Production slab recorder did not receive exactly one seat input per active update");
                    recorded.push_back(observe(*original, static_cast<int>(after)));
                    replay_trace(trace, "record", original->updates, static_cast<int>(after), recorded.back(), *original);
                    if (after % ReplayFile::checkpoint_interval == 0) { recorder.add_checkpoint(recorded.back().print); }
                }
            };
            int count_in = 0;
            while (!original->level->match_started() && count_in < 240) { tick(); ++count_in; }
            require(count_in == 181 && priming.size() == 180 && recorded.size() == 1,
                "Slab replay source countdown/filter boundary changed; do not invent ignored tape inputs");
            for (int update = 0; update < 60; ++update) { tick(); }
            flat_stop(*original, uphill ? groups.upper_body : groups.crossover_body,
                "Recorded slab Roller start lost original flat support");
            require(original->player().primary_weapon().ready_to_shoot() && original->player().weapon_ammo() == 1.0f &&
                original->paint().total_owned == 0.0, "Recorded slab route was not neutral/ready/fully funded");
            const float target = mirror(side_b, uphill ? 1500.0f : 1160.0f);
            const float direction = target > original->player().center().x ? 1.0f : -1.0f;
            bool braking = false;
            int braking_updates = 0;
            for (int update = 0; update < 480; ++update)
            {
                const float error = std::abs(target - original->player().center().x);
                const float speed = std::abs(original->player().velocity().x);
                const float stop = speed * speed / (2.0f * world.tuning.player().x_deacceleration) +
                    speed * moving_structure_acceptance::dt + 4.0f;
                if (!braking && speed > 0.0f && error <= stop) { braking = true; }
                tick(braking ? 0.0f : direction, !braking, true);
                braking_updates += braking ? 1 : 0;
                if (braking && original->player().velocity().x == 0.0f && supported(original->player())) { break; }
            }
            require(braking && original->paid > 0 && original->airborne == 0 && original->slope > 0,
                "Recorded public slab route missed paid grounded Roller travel");
            flat_stop(*original, uphill ? groups.crossover_body : groups.upper_body,
                "Recorded slab route failed to stop on its fixed receiving floor");
            for (int update = 0; update < 180; ++update) { tick(); }
            flat_stop(*original, uphill ? groups.crossover_body : groups.upper_body,
                "Recorded slab route lost final quiet support");
            require(original->paint().receiver[0] > 0.0, "Recorded earned slab walk did not retain raw-owner top paint");
            recorder.file().updates = static_cast<int>(recorded.size());
            recorder.file().final_print = recorded.back().print;
            const auto tape = evidence / (name + "-tape.json");
            require(!fs::exists(tape), "Refusing earlier production slab replay tape"); recorder.file().write(tape);
            ReplayCounts counts{ original->updates, static_cast<int>(recorded.size()), static_cast<int>(priming.size()),
                static_cast<int>(recorder.file().checkpoints.size()), original->paid, original->held,
                braking_updates, 0, original->paint().receiver[0] };
            const unsigned int terminal_rng = static_cast<unsigned int>(std::rand());
            original.reset();

            // The current production codec contains input-filter calls, not the
            // 180 Level updates that return before that filter. This separate
            // receipt records the required cold priming explicitly.
            const auto priming_path = evidence / (name + "-priming.json");
            {
                std::ofstream receipt(priming_path);
                receipt << "{\"scope\":\"unfiltered-source-countdown-observations\",\"unfiltered_updates\":" << counts.priming_updates <<
                    ",\"level_updates\":" << counts.record_updates << ",\"production_tape_updates\":" << counts.input_updates <<
                    ",\"source_stage_hash\":\"" << hex64(fnv1a(stage_bytes)) << "\",\"initial_hash\":\"" << hex64(initial.print.hash) <<
                    "\",\"codec_contains_priming\":false,\"director_cold_playback_qualified\":false}\n";
                receipt.close(); require(receipt.good(), "Cannot finish explicit slab replay priming receipt");
            }
            const auto decoded = ReplayFile::read(tape);
            const auto priming_receipt = parse(read(priming_path));
            const int priming_count = priming_receipt["unfiltered_updates"].GetInt();
            require(decoded.seed && decoded.build == header.build && decoded.dt_bits == header.dt_bits &&
                decoded.tuning_hash == fnv1a(read(content / "tuning.json")) &&
                decoded.presentation_hash == fnv1a(read(content / "presentation.json")) && !decoded.tape.damage_immune() && !decoded.tape.infinite_ammo() &&
                decoded.tape.gaps().empty() && decoded.updates == counts.input_updates && decoded.inputs_by_seat().size() == 1 &&
                decoded.inputs_by_seat()[0].size() == recorded.size() && decoded.checkpoints.size() == static_cast<std::size_t>(counts.checkpoints) &&
                priming_count == counts.priming_updates && priming_count + decoded.updates == counts.record_updates,
                "Production slab tape or explicit priming lost rebuilding metadata/input counts");
            require(decoded.tape.settings().stage() == stage.stem().string() && decoded.tape.settings().player_count() == 1 &&
                decoded.tape.settings().screen_layout() == header.tape.settings().screen_layout() &&
                decoded.tape.settings().player_settings()[0].team == header.tape.settings().player_settings()[0].team &&
                decoded.tape.settings().player_settings()[0].num == header.tape.settings().player_settings()[0].num &&
                decoded.tape.settings().player_settings()[0].weapon == WeaponType::roller,
                "Production slab tape lost the actual complete roster/setup");
            for (std::size_t update = 0; update < recorded.size(); ++update)
            { require(replay_codec::same(recorder.file().tape.seats(static_cast<int>(update))[0], decoded.tape.seats(static_cast<int>(update))[0]), "Production slab input codec changed bits"); }
            check_codec_refusal(tape, evidence / (name + "-invalid-input.json"));
            const fs::path rebuild_stage = evidence / (decoded.tape.settings().stage() + ".json");
            require(read(rebuild_stage) == stage_bytes && fnv1a(read(rebuild_stage)) ==
                std::stoull(priming_receipt["source_stage_hash"].GetString(), nullptr, 16),
                "Decoded public slab stage selection/source hash differs");
            const float seconds = float_from_bits(decoded.dt_bits);
            const auto rebuilt_groups = roles(parse(read(rebuild_stage)), geometry, side_b);
            auto playback = std::make_unique<Trial>(world, rebuild_stage, rebuilt_groups, side_b, &decoded.tape.settings(), &*decoded.seed);
            same_observation(initial, *playback, 0, "Decoded cold slab rebuild differs before countdown");
            for (int update = 0; update < priming_count; ++update)
            {
                playback->tick(0.0f, false, false, nullptr, seconds);
                same_observation(priming[static_cast<std::size_t>(update)], *playback, update + 1,
                    "Cold slab countdown differs at unfiltered update " + std::to_string(update + 1));
                replay_trace(trace, "rebuild-unfiltered-priming", playback->updates, 0,
                    priming[static_cast<std::size_t>(update)], *playback);
            }
            ReplayPlayer player(name, decoded);
            for (int update = 0; update < decoded.updates; ++update)
            {
                // The ordinary baseline is neutral. Only the production
                // ReplayPlayer can inject the recorded walk and paid trigger.
                PlayerInputData effective;
                playback->tick(0.0f, false, false, &player, seconds, &effective);
                require(replay_codec::same(effective, decoded.tape.seats(update)[0]),
                    "Production ReplayPlayer did not inject the exact decoded seat input over the neutral baseline");
                const auto& expected = recorded[static_cast<std::size_t>(update)];
                same_observation(expected, *playback, update + 1,
                    "Production slab playback differs at tape update " + std::to_string(update + 1));
                replay_trace(trace, "production-playback", playback->updates, update + 1, expected, *playback);
                if (player.next_checkpoint < decoded.checkpoints.size() &&
                    decoded.checkpoints[player.next_checkpoint].update == update + 1)
                {
                    const auto& checkpoint = decoded.checkpoints[player.next_checkpoint++];
                    if (checkpoint.hash != expected.print.hash) { player.diverged_at = update + 1; }
                }
            }
            player.finished = true;
            player.matched = player.diverged_at == -1 && player.next_checkpoint == decoded.checkpoints.size() &&
                MatchFingerprint::difference(decoded.final_print, MatchFingerprint::of(*playback->level, decoded.updates)).empty();
            require(player.finished && player.matched && playback->paid == counts.paid && playback->held == counts.held &&
                static_cast<unsigned int>(std::rand()) == terminal_rng,
                "Production slab replay final fingerprint/checkpoints/paid counts/terminal RNG differ");
            require(playback->paint().receiver[0] == counts.receiver_top && read(rebuild_stage) == stage_bytes,
                "Production slab replay final earned paint or stage bytes differ");
            playback.reset();

            if (corruption_probe)
            {
                auto corrupt_json = parse(read(tape));
                require(!decoded.checkpoints.empty(), "Checkpoint corruption control needs a recorded checkpoint");
                const std::string changed = hex64(decoded.checkpoints[0].hash ^ 1u);
                corrupt_json["checkpoints"][0]["hash"].SetString(changed.c_str(), corrupt_json.GetAllocator());
                const auto corrupt_path = evidence / (name + "-wrong-checkpoint.json"); write_variant(corrupt_path, corrupt_json);
                const auto corrupt = ReplayFile::read(corrupt_path);
                ReplayPlayer mismatch(name + "-wrong-checkpoint", corrupt);
                const auto control_stage = evidence / (corrupt.tape.settings().stage() + ".json");
                const auto control_groups = roles(parse(read(control_stage)), geometry, side_b);
                auto control = std::make_unique<Trial>(world, control_stage, control_groups, side_b, &corrupt.tape.settings(), &*corrupt.seed);
                for (int update = 0; update < priming_count; ++update)
                { control->tick(0.0f, false, false, nullptr, float_from_bits(corrupt.dt_bits)); }
                const int checkpoint_update = corrupt.checkpoints[0].update;
                for (int update = 0; update < checkpoint_update; ++update)
                {
                    PlayerInputData effective;
                    control->tick(0.0f, false, false, &mismatch, float_from_bits(corrupt.dt_bits), &effective);
                    require(replay_codec::same(effective, corrupt.tape.seats(update)[0]),
                        "Altered-checkpoint production ReplayPlayer lost its exact decoded input");
                    const auto actual = MatchFingerprint::of(*control->level, update + 1);
                    require(MatchFingerprint::difference(recorded[static_cast<std::size_t>(update)].print, actual).empty(),
                        "Checkpoint corruption unexpectedly changed production replay state");
                    if (update + 1 == checkpoint_update && actual.hash != corrupt.checkpoints[0].hash)
                    { mismatch.diverged_at = update + 1; }
                }
                require(mismatch.diverged_at == checkpoint_update && mismatch.diverged_at == 60 && !mismatch.matched,
                    "Fresh production slab playback failed to identify its altered checkpoint");
                counts.corruption_updates = control->updates;
            }
            trace.close(); require(trace.good(), "Cannot finish slab replay exact trace");
            std::cout << "PASS " << name << ": " << counts.priming_updates << " exact unfiltered cold priming updates + " <<
                counts.input_updates << " actual production tape updates, " << counts.checkpoints <<
                " checkpoints, every public fingerprint/paint quad/credit, decoded fresh disk rebuild and terminal RNG\n";
            return counts;
        }
#endif
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

    void run_replay(const fs::path& content, const fs::path& slab, const fs::path& shoulder, const fs::path& evidence)
    {
#if COLOURWARS_DEV_TOOLS
        SourceGuard originals(content, slab, shoulder);
        const auto baseline = parse(read(content / "levels/turbulence_mover_trial.json"));
        const auto source = parse(read(slab));
        require(baseline["collision_objects"].Size() == 22 && source["collision_objects"].Size() == 24,
            "Slab replay requires the qualified 22-object source plus its fixed pair");
        int original_ramps = 0;
        for (rapidjson::SizeType index = 0; index < 22; ++index)
        {
            require(source["collision_objects"][index] == baseline["collision_objects"][index],
                "Slab replay changed an original qualified object");
            original_ramps += std::string(baseline["collision_objects"][index]["type"].GetString()) == "StructureRamp" ? 1 : 0;
        }
        const Geometry geometry{ "slab", slab, true, original_ramps };
        World world(content);
        int pairs = 0, record_updates = 0, tape_updates = 0, prime_updates = 0, checkpoints = 0, corruption_updates = 0;
        for (const bool side_b : { false, true })
        {
            for (const bool uphill : { true, false })
            {
                const auto result = replay_route(world, content, geometry, evidence, side_b, uphill, pairs == 0);
                ++pairs; record_updates += result.record_updates; tape_updates += result.input_updates;
                prime_updates += result.priming_updates; checkpoints += result.checkpoints;
                corruption_updates += result.corruption_updates;
                require(record_updates * 2 + corruption_updates <= maximum_scene_updates * 9,
                    "Slab replay matrix exceeded its finite recorded/playback/control budget");
            }
        }
        originals.check();
        require(pairs == 4, "Slab replay must rebuild four actual mirrored/directional Roller routes");
        std::ofstream summary(evidence / "slab-public-replay-summary.json");
        summary << "{\"outcome\":\"passed\",\"scope\":\"production-ReplayRecorder-codec-ReplayPlayer-public-slab\","
            "\"record_play_pairs\":" << pairs << ",\"record_level_updates\":" << record_updates << ",\"play_level_updates\":" << record_updates <<
            ",\"production_tape_updates\":" << tape_updates << ",\"recorded_unfiltered_priming_updates\":" << prime_updates <<
            ",\"checkpoints\":" << checkpoints << ",\"malformed_codec_controls\":4,\"fresh_checkpoint_mismatch_controls\":1,"
            "\"checkpoint_control_updates\":" << corruption_updates << ",\"total_actual_level_updates\":" << record_updates * 2 + corruption_updates <<
            ",\"maximum_actual_level_updates\":9000,\"full_paint_bits_compared_every_update\":true,\"terminal_rng_witness_per_pair\":true,"
            "\"post_construction_placements\":0,\"jump_inputs\":0,\"director_cold_playback_qualified\":false,"
            "\"production_tape_contains_unfiltered_countdown\":false,\"cross_build_or_human_acceptance\":false}\n";
        summary.close(); require(summary.good(), "Cannot finish production slab replay summary");
#else
        (void)content; (void)slab; (void)shoulder; (void)evidence;
        throw std::runtime_error("Slab --replay requires Debug developer tools; Release replay is explicitly unqualified");
#endif
    }
}
