#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/weapon_builder.h"
#include "game/objects/structure.h"
#include "engine/assets/sound_bank_loader.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/null/recording.h"
#include "engine/collision/partitioner.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace labrador;

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	bool close_to(float a, float b)
	{
		return std::abs(a - b) < 0.0001f;
	}

	const float weapon_step = 1.0f / 60.0f;

	auto update_weapon(Weapon& weapon, bool held, float dt = weapon_step)
	{
		PlayerInputData input;
		input.primary_shoot = held;
		return weapon.update_and_get_projectiles(input, mattmath::Vector2F::ZERO,
			mattmath::Vector2F::ZERO, true, dt);
	}

	// Content variants keep real factory weapons and their paid-shot workflow.
	// Only authored numbers/names change, in a reserved temporary file.
	class ContentVariant
	{
	public:
		explicit ContentVariant(const std::filesystem::path& original)
		{
			std::ifstream stream(original);
			require(stream.good(), "Cannot read audio acceptance tuning");
			this->source_ = std::string(std::istreambuf_iterator<char>(stream), {});
			wchar_t name[MAX_PATH]{};
			const auto temporary = std::filesystem::temp_directory_path();
			require(GetTempFileNameW(temporary.c_str(), L"cwa", 0, name) != 0,
				"Cannot reserve audio acceptance tuning file");
			this->path_ = name;
		}

		~ContentVariant()
		{
			std::error_code ignored;
			std::filesystem::remove(this->path_, ignored);
		}

		template<typename Edit>
		GameTuning tuning(Edit edit) const
		{
			rapidjson::Document json;
			json.Parse(this->source_.c_str());
			require(!json.HasParseError(), "Cannot parse audio acceptance tuning");
			edit(json);
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			json.Accept(writer);
			std::ofstream stream(this->path_, std::ios::binary | std::ios::trunc);
			stream.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
			stream.close();
			require(stream.good(), "Cannot write audio acceptance tuning variant");
			return GameTuning(this->path_.string().c_str());
		}

	private:
		std::string source_;
		std::filesystem::path path_;
	};

	// The normal game objects, with the engine's recording audio backend.
	// The bank definition is real: an unresolvable shipped cue fails here.
	struct World
	{
		AudioDevice device;
		RenderResources render;
		AudioResources audio;
		PlaySettings preferences;
		GameTuning tuning;
		Presentation presentation;
		Registry<LevelLoadedInfo> levels{ "Level" };
		ResolutionManager resolution;
		ViewportManager viewports{ &resolution };
		Mouse mouse;
		Partitioner partitioner;
		std::map<std::string, AudioDevice::VoiceHandle> voices;
		std::map<std::string, int> waves;
		std::unique_ptr<LevelBuilder> builder;
		std::unique_ptr<Level> level;
		std::vector<PlayerInputData> idle{ input_slots::count };
		size_t begin = 0;

		explicit World(const std::filesystem::path& content) :
			tuning((content / "tuning.json").string().c_str()),
			presentation((content / "presentation.json").string().c_str())
		{
			for (const char* sheet : { "sprite_sheet_1", "menu_paint", "weapon_paint", "workshop_kit" })
			{
				this->render.add_sprite_sheet(sheet, read_sprite_sheet(
					(content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
			}
			for (const char* font : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
				"gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
				"gill_sans_mt_bold_144", "courier_new_bold_16" })
			{
				const SpriteFontFile file = read_sprite_font_file(
					(content / "fonts" / (std::string(font) + ".spritefont")).string());
				auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
				if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
				this->render.add_font(font, std::move(loaded));
			}

			const SoundBankDefinition definition = read_sound_bank_definition(
				(content / "sounds/paint_exe.json").string().c_str());
			const auto bank = this->device.open_wave_bank("", "paint_exe", definition.waves);
			for (const auto& wave : definition.waves)
			{
				this->waves.emplace(wave, this->device.wave_index(bank, wave));
			}
			Registry<SoundBank::SoundEffect> effects("SoundEffect");
			for (const auto& effect : definition.effects)
			{
				const auto voice = this->device.create_voice(bank, this->device.wave_index(bank, effect.wave));
				this->voices.emplace(effect.name, voice);
				effects.add(effect.name, std::make_unique<SoundBank::SoundEffect>(voice));
			}
			this->audio.add_sound_bank("paint_exe", std::make_unique<SoundBank>(
				&this->device, bank, std::move(effects)));
			this->tuning.validate_resources(this->render, this->audio);
			auto info = std::make_unique<LevelLoadedInfo>((content / "levels/close_quarters.json").string().c_str());
			info->validate();
			this->levels.add("close_quarters", std::move(info));
			this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render, &this->audio,
				&this->preferences, &this->levels, &this->tuning, &this->presentation, &this->resolution,
				nullptr, &this->partitioner, &this->mouse);
		}

		void build(OpeningMode opening, int players = 2)
		{
			this->level.reset();
			this->begin = recorded_sounds(this->device).size();
			MenuLevelSettings settings;
			settings.set_stage("close_quarters");
			settings.set_player_count(players);
			settings.set_player_team(0, PlayerTeam::a);
			if (players == 2) { settings.set_player_team(1, PlayerTeam::b); }
			settings.begin_input_claims(input_slots::keyboard_and_mouse);
			settings.set_screen_layout(players == 1 ? ScreenLayout::one_player : ScreenLayout::two_player);
			settings.set_duration(0.25f);
			this->viewports.set_layout(settings.screen_layout());
			this->level = this->builder->build_level(settings, TeamColourPair::blue_orange, opening);
		}

		int plays(const std::string& name) const
		{
			const auto voice = this->voices.at(name);
			int count = 0;
			const auto& calls = recorded_sounds(this->device);
			for (size_t i = this->begin; i < calls.size(); ++i)
			{
				count += calls[i].call == SoundCall::play_voice && calls[i].voice == voice ? 1 : 0;
			}
			return count;
		}

		int wave_plays(const std::string& name) const
		{
			int count = 0;
			const auto& calls = recorded_sounds(this->device);
			for (size_t i = this->begin; i < calls.size(); ++i)
			{
				count += calls[i].call == SoundCall::play_wave && calls[i].wave == this->waves.at(name) ? 1 : 0;
			}
			return count;
		}

		void wave_volume_is(const SoundTuning& sound, float gain) const
		{
			bool found = false;
			const auto& calls = recorded_sounds(this->device);
			for (size_t i = this->begin; i < calls.size(); ++i)
			{
				if (calls[i].call == SoundCall::play_wave && calls[i].wave == this->waves.at(sound.name))
				{
					require(close_to(calls[i].volume, sound.volume * gain), "Weapon feedback ignored SFX gain");
					found = true;
				}
			}
			require(found, "Expected weapon feedback wave was never played");
		}

		std::unique_ptr<Weapon> weapon(const GameTuning& balance, WeaponType type,
			int index = 0, PlayerTeam team = PlayerTeam::a)
		{
			return WeaponBuilder::build_weapon(&balance, team, index, index, Colour::blue,
				type, mattmath::Vector2F::ZERO, &this->render, &this->audio, &this->preferences);
		}

		SoundState state(const std::string& name) const
		{
			return this->device.voice_state(this->voices.at(name));
		}

		float last_volume(const std::string& name) const
		{
			const auto voice = this->voices.at(name);
			const auto& calls = recorded_sounds(this->device);
			for (size_t i = calls.size(); i > this->begin; --i)
			{
				const auto& call = calls[i - 1];
				if (call.voice == voice && (call.call == SoundCall::play_voice ||
					call.call == SoundCall::set_voice_volume)) { return call.volume; }
			}
			throw std::runtime_error("Expected a volume for an owned cue");
		}

		void volume_is(const std::string& name, float volume) const
		{
			const auto voice = this->voices.at(name);
			const auto& calls = recorded_sounds(this->device);
			bool found = false;
			for (size_t i = this->begin; i < calls.size(); ++i)
			{
				if (calls[i].call == SoundCall::play_voice && calls[i].voice == voice)
				{
					require(!calls[i].loop && close_to(calls[i].volume, volume), "Room/countdown cue has wrong gain or loops");
					found = true;
				}
			}
			require(found, "Expected sound was never played");
		}

		template<typename Predicate>
		void until(Predicate done)
		{
			for (int i = 0; i < 10000 && !done(); ++i) { this->level->update(this->idle, 1.0f / 60.0f); }
			require(done(), "Match did not reach the expected audio boundary");
		}

		void skip(float dt)
		{
			auto inputs = this->idle;
			inputs[static_cast<size_t>(this->level->player(0).player_num())].skip_intro = true;
			this->level->update(inputs, dt);
		}
	};

	void check_story(World& world)
	{
		const MatchTuning& match = world.tuning.match();
		const OpeningTuning& opening = match.opening;
		const RevealTuning& reveal = match.reveal;
		const std::string& music = world.levels.get("close_quarters")->music_name();
		const float leave = opening.settle + opening.preview * 2.0f + opening.undo;
		const float chair = leave + opening.leave * 0.35f;
		const float length = leave + opening.leave;
		world.preferences.sfx_volume = 40;
		world.build(OpeningMode::full);
		world.level->update(world.idle, leave - 0.001f);
		require(world.plays(match.sigh_sound.name) == 0 && world.plays(match.chair_sound.name) == 0 &&
			world.plays(music) == 0, "Room cues or picture music started while the person was still choosing");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.sigh_sound.name) == 1 && world.plays(match.chair_sound.name) == 0,
			"The undo/leave edge must sigh once, before the chair moves");
		world.level->update(world.idle, 0.0f);
		require(world.plays(match.sigh_sound.name) == 1, "A stationary opening repeated its sigh");
		world.level->update(world.idle, chair - leave);
		require(world.plays(match.chair_sound.name) == 1, "Standing did not creak the chair once");
		world.level->suspend();
		require(world.state(match.sigh_sound.name) == SoundState::paused &&
			world.state(match.chair_sound.name) == SoundState::paused, "Suspension did not pause room voices");
		world.preferences.sfx_volume = 25;
		world.level->resume();
		require(world.state(match.chair_sound.name) == SoundState::playing &&
			world.plays(match.chair_sound.name) == 1, "Resume restarted a room cue instead of continuing it");
		require(close_to(world.last_volume(match.sigh_sound.name), match.sigh_sound.volume * 0.25f) &&
			close_to(world.last_volume(match.chair_sound.name), match.chair_sound.volume * 0.25f),
			"Resume did not apply edited SFX gain to the paused room cues");
		world.preferences.sfx_volume = 40;
		world.level->resume();
		world.volume_is(match.sigh_sound.name, match.sigh_sound.volume * 0.4f);
		world.volume_is(match.chair_sound.name, match.chair_sound.volume * 0.4f);
		world.level->update(world.idle, length - chair);
		require(world.level->state() == LevelState::start_countdown && world.plays(music) == 0,
			"Picture music must wait until the person has left");
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, 0.001f);
		require(world.plays(match.countdown_sound.name) == 1 && world.plays(music) == 1,
			"Zero-dt countdown updates repeated the initial beat or music start");
		world.until([&] { return world.level->state() == LevelState::active; });
		require(world.plays(match.countdown_sound.name) == static_cast<int>(std::ceil(match.countdown)) &&
			world.plays(match.go_sound.name) == 1, "Countdown must play each number once and GO once");
		world.volume_is(match.countdown_sound.name, match.countdown_sound.volume * 0.4f);
		world.volume_is(match.go_sound.name, match.go_sound.volume * 0.4f);
		world.until([&] { return world.level->state() == LevelState::zoom_out; });
		require(world.plays(match.time_up_sound.name) == 1 && world.state(music) == SoundState::stopped,
			"Time up must sound once and stop picture music before the person returns");
		world.until([&] { return world.level->state() == LevelState::overview; });
		world.level->update(world.idle, reveal.walk_in * 0.6f - 0.001f);
		require(world.plays(match.chair_sound.name) == 1, "Reveal chair sounded before sitting");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.chair_sound.name) == 2, "Sitting must creak the chair once");
		const float reveal_leave = reveal.walk_in + reveal.hover * 2.0f + reveal.decide + reveal.show;
		world.level->update(world.idle, reveal_leave - reveal.walk_in * 0.6f);
		require(world.plays(match.chair_sound.name) == 3 && world.plays(match.sigh_sound.name) == 1,
			"Reveal leaving must creak once without another sigh");
		world.until([&] { return world.level->state() == LevelState::finished; });
		world.level->update(world.idle, 1.0f);
		require(world.plays(match.chair_sound.name) == 3 && world.plays(match.time_up_sound.name) == 1 &&
			world.state(music) == SoundState::stopped, "Finished story repeated a cue or resumed picture music");
		world.level.reset();
		require(world.state(match.sigh_sound.name) == SoundState::stopped &&
			world.state(match.chair_sound.name) == SoundState::stopped &&
			world.state(match.go_sound.name) == SoundState::stopped &&
			world.state(match.time_up_sound.name) == SoundState::stopped, "Level destruction left an owned cue alive");
	}

	void check_skip_quick_and_solo(World& world)
	{
		const MatchTuning& match = world.tuning.match();
		const auto& timing = match.opening;
		const float leave = timing.settle + timing.preview * 2.0f + timing.undo;
		const float length = leave + timing.leave;
		world.build(OpeningMode::full);
		world.skip(length + 1.0f);
		require(world.level->state() == LevelState::start_countdown &&
			world.plays(match.sigh_sound.name) == 0 && world.plays(match.chair_sound.name) == 0,
			"Skipping over the whole opening burst its unheard room cues");
		world.build(OpeningMode::full);
		world.level->update(world.idle, leave + 0.001f);
		world.skip(length);
		require(world.plays(match.sigh_sound.name) == 1 && world.plays(match.chair_sound.name) == 0 &&
			world.state(match.sigh_sound.name) == SoundState::stopped,
			"Skipping an in-progress sigh must stop it without starting the chair");
		world.build(OpeningMode::quick);
		world.level->update(world.idle, length - timing.settle + 0.001f);
		world.level->update(world.idle, 0.0f);
		require(world.plays(match.sigh_sound.name) == 1 && world.plays(match.chair_sound.name) == 1,
			"A quick opening or long tick missed or repeated a crossed room cue");
		world.preferences.sfx_volume = 0;
		world.build(OpeningMode::off, 1);
		world.until([&] { return world.level->state() == LevelState::finished; });
		require(!world.level->revealed() && world.plays(match.sigh_sound.name) == 0 &&
			world.plays(match.chair_sound.name) == 0, "Solo practice played the person's room cues");
		world.volume_is(match.countdown_sound.name, 0.0f);
		world.volume_is(match.go_sound.name, 0.0f);
		world.volume_is(match.time_up_sound.name, 0.0f);
	}

	void check_weapon_starts(World& world)
	{
		world.level.reset();
		world.preferences.sfx_volume = 40;
		for (const auto type : { WeaponType::sprayer, WeaponType::roller, WeaponType::mister })
		{
			world.begin = recorded_sounds(world.device).size();
			const WeaponTuning& tuning = world.tuning.weapon(type);
			const std::string& start = tuning.details.shoot_sound_name;
			const std::string loop = tuning.loop_sounds.sound_name(PlayerTeam::a, 0);
			auto weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a, 0, 0, Colour::blue,
				type, mattmath::Vector2F::ZERO, &world.render, &world.audio, &world.preferences);
			const auto update = [&](bool held)
			{
				PlayerInputData input;
				input.primary_shoot = held;
				weapon->update_and_get_projectiles(input, mattmath::Vector2F::ZERO,
					mattmath::Vector2F::ZERO, true, 1.0f / 60.0f);
			};
			for (int i = 0; i < 10; ++i) { update(true); }
			require(world.wave_plays(start) == 1 && world.plays(loop) == 1,
				"Held firing repeated the weapon start or loop entry");
			weapon->pause_sounds();
			require(world.state(loop) == SoundState::paused, "Weapon suspension did not pause the loop");
			weapon->resume_sounds();
			update(true);
			require(world.wave_plays(start) == 1 && world.plays(loop) == 1 &&
				world.state(loop) == SoundState::playing, "Weapon resume retriggered a start cue");
			update(false);
			require(world.state(loop) == SoundState::stopped, "Released firing left its loop alive");
			update(true);
			require(world.wave_plays(start) == 2 && world.plays(loop) == 2,
				"A new trigger hold did not play a fresh weapon start");
			weapon->stop_sounds();
			require(world.state(loop) == SoundState::stopped, "Weapon stop left its loop alive");
			update(true);
			require(world.wave_plays(start) == 3 && world.plays(loop) == 3,
				"Stopping a weapon did not reset its next firing edge");
			weapon->stop_sounds();
		}

		// Player death bypasses the weapon's ordinary release update. Exercise
		// that real Level path so the held-fire flag cannot resurrect its loop.
		world.build(OpeningMode::off);
		world.until([&] { return world.level->state() == LevelState::active; });
		auto firing = world.idle;
		firing[static_cast<size_t>(world.level->player(0).player_num())].primary_shoot = true;
		world.level->update(firing, 1.0f / 60.0f);
		const auto& sprayer = world.tuning.weapon(WeaponType::sprayer);
		const std::string loop = sprayer.loop_sounds.sound_name(PlayerTeam::a, 0);
		require(world.state(loop) == SoundState::playing, "Player did not begin its firing loop");
		require(world.level->kill_player(0), "Death probe did not find its player");
		world.level->update(firing, 1.0f / 60.0f);
		world.level->update(firing, 1.0f / 60.0f);
		require(world.level->player(0).state() == PlayerState::dead &&
			world.state(loop) == SoundState::stopped &&
			world.wave_plays(sprayer.details.shoot_sound_name) == 1 &&
			world.wave_plays(sprayer.feedback_sounds.stop->name) == 0,
			"A dead player left or restarted its held-fire audio");
		world.level.reset();
		require(world.wave_plays(sprayer.feedback_sounds.stop->name) == 0,
			"Level teardown played an ordinary weapon stop cue");
	}

	void check_feedback_content(World& world, const ContentVariant& content)
	{
		// Every authored new cue is validated through the real bank, including
		// optional weapon-specific slots that can otherwise remain unnoticed.
		for (const auto& slot : { std::pair{ "sprayer", "empty" }, { "sprayer", "recovered" },
			{ "sprayer", "stop" }, { "sniper", "ready" }, { "roller", "lift" }, { "roller", "settle" } })
		{
			const GameTuning bad = content.tuning([&](rapidjson::Document& json)
			{
				json["weapons"][slot.first]["feedback_sounds"][slot.second]["name"].SetString(
					"missing.weapon.feedback", json.GetAllocator());
			});
			bool rejected = false;
			try { bad.validate_resources(world.render, world.audio); }
			catch (const std::exception& error)
			{
				const std::string key = std::string("weapons.") + slot.first + ".feedback_sounds." + slot.second + ".name";
				rejected = std::string(error.what()).find(key) != std::string::npos;
			}
			require(rejected, "Missing weapon feedback cue was accepted or lost its authored key");
		}
		for (float volume : { -0.01f, 1.01f })
		{
			bool rejected = false;
			try
			{
				content.tuning([&](rapidjson::Document& json)
				{
					json["weapons"]["sniper"]["feedback_sounds"]["ready"]["volume"].SetFloat(volume);
				});
			}
			catch (const std::exception& error)
			{
				const std::string message = error.what();
				rejected = message.find("feedback_sounds.ready") != std::string::npos &&
					message.find("'volume'") != std::string::npos;
			}
			require(rejected, "Out-of-range feedback volume was accepted or lost its authored key");
		}
		for (float delay : { 0.0f, -1.0f })
		{
			bool rejected = false;
			try
			{
				content.tuning([&](rapidjson::Document& json)
				{
					json["weapons"]["sprayer"]["feedback_sounds"]["empty_repeat_delay"].SetFloat(delay);
				});
			}
			catch (const std::exception& error)
			{
				const std::string message = error.what();
				rejected = message.find("feedback_sounds") != std::string::npos &&
					message.find("'empty_repeat_delay'") != std::string::npos;
			}
			require(rejected, "Nonpositive empty-repeat delay was accepted or lost its authored key");
		}
	}

	void exhaust(World& world, Weapon& weapon, const GameTuning& balance)
	{
		const auto& tuning = balance.weapon(weapon.type());
		const float cost = tuning.details.ammo_usage;
		const size_t volley = weapon.type() == WeaponType::mister ? 5u : 1u;
		int shots = 0;
		for (int tick = 0; tick < 2400 && world.wave_plays(tuning.feedback_sounds.empty.name) == 0; ++tick)
		{
			const float before = weapon.ammo();
			const auto projectiles = update_weapon(weapon, true);
			if (!projectiles.empty())
			{
				require(projectiles.size() == volley, "Weapon audio changed its paid volley size");
				require(static_cast<double>(cost) - before <= static_cast<double>(cost) * 0.000001,
					"Weapon feedback bought an underfunded shot");
				++shots;
			}
			require(weapon.ammo() >= 0.0f && weapon.ammo() <= balance.starting_ammo(),
				"Weapon feedback changed its tank bounds");
		}
		require(shots > 0 && world.wave_plays(tuning.feedback_sounds.empty.name) == 1,
			"Factory weapon did not exhaust with one empty edge");
	}

	void check_empty_and_recovered(World& world, const ContentVariant& content)
	{
		world.level.reset();
		const GameTuning no_regen = content.tuning([](rapidjson::Document& json)
		{
			json["weapons"]["ammo_regen_delay"].SetFloat(1000.0f);
		});
		for (const auto type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
			WeaponType::mister, WeaponType::bucket })
		{
			world.preferences.sfx_volume = 40;
			world.begin = recorded_sounds(world.device).size();
			const auto& feedback = no_regen.weapon(type).feedback_sounds;
			auto weapon = world.weapon(no_regen, type);
			update_weapon(*weapon, false, 0.0f);
			require(world.wave_plays(feedback.empty.name) == 0 && world.wave_plays(feedback.recovered.name) == 0,
				"New weapon played empty/recovered feedback before depletion");
			exhaust(world, *weapon, no_regen);
			const float depleted = weapon->ammo();
			for (int i = 0; i < 20; ++i)
			{
				require(update_weapon(*weapon, true, 0.0f).empty(), "Zero-dt depleted weapon fired");
			}
			require(world.wave_plays(feedback.empty.name) == 1 && weapon->ammo() == depleted,
				"Zero-dt updates repeated an empty cue or advanced ammunition");
			update_weapon(*weapon, true, feedback.empty_repeat_delay - 0.001f);
			require(world.wave_plays(feedback.empty.name) == 1, "Held empty feedback repeated before its delay");
			update_weapon(*weapon, true, 0.002f);
			require(world.wave_plays(feedback.empty.name) == 2, "Held empty feedback missed its repeat boundary");
			update_weapon(*weapon, true, feedback.empty_repeat_delay * 8.0f);
			require(world.wave_plays(feedback.empty.name) == 3, "A long update burst multiple empty cues");
			update_weapon(*weapon, false);
			update_weapon(*weapon, true);
			require(world.wave_plays(feedback.empty.name) == 4, "A new empty trigger press did not reset its latch");
			world.wave_volume_is(feedback.empty, 0.4f);
			if (feedback.stop)
			{
				const std::string loop = no_regen.weapon(type).loop_sounds.sound_name(PlayerTeam::a, 0);
				require(world.wave_plays(feedback.stop->name) == 1 && world.state(loop) == SoundState::stopped,
					"Depleted loop did not stop with exactly one ordinary stop cue");
				world.wave_volume_is(*feedback.stop, 0.4f);
			}
			weapon->stop_sounds();
			weapon->reset_ammo();
			update_weapon(*weapon, false);
			require(world.wave_plays(feedback.recovered.name) == 0, "Respawn/refill played recovered feedback");

			// Normal authored regeneration, with the trigger released: the cue
			// reports one full-shot affordability edge, without buying a shot.
			world.begin = recorded_sounds(world.device).size();
			weapon = world.weapon(world.tuning, type);
			const auto& ordinary = world.tuning.weapon(type);
			exhaust(world, *weapon, world.tuning);
			for (int tick = 0; tick < 600 && world.wave_plays(ordinary.feedback_sounds.recovered.name) == 0; ++tick)
			{
				const float before = weapon->ammo();
				require(update_weapon(*weapon, false).empty(), "Recovered feedback fired a released weapon");
				if (world.wave_plays(ordinary.feedback_sounds.recovered.name) != 0)
				{
					require(before < ordinary.details.ammo_usage &&
						weapon->ammo() >= ordinary.details.ammo_usage * 0.999999f,
						"Recovered cue did not follow full-shot affordability");
				}
			}
			require(world.wave_plays(ordinary.feedback_sounds.recovered.name) == 1,
				"Factory weapon did not announce its affordability recovery once");
			for (int tick = 0; tick < 300; ++tick) { update_weapon(*weapon, false); }
			require(world.wave_plays(ordinary.feedback_sounds.recovered.name) == 1 &&
				close_to(weapon->ammo(), world.tuning.starting_ammo()),
				"Filling an already affordable tank repeated recovered feedback");
			world.wave_volume_is(ordinary.feedback_sounds.recovered, 0.4f);
			weapon->stop_sounds();
		}
		world.preferences.sfx_volume = 0;
		world.begin = recorded_sounds(world.device).size();
		auto muted = world.weapon(world.tuning, WeaponType::sprayer);
		const auto& feedback = world.tuning.weapon(WeaponType::sprayer).feedback_sounds;
		exhaust(world, *muted, world.tuning);
		for (int tick = 0; tick < 600 && world.wave_plays(feedback.recovered.name) == 0; ++tick)
		{
			update_weapon(*muted, false);
		}
		world.wave_volume_is(feedback.empty, 0.0f);
		world.wave_volume_is(feedback.recovered, 0.0f);
		muted->stop_sounds();

		// Brief regenerated shots during the same hold must not repeatedly
		// re-arm empty as a fresh press and bypass its authored rate limit.
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		auto cycling = world.weapon(world.tuning, WeaponType::sprayer);
		exhaust(world, *cycling, world.tuning);
		for (int tick = 0; tick < 120; ++tick) { update_weapon(*cycling, true); }
		const int limit = 1 + static_cast<int>(std::floor(120.0f * weapon_step / feedback.empty_repeat_delay));
		require(world.wave_plays(feedback.empty.name) <= limit && world.wave_plays(feedback.recovered.name) > 0,
			"Held regeneration/depletion cycles bypassed the empty feedback rate limit");
		cycling->stop_sounds();
	}

	void check_sniper_ready(World& world, const ContentVariant& content)
	{
		const auto& tuning = world.tuning.weapon(WeaponType::sniper);
		const SoundTuning& ready = *tuning.feedback_sounds.ready;
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		auto weapon = world.weapon(world.tuning, WeaponType::sniper);
		update_weapon(*weapon, false, tuning.details.shoot_interval);
		require(!weapon->ready_to_shoot() && world.wave_plays(ready.name) == 0,
			"Sniper initial readiness ignored the strict cooldown or played a startup cue");
		require(update_weapon(*weapon, true, 0.125f).empty(), "Sniper fired at exact cooldown equality");
		require(update_weapon(*weapon, true, 0.25f).size() == 1 && world.wave_plays(tuning.details.shoot_sound_name) == 1,
			"Ready feedback changed the sniper's paid shot");
		update_weapon(*weapon, false, tuning.details.shoot_interval - 0.25f);
		require(!weapon->ready_to_shoot() && world.wave_plays(ready.name) == 0,
			"Sniper ready sounded at exact cooldown equality");
		update_weapon(*weapon, false, 0.0f);
		update_weapon(*weapon, false, 0.001f);
		require(weapon->ready_to_shoot() && world.wave_plays(ready.name) == 1,
			"Sniper did not announce the strict affordable cooldown edge once");
		for (int tick = 0; tick < 20; ++tick) { update_weapon(*weapon, false, 0.0f); }
		update_weapon(*weapon, false, 2.0f);
		require(world.wave_plays(ready.name) == 1, "Eligible sniper repeated its ready cue while idle");
		world.wave_volume_is(ready, 0.4f);
		weapon->reset_ammo();
		update_weapon(*weapon, false, 2.0f);
		require(world.wave_plays(ready.name) == 1, "Sniper refill played a readiness cue");

		// One paid shot empties this authored tank. Cooldown expires before
		// regeneration can afford a second shot, so there is no ready chime.
		const GameTuning small_tank = content.tuning([&](rapidjson::Document& json)
		{
			json["weapons"]["starting_ammo"].SetFloat(tuning.details.ammo_usage);
			json["weapons"]["ammo_regen_delay"].SetFloat(2.0f);
		});
		world.begin = recorded_sounds(world.device).size();
		weapon = world.weapon(small_tank, WeaponType::sniper);
		update_weapon(*weapon, false, tuning.details.shoot_interval + 0.001f);
		require(update_weapon(*weapon, true, 0.25f).size() == 1 && !weapon->ready_to_shoot(),
			"Small sniper tank did not pay for its final shot");
		update_weapon(*weapon, false, tuning.details.shoot_interval);
		require(world.wave_plays(ready.name) == 0 && !weapon->ready_to_shoot(),
			"Unaffordable sniper announced readiness at cooldown expiry");
		for (int tick = 0; tick < 300; ++tick) { update_weapon(*weapon, false); }
		require(weapon->ready_to_shoot() && world.wave_plays(ready.name) == 0 &&
			world.wave_plays(tuning.feedback_sounds.recovered.name) == 1,
			"Late affordability recovery replayed a previously unaffordable cooldown cue");

		world.preferences.sfx_volume = 0;
		world.begin = recorded_sounds(world.device).size();
		weapon = world.weapon(world.tuning, WeaponType::sniper);
		update_weapon(*weapon, false, 2.0f);
		update_weapon(*weapon, true);
		update_weapon(*weapon, false, 2.0f);
		world.wave_volume_is(ready, 0.0f);
	}

	void check_independent_loops(World& world)
	{
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		const auto& tuning = world.tuning.weapon(WeaponType::sprayer);
		std::vector<std::unique_ptr<Weapon>> weapons;
		std::vector<std::string> loops;
		for (int index = 0; index < 4; ++index)
		{
			const auto team = index < 2 ? PlayerTeam::a : PlayerTeam::b;
			loops.push_back(tuning.loop_sounds.sound_name(team, index));
			weapons.push_back(world.weapon(world.tuning, WeaponType::sprayer, index, team));
			update_weapon(*weapons.back(), true);
		}
		for (const auto& loop : loops)
		{
			require(world.state(loop) == SoundState::playing && world.plays(loop) == 1,
				"Four-player firing shared or failed to start an owned loop voice");
		}
		update_weapon(*weapons[0], false);
		update_weapon(*weapons[0], false);
		require(world.state(loops[0]) == SoundState::stopped &&
			world.wave_plays(tuning.feedback_sounds.stop->name) == 1,
			"Ordinary release did not stop one player's loop once");
		for (size_t index = 1; index < weapons.size(); ++index)
		{
			require(world.state(loops[index]) == SoundState::playing && world.plays(loops[index]) == 1,
				"One player's release stopped or restarted another player's loop");
			weapons[index]->pause_sounds();
			weapons[index]->resume_sounds();
			weapons[index]->stop_sounds();
		}
		require(world.wave_plays(tuning.feedback_sounds.stop->name) == 1,
			"Pause/resume or forced lifecycle stop played an ordinary stop cue");

		world.preferences.sfx_volume = 0;
		world.begin = recorded_sounds(world.device).size();
		update_weapon(*weapons[0], true);
		update_weapon(*weapons[0], false);
		world.wave_volume_is(*tuning.feedback_sounds.stop, 0.0f);
	}

	void check_roller_contacts(World& world)
	{
		using namespace mattmath;
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		const auto& tuning = world.tuning.weapon(WeaponType::roller);
		const RectangleF rectangle(0.0f, -world.tuning.player().size.y,
			world.tuning.player().size.x, world.tuning.player().size.y);
		Scene scene(nullptr, nullptr);
		Player* player = scene.add(std::make_unique<Player>(rectangle, &world.tuning,
			&world.presentation.movement_feedback(), &world.presentation.respawn_redraw(),
			&world.render, &world.audio, &world.preferences, 0, PlayerTeam::a,
			WeaponType::roller, world.weapon(world.tuning, WeaponType::roller), WeaponType::none,
			Colour::blue, Viewport{}, rectangle.top_left()));
		const RectangleF floor(-10000.0f, 0.0f, 20000.0f, 100.0f);
		scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", floor,
			std::make_unique<RectangleF>(floor), &world.render, CollisionObjectType::structure));
		scene.end_tick();
		const auto tick = [&](bool held, bool jump = false, float dt = weapon_step)
		{
			PlayerInputData input;
			input.primary_shoot = held;
			input.jump_pressed = jump;
			input.jump_held = jump;
			player->set_player_input(input);
			scene.update(dt);
			player->update_weapon_and_get_projectiles(dt, {});
			scene.resolve();
			player->end_contacts();
			player->update_prev_rectangle();
			scene.end_tick();
		};
		tick(true);
		tick(true);
		require(player->move_state() == PlayerMoveState::on_ground &&
			world.wave_plays(tuning.feedback_sounds.settle->name) == 0,
			"Initial roller ground contact played an artificial settle cue");
		tick(true, true);
		require(world.wave_plays(tuning.feedback_sounds.lift->name) == 1,
			"Player's held roller did not lift after resolved jump support loss");
		tick(true, false, 0.0f);
		for (int step = 0; step < 180 && world.wave_plays(tuning.feedback_sounds.settle->name) == 0; ++step)
		{
			tick(true);
		}
		require(world.wave_plays(tuning.feedback_sounds.lift->name) == 1 &&
			world.wave_plays(tuning.feedback_sounds.settle->name) == 1,
			"Roller jump/landing did not produce one lift and one settle");
		for (int step = 0; step < 10; ++step) { tick(true); }
		require(world.wave_plays(tuning.feedback_sounds.settle->name) == 1,
			"Continuous ground contact repeated roller settle");
		world.wave_volume_is(*tuning.feedback_sounds.lift, 0.4f);
		world.wave_volume_is(*tuning.feedback_sounds.settle, 0.4f);
		tick(false, true);
		for (int step = 0; step < 100; ++step) { tick(false); }
		require(world.wave_plays(tuning.feedback_sounds.lift->name) == 1 &&
			world.wave_plays(tuning.feedback_sounds.settle->name) == 1,
			"Released roller played airborne/landing feedback");
		world.preferences.sfx_volume = 0;
		world.begin = recorded_sounds(world.device).size();
		tick(true);
		tick(true, true);
		for (int step = 0; step < 100; ++step) { tick(true); }
		world.wave_volume_is(*tuning.feedback_sounds.lift, 0.0f);
		world.wave_volume_is(*tuning.feedback_sounds.settle, 0.0f);
		player->stop_sounds();
	}

	void check_player_weapon_lifecycle(World& world)
	{
		using namespace mattmath;
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		const RectangleF rectangle(Vector2F::ZERO, world.tuning.player().size);
		Player player(rectangle, &world.tuning, &world.presentation.movement_feedback(),
			&world.presentation.respawn_redraw(), &world.render, &world.audio,
			&world.preferences, 0, PlayerTeam::a, WeaponType::sniper,
			world.weapon(world.tuning, WeaponType::sniper), WeaponType::none,
			Colour::blue, Viewport{}, Vector2F::ZERO);
		player.set_infinite_ammo(true);
		PlayerInputData held;
		held.primary_shoot = true;
		player.set_player_input(held);
		const auto& sniper = world.tuning.weapon(WeaponType::sniper);
		player.update_weapon_and_get_projectiles(sniper.details.shoot_interval + 0.001f, {});
		require(player.update_weapon_and_get_projectiles(weapon_step, {}).size() == 1 &&
			player.weapon_ammo() == world.tuning.starting_ammo(),
			"Infinite-ammo player did not fire one paid sniper volley and refill");
		player.set_player_input(PlayerInputData{});
		player.update_weapon_and_get_projectiles(sniper.details.shoot_interval, {});
		require(world.wave_plays(sniper.feedback_sounds.ready->name) == 1,
			"Infinite-ammo refill discarded the sniper's pending cooldown feedback");
		require(world.wave_plays(sniper.feedback_sounds.recovered.name) == 0,
			"Infinite-ammo refill played artificial affordability feedback");
		player.set_player_input(held);
		const int shots = world.wave_plays(sniper.details.shoot_sound_name);
		player.kill();
		require(player.state() == PlayerState::alive && player.health() <= 0.0f,
			"Fatal-contact probe did not reach the pre-death-state boundary");
		require(player.update_weapon_and_get_projectiles(weapon_step, {}).empty() &&
			world.wave_plays(sniper.details.shoot_sound_name) == shots,
			"Just-fatal player fired before its state changed to dead");
		player.end_contacts();
		player.update(weapon_step);
		require(player.state() == PlayerState::dead &&
			player.update_weapon_and_get_projectiles(weapon_step, {}).empty(),
			"Death-state player resumed weapon fire");
	}
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: story_audio.exe <content directory>");
		World world{ std::filesystem::path(argv[1]) };
		const ContentVariant content(std::filesystem::path(argv[1]) / "tuning.json");
		check_feedback_content(world, content);
		check_story(world);
		check_skip_quick_and_solo(world);
		check_weapon_starts(world);
		check_empty_and_recovered(world, content);
		check_sniper_ready(world, content);
		check_independent_loops(world);
		check_roller_contacts(world);
		check_player_weapon_lifecycle(world);
		std::cout << "PASS story audio: opening/reveal edges, skipped and quick openings, solo, "
			"countdown/GO/time up, gains/mute, suspension, destruction, weapon feedback validation, "
			"empty/recovered/ready edges, independent loops, stop lifecycle and roller contacts\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL story audio: " << error.what() << '\n';
		return 1;
	}
}
