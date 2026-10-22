#pragma once

#include "game/states/menu_context.h"
#include "game/states/screen_music.h"
#include <limits>

// Included after story_audio.cpp's recording world and content helpers.
namespace
{
	int music_voice_calls(const World& world, const std::string& name, SoundCall kind)
	{
		int count = 0;
		const auto voice = world.voices.at(name);
		const auto& calls = recorded_sounds(world.device);
		for (size_t index = world.begin; index < calls.size(); ++index)
		{
			count += calls[index].voice == voice && calls[index].call == kind ? 1 : 0;
		}
		return count;
	}

	void check_results_music_content(World& world, const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		require(world.presentation.results_menu().music.name == "music.results",
			"Results music does not use the accepted results coda");
		const auto rejected = [&](auto edit)
		{
			bool named = false;
			try
			{
				const Presentation look = variants.load([&](rapidjson::Document& json)
					{ edit(json["results_menu"]["music"], json.GetAllocator()); });
				look.validate_resources(world.render, world.audio);
			}
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				named = message.find("results_menu") != std::string::npos &&
					message.find("music") != std::string::npos;
			}
			require(named, "Invalid results music did not fail at startup with its field named");
		};
		for (const double invalid : { -0.01, 1.01, 1.0e100 })
		{
			rejected([&](rapidjson::Value& sound, auto&) { sound["volume"].SetDouble(invalid); });
		}
		for (const char* name : { "", "missing.results_music" })
		{
			rejected([&](rapidjson::Value& sound, auto& allocator)
				{ sound["name"].SetString(name, allocator); });
		}
		for (const char* key : { "name", "volume" })
		{
			rejected([&](rapidjson::Value& sound, auto&) { sound.RemoveMember(key); });
		}
		for (const float volume : { 0.0f, 1.0f })
		{
			const Presentation look = variants.load([&](rapidjson::Document& json)
				{ json["results_menu"]["music"]["volume"].SetFloat(volume); });
			look.validate_resources(world.render, world.audio);
			require(look.results_menu().music.volume == volume,
				"Valid results-music gain boundary was not retained");
		}
	}

	void check_screen_music(World& world);
	void check_music_exit_callbacks();

	void check_results_audio(World& world)
	{
		world.level.reset();
		const int original_music = world.preferences.music_volume;
		const int original_sfx = world.preferences.sfx_volume;
		const auto& music = world.presentation.results_menu().music;
		const auto& mix = world.presentation.music_mix();
		world.set_music_volume(35);
		world.set_sfx_volume(0);
		world.begin = recorded_sounds(world.device).size();
		{
			ResultsAudio audio(&world.audio, &world.preferences, world.presentation);
			require(world.plays(music.name) == 1 && world.state(music.name) == SoundState::playing,
				"Results coda did not begin once on its owned voice");
			world.volume_is(music.name, 0.0f);
			require(!audio.finished(), "A fresh silent-gain coda was mistaken for a finished exit");
			audio.update(mix.fade_in * 0.5f);
			require(close_to(world.last_volume(music.name), music.volume * 0.35f * 0.5f),
				"Results coda missed its fade-in midpoint");
			audio.update(mix.fade_in * 0.5f);
			require(close_to(world.last_volume(music.name), music.volume * 0.35f),
				"Results coda failed to reach its authored music gain");
			audio.resume();
			audio.resume();
			require(world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 0,
				"An already playing results coda restarted or received a spurious resume");
			audio.suspend();
			audio.suspend();
			require(world.state(music.name) == SoundState::paused,
				"Focus loss did not preserve the results coda on its paused voice");
			world.set_music_volume(60);
			world.set_sfx_volume(100);
			audio.resume();
			audio.resume();
			require(world.state(music.name) == SoundState::playing && world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 1 &&
				close_to(world.last_volume(music.name), music.volume * 0.6f),
				"Results focus return restarted the coda or ignored current music gain");
			audio.suspend();
			world.set_music_volume(0);
			audio.resume();
			require(world.plays(music.name) == 1 && close_to(world.last_volume(music.name), 0.0f),
				"Results focus return ignored music mute or replayed the coda");

			// The null backend has no clock or drained-buffer event. Put the voice
			// in that same stopped state through its public API to check the guard;
			// this establishes no claim about natural tail duration or playback.
			world.device.stop_voice(world.voices.at(music.name), true);
			const int resumed = music_voice_calls(world, music.name, SoundCall::resume_voice);
			audio.suspend();
			world.set_music_volume(80);
			audio.resume();
			require(world.state(music.name) == SoundState::stopped && world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == resumed,
				"Focus return attempted to resurrect a completed results coda");
		}
		require(world.state(music.name) == SoundState::stopped,
			"Results-audio destruction left its voice alive");
		world.begin = recorded_sounds(world.device).size();
		world.set_music_volume(0);
		{
			ResultsAudio fresh(&world.audio, &world.preferences, world.presentation);
			require(world.plays(music.name) == 1 && close_to(world.last_volume(music.name), 0.0f),
				"A fresh results page failed to start its own muted coda");
			fresh.fade_out();
			require(fresh.finished() && world.state(music.name) == SoundState::stopped,
				"Muted results exit waited for an inaudible fade");
		}
		require(world.state(music.name) == SoundState::stopped &&
			music_voice_calls(world, music.name, SoundCall::stop_voice) > 0,
			"Leaving results while the coda played did not cancel its owned voice");
		AudioResources missing_bank;
		missing_bank.add_sound_bank(world.presentation.menu().sound_bank, SoundBank::silent());
		{
			ResultsAudio silent(&missing_bank, &world.preferences, world.presentation);
			silent.update(mix.fade_in);
			silent.suspend();
			silent.resume();
			silent.resume();
			silent.fade_out();
			require(silent.finished(), "Silent-bank results exit did not finish immediately");
		}
		world.set_music_volume(original_music);
		world.set_sfx_volume(original_sfx);
		check_screen_music(world);
		check_music_exit_callbacks();
	}

	void check_screen_music(World& world)
	{
		const int original_music = world.preferences.music_volume;
		const int original_sfx = world.preferences.sfx_volume;
		const auto& music = world.presentation.main_menu().music;
		const auto& mix = world.presentation.music_mix();
		const auto& bank = world.presentation.menu().sound_bank;
		world.set_music_volume(40);
		world.set_sfx_volume(0);
		world.begin = recorded_sounds(world.device).size();
		{
			ScreenMusic audio(bank, &world.audio, &world.preferences, music, mix, true);
			require(world.plays(music.name) == 1 && world.state(music.name) == SoundState::playing &&
				close_to(world.last_volume(music.name), 0.0f) && !audio.finished(),
				"Screen owner did not start exactly one zero-gain menu loop");
			const auto voice = world.voices.at(music.name);
			for (size_t index = world.begin; index < recorded_sounds(world.device).size(); ++index)
			{
				const auto& call = recorded_sounds(world.device)[index];
				if (call.voice == voice && call.call == SoundCall::play_voice)
				{
					require(call.loop, "Persistent menu music was played as a one-shot");
				}
			}
			const auto before_invalid = recorded_sounds(world.device).size();
			for (const float invalid : { -1.0f, std::numeric_limits<float>::infinity(),
				std::numeric_limits<float>::quiet_NaN() })
			{
				audio.update(invalid);
			}
			require(recorded_sounds(world.device).size() == before_invalid,
				"Negative/nonfinite screen-music time emitted audio calls");
			audio.update(mix.fade_in * 0.5f);
			require(close_to(world.last_volume(music.name), music.volume * 0.4f * 0.5f),
				"Persistent menu music missed its fade-in midpoint");
			world.set_music_volume(20);
			audio.update(0.0f);
			require(close_to(world.last_volume(music.name), music.volume * 0.2f * 0.5f),
				"Zero-time settings refresh advanced the envelope or ignored applied gain");
			audio.resume();
			audio.resume();
			require(world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 0,
				"Repeated screen-owner reuse restarted or resumed an already playing menu loop");
			audio.suspend();
			const auto paused = recorded_sounds(world.device).size();
			audio.update(mix.fade_in * 20.0f);
			require(world.state(music.name) == SoundState::paused && recorded_sounds(world.device).size() == paused,
				"Focus suspension advanced the menu fade or adjusted its paused voice");
			world.set_music_volume(60);
			world.set_sfx_volume(100);
			audio.resume();
			audio.resume();
			require(world.state(music.name) == SoundState::playing && world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 1 &&
				close_to(world.last_volume(music.name), music.volume * 0.6f * 0.5f),
				"Menu focus return lost its current envelope/preferences or replayed the loop");
			audio.update(mix.fade_in * 0.5f);
			require(close_to(world.last_volume(music.name), music.volume * 0.6f),
				"Menu fade-in did not continue from its suspended midpoint");
			audio.fade_out();
			audio.update(mix.fade_out * 0.5f);
			require(!audio.finished() && close_to(world.last_volume(music.name), music.volume * 0.6f * 0.5f),
				"Screen exit missed its fade midpoint or finished too early");
			audio.fade_out();
			audio.update(mix.fade_out * 0.5f);
			require(audio.finished() && world.state(music.name) == SoundState::stopped &&
				close_to(world.last_volume(music.name), 0.0f) && world.plays(music.name) == 1,
				"Repeated exit requests reset the fade or failed to stop the menu loop at zero");
			audio.resume();
			audio.update(mix.fade_out * 20.0f);
			require(world.state(music.name) == SoundState::stopped && world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 1,
				"Focus/updates resurrected a completed screen-music exit");
		}

		world.set_music_volume(100);
		world.begin = recorded_sounds(world.device).size();
		{
			ScreenMusic interrupted(bank, &world.audio, &world.preferences, music, mix, true);
			interrupted.update(mix.fade_in * 0.25f);
			interrupted.fade_out();
			interrupted.update(mix.fade_out * 0.5f);
			require(close_to(world.last_volume(music.name), music.volume * 0.125f),
				"Exit during entry jumped to full gain instead of fading from the current value");
			interrupted.suspend();
			interrupted.update(mix.fade_out * 10.0f);
			interrupted.resume();
			require(close_to(world.last_volume(music.name), music.volume * 0.125f),
				"Focus interruption advanced a pending exit fade");
			interrupted.fade_out();
			interrupted.update(mix.fade_out * 0.5f);
			require(interrupted.finished() && world.state(music.name) == SoundState::stopped &&
				world.plays(music.name) == 1, "Interrupted menu exit failed to finish without replay");
		}
		world.begin = recorded_sounds(world.device).size();
		{
			ScreenMusic drained(bank, &world.audio, &world.preferences, music, mix, false);
			drained.update(mix.fade_in);
			// Publicly reproduce the state reported after an asynchronous buffer
			// drain. The recorder itself has no clock or natural-completion event.
			world.device.stop_voice(world.voices.at(music.name), true);
			drained.suspend();
			drained.resume();
			drained.fade_out();
			drained.update(mix.fade_out);
			require(drained.finished() && world.state(music.name) == SoundState::stopped &&
				world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 0,
				"A drained one-shot restarted when its screen exited or regained focus");
		}
		for (const bool authored_silent : { false, true })
		{
			PresentationSound quiet = music;
			quiet.volume = authored_silent ? 0.0f : music.volume;
			world.set_music_volume(authored_silent ? 100 : 0);
			world.begin = recorded_sounds(world.device).size();
			ScreenMusic muted(bank, &world.audio, &world.preferences, quiet, mix, true);
			muted.update(mix.fade_in);
			muted.fade_out();
			require(muted.finished() && world.state(music.name) == SoundState::stopped && world.plays(music.name) == 1,
				"Muted/authored-zero screen exit waited for an inaudible fade or replayed music");
		}
		world.set_music_volume(100);
		world.begin = recorded_sounds(world.device).size();
		{
			ScreenMusic zero(bank, &world.audio, &world.preferences, music, mix, true);
			zero.fade_out();
			require(zero.finished() && world.state(music.name) == SoundState::stopped,
				"Exit at the untouched zero-gain entry boundary delayed the screen");
		}
		AudioResources missing;
		missing.add_sound_bank(bank, SoundBank::silent());
		const auto absent = recorded_sounds(world.device).size();
		{
			ScreenMusic silent(bank, &missing, &world.preferences, music, mix, true);
			silent.update(mix.fade_in);
			silent.suspend();
			silent.resume();
			silent.fade_out();
			require(silent.finished(), "Missing-bank screen exit failed its immediate completion path");
		}
		require(recorded_sounds(world.device).size() == absent,
			"Silent-bank screen music reached an audible recorder device");
		world.set_music_volume(original_music);
		world.set_sfx_volume(original_sfx);
	}

	void check_music_exit_callbacks()
	{
		MenuContext standalone(nullptr, nullptr, nullptr);
		int immediate = 0;
		standalone.finish_with_music([&] { ++immediate; });
		standalone.complete_music_exit();
		require(immediate == 1 && !standalone.music_exit_pending(),
			"A standalone page without a music owner did not finish immediately once");

		MenuContext context(nullptr, nullptr, nullptr);
		int updates = 0;
		float elapsed = -1.0f;
		int suspended = 0;
		context.set_music_update([&](float dt) { ++updates; elapsed = dt; });
		context.set_music_suspend([&] { ++suspended; });
		int completed = 0;
		context.finish_with_music([&]
		{
			require(!context.music_exit_pending(), "Music completion invoked its callback before clearing it");
			++completed;
		});
		require(context.music_exit_pending() && updates == 1 && elapsed == 0.0f && completed == 0,
			"Owned page exit failed to request a zero-time fade start and retain its callback");
		context.finish_with_music([&] { completed += 100; });
		require(updates == 1, "A repeated page exit restarted its pending music transition");
		context.suspend_music();
		require(suspended == 1 && context.music_exit_pending(), "Music suspension dropped its pending exit");
		context.update_music(0.25f);
		require(updates == 2 && elapsed == 0.25f && completed == 0,
			"A top-page music update ran the pending finish before the owner completed it");
		context.complete_music_exit();
		context.complete_music_exit();
		require(completed == 1 && !context.music_exit_pending(), "Music completion duplicated or replaced the first exit");
		context.finish_with_music([&] { completed += 100; });
		context.cancel_music_exit();
		context.complete_music_exit();
		require(completed == 1 && !context.music_exit_pending(), "Cancelled debug navigation retained a stale exit callback");
		context.finish_with_music([&]
		{
			++completed;
			context.finish_with_music([&] { ++completed; });
		});
		context.complete_music_exit();
		require(completed == 2 && context.music_exit_pending(), "A callback could not queue a distinct later exit safely");
		context.complete_music_exit();
		require(completed == 3 && !context.music_exit_pending(), "Reentrant exit bookkeeping lost or duplicated a callback");
	}

	class EndingStageVariant
	{
	public:
		explicit EndingStageVariant(const std::filesystem::path& content)
		{
			std::ifstream stream(content / "levels/close_quarters.json");
			require(stream.good(), "Cannot read ending-sting acceptance stage");
			this->source_ = std::string(std::istreambuf_iterator<char>(stream), {});
			wchar_t name[MAX_PATH]{};
			require(GetTempFileNameW(std::filesystem::temp_directory_path().c_str(), L"cwe", 0, name) != 0,
				"Cannot reserve ending-sting acceptance stage file");
			this->path_ = name;
		}

		~EndingStageVariant()
		{
			std::error_code ignored;
			std::filesystem::remove(this->path_, ignored);
		}

		template<typename Edit>
		LevelLoadedInfo load(Edit edit) const
		{
			rapidjson::Document json;
			json.Parse(this->source_.c_str());
			require(!json.HasParseError(), "Cannot parse ending-sting acceptance stage");
			edit(json["sound"], json.GetAllocator());
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			json.Accept(writer);
			std::ofstream stream(this->path_, std::ios::binary | std::ios::trunc);
			stream.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
			stream.close();
			require(stream.good(), "Cannot write ending-sting acceptance stage variant");
			return LevelLoadedInfo(this->path_.string().c_str());
		}

	private:
		std::string source_;
		std::filesystem::path path_;
	};

	void check_ending_sting_content(World& world, const std::filesystem::path& content)
	{
		const EndingStageVariant variants(content);
		const LevelObjectBuilder builder(&world.render, &world.audio, &world.preferences, &world.tuning.paint());
		const auto rejected = [&](auto edit)
		{
			bool named = false;
			try
			{
				const LevelLoadedInfo stage = variants.load(edit);
				stage.validate();
				stage.validate_content(builder, world.render, world.audio);
			}
			catch (const std::runtime_error& error)
			{
				named = std::string(error.what()).find("sound") != std::string::npos;
			}
			require(named, "Invalid ending-sting stage did not fail with its sound field named");
		};
		for (const char* key : { "end_sting_name", "end_sting_volume" })
		{
			rejected([&](rapidjson::Value& sound, auto&) { sound.RemoveMember(key); });
		}
		for (const double invalid : { -0.01, 1.01, 1.0e100 })
		{
			rejected([&](rapidjson::Value& sound, auto&) { sound["end_sting_volume"].SetDouble(invalid); });
		}
		for (const char* name : { "", "missing.ending_sting" })
		{
			rejected([&](rapidjson::Value& sound, auto& allocator)
				{ sound["end_sting_name"].SetString(name, allocator); });
		}
		rejected([](rapidjson::Value& sound, auto&) { sound["end_sting_name"].SetInt(4); });
		rejected([](rapidjson::Value& sound, auto&) { sound["end_sting_volume"].SetBool(true); });
		for (const float volume : { 0.0f, 1.0f })
		{
			const LevelLoadedInfo stage = variants.load([&](rapidjson::Value& sound, auto&)
				{ sound["end_sting_volume"].SetFloat(volume); });
			stage.validate();
			stage.validate_content(builder, world.render, world.audio);
			require(stage.has_end_sting() && stage.end_sting_volume() == volume,
				"Valid ending-sting gain boundary was not retained");
		}
		const LevelLoadedInfo absent = variants.load([](rapidjson::Value& sound, auto&)
		{
			sound.RemoveMember("end_sting_name");
			sound.RemoveMember("end_sting_volume");
		});
		absent.validate();
		absent.validate_content(builder, world.render, world.audio);
		require(!absent.has_end_sting() && absent.end_sting_name().empty() && absent.end_sting_volume() == 0.0f,
			"A stage without optional ending-sting fields did not remain silent");
	}

	void check_ending_stings(World& world)
	{
		const int original_music = world.preferences.music_volume;
		const int original_sfx = world.preferences.sfx_volume;
		struct ArenaSounds
		{
			const char* stage;
			const char* music;
			const char* ending_sting;
		};
		const ArenaSounds arenas[]{
			{ "close_quarters", "music.close_quarters", "music.end_sting.close_quarters" },
			{ "king_of_the_hill", "music.king_of_the_hill", "music.end_sting.king_of_the_hill" },
			{ "turbulence", "music.turbulence", "music.end_sting.turbulence" },
			{ "crossfeed", "music.close_quarters", "music.end_sting.close_quarters" }
		};
		for (const ArenaSounds& arena : arenas)
		{
			const LevelLoadedInfo* stage = world.levels.get(arena.stage);
			const std::string expected = arena.ending_sting;
			require(stage->has_music() && stage->music_name() == arena.music,
				"Arena music does not match its accepted cue binding");
			require(stage->has_end_sting() && stage->end_sting_name() == expected,
				"Arena ending sting does not match its accepted cue binding");
			for (const int players : { 1, 2, 4 })
			{
				world.set_music_volume(players == 1 ? 0 : 40);
				world.set_sfx_volume(0);
				world.build(OpeningMode::off, players, 0.25f, arena.stage);
				world.level->update(world.idle, 0.0f);
				world.level->update(world.idle, -1.0f);
				require(world.plays(expected) == 0, "Construction or frozen countdown played an ending sting");
				world.until([&] { return world.level->state() == LevelState::active; });
				require(world.plays(expected) == 0, "Arena ending sting sounded before time up");
				world.until([&] { return world.level->state() == LevelState::zoom_out; });
				require(world.plays(expected) == 1 && world.state(expected) == SoundState::playing,
					"Time up missed the ending sting or duplicated it per player");
				world.volume_is(expected, stage->end_sting_volume() * world.preferences.music_gain());
				for (const ArenaSounds& other : arenas)
				{
					if (expected == other.ending_sting) { continue; }
					require(world.plays(other.ending_sting) == 0,
						"Time up played a different arena's ending sting");
				}
				world.level->update(world.idle, 0.0f);
				world.level->update(world.idle, -1.0f);
				world.level->suspend();
				require(world.state(expected) == SoundState::paused, "Suspension did not pause the ending sting");
				world.set_music_volume(75);
				world.set_sfx_volume(100);
				world.level->resume();
				require(world.state(expected) == SoundState::playing && world.plays(expected) == 1 &&
					close_to(world.last_volume(expected), stage->end_sting_volume() * 0.75f),
					"Ending-sting resume replayed its phrase or ignored edited music gain");
				world.until([&] { return world.level->state() == LevelState::overview; });
				require(world.state(expected) == SoundState::stopped && world.plays(expected) == 1,
					"An ending sting survived the glass-out boundary or repeated");
				world.level->resume();
				world.until([&] { return world.level->state() == LevelState::finished; });
				require(world.state(expected) == SoundState::stopped && world.plays(expected) == 1,
					"Post-match updates or resume resurrected an ending sting");
				world.level.reset();
				require(world.state(expected) == SoundState::stopped, "Level teardown left an ending sting alive");
			}
			world.build(OpeningMode::off, 2, 0.25f, arena.stage);
			world.until([&] { return world.level->state() == LevelState::zoom_out; });
			world.level.reset();
			require(world.state(expected) == SoundState::stopped,
				"Leaving during glass-out did not cancel its owned ending sting");
		}
		const LevelLoadedInfo* course = world.levels.get("movement_course");
		require(!course->has_music() && !course->has_end_sting(),
			"The deliberately silent Movement Course acquired music or an ending sting");
		world.build(OpeningMode::off, 4, 0.25f, "movement_course");
		world.until([&] { return world.level->state() == LevelState::finished; });
		for (const ArenaSounds& arena : arenas)
		{
			require(world.plays(arena.ending_sting) == 0,
				"The silent Movement Course emitted an arena ending sting");
		}
		world.level.reset();
		world.set_music_volume(original_music);
		world.set_sfx_volume(original_sfx);
	}
}
