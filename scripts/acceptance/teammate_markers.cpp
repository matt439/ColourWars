#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/teammate_marker.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"
#include "scripts/acceptance/support/probe.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// ROAD#18: off-screen teammate markers in the actual Level. Every human pane
// at 1, 2, 3 and 4 humans (with bot teammates that have no pane) places its
// teammates on and off its view and checks presence, clamping, direction,
// fade, Reduce effects, enemies, death and the presentation contract.

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    enum class Seat { human, bot };
    struct SeatPlan { Seat seat; PlayerTeam team; };

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
                &this->presentation, &this->resolution, nullptr, &this->partitioner);
        }

        TeammateMarkerPresentation& look()
        {
            return const_cast<TeammateMarkerPresentation&>(this->presentation.hud().teammate_marker);
        }

        std::unique_ptr<Level> build(const std::vector<SeatPlan>& plan)
        {
            MatchSetup settings;
            settings.set_stage("movement_course");
            settings.set_player_count(static_cast<int>(plan.size()));
            settings.set_duration(300.0f);
            for (std::size_t seat = 1; seat < plan.size(); ++seat)
            {
                if (plan[seat].seat == Seat::bot)
                {
                    settings.set_player_bot(static_cast<int>(seat), BotDifficulty::normal);
                }
            }
            // Sparse pads, so a pane's pad slot is never its pane or roster index.
            constexpr std::array slots{ 3, 1, 2, 0 };
            settings.begin_input_claims(slots[0]);
            std::size_t next = 1;
            for (std::size_t seat = 1; seat < plan.size(); ++seat)
            {
                if (plan[seat].seat == Seat::human)
                {
                    require(settings.claim_input_slot(slots.at(next++)), "Could not claim a marker seat");
                }
            }
            require(settings.input_roster_complete(), "Marker roster is incomplete");
            settings.set_screen_layout(static_cast<ScreenLayout>(settings.human_count() - 1));
            for (std::size_t seat = 0; seat < plan.size(); ++seat)
            {
                settings.set_player_team(static_cast<int>(seat), plan[seat].team);
                settings.set_player_weapon(static_cast<int>(seat), WeaponType::sprayer);
            }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }
    };

    PlayerInputData neutral()
    {
        PlayerInputData result;
        result.connected = true;
        return result;
    }

    void activate(Level& level)
    {
        const std::vector<PlayerInputData> inputs(input_slots::count, neutral());
        for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { level.update(inputs, dt); }
        require(level.state() == LevelState::active, "Marker fixture did not reach live play");
    }

    struct Pane
    {
        int view = 0;
        int viewer = 0;
        Vector2F size;
        float scale = 1.0f;
        RectangleF visible;
    };

    Pane pane(World& world, Level& level, int view)
    {
        Pane result;
        result.view = view;
        result.viewer = level.participant_for_view(view);
        result.size = Viewport(world.viewports.player_viewport(view).pixel_rect()).size();
        const HudPresentation& hud = world.presentation.hud();
        result.scale = std::min(result.size.x / hud.player_identity_reference_width,
            result.size.y / hud.reference_height);
        result.visible = level.player(result.viewer).camera().visible_rectangle(
            Viewport(RectangleF(Vector2F::ZERO, result.size)));
        return result;
    }

    // Puts a body's centre at `centre`, keeping its size.
    void put(Level& level, int participant, const Vector2F& centre)
    {
        const Vector2F size = level.player(participant).rectangle().size();
        require(level.place_player(participant, centre - size * 0.5f), "Could not place a marker participant");
    }

    const TeammateMarkerPlacement* find(const std::vector<TeammateMarkerPlacement>& markers, int participant)
    {
        for (const TeammateMarkerPlacement& marker : markers)
        {
            if (marker.participant == participant) { return &marker; }
        }
        return nullptr;
    }

    // One marker, checked against the pane and the teammate's real body.
    void check_marker(World& world, Level& level, const Pane& at, const TeammateMarkerPlacement& marker,
        const std::string& where)
    {
        const TeammateMarkerPresentation& look = world.look();
        const float eps = 0.01f;
        const float margin = look.margin * at.scale;
        require(std::abs(marker.scale - at.scale) < 1e-5f, where + ": marker scale is not the pane HUD scale");
        const RectangleF chip = marker.chip;
        require(std::abs(chip.width - look.size.x * at.scale) < eps &&
            std::abs(chip.height - look.size.y * at.scale) < eps, where + ": chip is not the presented size");
        for (const Vector2F point : { chip.position(), chip.position() + chip.size(), marker.tip })
        {
            require(point.x >= margin - eps && point.y >= margin - eps &&
                point.x <= at.size.x - margin + eps && point.y <= at.size.y - margin + eps,
                where + ": chip or pointer left the pane margin");
        }
        const Vector2F middle = at.size * 0.5f;
        const Vector2F target = level.player(at.viewer).camera().calculate_view_position(
            level.player(marker.participant).rectangle().center());
        Vector2F expected = target - middle;
        expected = expected / expected.length();
        require(std::abs(marker.direction.length() - 1.0f) < 1e-4f, where + ": direction is not a unit vector");
        require(marker.direction.x * expected.x + marker.direction.y * expected.y > 0.9999f,
            where + ": marker does not point toward its teammate");
        const Vector2F from = marker.centre - middle;
        require(std::abs(from.x * expected.y - from.y * expected.x) < 0.05f &&
            from.x * expected.x + from.y * expected.y > 0.0f,
            where + ": marker is not on the line from the pane centre toward its teammate");
        const Vector2F tip = marker.tip - marker.centre;
        require(tip.x * expected.x + tip.y * expected.y > 0.0f, where + ": pointer aims away from its teammate");
        // Clamped to the edge: the marker's reach meets the margin on the side
        // the teammate is beyond.
        const float reach = margin + std::max(chip.width, chip.height) * 0.5f + look.pointer_length * at.scale;
        const bool on_x = std::abs(std::abs(from.x) - (middle.x - reach)) < 0.05f;
        const bool on_y = std::abs(std::abs(from.y) - (middle.y - reach)) < 0.05f;
        require(on_x || on_y, where + ": marker is not clamped to the pane edge");
        require(!chip.contains(RectangleF(target, Vector2F::ZERO)), where + ": chip covers its own target");
    }

    int checked_markers = 0;

    // Pane 0's first teammate, which has a marker once far away.
    int mates_of_first(const Level& level)
    {
        const int viewer = level.participant_for_view(0);
        for (int other = 0; other < level.player_count(); ++other)
        {
            if (other != viewer && level.player(other).team() == level.player(viewer).team()) { return other; }
        }
        throw std::runtime_error("Marker roster has no teammate for pane 0");
    }

    void check_roster(World& world, const std::vector<SeatPlan>& plan, const std::string& name)
    {
        auto level = world.build(plan);
        require(level->teammate_markers(0).empty(), name + ": a marker showed during the countdown");
        activate(*level);
        require(level->teammate_markers(-1).empty() && level->teammate_markers(level->view_count()).empty(),
            name + ": an invalid pane returned markers");
        int humans = 0;
        for (const SeatPlan& seat : plan) { humans += seat.seat == Seat::human ? 1 : 0; }
        require(level->view_count() == humans, name + ": panes are not one per human");

        const std::array<Vector2F, 8> compass{ {
            { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, { -1.0f, 1.0f },
            { -1.0f, 0.0f }, { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } } };
        bool bot_checked = false;
        for (int view = 0; view < level->view_count(); ++view)
        {
            const int viewer = level->participant_for_view(view);
            put(*level, viewer, Vector2F(2800.0f, 650.0f));
            const Pane at = pane(world, *level, view);
            std::vector<int> mates, enemies;
            for (int other = 0; other < level->player_count(); ++other)
            {
                if (other == viewer) { continue; }
                (level->player(other).team() == level->player(viewer).team() ? mates : enemies).push_back(other);
            }
            // Everyone in view: nothing to mark. Enemies are off-screen and still unmarked.
            for (std::size_t index = 0; index < mates.size(); ++index)
            {
                put(*level, mates[index], at.visible.center() + Vector2F(60.0f * static_cast<float>(index) - 60.0f, 0.0f));
            }
            for (const int enemy : enemies)
            {
                put(*level, enemy, at.visible.center() + Vector2F(at.visible.width * 2.0f, 0.0f));
            }
            require(level->teammate_markers(view).empty(), name + ": an on-screen teammate or an enemy was marked");

            // A teammate straddling the view's edge is still on screen.
            if (!mates.empty())
            {
                const float half = level->player(mates[0]).rectangle().width * 0.5f;
                put(*level, mates[0], Vector2F(at.visible.right(), at.visible.center().y) + Vector2F(half * 0.5f, 0.0f));
                require(find(level->teammate_markers(view), mates[0]) == nullptr,
                    name + ": a teammate half inside the pane was marked");
            }

            for (std::size_t heading = 0; heading < compass.size(); ++heading)
            {
                for (std::size_t index = 0; index < mates.size(); ++index)
                {
                    const Vector2F way = compass[(heading + index * 3) % compass.size()];
                    const Vector2F reach(at.visible.width * 0.5f + 400.0f, at.visible.height * 0.5f + 400.0f);
                    put(*level, mates[index], at.visible.center() + Vector2F(way.x * reach.x, way.y * reach.y));
                }
                const std::vector<TeammateMarkerPlacement> markers = level->teammate_markers(view);
                require(markers.size() == mates.size(), name + ": not one marker per off-screen teammate");
                for (const int mate : mates)
                {
                    const TeammateMarkerPlacement* marker = find(markers, mate);
                    const std::string where = name + " pane " + std::to_string(view) + " P" +
                        std::to_string(mate + 1) + " heading " + std::to_string(heading);
                    require(marker != nullptr, where + ": missing marker");
                    check_marker(world, *level, at, *marker, where);
                    ++checked_markers;
                    if (level->is_bot(mate))
                    {
                        require(level->view_for_participant(mate) < 0, where + ": a bot owns a pane");
                        bot_checked = true;
                    }
                }
                for (const int enemy : enemies)
                {
                    require(find(markers, enemy) == nullptr, name + ": an enemy was marked");
                }
                require(find(markers, viewer) == nullptr, name + ": a pane marked its own player");
            }

            // A dead teammate has no marker; the respawn brings it back once away.
            if (!mates.empty())
            {
                require(level->kill_player(mates[0]) && find(level->teammate_markers(view), mates[0]) == nullptr,
                    name + ": a fatally hit teammate kept its marker");
                require(level->respawn_player(mates[0]), name + ": could not respawn the teammate");
                put(*level, mates[0], at.visible.center() + Vector2F(at.visible.width * 2.0f, 0.0f));
                require(find(level->teammate_markers(view), mates[0]) != nullptr,
                    name + ": a respawned off-screen teammate had no marker");
            }
        }
        const bool has_bot_mate = [&]
        {
            for (std::size_t seat = 1; seat < plan.size(); ++seat)
            {
                if (plan[seat].seat == Seat::bot && plan[seat].team == plan[0].team) { return true; }
            }
            return false;
        }();
        require(bot_checked == has_bot_mate, name + ": the bot teammate was not checked");

        put(*level, mates_of_first(*level), Vector2F(-4000.0f, -4000.0f));
        require(!level->teammate_markers(0).empty(), name + ": a far teammate had no marker");
        world.look().enabled = false;
        require(level->teammate_markers(0).empty(), name + ": disabled presentation kept the markers");
        world.look().enabled = true;
        std::cout << "PASS teammate markers " << name << ": " << level->view_count() << " pane(s), on/off/straddle, "
            "8 headings clamped and aimed, enemies, self, death/respawn" << (has_bot_mate ? ", bot teammate" : "") << '\n';
    }

    void check_fade(World& world)
    {
        const std::vector<SeatPlan> plan{ { Seat::human, PlayerTeam::a }, { Seat::bot, PlayerTeam::b },
            { Seat::human, PlayerTeam::a } };
        auto level = world.build(plan);
        activate(*level);
        const std::vector<PlayerInputData> inputs(input_slots::count, neutral());
        const TeammateMarkerPresentation& look = world.look();
        require(look.fade_in > 0.0f, "The shipped marker has no fade to check");
        const int mate = 2;
        const auto away = [&](int view)
        {
            const Pane at = pane(world, *level, view);
            put(*level, mate, at.visible.center() + Vector2F(at.visible.width * 0.5f + 900.0f, -200.0f));
        };
        put(*level, 0, Vector2F(2000.0f, 650.0f));
        put(*level, mate, level->player(0).rectangle().center() + Vector2F(40.0f, 0.0f));
        level->update(inputs, dt);
        require(level->teammate_markers(0).empty(), "Fade fixture teammate was not on screen");
        away(0);
        const std::vector<TeammateMarkerPlacement> left = level->teammate_markers(0);
        const TeammateMarkerPlacement* first = find(left, mate);
        require(first != nullptr && first->opacity == 0.0f, "A teammate who just left the view was at full strength");
        float last = 0.0f;
        const int ticks = static_cast<int>(std::ceil(look.fade_in / dt)) + 2;
        for (int tick = 0; tick < ticks; ++tick)
        {
            level->update(inputs, dt);
            away(0);
            const std::vector<TeammateMarkerPlacement> markers = level->teammate_markers(0);
            const TeammateMarkerPlacement* marker = find(markers, mate);
            require(marker != nullptr, "The fading teammate lost its marker");
            require(marker->opacity >= last && marker->opacity <= 1.0f, "Marker fade went backwards or past full");
            if (tick == 0)
            {
                require(std::abs(marker->opacity - dt / look.fade_in) < 1e-4f, "Marker fade did not follow fade_in");
            }
            last = marker->opacity;
        }
        require(last == 1.0f, "Marker did not reach full strength after fade_in");

        // Reduce effects on pane 0's own pad shows the chip at once; pane 1 still fades.
        put(*level, mate, level->player(0).rectangle().center() + Vector2F(40.0f, 0.0f));
        level->update(inputs, dt);
        const int pad = level->player(level->participant_for_view(0)).player_num();
        require(input_slots::is_pad(pad), "Fade fixture pane 0 is not a pad");
        world.preferences.pad_comfort[static_cast<std::size_t>(pad)].reduce_effects = SeatSwitch::on;
        away(0);
        const auto marked = [&]
        {
            const std::vector<TeammateMarkerPlacement> markers = level->teammate_markers(0);
            const TeammateMarkerPlacement* marker = find(markers, mate);
            require(marker != nullptr, "The Reduce effects teammate lost its marker");
            return *marker;
        };
        require(marked().opacity == 1.0f, "Reduce effects on the pane's pad still faded the marker");
        world.preferences.pad_comfort = {};
        require(marked().opacity == 0.0f, "Returning the pad to Shared did not restore the fade");
        world.preferences.reduce_effects = true;
        const TeammateMarkerPlacement steady = marked();
        require(steady.opacity == 1.0f, "The shared Reduce effects switch did not hold the marker steady");
        world.preferences.reduce_effects = false;

        // A default DrawList has no view, so these only return if the drawable
        // records nothing for a faded-out, unowned or disabled marker.
        TeammateMarker drawable(&world.presentation.hud().teammate_marker, &world.render);
        PlayerBadge badge(&world.presentation.player_identity(), &world.render);
        DrawList unused;
        TeammateMarkerPlacement copy = steady;
        copy.opacity = 0.0f;
        drawable.draw(unused, copy, level->player(mate).team_colour(), badge);
        copy = steady;
        copy.participant = -1;
        drawable.draw(unused, copy, level->player(mate).team_colour(), badge);
        world.look().enabled = false;
        drawable.draw(unused, steady, level->player(mate).team_colour(), badge);
        world.look().enabled = true;
        std::cout << "PASS teammate markers fade: starts at 0, follows fade_in to full, per-pad and shared Reduce effects hold it steady, hidden markers draw nothing\n";
    }

    void check_validation(const std::filesystem::path& content, const std::filesystem::path& scratch)
    {
        std::ifstream input_file(content / "presentation.json");
        require(input_file.good(), "Cannot read marker presentation");
        const std::string source(std::istreambuf_iterator<char>(input_file), {});
        const auto path = scratch / "teammate-marker-invalid.json";
        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() { std::error_code ignored; std::filesystem::remove(this->path, ignored); }
        } cleanup{ path };
        int rejected_count = 0;
        const auto reject = [&](auto edit)
        {
            rapidjson::Document json;
            json.Parse(source.c_str());
            require(!json.HasParseError(), "Cannot parse marker presentation fixture");
            rapidjson::Value& look = json["hud"]["teammate_marker"];
            edit(look, json.GetAllocator());
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            { std::ofstream variant(path); variant << buffer.GetString(); require(variant.good(), "Cannot write marker variant"); }
            bool rejected = false;
            try { const Presentation invalid(path.string().c_str()); }
            catch (const std::exception& error)
            {
                rejected = std::string(error.what()).find("teammate_marker") != std::string::npos;
            }
            require(rejected, "Malformed teammate_marker presentation was accepted or lost its section name");
            ++rejected_count;
        };
        for (const char* field : { "enabled", "ink", "size", "keyline_width", "badge_inset",
            "pointer_length", "pointer_width", "margin", "fade_in" })
        {
            reject([field](rapidjson::Value& look, auto&) { look.RemoveMember(field); });
        }
        for (const char* field : { "keyline_width", "badge_inset", "pointer_length", "pointer_width", "margin", "fade_in" })
        {
            reject([field](rapidjson::Value& look, auto&) { look[field].SetString("small"); });
            reject([field](rapidjson::Value& look, auto&) { look[field].SetFloat(-1.0f); });
            reject([field](rapidjson::Value& look, auto&) { look[field].SetFloat(1000.0f); });
        }
        for (const char* field : { "keyline_width", "pointer_length", "pointer_width" })
        {
            reject([field](rapidjson::Value& look, auto&) { look[field].SetFloat(0.0f); });
        }
        reject([](rapidjson::Value& look, auto&) { look["size"]["x"].SetFloat(8.0f); });
        reject([](rapidjson::Value& look, auto&) { look["size"]["y"].SetFloat(200.0f); });
        reject([](rapidjson::Value& look, auto&) { look["badge_inset"].SetFloat(14.0f); });
        reject([](rapidjson::Value& look, auto&) { look["pointer_width"].SetFloat(look["keyline_width"].GetFloat()); });
        reject([](rapidjson::Value& look, auto&) { look["pointer_width"].SetFloat(look["size"]["y"].GetFloat()); });
        reject([](rapidjson::Value& look, auto&) { look["fade_in"].SetFloat(1.5f); });
        reject([](rapidjson::Value& look, auto& allocator) { look["ink"].SetString("14141480", allocator); });
        reject([](rapidjson::Value& look, auto&) { look["enabled"].SetFloat(1.0f); });
        std::cout << "PASS teammate markers content: " << rejected_count << " malformed teammate_marker variants rejected by name\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("teammate markers", [&]
    {
        // Unbuffered, so a crash still shows how far the case got.
        std::cout << std::unitbuf;
        require(argc == 2, "Usage: teammate_markers.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        using S = Seat;
        const PlayerTeam a = PlayerTeam::a, b = PlayerTeam::b;
        for (const ScreenResolution size : { ScreenResolution::s_1280_720, ScreenResolution::s_1920_1080 })
        {
            world.resolution.set_resolution(size);
            const std::string at = world.resolution.resolution_string();
            check_roster(world, { { S::human, a }, { S::bot, a }, { S::bot, a }, { S::bot, b } }, "1 human " + at);
            check_roster(world, { { S::human, a }, { S::bot, a }, { S::human, a }, { S::bot, b } }, "2 humans " + at);
            check_roster(world, { { S::human, a }, { S::human, a }, { S::bot, a }, { S::human, b } }, "3 humans " + at);
            check_roster(world, { { S::human, a }, { S::human, a }, { S::human, b }, { S::human, a } }, "4 humans " + at);
        }
        world.resolution.set_resolution(ScreenResolution::s_1280_720);
        check_fade(world);
        check_validation(content, std::filesystem::absolute(argv[0]).parent_path());
        std::cout << "PASS teammate markers: " << checked_markers << " placed markers checked\n";
        return 0;
    });
}
