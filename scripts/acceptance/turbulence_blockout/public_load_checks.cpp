#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/fixtures.h"
#include "game/objects/level_builder.h"
#include "game/input_bindings.h"
#include "engine/core/file_path.h"
#include "engine/render/sprite_font_file.h"

namespace turbulence_blockout_acceptance
{
    void public_load(Services& services, const std::filesystem::path& content)
    {
        // Complete CPU resource metadata, without Application, window, devices
        // or the player's save. LevelBuilder still resolves the actual fonts,
        // sheets, stage file, sound names, HUD and opening/countdown resources.
        RenderResources render;
        AudioResources audio;
        const JsonDocument manifest = read_json_file(path_to_utf8(content / "manifest.json").c_str());
        const JsonValue assets = manifest.root().array("assets");
        bool candidate_registered = false;
        for (std::size_t index = 0; index < assets.size(); ++index)
        {
            const JsonValue asset = assets.at(index);
            const std::string kind = asset.identifier("kind");
            const auto directory = content / path_from_utf8(asset.identifier("directory"));
            const JsonValue names = asset.array("names");
            for (std::size_t entry = 0; entry < names.size(); ++entry)
            {
                const std::string name = names.at(entry).as_identifier();
                if (kind == "sprite_sheet")
                {
                    render.add_sprite_sheet(name, read_sprite_sheet(path_to_utf8(directory /
                        path_from_utf8(name + ".json")).c_str(), TextureHandle{}));
                }
                else if (kind == "font")
                {
                    const SpriteFontFile file = read_sprite_font_file(path_to_utf8(directory /
                        path_from_utf8(name + ".spritefont")));
                    auto font = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                    if (font->contains(U'?')) { font->set_stand_in(U'?'); }
                    render.add_font(name, std::move(font));
                }
                else if (kind == "sound_bank") { audio.add_sound_bank(name, SoundBank::silent()); }
                else
                {
                    require(kind == "level", "Unknown manifest kind in candidate public loader");
                    if (name == "turbulence_blockout")
                    {
                        candidate_registered = true;
                        require((directory / path_from_utf8(name + ".json")).lexically_normal() ==
                            (content / "levels/turbulence_blockout.json").lexically_normal(),
                            "Candidate is registered at an unexpected path");
                    }
                }
            }
        }
        require(candidate_registered, "Public-load check requires the registered candidate");
        services.tuning.validate_resources(render, audio);
        services.presentation.validate_resources(render, audio);
        for (const int count : { 1, 4 })
        {
            Registry<LevelLoadedInfo> levels("Turbulence candidate Level");
            auto loaded = std::make_unique<LevelLoadedInfo>(path_to_utf8(content / "levels/turbulence_blockout.json").c_str());
            loaded->validate();
            const LevelObjectBuilder objects(&render, &audio, &services.preferences, &services.tuning.paint());
            loaded->validate_content(objects, render, audio);
            levels.add("turbulence_blockout", std::move(loaded));
            ResolutionManager resolution;
            ViewportManager viewports(&resolution);
            Mouse mouse;
            Partitioner partitioner;
            const InputBindings bindings;
            MatchSetup settings;
            settings.set_player_count(count);
            settings.set_stage("turbulence_blockout");
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            constexpr std::array<int, 4> slots{ 3, input_slots::keyboard_and_mouse, 1, 0 };
            settings.begin_input_claims(slots[0]);
            for (int seat = 1; seat < count; ++seat)
            { require(settings.claim_input_slot(slots[static_cast<std::size_t>(seat)]), "Cannot build public sparse roster"); }
            for (int seat = 0; seat < count; ++seat)
            {
                settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(seat, WeaponType::sprayer);
            }
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &render, &audio, &services.preferences, &levels,
                &services.tuning, &services.presentation, &resolution, nullptr, &partitioner, &bindings);
            std::srand(179u);
            auto level = builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
            require(level->player_count() == count && !level->has_horizontal_motion(),
                "Public candidate build lost its roster or acquired an unqualified mover");
            std::size_t a_index = 0, b_index = 0;
            for (int seat = 0; seat < count; ++seat)
            {
                const Player& player = level->player(seat);
                const bool a = settings.player_team(seat) == PlayerTeam::a;
                const auto spawns = a ? services.info.team_a_spawns() : services.info.team_b_spawns();
                const Vector2F expected = spawns[a ? a_index++ : b_index++];
                require(player.player_num() == slots[static_cast<std::size_t>(seat)] &&
                    player.team() == settings.player_team(seat) && player.rectangle() == RectangleF(expected, services.tuning.player().size),
                    "Public LevelBuilder confused spawn, team, device slot or view index");
            }
            std::vector<PlayerInputData> idle(input_slots::count, input());
            int countdown_ticks = 0;
            while (!level->match_started() && countdown_ticks++ < 240) { level->update(idle, dt); }
            require(level->match_started(), "Unmodified candidate count-in never reached gameplay");
            for (int tick = 0; tick < 12; ++tick) { level->update(idle, dt); }
            require(viewports.layout() == settings.screen_layout(), "Public Level construction lost the requested layout");
            const LevelEndInfo score = level->level_end_info();
            require(std::fabs(score.total_area - 12400.0f * services.tuning.paint().thickness) < 0.01f &&
                score.team_a_area == 0.0f && score.team_b_area == 0.0f,
                "Public-built Level score differs from the static face budget before firing");
            for (int seat = 0; seat < count; ++seat)
            {
                const Player& player = level->player(seat);
                require(supported(player) && player.state() == PlayerState::alive &&
                    std::fabs(player.rectangle().bottom() - 1120.0f) < tolerance,
                    "Public-built idle player did not retain live home support");
            }
            std::cout << "MEASURE Turbulence public LevelBuilder players=" << count << " count_in_ticks=" << countdown_ticks <<
                " neutral_score=" << score.total_area << " active_updates=12 sparse_slots=3,keyboard,1,0\n";
        }
        std::cout << "PASS actual registered candidate through fresh LevelLoadedInfo/LevelBuilder, "
            "1/4-player roster/spawn/layout, unchanged count-in and active physics; CPU metadata only\n";
    }
}
