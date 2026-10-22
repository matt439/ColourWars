#pragma once

#include <string>

// Which user volume a sound plays under. The engine mixes one group volume
// into every level it sends for a sound (engine/audio/sound_group.h); which
// sound is in which group is this game's call, made in game/sound_groups.cpp.
enum class SoundGroupRole
{
	sfx,
	music,
	// Mixed under neither. Only the Controls volume preview, which auditions
	// a pending SFX value the sfx group does not carry yet.
	none,
};

// One sound the content names: the authored key for a diagnostic, the bank
// and the wave or effect, and the group it plays under. GameTuning and
// Presentation list theirs once, and both the startup check and the group
// assignment walk the same list, so a sound cannot be checked and left out
// of its group.
struct SoundReference
{
	std::string key;
	std::string bank;
	std::string name;
	bool effect = false;
	SoundGroupRole group = SoundGroupRole::sfx;
};
