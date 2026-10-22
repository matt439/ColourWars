#pragma once

// Every sound the content names plays under the volume it belongs to. The
// engine has no query for a sound's group, so each one is played once with
// one group muted and the other at full volume, and the level the recording
// device was sent says which group it is in. A sound left out of its group
// would play at full volume with the player's setting at zero.
namespace
{
	void check_sound_groups(World& world)
	{
		const int original_music = world.preferences.music_volume;
		const int original_sfx = world.preferences.sfx_volume;
		std::vector<SoundReference> sounds = world.tuning.sound_references();
		const std::vector<SoundReference> menu = world.presentation.sound_references();
		sounds.insert(sounds.end(), menu.begin(), menu.end());
		const std::vector<SoundReference> match = world.tuning.match_sound_references("paint_exe");
		sounds.insert(sounds.end(), match.begin(), match.end());
		for (const Stage& stage : world.stages)
		{
			const LevelLoadedInfo& info = *world.levels.get(stage.name);
			if (info.has_music())
			{
				sounds.push_back({ stage.name, "paint_exe", info.music_name(), true, SoundGroupRole::music });
			}
			if (info.has_end_sting())
			{
				sounds.push_back({ stage.name, "paint_exe", info.end_sting_name(), true, SoundGroupRole::music });
			}
		}
		int previews = 0;
		const SoundBank* bank = world.audio.sound_bank("paint_exe");
		for (const bool music_muted : { true, false })
		{
			world.set_music_volume(music_muted ? 0 : 100);
			world.set_sfx_volume(music_muted ? 100 : 0);
			for (const SoundReference& sound : sounds)
			{
				const bool muted = sound.group == SoundGroupRole::none ? false :
					(sound.group == SoundGroupRole::music) == music_muted;
				const size_t before = recorded_sounds(world.device).size();
				if (sound.effect)
				{
					const auto effect = bank->resolve_effect(sound.name);
					bank->play_effect(effect, false, 0.5f);
					bank->stop_effect(effect, true);
				}
				else
				{
					bank->play_wave(bank->resolve_wave(sound.name), 0.5f);
				}
				const auto& calls = recorded_sounds(world.device);
				require(calls.size() > before, "A grouped sound sent nothing to the device");
				const float sent = calls[before].volume;
				if (!close_to(sent, muted ? 0.0f : 0.5f))
				{
					throw std::runtime_error(sound.key + " ('" + sound.name +
						"') is not in the sound group of its volume setting");
				}
				previews += sound.group == SoundGroupRole::none ? 1 : 0;
			}
		}
		require(previews == 2, "The Controls volume preview is not the one ungrouped sound");
		world.set_music_volume(original_music);
		world.set_sfx_volume(original_sfx);
	}
}
