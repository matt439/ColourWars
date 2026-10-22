#include "game/pch.h"
#include "game/sound_groups.h"

#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace labrador;

namespace
{
	const char* const music_group = "music";
	const char* const sfx_group = "sfx";

	SoundGroupHandle group(AudioResources& audio, const char* name)
	{
		try
		{
			return audio.resolve_sound_group(name);
		}
		catch (const std::out_of_range&)
		{
			return audio.add_sound_group(name);
		}
	}
}

void sound_groups::assign(AudioResources& audio, const GameTuning& tuning,
	const Presentation& presentation, const Registry<LevelLoadedInfo>& level_infos,
	const StageList& stages)
{
	std::vector<SoundReference> sounds = tuning.sound_references();
	const std::vector<SoundReference> menu = presentation.sound_references();
	sounds.insert(sounds.end(), menu.begin(), menu.end());
	for (const Stage& stage : stages)
	{
		const LevelLoadedInfo& info = *level_infos.get(stage.name);
		const std::string bank = info.sound_bank_name();
		const std::vector<SoundReference> match = tuning.match_sound_references(bank);
		sounds.insert(sounds.end(), match.begin(), match.end());
		const std::string where = "levels/" + stage.name + ".json: sound.";
		if (info.has_music())
		{
			sounds.push_back({ where + "music_name", bank, info.music_name(), true,
				SoundGroupRole::music });
		}
		if (info.has_end_sting())
		{
			sounds.push_back({ where + "end_sting_name", bank, info.end_sting_name(), true,
				SoundGroupRole::music });
		}
	}

	// Membership is per wave or effect, not per play, so one sound cannot be
	// music in one place and an effect in another - and the volume preview
	// stays outside the SFX group only while nothing else names its wave.
	std::map<std::tuple<std::string, std::string, bool>, const SoundReference*> seen;
	for (const SoundReference& sound : sounds)
	{
		const auto [at, added] = seen.try_emplace(
			std::make_tuple(sound.bank, sound.name, sound.effect), &sound);
		if (!added && at->second->group != sound.group)
		{
			throw std::runtime_error(sound.key + " and " + at->second->key + " both name " +
				(sound.effect ? "effect '" : "wave '") + sound.name +
				"', but play it under different volume settings; give one its own sound.");
		}
	}

	const SoundGroupHandle music = group(audio, music_group);
	const SoundGroupHandle sfx = group(audio, sfx_group);
	for (const SoundReference& sound : sounds)
	{
		if (sound.group == SoundGroupRole::none) { continue; }
		const SoundGroupHandle into = sound.group == SoundGroupRole::music ? music : sfx;
		SoundBank* bank = audio.sound_bank(sound.bank);
		if (sound.effect)
		{
			bank->set_effect_group(bank->resolve_effect(sound.name), into);
		}
		else
		{
			bank->set_wave_group(bank->resolve_wave(sound.name), into);
		}
	}
}

void sound_groups::apply(AudioResources& audio, const PlaySettings& settings)
{
	audio.set_group_volume(audio.resolve_sound_group(music_group), settings.music_gain());
	audio.set_group_volume(audio.resolve_sound_group(sfx_group), settings.sfx_gain());
}
