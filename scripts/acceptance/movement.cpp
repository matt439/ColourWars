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
#include "scripts/acceptance/movement/fixtures.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;
using namespace mattmath;
using namespace movement_acceptance;

// The checks are in movement/; main builds the Suite they share and runs them
// in order.
int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: movement.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        RenderResources render;
        for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
        {
            render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(),
                TextureHandle{}));
        }
        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const PlaySettings preferences;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        for (const auto& font : { tuning.match().controls_font })
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
        reject_animation([](rapidjson::Document& json)
        {
            json["player"]["animations"]["fire"]["paint_strip"].SetString("run_paint", json.GetAllocator());
        }, "player.animations.fire.paint_strip");
        std::cout << "PASS figure animation content: aligned body/paint strips and rejection of mismatched strips or zero frame time\n";
        // Built once, up front: the action and feedback checks both use them,
        // and constructing them has no side effect on any run.
        const ProjectileSpray shot(RectangleF(0, 0, 10, 10), Vector2F::ZERO,
            PlayerTeam::b, 1, Colour::white, &render, &tuning.projectile(ProjectileType::spray));
        const int fatal_hits = static_cast<int>(std::ceil(tuning.player().starting_health /
            tuning.projectile(ProjectileType::spray).details.player_damage)) + 1;
        const Suite suite{ content, render, audio, preferences, tuning, presentation, feedback,
            shot, fatal_hits };
        const auto course = [&](float bottom = 0.0f,
            const Vector2F& velocity = Vector2F::ZERO)
        {
            return suite.course(bottom, velocity);
        };

        check_jump_input(suite);
        check_action_events(suite);
        check_ceiling_and_windows(suite);
        check_ramp_support(suite);
        check_movement_feedback(suite);

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
