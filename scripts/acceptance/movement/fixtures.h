#pragma once

// What the movement checks share: the fixed step, the input helper, the
// Course that steps one real Player, the ContentVariant that writes edited
// content to a reserved temporary file, the Suite main builds once, and the
// checks main calls in order.
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/core/step_timer.h"
#include "engine/math/triangle.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace movement_acceptance
{
    using namespace labrador;
    using namespace mattmath;

    using acceptance::require;

    inline const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    inline PlayerInputData input(bool pressed = false, bool held = false,
        float movement = 0.0f, bool drop = false)
    {
        PlayerInputData result;
        result.connected = true;
        result.jump_pressed = pressed;
        result.jump_held = held;
        result.x_movement = movement;
        result.drop_through = drop;
        return result;
    }

    // Uses the same update/resolve/end_contacts ordering as Level, with actual
    // Player, Structure and engine collision objects. No renderer/audio device.
    struct Course
    {
        struct FeedbackEvent
        {
            MovementFeedback::Effect effect;
            int tick;
        };

        Scene scene{ nullptr, nullptr };
        Player* player;
        RenderResources* render;
        std::vector<FeedbackEvent> feedback_events;
        int tick_count = 0;
        size_t emitted_projectiles = 0;

        Course(const GameTuning& tuning, const MovementFeedbackPresentation& feedback,
            const CombatFeedbackPresentation& combat,
            const RespawnRedrawPresentation& redraw, RenderResources& resources,
            const AudioResources& audio, const PlaySettings& preferences,
            float bottom = 0.0f, const Vector2F& velocity = Vector2F::ZERO,
            const Vector2F& spawn = { 0.0f, -120.0f },
            WeaponType tool = WeaponType::sprayer) : render(&resources)
        {
            const RectangleF rectangle(0.0f, bottom - tuning.player().size.y,
                tuning.player().size.x, tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&tuning, PlayerTeam::a,
                0, 0, Colour::white, tool, rectangle.center(),
                &resources, &audio, &preferences);
            this->player = this->scene.add(std::make_unique<Player>(rectangle,
                &tuning, &feedback, &combat, &redraw, &resources, &audio, &preferences, 0, PlayerTeam::a,
                tool, std::move(weapon), WeaponType::none,
                Colour::white, Viewport{}, spawn, velocity));
            this->scene.end_tick();
        }

        void surface(const RectangleF& rectangle,
            CollisionObjectType type = CollisionObjectType::structure)
        {
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1",
                "square_white_4", rectangle, std::make_unique<RectangleF>(rectangle),
                this->render, type));
            this->scene.end_tick();
        }

        void tick(const PlayerInputData& actions = input())
        {
            ++this->tick_count;
            this->player->set_player_input(actions);
            this->scene.update(dt);
            // Exercise the real paid-shot path between movement and contacts,
            // as Level does. The launch results are counted, then discarded so
            // this movement fixture has no unrelated projectile collisions.
            this->emitted_projectiles += this->player->update_weapon_and_get_projectiles(
                dt, this->scene.collision_objects()).size();
            this->scene.resolve();
            this->player->end_contacts(this->scene.contacts());
            this->player->update_prev_rectangle();
            this->scene.end_tick();
            // Advancing and emitting happen in simulation, so a fresh effect is
            // age zero at the end of exactly its event tick. Keep history even
            // after the bounded live list expires an older effect.
            for (const auto& effect : this->player->movement_feedback().effects())
            {
                if (effect.age == 0.0f)
                {
                    this->feedback_events.push_back({ effect, this->tick_count });
                }
            }
        }

        size_t feedback_count(MovementFeedbackKind kind) const
        {
            return static_cast<size_t>(std::count_if(this->feedback_events.begin(),
                this->feedback_events.end(), [kind](const FeedbackEvent& event)
                { return event.effect.kind == kind; }));
        }

        void ramp(const TriangleRightAxisAligned& shape, CollisionObjectType type)
        {
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1",
                "square_white_4", shape.bounding_box(),
                std::make_unique<TriangleRightAxisAligned>(shape), this->render, type));
            this->scene.end_tick();
        }

        void floor(CollisionObjectType type = CollisionObjectType::structure)
        {
            this->surface({ -10000, 0, 20000, 100 }, type);
        }

        void leave_ledge()
        {
            this->surface({ -1000, 0, 1060, 100 });
            this->tick(input(false, false, 1));
            require(this->player->move_state() == PlayerMoveState::on_ground,
                "ledge fixture did not start supported");
            for (int tick = 0; tick < 20; ++tick)
            {
                this->tick(input(false, false, 1));
                if (this->player->move_state() == PlayerMoveState::in_air) { return; }
            }
            throw std::runtime_error("ledge fixture never left the ledge");
        }
    };

    // Variant content is isolated in a reserved temporary file and removed even
    // on failure; authored and staged content files are never overwritten.
    class ContentVariant
    {
    public:
        explicit ContentVariant(const std::filesystem::path& original)
        {
            std::ifstream stream(original);
            require(stream.good(), "Cannot read movement acceptance content");
            this->source_ = std::string(std::istreambuf_iterator<char>(stream), {});
            wchar_t name[MAX_PATH]{};
            const std::filesystem::path temporary = std::filesystem::temp_directory_path();
            require(GetTempFileNameW(temporary.c_str(), L"cwm", 0, name) != 0,
                "Cannot reserve movement acceptance content file");
            this->path_ = name;
        }

        ~ContentVariant()
        {
            std::error_code ignored;
            std::filesystem::remove(this->path_, ignored);
        }

        GameTuning tuning(const char* buffer, const char* grace) const
        {
            std::string source = this->source_;
            for (const auto& item : { std::pair{ "jump_buffer_time", buffer },
                std::pair{ "ledge_grace_time", grace } })
            {
                const size_t key = source.find(std::string("\"") + item.first + "\"");
                require(key != std::string::npos, "Movement tuning key is missing");
                const size_t begin = source.find(':', key) + 1;
                const size_t end = source.find(',', begin);
                require(end != std::string::npos, "Movement tuning value is malformed");
                source.replace(begin, end - begin, item.second);
            }
            this->write(source);
            return GameTuning(this->path_.string().c_str());
        }

        template<typename Edit>
        GameTuning tuning(Edit edit) const
        {
            rapidjson::Document json;
            json.Parse(this->source_.c_str());
            require(!json.HasParseError(), "Cannot parse movement acceptance tuning");
            edit(json);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            this->write(std::string(buffer.GetString(), buffer.GetSize()));
            return GameTuning(this->path_.string().c_str());
        }

        Presentation presentation(const char* key, const char* value) const
        {
            std::string source = this->source_;
            const size_t section = source.find("\"movement_feedback\"");
            require(section != std::string::npos, "Movement presentation is missing");
            const size_t field = source.find(std::string("\"") + key + "\"", section);
            require(field != std::string::npos, "Movement presentation field is missing");
            const size_t begin = source.find(':', field) + 1;
            const size_t end = source.find_first_of(",}", begin);
            require(end != std::string::npos, "Movement presentation value is malformed");
            source.replace(begin, end - begin, value);
            this->write(source);
            return Presentation(this->path_.string().c_str());
        }

    private:
        void write(const std::string& source) const
        {
            std::ofstream stream(this->path_);
            stream << source;
            stream.close();
            require(stream.good(), "Cannot write movement acceptance content variant");
        }

        std::string source_;
        std::filesystem::path path_;
    };

    // What main builds once and every check reads. Each check, and main, keeps
    // a local course() that forwards to course() here.
    struct Suite
    {
        const std::filesystem::path& content;
        RenderResources& render;
        AudioResources& audio;
        const PlaySettings& preferences;
        const GameTuning& tuning;
        const Presentation& presentation;
        const MovementFeedbackPresentation& feedback;
        const ProjectileSpray& shot;
        const int fatal_hits;

        std::unique_ptr<Course> course(float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO) const
        {
            return std::make_unique<Course>(tuning, feedback, presentation.combat_feedback(), presentation.respawn_redraw(),
                render, audio, preferences,
                bottom, velocity);
        }
    };

    // In the order main runs them.
    void check_jump_input(const Suite& suite);
    void check_action_events(const Suite& suite);
    void check_ceiling_and_windows(const Suite& suite);
    void check_ramp_support(const Suite& suite);
    void check_ramp_roof_clearance(const Suite& suite);
    void check_legacy_routes(const Suite& suite);
    void check_movement_feedback(const Suite& suite);
}
