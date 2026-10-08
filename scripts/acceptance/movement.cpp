#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/weapon_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

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
        Scene scene{ nullptr, nullptr };
        Player* player;
        RenderResources* render;

        Course(const GameTuning& tuning, RenderResources& resources,
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
                &tuning, &resources, &audio, &preferences, 0, PlayerTeam::a,
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
            this->player->set_player_input(actions);
            this->scene.update(dt);
            this->scene.resolve();
            this->player->end_contacts();
            this->player->update_prev_rectangle();
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
    // on failure; authored and staged tuning files are never overwritten.
    class TuningVariant
    {
    public:
        explicit TuningVariant(const std::filesystem::path& original)
        {
            std::ifstream stream(original);
            require(stream.good(), "Cannot read movement tuning");
            this->source_ = std::string(std::istreambuf_iterator<char>(stream), {});
            wchar_t name[MAX_PATH]{};
            const std::filesystem::path temporary = std::filesystem::temp_directory_path();
            require(GetTempFileNameW(temporary.c_str(), L"cwm", 0, name) != 0,
                "Cannot reserve movement tuning file");
            this->path_ = name;
        }

        ~TuningVariant()
        {
            std::error_code ignored;
            std::filesystem::remove(this->path_, ignored);
        }

        GameTuning load(const char* buffer, const char* grace) const
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
            std::ofstream stream(this->path_);
            stream << source;
            stream.close();
            require(stream.good(), "Cannot write movement tuning variant");
            return GameTuning(this->path_.string().c_str());
        }

    private:
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
        for (const char* name : { "sprite_sheet_1", "weapon_paint" })
        {
            render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(),
                TextureHandle{}));
        }
        AudioResources audio;
        audio.add_sound_bank("sound_bank_1", SoundBank::silent());
        const PlaySettings preferences;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const auto course = [&](float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO)
        {
            return std::make_unique<Course>(tuning, render, audio, preferences,
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
            PlayerTeam::b, 1, Colour::white, &render, &tuning.projectile(SPRAY));
        const int fatal_hits = static_cast<int>(std::ceil(tuning.player().starting_health /
            tuning.projectile(SPRAY).details.player_damage)) + 1;
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
        for (int tick = 0; tick < 300 && killed->player->state() == PlayerState::dead; ++tick)
        {
            killed->tick(input(true, true));
        }
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

        TuningVariant variant(content / "tuning.json");
        const GameTuning disabled = variant.load("0.0", "0.0");
        Course unbuffered(disabled, render, audio, preferences, -4, { 0, 100 });
        unbuffered.floor();
        unbuffered.tick(input(true, true));
        unbuffered.tick(input(false, true));
        require(unbuffered.player->velocity().y == 0, "zero did not disable jump buffering");
        unbuffered.tick();
        unbuffered.tick(input(true, true));
        require(unbuffered.player->velocity().y < 0,
            "zero buffer disabled ordinary grounded jumps");
        Course ungraced(disabled, render, audio, preferences, 0,
            { tuning.player().max_velocity.x, 0 });
        ungraced.leave_ledge();
        ungraced.tick(input(true, true, 1));
        require(ungraced.player->velocity().y > 0, "zero did not disable ledge grace");
        for (const char* invalid : { "-0.01", "1.01", "1e40" })
        {
            for (const bool buffer : { false, true })
            {
                bool rejected = false;
                try { variant.load(buffer ? invalid : "0", buffer ? "0" : invalid); }
                catch (const std::exception&) { rejected = true; }
                require(rejected, "invalid movement forgiveness duration was accepted");
            }
        }
        std::cout << "PASS ceiling, disabled windows and tuning validation\n";

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
