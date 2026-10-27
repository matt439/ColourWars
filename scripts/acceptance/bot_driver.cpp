#include "game/pch.h"
#include "game/ai/match_bot.h"
#include "game/ai/match_bot_driver.h"
#include "game/ai/match_bot_ballistics.h"
#include "scripts/acceptance/bot_driver/ascent_checks.h"
#include "scripts/acceptance/bot_driver/ascent_approach_checks.h"
#include "scripts/acceptance/bot_driver/ramp_checks.h"
#include "engine/assets/json.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace mattmath;

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	bool same(const PlayerInputData& left, const PlayerInputData& right)
	{
		return left.x_movement == right.x_movement && left.jump_pressed == right.jump_pressed &&
			left.jump_held == right.jump_held && left.shoot_angle == right.shoot_angle &&
			left.shoot_direction == right.shoot_direction &&
			left.shoot_direction_requested == right.shoot_direction_requested &&
			left.primary_shoot == right.primary_shoot && left.drop_through == right.drop_through &&
			left.connected == right.connected && left.left_analog_stick == right.left_analog_stick &&
			left.right_analog_stick == right.right_analog_stick;
	}

	void ordinary(const PlayerInputData& input)
	{
		require(input.connected && !input.toggle_pause_menu && !input.pause_held &&
			!input.toggle_debug && !input.skip_intro, "A shipping bot emitted a menu/debug action or disconnected");
		require(std::isfinite(input.x_movement) && input.x_movement >= -1.0f && input.x_movement <= 1.0f &&
			std::isfinite(input.shoot_angle) && std::isfinite(input.shoot_direction.x) &&
			std::isfinite(input.shoot_direction.y), "A shipping bot emitted invalid ordinary input");
	}

	MatchBotObservation fixture(const GameTuning& tuning, WeaponType type = WeaponType::sprayer)
	{
		MatchBotObservation view;
		view.alive = view.grounded = true;
		view.size = tuning.player().size;
		view.ammo = 1.0f;
		view.engage_distance = 800.0f;
		view.weapon = type;
		view.obstacles.emplace_back(-2000.0f, view.size.y * 0.5f, 4000.0f, 100.0f);
		view.supports.push_back({ view.obstacles.back(), false });
		view.territory.push_back({ 300.0f, 50.0f });
		return view;
	}

	void check_reactions_and_reserves(const GameTuning& tuning)
	{
		for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
		{
			for (const WeaponType tool : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
				WeaponType::mister, WeaponType::bucket })
			{
				MatchBot bot(&tuning, difficulty, 23);
				MatchBotObservation view = fixture(tuning, tool);
				view.enemies.push_back({ { 100.0f, 0.0f }, {} });
				const int reaction = tuning.bot(difficulty).reaction_steps;
				for (int step = 0; step < reaction - 1; ++step)
				{
					const PlayerInputData input = bot.think(view); ordinary(input);
					require(!input.primary_shoot && !input.shoot_direction_requested && input.x_movement == 0.0f,
						"An initial reaction delay was bypassed");
				}
				PlayerInputData input = bot.think(view); ordinary(input);
				require(input.primary_shoot && input.shoot_direction_requested,
					"A living visible opponent did not receive an ordinary fire/aim decision after reaction");
				require(input.shoot_direction.x > 0.0f,
					"A nearby visible enemy received aim in the opposite horizontal direction");
				const WeaponTuning& record = tuning.weapon(tool);
				const Vector2F nominal = match_bot_ballistics::aim_direction(record,
					tuning.projectile(record.details.proj_type), view.velocity, { 100.0f, 0.0f }, {});
				require(std::acos(std::clamp(nominal.dot(input.shoot_direction), -1.0f, 1.0f)) <=
					tuning.bot(difficulty).aim_error_radians + 0.001f,
					"Combat prediction bypassed the selected difficulty's authored aim-error envelope");
				view.ammo = 0.0f;
				require(!bot.think(view).primary_shoot, "A bot kept firing between reactions after depletion");
				view.ammo = (tuning.bots().reserve_low + tuning.bots().reserve_resume) * 0.5f;
				for (int step = 0; step < reaction * 2; ++step)
				{
					require(!bot.think(view).primary_shoot, "A bot resumed firing before its reserve recovered");
				}
				view.ammo = 1.0f;
				view.shot_empty = true;
				require(!bot.think(view).primary_shoot, "A positive tank bypassed full-shot affordability");
				view.shot_empty = false;
				bool resumed = false;
				for (int step = 0; step <= reaction; ++step) { const bool firing = bot.think(view).primary_shoot; resumed = resumed || firing; }
				require(resumed, "A recovered bot never resumed firing");
				view.alive = false;
				input = bot.think(view); ordinary(input);
				require(!input.primary_shoot && !input.jump_held && !input.jump_pressed &&
					input.x_movement == 0.0f && !input.drop_through, "A dead bot kept a gameplay decision");
				view.alive = true;
				for (int step = 0; step < reaction - 1; ++step)
				{
					require(!bot.think(view).primary_shoot, "Respawn kept a pre-death reaction decision");
				}
				require(bot.think(view).primary_shoot, "Respawn failed to rebuild living decisions");
			}
		}
	}

	void check_recovery_and_territory(const GameTuning& tuning)
	{
		MatchBot bot(&tuning, BotDifficulty::normal, 19);
		MatchBotObservation view = fixture(tuning);
		bool jumped = false;
		bool turned = false;
		bool previous_hold = false;
		for (int step = 0; step < tuning.bots().stuck_turn_steps + 90; ++step)
		{
			const PlayerInputData input = bot.think(view); ordinary(input);
			require(input.jump_pressed == (input.jump_held && !previous_hold),
				"A held bot jump repeated its press edge");
			previous_hold = input.jump_held;
			jumped = jumped || input.jump_pressed;
			turned = turned || input.x_movement < 0.0f;
		}
		require(jumped && turned, "A blocked bot failed its jump then turn recovery");
		MatchBot drop(&tuning, BotDifficulty::hard, 32);
		view = fixture(tuning);
		view.on_platform = true;
		view.territory = { { 300.0f, view.size.y * 3.0f } };
		view.obstacles.clear(); view.supports.clear();
		view.obstacles.emplace_back(-2000.0f, view.size.y * 3.5f, 4000.0f, 100.0f);
		view.supports.push_back({ view.obstacles.back(), false });
		bool dropped = false;
		for (int step = 0; step < 12; ++step) { const bool dropping = drop.think(view).drop_through; dropped = dropped || dropping; }
		require(dropped, "A lower territory goal never requested an ordinary platform drop");
		MatchBot territory(&tuning, BotDifficulty::hard, 7);
		view = fixture(tuning);
		view.territory = { { 400.0f, 100.0f }, { -300.0f, 0.0f } };
		PlayerInputData decision;
		for (int step = 0; step < tuning.bot(BotDifficulty::hard).reaction_steps; ++step)
		{
			decision = territory.think(view);
		}
		require(decision.x_movement < 0.0f, "Territory choice ignored the cheaper visible same-floor goal");
	}

	void check_private_sequences(const GameTuning& tuning)
	{
		for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
		{
			MatchBot left(&tuning, difficulty, 901), right(&tuning, difficulty, 901),
				other(&tuning, difficulty, 902), interleaved(&tuning, BotDifficulty::hard, 77);
			MatchBotObservation view = fixture(tuning);
			view.enemies.push_back({ { 200.0f, -50.0f }, { 25.0f, 0.0f } });
			bool differs = false;
			std::srand(281);
			const int expected_random = std::rand();
			std::srand(281);
			for (int step = 0; step < 600; ++step)
			{
				view.velocity.x = step % 19 == 0 ? 0.0f : 50.0f;
				view.ammo = step % 100 < 20 ? 0.1f : 1.0f;
				const PlayerInputData first = left.think(view);
				(void)interleaved.think(view);
				const PlayerInputData second = right.think(view);
				ordinary(first); ordinary(second);
				require(same(first, second), "Equal private seeds diverged when another bot was interleaved");
				const PlayerInputData different_seed = other.think(view);
				differs = differs || !same(first, different_seed);
			}
			require(differs, "Different bot seeds never changed a decision sequence");
			require(std::rand() == expected_random, "Bot decisions consumed the game's global random stream");
		}
		MenuLevelSettings settings;
		settings.set_player_count(4); settings.begin_input_claims(0);
		settings.set_player_bot(1, BotDifficulty::hard); settings.set_player_bot(3, BotDifficulty::easy);
		MatchBotDriver driver(&tuning, settings, 12);
		require(driver.drives(1) && driver.drives(3) && !driver.drives(0) && !driver.drives(2) &&
			!driver.drives(-1) && !driver.drives(4), "Driver confused an unclaimed human with an explicit bot");
	}

	void check_visible_approach(const GameTuning& tuning)
	{
		for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
		{
			MatchBot bot(&tuning, difficulty, 7);
			MatchBotObservation view = fixture(tuning);
			view.engage_distance = 200.0f;
			view.territory = { { -300.0f, 0.0f } };
			view.enemies.push_back({ { 600.0f, 0.0f }, {} });
			PlayerInputData input;
			for (int step = 0; step < tuning.bot(difficulty).reaction_steps; ++step) { input = bot.think(view); }
			require(input.x_movement > 0.0f,
				"A visible opponent outside firing range never received an ordinary approach");
			view.enemies.clear();
			view.velocity.x = 120.0f;
			for (int step = 0; step < tuning.bot(difficulty).reaction_steps; ++step) { input = bot.think(view); }
			require(input.x_movement < 0.0f, "An unseen opponent retained a pursuit goal after reaction");
		}
	}

	PlayerInputData react(MatchBot& bot, const MatchBotObservation& view,
		const GameTuning& tuning, BotDifficulty difficulty)
	{
		PlayerInputData input;
		for (int step = 0; step < tuning.bot(difficulty).reaction_steps; ++step)
		{ input = bot.think(view); ordinary(input); }
		return input;
	}

	MatchBotObservation platform_fixture(const GameTuning& tuning, bool lower_support)
	{
		MatchBotObservation view = fixture(tuning);
		view.on_platform = true;
		view.territory.clear(); view.obstacles.clear(); view.supports.clear();
		view.obstacles.emplace_back(-1000.0f, view.size.y * 0.5f, 2000.0f, 10.0f);
		view.supports.push_back({ view.obstacles.back(), true });
		if (lower_support)
		{
			view.obstacles.emplace_back(-1000.0f, view.size.y * 3.5f, 2000.0f, 100.0f);
			view.supports.push_back({ view.obstacles.back(), false });
		}
		return view;
	}

	// A bot riding a moving paintable support holds there while unpainted top
	// remains in sight, for at most the authored hold per ride, then resumes
	// its ordinary descent. A static platform with the same paint drops at once.
	void check_mover_hold(const GameTuning& tuning)
	{
		const int hold = tuning.bots().mover_hold_steps;
		for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
		{
			for (const bool moving : { true, false })
			{
				MatchBot bot(&tuning, difficulty, 7);
				MatchBotObservation view = platform_fixture(tuning, true);
				view.supports.front().paintable = true;
				view.supports.front().id = 1;
				view.supports.front().moving = moving;
				view.supports.back().id = 2;
				view.support_open = true;
				view.support_paint = Vector2F(200.0f, view.size.y * 0.5f - 4.0f);
				view.support_bounds = view.supports.front().bounds;
				const int reaction = tuning.bot(difficulty).reaction_steps;
				int first_drop = -1;
				for (int step = 0; step < hold + reaction * 4 && first_drop < 0; ++step)
				{
					const PlayerInputData input = bot.think(view); ordinary(input);
					if (input.drop_through) { first_drop = step; }
					require(!input.jump_pressed, "A bot holding on a support jumped off it");
				}
				if (moving)
				{
					require(first_drop >= hold, "A bot left a moving support with unpainted top before its hold ended");
					require(first_drop >= 0, "A bot held on a moving support past its bounded hold");
				}
				else
				{
					require(first_drop >= 0 && first_drop < hold,
						"A static platform's top paint held a bot like a moving support");
				}
			}
		}
	}

	// A refilling bot backs away only from an enemy within that enemy's own
	// reach plus the authored margin. A farther one leaves its goal in place.
	void check_refill_retreat(const GameTuning& tuning)
	{
		const float reach = 400.0f;
		const float threat = reach + tuning.bots().refill_retreat_margin;
		for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
		{
			for (const bool close : { true, false })
			{
				MatchBot bot(&tuning, difficulty, 7);
				MatchBotObservation view = fixture(tuning);
				view.ammo = tuning.bots().reserve_low * 0.5f;
				view.engage_distance = 200.0f;
				const float x = close ? threat - 50.0f : threat + 400.0f;
				view.enemies.push_back({ { x, 0.0f }, {}, reach });
				const PlayerInputData input = react(bot, view, tuning, difficulty);
				require(!input.primary_shoot, "A refilling bot fired before reaching its resume reserve");
				require(close ? input.x_movement < 0.0f : input.x_movement > 0.0f, close ?
					"A refilling bot did not back away from an enemy within that enemy's reach" :
					"A refilling bot retreated from an enemy well outside that enemy's reach");
			}
		}
	}

	void check_standing_and_descent(const GameTuning& tuning)
	{
		for (const BotDifficulty difficulty : { BotDifficulty::easy, BotDifficulty::normal, BotDifficulty::hard })
		{
			for (const float face_y : { -tuning.player().size.y, tuning.player().size.y })
			{
				MatchBot bot(&tuning, difficulty, 7);
				MatchBotObservation view = fixture(tuning);
				const Vector2F face(300.0f, face_y);
				view.territory = { face };
				const PlayerInputData input = react(bot, view, tuning, difficulty);
				require(input.x_movement > 0.0f && !input.jump_pressed && !input.jump_held && !input.drop_through,
					"A high/low paint aim point changed a clear same-floor standing route into a jump/drop");
				require(input.shoot_direction_requested &&
					std::fabs(input.shoot_angle - face.angle()) <= tuning.bot(difficulty).aim_error_radians + 0.001f,
					"Territory aim followed the standing center instead of the sampled paint face");
			}

			MatchBot distant(&tuning, difficulty, 7);
			MatchBotObservation distant_view = fixture(tuning);
			distant_view.engage_distance = 200.0f;
			const Vector2F distant_face(600.0f, -distant_view.size.y * 2.0f);
			distant_view.territory = { distant_face };
			const PlayerInputData distant_input = react(distant, distant_view, tuning, difficulty);
			require(distant_input.x_movement > 0.0f && !distant_input.jump_pressed && !distant_input.jump_held &&
				!distant_input.drop_through, "An out-of-range paint face stopped a valid ordinary movement candidate");
			require(distant_input.shoot_direction_requested &&
				std::fabs(distant_input.shoot_angle - distant_face.angle()) >
					tuning.bot(difficulty).aim_error_radians + 0.001f,
				"An out-of-range high paint face took aim away from local fallback painting");

			MatchBot unknown(&tuning, difficulty, 7);
			MatchBotObservation view = platform_fixture(tuning, false);
			const int window = tuning.bot(difficulty).replan_steps + tuning.bot(difficulty).reaction_steps;
			for (int step = 0; step < window; ++step)
			{
				const PlayerInputData input = unknown.think(view); ordinary(input);
				require(!input.drop_through, "A platform bot requested an exploration drop without a known lower support");
			}

			MatchBot safe(&tuning, difficulty, 7);
			view = platform_fixture(tuning, true);
			PlayerInputData input = react(safe, view, tuning, difficulty);
			require(input.drop_through && !input.jump_pressed && !input.jump_held,
				"A known clear lower support did not produce a deliberate drop without a simultaneous jump");

			// These seeds propose opposite headings at the middle of the same
			// platform. Moving that fixture to its outward edge must reverse the
			// descent instead of clamping its destination against the boundary.
			for (const bool left_edge : { true, false })
			{
				const std::uint64_t seed = left_edge ? 7 : 32;
				const float inward = left_edge ? 1.0f : -1.0f;
				MatchBot middle(&tuning, difficulty, seed);
				view = platform_fixture(tuning, true);
				input = react(middle, view, tuning, difficulty);
				require(input.drop_through && input.x_movement * inward < 0.0f,
					"Edge-descent control did not first propose the seeded outward heading");
				MatchBot edge(&tuning, difficulty, seed);
				const RectangleF& lower = view.supports.back().bounds;
				view.center.x = left_edge ? lower.left() + view.size.x : lower.right() - view.size.x;
				input = react(edge, view, tuning, difficulty);
				require(input.drop_through && input.x_movement * inward > 0.0f &&
					!input.jump_pressed && !input.jump_held,
					"A seeded outward descent near the landing-support edge did not turn inward");
			}

			// Warm up a held jump against a local obstruction, then reveal a clear
			// lower route. Positional progress prevents this from testing recovery.
			MatchBot interrupted(&tuning, difficulty, 7);
			view = platform_fixture(tuning, false);
			view.obstacles.emplace_back(-65.0f, -10.0f, 25.0f, 20.0f);
			view.obstacles.emplace_back(40.0f, -10.0f, 25.0f, 20.0f);
			input = react(interrupted, view, tuning, difficulty);
			require(input.jump_pressed && input.jump_held, "Descent interruption fixture did not first hold a jump");
			view = platform_fixture(tuning, true);
			bool dropped = false;
			for (int step = 0; step < window; ++step)
			{
				view.center.x += 4.0f; view.velocity.x = 240.0f;
				input = interrupted.think(view); ordinary(input);
				if (input.drop_through)
				{
					require(!input.jump_pressed && !input.jump_held,
						"A deliberate descent inherited a held or newly pressed jump");
					dropped = true; break;
				}
			}
			require(dropped, "A bot never replanned from its earlier jump to a known clear lower route");

			MatchBot blocked(&tuning, difficulty, 7);
			view = platform_fixture(tuning, true);
			// A ramp or rotated slab keeps an obstacle AABB without advertising
			// a flat top support. Its body-expanded bound still blocks descent.
			view.obstacles.emplace_back(-1000.0f, view.size.y * 1.5f, 2000.0f, 20.0f);
			input = react(blocked, view, tuning, difficulty);
			require(!input.drop_through, "A lower endpoint behind a solid non-support obstacle was treated as a clear drop");

			MatchBot covered(&tuning, difficulty, 7);
			view = platform_fixture(tuning, true);
			view.obstacles.emplace_back(-1000.0f, view.size.y * 3.0f, 2000.0f, view.size.y);
			input = react(covered, view, tuning, difficulty);
			require(!input.drop_through, "A lower landing without body clearance was accepted as a standing endpoint");
		}
	}

	void check_tuning(const std::filesystem::path& content)
	{
		std::ifstream input(content / "tuning.json", std::ios::binary);
		const std::string source{ std::istreambuf_iterator<char>(input), {} };
		const auto rejects = [&](const char* expected, const auto& edit)
		{
			rapidjson::Document document; document.Parse(source.c_str());
			require(!document.HasParseError(), "Cannot parse bot tuning source fixture");
			edit(document);
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer); document.Accept(writer);
			bool rejected = false;
			try { const GameTuning invalid(labrador::parse_json(buffer.GetString(), "bot-policy-invalid")); }
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				rejected = message.find("bot-policy-invalid") != std::string::npos &&
					message.find(expected) != std::string::npos;
			}
			require(rejected, "Bot tuning accepted malformed data or lost source/field context");
		};
		rejects("bots", [](auto& d) { d.RemoveMember("bots"); });
		rejects("sight_distance", [](auto& d) { d["bots"]["sight_distance"].SetInt(0); });
		rejects("reserve_resume", [](auto& d) { d["bots"]["reserve_resume"].SetDouble(0.01); });
		rejects("stuck_turn_steps", [](auto& d) { d["bots"]["stuck_turn_steps"].SetInt(1); });
		for (const char* field : { "recovery_steps", "progress_window_steps", "progress_distance",
			"finish_dwell_steps", "deliberate_shot_interval", "refill_retreat_margin", "mover_hold_steps" })
		{
			rejects(field, [&](auto& d) { d["bots"].RemoveMember(field); });
			rejects(field, [&](auto& d) { d["bots"][field].SetInt(0); });
		}
		for (const char* difficulty : { "easy", "normal", "hard" })
		{
			rejects(difficulty, [&](auto& d) { d["bots"].RemoveMember(difficulty); });
			for (const char* field : { "reaction_steps", "replan_steps", "territory_candidates" })
			{
				rejects(field, [&](auto& d) { d["bots"][difficulty][field].SetInt(0); });
				rejects(field, [&](auto& d) { d["bots"][difficulty][field].SetDouble(1.5); });
			}
			rejects("territory_candidates", [&](auto& d) { d["bots"][difficulty]["territory_candidates"].SetInt(257); });
			rejects("aim_error_radians", [&](auto& d) { d["bots"][difficulty]["aim_error_radians"].SetDouble(-0.1); });
		}
	}
}

int wmain(int argc, wchar_t** argv)
{
	try
	{
		require(argc == 2, "Usage: bot_driver <staged-content-directory>");
		const std::filesystem::path content(argv[1]);
		const GameTuning tuning((content / "tuning.json").string().c_str());
		check_tuning(content);
		check_reactions_and_reserves(tuning);
		check_recovery_and_territory(tuning);
		check_private_sequences(tuning);
		check_visible_approach(tuning);
		check_refill_retreat(tuning);
		check_mover_hold(tuning);
		check_standing_and_descent(tuning);
		bot_ascent_acceptance::check(tuning);
		bot_ascent_approach_acceptance::check(tuning);
		bot_ramp_acceptance::check(tuning);
		std::cout << "PASS shipping bot policy: all difficulties/five tools reaction, accuracy, ordinary inputs, reserve hysteresis/full-shot gate, refill retreat only within an enemy's reach, bounded moving-support paint hold, death/respawn, jump/turn/drop and territory decisions, separate paint aim/standing targets, safe known-support descent, private seeded streams and contextual tuning rejection\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL " << error.what() << '\n';
		return 1;
	}
}
