#pragma once

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

	void check_results_audio(World& world)
	{
		world.level.reset();
		const int original_music = world.preferences.music_volume;
		const int original_sfx = world.preferences.sfx_volume;
		const auto& music = world.presentation.results_menu().music;
		world.preferences.music_volume = 35;
		world.preferences.sfx_volume = 0;
		world.begin = recorded_sounds(world.device).size();
		{
			ResultsAudio audio(&world.audio, &world.preferences, world.presentation);
			require(world.plays(music.name) == 1 && world.state(music.name) == SoundState::playing,
				"Results coda did not begin once on its owned voice");
			world.volume_is(music.name, music.volume * 0.35f);
			audio.resume();
			audio.resume();
			require(world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 0,
				"An already playing results coda restarted or received a spurious resume");
			audio.suspend();
			audio.suspend();
			require(world.state(music.name) == SoundState::paused,
				"Focus loss did not preserve the results coda on its paused voice");
			world.preferences.music_volume = 60;
			world.preferences.sfx_volume = 100;
			audio.resume();
			audio.resume();
			require(world.state(music.name) == SoundState::playing && world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == 1 &&
				close_to(world.last_volume(music.name), music.volume * 0.6f),
				"Results focus return restarted the coda or ignored current music gain");
			audio.suspend();
			world.preferences.music_volume = 0;
			audio.resume();
			require(world.plays(music.name) == 1 && close_to(world.last_volume(music.name), 0.0f),
				"Results focus return ignored music mute or replayed the coda");

			// The null backend has no clock or drained-buffer event. Put the voice
			// in that same stopped state through its public API to check the guard;
			// this establishes no claim about natural tail duration or playback.
			world.device.stop_voice(world.voices.at(music.name), true);
			const int resumed = music_voice_calls(world, music.name, SoundCall::resume_voice);
			audio.suspend();
			world.preferences.music_volume = 80;
			audio.resume();
			require(world.state(music.name) == SoundState::stopped && world.plays(music.name) == 1 &&
				music_voice_calls(world, music.name, SoundCall::resume_voice) == resumed,
				"Focus return attempted to resurrect a completed results coda");
		}
		require(world.state(music.name) == SoundState::stopped,
			"Results-audio destruction left its voice alive");
		world.begin = recorded_sounds(world.device).size();
		world.preferences.music_volume = 0;
		{
			ResultsAudio fresh(&world.audio, &world.preferences, world.presentation);
			require(world.plays(music.name) == 1 && close_to(world.last_volume(music.name), 0.0f),
				"A fresh results page failed to start its own muted coda");
		}
		require(world.state(music.name) == SoundState::stopped &&
			music_voice_calls(world, music.name, SoundCall::stop_voice) > 0,
			"Leaving results while the coda played did not cancel its owned voice");
		AudioResources missing_bank;
		missing_bank.add_sound_bank(world.presentation.menu().sound_bank, SoundBank::silent());
		{
			ResultsAudio silent(&missing_bank, &world.preferences, world.presentation);
			silent.suspend();
			silent.resume();
			silent.resume();
		}
		world.preferences.music_volume = original_music;
		world.preferences.sfx_volume = original_sfx;
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
		for (const char* stage_name : { "close_quarters", "king_of_the_hill", "turbulence" })
		{
			const LevelLoadedInfo* stage = world.levels.get(stage_name);
			const std::string expected = std::string("music.end_sting.") + stage_name;
			require(stage->has_end_sting() && stage->end_sting_name() == expected,
				"Arena ending sting does not match its accepted stage cadence");
			for (const int players : { 1, 2, 4 })
			{
				world.preferences.music_volume = players == 1 ? 0 : 40;
				world.preferences.sfx_volume = 0;
				world.build(OpeningMode::off, players, 0.25f, stage_name);
				world.level->update(world.idle, 0.0f);
				world.level->update(world.idle, -1.0f);
				require(world.plays(expected) == 0, "Construction or frozen countdown played an ending sting");
				world.until([&] { return world.level->state() == LevelState::active; });
				require(world.plays(expected) == 0, "Arena ending sting sounded before time up");
				world.until([&] { return world.level->state() == LevelState::zoom_out; });
				require(world.plays(expected) == 1 && world.state(expected) == SoundState::playing,
					"Time up missed the ending sting or duplicated it per player");
				world.volume_is(expected, stage->end_sting_volume() * world.preferences.music_gain());
				for (const char* other : { "close_quarters", "king_of_the_hill", "turbulence" })
				{
					if (stage_name == std::string(other)) { continue; }
					require(world.plays(std::string("music.end_sting.") + other) == 0,
						"Time up played a different arena's ending sting");
				}
				world.level->update(world.idle, 0.0f);
				world.level->update(world.idle, -1.0f);
				world.level->suspend();
				require(world.state(expected) == SoundState::paused, "Suspension did not pause the ending sting");
				world.preferences.music_volume = 75;
				world.preferences.sfx_volume = 100;
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
			world.build(OpeningMode::off, 2, 0.25f, stage_name);
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
		for (const char* stage : { "close_quarters", "king_of_the_hill", "turbulence" })
		{
			require(world.plays(std::string("music.end_sting.") + stage) == 0,
				"The silent Movement Course emitted an arena ending sting");
		}
		world.level.reset();
		world.preferences.music_volume = original_music;
		world.preferences.sfx_volume = original_sfx;
	}
}
