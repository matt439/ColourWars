#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/weapon_builder.h"
#include "engine/assets/sound_bank_loader.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/null/recording.h"
#include "engine/collision/partitioner.h"
#include "engine/render/sprite_font_file.h"

#include <filesystem>
#include <iostream>
#include <map>

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
			world.wave_plays(sprayer.details.shoot_sound_name) == 1,
			"A dead player left or restarted its held-fire audio");
	}
}

int main(int argc, char** argv)
{
	try
	{
		require(argc == 2, "Usage: story_audio.exe <content directory>");
		World world{ std::filesystem::path(argv[1]) };
		check_story(world);
		check_skip_quick_and_solo(world);
		check_weapon_starts(world);
		std::cout << "PASS story audio: opening/reveal edges, skipped and quick openings, solo, "
			"countdown/GO/time up, gains/mute, suspension, destruction and weapon start edges\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL story audio: " << error.what() << '\n';
		return 1;
	}
}
