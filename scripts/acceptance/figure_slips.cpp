#include "game/pch.h"
#include "game/objects/figure_person.h"
#include "game/objects/match_random.h"
#include "game/objects/player.h"
#include "game/objects/weapon_builder.h"
#include "game/config/presentation/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

#include <filesystem>
#include <iostream>
#include <set>
#include <string>

// The figure slips (POL#46 stage 2, docs/drawing-mistakes-research.md section
// 9, decision 8): which person a hand draws, that every person the slips allow
// has a band over every body frame, and what their strokes and blink eyes are.
// build-painter-art.ps1 checks each person's cells against their strokes; this
// checks what the game reads.

using namespace labrador;

namespace
{
    using acceptance::require;

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
                render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(),
                    TextureHandle{}));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
    };

    std::unique_ptr<Player> make_player(World& world, const FigurePerson& person)
    {
        const RectangleF body(0.0f, -world.tuning.player().size.y,
            world.tuning.player().size.x, world.tuning.player().size.y);
        auto weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a, 0, 0,
            Colour::white, WeaponType::sprayer, body.center(), &world.render, &world.audio,
            &world.preferences);
        return std::make_unique<Player>(body, &world.tuning,
            &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
            &world.presentation.respawn_redraw(), &world.render, &world.audio, &world.preferences,
            0, PlayerTeam::a, WeaponType::sprayer, std::move(weapon), WeaponType::none, Colour::white,
            Viewport{}, body.position(), Vector2F::zero, 0.0f, Vector2F::zero,
            SpriteFlip::none, 0.0f, 0, &world.presentation.figure(), nullptr, person);
    }

    bool same_strokes(const std::vector<RespawnStroke>& a, const std::vector<RespawnStroke>& b)
    {
        if (a.size() != b.size())
        {
            return false;
        }
        for (size_t at = 0; at < a.size(); ++at)
        {
            if (a[at].part != b[at].part || a[at].points != b[at].points)
            {
                return false;
            }
        }
        return true;
    }

    size_t index_of(const FigureSlipsPresentation& slips, const std::string& id)
    {
        for (size_t index = 0; index < slips.list.size(); ++index)
        {
            if (slips.list[index].id == id)
            {
                return index;
            }
        }
        throw std::runtime_error("figure.slips has no slip '" + id + "'.");
    }

    void check_people(const World& world)
    {
        const FigureSlipsPresentation& slips = world.presentation.figure().slips;
        const std::vector<FigurePerson> people = FigurePerson::everyone(slips);
        require(!slips.list.empty() && people.size() > slips.list.size(),
            "The shipped figure slips allow no pairs");
        std::set<std::string> names;
        for (const FigurePerson& person : people)
        {
            const std::vector<size_t>& chosen = person.slips();
            require(chosen.size() == 1 || (chosen.size() == 2 && !slips.is_exclusive(chosen[0], chosen[1])),
                "A person drew an exclusive pair");
            require(names.insert(person.name(slips)).second, "Two people share a band name");
        }

        require(!FigurePerson(slips, 0).slipped(), "The house hand made a slip");
        std::set<std::string> seen;
        size_t pairs = 0;
        for (unsigned int seed = 0; seed < 4000; ++seed)
        {
            const std::uint64_t hand = MatchRandom(seed).hand();
            const FigurePerson person(slips, hand);
            require(person.slipped(), "A match's hand drew the house figure");
            require(person.name(slips) == FigurePerson(slips, hand).name(slips),
                "One hand drew two different people");
            require(names.count(person.name(slips)) == 1, "A hand drew a person the slips do not allow");
            pairs += person.slips().size() == 2 ? 1 : 0;
            seen.insert(person.name(slips));
        }
        require(seen.size() == people.size(), "4000 rounds never drew some allowed person");
        require(pairs > 1600 && pairs < 2400, "Two slips were not drawn about half the time");
        std::cout << "PASS figure slips: " << people.size() << " people, house hand clean, every person drawn, pairs "
            << pairs << "/4000\n";
    }

    void check_bands(World& world)
    {
        const FigureSlipsPresentation& slips = world.presentation.figure().slips;
        const PlayerTuning& player = world.tuning.player();
        const SpriteSheet* sheet = world.render.sprite_sheet(player.idle.sheet);
        for (const FigurePerson& person : FigurePerson::everyone(slips))
        {
            std::vector<std::string> names{ person.band_frame(slips, player.idle.strip, 0) };
            for (const PlayerAnimationTuning* action : { &player.takeoff, &player.land, &player.fire,
                &player.hurt, &player.elimination })
            {
                const int frames = sheet->animation_strip(sheet->resolve_animation_strip(action->strip)).frame_count();
                for (int frame = 0; frame < frames; ++frame)
                {
                    names.push_back(person.band_frame(slips, action->strip, frame));
                }
            }
            for (const std::string& name : names)
            {
                const RectangleI& band = sheet->sprite_frame(sheet->resolve_sprite_frame(name)).source_rectangle();
                require(band.width == static_cast<int>(slips.band.width) &&
                    band.height == static_cast<int>(slips.band.height), "A face band is not the band's size: " + name);
            }
            // Building a player resolves every band; a missing one throws here.
            const std::unique_ptr<Player> figure = make_player(world, person);
            require(figure->person().name(slips) == person.name(slips), "A player lost its person");
        }
        const std::unique_ptr<Player> house = make_player(world, FigurePerson());
        require(!house->person().slipped() &&
            same_strokes(house->drawn_strokes(), world.presentation.respawn_redraw().strokes),
            "A house player drew other strokes than the house ones");
        std::cout << "PASS figure slips: every person's bands resolve at the band size and build a player\n";
    }

    void check_strokes_and_eyes(const World& world)
    {
        const FigurePresentation& figure = world.presentation.figure();
        const FigureSlipsPresentation& slips = figure.slips;
        const RespawnRedrawPresentation& redraw = world.presentation.respawn_redraw();
        const std::vector<RespawnStroke>& house = redraw.strokes;
        for (const FigurePerson& person : FigurePerson::everyone(slips))
        {
            const std::vector<RespawnStroke> strokes = person.strokes(redraw, slips);
            require(!same_strokes(strokes, house), "A person drew exactly the house strokes: " + person.name(slips));
            const auto eyes = person.blink_eyes(figure, redraw);
            const bool moved_eyes = person.name(slips).find("eyes_close_set") != std::string::npos;
            require(moved_eyes == (eyes != figure.quiet.eyes), "Blink eyes moved without an eye slip, or stayed with one");
        }
        const FigurePerson house_hand(slips, 0);
        require(house_hand.blink_eyes(figure, redraw) == figure.quiet.eyes, "The house blink moved");
        for (const FigurePerson& person : FigurePerson::everyone(slips))
        {
            if (person.name(slips) != "eyes_close_set")
            {
                continue;
            }
            const auto eyes = person.blink_eyes(figure, redraw);
            require(eyes[0].x == figure.quiet.eyes[0].x + 4.0f && eyes[1] == figure.quiet.eyes[1],
                "eyes_close_set did not move the back eye's blink one cell forward");
            const auto drawn = eye_rectangles(person.strokes(redraw, slips), redraw.pen_width);
            require(drawn.size() == 2 && drawn[0] == eyes[0], "The blink and the drawn eye disagree");
        }
        // A short ear removes a stroke, and a removed stroke is gone.
        const FigurePerson short_ear = FigurePerson::everyone(slips)[index_of(slips, "front_ear_short")];
        require(short_ear.strokes(redraw, slips).size() == house.size() - 1, "front_ear_short kept every stroke");
        std::cout << "PASS figure slips: each person's own strokes, and blink eyes that follow eyes_close_set\n";
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("figure slips", [&]
    {
        require(argc == 2, "Usage: figure_slips.exe <content directory>");
        World world{ std::filesystem::path(argv[1]) };
        check_people(world);
        check_bands(world);
        check_strokes_and_eyes(world);
        return 0;
    });
}
