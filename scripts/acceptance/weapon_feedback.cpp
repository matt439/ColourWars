#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "game/objects/weapon_feedback.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    bool approximately_equal(float actual, float expected)
    {
        return std::abs(actual - expected) < 0.00001f;
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    constexpr WeaponType tools[] = { WeaponType::sprayer, WeaponType::sniper,
        WeaponType::roller, WeaponType::mister, WeaponType::bucket };

    PlayerInputData input(bool held = false, float movement = 0.0f,
        float angle = 0.0f)
    {
        PlayerInputData result;
        result.connected = true;
        result.primary_shoot = held;
        result.x_movement = movement;
        result.shoot_direction_requested = true;
        result.shoot_angle = angle;
        result.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return result;
    }

    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
                    TextureHandle{}));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
        }

        std::unique_ptr<Weapon> weapon(WeaponType type,
            const WeaponFeedbackPresentation* look,
            const PlaySettings* preference_override = nullptr)
        {
            return WeaponBuilder::build_weapon(&this->tuning, PlayerTeam::a,
                2, 0, Colour::white, type, Vector2F::ZERO,
                &this->render, &this->audio,
                preference_override != nullptr ? preference_override : &this->preferences,
                &this->presentation.tool_motion(), look);
        }

        std::unique_ptr<Weapon> weapon(WeaponType type)
        {
            return this->weapon(type, &this->presentation.weapon_feedback());
        }
    };

    // Only the fixture controls resource boundaries. All event decisions still
    // run through the production weapon update, affordability and refill path.
    class ProbeWeapon final : public Weapon
    {
    public:
        using Weapon::Weapon;

        void prime(float paint)
        {
            this->set_ammo(paint);
            this->set_ammo_timer(0.0f);
            this->set_shoot_timer(this->shoot_interval() + 1.0f);
        }

        void start_regeneration(float paint)
        {
            this->set_ammo(paint);
            this->set_ammo_timer(this->tuning_->ammo_regen_delay());
        }
    };

    auto update(Weapon& weapon, const PlayerInputData& actions = input(),
        float elapsed = dt, const Vector2F& center = Vector2F::ZERO,
        const Vector2F& velocity = Vector2F::ZERO)
    {
        return weapon.update_and_get_projectiles(actions, center, velocity,
            actions.x_movement >= 0.0f, elapsed);
    }

    const WeaponFeedbackState& state(const Weapon& weapon)
    {
        require(weapon.feedback() != nullptr, "Configured weapon lost its feedback object");
        return weapon.feedback()->state();
    }

    void prime(Weapon& weapon, const GameTuning& tuning)
    {
        const float interval = tuning.weapon(weapon.type()).details.shoot_interval;
        for (int tick = 0; static_cast<float>(tick) * dt <= interval + dt; ++tick)
        {
            require(update(weapon).empty(), "Released weapon fired while priming");
        }
        require(weapon.ready_to_shoot(), "Weapon did not become ready after its cooldown");
    }

    void check_timers(World& world)
    {
        const auto& look = world.presentation.weapon_feedback();
        WeaponFeedback feedback(&look, &world.render);
        for (const WeaponType type : tools)
        {
            feedback.clear();
            feedback.shot(type);
            require((feedback.state().muzzle_remaining > 0.0f) == (type != WeaponType::roller),
                "Paid-shot helper emitted a roller muzzle or omitted a firing muzzle");
        }
        feedback.empty();
        const auto empty = feedback.state();
        require(empty.empty_remaining > 0.0f && empty.muzzle_remaining == 0.0f,
            "Empty feedback retained a contradictory muzzle pulse");
        for (const float frozen : { 0.0f, -1.0f,
            std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
        {
            feedback.update(frozen);
            require(feedback.state() == empty,
                "Frozen or invalid delta advanced or corrupted weapon feedback");
        }
        feedback.recovered();
        require(feedback.state().recovered_remaining > 0.0f &&
            feedback.state().empty_remaining == 0.0f,
            "Recovery feedback retained the obsolete empty cue");
        feedback.update(1000.0f);
        require(feedback.state() == WeaponFeedbackState{},
            "Long update retained expired weapon feedback");
        feedback.shot(WeaponType::sprayer);
        feedback.update(dt);
        const auto stream = feedback.state();
        feedback.shot(WeaponType::sprayer);
        require(feedback.state() == stream,
            "A paid stream shot restarted an unexpired muzzle pulse");
        feedback.clear();
        feedback.clear();
        require(feedback.state() == WeaponFeedbackState{},
            "Idempotent clear retained weapon feedback");

        WeaponFeedbackPresentation disabled = look;
        disabled.enabled = false;
        WeaponFeedback quiet(&disabled, &world.render);
        quiet.shot(WeaponType::sniper);
        quiet.empty();
        quiet.recovered();
        quiet.update(dt);
        require(quiet.state() == WeaponFeedbackState{},
            "Disabled weapon presentation emitted a pulse");
        std::cout << "PASS weapon feedback: helper expiry, frozen/invalid deltas, stream limiter and disabled presentation\n";
    }

    void check_reduced_muzzle_draw(World& world)
    {
        WeaponFeedback feedback(&world.presentation.weapon_feedback(), &world.render);
        for (const WeaponType type : tools)
        {
            feedback.clear();
            feedback.shot(type);
            const auto before = feedback.state();
            // This list has no device. Only a deliberately suppressed muzzle
            // may use it; useful status/readiness cues are renderer-tested.
            DrawList suppressed;
            for (int view = 0; view < 4; ++view)
            {
                feedback.draw(suppressed, Vector2F(40.0f, -20.0f), Vector2F::ZERO,
                    PI / 3.0f, type, Colour::white, true);
            }
            require(feedback.state() == before,
                "Reduced-effects muzzle drawing mutated the shared shot event");
            feedback.update(world.presentation.weapon_feedback().muzzle_duration + dt);
            require(feedback.state() == WeaponFeedbackState{},
                "Suppressed muzzle event stopped ageing or survived its lifetime");
        }
        std::cout << "PASS weapon feedback reduced effects: five-tool muzzle draw suppression, frozen fan-out and ordinary expiry\n";
    }

    void check_paid_shots(World& world)
    {
        for (const WeaponType type : tools)
        {
            auto weapon = world.weapon(type);
            require(update(*weapon, input(true)).empty() &&
                state(*weapon) == WeaponFeedbackState{},
                "Initial cooldown request emitted paid-shot feedback");
            prime(*weapon, world.tuning);
            require(!update(*weapon, input(true), 0.0f).empty() &&
                state(*weapon) == WeaponFeedbackState{},
                "Zero-delta paid shot advanced weapon feedback");
            prime(*weapon, world.tuning);
            require(!update(*weapon, input(true)).empty(), "Primed stock weapon did not pay its shot");
            const float first = state(*weapon).muzzle_remaining;
            require((first > 0.0f) == (type != WeaponType::roller),
                "Stock paid shot omitted its pulse or gave the roller a muzzle");
            const auto next = update(*weapon, input(true));
            require(approximately_equal(state(*weapon).muzzle_remaining, std::max(0.0f, first - dt)),
                "Weapon aged its muzzle twice or restarted it on held input");
            if (type == WeaponType::sniper || type == WeaponType::bucket)
            {
                require(next.empty(), "Precision weapon fired during its existing cooldown");
                for (int tick = 0; tick < 15; ++tick)
                {
                    require(update(*weapon, input(true)).empty(),
                        "Precision weapon bypassed cooldown during pulse expiry");
                }
                require(state(*weapon).muzzle_remaining == 0.0f &&
                    state(*weapon).empty_remaining == 0.0f,
                    "Held precision cooldown retriggered muzzle or empty feedback");
            }
            const auto frozen = state(*weapon);
            std::ignore = update(*weapon, input(false), 0.0f);
            require(state(*weapon) == frozen, "Zero-delta weapon update changed a live pulse");
        }
        std::cout << "PASS weapon feedback: five stock paid-shot edges, roller exclusion and precision cooldown rejection\n";
    }

    void check_empty_and_recovery(World& world)
    {
        const auto& look = world.presentation.weapon_feedback();
        const auto& tuning = world.tuning.weapon(WeaponType::sniper);
        const float cost = tuning.details.ammo_usage;
        ProbeWeapon weapon(&world.tuning, PlayerTeam::a, 2, 0, Colour::white,
            WeaponType::sniper, Vector2F::ZERO, &world.render, &world.audio, &world.preferences);
        weapon.set_feedback_presentation(&look);
        weapon.prime(0.0f);
        require(update(weapon, input(true)).empty() && state(weapon).empty_remaining > 0.0f &&
            state(weapon).muzzle_remaining == 0.0f,
            "Unaffordable held request did not emit only its empty cue");
        const float empty_duration = state(weapon).empty_remaining;
        float previous = empty_duration;
        const float repeat = tuning.feedback_sounds.empty_repeat_delay;
        for (int tick = 1; static_cast<float>(tick) * dt < repeat - dt; ++tick)
        {
            weapon.prime(0.0f);
            require(update(weapon, input(true)).empty(), "Dry empty fixture emitted a projectile");
            require(approximately_equal(state(weapon).empty_remaining, std::max(0.0f, previous - dt)),
                "Held empty warning bypassed the existing sound repeat limiter");
            previous = state(weapon).empty_remaining;
        }
        require(previous == 0.0f, "Empty pulse never expired between limited warnings");
        bool repeated = false;
        for (int tick = 0; tick < 5 && !repeated; ++tick)
        {
            weapon.prime(0.0f);
            std::ignore = update(weapon, input(true));
            repeated = approximately_equal(state(weapon).empty_remaining, empty_duration);
        }
        require(repeated, "Held empty warning never rearmed after its configured delay");
        weapon.prime(0.0f);
        std::ignore = update(weapon, input(false));
        std::ignore = update(weapon, input(true));
        require(approximately_equal(state(weapon).empty_remaining, empty_duration),
            "Trigger release failed to rearm an empty warning immediately");

        weapon.reset_ammo();
        weapon.prime(cost);
        require(!update(weapon, input(true)).empty() && weapon.ammo() < cost,
            "Recovery fixture did not spend its final full shot");
        std::ignore = update(weapon, input(true));
        require(state(weapon).empty_remaining > 0.0f,
            "Depleted recovery fixture omitted its empty request");
        const float increment = world.tuning.ammo_regen_rate() * dt;
        weapon.start_regeneration(cost - increment * 1.5f);
        std::ignore = update(weapon, input(false));
        require(weapon.ammo() < cost && state(weapon).recovered_remaining == 0.0f,
            "Partial regeneration emitted recovery before a whole shot was affordable");
        require(update(weapon, input(true)).empty() && weapon.ammo() >= cost &&
            state(weapon).recovered_remaining > 0.0f && state(weapon).empty_remaining == 0.0f,
            "Full-shot recovery omitted its cue or retained a same-tick empty warning");
        const float recovered = state(weapon).recovered_remaining;
        std::ignore = update(weapon);
        require(approximately_equal(state(weapon).recovered_remaining, std::max(0.0f, recovered - dt)),
            "Recovery cue advanced twice or repeated without another depletion");

        weapon.reset_ammo();
        weapon.prime(cost);
        std::ignore = update(weapon, input(true));
        const auto paid = state(weapon);
        weapon.reset_ammo(false);
        require(state(weapon) == paid && weapon.ammo() == world.tuning.starting_ammo(),
            "Infinite-paint refill erased the paid shot's visual feedback");
        std::ignore = update(weapon);
        require(state(weapon).recovered_remaining == 0.0f,
            "Forced refill masqueraded as gameplay paint recovery");
        weapon.reset_ammo();
        require(state(weapon) == WeaponFeedbackState{},
            "Respawn-style refill retained old weapon feedback");
        std::cout << "PASS weapon feedback: empty repeat/release edges, whole-shot regeneration and silent forced refill\n";
    }

    void check_independent_motion(World& world)
    {
        ToolMotionPresentation disabled = world.presentation.tool_motion();
        disabled.enabled = false;
        for (const ToolMotionPresentation* motion : { static_cast<const ToolMotionPresentation*>(&disabled),
            static_cast<const ToolMotionPresentation*>(nullptr) })
        {
            auto weapon = world.weapon(WeaponType::sprayer);
            weapon->set_motion_presentation(motion);
            prime(*weapon, world.tuning);
            require(!update(*weapon, input(true)).empty(),
                "Independent feedback fixture did not fire");
            const float first = state(*weapon).muzzle_remaining;
            require(first > dt, "Independent feedback fixture omitted its muzzle pulse");
            require(!update(*weapon, input(true)).empty() &&
                approximately_equal(state(*weapon).muzzle_remaining, first - dt),
                "Disabled or omitted tool motion cleared or restarted independent weapon feedback");
            require(weapon->draw_bounds().contains(weapon->feedback()->bounds(
                weapon->nozzle_pos(), weapon->draw_pos(), weapon->type())),
                "Weapon culling omitted feedback when tool motion was disabled or absent");
        }
        std::cout << "PASS weapon feedback: disabled/omitted tool motion preserves independent pulses and cull bounds\n";
    }

    struct Course
    {
        Scene scene{ nullptr, nullptr };
        Player* player;

        explicit Course(World& world)
        {
            const RectangleF body(0, -world.tuning.player().size.y,
                world.tuning.player().size.x, world.tuning.player().size.y);
            this->player = this->scene.add(std::make_unique<Player>(body, &world.tuning,
                &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
                &world.presentation.respawn_redraw(), &world.render, &world.audio,
                &world.preferences, 2, PlayerTeam::a, WeaponType::sprayer,
                world.weapon(WeaponType::sprayer), WeaponType::none, Colour::white,
                Viewport{}, body.position()));
            const RectangleF floor(-10000, 0, 20000, 100);
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                floor, std::make_unique<RectangleF>(floor), &world.render,
                CollisionObjectType::structure));
            this->scene.end_tick();
        }

        void tick(const PlayerInputData& actions = input())
        {
            this->player->set_player_input(actions);
            this->scene.update(dt);
            std::ignore = this->player->update_weapon_and_get_projectiles(
                dt, this->scene.collision_objects());
            this->scene.resolve();
            this->player->end_contacts(this->scene.contacts());
            this->player->update_prev_rectangle();
            this->scene.end_tick();
        }

        void fire()
        {
            for (int tick = 0; tick < 10 &&
                state(this->player->primary_weapon()).muzzle_remaining == 0.0f; ++tick)
            {
                this->tick(input(true));
            }
            require(state(this->player->primary_weapon()).muzzle_remaining > 0.0f,
                "Player fixture never emitted a paid-shot pulse");
        }
    };

    void check_player_lifecycle(World& world)
    {
        Course course(world);
        course.fire();
        const auto paused = state(course.player->primary_weapon());
        MatchVoices voices;
        course.player->add_voices(voices);
        voices.pause();
        voices.resume();
        require(state(course.player->primary_weapon()) == paused,
            "Suspending and resuming sound changed the frozen weapon cue");
        course.player->place_at(Vector2F(80, -200));
        require(state(course.player->primary_weapon()) == WeaponFeedbackState{},
            "Developer placement retained old weapon feedback");
        course.fire();
        // Level's time-up path calls this same lifecycle hook.
        course.player->reset_tool_motion();
        require(state(course.player->primary_weapon()) == WeaponFeedbackState{},
            "Time-up reset hook retained weapon feedback");
        course.fire();
        course.player->kill();
        course.tick(input(true));
        require(course.player->state() == PlayerState::dead &&
            state(course.player->primary_weapon()) == WeaponFeedbackState{},
            "Fatal player update retained or emitted a weapon pulse");
        course.player->force_respawn();
        require(course.player->state() == PlayerState::alive &&
            state(course.player->primary_weapon()) == WeaponFeedbackState{} &&
            course.player->weapon_ammo() == world.tuning.starting_ammo(),
            "Forced respawn retained previous-life feedback or failed to refill paint");
        course.player->set_infinite_ammo(true);
        course.fire();
        require(course.player->weapon_ammo() == world.tuning.starting_ammo() &&
            state(course.player->primary_weapon()).recovered_remaining == 0.0f,
            "Real Player infinite-paint refill emitted recovery or omitted the paid muzzle");
        course.player->kill();
        course.tick();
        for (int tick = 0; tick < 600 && course.player->state() == PlayerState::dead; ++tick)
        {
            course.tick();
        }
        require(course.player->state() == PlayerState::alive &&
            state(course.player->primary_weapon()) == WeaponFeedbackState{},
            "Natural respawn retained previous-life weapon feedback");
        std::cout << "PASS weapon feedback: real Player placement, time-up hook, death, both respawns and infinite paint\n";
    }

    void compare_projectiles(std::vector<std::unique_ptr<Projectile>>& visible,
        std::vector<std::unique_ptr<Projectile>>& quiet)
    {
        require(visible.size() == quiet.size(), "Weapon feedback changed volley size or shot cadence");
        for (size_t index = 0; index < visible.size(); ++index)
        {
            auto& a = *visible[index];
            auto& b = *quiet[index];
            require(a.player_num() == 2 && a.player_num() == b.player_num() &&
                a.tag() == b.tag() && a.projectile_type() == b.projectile_type(),
                "Weapon feedback changed projectile ownership or kind");
            for (int travel = 0; travel < 6; ++travel)
            {
                require(a.shape()->bounding_box() == b.shape()->bounding_box() &&
                    a.bounds() == b.bounds() && a.for_deletion() == b.for_deletion(),
                    "Weapon feedback changed projectile launch, trajectory or lifetime");
                a.update(dt);
                b.update(dt);
            }
        }
    }

    void check_simulation_equivalence(World& world)
    {
        PlaySettings reduced = world.preferences;
        reduced.reduce_effects = true;
        for (const WeaponType type : tools)
        {
            auto visible = world.weapon(type);
            auto quiet = world.weapon(type, &world.presentation.weapon_feedback(), &reduced);
            size_t shots = 0;
            bool pulse_seen = false;
            for (int tick = 0; tick < 2400; ++tick)
            {
                const float movement = tick % 180 < 90 ? 1.0f : -1.0f;
                const float angle = PI * static_cast<float>((tick / 15) % 8 - 4) / 4.0f;
                const auto actions = input(tick < 1800 || tick >= 2100, movement, angle);
                const Vector2F center(static_cast<float>(tick % 71), static_cast<float>(tick % 37));
                const Vector2F velocity(movement * 300.0f, static_cast<float>(tick % 5) * 40.0f);
                auto first = update(*visible, actions, dt, center, velocity);
                auto second = update(*quiet, actions, dt, center, velocity);
                shots += first.empty() ? 0 : 1;
                compare_projectiles(first, second);
                require(visible->ammo() == quiet->ammo() &&
                    visible->ready_to_shoot() == quiet->ready_to_shoot() &&
                    visible->shooting_this_update() == quiet->shooting_this_update() &&
                    visible->rotation() == quiet->rotation() &&
                    visible->nozzle_pos() == quiet->nozzle_pos() &&
                    visible->motion_state() == quiet->motion_state(),
                    "Weapon feedback changed paint resources, readiness, aim, muzzle or tool motion");
                require(state(*quiet) == state(*visible),
                    "Reduced effects changed shot, dry-fire or refill event eligibility/timing");
                pulse_seen = pulse_seen || state(*visible) != WeaponFeedbackState{};
            }
            require(shots > 0 && pulse_seen,
                "Feedback equivalence route failed to exercise paid shots and visual cues");
        }
        auto omitted = world.weapon(WeaponType::sprayer, nullptr);
        require(omitted->feedback() == nullptr,
            "Optional CPU weapon client unexpectedly acquired visual feedback");
        prime(*omitted, world.tuning);
        require(!update(*omitted, input(true)).empty(),
            "Omitting presentation prevented normal weapon firing");
        std::cout << "PASS weapon feedback: five-tool reduced-effects paint, cadence, ownership, aim, mechanism, projectile and status-event equivalence\n";
    }

    void check_validation(const std::filesystem::path& content)
    {
        std::ifstream stream(content / "presentation.json");
        require(stream.good(), "Cannot read weapon feedback presentation content");
        const std::string source(std::istreambuf_iterator<char>(stream), {});
        wchar_t temporary_name[MAX_PATH]{};
        const auto directory = std::filesystem::temp_directory_path();
        require(GetTempFileNameW(directory.c_str(), L"cww", 0, temporary_name) != 0,
            "Cannot reserve weapon feedback presentation variant");
        const std::filesystem::path path(temporary_name);
        struct RemoveOnExit
        {
            std::filesystem::path path;
            ~RemoveOnExit()
            {
                std::error_code ignored;
                std::filesystem::remove(this->path, ignored);
            }
        } cleanup{ path };

        int fixture = 0;
        const auto reject = [&](auto edit)
        {
            ++fixture;
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse weapon feedback presentation fixture");
            edit(json["weapon_feedback"]);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            std::ofstream variant(path);
            variant.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            variant.close();
            require(variant.good(), "Cannot write weapon feedback presentation fixture");
            bool rejected = false;
            std::string diagnostic = "content was accepted";
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                diagnostic = error.what();
                rejected = diagnostic.find("weapon_feedback") != std::string::npos;
            }
            if (!rejected)
            {
                throw std::runtime_error("Weapon feedback schema fixture " +
                    std::to_string(fixture) + ": " + diagnostic);
            }
        };
        for (const char* field : { "muzzle_duration", "empty_duration", "recovered_duration",
            "keyline_width", "status_size", "status_offset" })
        {
            reject([field](rapidjson::Value& look) { look[field].SetFloat(0.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(-1.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetString("quick"); });
            reject([field](rapidjson::Value& look)
            {
                look[field].SetDouble(std::numeric_limits<double>::max());
            });
        }
        for (const char* tool : { "sprayer", "sniper", "roller", "mister", "bucket" })
        {
            reject([tool](rapidjson::Value& look) { look["muzzle_sizes"][tool]["x"].SetFloat(0.0f); });
            reject([tool](rapidjson::Value& look) { look["muzzle_sizes"][tool]["y"].SetFloat(-1.0f); });
            reject([tool](rapidjson::Value& look) { look["muzzle_sizes"][tool].SetString("large"); });
        }
        reject([](rapidjson::Value& look) { look["enabled"].SetInt(1); });
        reject([](rapidjson::Value& look) { look["sprite_sheet"].SetInt(1); });
        reject([](rapidjson::Value& look) { look["fill_frame"].SetInt(1); });
        reject([](rapidjson::Value& look) { look["keyline"].SetString("not-a-colour"); });
        reject([](rapidjson::Value& look) { look["keyline_width"].SetFloat(10000.0f); });
        reject([](rapidjson::Value& look) { look["status_size"].SetFloat(1.0f); });
        reject([](rapidjson::Value& look) { look["muzzle_sizes"]["sniper"]["x"].SetFloat(1.0f); });
        reject([](rapidjson::Value& look) { look.RemoveMember("recovered_duration"); });
        std::cout << "PASS weapon feedback content: required fields, numeric types, finite positive dimensions and visible keyline fill\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("weapon feedback", [&]
    {
        require(argc == 2, "Usage: weapon_feedback.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world{ content };
        require(world.presentation.weapon_feedback().enabled,
            "Reviewed weapon feedback must be enabled in staged content");
        check_timers(world);
        check_reduced_muzzle_draw(world);
        check_paid_shots(world);
        check_empty_and_recovery(world);
        check_independent_motion(world);
        check_player_lifecycle(world);
        check_simulation_equivalence(world);
        check_validation(content);
        return 0;
    });
}
