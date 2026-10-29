#include "game/pch.h"
#include "scripts/acceptance/turbulence_slab_tools/fixtures.h"
#include "editor/level_document.h"
#include "game/objects/level_builder.h"
#include <array>
#include <iomanip>
#include <optional>

namespace turbulence_slab_tools_acceptance
{
    namespace
    {
        using namespace mattmath;
        using World = moving_structure_acceptance::World;
        constexpr unsigned int seed = 260027u;
        constexpr int observation_updates = 180, maximum_readiness = 135;
        constexpr std::array<const char*, 3> group_names{ "own_slab", "other_slab", "lower_bed" };
        constexpr std::array<const char*, 4> face_names{ "top", "bottom", "left", "right" };
        using Areas = std::array<std::array<double, 4>, 3>;

        std::string read_text(const fs::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            require(input.good(), "Cannot read lower-approach source: " + path.string());
            const std::string bytes{ std::istreambuf_iterator<char>(input), {} };
            require(!input.bad(), "Cannot finish reading lower-approach source: " + path.string());
            return bytes;
        }

        rapidjson::Document parse(const std::string& bytes)
        {
            rapidjson::Document result; result.Parse(bytes.c_str());
            require(!result.HasParseError() && result.IsObject(), "Invalid lower-approach JSON source");
            return result;
        }

        std::vector<fs::path> level_inventory(const fs::path& content)
        {
            std::vector<fs::path> result;
            for (const auto& entry : fs::directory_iterator(content / "levels"))
            { if (entry.is_regular_file() && entry.path().extension() == ".json") { result.push_back(entry.path()); } }
            std::sort(result.begin(), result.end()); return result;
        }

        struct SourceGuard
        {
            fs::path content;
            std::vector<fs::path> inventory;
            std::vector<std::pair<fs::path, std::string>> files;
            SourceGuard(const fs::path& root, const fs::path& candidate) : content(root), inventory(level_inventory(root))
            {
                for (const auto& path : inventory) { files.emplace_back(path, read_text(path)); }
                for (const char* name : { "manifest.json", "tuning.json", "presentation.json" })
                { files.emplace_back(root / name, read_text(root / name)); }
                files.emplace_back(candidate, read_text(candidate));
            }
            void check() const
            {
                require(level_inventory(content) == inventory, "Lower-approach case changed the staged level inventory");
                for (const auto& [path, bytes] : files)
                { require(read_text(path) == bytes, "Lower-approach case changed a protected source: " + path.string()); }
            }
        };

        bool close_value(float actual, float expected)
        { return std::isfinite(actual) && std::abs(actual - expected) <= 0.002f; }

        bool same_pose(const RectangleF& actual, const RectangleF& expected)
        {
            return close_value(actual.x, expected.x) && close_value(actual.y, expected.y) &&
                actual.width == expected.width && actual.height == expected.height;
        }

        bool supported(const Player& player)
        {
            const auto state = player.move_state();
            return state == PlayerMoveState::on_ground || state == PlayerMoveState::on_drop_down_ground ||
                state == PlayerMoveState::on_ramp_left || state == PlayerMoveState::on_ramp_right;
        }

        RectangleF station(const Vector2F& size, bool side_b, bool upper)
        {
            const float centre = side_b ? 2680.0f : 1320.0f;
            const float support = !upper ? 1200.0f : (side_b ?
                800.0f + (centre - size.x / 2.0f - 2600.0f) * 0.5f :
                880.0f - (centre + size.x / 2.0f - 1240.0f) * 0.5f);
            return { centre - size.x / 2.0f, support - size.y, size.x, size.y };
        }

        void qualify(const fs::path& content, const fs::path& candidate)
        {
            const auto source = parse(read_text(candidate));
            const auto registered = parse(read_text(content / "levels/turbulence_slab_trial.json"));
            const auto mover = parse(read_text(content / "levels/turbulence_mover_trial.json"));
            require(source.HasMember("collision_objects") && source["collision_objects"].IsArray() &&
                source["collision_objects"].Size() == 24 && registered["collision_objects"].Size() == 24 &&
                source["collision_objects"] == registered["collision_objects"] && mover["collision_objects"].Size() == 22,
                "Lower-approach input must preserve the qualified registered 24-object slab geometry");
            for (rapidjson::SizeType index = 0; index < 22; ++index)
            { require(source["collision_objects"][index] == mover["collision_objects"][index], "Slab input changed an original mover-trial object"); }
            require(source.HasMember("include_in_random") && !source["include_in_random"].GetBool(),
                "Lower-approach input lost its non-random trial role");
            for (const auto& member : registered.GetObject())
            {
                if (std::string(member.name.GetString()) == "display_name") { continue; }
                require(source.HasMember(member.name) && source[member.name] == member.value,
                    "Lower-approach input differs from the qualified stage outside its display name");
            }
        }

        fs::path author(const fs::path& content, const fs::path& source, const fs::path& output,
            const Vector2F& size, bool side_b, bool paint_pass)
        {
            using namespace colourwars_editor;
            LevelDocument document(content / "tuning.json"); document.load(source);
            const auto original = parse(document.serialize());
            const auto spawn_kind = side_b ? EntryKind::spawn_b : EntryKind::spawn_a;
            const std::string own_name = side_b ? "fixed_slab_b" : "fixed_slab_a";
            std::optional<Entry> spawn, own;
            for (const auto& entry : document.entries())
            {
                if (entry.kind == spawn_kind && entry.index == 0) { spawn = entry; }
                if (entry.kind == EntryKind::collision &&
                    original["collision_objects"][static_cast<rapidjson::SizeType>(entry.index)]["name"].GetString() == own_name)
                { own = entry; }
            }
            require(spawn && own, "Editor lower-approach variant lost its legal source spawn or own slab");
            const auto body = station(size, side_b, false);
            document.set_rectangle(*spawn, { body.x, body.y, body.width, body.height });
            if (paint_pass)
            {
                document.erase(*own);
                const auto ramp = document.add_object(side_b ? "StructureRampLeft" : "StructureRampRight",
                    { side_b ? 2600.0 : 1240.0, 800.0, 160.0, 80.0 });
                document.set_property(ramp, "name", side_b ? "paint_pass_ramp_b" : "paint_pass_ramp_a");
            }
            require(document.validate().empty(), "Editor rejected the lower-approach source variant");
            const auto variant = parse(document.serialize());
            for (const auto& member : original.GetObject())
            {
                const std::string key = member.name.GetString();
                if (key == "spawn_locations" || key == "collision_objects") { continue; }
                require(variant.HasMember(member.name) && variant[member.name] == member.value,
                    "Editor lower-approach variant changed unrelated stage metadata");
            }
            const char* source_team = side_b ? "team_b" : "team_a";
            for (const char* team : { "team_a", "team_b" })
            {
                const auto& before = original["spawn_locations"][team];
                const auto& after = variant["spawn_locations"][team];
                require(before.Size() == after.Size(), "Editor changed the spawn roster");
                for (rapidjson::SizeType index = 0; index < before.Size(); ++index)
                {
                    if (std::string(team) == source_team && index == 0)
                    { require(after[index]["x"].GetFloat() == body.x && after[index]["y"].GetFloat() == body.y, "Editor source spawn missed its floor station"); }
                    else { require(before[index] == after[index], "Editor changed an unused or opposing legal spawn"); }
                }
            }
            require(variant["collision_objects"].Size() == 24, "Lower-approach variant changed its bounded object count");
            rapidjson::SizeType retained = 0;
            for (const auto& object : original["collision_objects"].GetArray())
            {
                if (paint_pass && std::string(object["name"].GetString()) == own_name) { continue; }
                require(variant["collision_objects"][retained++] == object, "Editor changed an original object or the other slab");
            }
            if (paint_pass)
            {
                const auto& ramp = variant["collision_objects"][retained];
                const auto& triangle = ramp["triangle"];
                const float x = side_b ? 2600.0f : 1240.0f;
                require(std::string(ramp["type"].GetString()) == "StructureRamp" &&
                    std::string(ramp["collision_type"].GetString()) == (side_b ? "STRUCTURE_RAMP_LEFT" : "STRUCTURE_RAMP_RIGHT") &&
                    !ramp.HasMember("paintable_faces") && !ramp.HasMember("motion") &&
                    triangle["x1"].GetFloat() == (side_b ? x + 160.0f : x) && triangle["y1"].GetFloat() == 880.0f &&
                    triangle["x2"].GetFloat() == (side_b ? x : x + 160.0f) && triangle["y2"].GetFloat() == 880.0f &&
                    triangle["x3"].GetFloat() == (side_b ? x : x + 160.0f) && triangle["y3"].GetFloat() == 800.0f,
                    "Editor paint-pass control lost the existing unscored ramp and identical top support segment");
            }
            require(!fs::exists(output), "Refusing an existing lower-approach variant"); document.save(output);
            LevelDocument reopened(content / "tuning.json"); reopened.load(output);
            require(reopened.validate().empty() && parse(reopened.serialize()) == variant, "Editor lower-approach save/reopen changed source");
            return output;
        }

        MatchSetup settings(bool side_b)
        {
            MatchSetup result; result.set_stage("turbulence_slab_tools"); result.set_player_count(2);
            result.set_screen_layout(labrador::ScreenLayout::two_player); result.begin_input_claims(3);
            require(result.claim_input_slot(1), "Cannot claim sparse lower-approach target seat");
            result.set_player_team(0, side_b ? PlayerTeam::b : PlayerTeam::a);
            result.set_player_team(1, side_b ? PlayerTeam::a : PlayerTeam::b);
            result.set_player_weapon(0, WeaponType::sniper); result.set_player_weapon(1, WeaponType::sprayer);
            return result;
        }

        class Trial final
        {
        private:
            labrador::Registry<LevelLoadedInfo> infos_{ "Lower-approach tool Level" };
            labrador::ResolutionManager resolution_;
            labrador::ViewportManager viewports_{ &resolution_ };
            labrador::Mouse mouse_;
            labrador::Partitioner partitioner_;
            RectangleF bounds_;
            std::ofstream& trace_;
            std::string label_;
        public:
            Trial(World& world, const fs::path& stage, bool side_b, std::ofstream& trace, const std::string& label) :
                trace_(trace), label_(label)
            {
                auto info = std::make_unique<LevelLoadedInfo>(labrador::path_to_utf8(stage).c_str()); info->validate();
                const LevelObjectBuilder objects(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
                info->validate_content(objects, world.render, world.audio); bounds_ = info->out_of_bounds_rectangle();
                const auto setup = settings(side_b); infos_.add(setup.stage(), std::move(info));
                viewports_.set_layout(setup.screen_layout());
                LevelBuilder builder(&viewports_, &world.render, &world.audio, &world.preferences, &infos_,
                    &world.tuning, &world.presentation, &resolution_, nullptr, &partitioner_, &mouse_);
                std::srand(seed); level = builder.build_level(setup, TeamColourPair::blue_orange, OpeningMode::off);
                require(level->player_count() == 2, "Lower-approach builder lost the opposed roster"); record("constructed", 0);
            }
            void tick(const char* phase, int phase_update, const PlayerInputData& action = moving_structure_acceptance::input())
            {
                class Inputs final : public SeatInputFilter
                {
                public:
                    explicit Inputs(const PlayerInputData& value) : action(value) {}
                    void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
                    {
                        require(seats.size() == 2, "Lower-approach input roster changed");
                        for (auto& seat : seats) { seat = moving_structure_acceptance::input(); }
                        seats[0] = action;
                    }
                    PlayerInputData action;
                } filter(action);
                level->update({}, moving_structure_acceptance::dt, &filter); ++updates;
                require(updates <= 181 + maximum_readiness + observation_updates, "Lower-approach scene exceeded its finite update bound");
                record(phase, phase_update);
            }
            void record(const char* phase, int phase_update)
            {
                for (int view = 0; view < 2; ++view)
                {
                    const auto& player = level->player(view); const auto& body = player.rectangle();
                    require(std::isfinite(body.x) && std::isfinite(body.y) && bounds_.contains(body) &&
                        std::isfinite(player.health()) && std::isfinite(player.weapon_ammo()), "Non-finite or out-of-bounds lower-approach actor");
                    trace_ << label_ << ',' << phase << ',' << phase_update << ',' << updates << ',' << view << ','
                        << player.player_num() << ',' << static_cast<int>(player.team()) << ',' << body.x << ',' << body.y << ','
                        << body.width << ',' << body.height << ',' << player.velocity().x << ',' << player.velocity().y << ','
                        << static_cast<int>(player.state()) << ',' << static_cast<int>(player.move_state()) << ',' << player.carrier_id() << ','
                        << player.health() << ',' << player.weapon_ammo() << ',' << static_cast<int>(player.primary_weapon().readiness()) << ','
                        << level->projectile_count() << '\n';
                }
            }
            std::unique_ptr<Level> level;
            int updates = 0;
        };

        struct Result
        {
            RectangleF source, target;
            float source_health = 0.0f, initial_health = 0.0f, final_health = 0.0f, minimum_health = 0.0f;
            float ammo_initial = 0.0f, ammo_after = 0.0f, ammo_final = 0.0f, target_ammo = 0.0f, angle = 0.0f;
            float total_owned = 0.0f, runtime_area = 0.0f;
            double tile_area = 0.0;
            int first_hit = -1, first_paint = -1, readiness = 0, paid = 0, updates = 0, owner = -1;
            int after_projectiles = 0, peak_projectiles = 0, final_projectiles = 0;
            std::array<std::optional<std::size_t>, 3> sources;
            std::array<int, 3> first_group_paint{ -1, -1, -1 };
            Areas areas{};
        };

        std::array<std::optional<std::size_t>, 3> source_map(const fs::path& stage, bool side_b, bool paint_pass)
        {
            const auto source = parse(read_text(stage));
            const std::array<std::string, 3> names{ side_b ? "fixed_slab_b" : "fixed_slab_a",
                side_b ? "fixed_slab_a" : "fixed_slab_b", side_b ? "lower_bed_b" : "lower_bed_a" };
            std::array<std::optional<std::size_t>, 3> result; std::size_t ordinal = 0;
            for (const auto& object : source["collision_objects"].GetArray())
            {
                const std::string type = object["type"].GetString();
                if (type != "StructurePaintable" && type != "FixedSlabPaintable") { continue; }
                for (std::size_t group = 0; group < names.size(); ++group)
                {
                    if (object["name"].GetString() == names[group])
                    { require(!result[group], "Duplicate lower-approach receiver"); result[group] = ordinal; }
                }
                ++ordinal;
            }
            require(result[0].has_value() == !paint_pass && result[1] && result[2], "Lower-approach source map lost a receiver or optional absence");
            return result;
        }

        Areas owned_areas(const Level& level, const Result& result)
        {
            Areas areas{}; std::array<bool, 3> seen{};
            for (const auto& sample : level.paint_surfaces())
            {
                for (std::size_t group = 0; group < result.sources.size(); ++group)
                {
                    if (!result.sources[group] || sample.source_index != *result.sources[group]) { continue; }
                    seen[group] = true;
                    if (sample.team == level.player(0).team() && sample.paint_owner == result.owner)
                    { areas[group][static_cast<std::size_t>(sample.face)] += sample.scored_area; }
                }
            }
            for (std::size_t group = 0; group < seen.size(); ++group)
            { require(!result.sources[group] || seen[group], "Public collector lost a lower-approach source ordinal"); }
            return areas;
        }

        Result measure(World& world, const fs::path& stage, bool side_b, bool paint_pass,
            std::ofstream& trace, const std::string& label)
        {
            Trial trial(world, stage, side_b, trace, label); Result result;
            int count_in = 0;
            while (!trial.level->match_started() && count_in < 240) { trial.tick("count_in", ++count_in); }
            require(count_in == 181 && trial.level->match_started(), "Lower-approach case changed the full production count-in");
            const auto expected_source = station(world.tuning.player().size, side_b, false);
            const auto expected_target = station(world.tuning.player().size, side_b, true);
            require(trial.level->player(1).state() == PlayerState::alive &&
                trial.level->player(1).health() == world.tuning.player().starting_health,
                "Neutral guest must be alive/full before the single public setup placement");
            // One explicitly controlled defender setup after admission, before
            // readiness. Source admission uses the real editor-authored legal
            // spawn. This measures neither earned access nor route cost.
            require(trial.level->place_player(1, expected_target.position()), "Public lower-approach target placement rejected its seat");
            trial.record("target_setup_placement", 1);
            const auto stable = [&](const Player& player, const RectangleF& expected)
            {
                return player.state() == PlayerState::alive && supported(player) && player.carrier_id() == 0 &&
                    close_value(player.velocity().x, 0.0f) && close_value(player.velocity().y, 0.0f) && same_pose(player.rectangle(), expected);
            };
            do
            {
                trial.tick("readiness", ++result.readiness);
                require(same_pose(trial.level->player(0).rectangle(), expected_source) &&
                    same_pose(trial.level->player(1).rectangle(), expected_target), "Neutral readiness moved a lower-approach station");
            } while (result.readiness < 60 || (trial.level->player(0).primary_weapon().readiness() != WeaponReadiness::ready &&
                result.readiness < maximum_readiness));
            const auto& source = trial.level->player(0); const auto& target = trial.level->player(1);
            require(stable(source, expected_source) && stable(target, expected_target) &&
                source.primary_weapon().readiness() == WeaponReadiness::ready && trial.level->projectile_count() == 0 &&
                source.health() == world.tuning.player().starting_health && target.health() == world.tuning.player().starting_health &&
                source.weapon_ammo() == 1.0f && target.weapon_ammo() == 1.0f && source.team() != target.team(),
                "Lower-approach actors did not reach stable full-health/full-reserve neutral readiness");
            result.source = source.rectangle(); result.target = target.rectangle(); result.source_health = source.health();
            result.initial_health = target.health(); result.minimum_health = target.health(); result.ammo_initial = source.weapon_ammo();
            result.target_ammo = target.weapon_ammo(); result.owner = paint_owners::index(source.player_num(), 0);
            require(source.player_num() == 3 && target.player_num() == 1 && result.owner == 3, "Lower-approach sparse owner mapping changed");
            result.sources = source_map(stage, side_b, paint_pass);
            result.runtime_area = trial.level->level_end_info().total_area;
            for (const auto& sample : trial.level->paint_surfaces())
            { require(sample.scored_area > 0.0f && std::isfinite(sample.scored_area), "Invalid public tile budget"); result.tile_area += sample.scored_area; }
            require(owned_areas(*trial.level, result) == Areas{} && trial.level->level_end_info().coverage_ratio() == 0.0f,
                "Lower-approach scene did not start with neutral public paint");
            result.angle = (target.center() - source.center()).angle();
            const auto direction = Vector2F::unit_vec_from_angle(result.angle);
            require(close_value(direction.x, 0.0f) && close_value(direction.y, -1.0f), "Lower-approach fixed aim is not upward at the actual ready target");
            for (int update = 1; update <= observation_updates; ++update)
            {
                const float ammo_before = source.weapon_ammo();
                trial.tick("observation", update, moving_structure_acceptance::aimed_input(direction, result.angle, update == 1));
                require(stable(source, result.source) && source.health() == result.source_health &&
                    target.weapon_ammo() == result.target_ammo, "Lower-approach source moved/took damage or neutral target spent paint");
                if (source.weapon_ammo() < ammo_before) { ++result.paid; require(update == 1, "Released lower-approach input paid for another shot"); }
                if (update == 1)
                {
                    result.ammo_after = source.weapon_ammo(); result.after_projectiles = trial.level->projectile_count();
                    require(result.paid == 1 && std::abs((result.ammo_initial - result.ammo_after) -
                        world.tuning.weapon(WeaponType::sniper).details.ammo_usage) <= 0.000003f,
                        "Lower-approach ready pulse did not pay the source Sniper cost exactly once");
                }
                result.peak_projectiles = (std::max)(result.peak_projectiles, trial.level->projectile_count());
                result.minimum_health = (std::min)(result.minimum_health, target.health());
                if (target.health() < result.initial_health && result.first_hit < 0) { result.first_hit = update; }
                result.areas = owned_areas(*trial.level, result);
                result.total_owned = trial.level->level_end_info().player_areas[static_cast<std::size_t>(result.owner)];
                if (result.total_owned > 0.0f && result.first_paint < 0) { result.first_paint = update; }
                for (std::size_t group = 0; group < result.areas.size(); ++group)
                {
                    double area = 0.0; for (double face : result.areas[group]) { area += face; }
                    if (area > 0.0 && result.first_group_paint[group] < 0) { result.first_group_paint[group] = update; }
                }
                require(trial.level->level_end_info().total_area == result.runtime_area, "Observation changed the geometry score budget");
            }
            result.final_health = target.health(); result.ammo_final = source.weapon_ammo();
            result.final_projectiles = trial.level->projectile_count(); result.updates = trial.updates;
            require(result.paid == 1 && result.ammo_final >= result.ammo_after, "Lower-approach pulse count or natural reserve recovery changed");
            return result;
        }

        void matched(const Result& solid, const Result& pass)
        {
            require(same_pose(solid.source, pass.source) && same_pose(solid.target, pass.target) &&
                solid.source_health == pass.source_health && solid.initial_health == pass.initial_health &&
                solid.ammo_initial == pass.ammo_initial && solid.target_ammo == pass.target_ammo &&
                solid.readiness == pass.readiness && solid.angle == pass.angle && solid.ammo_after == pass.ammo_after &&
                solid.paid == pass.paid && solid.owner == pass.owner,
                "Solid/paint-pass control lost its matched actual ready pose, health, input, cost or fixed clock");
        }
    }

    void run(const fs::path& content, const fs::path& slab, const fs::path& evidence)
    {
        SourceGuard guard(content, slab); qualify(content, slab); World world(content);
        std::ofstream trace(evidence / "poses.csv"), results(evidence / "outcomes.csv"), faces(evidence / "faces.csv");
        require(trace.good() && results.good() && faces.good(), "Cannot create lower-approach receipts");
        trace << std::setprecision(17); results << std::setprecision(17); faces << std::setprecision(17);
        trace << "trial,phase,phase_update,total_update,view,raw_slot,team,x,y,width,height,vx,vy,state,move_state,carrier,health,ammo,readiness,projectiles\n";
        results << "side,geometry,seed,dt,count_in,readiness,observation,updates,setup_placements,post_ready_placements,paid,owner,source_x,source_y,target_x,target_y,angle,source_health,target_initial_health,target_minimum_health,target_final_health,first_hit,ammo_initial,ammo_after,ammo_final,target_ammo,after_projectiles,peak_projectiles,final_projectiles,first_paint,total_owned,runtime_float_budget,public_tile_budget\n";
        faces << "side,geometry,group,present,source_index,first_paint,face,source_team,owner,held_area\n";
        int updates = 0, hits = 0, underside_positive = 0;
        for (bool side_b : { false, true })
        {
            std::array<Result, 2> pair;
            for (bool paint_pass : { false, true })
            {
                const std::string side = side_b ? "b" : "a", geometry = paint_pass ? "paint_pass" : "solid";
                const std::string label = side + "_" + geometry;
                const auto stage = author(content, slab, evidence / (label + ".json"), world.tuning.player().size, side_b, paint_pass);
                const auto result = measure(world, stage, side_b, paint_pass, trace, label);
                pair[paint_pass ? 1u : 0u] = result; updates += result.updates;
                if (result.first_hit >= 0) { ++hits; }
                if (result.areas[0][static_cast<std::size_t>(PaintSurfaceFace::bottom)] > 0.0) { ++underside_positive; }
                results << side << ',' << geometry << ',' << seed << ',' << moving_structure_acceptance::dt << ",181," << result.readiness << ",180,"
                    << result.updates << ",1,0," << result.paid << ',' << result.owner << ',' << result.source.x << ',' << result.source.y << ','
                    << result.target.x << ',' << result.target.y << ',' << result.angle << ',' << result.source_health << ',' << result.initial_health << ','
                    << result.minimum_health << ',' << result.final_health << ',' << result.first_hit << ',' << result.ammo_initial << ',' << result.ammo_after << ','
                    << result.ammo_final << ',' << result.target_ammo << ',' << result.after_projectiles << ',' << result.peak_projectiles << ',' << result.final_projectiles << ','
                    << result.first_paint << ',' << result.total_owned << ',' << result.runtime_area << ',' << result.tile_area << '\n';
                for (std::size_t group = 0; group < result.sources.size(); ++group)
                {
                    for (std::size_t face = 0; face < face_names.size(); ++face)
                    {
                        faces << side << ',' << geometry << ',' << group_names[group] << ',' << result.sources[group].has_value() << ',';
                        if (result.sources[group]) { faces << *result.sources[group]; } else { faces << -1; }
                        faces << ',' << result.first_group_paint[group] << ',' << face_names[face] << ',' << static_cast<int>(side_b ? PlayerTeam::b : PlayerTeam::a)
                            << ',' << result.owner << ',' << result.areas[group][face] << '\n';
                    }
                }
            }
            matched(pair[0], pair[1]);
        }
        trace.flush(); results.flush(); faces.flush();
        require(trace.good() && results.good() && faces.good(), "Cannot finish lower-approach receipts"); guard.check();
        std::ofstream summary(evidence / "summary.txt");
        summary << "fresh_scenes=4\nsetup_defender_placements=4\npost_ready_placements=0\npaid_sniper_pulses=4\nobservation_updates_each=180\n"
            << "total_updates=" << updates << "\nmaximum_updates=1984\nmeasured_target_hit_scenes=" << hits
            << "\nmeasured_own_slab_underside_positive_scenes=" << underside_positive << "\n"
            << "scope=controlled lower-floor upward aim at defender on own top support; paint-pass replaces only own slab with existing unscored ramp\n"
            << "outcomes=measured health and public local-face ownership; no forced hit, cover advantage or exact painted area\n"
            << "limits=no earned access, tracking, extra pulses, opposed survival, human acceptance, real device or global balance claim\n";
        summary.flush(); require(summary.good(), "Cannot finish lower-approach summary");
    }
}
