#include "game/pch.h"
#include "scripts/acceptance/skirmish_bot/fixtures.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot.h"
#include "game/debug/replay.h"
#include <cmath>
#include <cstdlib>
#include <vector>

using namespace mattmath;

namespace skirmish_acceptance
{
	namespace
	{
		BotSeatView base_view()
		{
			BotSeatView view;
			view.player_height = 120.0f;
			view.engage_distance = 600.0f;
			view.self.team = PlayerTeam::a;
			view.self.center = { 0, 0 };
			view.self.velocity = { 300, 0 };
			view.self.on_ground = true;
			return view;
		}

		BotSeatView::Seat enemy(float x, float y = 0.0f)
		{
			BotSeatView::Seat seat;
			seat.team = PlayerTeam::b;
			seat.center = { x, y };
			return seat;
		}

		void sane(const PlayerInputData& input)
		{
			require(input.connected && !input.toggle_debug && !input.drop_through,
				"Skirmish opened UI, disconnected or requested an unrelated action");
			require(input.x_movement >= -1.0f && input.x_movement <= 1.0f &&
				input.left_analog_stick.x == input.x_movement && input.left_analog_stick.y == 0.0f,
				"Skirmish movement is not a consistent input request");
			require(!input.shoot_direction_requested ||
				std::fabs(input.shoot_direction.length() - 1.0f) < 1e-4f,
				"Skirmish aim is not normalized");
		}

		std::vector<std::string> sequence(std::uint64_t seed, int seat)
		{
			auto bot = Bot::make("skirmish", seed, seat);
			require(bot != nullptr && bot->name() == "skirmish", "Skirmish factory/word registration failed");
			std::vector<std::string> result;
			bool held = false;
			for (int step = 0; step < 1800; ++step)
			{
				BotSeatView view = base_view();
				view.step = step;
				view.self.center = { static_cast<float>(step % 300), 700.0f };
				view.self.velocity.x = step % 80 < 20 ? 0.0f : 300.0f;
				view.self.on_ground = step % 47 < 30;
				view.self.ammo = static_cast<float>(100 - step % 100) / 100.0f;
				view.shot_empty = step % 100 > 75;
				view.self.alive = step % 240 < 220;
				if (step % 200 < 90) { view.others.push_back(enemy(view.self.center.x + 450.0f, 650.0f)); }
				if (step % 200 >= 150) { view.others.push_back(enemy(view.self.center.x - 150.0f, 450.0f)); }
				const PlayerInputData input = bot->think(view);
				sane(input);
				require(!input.jump_pressed || (input.jump_held && !held),
					"Skirmish repeated a jump press without releasing the hold");
				held = input.jump_held;
				result.push_back(replay_codec::encode(input));
			}
			return result;
		}
	}

	void check_policy()
	{
		require(Bot::behaviours() == std::vector<std::string>{
			"idle", "walk", "jump", "chase", "paint", "skirmish" } &&
			Bot::is_behaviour("skirmish") && !Bot::is_behaviour("Skirmish") &&
			Bot::make("idle", 7, 0) == nullptr, "Bot words/order/idle semantics changed");
		BotSeatView view = base_view();
		auto bot = Bot::make("skirmish", 1, 0);
		PlayerInputData input = bot->think(view);
		sane(input);
		require(input.primary_shoot && input.shoot_direction.y > 0.0f,
			"Uncontested skirmish did not paint the floor while roaming");

		view.others = { enemy(450) };
		bot = Bot::make("skirmish", 1, 0);
		input = bot->think(view);
		require(input.primary_shoot && input.x_movement == 1.0f &&
			input.shoot_direction == Vector2F(1, 0), "In-range enemy did not trigger approach/engagement");
		view.others = { enemy(1000) };
		bot = Bot::make("skirmish", 1, 0);
		input = bot->think(view);
		require(input.primary_shoot && input.shoot_direction.y > 0.0f,
			"Out-of-range enemy stopped the painting policy");

		// Friends, neutral teams, health-zero and dead seats cannot take target priority.
		BotSeatView::Seat friendly = enemy(20); friendly.team = PlayerTeam::a;
		BotSeatView::Seat neutral = enemy(15); neutral.team = PlayerTeam::none;
		BotSeatView::Seat dead = enemy(10); dead.alive = false;
		BotSeatView::Seat terminal = enemy(5); terminal.health = 0.0f;
		view.others = { friendly, neutral, dead, terminal, enemy(-450), enemy(450) };
		bot = Bot::make("skirmish", 1, 0);
		input = bot->think(view);
		require(input.shoot_direction == Vector2F(-1, 0) && input.x_movement == -1.0f,
			"Nearest-opponent selection or stable view-order tie break failed");
		view.others = { friendly, neutral, dead, terminal };
		bot = Bot::make("skirmish", 1, 0);
		input = bot->think(view);
		require(input.shoot_direction.y > 0.0f, "A friendly/neutral/dead target was engaged");

		// Positive ammo insufficient for one complete shot must trigger retreat.
		view = base_view(); view.others = { enemy(450) };
		view.self.ammo = 0.4f; view.shot_empty = true;
		bot = Bot::make("skirmish", 2, 0);
		input = bot->think(view);
		require(!input.primary_shoot && input.x_movement == -1.0f,
			"Actual full-shot-empty state did not withdraw with positive ammunition");
		view.self.center = { -300, 0 };
		view.self.ammo = 0.6f; view.shot_empty = false;
		input = bot->think(view);
		require(!input.primary_shoot && input.x_movement == -1.0f,
			"Refill hysteresis ended at a small affordable remainder");
		view.self.ammo = 0.9f; view.shot_empty = true;
		input = bot->think(view);
		require(!input.primary_shoot && input.x_movement == -1.0f,
			"Refill ended before the real full-shot gate recovered");
		view.self.ammo = 0.8f; view.shot_empty = false;
		input = bot->think(view);
		require(!input.primary_shoot && input.x_movement == 1.0f,
			"Refilled bot did not return while conserving ammunition");
		view.self.center = { 0, 0 };
		input = bot->think(view);
		require(input.primary_shoot && input.shoot_direction == Vector2F(1, 0),
			"Refilled bot did not re-engage on returning to the fight");

		// A targetless refill returns to its old paint location, then paints again.
		view = base_view(); view.self.ammo = 0.1f;
		bot = Bot::make("skirmish", 3, 0);
		require(!bot->think(view).primary_shoot, "Low reserve did not start targetless refill");
		view.self.center = { -300, 0 }; view.self.ammo = 1.0f;
		input = bot->think(view);
		require(!input.primary_shoot && input.x_movement == 1.0f,
			"Targetless refill lost its prior paint location");
		view.self.center = { 0, 0 };
		input = bot->think(view);
		require(input.primary_shoot && input.shoot_direction.y > 0.0f,
			"Return to a paint location did not resume painting");

		// Death can precede the next PlayerState transition. Release jump and
		// discard the previous life's refill/return state in both representations.
		view = base_view(); view.others = { enemy(100, -300) };
		bot = Bot::make("skirmish", 4, 0);
		input = bot->think(view);
		require(input.jump_pressed && input.jump_held, "Fixture did not begin a real held jump");
		view.self.ammo = 0.1f; require(!bot->think(view).primary_shoot, "Fixture did not enter refill");
		view.self.health = 0.0f;
		for (int step = 0; step < 3; ++step)
		{
			input = bot->think(view); sane(input);
			require(input.x_movement == 0.0f && !input.primary_shoot && !input.jump_held &&
				!input.jump_pressed && !input.shoot_direction_requested,
				"Terminal/dead skirmish requested an action");
			view.self.alive = false;
		}
		view = base_view(); view.self.center = { 1000, 0 };
		input = bot->think(view);
		require(input.primary_shoot && !input.jump_held && !input.jump_pressed,
			"Respawn retained a held jump or the previous life's refill/return goal");
	}

	void check_sequences()
	{
		const auto first = sequence(7, 0);
		require(sequence(7, 0) == first, "Same skirmish seed/view sequence did not reproduce all input bytes");
		require(sequence(8, 0) != first && sequence(7, 1) != first,
			"Skirmish private seed/seat streams were not independent");
		std::srand(4291); const int expected = std::rand(); std::srand(4291);
		require(sequence(19, 0).size() == 1800 && std::rand() == expected,
			"Skirmish creation/thinking consumed the gameplay RNG");

		// The optional observation fields must not influence established bots.
		// Their actual pre-existing bodies are also pinned unchanged in source review.
		for (const char* word : { "walk", "jump", "chase", "paint" })
		{
			auto legacy = Bot::make(word, 7, 0);
			auto observed = Bot::make(word, 7, 0);
			for (int step = 0; step < 600; ++step)
			{
				BotSeatView view = base_view();
				view.step = step; view.self.ammo = static_cast<float>(100 - step % 100) / 100.0f;
				view.self.velocity.x = step % 70 < 20 ? 0.0f : 300.0f;
				view.others = { enemy(450, step % 80 < 40 ? -240.0f : 0.0f) };
				BotSeatView extra = view;
				view.shot_empty = false; view.engage_distance = 0.0f;
				extra.shot_empty = step % 2 == 0; extra.engage_distance = static_cast<float>(step * 10);
				require(replay_codec::same(legacy->think(view), observed->think(extra)),
					std::string(word) + " began consuming skirmish-only tool observations");
			}
		}
	}
}
#endif
