#pragma once

void check_umbrella_cues(World& world)
{
	const auto& record = world.tuning.weapon(WeaponType::umbrella);
	const auto& canopy = record.canopy;
	const int original_volume = world.preferences.sfx_volume;
	world.set_sfx_volume(50);
	auto weapon = world.weapon(world.tuning, WeaponType::umbrella);
	world.begin = recorded_sounds(world.device).size();

	// Start with a ready tool. The fan gets one cue for the whole paid shot.
	update_weapon(*weapon, false, record.details.shoot_interval + weapon_step);
	const auto fan = update_weapon(*weapon, true);
	update_weapon(*weapon, true);
	require(fan.size() == static_cast<size_t>(record.pellets) &&
		world.wave_plays(record.details.shoot_sound_name) == 1 &&
		world.wave_plays(canopy.open_sound.name) == 1,
		"Umbrella fan or held canopy repeated its opening or firing sound");
	weapon->take_canopy_hit();
	update_weapon(*weapon, false);
	weapon->take_canopy_hit();
	require(world.wave_plays(canopy.block_sound.name) == 1,
		"Closed umbrella played a block sound");
	update_weapon(*weapon, true);
	require(world.wave_plays(canopy.open_sound.name) == 2,
		"Reopening the umbrella did not play its opening sound");
	for (int i = 1; i < canopy.hits; ++i) { weapon->take_canopy_hit(); }
	weapon->take_canopy_hit();
	require(world.wave_plays(canopy.block_sound.name) == canopy.hits - 1 &&
		world.wave_plays(canopy.break_sound.name) == 1,
		"Umbrella ordinary blocks, final break or hits during recovery sounded out of order");
	const float gain = world.preferences.sfx_gain();
	world.wave_volume_is({ record.details.shoot_sound_name, record.details.shoot_sound_volume }, gain);
	world.wave_volume_is(canopy.open_sound, gain);
	world.wave_volume_is(canopy.block_sound, gain);
	world.wave_volume_is(canopy.break_sound, gain);
	world.set_sfx_volume(original_volume);
}
