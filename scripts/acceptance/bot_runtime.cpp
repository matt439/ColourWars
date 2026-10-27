#include "game/pch.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/ai/match_bot_support.h"
#include "game/debug/dev_tools.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#include "game/debug/replay.h"
#endif
#include <iostream>

namespace
{
    using namespace labrador;
    using namespace mattmath;
    using paint_surfaces_acceptance::require;

    class World
    {
    public:
        explicit World(const std::filesystem::path& content) : assets(content)
        {
            for (const char* stage : { "crossfeed", "king_of_the_hill", "turbulence", "close_quarters", "movement_course" })
            {
                auto info = std::make_unique<LevelLoadedInfo>((content / "levels" /
                    (std::string(stage) + ".json")).string().c_str());
                info->validate(); levels.add(stage, std::move(info));
            }
        }
        std::unique_ptr<Level> build(MenuLevelSettings settings)
        {
            viewports.set_layout(settings.screen_layout());
            LevelBuilder builder(&viewports, &assets.render, &assets.audio, &assets.preferences,
                &levels, &assets.tuning, &assets.presentation, &resolution, nullptr, &partitioner, &mouse);
            return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }
        paint_surfaces_acceptance::World assets;
        Registry<LevelLoadedInfo> levels{ "bot-runtime" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        Mouse mouse;
        Partitioner partitioner;
    };

    MenuLevelSettings roster(int count, unsigned int mask, BotDifficulty difficulty)
    {
        MenuLevelSettings settings; settings.set_player_count(count);
        for (int seat = 1; seat < count; ++seat)
        {
            if ((mask & (1u << (seat - 1))) != 0) { settings.set_player_bot(seat, difficulty); }
        }
        settings.begin_input_claims(3);
        for (const int slot : { input_slots::keyboard_and_mouse, 1, 2 })
        { std::ignore = settings.claim_input_slot(slot); }
        settings.set_screen_layout(static_cast<ScreenLayout>(settings.human_count() - 1));
        settings.set_stage("crossfeed"); settings.set_duration(30.0f);
        for (int seat = 0; seat < count; ++seat)
        {
            settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(seat, static_cast<WeaponType>(seat % 5));
        }
        return settings;
    }

    void check_mapping(World& world)
    {
        int cases = 0;
        for (int count = 1; count <= 4; ++count)
        {
            for (unsigned int mask = 0; mask < (1u << (count - 1)); ++mask)
            {
                // Every legal human/bot arrangement; difficulties rotate across
                // cases, while per-seat enum/data coverage lives in bot_driver.
                const auto difficulty = static_cast<BotDifficulty>(cases % 3);
                const MenuLevelSettings settings = roster(count, mask, difficulty);
                std::srand(8301); auto level = world.build(settings);
                require(level->player_count() == count && level->view_count() == settings.human_count(),
                    "Runtime conflated participants and human views");
                int view = 0;
                for (int seat = 0; seat < count; ++seat)
                {
                    const auto& planned = settings.player_settings()[static_cast<size_t>(seat)];
                    const bool bot = planned.controller == PlayerController::bot;
                    require(level->is_bot(seat) == bot && level->player(seat).player_num() == planned.num &&
                        level->player(seat).team() == planned.team &&
                        level->player(seat).primary_weapon().type() == planned.weapon,
                        "Runtime lost participant controller/device/team/tool identity");
                    if (bot)
                    {
                        require(level->view_for_participant(seat) == -1 &&
                            level->bot_difficulty(seat) == difficulty &&
                            level->bot_identity(seat).find(L"P" + std::to_wstring(seat + 1)) == 0,
                            "Bot gained a human pane or lost its world identity/difficulty");
                    }
                    else
                    {
                        require(level->view_for_participant(seat) == view &&
                            level->participant_for_view(view) == seat,
                            "Sparse humans lost their explicit participant/view mapping");
                        ++view;
                    }
                }
                ++cases;
            }
        }
        require(cases == 15, "Mapping matrix missed a legal roster arrangement");
        std::cout << "PASS all 15 participant/human-view patterns: sparse devices, bot identity, teams and tools\n";
    }

    class Observe final : public SeatInputFilter
    {
    public:
        void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
        { this->last = seats; ++this->steps; }
        std::vector<PlayerInputData> last;
        int steps = 0;
    };

    void check_support_roles(World& world)
    {
        // Crossfeed's six home/roof/upper-step platforms score paint while
        // retaining one-way collision. King of the Hill also has ordinary
        // non-scoring one-way supports; collision alone cannot supply this role.
        for (const char* stage : { "crossfeed", "king_of_the_hill" })
        {
            MenuLevelSettings settings = roster(2, 1, BotDifficulty::normal);
            settings.set_stage(stage);
            auto level = world.build(settings);
            int scoring_one_way = 0, plain_one_way = 0;
            bool scoring_solid = false, plain_solid = false;
            for (const MatchBotSupport& support : level->bot_supports())
            {
                if (support.one_way)
                {
                    scoring_one_way += support.paintable ? 1 : 0;
                    plain_one_way += support.paintable ? 0 : 1;
                }
                else
                {
                    scoring_solid = scoring_solid || support.paintable;
                    plain_solid = plain_solid || !support.paintable;
                }
            }
            require(scoring_solid && plain_solid, "Bot support observations lost solid scoring/boundary roles");
            if (std::string_view(stage) == "crossfeed")
            {
                require(scoring_one_way == 6 && plain_one_way == 0,
                    "Crossfeed paintable one-way platforms lost their independent scoring role");
            }
            else
            {
                require(plain_one_way > 0, "Non-scoring one-way supports acquired an invented paint role");
            }
        }
        std::cout << "PASS actual scoring/non-scoring support roles remain independent of one-way collision\n";
    }

    void check_inputs(World& world)
    {
        MenuLevelSettings settings = roster(4, 5, BotDifficulty::hard); // P1/P3 humans, P2/P4 bots.
        settings.set_stage("movement_course");
        settings.set_player_weapon(1, WeaponType::sprayer); settings.set_player_weapon(3, WeaponType::roller);
        std::srand(8301); auto level = world.build(settings);
        std::vector<PlayerInputData> physical(input_slots::count);
        for (auto& input : physical) { input.connected = true; }
        const float step = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::SecondsToTicks(1.0 / 60.0)));
        for (int tick = 0; tick < 900 && level->state() != LevelState::active; ++tick) { level->update(physical, step); }
        require(level->state() == LevelState::active, "Bot runtime did not finish countdown");
        Observe observer;
#if COLOURWARS_DEV_TOOLS
        ReplayFile header; header.settings = settings; header.dt_bits = std::bit_cast<std::uint32_t>(step);
        ReplayRecorder recorder("shipping-bots", header, &observer, 1000);
        SeatInputFilter* final_filter = &recorder;
#else
        SeatInputFilter* final_filter = &observer;
#endif
        bool drove = false;
        for (int tick = 0; tick < 120; ++tick)
        {
            level->update(physical, step, final_filter);
            require(observer.last.size() == 4 && !observer.last[0].primary_shoot &&
                observer.last[0].x_movement == 0 && observer.last[2].x_movement == 0,
                "Shipping driver changed an ordinary human input");
            for (const int seat : { 1, 3 })
            {
                const auto& input = observer.last[static_cast<size_t>(seat)];
                require(input.connected && !input.toggle_pause_menu && !input.pause_held && !input.toggle_debug,
                    "Shipping bot emitted a menu/pause/debug action or claimed a device");
                drove |= input.primary_shoot || input.jump_pressed || input.x_movement != 0;
            }
        }
        require(drove, "Runtime did not actually drive the selected bots");
        const int observed_steps = observer.steps;
        const RectangleF frozen = level->player(1).rectangle();
        for (int tick = 0; tick < 10; ++tick) { level->update(physical, 0.0f, final_filter); }
        require(observer.steps == observed_steps && level->player(1).rectangle() == frozen,
            "Zero-time updates advanced bot decisions or actors");
#if COLOURWARS_DEV_TOOLS
        require(recorder.file().inputs.size() == 4 && recorder.file().inputs[1].size() == 120 &&
            replay_codec::same(recorder.file().inputs[1].back(), observer.last[1]),
            "Recording did not capture the final shipping bot input");
        // Playback directly replaces inputs, even though fresh bot drivers exist.
        std::srand(8301); auto replay_level = world.build(settings);
        for (int tick = 0; tick < 900 && replay_level->state() != LevelState::active; ++tick)
        { replay_level->update(physical, step); }
        ReplayPlayer player("shipping-bots", recorder.file());
        require(player.replaces_bot_inputs(), "Replay failed to bypass live bot decisions");
        for (int tick = 0; tick < 120; ++tick) { replay_level->update(physical, step, &player); }
        require(MatchFingerprint::difference(MatchFingerprint::of(*level, 120),
            MatchFingerprint::of(*replay_level, 120)).empty(),
            "Actual shipping-bot record/play changed final simulation state");
#endif
        std::cout << "PASS ordinary human inputs retained, shipping bot actions and final-input replay ordering\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: bot_runtime <content>");
        World world(std::filesystem::absolute(argv[1]));
        check_mapping(world); check_support_roles(world); check_inputs(world);
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "FAIL bot runtime: " << error.what() << '\n'; return 1; }
}
