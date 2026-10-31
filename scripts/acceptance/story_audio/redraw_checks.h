#pragma once

// Included after story_audio.cpp's shared recording world and content helpers.
namespace
{
    std::unique_ptr<Player> redraw_player(World& world, const GameTuning& tuning,
        const RespawnRedrawPresentation& presentation, int index = 0, int device = 0)
    {
        using namespace mattmath;
        return std::make_unique<Player>(RectangleF(Vector2F::ZERO, tuning.player().size),
            &tuning, &world.presentation.movement_feedback(), &world.presentation.combat_feedback(), &presentation,
            &world.render, &world.audio, &world.preferences, device, PlayerTeam::a,
            WeaponType::sprayer, world.weapon(tuning, WeaponType::sprayer, index),
            WeaponType::none, Colour::blue, Viewport{}, Vector2F::ZERO,
            Vector2F::ZERO, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f, index);
    }

    size_t redraw_calls(const World& world, const std::string& name)
    {
        const auto voice = world.voices.at(name);
        const auto& calls = recorded_sounds(world.device);
        return static_cast<size_t>(std::count_if(calls.begin() + world.begin, calls.end(),
            [&](const auto& call) { return call.voice == voice; }));
    }

    void check_redraw_content(World& world, const ContentVariant& content)
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
                invalid = message.find("redraw_sounds") != std::string::npos &&
                    message.find(key) != std::string::npos;
            }
            require(invalid, "Invalid redraw cue was accepted or lost its authored key");
        };
        rejected([](rapidjson::Document& json) { json["player"]["redraw_sounds"].Clear(); }, "redraw_sounds");
        rejected([](rapidjson::Document& json)
        {
            json["player"]["redraw_sounds"][1]["name"].SetString("player.redraw.0", json.GetAllocator());
        }, "redraw_sounds");
        for (rapidjson::SizeType index = 0; index < 4; ++index)
        {
            rejected([&](rapidjson::Document& json)
            {
                json["player"]["redraw_sounds"][index]["name"].SetString("not.a.redraw.effect", json.GetAllocator());
            }, "[" + std::to_string(index) + "].name", true);
            for (float volume : { -0.1f, 1.1f })
            {
                rejected([&](rapidjson::Document& json)
                {
                    json["player"]["redraw_sounds"][index]["volume"].SetFloat(volume);
                }, "volume");
            }
        }
        AudioResources silent;
        silent.add_sound_bank("paint_exe", SoundBank::silent());
        world.tuning.validate_resources(world.render, silent);
    }

    void check_redraw_boundaries(World& world, const ContentVariant& content)
    {
        world.level.reset();
        world.set_sfx_volume(40);
        GameTuning tuning = world.tuning;
        RespawnRedrawPresentation look = world.presentation.respawn_redraw();
        look.draw_from = 0.25f;
        look.fill_at = 0.75f;
        const float delay = tuning.player().respawn_delay;
        const auto& name = tuning.player().redraw_sounds[0].name;
        auto player = redraw_player(world, tuning, look);
        world.begin = recorded_sounds(world.device).size();
        player->kill();
        player->update(weapon_step);
        player->update(delay * look.draw_from - 0.001f);
        require(world.plays(name) == 0, "Redraw pen started before the visible outline");
        player->update(0.002f);
        require(world.plays(name) == 1 && world.state(name) == SoundState::playing,
            "Visible redraw failed to start its owned loop once");
        const auto voice = world.voices.at(name);
        const auto& calls = recorded_sounds(world.device);
        require(std::any_of(calls.begin() + world.begin, calls.end(), [&](const auto& call)
            { return call.call == SoundCall::play_voice && call.voice == voice && call.loop; }),
            "Redraw pen did not request looping playback");
        require(close_to(world.last_volume(name), tuning.player().redraw_sounds[0].volume * 0.4f),
            "Redraw pen ignored authored or saved gain");
        player->update_weapon_and_get_projectiles(weapon_step, {});
        require(world.state(name) == SoundState::playing && world.plays(name) == 1,
            "Dead-player weapon suppression stopped or restarted its redraw pen");
        const size_t before = redraw_calls(world, name);
        const float remaining = player->respawn_timer();
        player->update(0.0f);
        player->update(-weapon_step);
        require(redraw_calls(world, name) == before && player->respawn_timer() == remaining,
            "Nonpositive player updates touched redraw audio or rewound its visible outline");
        MatchVoices voices;
        player->add_voices(voices);
        voices.pause();
        require(world.state(name) == SoundState::paused, "Player pause left the redraw pen running");
        world.set_sfx_volume(25);
        voices.resume();
        require(world.state(name) == SoundState::playing && world.plays(name) == 1 &&
            close_to(world.last_volume(name), tuning.player().redraw_sounds[0].volume * 0.25f),
            "Redraw resume restarted the pen or kept stale SFX gain");
        world.set_sfx_volume(0);
        player->update(weapon_step);
        require(close_to(world.last_volume(name), 0.0f), "Running redraw pen ignored mute");
        world.set_sfx_volume(100);
        tuning = content.tuning([](rapidjson::Document& json)
        {
            json["player"]["redraw_sounds"][0]["volume"].SetFloat(0.08f);
        });
        player->update(weapon_step);
        require(close_to(world.last_volume(name), 0.08f), "Running redraw pen ignored live authored gain");
        // The presentation is borrowed too: moving the fill boundary takes
        // effect on the next progressing tick, without rebuilding the player.
        look.fill_at = player->respawn_progress();
        player->update(weapon_step);
        require(world.state(name) == SoundState::stopped, "Redraw pen continued after the figure filled");
        player->force_respawn();
        player->kill();
        player->update(weapon_step);
        player->update(delay + 1.0f);
        require(world.plays(name) == 1 && player->state() == PlayerState::alive,
            "A long respawn tick caught up a redraw that was never visible");
    }

    void check_redraw_player_lifecycle(World& world)
    {
        world.level.reset();
        world.set_sfx_volume(40);
        RespawnRedrawPresentation look = world.presentation.respawn_redraw();
        look.draw_from = 0.0f;
        look.fill_at = 1.0f;
        const auto& name = world.tuning.player().redraw_sounds[0].name;
        const auto start = [&](Player& player)
        {
            player.kill();
            player.update(weapon_step);
            require(world.state(name) == SoundState::playing, "Redraw lifecycle fixture failed to start");
        };
        auto player = redraw_player(world, world.tuning, look);
        start(*player);
        player->force_respawn();
        require(world.state(name) == SoundState::stopped, "Forced respawn left the pen running");
        start(*player);
        player->update(world.tuning.player().respawn_delay);
        require(player->state() == PlayerState::alive && world.state(name) == SoundState::stopped,
            "Natural respawn left the pen running");
        start(*player);
        MatchVoices voices;
        player->add_voices(voices);
        voices.pause();
        player->stop_sounds();
        voices.resume();
        require(world.state(name) == SoundState::stopped, "Stopped paused pen restarted on resume");
        player->update(weapon_step);
        require(world.state(name) == SoundState::playing, "Progressing redraw did not rearm after an explicit stop");
        FigurePose whole;
        whole.whole = true;
        player->set_pose(whole);
        require(world.state(name) == SoundState::stopped, "Whole figure retained the hidden redraw pen");
        player->set_pose({});
        player->update(weapon_step);
        player.reset();
        require(world.state(name) == SoundState::stopped, "Player teardown left the redraw pen running");
    }

    void check_redraw_levels(World& world)
    {
        const auto build = [&](int count, float duration)
        {
            world.level.reset();
            world.begin = recorded_sounds(world.device).size();
            MatchSetup settings;
            settings.set_stage("close_quarters");
            settings.set_player_count(count);
            for (int index = 0; index < count; ++index)
            {
                settings.set_player_team(index, index < (count + 1) / 2 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(index, WeaponType::sprayer);
            }
            // All seats deliberately remain device-less. Their audio belongs
            // to roster position, and must never alias through player_num=-1.
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
                count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
            settings.set_duration(duration);
            world.viewports.set_layout(settings.screen_layout());
            world.level = world.builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off, MatchRandom());
            world.until([&] { return world.level->state() == LevelState::active; });
            for (int index = 0; index < count; ++index)
            {
                require(world.level->player(index).player_num() == -1 && world.level->kill_player(index),
                    "Redraw roster fixture acquired an input device or lost a seat");
            }
            world.level->update(world.idle, weapon_step);
            const auto& look = world.presentation.respawn_redraw();
            world.level->update(world.idle, world.tuning.player().respawn_delay *
                (look.draw_from + (look.fill_at - look.draw_from) * 0.25f));
        };
        world.set_sfx_volume(40);
        for (int count : { 1, 2, 4 })
        {
            build(count, 20.0f);
            for (int index = 0; index < count; ++index)
            {
                const auto& sound = world.tuning.player().redraw_sounds[static_cast<size_t>(index)];
                require(world.plays(sound.name) == 1 && world.state(sound.name) == SoundState::playing,
                    "Device-less roster seats shared or lost a redraw voice");
            }
            world.level->suspend();
            for (int index = 0; index < count; ++index)
            {
                require(world.state(world.tuning.player().redraw_sounds[static_cast<size_t>(index)].name) ==
                    SoundState::paused, "Level suspension did not pause each redraw voice");
            }
            world.set_sfx_volume(25);
            world.level->resume();
            const size_t before = recorded_sounds(world.device).size();
            world.level->update(world.idle, 0.0f);
            world.level->update(world.idle, -weapon_step);
            require(recorded_sounds(world.device).size() == before, "Nonpositive Level update touched audio");
            require(world.level->respawn_player(0), "Could not force the first Level respawn");
            for (int index = 0; index < count; ++index)
            {
                const auto& sound = world.tuning.player().redraw_sounds[static_cast<size_t>(index)];
                require(world.state(sound.name) == (index == 0 ? SoundState::stopped : SoundState::playing) &&
                    world.plays(sound.name) == 1 && close_to(world.last_volume(sound.name), sound.volume * 0.25f),
                    "Resumed redraw gains or independent per-player stops failed");
            }
            world.level.reset();
            for (const auto& sound : world.tuning.player().redraw_sounds)
            {
                require(world.state(sound.name) == SoundState::stopped, "Level teardown left a redraw pen running");
            }
        }
        build(4, 1.2f);
        world.level->update(world.idle, 0.6f);
        require(world.level->state() != LevelState::active, "Redraw time-up fixture did not end its match");
        for (const auto& sound : world.tuning.player().redraw_sounds)
        {
            require(world.state(sound.name) == SoundState::stopped, "Match end left a redraw pen running");
        }
        world.level.reset();
    }
}
