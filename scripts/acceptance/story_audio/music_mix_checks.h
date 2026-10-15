#pragma once

// Included after story_audio.cpp's recording world and content helpers.
namespace
{
	void check_music_mix_content(World& world, const std::filesystem::path& content)
	{
		const MenuPresentationVariant variants(content);
		const auto rejected = [&](const char* key, auto edit)
		{
			bool named = false;
			try
			{
				std::ignore = variants.load([&](rapidjson::Document& json)
					{ edit(json["music_mix"], json.GetAllocator()); });
			}
			catch (const std::runtime_error& error)
			{
				const std::string message = error.what();
				named = message.find("music_mix") != std::string::npos &&
					message.find(key) != std::string::npos;
			}
			require(named, "Invalid music mix did not fail with its authored field named");
		};
		for (const char* key : { "fade_in", "fade_out", "duck_time", "restore_time" })
		{
			for (const double invalid : { 0.0, -0.01, 5.01, 1.0e100 })
			{
				rejected(key, [&](rapidjson::Value& mix, auto&) { mix[key].SetDouble(invalid); });
			}
			rejected(key, [&](rapidjson::Value& mix, auto&) { mix.RemoveMember(key); });
			rejected(key, [&](rapidjson::Value& mix, auto&) { mix[key].SetBool(true); });
		}
		for (const double invalid : { -0.01, 1.01, 1.0e100 })
		{
			rejected("pause_gain", [&](rapidjson::Value& mix, auto&) { mix["pause_gain"].SetDouble(invalid); });
		}
		rejected("pause_gain", [](rapidjson::Value& mix, auto&) { mix.RemoveMember("pause_gain"); });
		rejected("pause_gain", [](rapidjson::Value& mix, auto& allocator)
			{ mix["pause_gain"].SetString("quiet", allocator); });
		for (const float gain : { 0.0f, 1.0f })
		{
			const Presentation boundary = variants.load([&](rapidjson::Document& json)
			{
				auto& mix = json["music_mix"];
				for (const char* key : { "fade_in", "fade_out", "duck_time", "restore_time" })
				{
					mix[key].SetFloat(5.0f);
				}
				mix["pause_gain"].SetFloat(gain);
			});
			const auto& mix = boundary.music_mix();
			require(mix.fade_in == 5.0f && mix.fade_out == 5.0f && mix.duck_time == 5.0f &&
				mix.restore_time == 5.0f && mix.pause_gain == gain,
				"Valid music-mix duration/gain boundaries were not retained");
		}
		const auto& mix = world.presentation.music_mix();
		require(mix.fade_in > 0.0f && mix.fade_out > 0.0f && mix.duck_time > 0.0f &&
			mix.restore_time > 0.0f && mix.pause_gain >= 0.0f && mix.pause_gain <= 1.0f,
			"Shipped music mix has no usable envelope");
		bool missing = false;
		try
		{
			std::ignore = variants.load([](rapidjson::Document& json) { json.RemoveMember("music_mix"); });
		}
		catch (const std::runtime_error& error)
		{
			missing = std::string(error.what()).find("music_mix") != std::string::npos;
		}
		require(missing, "A missing music-mix block did not fail at startup with its name");
	}

	void start_music_mix(World& world, int players = 2)
	{
		world.build(OpeningMode::off, players, 60.0f);
		world.level->update(world.idle, 0.001f);
		const std::string music = world.levels.get("close_quarters")->music_name();
		require(world.plays(music) == 1 && world.state(music) == SoundState::playing &&
			close_to(world.last_volume(music), 0.0f),
			"Arena countdown did not start exactly one silent music loop");
		bool looping = false;
		const auto voice = world.voices.at(music);
		for (size_t index = world.begin; index < recorded_sounds(world.device).size(); ++index)
		{
			const auto& call = recorded_sounds(world.device)[index];
			if (call.voice == voice && call.call == SoundCall::play_voice) { looping = call.loop; }
		}
		require(looping, "Arena fade-in started its track as a one-shot");
	}

	void check_music_mix_clock_and_gain(World& world)
	{
		const int original_music = world.preferences.music_volume;
		const int original_sfx = world.preferences.sfx_volume;
		world.preferences.music_volume = 40;
		world.preferences.sfx_volume = 0;
		const auto& mix = world.presentation.music_mix();
		const LevelLoadedInfo* stage = world.levels.get("close_quarters");
		const std::string music = stage->music_name();
		const float full = stage->music_volume() * 0.4f;
		start_music_mix(world, 4);
		const auto untouched = recorded_sounds(world.device).size();
		for (const float invalid : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
			std::numeric_limits<float>::quiet_NaN() })
		{
			world.level->update_music(invalid);
		}
		require(recorded_sounds(world.device).size() == untouched,
			"Nonpositive/nonfinite raw audio time emitted music calls");
		world.level->update_music(mix.fade_in * 0.25f);
		require(close_to(world.last_volume(music), full * 0.25f),
			"Arena fade-in did not reach its linear quarter gain");
		for (int tick = 0; tick < 20; ++tick) { world.level->update(world.idle, weapon_step); }
		world.level->refresh_layout();
		world.level->refresh_layout();
		require(close_to(world.last_volume(music), full * 0.25f) && world.plays(music) == 1,
			"Simulation updates/layout redraw advanced the raw music envelope or restarted it");
		world.level->update_music(mix.fade_in * 0.75f);
		require(close_to(world.last_volume(music), full), "Arena fade-in missed its full-gain endpoint");
		world.level->update_music(mix.fade_in * 20.0f);
		require(close_to(world.last_volume(music), full) && world.plays(music) == 1,
			"Long raw audio time overshot or replayed the full music loop");
		world.preferences.music_volume = 0;
		world.preferences.sfx_volume = 100;
		world.level->update_music(0.001f);
		require(close_to(world.last_volume(music), 0.0f), "Settled music ignored a newly applied mute");
		world.preferences.music_volume = 65;
		world.level->update_music(0.001f);
		require(close_to(world.last_volume(music), stage->music_volume() * 0.65f) && world.plays(music) == 1,
			"Settled music ignored unmute/current music gain or restarted its loop");
		world.device.stop_voice(world.voices.at(music), true);
		const auto stopped = recorded_sounds(world.device).size();
		world.level->resume_music();
		world.level->update_music(10.0f);
		require(world.state(music) == SoundState::stopped && recorded_sounds(world.device).size() == stopped,
			"Stopped music was restarted or adjusted by resume/raw time");
		world.level.reset();
		world.preferences.music_volume = original_music;
		world.preferences.sfx_volume = original_sfx;
	}

	void check_music_mix_duck_and_suspend(World& world)
	{
		const int original_music = world.preferences.music_volume;
		const auto& mix = world.presentation.music_mix();
		const LevelLoadedInfo* stage = world.levels.get("close_quarters");
		const std::string music = stage->music_name();
		world.preferences.music_volume = 50;
		const float full = stage->music_volume() * 0.5f;
		start_music_mix(world);
		world.level->update_music(mix.fade_in);
		world.level->set_music_ducked(true);
		world.level->update_music(mix.duck_time * 0.5f);
		const float middle = (1.0f + mix.pause_gain) * 0.5f;
		require(close_to(world.last_volume(music), full * middle), "Pause duck missed its linear midpoint");
		world.level->set_music_ducked(true);
		world.level->update_music(mix.duck_time * 0.5f);
		require(close_to(world.last_volume(music), full * mix.pause_gain),
			"Repeated pause target reset the ramp instead of reaching the duck gain");
		world.level->set_music_ducked(false);
		world.level->update_music(mix.restore_time * 0.5f);
		require(close_to(world.last_volume(music), full * middle), "Music restore missed its linear midpoint");
		world.level->set_music_ducked(true);
		world.level->update_music(mix.duck_time * 0.5f);
		require(close_to(world.last_volume(music), full * (middle + mix.pause_gain) * 0.5f),
			"Interrupted restoration jumped gain instead of ducking from its current value");
		world.level->set_music_ducked(true);
		world.level->update_music(mix.duck_time * 0.5f);
		require(close_to(world.last_volume(music), full * mix.pause_gain),
			"Repeated interrupted duck target reset its remaining ramp");
		world.level->set_music_ducked(false);
		world.level->update_music(mix.restore_time);
		world.level->set_music_ducked(true);
		world.level->update_music(mix.duck_time * 0.25f);
		const float paused_gain = 1.0f + (mix.pause_gain - 1.0f) * 0.25f;
		world.level->suspend();
		const auto suspended = recorded_sounds(world.device).size();
		world.level->update_music(mix.duck_time * 20.0f);
		require(world.state(music) == SoundState::paused && recorded_sounds(world.device).size() == suspended,
			"Hard suspension advanced the music envelope or emitted gain calls");
		world.preferences.music_volume = 30;
		world.level->resume_music();
		require(world.state(music) == SoundState::playing && world.plays(music) == 1 &&
			close_to(world.last_volume(music), stage->music_volume() * 0.3f * paused_gain),
			"Music-only resume lost its paused envelope, edited gain or existing voice");
		world.level->update_music(mix.duck_time * 0.75f);
		require(close_to(world.last_volume(music), stage->music_volume() * 0.3f * mix.pause_gain),
			"Music-only resume failed to finish its existing duck target");
		world.level->resume();
		world.level->update_music(mix.restore_time * 0.5f);
		require(close_to(world.last_volume(music), stage->music_volume() * 0.3f * middle),
			"Live resume failed to restore ducked music gradually");
		world.level->resume();
		world.level->update_music(mix.restore_time * 0.5f);
		require(close_to(world.last_volume(music), stage->music_volume() * 0.3f) && world.plays(music) == 1,
			"Repeated live resume reset the restore ramp or replayed the music");
		world.level->set_music_ducked(true);
		world.level->update_music(mix.duck_time * 20.0f);
		require(close_to(world.last_volume(music), stage->music_volume() * 0.3f * mix.pause_gain),
			"Long raw time overshot the pause duck endpoint");
		world.level->set_music_ducked(false);
		world.level->update_music(mix.restore_time * 20.0f);
		require(close_to(world.last_volume(music), stage->music_volume() * 0.3f) && world.plays(music) == 1,
			"Long raw time overshot restoration or replayed the music");
		world.level.reset();
		require(world.state(music) == SoundState::stopped, "Teardown left the enveloped music loop alive");
		world.preferences.music_volume = original_music;
	}

	MenuLevelSettings music_mix_settings()
	{
		MenuLevelSettings settings;
		settings.set_stage("close_quarters");
		settings.set_duration(0.25f);
		settings.set_player_count(2);
		settings.begin_input_claims(input_slots::keyboard_and_mouse);
		settings.set_screen_layout(ScreenLayout::two_player);
		for (int view = 0; view < 2; ++view)
		{
			settings.set_player_team(view, view == 0 ? PlayerTeam::a : PlayerTeam::b);
			settings.set_player_weapon(view, WeaponType::sprayer);
		}
		return settings;
	}

	void check_music_mix_ending(World& world, const std::filesystem::path& content)
	{
		const int original_music = world.preferences.music_volume;
		world.preferences.music_volume = 40;
		const auto& mix = world.presentation.music_mix();
		const LevelLoadedInfo* stage = world.levels.get("close_quarters");
		const std::string music = stage->music_name();
		const std::string sting = stage->end_sting_name();
		world.build(OpeningMode::off, 2, 0.25f);
		world.level->update(world.idle, 0.001f);
		world.level->update_music(mix.fade_in);
		world.until([&] { return world.level->state() == LevelState::zoom_out; });
		require(world.state(music) == SoundState::playing && world.plays(music) == 1 && world.plays(sting) == 1,
			"Time up cut its loop before the fade or missed the ending sting");
		world.level->update_music(mix.fade_out * 0.5f);
		require(world.state(music) == SoundState::playing &&
			close_to(world.last_volume(music), stage->music_volume() * 0.4f * 0.5f),
			"Time-up fade missed its linear half-gain boundary");
		world.level->suspend();
		const auto suspended = recorded_sounds(world.device).size();
		world.level->update_music(mix.fade_out * 10.0f);
		require(recorded_sounds(world.device).size() == suspended,
			"Suspended time-up fade advanced while the window was inactive");
		world.level->resume_music();
		world.level->update_music(mix.fade_out * 0.5f);
		require(world.state(music) == SoundState::stopped && close_to(world.last_volume(music), 0.0f) &&
			world.plays(music) == 1, "Time-up fade did not stop exactly at zero without replaying its loop");
		const auto finished = recorded_sounds(world.device).size();
		world.level->resume_music();
		world.level->update_music(mix.fade_out * 20.0f);
		require(recorded_sounds(world.device).size() == finished && world.state(music) == SoundState::stopped,
			"Completed time-up fade restarted or continued adjusting the stopped loop");
		world.level.reset();
		world.build(OpeningMode::off, 2, 0.25f);
		world.level->update(world.idle, 0.001f);
		world.level->update_music(mix.fade_in);
		world.until([&] { return world.level->state() == LevelState::zoom_out; });
		world.level->update_music(mix.fade_out * 20.0f);
		require(world.state(music) == SoundState::stopped && close_to(world.last_volume(music), 0.0f) &&
			world.plays(music) == 1, "A long audio tick overshot the ending fade or left its loop alive");
		world.level.reset();

		const ContentVariant variants(content / "tuning.json");
		const GameTuning shortened = variants.tuning([&](rapidjson::Document& json)
			{ json["match"]["zoom_out"].SetFloat(mix.fade_out * 0.25f); });
		LevelBuilder builder(&world.viewports, &world.render, &world.audio, &world.preferences, &world.levels,
			&shortened, &world.presentation, &world.resolution, nullptr, &world.partitioner, &world.mouse);
		world.begin = recorded_sounds(world.device).size();
		auto short_level = builder.build_level(music_mix_settings(), TeamColourPair::blue_orange, OpeningMode::full);
		auto skip = world.idle;
		skip.at(static_cast<size_t>(short_level->player(0).player_num())).skip_intro = true;
		short_level->update(skip, 0.001f);
		short_level->update(world.idle, 0.001f);
		short_level->update_music(mix.fade_in);
		for (int tick = 0; tick < 1000 && short_level->state() != LevelState::zoom_out; ++tick)
		{
			short_level->update(world.idle, weapon_step);
		}
		require(short_level->state() == LevelState::zoom_out && world.state(music) == SoundState::playing,
			"Short-glass fixture did not retain its outgoing loop at time up");
		short_level->update_music(mix.fade_out * 0.125f);
		short_level->update(world.idle, shortened.match().zoom_out);
		require(short_level->revealed() && short_level->state() == LevelState::overview &&
			world.state(music) == SoundState::stopped &&
			world.state(sting) == SoundState::stopped,
			"Shortened glass-out let arena music or its sting follow the returning person");
		short_level->resume();
		short_level->update_music(mix.fade_out * 10.0f);
		require(world.state(music) == SoundState::stopped && world.state(sting) == SoundState::stopped &&
			world.plays(music) == 1 && world.plays(sting) == 1,
			"Overview resume/raw time resurrected forced-silent transition music");
		world.preferences.music_volume = original_music;
	}

	void check_music_mix_silence(World& world)
	{
		world.build(OpeningMode::off, 4, 0.25f, "movement_course");
		world.level->update(world.idle, 0.001f);
		const auto quiet = recorded_sounds(world.device).size();
		world.level->set_music_ducked(true);
		world.level->update_music(20.0f);
		world.level->set_music_ducked(false);
		world.level->resume_music();
		world.level->update_music(20.0f);
		require(recorded_sounds(world.device).size() == quiet,
			"Silent Movement Course acquired music calls through duck/restore/raw time");
		world.level.reset();
		AudioResources missing;
		missing.add_sound_bank("paint_exe", SoundBank::silent());
		LevelBuilder builder(&world.viewports, &world.render, &missing, &world.preferences, &world.levels,
			&world.tuning, &world.presentation, &world.resolution, nullptr, &world.partitioner, &world.mouse);
		const auto silent_calls = recorded_sounds(world.device).size();
		{
			auto silent = builder.build_level(music_mix_settings(), TeamColourPair::blue_orange, OpeningMode::off);
			silent->update(world.idle, 0.001f);
			silent->update_music(world.presentation.music_mix().fade_in);
			silent->set_music_ducked(true);
			silent->update_music(world.presentation.music_mix().duck_time);
			silent->suspend();
			silent->update_music(20.0f);
			silent->resume_music();
			silent->resume();
			for (int tick = 0; tick < 1000 && silent->state() != LevelState::zoom_out; ++tick)
			{
				silent->update(world.idle, weapon_step);
			}
			require(silent->state() == LevelState::zoom_out, "Silent-bank music fixture did not reach time up");
			silent->update_music(world.presentation.music_mix().fade_out);
			silent->update(world.idle, world.tuning.match().zoom_out);
		}
		require(recorded_sounds(world.device).size() == silent_calls,
			"Missing-bank music fade/duck lifecycle reached an audible device");
	}
}
