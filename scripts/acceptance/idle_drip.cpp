#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/idle_drip.h"
#include "game/objects/level_builder.h"
#include "game/objects/structure.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/math/triangle.h"
#include "engine/render/sprite_font_file.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <type_traits>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    bool nearly_equal(float first, float second)
    {
        return std::abs(first - second) < 0.0001f;
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    auto observation(const IdleDrip& drip)
    {
        return std::tuple{ drip.attached(), drip.bead_age(), drip.interval_remaining(),
            drip.bead_rectangle(), drip.bounds(),
            std::vector<IdleDrip::Drop>(drip.drops().begin(), drip.drops().end()) };
    }

    void attach(IdleDrip& drip, const Vector2F& nozzle = Vector2F::ZERO,
        float angle = 0.0f)
    {
        drip.advance(true, nozzle, angle, drip.interval_remaining() + 0.001f, {});
        require(drip.attached() && drip.bead_age() == 0.0f,
            "Quiet interval did not begin a fresh attached bead");
    }

    void detach(IdleDrip& drip, const IdleDripPresentation& look,
        const Vector2F& nozzle = Vector2F::ZERO)
    {
        attach(drip, nozzle);
        drip.advance(true, nozzle, 0.0f, look.grow_time + look.stretch_time + 0.001f, {});
        require(!drip.attached() && !drip.drops().empty() && drip.drops().back().age == 0.0f,
            "Grown and stretched bead did not detach into a fresh falling drop");
    }

    constexpr std::array supported_tools{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket };
    constexpr std::array roster{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket, WeaponType::roller };

    class ContentVariant
    {
    public:
        explicit ContentVariant(const std::filesystem::path& original)
        {
            std::ifstream stream(original);
            require(stream.good(), "Cannot read idle-drip acceptance content");
            this->source_ = std::string(std::istreambuf_iterator<char>(stream), {});
            wchar_t name[MAX_PATH]{};
            const auto temporary = std::filesystem::temp_directory_path();
            require(GetTempFileNameW(temporary.c_str(), L"cwd", 0, name) != 0,
                "Cannot reserve idle-drip acceptance content file");
            this->path_ = name;
        }

        ~ContentVariant()
        {
            std::error_code ignored;
            std::filesystem::remove(this->path_, ignored);
        }

        template<typename Edit>
        const std::filesystem::path& write(Edit edit) const
        {
            rapidjson::Document json;
            json.Parse(this->source_.c_str());
            require(!json.HasParseError(), "Cannot parse idle-drip acceptance content");
            edit(json);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            std::ofstream stream(this->path_);
            stream.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            require(stream.good(), "Cannot write idle-drip acceptance content");
            return this->path_;
        }

    private:
        std::filesystem::path path_;
        std::string source_;
    };

    // Only metadata is loaded. These objects never create a device or a save.
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

        explicit World(const std::filesystem::path& content,
            const std::filesystem::path& tuning_source = {}) :
            tuning((tuning_source.empty() ? content / "tuning.json" : tuning_source).string().c_str()),
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

        std::unique_ptr<Level> build(float duration = 300.0f,
            const std::array<WeaponType, 4>& tools = roster)
        {
            MenuLevelSettings settings;
            settings.set_stage("movement_course");
            settings.set_player_count(4);
            settings.set_screen_layout(ScreenLayout::four_player);
            settings.set_duration(duration);
            for (int index = 0; index < 4; ++index)
            {
                settings.set_player_team(index, PlayerTeam::a);
                settings.set_player_weapon(index, tools.at(static_cast<size_t>(index)));
            }
            settings.begin_input_claims(0);
            for (int slot = 1; slot < 4; ++slot) { require(settings.claim_input_slot(slot), "Could not claim CPU pad slot"); }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }

        std::unique_ptr<Structure> structure(std::unique_ptr<Shape> shape,
            CollisionObjectType type = CollisionObjectType::structure)
        {
            const RectangleF rectangle = shape->bounding_box();
            return std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
                std::move(shape), &this->render, type);
        }
    };
}

#include "scripts/acceptance/idle_drip/standalone_checks.h"
#include "scripts/acceptance/idle_drip/level_checks.h"

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: idle_drip.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_profiles(world, content);
        check_quiet_intervals(world);
        check_attachment_and_falling(world);
        check_terrain(world);
        check_freeze_disable_and_capacity(world);
        check_live_preference(world);
        check_level_integration(world, content);
        check_empty_held_fire(content);
        check_round_end(content);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL idle drip: " << error.what() << '\n';
        return 1;
    }
}
