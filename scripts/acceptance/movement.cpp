#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/render/sprite_font_file.h"
#include "engine/core/step_timer.h"
#include "engine/math/triangle.h"
#include "engine/scene/scene.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    PlayerInputData input(bool pressed = false, bool held = false,
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

        Course(const GameTuning& tuning, const MovementFeedbackPresentation& feedback,
            const RespawnRedrawPresentation& redraw, RenderResources& resources,
            const AudioResources& audio, const PlaySettings& preferences,
            float bottom = 0.0f, const Vector2F& velocity = Vector2F::ZERO,
            const Vector2F& spawn = { 0.0f, -120.0f }) : render(&resources)
        {
            const RectangleF rectangle(0.0f, bottom - tuning.player().size.y,
                tuning.player().size.x, tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&tuning, PlayerTeam::a,
                0, 0, Colour::white, WeaponType::sprayer, rectangle.center(),
                &resources, &audio, &preferences);
            this->player = this->scene.add(std::make_unique<Player>(rectangle,
                &tuning, &feedback, &redraw, &resources, &audio, &preferences, 0, PlayerTeam::a,
                WeaponType::sprayer, std::move(weapon), WeaponType::none,
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
            this->scene.resolve();
            this->player->end_contacts();
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
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: movement.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        RenderResources render;
        for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint" })
        {
            render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(),
                TextureHandle{}));
        }
        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const PlaySettings preferences;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        for (const auto& font : { tuning.match().countdown_font, tuning.match().controls_font })
        {
            const SpriteFontFile file = read_sprite_font_file(
                (content / "fonts" / (font + ".spritefont")).string());
            auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
            if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
            render.add_font(font, std::move(loaded));
        }
        const Presentation presentation((content / "presentation.json").string().c_str());
        const MovementFeedbackPresentation& feedback = presentation.movement_feedback();
        tuning.validate_resources(render, audio);
        const ContentVariant animation_content(content / "tuning.json");
        const auto reject_animation = [&](auto edit, const char* authored_key)
        {
            bool rejected = false;
            try
            {
                const GameTuning invalid = animation_content.tuning(edit);
                invalid.validate_resources(render, audio);
            }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find(authored_key) != std::string::npos;
            }
            require(rejected, "Misaligned figure animation was accepted or lost its authored key");
        };
        reject_animation([](rapidjson::Document& json)
        {
            json["player"]["animations"]["idle"]["paint_strip"].SetString("run_paint", json.GetAllocator());
        }, "player.animations.idle.paint_strip");
        reject_animation([](rapidjson::Document& json)
        {
            json["player"]["animations"]["running"]["frame_time"].SetFloat(0.0f);
        }, "frame_time");
        std::cout << "PASS figure animation content: aligned body/paint strips and rejection of mismatched strips or zero frame time\n";
        const auto course = [&](float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO)
        {
            return std::make_unique<Course>(tuning, feedback, presentation.respawn_redraw(),
                render, audio, preferences,
                bottom, velocity);
        };

        for (const auto type : { CollisionObjectType::structure,
            CollisionObjectType::structure_jump_through })
        {
            auto buffered = course(-4, { 0, 100 });
            buffered->floor(type);
            buffered->tick(input(true, false)); // A tap released before landing.
            require(buffered->player->velocity().y > 0,
                "airborne press launched without support");
            buffered->tick();
            require(buffered->player->move_state() == PlayerMoveState::jumping &&
                buffered->player->velocity().y == tuning.player().jump_launch_velocity,
                "landing did not consume buffered tap on its contact tick");

            auto expired = course(-400);
            expired->floor(type);
            expired->tick(input(true, true));
            for (int tick = 0; tick < 120; ++tick)
            {
                expired->tick(input(false, true));
            }
            require(expired->player->velocity().y == 0 &&
                std::fabs(expired->player->rectangle().bottom()) < 0.01f,
                "expired buffer or held button repeated a jump on landing");
        }
        std::cout << "PASS landing buffer: solid/platform, released taps, expiry, held input\n";

        auto grace = course(0, { tuning.player().max_velocity.x, 0 });
        grace->leave_ledge();
        grace->tick(input(true, true, 1));
        require(grace->player->move_state() == PlayerMoveState::jumping,
            "fresh ledge grace jump failed");
        require(grace->feedback_count(MovementFeedbackKind::takeoff) == 1,
            "successful grace jump did not produce exactly one takeoff cue");
        grace->tick(input(false, false, 1));
        grace->tick(input(true, false, 1));
        require(grace->player->velocity().y > tuning.player().jump_launch_velocity,
            "one takeoff left a second grace jump available");

        auto late = course(0, { tuning.player().max_velocity.x, 0 });
        late->leave_ledge();
        const int grace_ticks = static_cast<int>(std::ceil(tuning.player().ledge_grace_time / dt)) + 1;
        for (int tick = 0; tick < grace_ticks; ++tick) { late->tick(input(false, false, 1)); }
        late->tick(input(true, true, 1));
        require(late->player->velocity().y > 0, "expired ledge grace allowed a jump");
        require(late->feedback_count(MovementFeedbackKind::takeoff) == 0,
            "failed grace jump produced a takeoff cue");
        std::cout << "PASS ledge grace: first jump, expiry, no midair second jump\n";

        auto paused = course(-4, { 0, 100 });
        paused->floor();
        paused->tick(input(true, true));
        paused->player->reset_jump_input();
        paused->tick(input(true, true));
        require(paused->player->velocity().y == 0,
            "pause left a queued/menu-held landing jump");
        paused->tick(input(false, true));
        require(paused->player->velocity().y == 0, "pause held jump was rearmed");
        paused->tick();
        paused->tick(input(true, true));
        require(paused->player->velocity().y < 0, "release did not rearm jump after pause");

        auto paused_grace = course(0, { tuning.player().max_velocity.x, 0 });
        paused_grace->leave_ledge();
        paused_grace->player->reset_jump_input();
        paused_grace->tick(input(false, false, 1));
        paused_grace->tick(input(true, true, 1));
        require(paused_grace->player->velocity().y > 0,
            "pause retained a ledge grace jump");

        auto uninterrupted = course();
        auto suspended = course();
        uninterrupted->floor();
        suspended->floor();
        uninterrupted->tick();
        suspended->tick();
        bool returned_to_floor = false;
        for (int tick = 0; tick < 150; ++tick)
        {
            if (tick == 10) { suspended->player->reset_jump_input(); }
            uninterrupted->tick(input(tick == 0, true));
            // Model the held jump plus an activation edge from the menu on
            // resume. It must not become a new buffer or cut the current jump.
            suspended->tick(input(tick == 0 || tick == 10, true));
            require(suspended->player->rectangle().position() ==
                    uninterrupted->player->rectangle().position() &&
                suspended->player->velocity() == uninterrupted->player->velocity() &&
                suspended->player->move_state() == uninterrupted->player->move_state(),
                "pause/resume changed an existing held jump trajectory");
            returned_to_floor = returned_to_floor ||
                suspended->player->move_state() == PlayerMoveState::on_ground;
        }
        require(returned_to_floor && suspended->player->velocity().y == 0,
            "pause/resume queued an extra jump while the button remained held");

        auto dropped = course();
        dropped->floor(CollisionObjectType::structure_jump_through);
        dropped->surface({ -10000, 80, 20000, 100 });
        dropped->tick();
        dropped->tick(input(false, false, 0, true));
        require(dropped->player->move_state() == PlayerMoveState::dropping_down,
            "drop did not leave the platform");
        dropped->tick(input(true, true));
        require(dropped->player->velocity().y > 0, "drop supplied a grace jump");
        for (int tick = 0; tick < 60; ++tick) { dropped->tick(input(false, true)); }
        require(dropped->player->velocity().y == 0 &&
            std::fabs(dropped->player->rectangle().bottom() - 80) < 0.01f,
            "drop carried a held/queued jump to the lower floor");
        require(dropped->feedback_count(MovementFeedbackKind::takeoff) == 0 &&
            dropped->feedback_count(MovementFeedbackKind::landing) == 1,
            "platform drop emitted a takeoff or missed/duplicated its lower landing");
        dropped->tick();
        dropped->tick(input(true, true));
        require(dropped->player->velocity().y < 0, "drop failed to rearm after release");

        auto simultaneous = course();
        simultaneous->floor(CollisionObjectType::structure_jump_through);
        simultaneous->tick();
        simultaneous->tick(input(true, true, 0, true));
        require(simultaneous->player->move_state() == PlayerMoveState::jumping,
            "fresh jump lost its precedence over simultaneous drop");

        auto buffered_drop = course(-4, { 0, 100 });
        buffered_drop->floor(CollisionObjectType::structure_jump_through);
        buffered_drop->tick(input(true, false));
        buffered_drop->tick(input(false, false, 0, true));
        require(buffered_drop->player->move_state() == PlayerMoveState::on_drop_down_ground,
            "old airborne buffer overrode a held drop on platform landing");
        buffered_drop->tick(input(false, false, 0, true));
        require(buffered_drop->player->move_state() == PlayerMoveState::dropping_down,
            "held drop did not proceed after buffered platform landing");
        std::cout << "PASS pause/release and platform-drop input isolation\n";

        auto killed = course(-4, { 0, 100 });
        killed->floor();
        killed->tick(input(true, true));
        const ProjectileSpray shot(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
            PlayerTeam::b, 1, Colour::white, &render, &tuning.projectile(ProjectileType::spray));
        const int fatal_hits = static_cast<int>(std::ceil(tuning.player().starting_health /
            tuning.projectile(ProjectileType::spray).details.player_damage)) + 1;
        for (int hit = 0; hit < fatal_hits && killed->player->health() > 0; ++hit)
        {
            killed->player->on_contact(shot, Vector2F::ZERO, 0);
        }
        require(killed->player->health() <= 0, "damage fixture did not eliminate player");
        const Vector2F death_position = killed->player->rectangle().position();
        killed->tick(input(true, true));
        require(killed->player->state() == PlayerState::dead &&
            killed->player->rectangle().position() == death_position,
            "fatal damage allowed one final movement/jump tick");
        // The redraw at the spawn: drawn there, culled with it, and watched.
        const RespawnRedrawPresentation& redraw = presentation.respawn_redraw();
        const RectangleF spawn = killed->player->spawn_rectangle();
        require(spawn.position() == Vector2F(0.0f, -120.0f) &&
            spawn.size() == killed->player->rectangle().size(),
            "the redraw is not at the player's spawn");
        require(killed->player->cull_bounds(1.0f).contains(spawn) &&
            killed->player->cull_bounds(1.0f).contains(killed->player->rectangle()),
            "a dead player's cull bounds miss the spawn where it is redrawn");
        require(killed->player->camera_focus() == killed->player->center(),
            "the camera left the fall before camera_hold");
        float previous_progress = killed->player->respawn_progress();
        bool watched_spawn = false;
        for (int tick = 0; tick < 300 && killed->player->state() == PlayerState::dead; ++tick)
        {
            const float waited = tuning.player().respawn_delay - killed->player->respawn_timer();
            require((killed->player->camera_focus() == spawn.center()) ==
                (waited >= redraw.camera_hold),
                "the camera did not cut to the spawn at camera_hold");
            watched_spawn = watched_spawn || killed->player->camera_focus() == spawn.center();
            killed->tick(input(true, true));
            const float progress = killed->player->respawn_progress();
            require(killed->player->state() == PlayerState::alive ||
                (progress >= previous_progress && progress <= 1.0f),
                "the redraw went backwards or past the end");
            previous_progress = progress;
        }
        require(watched_spawn, "the camera never watched the redraw");
        require(killed->player->state() == PlayerState::alive,
            "eliminated player did not respawn");
        for (int tick = 0; tick < 30; ++tick) { killed->tick(input(false, true)); }
        require(killed->player->velocity().y == 0,
            "dead or respawning player carried a jump into its new life");
        killed->tick();
        killed->tick(input(true, true));
        require(killed->player->velocity().y < 0, "respawn failed to rearm after release");
        std::cout << "PASS lethal contact, death, respawn and held-jump isolation\n";

        auto ceiling = course();
        ceiling->floor();
        ceiling->surface({ -1000, -180, 2000, 20 });
        ceiling->tick();
        ceiling->tick(input(true, true));
        bool bonked = false;
        for (int tick = 0; tick < 30; ++tick)
        {
            ceiling->tick(input(false, true));
            bonked = bonked || ceiling->player->move_state() == PlayerMoveState::on_ceiling;
            require(ceiling->player->rectangle().top() >= -160.01f,
                "jump escaped the ceiling");
        }
        require(bonked && ceiling->player->velocity().y == 0,
            "ceiling contact did not return the player to the floor");
        require(ceiling->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            ceiling->feedback_count(MovementFeedbackKind::landing) == 1,
            "ceiling contact emitted a landing or duplicated the return-to-floor cue");

        ContentVariant variant(content / "tuning.json");
        const GameTuning disabled = variant.tuning("0.0", "0.0");
        Course unbuffered(disabled, feedback, presentation.respawn_redraw(), render, audio,
            preferences, -4, { 0, 100 });
        unbuffered.floor();
        unbuffered.tick(input(true, true));
        unbuffered.tick(input(false, true));
        require(unbuffered.player->velocity().y == 0, "zero did not disable jump buffering");
        unbuffered.tick();
        unbuffered.tick(input(true, true));
        require(unbuffered.player->velocity().y < 0,
            "zero buffer disabled ordinary grounded jumps");
        Course ungraced(disabled, feedback, presentation.respawn_redraw(), render, audio,
            preferences, 0,
            { tuning.player().max_velocity.x, 0 });
        ungraced.leave_ledge();
        ungraced.tick(input(true, true, 1));
        require(ungraced.player->velocity().y > 0, "zero did not disable ledge grace");
        for (const char* invalid : { "-0.01", "1.01", "1e40" })
        {
            for (const bool buffer : { false, true })
            {
                bool rejected = false;
                try { variant.tuning(buffer ? invalid : "0", buffer ? "0" : invalid); }
                catch (const std::exception&) { rejected = true; }
                require(rejected, "invalid movement forgiveness duration was accepted");
            }
        }
        std::cout << "PASS ceiling, disabled windows and tuning validation\n";

        auto pulses = course();
        pulses->floor();
        for (int tick = 0; tick < 30; ++tick) { pulses->tick(); }
        require(pulses->feedback_events.empty(),
            "initial settling or sustained support emitted a movement cue");
        const Vector2F launch_anchor(pulses->player->center().x,
            pulses->player->rectangle().bottom());
        pulses->tick(input(true, false));
        require(pulses->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            pulses->feedback_events.front().effect.position == launch_anchor,
            "grounded takeoff was not emitted once at its launch contact");
        bool pulse_landed = false;
        for (int tick = 0; tick < 150; ++tick)
        {
            pulses->tick();
            if (pulses->player->move_state() == PlayerMoveState::on_ground)
            {
                pulse_landed = true;
                break;
            }
        }
        require(pulse_landed && pulses->feedback_count(MovementFeedbackKind::landing) == 1,
            "small hop did not emit exactly one landing");
        require(std::fabs(pulses->feedback_events.back().effect.position.y) < 0.01f,
            "landing cue was not attached to the resolved floor");
        const float longest_duration = std::max(feedback.takeoff.duration,
            feedback.landing.duration);
        const int expiry_ticks = static_cast<int>(std::ceil(longest_duration / dt)) + 1;
        for (int tick = 0; tick < expiry_ticks; ++tick) { pulses->tick(); }
        require(pulses->player->movement_feedback().effects().empty() &&
            pulses->feedback_events.size() == 2,
            "movement cues leaked past expiry or repeated while standing");

        auto long_fall = course(0, { tuning.player().max_velocity.x, 0 });
        long_fall->leave_ledge();
        long_fall->surface({ -10000, 1000, 20000, 100 });
        for (int tick = 0; tick < 180; ++tick) { long_fall->tick(); }
        require(long_fall->feedback_count(MovementFeedbackKind::takeoff) == 0 &&
            long_fall->feedback_count(MovementFeedbackKind::landing) == 1 &&
            std::fabs(long_fall->feedback_events.back().effect.position.y - 1000) < 0.01f &&
            long_fall->feedback_events.back().effect.strength >
                pulses->feedback_events.back().effect.strength,
            "long non-jump fall did not produce one stronger cue at its actual landing");

        for (const auto type : { CollisionObjectType::structure,
            CollisionObjectType::structure_jump_through })
        {
            auto rebound = course();
            rebound->floor(type);
            rebound->tick();
            rebound->tick(input(true, false));
            bool queued = false;
            for (int tick = 0; tick < 150; ++tick)
            {
                const bool press = !queued && rebound->player->velocity().y > 0 &&
                    rebound->player->rectangle().bottom() > -25.0f;
                rebound->tick(input(press, false));
                queued = queued || press;
                if (rebound->feedback_count(MovementFeedbackKind::takeoff) == 2) { break; }
            }
            require(queued && rebound->feedback_events.size() == 3,
                "buffered rebound missed or duplicated a movement event");
            const auto& landing = rebound->feedback_events[1];
            const auto& takeoff = rebound->feedback_events[2];
            require(landing.effect.kind == MovementFeedbackKind::landing &&
                takeoff.effect.kind == MovementFeedbackKind::takeoff &&
                landing.tick == takeoff.tick &&
                landing.effect.position == takeoff.effect.position,
                "buffered rebound did not emit landing then takeoff at the same contact tick");
        }
        std::cout << "PASS movement feedback: takeoffs, small hops, long falls, buffered ordering, placement, expiry\n";

        auto seam = course();
        seam->surface({ -1000, 0, 1026, 100 });
        seam->surface({ 26, 0, 1000, 100 });
        seam->tick();
        seam->tick(input(true, false));
        for (int tick = 0; tick < 100; ++tick) { seam->tick(); }
        require(seam->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            seam->feedback_count(MovementFeedbackKind::landing) == 1,
            "two simultaneous support contacts duplicated a landing cue");
        for (int tick = 0; tick < 20; ++tick) { seam->tick(input(false, false, 1)); }
        require(seam->feedback_count(MovementFeedbackKind::landing) == 1,
            "walking across a platform seam retriggered landing feedback");

        auto edge = course();
        edge->surface({ -1000, 0, 1020, 100 });
        edge->tick();
        edge->tick(input(true, false));
        require(edge->feedback_events.front().effect.position == Vector2F(10, 0),
            "edge takeoff was placed outside the actual support overlap");
        for (int tick = 0; tick < 100; ++tick) { edge->tick(); }
        require(edge->feedback_count(MovementFeedbackKind::landing) == 1 &&
            edge->feedback_events.back().effect.position == Vector2F(10, 0),
            "edge landing was placed outside the actual support overlap");

        for (const bool uphill_right : { false, true })
        {
            // Match the existing King of the Hill ramps' rise/run while leaving
            // enough width to sustain travel in both directions.
            const float gradient = 450.0f / 1400.0f;
            const float ramp_height = 1000.0f * gradient;
            auto slope = course(uphill_right ? -tuning.player().size.x * gradient : 0.0f);
            const TriangleRightAxisAligned triangle = uphill_right
                ? TriangleRightAxisAligned(1000, -ramp_height, -1000, ramp_height, 1000, ramp_height)
                : TriangleRightAxisAligned(-1000, -ramp_height, -1000, ramp_height, 1000, ramp_height);
            slope->ramp(triangle, uphill_right
                ? CollisionObjectType::structure_ramp_right
                : CollisionObjectType::structure_ramp_left);
            slope->tick();
            require(slope->player->move_state() == (uphill_right
                ? PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left),
                "slope fixture did not start supported");
            slope->tick(input(true, false));
            for (int tick = 0; tick < 120; ++tick) { slope->tick(); }
            require(slope->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
                slope->feedback_count(MovementFeedbackKind::landing) == 1,
                "slope landing was missing or repeated under sustained ramp gravity");
            for (const auto& event : slope->feedback_events)
            {
                const Vector2F position = event.effect.position;
                const float expected_x = uphill_right ? tuning.player().size.x : 0.0f;
                const float expected_y = position.x * (uphill_right ? -gradient : gradient);
                require(std::fabs(position.x - expected_x) < 0.01f &&
                    std::fabs(position.y - expected_y) < 0.01f,
                    "ramp cue was not attached to the actual supporting corner");
            }
            for (int tick = 0; tick < 15; ++tick)
            {
                slope->tick(input(false, false, uphill_right ? 1.0f : -1.0f));
            }
            require(slope->feedback_count(MovementFeedbackKind::landing) == 1,
                "walking uphill repeatedly emitted landing cues");
            for (int tick = 0; tick < 60; ++tick)
            {
                slope->tick(input(false, false, uphill_right ? -1.0f : 1.0f));
            }
            require(slope->feedback_count(MovementFeedbackKind::landing) == 1,
                "running downhill repeatedly emitted landing cues");
        }
        std::cout << "PASS movement feedback: drop, ceiling, seams and both slope directions\n";

        auto frozen = course();
        frozen->floor();
        frozen->tick();
        frozen->tick(input(true, true));
        const auto before_pause = frozen->player->movement_feedback().effects().front();
        frozen->player->reset_jump_input();
        const auto& paused_effect = frozen->player->movement_feedback().effects().front();
        require(paused_effect.age == before_pause.age &&
            paused_effect.position == before_pause.position,
            "pause input reset cleared or advanced a live movement cue");
        frozen->tick(input(true, true));
        require(frozen->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            frozen->player->movement_feedback().effects().front().age > before_pause.age &&
            frozen->player->movement_feedback().effects().front().position == before_pause.position,
            "resume retriggered a takeoff or moved its world-space anchor");

        for (int hit = 0; hit < fatal_hits && frozen->player->health() > 0; ++hit)
        {
            frozen->player->on_contact(shot, Vector2F::ZERO, 0);
        }
        frozen->player->end_contacts();
        require(frozen->player->movement_feedback().effects().empty(),
            "fatal contact retained a live movement cue until the next update");
        frozen->tick(input(true, true));
        for (int tick = 0; tick < 360; ++tick) { frozen->tick(input(false, true)); }
        require(frozen->player->state() == PlayerState::alive &&
            frozen->player->move_state() == PlayerMoveState::on_ground &&
            frozen->feedback_count(MovementFeedbackKind::takeoff) == 1 &&
            frozen->feedback_count(MovementFeedbackKind::landing) == 0 &&
            frozen->player->movement_feedback().effects().empty(),
            "death/respawn settling emitted or retained a movement cue");
        frozen->tick();
        frozen->tick(input(true, false));
        for (int tick = 0; tick < 100; ++tick) { frozen->tick(); }
        require(frozen->feedback_count(MovementFeedbackKind::takeoff) == 2 &&
            frozen->feedback_count(MovementFeedbackKind::landing) == 1,
            "respawn failed to rearm ordinary movement feedback");
        std::cout << "PASS movement feedback: pause freeze/resume, lethal contact cleanup, respawn suppression and rearming\n";

        ContentVariant presentation_variant(content / "presentation.json");
        for (const auto& invalid : {
            std::pair{ "duration", "0" },
            std::pair{ "pixel_size", "-1" },
            std::pair{ "spread", "1e40" },
            std::pair{ "landing_min_speed", "1001" },
            std::pair{ "minimum_strength", "1.01" },
            std::pair{ "particle_size", "1" } })
        {
            bool rejected = false;
            try { presentation_variant.presentation(invalid.first, invalid.second); }
            catch (const std::exception&) { rejected = true; }
            require(rejected, "invalid movement presentation was accepted");
        }
        const Presentation without_feedback = presentation_variant.presentation("enabled", "false");
        Course decorated(tuning, feedback, presentation.respawn_redraw(), render, audio,
            preferences);
        Course undecorated(tuning, without_feedback.movement_feedback(),
            without_feedback.respawn_redraw(), render, audio, preferences);
        decorated.floor();
        undecorated.floor();
        for (int tick = 0; tick < 240; ++tick)
        {
            if (tick == 15)
            {
                decorated.player->reset_jump_input();
                undecorated.player->reset_jump_input();
            }
            const PlayerInputData actions = input(tick == 1 || tick == 120,
                tick >= 1 && tick < 45, tick < 80 ? 1.0f : -1.0f);
            decorated.tick(actions);
            undecorated.tick(actions);
            require(decorated.player->rectangle() == undecorated.player->rectangle() &&
                decorated.player->velocity() == undecorated.player->velocity() &&
                decorated.player->move_state() == undecorated.player->move_state() &&
                decorated.player->health() == undecorated.player->health() &&
                decorated.player->weapon_ammo() == undecorated.player->weapon_ammo(),
                "cosmetic movement feedback changed simulation or resources");
        }
        require(decorated.feedback_count(MovementFeedbackKind::takeoff) == 2 &&
            decorated.feedback_count(MovementFeedbackKind::landing) == 2 &&
            undecorated.feedback_events.empty(),
            "disabled feedback still emitted cues or equivalence route omitted jumps");

        MovementFeedback bounded(&feedback, &render);
        for (int burst = 0; burst < 40; ++burst)
        {
            bounded.takeoff({ static_cast<float>(burst), 0 });
        }
        require(bounded.effects().size() == 8 &&
            bounded.effects().front().position.x == 32 &&
            bounded.effects().back().position.x == 39,
            "repeated movement cues exceeded capacity or discarded the newest cue");
        bounded.clear();
        require(bounded.effects().empty(), "movement cue cleanup retained live effects");
        bounded.land({ 1000, 1000 }, feedback.landing_min_speed / 2.0f);
        require(bounded.effects().empty(), "micro-detach impact emitted a landing cue");
        bounded.takeoff({ 1000, 1000 });
        bounded.land({ -1000, -1000 }, feedback.full_impact_speed);
        const RectangleF actor(-26, -120, 52, 120);
        const RectangleF expanded = bounded.bounds(actor);
        require(expanded.contains(actor) &&
            expanded.contains(RectangleF(999, 999, 2, 2)) &&
            expanded.contains(RectangleF(-1001, -1001, 2, 2)),
            "culling bounds omitted a live effect left behind by its player");
        bounded.update(longest_duration);
        require(bounded.effects().empty() && bounded.bounds(actor) == actor,
            "expired cues remained alive or kept stale culling bounds");
        std::cout << "PASS movement feedback: presentation rejection, on/off simulation equivalence, bounded storage and culling lifetime\n";

        for (const bool held : { false, true })
        {
            auto measured = course(0, { tuning.player().max_velocity.x, 0 });
            measured->floor();
            measured->tick(input(false, false, 1));
            const Vector2F start = measured->player->rectangle().position();
            float rise = 0;
            int ticks = 0;
            do
            {
                measured->tick(input(ticks == 0, held, 1));
                rise = std::max(rise, start.y - measured->player->rectangle().y);
                ++ticks;
            }
            while (measured->player->move_state() != PlayerMoveState::on_ground && ticks < 300);
            require(ticks < 300 && rise > 0, "measured jump never returned to the floor");
            std::cout << "MEASURE " << (held ? "held" : "tap") << ": dt=" << dt
                << ", rise=" << rise << ", airtime=" << ticks * dt
                << ", running_range=" << measured->player->rectangle().x - start.x
                << ", speed=" << measured->player->velocity().x << '\n';
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
