#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/weapon_builder.h"
#include "game/objects/structure.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/paint_audio.h"
#include "game/states/connection_audio.h"
#include "game/states/resume_audio.h"
#include "game/states/results_audio.h"
#include "game/objects/projectile_jet.h"
#include "engine/assets/sound_bank_loader.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/null/recording.h"
#include "engine/collision/partitioner.h"
#include "engine/math/triangle.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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
			for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx" })
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
			this->tuning.validate_match_sounds(this->render, this->audio, "paint_exe");
			for (const char* stage : { "close_quarters", "king_of_the_hill", "turbulence", "movement_course" })
			{
				auto info = std::make_unique<LevelLoadedInfo>(
					(content / "levels" / (std::string(stage) + ".json")).string().c_str());
				info->validate();
				this->levels.add(stage, std::move(info));
			}
			this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render, &this->audio,
				&this->preferences, &this->levels, &this->tuning, &this->presentation, &this->resolution,
				nullptr, &this->partitioner, &this->mouse);
		}

		void build(OpeningMode opening, int players = 2, float duration = 0.25f,
			const std::string& stage = "close_quarters")
		{
			this->level.reset();
			this->begin = recorded_sounds(this->device).size();
			MenuLevelSettings settings;
			settings.set_stage(stage);
			settings.set_player_count(players);
			for (int index = 0; index < players; ++index)
			{
				settings.set_player_team(index, index < (players + 1) / 2 ? PlayerTeam::a : PlayerTeam::b);
				settings.set_player_weapon(index, WeaponType::sprayer);
			}
			settings.begin_input_claims(input_slots::keyboard_and_mouse);
			settings.set_screen_layout(players == 1 ? ScreenLayout::one_player :
				players == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
			settings.set_duration(duration);
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

		float last_wave_volume(const SoundTuning& sound) const
		{
			const auto& calls = recorded_sounds(this->device);
			for (size_t i = calls.size(); i > this->begin; --i)
			{
				const auto& call = calls[i - 1];
				if (call.call == SoundCall::play_wave && call.wave == this->waves.at(sound.name)) { return call.volume; }
			}
			throw std::runtime_error("Expected movement feedback wave was never played: " + sound.name);
		}

		void wave_volume_is(const SoundTuning& sound, float gain) const
		{
			bool found = false;
			const auto& calls = recorded_sounds(this->device);
			for (size_t i = this->begin; i < calls.size(); ++i)
			{
				if (calls[i].call == SoundCall::play_wave && calls[i].wave == this->waves.at(sound.name))
				{
					require(close_to(calls[i].volume, sound.volume * gain), "Feedback wave ignored SFX gain");
					found = true;
				}
			}
			if (!found) { throw std::runtime_error("Expected feedback wave was never played: " + sound.name); }
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
		require(world.plays(match.countdown_sound.name) == 0 && world.plays(music) == 0,
			"Frozen countdown started its beat or picture music");
		world.level->update(world.idle, 0.001f);
		require(world.plays(match.countdown_sound.name) == 1 && world.plays(music) == 1,
			"Zero-dt countdown updates repeated the initial beat or music start");
		require(close_to(world.last_volume(music), 0.0f), "Arena music did not start at the silent fade boundary");
		world.level->update_music(world.presentation.music_mix().fade_in);
		require(close_to(world.last_volume(music), world.levels.get("close_quarters")->music_volume() *
			world.preferences.music_gain()), "Raw audio time did not complete the opening music fade");
		world.until([&] { return world.level->state() == LevelState::active; });
		require(world.plays(match.countdown_sound.name) == static_cast<int>(std::ceil(match.countdown)) &&
			world.plays(match.go_sound.name) == 1, "Countdown must play each number once and GO once");
		world.volume_is(match.countdown_sound.name, match.countdown_sound.volume * 0.4f);
		world.volume_is(match.go_sound.name, match.go_sound.volume * 0.4f);
		world.until([&] { return world.level->state() == LevelState::zoom_out; });
		require(world.plays(match.time_up_sound.name) == 1 && world.state(music) == SoundState::playing,
			"Time up must sound once and let the arena loop fade under its ending sting");
		world.level->update_music(world.presentation.music_mix().fade_out);
		require(world.state(music) == SoundState::stopped,
			"Raw audio time did not finish the time-up music fade");
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

	void check_match_content(World& world, const ContentVariant& content)
	{
		for (const char* key : { "glass_in", "close", "celebrate", "slump", "colour_in", "skip", "undo",
			"last_ten", "room_tone" })
		{
			bool rejected = false;
			try
			{
				const GameTuning bad = content.tuning([&](rapidjson::Document& json)
				{
					json["match"]["sounds"][key]["name"].SetString("missing.match.cue", json.GetAllocator());
				});
				bad.validate_match_sounds(world.render, world.audio, "paint_exe");
			}
			catch (const std::exception& error)
			{
				rejected = std::string(error.what()).find(std::string("match.sounds.") + key + ".name") !=
					std::string::npos;
			}
			require(rejected, "Missing match cue was accepted or lost its authored key");
			for (float volume : { -0.01f, 1.01f })
			{
				rejected = false;
				try
				{
					content.tuning([&](rapidjson::Document& json)
					{
						json["match"]["sounds"][key]["volume"].SetFloat(volume);
					});
				}
				catch (const std::exception& error)
				{
					const std::string message = error.what();
					rejected = message.find(std::string("match.sounds.") + key) != std::string::npos &&
						message.find("'volume'") != std::string::npos;
				}
				require(rejected, "Out-of-range match cue volume was accepted or lost its authored key");
			}
		}
	}

	void check_opening_details(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.opening;
		const float first = timing.settle + timing.preview * 0.4f;
		const float second = timing.settle + timing.preview * 1.4f;
		const float undo = timing.settle + timing.preview * 2.0f;
		const float length = undo + timing.undo + timing.leave;
		world.preferences.sfx_volume = 40;
		world.build(OpeningMode::full);
		world.level->update(world.idle, first - 0.001f);
		require(world.wave_plays(match.hover_sound.name) == 0 &&
			world.plays(match.undo_sound.name) == 0 && world.plays(match.colour_in_sound.name) == 0,
			"An opening detail sounded before its visual edge");
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, -1.0f);
		world.level->update(world.idle, 0.002f);
		require(world.wave_plays(match.hover_sound.name) == 1,
			"Nonpositive opening updates rewound or consumed the first swatch arrival");
		world.level->update(world.idle, second - first);
		require(world.wave_plays(match.hover_sound.name) == 2 && world.plays(match.undo_sound.name) == 0,
			"Second opening swatch arrival missed, repeated or undid early");
		world.level->update(world.idle, undo - second - 0.002f);
		require(world.plays(match.undo_sound.name) == 0, "Undo sounded before its return-to-grey edge");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.undo_sound.name) == 1 && world.plays(match.colour_in_sound.name) == 0,
			"Opening undo missed its edge or coloured the figures early");
		world.volume_is(match.undo_sound.name, match.undo_sound.volume * 0.4f);
		world.level->suspend();
		require(world.state(match.undo_sound.name) == SoundState::paused, "Suspension did not pause opening undo");
		world.preferences.sfx_volume = 25;
		world.level->resume();
		require(world.state(match.undo_sound.name) == SoundState::playing && world.plays(match.undo_sound.name) == 1 &&
			close_to(world.last_volume(match.undo_sound.name), match.undo_sound.volume * 0.25f),
			"Opening undo resume restarted its voice or ignored edited SFX gain");
		world.level->update(world.idle, length - undo);
		require(world.level->state() == LevelState::start_countdown && world.plays(match.colour_in_sound.name) == 1,
			"Figures did not receive colour once at the countdown entrance");
		world.volume_is(match.colour_in_sound.name, match.colour_in_sound.volume * 0.25f);
		world.wave_volume_is(match.hover_sound, 0.4f);
		world.level->update(world.idle, 0.0f);
		world.level->update(world.idle, -1.0f);
		require(world.plays(match.glass_in_sound.name) == 0 && world.plays(match.colour_in_sound.name) == 1,
			"Frozen countdown invented glass motion or repeated colour arrival");
		world.level->update(world.idle, match.countdown - timing.push - 0.001f);
		require(world.plays(match.glass_in_sound.name) == 0, "Glass entry sounded before the push began");
		world.level->update(world.idle, 0.002f);
		require(world.plays(match.glass_in_sound.name) == 1, "Countdown missed its glass push edge");
		world.volume_is(match.glass_in_sound.name, match.glass_in_sound.volume * 0.25f);
		world.level->suspend();
		require(world.state(match.colour_in_sound.name) == SoundState::paused &&
			world.state(match.glass_in_sound.name) == SoundState::paused, "Suspension did not pause owned opening cues");
		world.preferences.sfx_volume = 0;
		world.level->resume();
		require(world.plays(match.glass_in_sound.name) == 1 &&
			close_to(world.last_volume(match.glass_in_sound.name), 0.0f) &&
			close_to(world.last_volume(match.colour_in_sound.name), 0.0f),
			"Opening cue resume retriggered glass or ignored mute");
		world.skip(0.001f);
		require(world.state(match.glass_in_sound.name) == SoundState::stopped &&
			world.state(match.colour_in_sound.name) == SoundState::stopped &&
			world.state(match.undo_sound.name) == SoundState::stopped && world.plays(match.skip_sound.name) == 1,
			"Skipping the glass push failed to stop the story voices or emit one skip cue");
		world.volume_is(match.skip_sound.name, 0.0f);
		world.skip(0.001f);
		require(world.plays(match.skip_sound.name) == 1 && world.plays(match.glass_in_sound.name) == 1,
			"Split countdown repeated a skip or glass entry");
		world.level->suspend();
		require(world.state(match.skip_sound.name) == SoundState::paused, "Suspension did not pause skip audio");
		world.preferences.sfx_volume = 40;
		world.level->resume();
		require(world.plays(match.skip_sound.name) == 1 &&
			close_to(world.last_volume(match.skip_sound.name), match.skip_sound.volume * 0.4f),
			"Skip audio resume restarted or ignored the current gain");
		world.level.reset();
		for (const auto* sound : { &match.undo_sound, &match.colour_in_sound, &match.glass_in_sound, &match.skip_sound })
		{
			require(world.state(sound->name) == SoundState::stopped, "Teardown left an opening detail alive");
		}
	}

	void check_opening_modes_and_long_ticks(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.opening;
		const float length = timing.settle + timing.preview * 2.0f + timing.undo + timing.leave;
		world.preferences.sfx_volume = 0;
		for (const auto mode : { OpeningMode::full, OpeningMode::quick })
		{
			world.build(mode);
			world.level->update(world.idle, length + 1.0f);
			require(world.wave_plays(match.hover_sound.name) == 2 && world.plays(match.undo_sound.name) == 1 &&
				world.plays(match.colour_in_sound.name) == 1 && world.plays(match.glass_in_sound.name) == 0,
				"A long full/quick opening update lost, repeated or advanced beyond its crossed edges");
			world.wave_volume_is(match.hover_sound, 0.0f);
			world.volume_is(match.undo_sound.name, 0.0f);
			world.volume_is(match.colour_in_sound.name, 0.0f);
			// Level's established countdown path gives the GO tick to physics
			// at its full delta. Keep this audio-only long step inside the arena.
			for (int view = 0; view < world.level->player_count(); ++view)
			{
				require(world.level->place_player(view, { 1000.0f + view * 100.0f, 4000.0f }),
					"Cannot position long-countdown audio fixture");
			}
			world.level->update(world.idle, match.countdown - timing.push - 0.001f);
			world.level->update(world.idle, timing.push + 0.002f);
			require(world.plays(match.glass_in_sound.name) == 1 && world.plays(match.go_sound.name) == 1,
				"Long countdown crossing GO missed or repeated the glass entry");
			world.volume_is(match.glass_in_sound.name, 0.0f);
		}
		world.build(OpeningMode::full);
		world.skip(0.0f);
		world.skip(-1.0f);
		require(world.level->state() == LevelState::opening && world.plays(match.skip_sound.name) == 0,
			"Nonpositive updates consumed intro skip input");
		world.skip(length + 1.0f);
		require(world.plays(match.skip_sound.name) == 1 && world.wave_plays(match.hover_sound.name) == 0 &&
			world.plays(match.undo_sound.name) == 0 && world.plays(match.colour_in_sound.name) == 0 &&
			world.plays(match.glass_in_sound.name) == 0,
			"Skipping the whole opening played unheard story edges");
		for (int players : { 1, 2, 4 })
		{
			world.build(OpeningMode::off, players);
			world.until([&] { return world.level->state() == LevelState::finished; });
			require(!world.level->revealed() && world.wave_plays(match.hover_sound.name) == 0 &&
				world.wave_plays(match.click_sound.name) == 0 && world.wave_plays(match.close_sound.name) == 0,
				"An opening-off/solo match played the monitor's reveal cues");
			for (const auto* sound : { &match.undo_sound, &match.colour_in_sound, &match.glass_in_sound,
				&match.skip_sound, &match.celebrate_sound, &match.slump_sound })
			{
				require(world.plays(sound->name) == 0, "An opening-off/solo match played a story-only voice");
			}
		}
	}

	// Shoot at a real exposed scoring face through Level's normal input and
	// projectile phases. Four figures then prove reactions belong to teams,
	// rather than retriggering once per figure or player view.
	class PaintForReveal final : public SeatInputFilter
	{
	public:
		explicit PaintForReveal(size_t view) : view_(view) {}
		void filter_seats(std::vector<PlayerInputData>& seats, const Level&) override
		{
			auto& input = seats.at(this->view_);
			input.primary_shoot = true;
			input.shoot_angle = 1.57079632679f;
			input.shoot_direction = { 0.0f, 1.0f };
			input.shoot_direction_requested = true;
		}
	private:
		size_t view_;
	};

	void prepare_reveal(World& world, PlayerTeam winner)
	{
		const auto& timing = world.tuning.match().opening;
		world.build(OpeningMode::full, 4, 1.0f);
		world.level->update(world.idle, timing.settle + timing.preview * 2.0f +
			timing.undo + timing.leave + 0.001f);
		world.until([&] { return world.level->state() == LevelState::active; });
		if (winner != PlayerTeam::none)
		{
			const int view = winner == PlayerTeam::a ? 0 : 2;
			require(world.level->place_player(view, { 2000.0f, 4790.0f }), "Cannot place reveal painting fixture");
			PaintForReveal paint(static_cast<size_t>(view));
			for (int tick = 0; tick < 100 && world.level->state() == LevelState::active; ++tick)
			{
				world.level->update(world.idle, weapon_step, &paint);
			}
		}
		world.until([&] { return world.level->state() == LevelState::overview; });
		require(world.level->level_end_info().winning_team() == winner,
			"Real paint fixture did not produce the intended reveal winner/draw");
		world.begin = recorded_sounds(world.device).size();
	}

	void check_reveal_details(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.reveal;
		const float first = timing.walk_in + timing.hover * 0.4f;
		const float second = timing.walk_in + timing.hover * 1.4f;
		const float click = timing.walk_in + timing.hover * 2.0f + timing.decide;
		const float alone = click + timing.show + timing.leave;
		for (const auto winner : { PlayerTeam::none, PlayerTeam::a, PlayerTeam::b })
		{
			world.preferences.sfx_volume = 40;
			prepare_reveal(world, winner);
			world.level->update(world.idle, first - 0.001f);
			require(world.wave_plays(match.hover_sound.name) == 0 && world.wave_plays(match.click_sound.name) == 0 &&
				world.wave_plays(match.close_sound.name) == 0, "Reveal cue played before its visual edge");
			world.level->update(world.idle, 0.0f);
			world.level->update(world.idle, -1.0f);
			world.level->update(world.idle, 0.002f);
			require(world.wave_plays(match.hover_sound.name) == (winner == PlayerTeam::none ? 0 : 1),
				"Reveal swatch cue ignored winner/draw or nonpositive updates");
			world.level->update(world.idle, second - first);
			require(world.wave_plays(match.hover_sound.name) == (winner == PlayerTeam::none ? 0 : 2),
				"Second reveal swatch arrival missed, repeated or played during a draw");
			world.preferences.sfx_volume = 0;
			world.level->update(world.idle, click - second - 0.002f);
			require(world.wave_plays(match.click_sound.name) == 0 && world.wave_plays(match.close_sound.name) == 0,
				"Reveal click/close sounded before the cursor committed");
			world.level->update(world.idle, 0.002f);
			require(world.wave_plays(match.click_sound.name) == (winner == PlayerTeam::none ? 0 : 1) &&
				world.wave_plays(match.close_sound.name) == (winner == PlayerTeam::none ? 1 : 0),
				"Reveal failed to distinguish saving a winner from closing a draw");
			world.wave_volume_is(winner == PlayerTeam::none ? match.close_sound : match.click_sound, 0.0f);
			if (winner != PlayerTeam::none) { world.wave_volume_is(match.hover_sound, 0.4f); }
			world.level->update(world.idle, alone - click - 0.002f);
			require(world.plays(match.celebrate_sound.name) == 0 && world.plays(match.slump_sound.name) == 0,
				"Figures reacted while the person was still in the room");
			world.preferences.sfx_volume = 25;
			world.level->update(world.idle, 0.002f);
			const int reactions = winner == PlayerTeam::none ? 0 : 1;
			require(world.plays(match.celebrate_sound.name) == reactions && world.plays(match.slump_sound.name) == reactions,
				"Winner/loser reactions missed, repeated per figure or played for a draw");
			if (reactions > 0)
			{
				world.volume_is(match.celebrate_sound.name, match.celebrate_sound.volume * 0.25f);
				world.volume_is(match.slump_sound.name, match.slump_sound.volume * 0.25f);
				world.level->suspend();
				require(world.state(match.celebrate_sound.name) == SoundState::paused &&
					world.state(match.slump_sound.name) == SoundState::paused, "Suspension did not pause reaction voices");
				world.preferences.sfx_volume = 0;
				world.level->resume();
				require(world.plays(match.celebrate_sound.name) == 1 && world.plays(match.slump_sound.name) == 1 &&
					close_to(world.last_volume(match.celebrate_sound.name), 0.0f) &&
					close_to(world.last_volume(match.slump_sound.name), 0.0f),
					"Reaction resume restarted its voices or ignored current mute");
			}
			world.level->update(world.idle, 0.0f);
			world.level->update(world.idle, -1.0f);
			world.until([&] { return world.level->state() == LevelState::finished; });
			world.level->update(world.idle, timing.react + 1.0f);
			require(world.plays(match.celebrate_sound.name) == reactions && world.plays(match.slump_sound.name) == reactions &&
				world.wave_plays(match.hover_sound.name) == reactions * 2,
				"Finished reveal or nonpositive ticks repeated a detail");
			world.level.reset();
			require(world.state(match.celebrate_sound.name) == SoundState::stopped &&
				world.state(match.slump_sound.name) == SoundState::stopped, "Teardown left a reaction voice alive");
		}
	}

	void check_long_reveal(World& world)
	{
		const auto& match = world.tuning.match();
		const auto& timing = match.reveal;
		for (const auto winner : { PlayerTeam::none, PlayerTeam::a })
		{
			prepare_reveal(world, winner);
			world.preferences.sfx_volume = 40;
			world.level->update(world.idle, timing.walk_in + timing.hover * 2.0f + timing.decide +
				timing.show + timing.leave + timing.react + 0.001f);
			require(world.level->state() == LevelState::finished, "Long reveal did not finish");
			std::vector<std::string> events;
			const auto& calls = recorded_sounds(world.device);
			for (size_t index = world.begin; index < calls.size(); ++index)
			{
				const auto& call = calls[index];
				for (const auto* sound : { &match.chair_sound, &match.celebrate_sound, &match.slump_sound })
				{
					if (call.call == SoundCall::play_voice && call.voice == world.voices.at(sound->name))
					{
						events.push_back(sound->name);
					}
				}
				for (const auto* sound : { &match.hover_sound, &match.click_sound, &match.close_sound })
				{
					if (call.call == SoundCall::play_wave && call.wave == world.waves.at(sound->name))
					{
						events.push_back(sound->name);
					}
				}
			}
			const std::vector<std::string> expected = winner == PlayerTeam::none ?
				std::vector<std::string>{ match.chair_sound.name, match.close_sound.name, match.chair_sound.name } :
				std::vector<std::string>{ match.chair_sound.name, match.hover_sound.name, match.hover_sound.name,
					match.click_sound.name, match.chair_sound.name, match.celebrate_sound.name, match.slump_sound.name };
			require(events == expected, "Long reveal lost, duplicated or reordered its crossed winner/draw edges");
			world.wave_volume_is(winner == PlayerTeam::none ? match.close_sound : match.click_sound, 0.4f);
			if (winner != PlayerTeam::none)
			{
				world.wave_volume_is(match.hover_sound, 0.4f);
				world.volume_is(match.celebrate_sound.name, match.celebrate_sound.volume * 0.4f);
				world.volume_is(match.slump_sound.name, match.slump_sound.volume * 0.4f);
			}
		}
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
			&world.presentation.movement_feedback(), &world.presentation.combat_feedback(), &world.presentation.respawn_redraw(),
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
		Player player(rectangle, &world.tuning, &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
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

	void check_movement_content(World& world, const ContentVariant& content)
	{
		const auto rejected = [&](auto edit, const std::string& key, bool resources = false)
		{
			bool invalid = false;
			try
			{
				const GameTuning tuning = content.tuning(edit);
				if (resources) { tuning.validate_resources(world.render, world.audio); }
			}
			catch (const std::exception& error)
			{
				const std::string message = error.what();
				invalid = message.find("movement_sounds") != std::string::npos &&
					message.find(key) != std::string::npos;
			}
			require(invalid, "Invalid movement sound content was accepted or lost its authored key");
		};
		for (const char* key : { "land", "step", "step_painted" })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["player"]["movement_sounds"][key].Clear();
			}, key);
			const auto& sounds = world.tuning.player().movement_sounds;
			const auto& choices = std::string(key) == "land" ? sounds.land :
				(std::string(key) == "step" ? sounds.step : sounds.step_painted);
			for (size_t index = 0; index < choices.size(); ++index)
			{
				rejected([&](rapidjson::Document& json)
				{
					json["player"]["movement_sounds"][key][static_cast<rapidjson::SizeType>(index)]["name"].SetString(
						"missing.player.movement", json.GetAllocator());
				}, std::string(key) + "[" + std::to_string(index) + "]", true);
				for (float gain : { -0.01f, 1.01f })
				{
					rejected([&](rapidjson::Document& json)
					{
						json["player"]["movement_sounds"][key][static_cast<rapidjson::SizeType>(index)]["volume"].SetFloat(gain);
					}, key);
				}
			}
		}
		rejected([](rapidjson::Document& json)
		{
			json["player"]["movement_sounds"]["skid"]["name"].SetString("missing.player.skid", json.GetAllocator());
		}, "skid", true);
		for (float gain : { -0.01f, 1.01f })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["player"]["movement_sounds"]["skid"]["volume"].SetFloat(gain);
			}, "skid");
		}
		for (const char* key : { "stride_distance", "min_step_interval", "skid_min_speed", "skid_repeat_delay", "landing_full_speed" })
		{
			for (float value : { 0.0f, -1.0f })
			{
				rejected([&](rapidjson::Document& json)
				{
					json["player"]["movement_sounds"][key].SetFloat(value);
				}, key);
			}
		}
		for (float value : { -0.01f, 1.01f })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["player"]["movement_sounds"]["landing_min_gain"].SetFloat(value);
			}, "landing_min_gain");
		}
	}

	// Actual players, factory weapons and collision resolution. Audio is observed
	// only after every contact, as in Level's normal simulation phase ordering.
	struct MovementCourse
	{
		World& world;
		const GameTuning& tuning;
		Scene scene{ nullptr, nullptr };
		std::vector<Player*> players;
		std::vector<float> downward_speed;

		MovementCourse(World& resources, const GameTuning& balance) : world(resources), tuning(balance) {}

		Player& add(float x = 0.0f, float bottom = 0.0f, float speed = 0.0f)
		{
			using namespace mattmath;
			const int index = static_cast<int>(this->players.size());
			const auto team = index < 2 ? PlayerTeam::a : PlayerTeam::b;
			const RectangleF rectangle(x, bottom - this->tuning.player().size.y,
				this->tuning.player().size.x, this->tuning.player().size.y);
			Player* player = this->scene.add(std::make_unique<Player>(rectangle, &this->tuning,
				&this->world.presentation.movement_feedback(), &this->world.presentation.combat_feedback(), &this->world.presentation.respawn_redraw(),
				&this->world.render, &this->world.audio, &this->world.preferences, index, team,
				WeaponType::sprayer, this->world.weapon(this->tuning, WeaponType::sprayer, index, team),
				WeaponType::none, Colour::blue, Viewport{}, rectangle.top_left(), Vector2F(speed, 0.0f)));
			this->players.push_back(player);
			this->scene.end_tick();
			return *player;
		}

		void surface(const mattmath::RectangleF& rectangle,
			CollisionObjectType type = CollisionObjectType::structure)
		{
			this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
				std::make_unique<mattmath::RectangleF>(rectangle), &this->world.render,
				type));
			this->scene.end_tick();
		}

		void floor() { this->surface({ -10000.0f, 0.0f, 20000.0f, 100.0f }); }

		void ramp(const mattmath::TriangleRightAxisAligned& shape, CollisionObjectType type)
		{
			this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4", shape.bounding_box(),
				std::make_unique<mattmath::TriangleRightAxisAligned>(shape), &this->world.render, type));
			this->scene.end_tick();
		}

		void tick(const PlayerInputData& input = {}, float dt = weapon_step, bool fatal = false)
		{
			for (auto* player : this->players) { player->set_player_input(input); }
			this->scene.update(dt);
			this->downward_speed.clear();
			for (auto* player : this->players) { this->downward_speed.push_back(player->velocity().y); }
			for (auto* player : this->players) { player->update_weapon_and_get_projectiles(dt, {}); }
			this->scene.resolve();
			for (auto* player : this->players)
			{
				if (fatal) { player->kill(); }
				player->end_contacts(this->scene.contacts());
				player->update_prev_rectangle();
			}
			this->scene.end_tick();
		}

		void land()
		{
			for (int tick = 0; tick < 600; ++tick)
			{
				this->tick();
				if (std::all_of(this->players.begin(), this->players.end(), [](const Player* player)
					{ return player->move_state() == PlayerMoveState::on_ground; })) { return; }
			}
			throw std::runtime_error("Movement audio fixture did not reach the floor");
		}
	};

	PlayerInputData movement_input(float x, bool jump = false)
	{
		PlayerInputData input;
		input.x_movement = x;
		input.jump_pressed = jump;
		input.jump_held = jump;
		return input;
	}

	int choice_plays(const World& world, const std::vector<SoundTuning>& choices)
	{
		int count = 0;
		for (const auto& choice : choices) { count += world.wave_plays(choice.name); }
		return count;
	}

	void check_movement_landings(World& world)
	{
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		const auto& sounds = world.tuning.player().movement_sounds;
		MovementCourse course(world, world.tuning);
		Player& player = course.add(0.0f, -300.0f);
		course.floor();
		course.land();
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Spawn falling/settling played movement audio");
		for (size_t landing = 0; landing < sounds.land.size() + 1; ++landing)
		{
			course.tick(movement_input(0.0f, true));
			require(player.velocity().y < 0.0f, "Movement audio fixture failed to jump");
			course.tick({}, 0.0f);
			course.land();
			const auto& sound = sounds.land[landing % sounds.land.size()];
			const float impact_gain = std::clamp(course.downward_speed.front() / sounds.landing_full_speed,
				sounds.landing_min_gain, 1.0f);
			require(choice_plays(world, sounds.land) == static_cast<int>(landing + 1) &&
				world.wave_plays(sound.name) ==
				static_cast<int>(landing / sounds.land.size() + 1),
				"Completed jump landing missed, repeated or failed to cycle its variants");
			require(close_to(world.last_wave_volume(sound), sound.volume * impact_gain * 0.4f),
				"Landing ignored resolved impact speed or current SFX gain");
			course.tick({}, 0.0f);
			course.tick({}, -weapon_step);
			course.tick();
			require(choice_plays(world, sounds.land) == static_cast<int>(landing + 1),
				"Nonpositive movement ticks or sustained support invented a landing");
		}
		require(choice_plays(world, sounds.step) == 0 && world.wave_plays(sounds.skid.name) == 0,
			"Vertical jumps emitted footsteps or skids");
		player.force_respawn();
		world.begin = recorded_sounds(world.device).size();
		course.land();
		require(choice_plays(world, sounds.land) == 0, "Respawn fall/settling played a landing");
		world.preferences.sfx_volume = 0;
		course.tick(movement_input(0.0f, true));
		course.land();
		require(choice_plays(world, sounds.land) == 1, "Respawn did not rearm its next ordinary landing");
		for (const auto& sound : sounds.land)
		{
			if (world.wave_plays(sound.name) > 0) { world.wave_volume_is(sound, 0.0f); }
		}

		// A fatal hit can arrive after update and ground resolution, before the
		// final state changes to dead on the following update.
		course.tick(movement_input(0.0f, true));
		bool next_contact = false;
		for (int tick = 0; tick < 600; ++tick)
		{
			const float next_speed = std::min(player.velocity().y + world.tuning.player().gravity * weapon_step,
				world.tuning.player().max_velocity.y);
			if (next_speed > 0.0f && player.rectangle().bottom() + next_speed * weapon_step >= 0.0f)
			{
				next_contact = true;
				break;
			}
			course.tick();
		}
		require(next_contact, "Fatal movement audio fixture never approached its landing");
		world.begin = recorded_sounds(world.device).size();
		course.tick({}, weapon_step, true);
		require(player.state() == PlayerState::alive && player.health() <= 0.0f,
			"Movement audio fatal-contact fixture missed its boundary");
		for (int tick = 0; tick < 30; ++tick) { course.tick(movement_input(1.0f)); }
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Fatal/dead player emitted movement audio");
	}

	void check_movement_impact_gain(World& world)
	{
		const auto& sounds = world.tuning.player().movement_sounds;
		world.preferences.sfx_volume = 40;
		float short_volume = 0.0f;
		for (float drop : { 20.0f, 1000.0f })
		{
			MovementCourse course(world, world.tuning);
			Player& player = course.add(0.0f, 0.0f, world.tuning.player().max_velocity.x);
			course.surface({ -1000.0f, 0.0f, 1020.0f, 100.0f });
			course.surface({ -10000.0f, drop, 20000.0f, 100.0f });
			course.tick(movement_input(1.0f));
			require(player.move_state() == PlayerMoveState::on_ground, "Impact fixture did not start supported");
			world.begin = recorded_sounds(world.device).size();
			bool airborne = false;
			bool landed = false;
			for (int tick = 0; tick < 600; ++tick)
			{
				course.tick(movement_input(1.0f));
				airborne = airborne || player.move_state() == PlayerMoveState::in_air;
				if (airborne && player.move_state() == PlayerMoveState::on_ground) { landed = true; break; }
			}
			require(landed && choice_plays(world, sounds.land) == 1, "Walked-off drop missed or repeated its landing");
			const float gain = std::clamp(course.downward_speed.front() / sounds.landing_full_speed,
				sounds.landing_min_gain, 1.0f);
			const float volume = world.last_wave_volume(sounds.land.front());
			require(close_to(volume, sounds.land.front().volume * gain * 0.4f),
				"Small/large drop landing ignored authored impact scaling");
			if (drop == 20.0f) { short_volume = volume; }
			else { require(volume > short_volume, "Long fall did not produce a stronger landing than a short drop"); }
		}
	}

	void check_movement_steps_and_walls(World& world)
	{
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		const auto& sounds = world.tuning.player().movement_sounds;
		MovementCourse course(world, world.tuning);
		Player& player = course.add();
		// Adjacent floor rectangles exercise the completed support state at seams.
		course.surface({ -10000.0f, 0.0f, 10300.0f, 100.0f });
		course.surface({ 300.0f, 0.0f, 10000.0f, 100.0f });
		course.land();
		float distance = 0.0f;
		float since_step = 1000.0f;
		int steps = 0;
		for (int tick = 0; tick < 240; ++tick)
		{
			const float before = player.rectangle().left();
			course.tick(movement_input(1.0f));
			distance += std::abs(player.rectangle().left() - before);
			since_step += weapon_step;
			const int count = choice_plays(world, sounds.step);
			require(count <= steps + 1 && distance + 0.001f >= count * sounds.stride_distance,
				"Footsteps burst or played without resolved grounded stride distance");
			if (count != steps)
			{
				require(since_step + 0.0001f >= sounds.min_step_interval &&
					world.wave_plays(sounds.step[static_cast<size_t>(steps) % sounds.step.size()].name) ==
					steps / static_cast<int>(sounds.step.size()) + 1,
					"Footstep rate limit or deterministic variant cycle failed");
				since_step = 0.0f;
				steps = count;
			}
		}
		require(steps >= static_cast<int>(sounds.step.size()) + 1 && choice_plays(world, sounds.land) == 0,
			"Ground travel missed footsteps or invented landings at floor seams");
		for (const auto& sound : sounds.step) { world.wave_volume_is(sound, 0.4f); }
		const float wall_x = player.rectangle().right() + 1.0f;
		course.surface({ wall_x, -1000.0f, 100.0f, 1000.0f });
		for (int tick = 0; tick < 120; ++tick) { course.tick(movement_input(1.0f)); }
		const int blocked_steps = choice_plays(world, sounds.step);
		for (int tick = 0; tick < 120; ++tick) { course.tick(movement_input(1.0f)); }
		require(choice_plays(world, sounds.step) == blocked_steps && choice_plays(world, sounds.land) == 0,
			"Pushing against a wall or contact separation invented steps/landings");
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(-1.0f, true));
		for (int tick = 0; tick < 12; ++tick) { course.tick(movement_input(-1.0f)); }
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Airborne horizontal movement emitted ground cues");
	}

	void check_movement_contact_kinds(World& world)
	{
		using namespace mattmath;
		const auto& sounds = world.tuning.player().movement_sounds;
		MovementCourse buffered(world, world.tuning);
		Player& player = buffered.add();
		buffered.floor();
		buffered.land();
		world.begin = recorded_sounds(world.device).size();
		buffered.tick(movement_input(0.0f, true));
		bool queued = false;
		for (int tick = 0; tick < 150 && choice_plays(world, sounds.land) == 0; ++tick)
		{
			const bool press = !queued && player.velocity().y > 0.0f && player.rectangle().bottom() > -25.0f;
			buffered.tick(movement_input(0.0f, press));
			queued = queued || press;
		}
		require(queued && player.velocity().y < 0.0f && choice_plays(world, sounds.land) == 1 &&
			choice_plays(world, sounds.step) == 0, "Buffered landing/takeoff missed its landing or invented a step");
		buffered.land();
		require(choice_plays(world, sounds.land) == 2, "Buffered takeoff failed to rearm its next landing");

		MovementCourse platform(world, world.tuning);
		platform.add();
		platform.surface({ -10000.0f, 0.0f, 20000.0f, 20.0f }, CollisionObjectType::structure_jump_through);
		platform.surface({ -10000.0f, 300.0f, 20000.0f, 100.0f });
		platform.tick();
		world.begin = recorded_sounds(world.device).size();
		PlayerInputData drop;
		drop.drop_through = true;
		platform.tick(drop);
		platform.land();
		require(choice_plays(world, sounds.land) == 1 && choice_plays(world, sounds.step) == 0,
			"Drop-through platform missed its lower-floor landing or invented footsteps");

		for (bool uphill_right : { false, true })
		{
			const float gradient = 450.0f / 1400.0f;
			const float height = 1000.0f * gradient;
			MovementCourse slope(world, world.tuning);
			slope.add(0.0f, uphill_right ? -world.tuning.player().size.x * gradient : 0.0f);
			slope.ramp(uphill_right ? TriangleRightAxisAligned(1000, -height, -1000, height, 1000, height) :
				TriangleRightAxisAligned(-1000, -height, -1000, height, 1000, height),
				uphill_right ? CollisionObjectType::structure_ramp_right : CollisionObjectType::structure_ramp_left);
			slope.tick();
			world.begin = recorded_sounds(world.device).size();
			slope.tick(movement_input(0.0f, true));
			for (int tick = 0; tick < 120; ++tick) { slope.tick(); }
			require(choice_plays(world, sounds.land) == 1 && choice_plays(world, sounds.step) == 0,
				"Ramp hop missed or repeated its landing under sustained slope gravity");
			for (int tick = 0; tick < 15; ++tick) { slope.tick(movement_input(uphill_right ? 1.0f : -1.0f)); }
			for (int tick = 0; tick < 60; ++tick) { slope.tick(movement_input(uphill_right ? -1.0f : 1.0f)); }
			require(choice_plays(world, sounds.land) == 1 && choice_plays(world, sounds.step) > 0,
				"Slope travel invented landings or failed to count resolved grounded strides");
		}
	}

	void check_movement_skids_and_players(World& world, const ContentVariant& content)
	{
		// Slow authored deceleration keeps the opposite-input condition true long
		// enough to distinguish an edge cue from a held/repeating cue.
		const GameTuning tuning = content.tuning([](rapidjson::Document& json)
		{
			json["player"]["x_acceleration"].SetFloat(1.0f);
			json["player"]["x_deacceleration"].SetFloat(1.0f);
		});
		const auto& sounds = tuning.player().movement_sounds;
		MovementCourse course(world, tuning);
		for (int index = 0; index < 4; ++index) { course.add(index * 2000.0f, 0.0f, tuning.player().max_velocity.x); }
		course.floor();
		course.tick(movement_input(1.0f));
		world.preferences.sfx_volume = 40;
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 4, "Four players shared or missed skid edges");
		for (int tick = 0; tick < 10; ++tick) { course.tick(movement_input(-1.0f), 0.0f); }
		course.tick(movement_input(1.0f), -weapon_step);
		course.tick(movement_input(1.0f));
		course.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 4, "Nonpositive ticks or rapid reversal bypassed skid cooldown");
		for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(-1.0f)); }
		require(world.wave_plays(sounds.skid.name) == 4, "Held opposite input repeated its skid edge");
		course.tick(movement_input(1.0f));
		course.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 8, "Fresh reversal after skid delay did not rearm per player");
		world.wave_volume_is(sounds.skid, 0.4f);
		for (const auto& sound : sounds.step)
		{
			require(world.wave_plays(sound.name) > 0, "Four-player travel failed to cycle independent footsteps");
		}
		world.preferences.sfx_volume = 0;
		for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(1.0f)); }
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(-1.0f));
		for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(-1.0f)); }
		world.wave_volume_is(sounds.skid, 0.0f);
		for (const auto& sound : sounds.step) { world.wave_volume_is(sound, 0.0f); }
		world.begin = recorded_sounds(world.device).size();
		course.tick(movement_input(0.0f, true));
		for (int tick = 0; tick < 4; ++tick) { course.tick(movement_input(-1.0f)); }
		require(choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0 &&
			world.wave_plays(sounds.skid.name) == 0, "Airborne high-speed reversal emitted ground movement cues");
		course.land();
		for (const auto& sound : sounds.land)
		{
			require(world.wave_plays(sound.name) == 1, "Four-player landings shared a variant or support history");
			world.wave_volume_is(sound, 0.0f);
		}
		MovementCourse slow(world, tuning);
		slow.add(0.0f, 0.0f, sounds.skid_min_speed - 1.0f);
		slow.floor();
		slow.tick(movement_input(1.0f));
		world.begin = recorded_sounds(world.device).size();
		slow.tick(movement_input(-1.0f));
		require(world.wave_plays(sounds.skid.name) == 0, "Below-threshold reversal emitted a skid");
	}

	void check_paint_content(World& world, const ContentVariant& content)
	{
		const auto rejected = [&](auto edit, const std::string& key, bool resources = false)
		{
			bool invalid = false;
			std::string message = "no exception";
			try
			{
				const GameTuning tuning = content.tuning(edit);
				if (resources) { tuning.validate_resources(world.render, world.audio); }
			}
			catch (const std::exception& error)
			{
				message = error.what();
				invalid = message.find(": paint") != std::string::npos && message.find(key) != std::string::npos;
			}
			if (!invalid) { throw std::runtime_error("Invalid paint audio content '" + key + "': " + message); }
		};
		rejected([](rapidjson::Document& json) { json["paint"]["splash_sounds"].Clear(); }, "splash_sounds");
		for (size_t index = 0; index < world.tuning.paint().splash_sounds.size(); ++index)
		{
			rejected([&](rapidjson::Document& json)
			{
				json["paint"]["splash_sounds"][static_cast<rapidjson::SizeType>(index)]["name"].SetString(
					"missing.paint.splash", json.GetAllocator());
			}, "splash_sounds", true);
			for (float gain : { -0.01f, 1.01f })
			{
				rejected([&](rapidjson::Document& json)
				{
					json["paint"]["splash_sounds"][static_cast<rapidjson::SizeType>(index)]["volume"].SetFloat(gain);
				}, "splash_sounds");
			}
		}
		for (const char* key : { "capture_sound", "dud_sound" })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["paint"][key]["name"].SetString("missing.paint.feedback", json.GetAllocator());
			}, key, true);
			for (float gain : { -0.01f, 1.01f })
			{
				rejected([&](rapidjson::Document& json) { json["paint"][key]["volume"].SetFloat(gain); }, key);
			}
		}
		for (const char* tool : { "spray", "jet", "rolling", "mist", "ball" })
		{
			rejected([&](rapidjson::Document& json)
			{
				json["paint"]["impact_sounds"][tool]["name"].SetString("missing.paint.impact", json.GetAllocator());
			}, std::string("impact_sounds.") + tool, true);
			for (float gain : { -0.01f, 1.01f })
			{
				rejected([&](rapidjson::Document& json)
				{
					json["paint"]["impact_sounds"][tool]["volume"].SetFloat(gain);
				}, "impact_sounds");
			}
		}
		for (const char* key : { "min_impact_interval", "min_capture_interval" })
		{
			for (float value : { 0.0f, -1.0f })
			{
				rejected([&](rapidjson::Document& json) { json["paint"][key].SetFloat(value); }, key);
			}
		}
	}

	void check_paint_limits_and_variants(World& world, const ContentVariant& content)
	{
		const GameTuning tuning = content.tuning([](rapidjson::Document& json)
		{
			json["paint"]["min_impact_interval"].SetFloat(0.2f);
			json["paint"]["min_capture_interval"].SetFloat(0.4f);
		});
		const auto& paint = tuning.paint();
		PaintAudio audio(&paint, &world.audio, &world.preferences);
		world.preferences.sfx_volume = 35;
		world.begin = recorded_sounds(world.device).size();
		audio.update(weapon_step);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1,
			"Fresh painted contact did not emit one splash and one capture");
		world.wave_volume_is(paint.splash_sounds[0], 0.35f);
		world.wave_volume_is(paint.capture_sound, 0.35f);
		for (int repeat = 0; repeat < 20; ++repeat)
		{
			audio.contact(ProjectileType::spray, 0, true, true);
			audio.contact(ProjectileType::spray, 0, false, false);
		}
		audio.update(0.0f);
		audio.update(-1.0f);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1 &&
			world.wave_plays(paint.dud_sound.name) == 0,
			"Repeated paint/dud contacts or nonpositive updates bypassed the shared rate limit");
		audio.update(0.201f);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(choice_plays(world, paint.splash_sounds) == 2 && world.wave_plays(paint.capture_sound.name) == 1,
			"Impact and capture cooldowns did not remain independent");
		audio.update(0.201f);
		audio.contact(ProjectileType::spray, 0, true, true);
		require(world.wave_plays(paint.capture_sound.name) == 2, "Capture did not recover after its authored interval");
		for (size_t index = 3; index < paint.splash_sounds.size(); ++index)
		{
			audio.update(0.201f);
			audio.contact(ProjectileType::spray, 0, true, false);
		}
		for (const auto& sound : paint.splash_sounds)
		{
			require(world.wave_plays(sound.name) == 1, "Audible splash variants skipped or repeated before cycling");
		}
		audio.update(100.0f);
		audio.contact(ProjectileType::spray, 0, false, false);
		audio.contact(ProjectileType::spray, 0, true, false);
		require(world.wave_plays(paint.dud_sound.name) == 1 && choice_plays(world, paint.splash_sounds) ==
			static_cast<int>(paint.splash_sounds.size()), "Long update created an impact backlog or separate dud budget");

		PaintAudio independent(&paint, &world.audio, &world.preferences);
		world.preferences.sfx_volume = 0;
		world.begin = recorded_sounds(world.device).size();
		independent.update(weapon_step);
		for (int owner = 0; owner < 4; ++owner)
		{
			independent.contact(ProjectileType::spray, owner, true, true);
			independent.contact(ProjectileType::spray, owner, true, true);
		}
		require(choice_plays(world, paint.splash_sounds) == 4 && world.wave_plays(paint.capture_sound.name) == 4,
			"Four paint owners shared a cooldown or failed to coalesce repeated contacts");
		for (size_t index = 0; index < 4; ++index)
		{
			require(world.wave_plays(paint.splash_sounds[index].name) == 1, "Paint owners shared their initial splash variant");
			world.wave_volume_is(paint.splash_sounds[index], 0.0f);
		}
		world.wave_volume_is(paint.capture_sound, 0.0f);
		world.preferences.sfx_volume = 65;
		independent.update(0.401f);
		independent.contact(ProjectileType::spray, 0, true, true);
		require(close_to(world.last_wave_volume(paint.splash_sounds[1]), paint.splash_sounds[1].volume * 0.65f),
			"Paint feedback retained the old SFX preference");
		world.begin = recorded_sounds(world.device).size();
		independent.contact(ProjectileType::spray, paint_owners::index(-1, 0), true, false);
		independent.contact(ProjectileType::spray, paint_owners::index(-1, 1), true, false);
		independent.contact(ProjectileType::spray, -1, true, false);
		independent.contact(ProjectileType::spray, 1000, true, false);
		require(choice_plays(world, paint.splash_sounds) == 3,
			"Device-less paint owners shared a budget or invalid owners escaped their bounded fallback");

		AudioResources silent;
		silent.add_sound_bank(paint.sound_bank, SoundBank::silent());
		PaintAudio fallback(&paint, &silent, &world.preferences);
		fallback.update(weapon_step);
		fallback.contact(ProjectileType::ball, 0, true, true);
		fallback.update(1.0f);
		fallback.contact(ProjectileType::ball, 0, false, false);
	}

	std::unique_ptr<StructurePaintable> paint_surface(World& world, const mattmath::RectangleF& rectangle,
		const PaintableFaces& faces = { false, true, false, false })
	{
		return std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4", rectangle,
			std::make_unique<mattmath::RectangleF>(rectangle), &world.render, &world.audio,
			&world.preferences, &world.tuning.paint(), CollisionObjectType::structure_paintable, TeamColour{}, faces);
	}

	auto paint_projectiles(World& world, ProjectileType type, PlayerTeam team = PlayerTeam::a,
		int owner = 0, mattmath::Vector2F position = { 50.0f, 0.0f },
		mattmath::Vector2F velocity = mattmath::Vector2F::ZERO)
	{
		return ProjectileBuilder(&world.tuning).build_projectiles(position, velocity, team, owner,
			Colour::blue, type, &world.render);
	}

	void paint_contact(Projectile& projectile, StructurePaintable& surface, bool projectile_first = false)
	{
		if (projectile_first) { projectile.on_contact(surface, mattmath::Vector2F::ZERO, 0.0f); }
		surface.on_contact(projectile, mattmath::Vector2F::ZERO, 0.0f);
		if (!projectile_first) { projectile.on_contact(surface, mattmath::Vector2F::ZERO, 0.0f); }
	}

	void check_paint_contacts(World& world)
	{
		using namespace mattmath;
		const auto& paint = world.tuning.paint();
		world.preferences.sfx_volume = 50;
		for (const auto type : { ProjectileType::spray, ProjectileType::jet, ProjectileType::rolling,
			ProjectileType::mist, ProjectileType::ball })
		{
			for (bool projectile_first : { false, true })
			{
				PaintAudio audio(&paint, &world.audio, &world.preferences);
				audio.update(weapon_step);
				auto surface = paint_surface(world, { 0, 0, 100, 50 });
				auto shots = paint_projectiles(world, type);
				world.begin = recorded_sounds(world.device).size();
				for (auto& shot : shots)
				{
					paint_contact(*shot, *surface, projectile_first);
					shot->flush_paint_audio(audio);
					shot->flush_paint_audio(audio);
				}
				require(surface->paint_total().team_a > 0.0f && choice_plays(world, paint.splash_sounds) == 1 &&
					world.wave_plays(paint.capture_sound.name) == 1 && world.wave_plays(paint.dud_sound.name) == 0,
					"Actual tool contact lost paint, repeated its volley, or depended on callback order");
				const auto& impact = paint.impact_sounds[static_cast<size_t>(type)];
				require(world.wave_plays(impact.name) == 1, "Painted projectile did not use its tool's impact cue");
				world.wave_volume_is(impact, 0.5f);
				const float captured = surface->paint_total().player_areas[0];
				audio.update(std::max(paint.min_impact_interval, paint.min_capture_interval) + 0.001f);
				auto friendly = paint_projectiles(world, type, PlayerTeam::a, 1);
				world.begin = recorded_sounds(world.device).size();
				for (auto& shot : friendly) { paint_contact(*shot, *surface); shot->flush_paint_audio(audio); }
				require(world.wave_plays(paint.capture_sound.name) == 0 &&
					close_to(surface->paint_total().player_areas[0], captured) && surface->paint_total().player_areas[1] == 0.0f,
					"Friendly repaint emitted capture feedback or transferred scoring credit");
				audio.update(std::max(paint.min_impact_interval, paint.min_capture_interval) + 0.001f);
				auto enemy = paint_projectiles(world, type, PlayerTeam::b, 1);
				world.begin = recorded_sounds(world.device).size();
				for (auto& shot : enemy) { paint_contact(*shot, *surface); shot->flush_paint_audio(audio); }
				require(world.wave_plays(paint.capture_sound.name) == 1 && surface->paint_total().team_b > 0.0f &&
					surface->paint_total().player_areas[1] > 0.0f, "Enemy repaint missed capture feedback or scoring transfer");
			}
		}

		PaintAudio audio(&paint, &world.audio, &world.preferences);
		audio.update(weapon_step);
		auto first = paint_surface(world, { 0, 0, 100, 50 });
		auto second = paint_surface(world, { 0, 0, 100, 50 });
		auto unpaintable = paint_surface(world, { 0, 0, 100, 50 }, {});
		auto rollers = paint_projectiles(world, ProjectileType::rolling);
		world.begin = recorded_sounds(world.device).size();
		paint_contact(*rollers[0], *unpaintable);
		paint_contact(*rollers[0], *first);
		paint_contact(*rollers[0], *second);
		rollers[0]->flush_paint_audio(audio);
		require(!rollers[0]->for_deletion() && first->paint_total().team_a > 0.0f && second->paint_total().team_a > 0.0f &&
			choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1 &&
			world.wave_plays(paint.dud_sound.name) == 0, "Roller contacts did not aggregate across its full painting volume");
		for (float dt : { 0.0f, -1.0f })
		{
			audio.update(dt);
			auto pending = paint_projectiles(world, ProjectileType::jet);
			paint_contact(*pending[0], *first);
			world.begin = recorded_sounds(world.device).size();
			pending[0]->flush_paint_audio(audio);
			audio.update(10.0f);
			pending[0]->flush_paint_audio(audio);
			require(choice_plays(world, paint.splash_sounds) == 0 && world.wave_plays(paint.capture_sound.name) == 0,
				"Nonpositive paint step emitted or retained a catch-up contact");
		}
	}

	void check_paint_duds_and_launch(World& world)
	{
		using namespace mattmath;
		const auto& paint = world.tuning.paint();
		for (bool launch : { false, true })
		{
			PaintAudio audio(&paint, &world.audio, &world.preferences);
			audio.update(weapon_step);
			const RectangleF rectangle(0, -50, 5, 100);
			Structure cover("sprite_sheet_1", "square_white_4", rectangle,
				std::make_unique<RectangleF>(rectangle), &world.render, CollisionObjectType::structure);
			CollisionObject* objects[]{ &cover };
			auto shots = paint_projectiles(world, ProjectileType::jet, PlayerTeam::a, 0,
				launch ? Vector2F(20, 0) : Vector2F(-20, 0), { 1000, 0 });
			world.begin = recorded_sounds(world.device).size();
			if (launch) { shots[0]->resolve_launch_obstruction({ -20, 0 }, objects); }
			else { shots[0]->update(0.05f); shots[0]->resolve_movement_contacts(objects); }
			shots[0]->flush_paint_audio(audio);
			shots[0]->flush_paint_audio(audio);
			require(shots[0]->for_deletion() && world.wave_plays(paint.dud_sound.name) == 1 &&
				choice_plays(world, paint.splash_sounds) == 0 && world.wave_plays(paint.capture_sound.name) == 0,
				"Swept or launch bare-geometry contact missed or duplicated its dud");
		}
		PaintAudio audio(&paint, &world.audio, &world.preferences);
		audio.update(weapon_step);
		auto no_faces = paint_surface(world, { 0, 0, 100, 50 }, {});
		auto shot = paint_projectiles(world, ProjectileType::jet);
		world.begin = recorded_sounds(world.device).size();
		paint_contact(*shot[0], *no_faces, true);
		shot[0]->flush_paint_audio(audio);
		require(world.wave_plays(paint.dud_sound.name) == 1 && choice_plays(world, paint.splash_sounds) == 0,
			"Paintable geometry with no touched scoring face did not classify as a dud");
		audio.update(paint.min_impact_interval + 0.001f);
		world.begin = recorded_sounds(world.device).size();
		auto lifetime = paint_projectiles(world, ProjectileType::jet);
		lifetime[0]->update(weapon_step);
		lifetime[0]->update(world.tuning.projectile(ProjectileType::jet).details.delete_timer + 1.0f);
		lifetime[0]->flush_paint_audio(audio);
		require(lifetime[0]->for_deletion() && world.wave_plays(paint.dud_sound.name) == 0,
			"Lifetime retirement invented a geometry dud");

		// The actual Player launch path discards obstructed shots before returning
		// them to Level. Their accumulated audio must be flushed before that erase.
		const RectangleF player_rectangle(Vector2F::ZERO, world.tuning.player().size);
		Player player(player_rectangle, &world.tuning, &world.presentation.movement_feedback(), &world.presentation.combat_feedback(),
			&world.presentation.respawn_redraw(), &world.render, &world.audio, &world.preferences,
			0, PlayerTeam::a, WeaponType::sniper, world.weapon(world.tuning, WeaponType::sniper),
			WeaponType::none, Colour::blue, Viewport{}, Vector2F::ZERO);
		player.set_paint_audio(&audio);
		const RectangleF enclosure(-1000, -1000, 2000, 2000);
		Structure cover("sprite_sheet_1", "square_white_4", enclosure,
			std::make_unique<RectangleF>(enclosure), &world.render, CollisionObjectType::structure);
		CollisionObject* objects[]{ &cover };
		PlayerInputData held;
		held.primary_shoot = true;
		player.set_player_input(held);
		audio.update(paint.min_impact_interval + 0.001f);
		world.begin = recorded_sounds(world.device).size();
		const auto& sniper = world.tuning.weapon(WeaponType::sniper);
		player.update_weapon_and_get_projectiles(sniper.details.shoot_interval + 0.001f, objects);
		const auto launched = player.update_weapon_and_get_projectiles(weapon_step, objects);
		require(launched.empty() && world.wave_plays(sniper.details.shoot_sound_name) == 1 &&
			world.wave_plays(paint.dud_sound.name) == 1, "Player erased an obstructed paid shot before emitting its launch dud");
	}

	void check_level_paint_after_owner_death(World& world)
	{
		const auto& paint = world.tuning.paint();
		world.build(OpeningMode::off, 2, 10.0f);
		world.until([&] { return world.level->state() == LevelState::active; });
		world.level->set_player_cheats(true, true);
		require(world.level->place_player(0, { 2000.0f, 4700.0f }), "Cannot place Level paint audio fixture");
		PaintForReveal fire(0);
		for (int tick = 0; tick < 60 && world.level->projectile_count() == 0; ++tick)
		{
			world.level->update(world.idle, weapon_step, &fire);
		}
		require(world.level->projectile_count() > 0, "Level fixture did not admit a flying paint projectile");
		world.begin = recorded_sounds(world.device).size();
		require(world.level->kill_player(0), "Cannot kill Level paint owner fixture");
		world.level->update(world.idle, 0.0f, &fire);
		world.level->update(world.idle, -1.0f, &fire);
		require(choice_plays(world, paint.splash_sounds) == 0 && world.wave_plays(paint.capture_sound.name) == 0,
			"Nonpositive Level updates emitted pending paint feedback");
		for (int tick = 0; tick < 90; ++tick) { world.level->update(world.idle, weapon_step); }
		require(world.level->player(0).state() == PlayerState::dead &&
			choice_plays(world, paint.splash_sounds) == 1 && world.wave_plays(paint.capture_sound.name) == 1 &&
			world.wave_plays(paint.impact_sounds[static_cast<size_t>(ProjectileType::spray)].name) == 1 &&
			world.level->level_end_info().team_a_area > 0.0f,
			"Level failed to flush a flying paint hit after its owner died");
	}

#include "scripts/acceptance/story_audio/painted_checks.h"
#include "scripts/acceptance/story_audio/connection_checks.h"
#include "scripts/acceptance/story_audio/resume_checks.h"
#include "scripts/acceptance/story_audio/menu_content_checks.h"
#include "scripts/acceptance/story_audio/main_menu_content_checks.h"
#include "scripts/acceptance/story_audio/controls_content_checks.h"
#include "scripts/acceptance/story_audio/resume_content_checks.h"
#include "scripts/acceptance/story_audio/closing_content_checks.h"
#include "scripts/acceptance/story_audio/hud_content_checks.h"
#include "scripts/acceptance/story_audio/end_menu_content_checks.h"
#include "scripts/acceptance/story_audio/pause_menu_content_checks.h"
#include "scripts/acceptance/story_audio/results_menu_content_checks.h"
}

#include "scripts/acceptance/story_audio/redraw_checks.h"
#include "scripts/acceptance/story_audio/match_checks.h"
#include "scripts/acceptance/story_audio/results_music_checks.h"
#include "scripts/acceptance/story_audio/music_mix_checks.h"

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: story_audio.exe <content directory>");
		World world{ std::filesystem::path(argv[1]) };
		check_menu_audio_content(world, std::filesystem::path(argv[1]));
		check_main_menu_art_content(std::filesystem::path(argv[1]));
		check_controls_art_content(world, std::filesystem::path(argv[1]));
		check_resume_content(world, std::filesystem::path(argv[1]));
		check_foreground_art_content(std::filesystem::path(argv[1]));
		check_countdown_art_content(std::filesystem::path(argv[1]));
		check_closing_art_content(std::filesystem::path(argv[1]));
		check_hud_paper_content(std::filesystem::path(argv[1]));
		check_end_menu_art_content(std::filesystem::path(argv[1]));
		check_pause_menu_art_content(std::filesystem::path(argv[1]));
		check_results_menu_art_content(std::filesystem::path(argv[1]));
		check_connection_audio(world);
		check_resume_audio(world);
		check_resume_weapon_audio(world);
		check_results_music_content(world, std::filesystem::path(argv[1]));
		check_results_audio(world);
		check_ending_sting_content(world, std::filesystem::path(argv[1]));
		check_ending_stings(world);
		check_music_mix_content(world, std::filesystem::path(argv[1]));
		check_music_mix_clock_and_gain(world);
		check_music_mix_duck_and_suspend(world);
		check_music_mix_ending(world, std::filesystem::path(argv[1]));
		check_music_mix_silence(world);
		const ContentVariant content(std::filesystem::path(argv[1]) / "tuning.json");
		check_match_content(world, content);
		check_last_ten_boundaries(world);
		check_last_ten_short_and_long_matches(world);
		check_room_tone_boundaries(world);
		check_room_tone_skip_off_and_teardown(world);
		check_feedback_content(world, content);
		check_story(world);
		check_skip_quick_and_solo(world);
		check_opening_details(world);
		check_opening_modes_and_long_ticks(world);
		check_reveal_details(world);
		check_long_reveal(world);
		check_weapon_starts(world);
		check_empty_and_recovered(world, content);
		check_sniper_ready(world, content);
		check_independent_loops(world);
		check_roller_contacts(world);
		check_player_weapon_lifecycle(world);
		check_movement_content(world, content);
		check_movement_landings(world);
		check_movement_impact_gain(world);
		check_movement_steps_and_walls(world);
		check_movement_contact_kinds(world);
		check_movement_skids_and_players(world, content);
		check_redraw_content(world, content);
		check_redraw_boundaries(world, content);
		check_redraw_player_lifecycle(world);
		check_redraw_levels(world);
		check_painted_support_query(world);
		check_painted_player_contacts(world);
		check_painted_audio_budget(world);
		check_painted_landings(world);
		check_paint_content(world, content);
		check_paint_limits_and_variants(world, content);
		check_paint_contacts(world);
		check_paint_duds_and_launch(world);
		check_level_paint_after_owner_death(world);
		std::cout << "PASS story audio: menu cue content, connection edges/aggregation/gains/mute, "
			"resume content/beats/cancellation/gains/mute and stale-fire loop cleanup, "
			"results coda one-shot/focus/gains/mute/completed-state guard/teardown and arena ending-sting content/selection/lifecycle, "
			"arena music raw-clock fades/ducking/restore/preferences/suspension/ending/silent fallback, "
			"opening/reveal content, swatch/undo/colour/glass/skip edges, "
			"winner/draw click/close and per-team reactions, long-step cue order, off/solo, "
			"countdown/GO/last-ten/time up, room-tone loop boundaries, gains/mute, suspension, destruction, weapon feedback validation, "
			"empty/recovered/ready edges, independent loops, stop lifecycle, roller contacts, "
			"movement content, resolved land/step/skid edges, rates, variants and four-player independence, "
			"redraw pen boundaries/voices/lifecycle, painted support ownership and wet movement budgets, "
			"paint content, tool impacts/capture/duds, swept/launch contacts, roller aggregation and shared limits\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL story audio: " << error.what() << '\n';
		return 1;
	}
}
