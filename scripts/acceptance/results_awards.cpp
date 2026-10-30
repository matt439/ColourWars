#include "game/pch.h"
#include "game/objects/level_end_info.h"
#include "game/objects/paintable_object.h"
#include "game/objects/player.h"
#include "game/objects/structure.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "game/input_slots.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/render/sprite_font_file.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"

#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

using namespace labrador;
using namespace mattmath;

// ROAD#12: the results call-outs. The tallies are passive and the choice of
// award is a pure function of LevelEndInfo; the layout is validated with the
// rest of presentation.json and checked in the menu_text case.
namespace
{
    using acceptance::require;

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    using Awards = std::vector<std::optional<MatchAward>>;

    void award_choice()
    {
        const std::vector<int> owners{ 0, 1, paint_owners::index(-1, 2), paint_owners::index(-1, 3) };
        LevelEndInfo info;
        require(info.awards(owners) == Awards(4), "A scoreless match handed out an award");

        // One leader per tally: each award goes to its own leader.
        info.laid_areas[0] = 50.0f; info.laid_areas[1] = 40.0f;
        info.recaptured_areas[1] = 12.0f; info.recaptured_areas[0] = 3.0f;
        info.longest_airtimes[static_cast<size_t>(owners[2])] = 1.5f;
        info.splats[static_cast<size_t>(owners[3])] = 2;
        require(info.awards(owners) == Awards{ MatchAward::paint_laid, MatchAward::recaptured,
            MatchAward::airtime, MatchAward::splats }, "Distinct leaders did not each get their award");

        // A sweep shows only the first award; nothing passes to a runner-up.
        LevelEndInfo sweep;
        sweep.laid_areas[0] = 9.0f; sweep.laid_areas[1] = 8.0f;
        sweep.recaptured_areas[0] = 4.0f; sweep.recaptured_areas[1] = 3.0f;
        sweep.longest_airtimes[0] = 2.0f; sweep.longest_airtimes[1] = 1.0f;
        sweep.splats[0] = 5; sweep.splats[1] = 4;
        require(sweep.awards({ 0, 1 }) == Awards{ MatchAward::paint_laid, std::nullopt },
            "A swept award was passed down to the runner-up");

        // Ties share; a tied leader who already shows an award keeps it.
        LevelEndInfo tie;
        tie.laid_areas[0] = 10.0f;
        tie.splats[0] = 2; tie.splats[1] = 2; tie.splats[2] = 1;
        require(tie.awards({ 0, 1, 2 }) == Awards{ MatchAward::paint_laid, MatchAward::splats, std::nullopt },
            "Tied leaders did not share, or a lower tally won");

        // Splats come last, so paint wins a player's one call-out.
        LevelEndInfo order;
        order.laid_areas[1] = 1.0f; order.splats[1] = 9;
        order.recaptured_areas[0] = 0.5f;
        require(order.awards({ 0, 1 }) == Awards{ MatchAward::recaptured, MatchAward::paint_laid },
            "Award priority is not paint laid, recaptured, airtime, splats");

        // Nobody to beat alone; an unknown owner has no tally.
        require(info.awards({ 0 }) == Awards(1) && info.awards({}).empty(),
            "A lone player was given an award");
        require(info.awards({ -1, paint_owners::count, 1 }) == Awards{ std::nullopt, std::nullopt,
            MatchAward::paint_laid }, "An invalid owner read a tally");
    }

    void paint_credit()
    {
        PaintCredit credit;
        credit.add(0, PlayerTeam::none, PlayerTeam::a, 4.0f);
        credit.add(0, PlayerTeam::a, PlayerTeam::a, 4.0f);
        credit.add(1, PlayerTeam::a, PlayerTeam::b, 4.0f);
        credit.add(1, PlayerTeam::b, PlayerTeam::none, 4.0f);
        credit.add(-1, PlayerTeam::none, PlayerTeam::a, 4.0f);
        credit.add(paint_owners::count, PlayerTeam::none, PlayerTeam::a, 4.0f);
        require(credit.laid[0] == 4.0f && credit.recaptured[0] == 0.0f &&
            credit.laid[1] == 4.0f && credit.recaptured[1] == 4.0f,
            "Capture credit counted friendly repaint, or missed a capture or recapture");
        PaintCredit merged;
        merged.merge(credit); merged.merge(credit);
        require(merged.laid[1] == 8.0f && merged.recaptured[1] == 8.0f, "Credit did not merge");
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
            const SpriteFontFile file = read_sprite_font_file(
                (content / "fonts" / "lato_black_24.spritefont").string());
            this->render.add_font("lato_black_24",
                std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing));
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
    };

    struct Course
    {
        World& world;
        Scene scene{ nullptr, nullptr };
        Player* player;

        Course(World& resources, bool floor) : world(resources)
        {
            const RectangleF rectangle(0.0f, -world.tuning.player().size.y,
                world.tuning.player().size.x, world.tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a,
                0, 0, Colour::white, WeaponType::sprayer, rectangle.center(),
                &world.render, &world.audio, &world.preferences);
            this->player = this->scene.add(std::make_unique<Player>(rectangle,
                &world.tuning, &world.presentation.movement_feedback(),
                &world.presentation.combat_feedback(), &world.presentation.respawn_redraw(),
                &world.render, &world.audio, &world.preferences, 0, PlayerTeam::a,
                WeaponType::sprayer, std::move(weapon), WeaponType::none,
                Colour::white, Viewport{}, rectangle.top_left()));
            if (floor)
            {
                const RectangleF ground(-10000.0f, 0.0f, 20000.0f, 100.0f);
                this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                    ground, std::make_unique<RectangleF>(ground), &world.render,
                    CollisionObjectType::structure));
            }
            this->scene.end_tick();
        }

        void tick(bool jump = false)
        {
            PlayerInputData input;
            input.connected = true;
            input.jump_pressed = jump;
            input.jump_held = jump;
            this->player->set_player_input(input);
            this->scene.update(dt);
            std::ignore = this->player->update_weapon_and_get_projectiles(dt, this->scene.collision_objects());
            this->scene.resolve();
            this->player->end_contacts(this->scene.contacts());
            this->player->update_prev_rectangle();
            this->scene.end_tick();
        }

        bool airborne() const
        {
            const PlayerMoveState move = this->player->move_state();
            return move == PlayerMoveState::jumping || move == PlayerMoveState::in_air ||
                move == PlayerMoveState::dropping_down;
        }

        // Jumps from the ground and returns the ticks spent in the air.
        int hop(int held)
        {
            for (int tick = 0; tick < 30; ++tick) { this->tick(); }
            require(!this->airborne(), "Fixture player is not standing before the jump");
            int air = 0;
            for (int tick = 0; tick < 600; ++tick)
            {
                this->tick(tick < held);
                if (this->airborne()) { ++air; }
                else if (air > 0)
                {
                    // The player's next update banks the landed flight.
                    this->tick();
                    return air;
                }
            }
            throw std::runtime_error("Fixture jump never landed");
        }
    };

    void airtime(World& world)
    {
        // The spawn rectangle meets the floor; settling onto it may count a tick.
        Course standing(world, true);
        for (int tick = 0; tick < 5; ++tick) { standing.tick(); }
        const float settled = standing.player->longest_airtime();
        require(settled <= dt, "Settling onto the floor counted as a flight");
        for (int tick = 0; tick < 60; ++tick) { standing.tick(); }
        require(standing.player->longest_airtime() == settled, "Standing still counted as airtime");

        const int air = standing.hop(30);
        const float longest = standing.player->longest_airtime();
        require(air > 10 && longest > static_cast<float>(air - 3) * dt &&
            longest < static_cast<float>(air + 3) * dt,
            "A landed jump's airtime does not match the ticks spent in the air");
        const int short_air = standing.hop(1);
        require(short_air < air && standing.player->longest_airtime() == longest,
            "A shorter hop replaced the longest airtime");

        Course falling(world, false);
        for (int tick = 0; tick < 120; ++tick) { falling.tick(); }
        require(falling.airborne() && falling.player->longest_airtime() == 0.0f,
            "A flight that never landed counted as airtime");
    }

    // The page leaves out an award that would meet its row. With the shipped
    // copy and layout that never happens: every award clears the widest row
    // any roster label, team and percentage can make, with room to spare.
    void shipped_layout(World& world)
    {
        const ResultsMenuPresentation& look = world.presentation.results_menu();
        const MenuText& text = world.presentation.menu_text();
        require(look.contribution_font == "lato_black_24", "Fixture loads the wrong contribution font");
        const Font& font = *world.render.font(world.render.resolve_sprite_font(look.contribution_font));
        float widest = 0.0f;
        for (int player = 0; player < 4; ++player)
        {
            std::vector<std::wstring> labels;
            for (int slot = 0; slot < input_slots::count; ++slot) { labels.push_back(text.roster_label(player, slot)); }
            for (BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
            { labels.push_back(text.bot_roster_label(player, difficulty)); }
            for (const std::wstring& label : labels)
                for (const std::wstring* team : { &text.results.contribution_team_a, &text.results.contribution_team_b })
                    for (int tenth = 0; tenth <= 1000; ++tenth)
                    {
                        const std::wstring row = label + *team + std::to_wstring(tenth / 10) + L"." +
                            std::to_wstring(tenth % 10) + L"%";
                        widest = std::max(widest, font.measure(row).x);
                    }
        }
        const float row_right = look.contribution_position.x + widest;
        const float fill_right = (1920.0f + look.fill_box_size.x) / 2.0f;
        float margin = std::numeric_limits<float>::max();
        for (int a = 0; a < match_award_count; ++a)
        {
            margin = std::min(margin, fill_right -
                font.measure(text.results.award_line(static_cast<MatchAward>(a))).x - row_right);
        }
        // Validation only needs the award inside the panel; the shipped copy
        // also keeps a readable gap from the longest row.
        require(margin >= 24.0f, "A shipped award would crowd or meet the widest contribution row");
        std::cout << "Narrowest gap between a contribution row and an award: " << margin << '\n';
    }

    void award_text(const Presentation& presentation)
    {
        const MenuResultsText& text = presentation.menu_text().results;
        require(text.award_line(MatchAward::paint_laid) == L"Most paint laid" &&
            text.award_line(MatchAward::recaptured) == L"Most retaken" &&
            text.award_line(MatchAward::airtime) == L"Best airtime" &&
            text.award_line(MatchAward::splats) == L"Most splats",
            "Results award copy changed or maps to the wrong award");
    }
}

int main(int argc, char** argv)
{
    return acceptance::run_probe("results awards", [&]
    {
        require(argc == 2, "Usage: results_awards.exe <staged content directory>");
        const std::filesystem::path content(argv[1]);
        award_choice();
        paint_credit();
        World world(content);
        airtime(world);
        award_text(world.presentation);
        shipped_layout(world);
        std::cout << "Results awards acceptance passed\n";
        return 0;
    });
}
