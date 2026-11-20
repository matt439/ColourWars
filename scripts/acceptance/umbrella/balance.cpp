#include "game/pch.h"
#include "scripts/acceptance/umbrella/balance.h"
#include "game/ai/match_bot_driver.h"
#include "scripts/acceptance/bot_navigation/fixtures.h"
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <limits>

namespace umbrella_balance
{
    namespace
    {
        using bot_navigation_acceptance::World;
        using bot_navigation_acceptance::require;

        unsigned int read_number(std::string_view text)
        {
            unsigned int value{};
            const auto end = text.data() + text.size();
            const auto result = std::from_chars(text.data(), end, value);
            require(result.ec == std::errc{} && result.ptr == end, "Malformed seed or seed count");
            return value;
        }

        // Both unclaimed participant seats run the same driver as a regular
        // bot. The normal roster keeps a human host so cannot express this.
        class BotSeats final : public SeatInputFilter
        {
        public:
            BotSeats(const GameTuning& tuning, unsigned int seed, MatchVariant variant) :
                driver_(&tuning, { BotDifficulty::normal, BotDifficulty::normal }, seed, variant) {}

            void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override
            {
                driver_.drive(seats, level.bot_world());
                ++active_ticks;
            }

            int active_ticks = 0;

        private:
            MatchBotDriver driver_;
        };

        void play(World& world, const std::string& stage, WeaponType opponent, SubType umbrella_sub,
            MatchVariant variant, unsigned int seed, int umbrella_seat)
        {
            MatchSetup setup;
            setup.set_player_count(2);
            setup.set_screen_layout(labrador::ScreenLayout::two_player);
            setup.set_stage(stage);
            setup.set_duration(60.0f);
            setup.set_variant(variant);
            for (int seat = 0; seat < 2; ++seat)
            {
                setup.set_player_team(seat, seat == 0 ? PlayerTeam::a : PlayerTeam::b);
                setup.set_player_weapon(seat, seat == umbrella_seat ? WeaponType::umbrella : opponent);
                setup.set_player_sub(seat, seat == umbrella_seat ? umbrella_sub : SubType::grenade);
            }
            const auto level = world.build(setup, seed);
            BotSeats bots(world.assets.tuning, seed, variant);
            const Player& holder = level->player(umbrella_seat);
            const auto inputs = bot_navigation_acceptance::physical_inputs();
            int open_ticks = 0, alive_ticks = 0, hits = 0, breaks = 0;
            for (int step = 0; step < 4800 && level->state() != LevelState::zoom_out; ++step)
            {
                const auto before = holder.primary_weapon().canopy_state();
                const bool was_alive = holder.state() == PlayerState::alive && holder.health() > 0.0f;
                const int active_before = bots.active_ticks;
                level->update(inputs, bot_navigation_acceptance::dt, &bots);
                if (active_before == bots.active_ticks || holder.state() != PlayerState::alive || holder.health() <= 0.0f)
                { continue; }
                const auto after = holder.primary_weapon().canopy_state();
                ++alive_ticks;
                if (after.open) { ++open_ticks; }
                // A death, reset or repair may obscure damage within one step.
                // These are observed shield hit-point losses and breaks.
                if (was_alive && before.hits_left > after.hits_left)
                {
                    hits += before.hits_left - after.hits_left;
                    if (after.recovery_remaining > 0.0f && before.recovery_remaining <= 0.0f) { ++breaks; }
                }
            }
            require(level->state() == LevelState::zoom_out && bots.active_ticks >= 3599 && bots.active_ticks <= 3601,
                "Umbrella balance round failed to reach its one-minute time-up");
            const auto end = level->level_end_info();
            const int other_seat = 1 - umbrella_seat;
            const auto own = static_cast<std::size_t>(paint_owners::index(holder.player_num(), umbrella_seat));
            const auto other = static_cast<std::size_t>(paint_owners::index(level->player(other_seat).player_num(), other_seat));
            std::cout << "CW-CANOPY stage=" << stage << " opponent=" << weapon_name(opponent)
                << " variant=" << match_variant_key(variant) << " sub=" << sub_name(umbrella_sub)
                << " seed=" << seed << " umbrella_seat=" << umbrella_seat + 1 << " ticks=" << bots.active_ticks
                << " paint=" << (umbrella_seat == 0 ? end.team_a_area : end.team_b_area)
                << " opposing_paint=" << (umbrella_seat == 0 ? end.team_b_area : end.team_a_area)
                << " paintable=" << end.total_area << " kills=" << end.splats.at(own) << " deaths=" << end.splats.at(other)
                << " open=" << open_ticks << " alive=" << alive_ticks << " hits=" << hits << " breaks=" << breaks << '\n';
        }
    }

    int run(int argc, char** argv)
    {
        require(argc == 9, "usage: umbrella <content> --balance <stage> <opponent> <variant> <umbrella sub> <first seed> <seed count>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::string stage(argv[3]);
        const auto stages = acceptance::manifest_entries(content, "level", ".json");
        require(std::any_of(stages.begin(), stages.end(), [&](const auto& entry) { return entry.name == stage; }),
            "Balance stage is absent from the manifest");
        const auto opponent = weapon_from_name(argv[4]);
        require(opponent && *opponent < WeaponType::umbrella, "Expected one of the original five primary tools");
        MatchVariant variant{};
        require(read_match_variant(argv[5], variant), "Unknown match variant");
        const auto sub = sub_from_name(argv[6]);
        require(sub == SubType::grenade || sub == SubType::wall, "Expected grenade or wall");
        const auto first = read_number(argv[7]), count = read_number(argv[8]);
        require(count > 0 && count <= 64 && first <= std::numeric_limits<unsigned int>::max() - (count - 1),
            "Expected 1 to 64 representable seeds");
        World world(content, "umbrella-balance", "Umbrella balance needs support '", stage.c_str());
        require(world.levels.get(stage)->include_in_random(), "Balance stage must be in Random");
        std::cout << std::setprecision(std::numeric_limits<float>::max_digits10);
        for (unsigned int i = 0; i < count; ++i)
        {
            play(world, stage, *opponent, *sub, variant, first + i, 0);
            play(world, stage, *opponent, *sub, variant, first + i, 1);
        }
        return 0;
    }
}
