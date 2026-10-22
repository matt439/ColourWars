#include "game/pch.h"
#include "game/tuning.h"
#include "game/resource_validation.h"
#include "game/controls_text.h"
#include "game/drawable_text.h"

#include <optional>
#include <stdexcept>
#include <string>

// GameTuning::validate_resources and validate_match_sounds: the startup checks
// of the parsed tuning against the loaded fonts, frames, animations and sounds.
// The sounds are the two reference lists below, which game/sound_groups.cpp
// also walks to put each one in its group.

using namespace labrador;
using namespace mattmath;

void GameTuning::validate_resources(const RenderResources& render,
	const AudioResources& audio) const
{
	const ResourceValidation check(this->source_path_, render, audio);
	check.font("match.controls_text.font", this->match_.controls_font);
	for (const Key key : controls_text::jump_keys())
	{
		const std::wstring words = controls_text::keyboard_controls_reminder(key);
		const std::string where = this->source_path_ + ": match.controls_text.font";
		drawable_text::check_font_can_draw(&render, this->match_.controls_font, words, where);
		const Vector2F size = render.measure_text(render.resolve_sprite_font(this->match_.controls_font), words);
		if (!std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0.0f || size.y <= 0.0f)
		{ throw std::runtime_error(where + " requires finite positive effective keyboard reminder bounds."); }
	}
	check.frame("paint.frame", this->paint_.sheet, this->paint_.frame);
	const auto splash = [&](ProjectileType type, const char* key)
	{
		check.animation(std::string("paint.splash.animations.") + key,
			this->paint_.splash_sheet,
			this->paint_.splash_animations[static_cast<size_t>(type)]);
		const SpriteSheet* sheet = render.sprite_sheet(this->paint_.splash_sheet);
		const AnimationStrip& clip = sheet->animation_strip(sheet->resolve_animation_strip(
			this->paint_.splash_animations[static_cast<size_t>(type)]));
		if (clip.looping() || !(clip.frame_time() > 0.0f) || clip.frame_count() <= 0 ||
			!std::isfinite(clip.frame_time() * static_cast<float>(clip.frame_count())))
		{
			throw std::runtime_error(this->source_path_ + ": paint.splash.animations." + key +
				" must be nonlooping with a finite, positive lifetime.");
		}
	};
	splash(ProjectileType::spray, "spray");
	splash(ProjectileType::jet, "jet");
	splash(ProjectileType::rolling, "rolling");
	splash(ProjectileType::mist, "mist");
	splash(ProjectileType::ball, "ball");
	for (const SoundReference& sound : this->sound_references())
	{
		check.sound(sound);
	}

	const auto animation = [&](const char* key, const PlayerAnimationTuning& clip,
		bool action = false)
	{
		const std::string prefix = std::string("player.animations.") + key;
		check.animation(prefix + ".strip", clip.sheet, clip.strip);
		check.animation(prefix + ".paint_strip", clip.sheet, clip.paint_strip);
		check.frame("player.animations.uniform", clip.sheet, clip.uniform_texture);
		const SpriteSheet* sheet = render.sprite_sheet(clip.sheet);
		const AnimationStrip& body = sheet->animation_strip(sheet->resolve_animation_strip(clip.strip));
		const AnimationStrip& paint = sheet->animation_strip(sheet->resolve_animation_strip(clip.paint_strip));
		const auto& body_size = body.frame_rect(0);
		const auto& paint_size = paint.frame_rect(0);
		if (body.frame_count() != paint.frame_count() || body.frame_time() != paint.frame_time() ||
			body.looping() != paint.looping() || body_size.width != paint_size.width ||
			body_size.height != paint_size.height)
		{
			throw std::runtime_error(this->source_path_ + ": " + prefix +
				".paint_strip must match body frame count, time, looping and dimensions.");
		}
		const float frame_time = clip.frame_time.value_or(body.frame_time());
		if (action && (body.looping() || !std::isfinite(frame_time) || frame_time <= 0.0f ||
			!std::isfinite(frame_time * static_cast<float>(body.frame_count()))))
		{
			throw std::runtime_error(this->source_path_ + ": " + prefix +
				".strip must be a non-looping clip with a finite positive duration.");
		}
	};
	animation("idle", this->player_.idle);
	animation("walk", this->player_.walk);
	animation("running", this->player_.running);
	animation("jumping", this->player_.jumping);
	animation("falling", this->player_.falling);
	animation("takeoff", this->player_.takeoff, true);
	animation("land", this->player_.land, true);
	animation("fire", this->player_.fire, true);
	animation("hurt", this->player_.hurt, true);
	animation("elimination", this->player_.elimination, true);

	const auto weapon = [&](WeaponType type, const char* key)
	{
		const WeaponTuning& entry = this->weapon(type);
		const WeaponDetails& details = entry.details;
		const std::string prefix = std::string("weapons.") + key;
		check.frame(prefix + ".frame", details.sheet_name, details.frame_name);
		const std::string paint_name = details.frame_name + "_paint";
		check.frame(prefix + ".frame (paint overlay)", details.sheet_name, paint_name);
		const SpriteSheet* sheet = render.sprite_sheet(details.sheet_name);
		const SpriteFrame& body = sheet->sprite_frame(
			sheet->resolve_sprite_frame(details.frame_name));
		const SpriteFrame& paint = sheet->sprite_frame(
			sheet->resolve_sprite_frame(paint_name));
		if (body.source_rectangle().size() != paint.source_rectangle().size() ||
			body.origin() != paint.origin())
		{
			throw std::runtime_error(this->source_path_ + ": " + prefix +
				" paint overlay must match the base frame dimensions and origin.");
		}
		// Weapon's debug marker uses the weapon's own sheet as well.
		check.frame(prefix + ".sheet (nozzle marker)", details.sheet_name, "nozzle");
	};
	weapon(WeaponType::sprayer, "sprayer");
	weapon(WeaponType::sniper, "sniper");
	weapon(WeaponType::roller, "roller");
	weapon(WeaponType::mister, "mister");
	weapon(WeaponType::bucket, "bucket");

	const auto projectile = [&](ProjectileType type, const char* key)
	{
		const ProjectileDetails& details = this->projectile(type).details;
		check.animation(std::string("projectiles.") + key + ".animation",
			details.sheet_name, details.animation_strip_name);
	};
	projectile(ProjectileType::spray, "spray");
	projectile(ProjectileType::jet, "jet");
	projectile(ProjectileType::rolling, "rolling");
	projectile(ProjectileType::mist, "mist");
	projectile(ProjectileType::ball, "ball");
}

void GameTuning::validate_match_sounds(const RenderResources& render,
	const AudioResources& audio, const std::string& bank) const
{
	const ResourceValidation check(this->source_path_, render, audio);
	for (const SoundReference& sound : this->match_sound_references(bank))
	{
		check.sound(sound);
	}
}

std::vector<SoundReference> GameTuning::sound_references() const
{
	std::vector<SoundReference> sounds;
	const auto wave = [&](std::string key, const std::string& bank, const std::string& name)
	{
		sounds.push_back({ std::move(key), bank, name, false });
	};
	const auto effect = [&](std::string key, const std::string& bank, const std::string& name)
	{
		sounds.push_back({ std::move(key), bank, name, true });
	};

	for (size_t i = 0; i < this->paint_.splash_sounds.size(); ++i)
	{
		wave("paint.splash_sounds[" + std::to_string(i) + "].name",
			this->paint_.sound_bank, this->paint_.splash_sounds[i].name);
	}
	wave("paint.capture_sound.name", this->paint_.sound_bank,
		this->paint_.capture_sound.name);
	wave("paint.dud_sound.name", this->paint_.sound_bank,
		this->paint_.dud_sound.name);
	const auto impact = [&](ProjectileType type, const char* key)
	{
		wave(std::string("paint.impact_sounds.") + key + ".name",
			this->paint_.sound_bank,
			this->paint_.impact_sounds[static_cast<size_t>(type)].name);
	};
	impact(ProjectileType::spray, "spray");
	impact(ProjectileType::jet, "jet");
	impact(ProjectileType::rolling, "rolling");
	impact(ProjectileType::mist, "mist");
	impact(ProjectileType::ball, "ball");

	wave("player.sounds.damage.name", this->player_.sound_bank,
		this->player_.damage_sound.name);
	wave("player.sounds.death.name", this->player_.sound_bank,
		this->player_.death_sound.name);
	wave("player.sounds.jump.name", this->player_.sound_bank,
		this->player_.jump_sound.name);
	wave("player.sounds.respawn.name", this->player_.sound_bank,
		this->player_.respawn_sound.name);
	for (size_t i = 0; i < this->player_.redraw_sounds.size(); ++i)
	{
		effect("player.redraw_sounds[" + std::to_string(i) + "].name",
			this->player_.sound_bank, this->player_.redraw_sounds[i].name);
	}
	const auto movement_choices = [&](const char* key,
		const std::vector<SoundTuning>& choices)
	{
		for (size_t i = 0; i < choices.size(); ++i)
		{
			wave(std::string("player.movement_sounds.") + key + "[" +
				std::to_string(i) + "].name", this->player_.sound_bank, choices[i].name);
		}
	};
	movement_choices("land", this->player_.movement_sounds.land);
	movement_choices("step", this->player_.movement_sounds.step);
	movement_choices("step_painted", this->player_.movement_sounds.step_painted);
	wave("player.movement_sounds.skid.name", this->player_.sound_bank,
		this->player_.movement_sounds.skid.name);

	const auto weapon = [&](WeaponType type, const char* key)
	{
		const WeaponTuning& entry = this->weapon(type);
		const WeaponDetails& details = entry.details;
		const std::string prefix = std::string("weapons.") + key;
		wave(prefix + ".shoot_sound.name", details.sound_bank_name,
			details.shoot_sound_name);
		wave(prefix + ".feedback_sounds.empty.name", details.sound_bank_name,
			entry.feedback_sounds.empty.name);
		wave(prefix + ".feedback_sounds.recovered.name", details.sound_bank_name,
			entry.feedback_sounds.recovered.name);
		const auto feedback_sound = [&](const char* feedback,
			const std::optional<SoundTuning>& sound)
		{
			if (sound.has_value())
			{
				wave(prefix + ".feedback_sounds." + feedback + ".name",
					details.sound_bank_name, sound->name);
			}
		};
		feedback_sound("stop", entry.feedback_sounds.stop);
		feedback_sound("ready", entry.feedback_sounds.ready);
		feedback_sound("lift", entry.feedback_sounds.lift);
		feedback_sound("settle", entry.feedback_sounds.settle);
		if (entry.loop_sounds.present)
		{
			for (size_t index = 0; index < entry.loop_sounds.team_a.size(); ++index)
			{
				effect(prefix + ".loop_sounds.team_a[" + std::to_string(index) + "]",
					details.sound_bank_name, entry.loop_sounds.team_a[index]);
				effect(prefix + ".loop_sounds.team_b[" + std::to_string(index) + "]",
					details.sound_bank_name, entry.loop_sounds.team_b[index]);
			}
		}
	};
	weapon(WeaponType::sprayer, "sprayer");
	weapon(WeaponType::sniper, "sniper");
	weapon(WeaponType::roller, "roller");
	weapon(WeaponType::mister, "mister");
	weapon(WeaponType::bucket, "bucket");
	return sounds;
}

std::vector<SoundReference> GameTuning::match_sound_references(const std::string& bank) const
{
	const auto effect = [&](const char* key, const SoundTuning& sound)
	{
		return SoundReference{ key, bank, sound.name, true };
	};
	const auto wave = [&](const char* key, const SoundTuning& sound)
	{
		return SoundReference{ key, bank, sound.name, false };
	};
	return {
		effect("match.sounds.countdown.name", this->match_.countdown_sound),
		effect("match.sounds.go.name", this->match_.go_sound),
		effect("match.sounds.time_up.name", this->match_.time_up_sound),
		effect("match.sounds.last_ten.name", this->match_.last_ten_sound),
		effect("match.sounds.zoom_out.name", this->match_.zoom_out_sound),
		effect("match.sounds.room_tone.name", this->match_.room_tone_sound),
		wave("match.sounds.hover.name", this->match_.hover_sound),
		wave("match.sounds.click.name", this->match_.click_sound),
		effect("match.sounds.sigh.name", this->match_.sigh_sound),
		effect("match.sounds.chair.name", this->match_.chair_sound),
		effect("match.sounds.glass_in.name", this->match_.glass_in_sound),
		wave("match.sounds.close.name", this->match_.close_sound),
		effect("match.sounds.celebrate.name", this->match_.celebrate_sound),
		effect("match.sounds.slump.name", this->match_.slump_sound),
		effect("match.sounds.colour_in.name", this->match_.colour_in_sound),
		effect("match.sounds.skip.name", this->match_.skip_sound),
		effect("match.sounds.undo.name", this->match_.undo_sound),
	};
}
