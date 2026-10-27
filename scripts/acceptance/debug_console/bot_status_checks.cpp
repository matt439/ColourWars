#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include <array>
#include <iostream>

namespace debug_console_acceptance
{
	void check_bot_status_labels(GameTuning& tuning, Presentation& presentation)
	{
		constexpr std::array difficulties{ BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard };
		constexpr std::array names{ "easy", "normal", "hard" };
		for (std::size_t index = 0; index < difficulties.size(); ++index)
		{
			TestDirector test(tuning, presentation);
			FakeMatch match(test.director);
			match.settings = planned(4);
			match.settings.set_player_bot(1, difficulties[index]);
			require(match.settings.claim_input_slot(2), "Could not claim sparse P3 identity device");
			// P4 is still an unclaimed human. Both this dummy and P2 have
			// num == -1; only the explicit controller identifies the bot.
			test.director.attach_match(match);
			require(test.impl.execute("status").ok, "Could not request bot identity status");
			const auto contains = [&](const std::string& words)
			{
				return log_contains(test.impl.log(), words, DebugSeverity::info);
			};
			require(contains("  P1 kbm team ") &&
				contains(std::string("  P2 bot ") + names[index] + " team ") &&
				contains("  P3 pad3 team ") && contains("  P4 idle team "),
				"Status must preserve stable roster numbers and distinguish a bot's difficulty from a human dummy");
			require(test.impl.execute("overlay paint").ok, "Could not enable the bot identity paint overlay");
			const std::vector<std::string> paint = test.impl.paint_lines();
			require(paint.size() == 2 && paint[1] ==
				std::string("P1 kbm 30.0%   P2 bot ") + names[index] +
				" 0.0%   P3 pad3 0.0%   P4 idle 0.0%",
				"Paint overlay must name explicit bot difficulty while retaining human device identity");
			require(match.settings.player_settings()[1].num == -1 &&
				match.settings.player_settings()[3].controller == PlayerController::human &&
				match.settings.player_settings()[3].num == -1 && match.rebuilds == 0 && match.updates == 0,
				"Reporting bot identity must not claim devices, change the roster or drive gameplay");
			test.director.detach_match(match);
		}
		std::cout << "PASS explicit bot difficulty in Debug status and paint overlays; sparse human devices and human dummies retain identity\n";
	}
}
#endif
