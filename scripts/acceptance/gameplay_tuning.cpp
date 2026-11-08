#include "game/pch.h"
#include "game/objects/weapon_builder.h"
#include "game/objects/projectile_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "scripts/acceptance/support/json_fixture.h"
#include "scripts/acceptance/support/probe.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;

namespace
{
    using acceptance::require;

    constexpr ProjectileType projectile_types[]{ ProjectileType::spray, ProjectileType::jet,
        ProjectileType::rolling, ProjectileType::mist, ProjectileType::ball };
    constexpr const char* projectile_names[]{ "spray", "jet", "rolling", "mist", "ball" };
    constexpr float fan_order[]{ 0.0f, 1.0f, -1.0f, 2.0f, -2.0f };

    struct Fixtures : acceptance::JsonVariants
    {
        Fixtures(const std::filesystem::path& content, const std::filesystem::path& folder) :
            acceptance::JsonVariants(content / "tuning.json", L"variant", folder) {}

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
                Colour::white, type, Vector2F::zero, &render, &audio, &preferences);
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
            Projectile(velocity, PlayerTeam::a, 0, Colour::white, type, render,
                resolve_projectile_strip(*tuning, *render), tuning),
            rectangle_(Vector2F::zero, tuning->details.col_rect_size)
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
            IntegratorProbe lifetime(&record, type, Vector2F::zero, &world.render);
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
            const float angle = legacy ? fan_order[index] * pi / 12.0f : fan_order[index] * tuning.spread_step;
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
        input.shoot_angle = -pi / 2.0f; // Roller deliberately follows facing.
        for (const bool right : { true, false })
        {
            require(weapon.update_and_get_projectiles(input, center, {100.0f, -80.0f}, right, 0.0f).empty(),
                "A released roller fired during its aim check");
            const float rotation = legacy ? (right ? pi / 4.0f : 3.0f * pi / 4.0f) :
                (right ? tuning.aim_tilt : pi - tuning.aim_tilt);
            require(weapon.rotation() == rotation &&
                weapon.nozzle_pos() == expected_nozzle(tuning.details, center, rotation, right),
                "Actual roller angle or mirrored launch nozzle changed from its reference formula");
        }
    }

    void check_actual_objects(World& world, const GameTuning& tuning, bool legacy)
    {
        auto roller = world.weapon(tuning, WeaponType::roller);
        check_roller(*roller, tuning.weapon(WeaponType::roller), legacy);
        const ProjectileBuilder builder(&tuning, &world.render);
        for (const Vector2F velocity : { Vector2F(1000.0f, 0.0f), Vector2F(-750.0f, 200.0f) })
        {
            auto mist = builder.build_projectiles(Vector2F::zero, velocity,
                PlayerTeam::a, 0, Colour::white, ProjectileType::mist, &world.render);
            check_mist(mist, tuning.projectile(ProjectileType::mist), velocity, legacy);
        }

        auto mister = world.weapon(tuning, WeaponType::mister);
        PlayerInputData input;
        input.connected = true;
        input.shoot_direction_requested = true;
        input.shoot_angle = 0.35f;
        require(mister->update_and_get_projectiles(input, Vector2F::zero, Vector2F::zero, true,
            tuning.weapon(WeaponType::mister).details.shoot_interval + 1.0f).empty(), "Released mister fired while priming");
        input.primary_shoot = true;
        auto fired = mister->update_and_get_projectiles(input, Vector2F::zero, Vector2F::zero, true, 0.0f);
        const Vector2F velocity = Vector2F::unit_vec_from_angle(mister->rotation()) *
            tuning.weapon(WeaponType::mister).details.starting_vel_length;
        check_mist(fired, tuning.projectile(ProjectileType::mist), velocity, legacy);

        for (const ProjectileType type : projectile_types)
        {
            const auto& record = tuning.projectile(type);
            auto objects = builder.build_projectiles(Vector2F::zero, {1200.0f, -300.0f},
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
        const ProjectileBuilder builder(&live, &world.render);
        auto existing_mist = builder.build_projectiles(Vector2F::zero, {1000.0f, 0.0f},
            PlayerTeam::a, 0, Colour::white, ProjectileType::mist, &world.render);
        IntegratorProbe flying(&live.projectile(ProjectileType::spray), ProjectileType::spray,
            {60.0f, 0.0f}, &world.render);
        const ProjectileTuning* flying_record = flying.borrowed_record();
        auto existing = builder.build_projectiles(Vector2F::zero, {60.0f, 0.0f},
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
        auto new_mist = builder.build_projectiles(Vector2F::zero, {1000.0f, 0.0f},
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
            d["weapons"]["roller"]["aim_tilt"].SetFloat(std::nextafter(pi / 2.0f, 0.0f));
            d["projectiles"]["mist"]["spread_step"].SetFloat(pi / 4.0f);
        });
        require(angle_upper.weapon(WeaponType::roller).aim_tilt < pi / 2.0f &&
            angle_upper.projectile(ProjectileType::mist).spread_step == pi / 4.0f,
            "The valid upper angle boundaries were rejected or changed");
        const auto max_factor = fixtures.load([](rapidjson::Document& d)
        {
            d["projectiles"]["wind_stop_factor"].SetFloat(std::numeric_limits<float>::max());
            for (const char* name : projectile_names) { d["projectiles"][name]["wind_resistance"].SetInt(1); }
            // A sub's two records share the factor (ROAD#11).
            for (const char* name : sub_names)
            {
                d["subs"][name]["body"]["wind_resistance"].SetInt(1);
                d["subs"][name]["burst"]["wind_resistance"].SetInt(1);
            }
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
            { d["weapons"]["roller"]["aim_tilt"].SetFloat(pi / 2.0f); });
        fixtures.rejects("mist", "spread_step", [](rapidjson::Document& d)
            { d["projectiles"]["mist"]["spread_step"].SetFloat(std::nextafter(pi / 4.0f, pi)); });
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

    // The shared content-value reads (game/config/content_value.h) and the
    // cross-field rules written at the end of each reader.
    void check_rumble_tuning(Fixtures& fixtures, const GameTuning& defaults)
    {
        constexpr const char* names[]{ "hit", "elimination", "hard_landing", "dry_fire" };
        const RumbleProfile expected{ RumblePulse{ { 0.35f, 0.55f }, 0.10f },
            RumblePulse{ { 0.80f, 0.40f }, 0.25f }, RumblePulse{ { 0.40f, 0.15f }, 0.08f },
            RumblePulse{ { 0.0f, 0.25f }, 0.04f } };
        for (size_t event = 0; event < expected.size(); ++event)
        {
            const RumblePulse& actual = defaults.rumble()[event];
            require(actual.motors.low_frequency == expected[event].motors.low_frequency &&
                actual.motors.high_frequency == expected[event].motors.high_frequency &&
                actual.duration_seconds == expected[event].duration_seconds,
                "Named rumble data was assigned to the wrong event or motor");
        }
        fixtures.rejects("rumble", "rumble", [](rapidjson::Document& d) { d.RemoveMember("rumble"); });
        for (const char* name : names)
        {
            fixtures.rejects("rumble", name, [&](rapidjson::Document& d) { d["rumble"].RemoveMember(name); });
            fixtures.rejects("rumble", name, [&](rapidjson::Document& d) { d["rumble"][name].SetBool(false); });
            for (const char* field : { "low_frequency", "high_frequency", "duration" })
            {
                fixtures.rejects(name, field, [&](rapidjson::Document& d) { d["rumble"][name].RemoveMember(field); });
                fixtures.rejects(name, field, [&](rapidjson::Document& d) { d["rumble"][name][field].SetNull(); });
                fixtures.rejects(name, field, [&](rapidjson::Document& d)
                    { d["rumble"][name][field].SetString("0.5", d.GetAllocator()); });
                for (const double invalid : { -0.01, 1.01, 1e100 })
                {
                    fixtures.rejects(name, field, [&](rapidjson::Document& d)
                        { d["rumble"][name][field].SetDouble(invalid); });
                }
            }
        }
        for (const float boundary : { 0.0f, 1.0f })
        {
            const GameTuning edges = fixtures.load([&](rapidjson::Document& d)
            {
                for (const char* name : names)
                {
                    d["rumble"][name]["low_frequency"].SetFloat(boundary);
                    d["rumble"][name]["high_frequency"].SetFloat(boundary);
                    d["rumble"][name]["duration"].SetFloat(boundary);
                }
            });
            for (const RumblePulse& pulse : edges.rumble())
            {
                require(pulse.motors.low_frequency == boundary && pulse.motors.high_frequency == boundary &&
                    pulse.duration_seconds == boundary, "Rumble zero/maximum boundary was rejected or changed");
            }
            require(edges.starting_ammo() == defaults.starting_ammo() &&
                edges.player().x_acceleration == defaults.player().x_acceleration &&
                edges.weapon(WeaponType::sprayer).details.ammo_usage == defaults.weapon(WeaponType::sprayer).details.ammo_usage,
                "Rumble data changed unrelated gameplay tuning");
        }
    }

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
            { { "player", "animations", "run_speed" }, 0.0 },
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

    // ENG#31: every tuning.json object names the keys its reader reads. A key
    // it does not read, or a name written twice, fails at load with the file,
    // the object and the key; a '_' key is a comment.
    void check_known_keys(Fixtures& fixtures, const GameTuning& defaults)
    {
        const auto add = [](rapidjson::Document& d, rapidjson::Value& object, const char* key,
            rapidjson::Value value)
        {
            object.AddMember(rapidjson::Value(key, d.GetAllocator()), value, d.GetAllocator());
        };
        const auto number = [](double value) { return rapidjson::Value(value); };
        const auto copy = [](rapidjson::Document& d, const rapidjson::Value& value)
        {
            return rapidjson::Value(value, d.GetAllocator());
        };

        // Unknown at the top level and in nested objects, down to a vector and
        // a sound. paint_undrfoot is the report's exhibit: an optional block
        // that a misspelling used to switch off without a word.
        fixtures.rejects("the document", "pleyer", [&](rapidjson::Document& d)
            { add(d, d, "pleyer", rapidjson::Value(rapidjson::kObjectType)); });
        fixtures.rejects("player", "paint_undrfoot", [&](rapidjson::Document& d)
            { add(d, d["player"], "paint_undrfoot", copy(d, d["player"]["paint_underfoot"])); });
        fixtures.rejects("runoff", "dealy", [&](rapidjson::Document& d)
            { add(d, d["paint"]["visual"]["runoff"], "dealy", number(0.1)); });
        fixtures.rejects("size", "z", [&](rapidjson::Document& d)
            { add(d, d["player"]["size"], "z", number(1.0)); });
        fixtures.rejects("go", "volum", [&](rapidjson::Document& d)
            { add(d, d["match"]["sounds"]["go"], "volum", number(0.5)); });
        fixtures.rejects("duration_choices", "minutes", [&](rapidjson::Document& d)
            { add(d, d["match"]["duration_choices"][0], "minutes", number(1.0)); });

        // Each weapon has its own list: a misspelling, and keys another
        // weapon reads that this one's class never acts on.
        fixtures.rejects("roller", "aim_tlit", [&](rapidjson::Document& d)
            { add(d, d["weapons"]["roller"], "aim_tlit", number(0.3)); });
        fixtures.rejects("sprayer", "aim_tilt", [&](rapidjson::Document& d)
            { add(d, d["weapons"]["sprayer"], "aim_tilt", number(0.3)); });
        fixtures.rejects("sniper", "loop_sounds", [&](rapidjson::Document& d)
            { add(d, d["weapons"]["sniper"], "loop_sounds", copy(d, d["weapons"]["sprayer"]["loop_sounds"])); });
        fixtures.rejects("sniper", "add_player_velocity", [&](rapidjson::Document& d)
        {
            add(d, d["weapons"]["sniper"], "add_player_velocity", rapidjson::Value("x_only", d.GetAllocator()));
            add(d, d["weapons"]["sniper"], "player_velocity_amount", number(1.0));
        });
        fixtures.rejects("roller", "add_player_velocity", [&](rapidjson::Document& d)
            { add(d, d["weapons"]["roller"], "add_player_velocity", rapidjson::Value("x_only", d.GetAllocator())); });
        fixtures.rejects("feedback_sounds", "stop", [&](rapidjson::Document& d)
            { add(d, d["weapons"]["bucket"]["feedback_sounds"], "stop", copy(d, d["weapons"]["sprayer"]["feedback_sounds"]["stop"])); });
        fixtures.rejects("feedback_sounds", "lift", [&](rapidjson::Document& d)
            { add(d, d["weapons"]["mister"]["feedback_sounds"], "lift", copy(d, d["weapons"]["roller"]["feedback_sounds"]["lift"])); });
        fixtures.rejects("jet", "spread_step", [&](rapidjson::Document& d)
            { add(d, d["projectiles"]["jet"], "spread_step", number(0.1)); });
        fixtures.rejects("ball", "diffusion", [&](rapidjson::Document& d)
            { add(d, d["projectiles"]["ball"], "diffusion", copy(d, d["projectiles"]["spray"]["diffusion"])); });

        // A name written twice: has() and every read take the first.
        fixtures.rejects("player", "gravity", [&](rapidjson::Document& d)
            { add(d, d["player"], "gravity", number(1.0)); });
        fixtures.rejects("sniper", "shoot_interval", [&](rapidjson::Document& d)
            { add(d, d["weapons"]["sniper"], "shoot_interval", number(2.0)); });
        fixtures.rejects("bots", "_comment", [&](rapidjson::Document& d)
            { add(d, d["bots"], "_comment", rapidjson::Value("twice", d.GetAllocator())); });

        // Comments of any value are accepted wherever an object is read, and
        // a key one weapon's class acts on is accepted on another that does.
        const GameTuning commented = fixtures.load([&](rapidjson::Document& d)
        {
            add(d, d, "_comment", rapidjson::Value("top", d.GetAllocator()));
            add(d, d["player"]["size"], "_note", number(3.0));
            add(d, d["weapons"]["sniper"], "_comment", rapidjson::Value(rapidjson::kObjectType));
            add(d, d["projectiles"]["mist"]["diffusion"], "_", rapidjson::Value(rapidjson::kArrayType));
            add(d, d["match"]["duration_choices"][0], "_comment", rapidjson::Value(true));
            add(d, d["weapons"]["bucket"]["feedback_sounds"], "ready",
                copy(d, d["weapons"]["sniper"]["feedback_sounds"]["ready"]));
        });
        require(commented.player().size == defaults.player().size &&
            commented.weapon(WeaponType::sniper).details.shoot_interval ==
                defaults.weapon(WeaponType::sniper).details.shoot_interval &&
            commented.projectile(ProjectileType::mist).diffusion.end_time ==
                defaults.projectile(ProjectileType::mist).diffusion.end_time &&
            commented.match().duration_choices == defaults.match().duration_choices &&
            commented.weapon(WeaponType::bucket).feedback_sounds.ready.has_value() &&
            !defaults.weapon(WeaponType::bucket).feedback_sounds.ready.has_value(),
            "A '_' comment key or a bucket ready cue was refused or changed the tuning read");
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("gameplay tuning", [&]
    {
        require(argc == 2, "Usage: gameplay_tuning.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        Fixtures fixtures(content, std::filesystem::absolute(argv[0]).parent_path() / "gameplay-tuning-fixtures");
        World world(content);
        const GameTuning defaults((content / "tuning.json").string().c_str());
        require(defaults.weapon(WeaponType::roller).aim_tilt == pi / 4.0f &&
            defaults.projectile(ProjectileType::mist).spread_step == pi / 12.0f,
            "Authored aim/spread values do not preserve the previous binary32 constants");
        for (const float step : fan_order)
        {
            require(step * defaults.projectile(ProjectileType::mist).spread_step == step * pi / 12.0f,
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
        check_rumble_tuning(fixtures, defaults);
        check_known_keys(fixtures, defaults);
        std::cout << "PASS gameplay tuning: exact legacy roller/mist/integrator parity, real custom values, "
            "stable live borrowed records, strict boundaries, named rumble data, contextual rejection "
            "and refused unknown or repeated keys.\n";
        return 0;
    });
}
