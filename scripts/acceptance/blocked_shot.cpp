#include "game/pch.h"
#include "game/input_slots.h"
#include "game/paint_owners.h"
#include "game/objects/blocked_shot_feedback.h"
#include "game/objects/level_builder.h"
#include "game/objects/paint_audio.h"
#include "game/objects/projectile_builder.h"
#include "game/objects/structure.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/weapon_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/math/triangle.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <tuple>
#include <type_traits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    bool close_to(float first, float second)
    {
        return std::abs(first - second) < 0.001f;
    }

    bool close_to(const Vector2F& first, const Vector2F& second)
    {
        return close_to(first.x, second.x) && close_to(first.y, second.y);
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    // Metadata only: no graphics/audio device, player save or window is opened.
    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        Registry<LevelLoadedInfo> levels{ "Level" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        labrador::Mouse mouse;
        Partitioner partitioner;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content) :
            tuning((content / "tuning.json").string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* font : { "bowlby_one_144", "lato_black_24",
                "lato_black_36", "lato_black_48", "lato_black_72",
                "lato_black_144", "courier_prime_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(font) + ".spritefont")).string());
                auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
                this->render.add_font(font, std::move(loaded));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
            auto info = std::make_unique<LevelLoadedInfo>(
                (content / "levels/movement_course.json").string().c_str());
            info->validate();
            this->levels.add("movement_course", std::move(info));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner, &this->mouse);
        }

        auto shots(ProjectileType type = ProjectileType::jet,
            Vector2F position = { -40.0f, 50.0f }, Vector2F velocity = { 2000.0f, 0.0f },
            int owner = 3)
        {
            return ProjectileBuilder(&this->tuning, &this->render).build_projectiles(position,
                velocity, PlayerTeam::a, owner, Colour::blue, type, &this->render);
        }

        std::unique_ptr<Structure> cover(const RectangleF& rectangle)
        {
            return std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<RectangleF>(rectangle), &this->render, CollisionObjectType::structure);
        }

        std::unique_ptr<StructurePaintable> surface(const PaintableFaces& faces)
        {
            const RectangleF rectangle(0.0f, 0.0f, 100.0f, 100.0f);
            return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4", rectangle,
                std::make_unique<RectangleF>(rectangle), &this->render, &this->audio,
                &this->preferences, &this->tuning.paint(), CollisionObjectType::structure_paintable,
                TeamColour{}, faces);
        }

        std::unique_ptr<Player> player(PlayerTeam team = PlayerTeam::a, int slot = 3)
        {
            const RectangleF rectangle(Vector2F::ZERO, this->tuning.player().size);
            auto weapon = WeaponBuilder::build_weapon(&this->tuning, team, slot, 0,
                Colour::blue, WeaponType::sniper, rectangle.center(), &this->render,
                &this->audio, &this->preferences);
            return std::make_unique<Player>(rectangle, &this->tuning,
                &this->presentation.movement_feedback(), &this->presentation.combat_feedback(),
                &this->presentation.respawn_redraw(), &this->render, &this->audio,
                &this->preferences, slot, team, WeaponType::sniper, std::move(weapon),
                WeaponType::none, Colour::blue, Viewport{}, Vector2F::ZERO);
        }

        std::unique_ptr<Level> build(float duration = 300.0f)
        {
            MatchSetup settings;
            settings.set_stage("movement_course");
            settings.set_player_count(1);
            settings.set_screen_layout(ScreenLayout::one_player);
            settings.set_duration(duration);
            settings.begin_input_claims(3);
            settings.set_player_team(0, PlayerTeam::a);
            settings.set_player_weapon(0, WeaponType::sniper);
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }
    };

    auto observation(const BlockedShotFeedback& feedback)
    {
        using Record = std::tuple<Vector2F, Vector2F, Colour, int, float>;
        std::vector<Record> result;
        for (const auto& cue : feedback.cues())
        {
            result.emplace_back(cue.contact.position, cue.contact.normal,
                cue.contact.team_colour, cue.contact.player_num, cue.age);
        }
        return result;
    }

    void check_faces_and_sweeps(World& world)
    {
        struct Face { Vector2F start; Vector2F finish; Vector2F point; Vector2F normal; };
        const std::array faces{
            Face{ { -40, 50 }, { 140, 50 }, { 0, 50 }, { -1, 0 } },
            Face{ { 140, 50 }, { -40, 50 }, { 100, 50 }, { 1, 0 } },
            Face{ { 50, -40 }, { 50, 140 }, { 50, 0 }, { 0, -1 } },
            Face{ { 50, 140 }, { 50, -40 }, { 50, 100 }, { 0, 1 } }
        };
        for (const Face& face : faces)
        {
            for (const bool launch : { false, true })
            {
                auto cover = world.cover({ 0, 0, 100, 100 });
                CollisionObject* targets[]{ cover.get() };
                auto shots = world.shots(ProjectileType::jet,
                    launch ? face.finish : face.start, (face.finish - face.start) * 10.0f);
                Projectile& shot = *shots[0];
                if (launch) { shot.resolve_launch_obstruction(face.start, targets); }
                else { shot.update(0.1f); shot.resolve_movement_contacts(targets); }
                const auto contact = shot.take_blocked_contact();
                require(shot.for_deletion() && contact.has_value(),
                    "Launch or flying shot missed bare cover feedback");
                require(close_to(contact->position, face.point) && close_to(contact->normal, face.normal),
                    "Blocked feedback was not anchored to the entered face with an outward normal");
                require(contact->player_num == 3 && contact->team_colour == Colour::blue &&
                    !shot.take_blocked_contact(), "Blocked contact lost its owner/tint or was consumed twice");
            }
        }

        for (const bool reverse_order : { false, true })
        {
            auto near_cover = world.cover({ 0, 0, 0.1f, 100 });
            auto far_cover = world.cover({ 80, 0, 0.1f, 100 });
            CollisionObject* targets[]{ reverse_order ? near_cover.get() : far_cover.get(),
                reverse_order ? far_cover.get() : near_cover.get() };
            auto shots = world.shots();
            shots[0]->update(0.1f);
            shots[0]->resolve_movement_contacts(targets);
            const auto contact = shots[0]->take_blocked_contact();
            require(contact && close_to(contact->position, { 0, 50 }) &&
                close_to(contact->normal, { -1, 0 }),
                "Thin-wall cue tunnelled or selected the farther cover in scene order");
        }
        std::cout << "PASS blocked shot: launch/flight contacts on four faces, exact surface anchors, one-shot ownership and nearest thin cover\n";
    }

    void check_paint_and_exclusions(World& world)
    {
        for (const bool projectile_first : { false, true })
        {
            PaintAudio audio(&world.tuning.paint(), &world.audio);
            audio.update(dt);
            auto surface = world.surface({ true, true, true, true });
            auto shots = world.shots(ProjectileType::jet, { -8, 50 }, Vector2F::ZERO);
            if (projectile_first) { shots[0]->on_contact(*surface, { -1, 0 }, 2.0f); }
            surface->on_contact(*shots[0], { 1, 0 }, 2.0f);
            if (!projectile_first) { shots[0]->on_contact(*surface, { -1, 0 }, 2.0f); }
            // Level flushes sound before taking the visual result. Consuming
            // audio must not erase the independent painted classification.
            shots[0]->flush_paint_audio(audio);
            shots[0]->flush_paint_audio(audio);
            require(surface->paint_total().team_a > 0.0f && shots[0]->for_deletion() &&
                !shots[0]->take_blocked_contact(),
                "Painted contact duplicated its existing splash or depended on callback order");
        }
        auto bare_face = world.surface({});
        CollisionObject* bare_targets[]{ bare_face.get() };
        auto unpainted = world.shots();
        unpainted[0]->update(0.1f);
        unpainted[0]->resolve_movement_contacts(bare_targets);
        require(unpainted[0]->take_blocked_contact().has_value() &&
            bare_face->paint_total().team_a == 0.0f,
            "Paintable object with no touched scoring tile lost its bare-contact cue");

        auto enemy = world.player(PlayerTeam::b, 1);
        enemy->place_at({ 0, 0 });
        CollisionObject* enemy_targets[]{ enemy.get() };
        const float health = enemy->health();
        auto hit = world.shots();
        hit[0]->update(0.1f);
        hit[0]->resolve_movement_contacts(enemy_targets);
        require(hit[0]->for_deletion() && enemy->health() < health &&
            !hit[0]->take_blocked_contact(), "Enemy hit was misclassified as bare cover");

        auto cover = world.cover({ 0, 0, 100, 100 });
        CollisionObject* targets[]{ cover.get() };
        auto roller = world.shots(ProjectileType::rolling, { 50, 50 }, Vector2F::ZERO);
        roller[0]->resolve_launch_obstruction({ -40, 50 }, targets);
        roller[0]->on_contact(*cover, { -1, 0 }, 1.0f);
        roller[0]->update(dt);
        roller[0]->update(1.0f);
        require(!roller[0]->take_blocked_contact(), "Roller created flying-shot obstruction feedback");
        auto expired = world.shots();
        expired[0]->update(dt);
        expired[0]->update(world.tuning.projectile(ProjectileType::jet).details.delete_timer + 1.0f);
        require(expired[0]->for_deletion() && !expired[0]->take_blocked_contact(),
            "Lifetime retirement invented a cover contact");

        for (const auto type : { CollisionObjectType::structure_ramp_left,
            CollisionObjectType::structure_ramp_right })
        {
            Structure ramp("sprite_sheet_1", "square_white_4", { 0, 0, 100, 100 },
                std::make_unique<Triangle>(0.0f, 100.0f, 100.0f, 0.0f, 100.0f, 100.0f),
                &world.render, type);
            CollisionObject* ramp_targets[]{ &ramp };
            for (const bool launch : { false, true })
            {
                auto shot = world.shots(ProjectileType::jet,
                    launch ? Vector2F(140, 50) : Vector2F(-40, 50));
                if (launch) { shot[0]->resolve_launch_obstruction({ -40, 50 }, ramp_targets); }
                else { shot[0]->update(0.1f); shot[0]->resolve_movement_contacts(ramp_targets); }
                require(!shot[0]->for_deletion() && !shot[0]->take_blocked_contact(),
                    "Pass-through ramp became a projectile obstruction");
            }
        }
        std::cout << "PASS blocked shot: painted-splash suppression, non-scoring face, player/roller/expiry/ramp exclusions\n";
    }

    void check_flying_tools(World& world)
    {
        for (const auto type : { ProjectileType::spray, ProjectileType::mist, ProjectileType::ball })
        {
            BlockedShotFeedback feedback(&world.presentation.blocked_shot(), &world.render);
            auto cover = world.cover({ 0, -1000, 5, 2000 });
            CollisionObject* targets[]{ cover.get() };
            auto shots = world.shots(type, { 200, 50 });
            for (const auto& shot : shots)
            {
                shot->resolve_launch_obstruction({ -200, 50 }, targets);
                const auto contact = shot->take_blocked_contact();
                require(shot->for_deletion() && contact && close_to(contact->position.x, 0.0f) &&
                    close_to(contact->normal, { -1, 0 }),
                    "Spray, mist or bucket launch failed to report its entered bare face");
                feedback.emit(*contact);
            }
            require(feedback.cues().size() == 1,
                "One paid flying-tool volley created more than one owner contact cue");
        }
        std::cout << "PASS blocked shot: spray/mist/bucket launch geometry and mist-volley owner aggregation\n";
    }

    void check_feedback_limits(World& world)
    {
        BlockedShotPresentation look = world.presentation.blocked_shot();
        BlockedShotFeedback feedback(&look, &world.render);
        const BlockedShotContact contact{ { 100, -50 }, { -2, 0 }, Colour::blue, 3 };
        feedback.emit(contact);
        require(feedback.cues().size() == 1 && feedback.cues()[0].age == 0.0f &&
            close_to(feedback.cues()[0].contact.normal, { -1, 0 }),
            "Initial valid contact was lost or its normal was not normalized");
        const auto initial = observation(feedback);
        const float repeat = feedback.repeat_remaining(3);
        const RectangleF bounds = feedback.bounds();
        for (const float invalid : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
        {
            feedback.update(invalid);
            require(observation(feedback) == initial && feedback.repeat_remaining(3) == repeat &&
                feedback.bounds() == bounds, "Frozen or nonfinite update changed cue age, bounds or repeat budget");
        }
        for (int repeat_index = 0; repeat_index < 1000; ++repeat_index) { feedback.emit(contact); }
        require(feedback.cues().size() == 1, "Held repeated shots bypassed the per-owner limit");
        feedback.update(look.repeat_delay + 0.001f);
        feedback.emit(contact);
        require(feedback.cues().back().age == 0.0f, "Owner failed to recover after its repeat delay");

        feedback.clear();
        for (int owner = 0; owner < paint_owners::count; ++owner)
        {
            auto owned = contact;
            owned.player_num = owner;
            feedback.emit(owned);
        }
        require(feedback.cues().size() == static_cast<size_t>(paint_owners::count),
            "One owner's volley suppressed a separate device or device-less owner");
        auto invalid_owner = contact;
        invalid_owner.player_num = -1;
        feedback.emit(invalid_owner);
        invalid_owner.player_num = 1000;
        feedback.emit(invalid_owner);
        require(feedback.cues().size() == static_cast<size_t>(paint_owners::count + 1),
            "Invalid owners were unbounded or consumed a real owner's repeat budget");

        feedback.clear();
        look.repeat_delay = 0.05f;
        look.duration = 0.4f;
        std::srand(7543);
        const int next_random = std::rand();
        std::srand(7543);
        for (int volley = 0; volley < 500; ++volley)
        {
            feedback.update(0.051f);
            for (int owner = 0; owner < paint_owners::count; ++owner)
            {
                auto owned = contact;
                owned.player_num = owner;
                owned.position.x = static_cast<float>(volley);
                feedback.emit(owned);
                require(feedback.cues().size() <= BlockedShotFeedback::cue_capacity,
                    "Sustained independent volleys exceeded the fixed cue capacity");
            }
        }
        require(feedback.cues().size() == BlockedShotFeedback::cue_capacity &&
            feedback.cues().back().contact.position.x == 499.0f && std::rand() == next_random,
            "Full cue pool lost recent contacts or consumed gameplay RNG");
        for (const auto& cue : feedback.cues())
        {
            const auto cue_bounds = feedback.cue_bounds(cue);
            require(feedback.bounds().contains(cue_bounds) && cue_bounds.width > 0.0f &&
                cue_bounds.height > 0.0f && std::isfinite(cue_bounds.x) && std::isfinite(cue_bounds.y),
                "Cue or aggregate bounds omitted the finite world-space flourish");
        }
        feedback.update(look.duration);
        require(feedback.cues().empty(), "Cue persisted past its bounded lifetime");

        feedback.clear();
        for (const auto normal : { Vector2F::ZERO,
            Vector2F(std::numeric_limits<float>::infinity(), 0.0f),
            Vector2F(0.0f, std::numeric_limits<float>::quiet_NaN()) })
        {
            auto malformed = contact;
            malformed.normal = normal;
            feedback.emit(malformed);
        }
        auto malformed = contact;
        malformed.position.x = std::numeric_limits<float>::infinity();
        feedback.emit(malformed);
        malformed = contact;
        malformed.team_colour.r = std::numeric_limits<float>::quiet_NaN();
        feedback.emit(malformed);
        require(feedback.cues().empty(), "Malformed geometry poisoned the world-space cue pool");
        feedback.emit(contact);
        const auto before_disable = observation(feedback);
        look.enabled = false;
        feedback.emit(contact);
        DrawList unused;
        feedback.draw(unused);
        feedback.update(0.0f);
        require(observation(feedback) == before_disable, "Disabled frozen update changed existing state");
        feedback.update(dt);
        require(feedback.cues().empty() && feedback.repeat_remaining(3) == 0.0f,
            "Live presentation disable retained hidden cues or owner budgets");
        look.enabled = true;
        feedback.emit(contact);
        require(feedback.cues().size() == 1, "Re-enabled feedback retained a stale repeat budget");
        std::cout << "PASS blocked shot: pause/nonfinite freeze, owner/repeat limits, sustained bounded pool, expiry, bounds and isolated RNG\n";
    }

    PlayerInputData aimed(bool held, float angle)
    {
        PlayerInputData input;
        input.connected = true;
        input.primary_shoot = held;
        input.shoot_direction_requested = true;
        input.shoot_angle = angle;
        input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return input;
    }

    void check_player_launch(World& world)
    {
        BlockedShotFeedback feedback(&world.presentation.blocked_shot(), &world.render);
        auto player = world.player();
        player->set_blocked_shot_feedback(&feedback);
        auto enclosure = world.cover({ -1000, -1000, 2000, 2000 });
        CollisionObject* targets[]{ enclosure.get() };
        player->set_player_input(aimed(true, 0.0f));
        const auto& sniper = world.tuning.weapon(WeaponType::sniper).details;
        player->update_weapon_and_get_projectiles(sniper.shoot_interval + 0.001f, targets);
        const float before = player->weapon_ammo();
        const auto launched = player->update_weapon_and_get_projectiles(dt, targets);
        require(launched.empty() && player->weapon_ammo() < before && feedback.cues().size() == 1 &&
            feedback.cues()[0].contact.player_num == 3,
            "Player erased an obstructed paid launch before flushing its owner cue");
        std::cout << "PASS blocked shot: actual Player paid-launch cue survives pre-admission projectile erase\n";
    }

    void activate(Level& level, const std::vector<PlayerInputData>& inputs)
    {
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { level.update(inputs, dt); }
        require(level.state() == LevelState::active, "Blocked-shot fixture did not reach active play");
    }

    void check_level_lifecycle(World& world)
    {
        static_assert(std::is_same_v<decltype(&Level::blocked_shot_feedback),
            const BlockedShotFeedback& (Level::*)() const>);
        std::vector<PlayerInputData> inputs(input_slots::count, aimed(false, PI));
        auto level = world.build(2.0f);
        require(level->blocked_shot_feedback().cues().empty(), "Countdown began with a blocked-shot cue");
        activate(*level, inputs);
        require(level->place_player(0, { 180, 875 }), "Cannot place bare-wall Level fixture");
        inputs[3] = aimed(true, PI);
        for (int tick = 0; tick < 90 && level->blocked_shot_feedback().cues().empty(); ++tick)
        {
            level->update(inputs, dt);
        }
        require(!level->blocked_shot_feedback().cues().empty(),
            "Level did not flush a flying bare-wall contact before Scene retirement");
        require(level->blocked_shot_feedback().cues().back().contact.player_num == 3,
            "Sparse input slot was replaced by the Level view index");
        const auto frozen = observation(level->blocked_shot_feedback());
        const float budget = level->blocked_shot_feedback().repeat_remaining(3);
        level->suspend();
        for (int repeat = 0; repeat < 10; ++repeat)
        {
            level->refresh_layout();
            level->update(inputs, 0.0f);
            level->update(inputs, -1.0f);
        }
        require(observation(level->blocked_shot_feedback()) == frozen &&
            level->blocked_shot_feedback().repeat_remaining(3) == budget,
            "Pause or frozen layout refresh advanced blocked-shot state");
        level->resume();
        for (int tick = 0; tick < 180 && level->state() == LevelState::active; ++tick) { level->update(inputs, dt); }
        require(level->state() != LevelState::active && level->blocked_shot_feedback().cues().empty(),
            "Time-up retained active-play obstruction cues");
        level.reset();
        require(world.build()->blocked_shot_feedback().cues().empty(),
            "New match inherited another match's blocked-shot state");
        std::cout << "PASS blocked shot: Level flight flush, sparse owner, pause/layout freeze, time-up and fresh-match cleanup\n";
    }

    auto physics_trace(World& world, bool enabled)
    {
        auto& look = const_cast<BlockedShotPresentation&>(world.presentation.blocked_shot());
        look.enabled = enabled;
        std::srand(8817);
        auto level = world.build();
        std::vector<PlayerInputData> inputs(input_slots::count, aimed(false, PI));
        activate(*level, inputs);
        require(level->place_player(0, { 180, 875 }), "Cannot place equivalent Level fixture");
        inputs[3] = aimed(true, PI);
        using Record = std::tuple<RectangleF, Vector2F, float, float, float, float, int>;
        std::vector<Record> result;
        for (int tick = 0; tick < 180; ++tick)
        {
            level->update(inputs, dt);
            const auto score = level->level_end_info();
            const auto& player = level->player(0);
            result.emplace_back(player.rectangle(), player.velocity(), player.health(),
                player.weapon_ammo(), score.team_a_area, score.team_b_area, level->projectile_count());
        }
        require(enabled || level->blocked_shot_feedback().cues().empty(),
            "Disabled presentation emitted a Level obstruction cue");
        look.enabled = true;
        return result;
    }

    void check_physics_equivalence(World& world)
    {
        const auto enabled = physics_trace(world, true);
        const auto disabled = physics_trace(world, false);
        require(enabled == disabled,
            "Blocked-shot presentation changed movement, health, ammunition, score or projectile retirement");
        std::cout << "PASS blocked shot: enabled/disabled fixed-input physics, health, ammo, score and projectile traces match\n";
    }

    void check_validation(World& world, const std::filesystem::path& content,
        const std::filesystem::path& scratch)
    {
        const auto& defaults = world.presentation.blocked_shot();
        require(defaults.enabled && defaults.duration > 0.0f && defaults.duration <= 0.4f &&
            defaults.repeat_delay >= 0.05f && defaults.repeat_delay <= 1.0f &&
            defaults.width <= 48.0f && defaults.reach <= 48.0f &&
            defaults.keyline_width * 2.0f < std::min(defaults.stroke_width, defaults.bead_size * 0.6f),
            "Shipped blocked-shot defaults exceeded their presentation budget or hid their visible fill");
        world.render.sprite_sheet(defaults.sprite_sheet)->resolve_sprite_frame(defaults.fill_frame);

        std::ifstream input_file(content / "presentation.json");
        require(input_file.good(), "Cannot read blocked-shot presentation");
        const std::string source(std::istreambuf_iterator<char>(input_file), {});
        const auto path = scratch / "blocked-shot-invalid.json";
        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove(this->path, ignored); }
        } cleanup{ path };
        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse blocked-shot presentation fixture");
            edit(json["blocked_shot"]);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            { std::ofstream variant(path); variant << buffer.GetString(); require(variant.good(), "Cannot write blocked-shot variant"); }
            bool rejected = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("blocked_shot") != std::string::npos;
            }
            require(rejected, "Malformed blocked-shot content was accepted or lost its section name");
        };
        for (const char* field : { "duration", "repeat_delay", "width", "reach",
            "bead_size", "stroke_width", "keyline_width" })
        {
            reject([field](rapidjson::Value& look) { look.RemoveMember(field); });
            reject([field](rapidjson::Value& look) { look[field].SetString("small"); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(0.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(-1.0f); });
            reject([field](rapidjson::Value& look) { look[field].SetFloat(10000.0f); });
            // This is valid JSON and a finite double, but overflows the float
            // presentation value. It must be rejected before it reaches draw.
            reject([field](rapidjson::Value& look) { look[field].SetDouble(std::numeric_limits<double>::max()); });
        }
        reject([](rapidjson::Value& look) { look["enabled"].SetString("true"); });
        reject([](rapidjson::Value& look) { look["highlight"].SetString("invalid_colour"); });
        reject([](rapidjson::Value& look) { look["keyline"].SetString("XYZ123"); });
        reject([](rapidjson::Value& look) { look["duration"].SetFloat(0.401f); });
        reject([](rapidjson::Value& look) { look["repeat_delay"].SetFloat(0.049f); });
        reject([](rapidjson::Value& look) { look["repeat_delay"].SetFloat(1.001f); });
        reject([](rapidjson::Value& look) { look["bead_size"].SetFloat(look["width"].GetFloat() * 0.501f); });
        reject([](rapidjson::Value& look) { look["stroke_width"].SetFloat(look["width"].GetFloat() * 0.251f); });
        reject([](rapidjson::Value& look) { look["keyline_width"].SetFloat(look["stroke_width"].GetFloat() * 0.5f); });
        reject([](rapidjson::Value& look) { look["keyline_width"].SetFloat(look["bead_size"].GetFloat() * 0.3f); });
        std::cout << "PASS blocked shot content: shipped assets/defaults, required positive finite bounded values and visible keyline/fill relationships\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("blocked shot", [&]
    {
        require(argc == 2, "Usage: blocked_shot.exe <game/content directory>");
        World world{ std::filesystem::path(argv[1]) };
        check_faces_and_sweeps(world);
        check_paint_and_exclusions(world);
        check_flying_tools(world);
        check_feedback_limits(world);
        check_player_launch(world);
        check_level_lifecycle(world);
        check_physics_equivalence(world);
        check_validation(world, std::filesystem::path(argv[1]), std::filesystem::absolute(argv[0]).parent_path());
        return 0;
    });
}
