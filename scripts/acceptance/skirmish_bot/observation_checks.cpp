#include "game/pch.h"
#include "scripts/acceptance/skirmish_bot/fixtures.h"
#if COLOURWARS_DEV_TOOLS
#include "game/debug/bot_driver.h"
#include "game/debug/match_fingerprint.h"
#include "game/debug/replay.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/weapon.h"
#include "engine/assets/json.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/core/step_timer.h"
#include "engine/render/sprite_font_file.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>

using namespace labrador;
using namespace mattmath;

namespace skirmish_acceptance
{
	namespace
	{
		class CpuWorld final
		{
		public:
			explicit CpuWorld(const std::filesystem::path& content) :
				tuning(path_to_utf8(content / "tuning.json").c_str()),
				presentation(path_to_utf8(content / "presentation.json").c_str())
			{
				const JsonDocument manifest = read_json_file(path_to_utf8(content / "manifest.json").c_str());
				const JsonValue assets = manifest.root().array("assets");
				for (std::size_t index = 0; index < assets.size(); ++index)
				{
					const JsonValue asset = assets.at(index);
					const std::string kind = asset.identifier("kind");
					const std::filesystem::path directory = content / path_from_utf8(asset.identifier("directory"));
					const JsonValue names = asset.array("names");
					for (std::size_t entry = 0; entry < names.size(); ++entry)
					{
						const std::string name = names.at(entry).as_identifier();
						if (kind == "sprite_sheet")
						{
							render.add_sprite_sheet(name, read_sprite_sheet(
								path_to_utf8(directory / path_from_utf8(name + ".json")).c_str(), TextureHandle{}));
						}
						else if (kind == "font")
						{
							const SpriteFontFile file = read_sprite_font_file(
								path_to_utf8(directory / path_from_utf8(name + ".spritefont")));
							auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
							if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
							render.add_font(name, std::move(loaded));
						}
						else if (kind == "sound_bank") { audio.add_sound_bank(name, SoundBank::silent()); }
						else if (kind == "level")
						{
							auto info = std::make_unique<LevelLoadedInfo>(
								path_to_utf8(directory / path_from_utf8(name + ".json")).c_str());
							info->validate(); levels.add(name, std::move(info));
						}
						else { throw std::runtime_error("Unsupported CPU fixture manifest kind: " + kind); }
					}
				}
				builder = std::make_unique<LevelBuilder>(&viewports, &render, &audio, &preferences,
					&levels, &tuning, &presentation, &resolution, nullptr, &partitioner, &mouse);
			}

			std::unique_ptr<Level> build(WeaponType tool)
			{
				MenuLevelSettings settings;
				settings.set_stage("crossfeed"); settings.set_player_count(2);
				settings.set_screen_layout(ScreenLayout::two_player); settings.set_duration(60.0f);
				for (int view = 0; view < 2; ++view)
				{
					settings.set_player_team(view, view == 0 ? PlayerTeam::a : PlayerTeam::b);
					settings.set_player_weapon(view, tool);
				}
				std::srand(741);
				viewports.set_layout(settings.screen_layout());
				return builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
			}

			GameTuning tuning;
			Presentation presentation;
			RenderResources render;
			AudioResources audio;
			PlaySettings preferences;
			Registry<LevelLoadedInfo> levels{ "Level" };
			ResolutionManager resolution;
			ViewportManager viewports{ &resolution };
			Mouse mouse;
			Partitioner partitioner;
			std::unique_ptr<LevelBuilder> builder;
		};

		bool same_seat(const BotSeatView::Seat& a, const BotSeatView::Seat& b)
		{
			return a.center == b.center && a.velocity == b.velocity && a.team == b.team &&
				a.alive == b.alive && a.on_ground == b.on_ground && a.facing_right == b.facing_right &&
				a.health == b.health && a.ammo == b.ammo;
		}

		class FireSeat final : public SeatInputFilter
		{
		public:
			void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
			{
				for (auto& input : seats) { input = Bot::neutral(); }
				if (!seats.empty())
				{
					seats[0].primary_shoot = true;
					seats[0].shoot_direction_requested = true;
					seats[0].shoot_direction = { 0, -1 };
					seats[0].shoot_angle = -1.57079632679f;
					seats[0].right_analog_stick = seats[0].shoot_direction;
				}
			}
		};
	}

	void check_actual_observations(const std::filesystem::path& content)
	{
		CpuWorld world(content);
		const std::array tools{ WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
			WeaponType::mister, WeaponType::bucket };
		std::array<float, 5> reach{};
		for (std::size_t index = 0; index < tools.size(); ++index)
		{
			auto level = world.build(tools[index]);
			const MatchFingerprint before = MatchFingerprint::of(*level, 0);
			std::srand(1947); const int expected_random = std::rand(); std::srand(1947);
			for (int view = 0; view < level->player_count(); ++view)
			{
				const BotSeatView old = BotDriver::view_of(*level, view, 17, world.tuning);
				const BotSeatView observed = BotDriver::view_of(*level, view, 17, world.tuning, true);
				require(old.step == observed.step && old.player_height == observed.player_height &&
					same_seat(old.self, observed.self) && old.others.size() == observed.others.size() &&
					old.others.size() == 1 && same_seat(old.others.front(), observed.others.front()),
					"Opt-in tool observations changed the established public seat view");
				require(!old.shot_empty && old.engage_distance == 0.0f,
					"Legacy observation path began populating skirmish-only data");
				require(observed.shot_empty ==
					(level->player(view).primary_weapon().readiness() == WeaponReadiness::empty),
					"Skirmish observation does not use actual full-shot readiness");
				require(std::isfinite(observed.engage_distance) && observed.engage_distance > 0.0f,
					"Tool-specific engagement heuristic is not finite and positive");
				if (view == 0) { reach[index] = observed.engage_distance; }
			}
			require(std::rand() == expected_random &&
				MatchFingerprint::difference(before, MatchFingerprint::of(*level, 0)).empty(),
				"Read-only tool observations changed match state or gameplay RNG");

			// Only the new behavior opts in. Other seats retain their input.
			for (const char* word : { "paint", "skirmish" })
			{
				BotDriver driver(&world.tuning, 7);
				driver.assign(0, word);
				auto direct = Bot::make(word, 7, 0);
				std::vector<PlayerInputData> inputs(2);
				inputs[1].connected = true; inputs[1].x_movement = 0.375f;
				const PlayerInputData untouched = inputs[1];
				driver.filter_seats(inputs, *level);
				const PlayerInputData expected = direct->think(
					BotDriver::view_of(*level, 0, 0, world.tuning, std::string_view(word) == "skirmish"));
				require(replay_codec::same(inputs[0], expected) && replay_codec::same(inputs[1], untouched),
					"BotDriver failed its behavior-specific observation/seat isolation seam");
			}
		}
		require(reach[1] > reach[0] && reach[0] > reach[4] && reach[4] > reach[3] && reach[3] > reach[2],
			"Current five-tool data did not produce distinct expected engagement heuristics");

		// Exhaust a real Sniper to a positive remainder below its full-shot cost.
		// No physical device, debug ammo setter or guessed normalized threshold.
		auto level = world.build(WeaponType::sniper);
		const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::TicksPerSecond / 60));
		const std::vector<PlayerInputData> slots(input_slots::count);
		for (int tick = 0; tick < 1800 && level->state() != LevelState::active; ++tick) { level->update(slots, dt); }
		require(level->state() == LevelState::active, "Sniper observation fixture missed active state");
		FireSeat fire;
		bool observed_positive_empty = false;
		for (int tick = 0; tick < 900; ++tick)
		{
			level->update(slots, dt, &fire);
			const Player& player = level->player(0);
			if (player.weapon_ammo() > 0.0f && player.primary_weapon().readiness() == WeaponReadiness::empty)
			{
				const BotSeatView observed = BotDriver::view_of(*level, 0, tick, world.tuning, true);
				require(observed.self.ammo > 0.0f && observed.shot_empty,
					"A real positive-but-unaffordable Sniper remainder was hidden");
				auto bot = Bot::make("skirmish", 13, 0);
				require(!bot->think(observed).primary_shoot,
					"Real positive-but-unaffordable Sniper observation did not start refill");
				observed_positive_empty = true;
				break;
			}
		}
		require(observed_positive_empty, "Fixture never observed a paid real Sniper reaching a positive empty remainder");
	}
}
#endif
