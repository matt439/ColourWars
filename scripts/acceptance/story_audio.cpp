#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/level_loaded_info.h"
#include "game/objects/level_object_builder.h"
#include "game/objects/weapon_builder.h"
#include "game/objects/structure.h"
#include "game/objects/structure_paintable.h"
#include "game/objects/terrain_material.h"
#include "game/objects/paint_audio.h"
#include "game/states/connection_audio.h"
#include "game/states/resume_audio.h"
#include "game/states/results_audio.h"
#include "game/sound_groups.h"
#include "game/objects/projectile_jet.h"
#include "engine/assets/sound_bank_loader.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/audio/null/recording.h"
#include "engine/collision/partitioner.h"
#include "engine/math/triangle.h"
#include "engine/math/vector2i.h"
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
		StageList stages;
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
			for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx", "terrain" })
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
			for (const char* stage : { "close_quarters", "king_of_the_hill", "turbulence", "movement_course", "crossfeed", "press_tower" })
			{
				auto info = std::make_unique<LevelLoadedInfo>(
					(content / "levels" / (std::string(stage) + ".json")).string().c_str());
				info->validate();
				this->levels.add(stage, std::move(info));
				this->stages.push_back(Stage{ stage });
			}
			// The groups main.cpp assigns. Every volume change below goes
			// through set_*_volume, as the Controls page's Apply does.
			sound_groups::assign(this->audio, this->tuning, this->presentation,
				this->levels, this->stages);
			sound_groups::apply(this->audio, this->preferences);
			this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render, &this->audio,
				&this->preferences, &this->levels, &this->tuning, &this->presentation, &this->resolution,
				nullptr, &this->partitioner, &this->mouse);
		}

		void set_sfx_volume(int volume)
		{
			this->preferences.sfx_volume = volume;
			sound_groups::apply(this->audio, this->preferences);
		}

		void set_music_volume(int volume)
		{
			this->preferences.music_volume = volume;
			sound_groups::apply(this->audio, this->preferences);
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

#include "scripts/acceptance/story_audio/story_checks.h"
#include "scripts/acceptance/story_audio/weapon_feedback_checks.h"
#include "scripts/acceptance/story_audio/movement_checks.h"
#include "scripts/acceptance/story_audio/paint_checks.h"
#include "scripts/acceptance/story_audio/painted_checks.h"
#include "scripts/acceptance/story_audio/sound_group_checks.h"
#include "scripts/acceptance/story_audio/connection_checks.h"
#include "scripts/acceptance/story_audio/resume_checks.h"
#include "scripts/acceptance/story_audio/menu_content_checks.h"
#include "scripts/acceptance/story_audio/opening_palette_content_checks.h"
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
		check_opening_palette_content(world, std::filesystem::path(argv[1]));
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
		check_sound_groups(world);
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
		std::cout << "PASS story audio: menu cue content, every authored sound in its Music/SFX group, connection edges/aggregation/gains/mute, "
			"resume content/beats/cancellation/gains/mute and stale-fire loop cleanup, "
			"results coda one-shot/focus/gains/mute/completed-state guard/teardown and arena ending-sting content/selection/lifecycle, "
			"arena music raw-clock fades/ducking/restore/preferences/suspension/ending/silent fallback, "
			"opening/reveal content and atlas palette validation, swatch/undo/colour/glass/skip edges, "
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
