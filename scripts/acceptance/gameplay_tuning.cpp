#include "game/pch.h"
#include "game/objects/weapon_builder.h"
#include "game/objects/projectile_builder.h"
#include "engine/assets/sprite_sheet_loader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    constexpr ProjectileType projectile_types[]{ ProjectileType::spray, ProjectileType::jet,
        ProjectileType::rolling, ProjectileType::mist, ProjectileType::ball };
    constexpr const char* projectile_names[]{ "spray", "jet", "rolling", "mist", "ball" };
    constexpr float fan_order[]{ 0.0f, 1.0f, -1.0f, 2.0f, -2.0f };

    struct Fixtures
    {
        rapidjson::Document source;
        std::filesystem::path scratch;
        size_t next = 0;

        Fixtures(const std::filesystem::path& content, const std::filesystem::path& folder) : scratch(folder)
        {
            std::ifstream input(content / "tuning.json", std::ios::binary);
            require(input.good(), "Cannot read authored gameplay tuning");
            const std::string text{ std::istreambuf_iterator<char>(input), {} };
            source.Parse(text.c_str());
            require(!source.HasParseError(), "Cannot parse authored gameplay tuning");
            std::filesystem::create_directories(scratch);
        }

        template<class Edit>
        std::filesystem::path write(Edit edit)
        {
            rapidjson::Document document;
            document.CopyFrom(source, document.GetAllocator());
            edit(document);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            require(document.Accept(writer), "Cannot encode isolated gameplay tuning");
            const auto path = scratch / ("variant-" + std::to_string(next++) + ".json");
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            output.close();
            require(output.good(), "Cannot write isolated gameplay tuning");
            return path;
        }

        template<class Edit>
        GameTuning load(Edit edit)
        {
            const auto path = write(edit);
            return GameTuning(path.string().c_str());
        }

        template<class Edit>
        void rejects(const char* owner, const char* field, Edit edit)
        {
            const auto path = write(edit);
            bool contextual = false;
            try { const GameTuning invalid(path.string().c_str()); }
            catch (const std::runtime_error& error)
            {
                const std::string message = error.what();
                contextual = message.find(path.filename().string()) != std::string::npos &&
                    message.find(owner) != std::string::npos && message.find(field) != std::string::npos;
            }
            require(contextual, std::string("Invalid ") + owner + '.' + field +
                " did not fail at load with its source file and key");
        }
    };

    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;

        explicit World(const std::filesystem::path& content)
        {
            for (const char* sheet : { "sprite_sheet_1", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }

        std::unique_ptr<Weapon> weapon(const GameTuning& tuning, WeaponType type)
        {
            return WeaponBuilder::build_weapon(&tuning, PlayerTeam::a, 0, 0,
                Colour::white, type, Vector2F::ZERO, &render, &audio, &preferences);
        }
    };

    // Concrete production projectiles are observed through their real shapes.
    // This small client exposes only the inherited integrator's protected
    // velocity and age, so threshold/clamp checks can compare exact floats.
    class IntegratorProbe final : public Projectile
    {
    public:
        IntegratorProbe(const ProjectileTuning* tuning, ProjectileType type,
            const Vector2F& velocity, RenderResources* render) :
            Projectile(velocity, PlayerTeam::a, 0, Colour::white, type, render, tuning),
            rectangle_(Vector2F::ZERO, tuning->details.col_rect_size)
        {
        }
        void update(float dt) override
        {
            const Vector2F displacement = this->update_movement(this->details().gravity,
                this->details().wind_resistance, dt);
            rectangle_.offset(displacement.x, displacement.y);
        }
        void draw(DrawList&) const override {}
        RectangleF bounds() const override { return rectangle_; }
        const Shape* shape() const override { return &rectangle_; }
        Vector2F current_velocity() const { return this->velocity(); }
        float age() const { return this->timer(); }
        const ProjectileTuning* borrowed_record() const { return &this->tuning(); }
    private:
        RectangleF rectangle_;
        void set_rectangle(const RectangleF& rectangle) override { rectangle_ = rectangle; }
    };

    struct Reference
    {
        Vector2F velocity;
        RectangleF rectangle;
        float age = 0.0f;
        bool first_update = true;
        bool retired = false;

        void update(const ProjectileTuning& tuning, float wind_stop_factor, float dt)
        {
            // This is the previous gameplay formula, including its strict
            // cutoff, drag-before-clamp order and first-sweep lifetime grace.
            velocity.y += tuning.details.gravity * dt;
            if (velocity.y > tuning.max_velocity.y) { velocity.y = tuning.max_velocity.y; }
            else if (velocity.y < -tuning.max_velocity.y) { velocity.y = -tuning.max_velocity.y; }
            if (std::fabs(velocity.x) > tuning.details.wind_resistance * wind_stop_factor)
            {
                if (velocity.x > 0.0f) { velocity.x += -tuning.details.wind_resistance * dt; }
                else { velocity.x += tuning.details.wind_resistance * dt; }
            }
            else { velocity.x = 0.0f; }
            if (velocity.x > tuning.max_velocity.x) { velocity.x = tuning.max_velocity.x; }
            else if (velocity.x < -tuning.max_velocity.x) { velocity.x = -tuning.max_velocity.x; }
            const Vector2F displacement = velocity * dt;
            rectangle.offset(displacement.x, displacement.y);
            age += dt;
            if (first_update) { first_update = false; }
            else if (age > tuning.details.delete_timer) { retired = true; }
        }
    };

    void check_integrator(World& world, const GameTuning& tuning, float factor)
    {
        for (const ProjectileType type : projectile_types)
        {
            const ProjectileTuning& record = tuning.projectile(type);
            const float cutoff = record.details.wind_resistance * factor;
            for (const float x : { 0.0f, cutoff, -cutoff,
                std::nextafter(cutoff, std::numeric_limits<float>::infinity()),
                -std::nextafter(cutoff, std::numeric_limits<float>::infinity()),
                cutoff * 2.0f + 1.0f, -cutoff * 2.0f - 1.0f, 9000.0f, -9000.0f })
            {
                for (const float y : { -9000.0f, -25.0f, 25.0f, 9000.0f })
                {
                    IntegratorProbe actual(&record, type, {x, y}, &world.render);
                    Reference expected{ {x, y}, actual.bounds() };
                    for (const float dt : { 0.0f, 0.01f, 0.05f, 0.25f, 6.0f, 0.01f })
                    {
                        expected.update(record, factor, dt);
                        actual.update(dt);
                        require(actual.current_velocity() == expected.velocity && actual.bounds() == expected.rectangle &&
                            actual.age() == expected.age && actual.for_deletion() == expected.retired,
                            "Actual projectile integrator changed strict wind cutoff, gravity, clamps, displacement or lifetime");
                    }
                }
            }
            IntegratorProbe lifetime(&record, type, Vector2F::ZERO, &world.render);
            lifetime.update(record.details.delete_timer);
            require(!lifetime.for_deletion(), "First projectile sweep was retired at its lifetime boundary");
            lifetime.update(0.0f);
            require(!lifetime.for_deletion(), "Strict projectile lifetime comparison changed at equality");
            lifetime.update(0.01f);
            require(lifetime.for_deletion(), "Projectile outlived its strict lifetime boundary");
        }
    }

    void check_mist(std::vector<std::unique_ptr<Projectile>>& shots,
        const ProjectileTuning& tuning, const Vector2F& velocity, bool legacy)
    {
        require(shots.size() == std::size(fan_order), "Mist builder changed its five-shot count");
        constexpr float dt = 0.01f; // Before authored diffusion begins.
        for (size_t index = 0; index < shots.size(); ++index)
        {
            const float angle = legacy ? fan_order[index] * PI / 12.0f : fan_order[index] * tuning.spread_step;
            const Vector2F launch = Vector2F::rotate_vector(velocity, angle);
            Reference expected{ launch, shots[index]->shape()->bounding_box() };
            expected.update(tuning, tuning.wind_stop_factor, dt);
            shots[index]->update(dt);
            require(shots[index]->projectile_type() == ProjectileType::mist &&
                shots[index]->shape()->bounding_box() == expected.rectangle &&
                !shots[index]->for_deletion(),
                "Actual mist fan velocity/order differs from its reference firing formula");
        }
    }

    Vector2F expected_nozzle(const WeaponDetails& details, const Vector2F& center, float rotation, bool right)
    {
        Vector2F result = details.nozzle_offset;
        result.x += details.size.x;
        result = Vector2F::rotate_vector(result, rotation);
        result += Vector2F(right ? details.offset.x : -details.offset.x, details.offset.y);
        result += center;
        return result;
    }

    void check_roller(Weapon& weapon, const WeaponTuning& tuning, bool legacy)
    {
        const Vector2F center(300.0f, 400.0f);
        PlayerInputData input;
        input.connected = true;
        input.shoot_direction_requested = true;
        input.shoot_angle = -PI / 2.0f; // Roller deliberately follows facing.
        for (const bool right : { true, false })
        {
            require(weapon.update_and_get_projectiles(input, center, {100.0f, -80.0f}, right, 0.0f).empty(),
                "A released roller fired during its aim check");
            const float rotation = legacy ? (right ? PI / 4.0f : 3.0f * PI / 4.0f) :
                (right ? tuning.aim_tilt : PI - tuning.aim_tilt);
            require(weapon.rotation() == rotation &&
                weapon.nozzle_pos() == expected_nozzle(tuning.details, center, rotation, right),
                "Actual roller angle or mirrored launch nozzle changed from its reference formula");
        }
    }

    void check_actual_objects(World& world, const GameTuning& tuning, bool legacy)
    {
        auto roller = world.weapon(tuning, WeaponType::roller);
        check_roller(*roller, tuning.weapon(WeaponType::roller), legacy);
        const ProjectileBuilder builder(&tuning);
        for (const Vector2F velocity : { Vector2F(1000.0f, 0.0f), Vector2F(-750.0f, 200.0f) })
        {
            auto mist = builder.build_projectiles(Vector2F::ZERO, velocity,
                PlayerTeam::a, 0, Colour::white, ProjectileType::mist, &world.render);
            check_mist(mist, tuning.projectile(ProjectileType::mist), velocity, legacy);
        }

        auto mister = world.weapon(tuning, WeaponType::mister);
        PlayerInputData input;
        input.connected = true;
        input.shoot_direction_requested = true;
        input.shoot_angle = 0.35f;
        require(mister->update_and_get_projectiles(input, Vector2F::ZERO, Vector2F::ZERO, true,
            tuning.weapon(WeaponType::mister).details.shoot_interval + 1.0f).empty(), "Released mister fired while priming");
        input.primary_shoot = true;
        auto fired = mister->update_and_get_projectiles(input, Vector2F::ZERO, Vector2F::ZERO, true, 0.0f);
        const Vector2F velocity = Vector2F::unit_vec_from_angle(mister->rotation()) *
            tuning.weapon(WeaponType::mister).details.starting_vel_length;
        check_mist(fired, tuning.projectile(ProjectileType::mist), velocity, legacy);

        for (const ProjectileType type : projectile_types)
        {
            const auto& record = tuning.projectile(type);
            auto objects = builder.build_projectiles(Vector2F::ZERO, {1200.0f, -300.0f},
                PlayerTeam::a, 0, Colour::white, type, &world.render);
            Projectile& object = *objects.front();
            const RectangleF initial = object.shape()->bounding_box();
            Reference expected{ {1200.0f, -300.0f}, initial };
            expected.update(record, record.wind_stop_factor, 0.01f);
            object.update(0.01f);
            require(object.shape()->bounding_box() == (type == ProjectileType::rolling ? initial : expected.rectangle) &&
                !object.for_deletion(), "Concrete projectile changed its first displacement or one-sweep lifetime grace");
            object.update(6.0f);
            require(object.for_deletion(), "Concrete projectile did not retire after its first sweep and lifetime");
        }
    }

    void check_live_assignment(World& world, const GameTuning& defaults, const GameTuning& custom)
    {
        GameTuning live = defaults;
        const WeaponTuning* roller_record = &live.weapon(WeaponType::roller);
        const ProjectileTuning* mist_record = &live.projectile(ProjectileType::mist);
        auto roller = world.weapon(live, WeaponType::roller);
        const ProjectileBuilder builder(&live);
        auto existing_mist = builder.build_projectiles(Vector2F::ZERO, {1000.0f, 0.0f},
            PlayerTeam::a, 0, Colour::white, ProjectileType::mist, &world.render);
        IntegratorProbe flying(&live.projectile(ProjectileType::spray), ProjectileType::spray,
            {60.0f, 0.0f}, &world.render);
        const ProjectileTuning* flying_record = flying.borrowed_record();
        auto existing = builder.build_projectiles(Vector2F::ZERO, {60.0f, 0.0f},
            PlayerTeam::a, 0, Colour::white, ProjectileType::ball, &world.render);
        const RectangleF initial = existing.front()->shape()->bounding_box();
        live = custom; // Same operation used by developer live tuning.
        require(&live.weapon(WeaponType::roller) == roller_record &&
            &live.projectile(ProjectileType::mist) == mist_record &&
            &live.projectile(ProjectileType::spray) == flying_record,
            "Live tuning assignment invalidated borrowed record addresses");
        check_roller(*roller, live.weapon(WeaponType::roller), false);
        // Spread is a new-shot setting: existing mist retains its old launch fan
        // while still borrowing the newly assigned integrator values.
        check_mist(existing_mist, live.projectile(ProjectileType::mist), {1000.0f, 0.0f}, true);
        flying.update(0.01f);
        require(flying.current_velocity().x == 60.0f - custom.projectile(ProjectileType::spray).details.wind_resistance * 0.01f,
            "An existing projectile ignored its live wind cutoff record");
        auto new_mist = builder.build_projectiles(Vector2F::ZERO, {1000.0f, 0.0f},
            PlayerTeam::a, 0, Colour::white, ProjectileType::mist, &world.render);
        check_mist(new_mist, live.projectile(ProjectileType::mist), {1000.0f, 0.0f}, false);
        Reference expected{ {60.0f, 0.0f}, initial };
        expected.update(live.projectile(ProjectileType::ball), live.projectile(ProjectileType::ball).wind_stop_factor, 0.01f);
        existing.front()->update(0.01f);
        require(existing.front()->shape()->bounding_box() == expected.rectangle && expected.velocity.x != 0.0f,
            "An already-built concrete projectile ignored live wind tuning");
    }

    void check_validation(Fixtures& fixtures)
    {
        const auto angle_zero = fixtures.load([](rapidjson::Document& d)
        {
            d["weapons"]["roller"]["aim_tilt"].SetInt(0);
            d["projectiles"]["mist"]["spread_step"].SetInt(0);
            d["projectiles"]["wind_stop_factor"].SetInt(0);
        });
        require(angle_zero.weapon(WeaponType::roller).aim_tilt == 0.0f &&
            angle_zero.projectile(ProjectileType::mist).spread_step == 0.0f,
            "Zero aim/spread boundaries must be accepted");
        const auto angle_upper = fixtures.load([](rapidjson::Document& d)
        {
            d["weapons"]["roller"]["aim_tilt"].SetFloat(std::nextafter(PI / 2.0f, 0.0f));
            d["projectiles"]["mist"]["spread_step"].SetFloat(PI / 4.0f);
        });
        require(angle_upper.weapon(WeaponType::roller).aim_tilt < PI / 2.0f &&
            angle_upper.projectile(ProjectileType::mist).spread_step == PI / 4.0f,
            "The valid upper angle boundaries were rejected or changed");
        const auto max_factor = fixtures.load([](rapidjson::Document& d)
        {
            d["projectiles"]["wind_stop_factor"].SetFloat(std::numeric_limits<float>::max());
            for (const char* name : projectile_names) { d["projectiles"][name]["wind_resistance"].SetInt(1); }
        });
        require(max_factor.projectile(ProjectileType::spray).wind_stop_factor == std::numeric_limits<float>::max(),
            "Finite wind product at the float boundary was rejected");

        for (const char* field : { "aim_tilt", "spread_step", "wind_stop_factor" })
        {
            const bool roller = std::string(field) == "aim_tilt";
            const bool mist = std::string(field) == "spread_step";
            const char* owner = roller ? "roller" : (mist ? "mist" : "projectiles");
            const auto container = [roller, mist](rapidjson::Document& d) -> rapidjson::Value&
            {
                if (roller) { return d["weapons"]["roller"]; }
                if (mist) { return d["projectiles"]["mist"]; }
                return d["projectiles"];
            };
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { container(d).RemoveMember(field); });
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { container(d)[field].SetString("0.1"); });
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { container(d)[field].SetBool(true); });
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { container(d)[field].SetNull(); });
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { container(d)[field].SetArray(); });
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { container(d)[field].SetDouble(-0.01); });
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { container(d)[field].SetDouble(1e100); });
        }
        fixtures.rejects("roller", "aim_tilt", [](rapidjson::Document& d)
            { d["weapons"]["roller"]["aim_tilt"].SetFloat(PI / 2.0f); });
        fixtures.rejects("mist", "spread_step", [](rapidjson::Document& d)
            { d["projectiles"]["mist"]["spread_step"].SetFloat(std::nextafter(PI / 4.0f, PI)); });
        fixtures.rejects("projectiles", "wind_stop_factor", [](rapidjson::Document& d)
            { d["projectiles"]["wind_stop_factor"].SetFloat(std::numeric_limits<float>::max()); });
        for (const char* name : projectile_names)
        {
            fixtures.rejects(name, "wind_resistance", [name](rapidjson::Document& d)
                { d["projectiles"][name]["wind_resistance"].SetDouble(-1.0); });
            fixtures.rejects(name, "wind_resistance", [name](rapidjson::Document& d)
                { d["projectiles"][name]["wind_resistance"].SetDouble(1e100); });
            fixtures.rejects(name, "wind_resistance", [name](rapidjson::Document& d)
                { d["projectiles"][name]["wind_resistance"].SetFloat(std::numeric_limits<float>::max()); });
            fixtures.rejects(name, "wind_resistance", [name](rapidjson::Document& d)
                { d["projectiles"][name].RemoveMember("wind_resistance"); });
            fixtures.rejects(name, "wind_resistance", [name](rapidjson::Document& d)
                { d["projectiles"][name]["wind_resistance"].SetString("20"); });
        }
    }

    // The shared content-value reads (game/content_value.h) and the
    // cross-field rules written at the end of each reader.
    void check_shared_bounds(Fixtures& fixtures)
    {
        const auto at = [](rapidjson::Document& d, const std::vector<const char*>& path) -> rapidjson::Value&
        {
            rapidjson::Value* value = &d;
            for (const char* key : path) { value = &(*value)[key]; }
            return *value;
        };
        struct Bound { std::vector<const char*> path; double invalid; };
        const Bound bounds[]{
            { { "match", "countdown" }, -0.01 },
            { { "match", "overview" }, -0.01 },
            { { "player", "gravity" }, 1e100 },
            { { "player", "x_initial_velocity" }, 0.0 },
            { { "player", "x_acceleration" }, -0.01 },
            { { "player", "x_deacceleration" }, 0.0 },
            { { "player", "ramp_gravity_multiplier" }, -0.01 },
            { { "player", "jump_max_time" }, -0.01 },
            { { "player", "drop_down_analog_threshold" }, 1.01 },
            { { "player", "respawn_delay" }, -0.01 },
            { { "player", "health_regen_rate" }, -0.01 },
            { { "weapons", "ammo_regen_delay" }, -0.01 },
            { { "weapons", "ammo_regen_rate" }, 1e100 },
            { { "weapons", "sprayer", "shoot_interval" }, -0.01 },
            { { "weapons", "sniper", "starting_vel_length" }, -1.0 },
            { { "weapons", "bucket", "ammo_usage" }, -0.01 },
            { { "projectiles", "spray", "delete_timer" }, 0.0 },
            { { "projectiles", "jet", "player_damage" }, -0.01 },
            { { "projectiles", "ball", "gravity" }, 1e100 },
            { { "projectiles", "mist", "diffusion", "start_time" }, -0.01 },
        };
        for (const Bound& bound : bounds)
        {
            const char* owner = *(bound.path.end() - 2);
            const char* field = *(bound.path.end() - 1);
            fixtures.rejects(owner, field, [&](rapidjson::Document& d) { at(d, bound.path).SetDouble(bound.invalid); });
        }
        fixtures.rejects("projectiles", "max_velocity", [](rapidjson::Document& d)
            { d["projectiles"]["max_velocity"]["x"].SetDouble(0.0); });
        fixtures.rejects("rolling", "collision_size", [](rapidjson::Document& d)
            { d["projectiles"]["rolling"]["collision_size"]["y"].SetDouble(0.0); });

        // A diffusion that would end before it starts.
        fixtures.rejects("diffusion", "start_time", [](rapidjson::Document& d)
        {
            auto& diffusion = d["projectiles"]["spray"]["diffusion"];
            diffusion["start_time"].SetDouble(diffusion["end_time"].GetDouble() + 0.01);
        });

        // The boundaries themselves are accepted unchanged.
        const GameTuning edges = fixtures.load([](rapidjson::Document& d)
        {
            d["player"]["jump_max_time"].SetDouble(0.0);
            d["player"]["x_acceleration"].SetDouble(0.0);
            d["player"]["ramp_gravity_multiplier"].SetDouble(0.0);
            d["player"]["drop_down_analog_threshold"].SetDouble(1.0);
            d["weapons"]["roller"]["shoot_interval"].SetDouble(0.0);
            auto& diffusion = d["projectiles"]["spray"]["diffusion"];
            diffusion["start_time"].SetDouble(diffusion["end_time"].GetDouble());
            d["match"]["overview"].SetDouble(0.0);
        });
        require(edges.match().overview == 0.0f && edges.player().jump_max_time == 0.0f &&
            edges.player().x_acceleration == 0.0f && edges.player().ramp_gravity_multiplier == 0.0f &&
            edges.player().drop_down_analog_threshold == 1.0f &&
            edges.weapon(WeaponType::roller).details.shoot_interval == 0.0f &&
            edges.projectile(ProjectileType::spray).diffusion.start_time ==
                edges.projectile(ProjectileType::spray).diffusion.end_time,
            "A shared content-value boundary was rejected or changed");
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: gameplay_tuning.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        Fixtures fixtures(content, std::filesystem::absolute(argv[0]).parent_path() / "gameplay-tuning-fixtures");
        World world(content);
        const GameTuning defaults((content / "tuning.json").string().c_str());
        require(defaults.weapon(WeaponType::roller).aim_tilt == PI / 4.0f &&
            defaults.projectile(ProjectileType::mist).spread_step == PI / 12.0f,
            "Authored aim/spread values do not preserve the previous binary32 constants");
        for (const float step : fan_order)
        {
            require(step * defaults.projectile(ProjectileType::mist).spread_step == step * PI / 12.0f,
                "Default mist fan changed a legacy binary32 rotation angle");
        }
        for (const ProjectileType type : projectile_types)
        {
            require(defaults.projectile(type).wind_stop_factor == 4.0f &&
                (type == ProjectileType::mist || defaults.projectile(type).spread_step == 0.0f),
                "Shared wind factor or non-mist spread defaults were not loaded correctly");
        }
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::mister, WeaponType::bucket })
        {
            require(defaults.weapon(type).aim_tilt == 0.0f, "Unrelated weapons acquired a roller-only aim tilt");
        }
        check_integrator(world, defaults, 4.0f);
        check_actual_objects(world, defaults, true);
        const auto custom = fixtures.load([](rapidjson::Document& d)
        {
            d["weapons"]["roller"]["aim_tilt"].SetDouble(0.3);
            d["projectiles"]["mist"]["spread_step"].SetDouble(0.1);
            d["projectiles"]["wind_stop_factor"].SetDouble(2.0);
        });
        for (const ProjectileType type : projectile_types)
        {
            require(custom.projectile(type).wind_stop_factor == 2.0f,
                "Shared custom wind factor was not copied into every projectile record");
        }
        check_integrator(world, custom, 2.0f);
        check_actual_objects(world, custom, false);
        check_live_assignment(world, defaults, custom);
        check_validation(fixtures);
        check_shared_bounds(fixtures);
        std::cout << "PASS gameplay tuning: exact legacy roller/mist/integrator parity, real custom values, "
            "stable live borrowed records, strict boundaries and contextual rejection.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL gameplay tuning: " << error.what() << '\n';
        return 1;
    }
}
