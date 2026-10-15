#include "game/pch.h"
#include "game/tuning.h"
#include "game/resource_validation.h"

#include "engine/assets/json.h"

#include <optional>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

namespace
{
	// Enum parsers use JsonValue's source context for unknown names.

	Vector2F read_vector(const JsonValue& parent, const char* key)
	{
		const JsonValue vector = parent.object(key);
		return Vector2F(vector.number("x"), vector.number("y"));
	}

	ProjectileType read_projectile_type(const JsonValue& parent,
		const char* key)
	{
		const std::string name = parent.string(key);

		if (name == "spray") { return ProjectileType::spray; }
		if (name == "jet") { return ProjectileType::jet; }
		if (name == "rolling") { return ProjectileType::rolling; }
		if (name == "mist") { return ProjectileType::mist; }
		if (name == "ball") { return ProjectileType::ball; }

		throw std::runtime_error(parent.where() + ": '" + name +
			"' is not a projectile. Expected one of spray, jet, rolling, "
			"mist, ball.");
	}

	RotationOrigin read_rotation_origin(const JsonValue& parent,
		const char* key)
	{
		const std::string name = parent.string(key);

		if (name == "no_rotation") { return RotationOrigin::no_rotation; }
		if (name == "center") { return RotationOrigin::center; }
		if (name == "top_left") { return RotationOrigin::top_left; }
		if (name == "left_center") { return RotationOrigin::left_center; }

		throw std::runtime_error(parent.where() + ": '" + name +
			"' is not a rotation origin. Expected one of no_rotation, center, "
			"top_left, left_center.");
	}

	AddPlayerVelocity read_add_player_velocity(const JsonValue& parent,
		const char* key)
	{
		const std::string name = parent.string(key);

		if (name == "none") { return AddPlayerVelocity::none; }
		if (name == "x_only") { return AddPlayerVelocity::x_only; }
		if (name == "y_only") { return AddPlayerVelocity::y_only; }
		if (name == "x_and_y") { return AddPlayerVelocity::x_and_y; }

		throw std::runtime_error(parent.where() + ": '" + name +
			"' is not a velocity mode. Expected one of none, x_only, y_only, "
			"x_and_y.");
	}

	DiffusionType read_diffusion_type(const JsonValue& parent, const char* key)
	{
		const std::string name = parent.string(key);

		if (name == "no_diffusion") { return DiffusionType::no_diffusion; }
		if (name == "linear_size") { return DiffusionType::linear_size; }
		if (name == "linear_size_and_col_rect")
		{
			return DiffusionType::linear_size_and_col_rect;
		}

		throw std::runtime_error(parent.where() + ": '" + name +
			"' is not a diffusion. Expected one of no_diffusion, linear_size, "
			"linear_size_and_col_rect.");
	}

	// Avoid trailing decimal zeroes in authored-value diagnostics.
	std::string number_text(float value)
	{
		std::string text = std::to_string(value);
		const size_t last = text.find_last_not_of('0');
		if (text.find('.') != std::string::npos && last != std::string::npos)
		{
			text.erase(text[last] == '.' ? last : last + 1);
		}
		return text;
	}

	// Reject only ranges required by gameplay. Write comparisons so NaN fails.
	float read_number_in(const JsonValue& parent, const char* key,
		float low, float high)
	{
		const float value = parent.number(key);

		if (!(value >= low && value <= high))
		{
			throw std::runtime_error(parent.where() + ": '" + key + "' is " +
				number_text(value) + ", which is outside [" +
				number_text(low) + ", " + number_text(high) + "].");
		}
		return value;
	}

	float read_positive(const JsonValue& parent, const char* key)
	{
		const float value = parent.number(key);

		if (!std::isfinite(value) || !(value > 0.0f))
		{
			throw std::runtime_error(parent.where() + ": '" + key + "' is " +
				number_text(value) + ", and must be finite and greater than zero.");
		}
		return value;
	}

	SoundTuning read_sound(const JsonValue& sound)
	{
		SoundTuning tuning;
		tuning.name = sound.string("name");

		// Reject authored volumes outside [0, 1] before SoundBank clamps them.
		tuning.volume = read_number_in(sound, "volume", 0.0f, 1.0f);
		return tuning;
	}

	SoundTuning read_sound(const JsonValue& parent, const char* key)
	{
		return read_sound(parent.object(key));
	}

	// Clips share the sheet and uniform texture declared on their parent.
	PlayerAnimationTuning read_animation(const JsonValue& parent,
		const char* key, const std::string& sheet, const std::string& uniform)
	{
		const JsonValue clip = parent.object(key);

		PlayerAnimationTuning tuning;
		tuning.sheet = sheet;
		tuning.uniform_texture = uniform;
		tuning.strip = clip.string("strip");
		tuning.frame_time = clip.has("frame_time")
			? std::optional<float>(clip.number("frame_time"))
			: std::nullopt;
		return tuning;
	}

	Colour read_colour(const JsonValue& parent, const char* key)
	{
		const std::string name = parent.string(key);
		const std::optional<Colour> colour = colour_from_name(name);
		if (!colour.has_value())
		{
			throw std::runtime_error(parent.where() + ": '" + name +
				"' is not a colour this engine knows by name.");
		}
		return *colour;
	}

	float json_non_negative(const JsonValue& parent, const char* key)
	{
		const float value = parent.number(key);
		if (!std::isfinite(value) || value < 0.0f)
		{
			throw std::runtime_error(parent.where() + ": '" + key +
				"' must be finite and not negative.");
		}
		return value;
	}

	InputTuning read_input(const JsonValue& root)
	{
		const JsonValue json = root.object("input");
		InputTuning input;
		input.trigger_shoot_threshold = read_number_in(json, "trigger_shoot_threshold", 0.0f, 1.0f);
		input.trigger_jump_threshold = read_number_in(json, "trigger_jump_threshold", 0.0f, 1.0f);
		input.movement_deadzone = read_number_in(json, "movement_deadzone", 0.0f, 1.0f);
		input.menu_engage_threshold = read_number_in(json, "menu_engage_threshold", 0.0f, 1.0f);
		input.menu_release_threshold = read_number_in(json, "menu_release_threshold", 0.0f, 1.0f);
		if (input.movement_deadzone >= 1.0f || input.menu_release_threshold <= 0.0f ||
			input.menu_release_threshold >= input.menu_engage_threshold)
		{
			throw std::runtime_error(json.where() +
				": movement_deadzone must be below 1; menu thresholds must satisfy 0 < release < engage <= 1.");
		}
		return input;
	}

	MatchTuning read_match(const JsonValue& root)
	{
		const JsonValue json = root.object("match");

		MatchTuning match;

		// Require a playable duration and a positive divisor for the zoom-out ratio.
		match.duration = read_positive(json, "duration");
		const JsonValue choices = json.array("duration_choices");
		for (size_t index = 0; index < choices.size(); ++index)
		{
			match.duration_choices.push_back(read_positive(choices.at(index), "seconds"));
		}
		if (match.duration_choices.empty())
		{
			throw std::runtime_error(choices.where() + ": must contain a match duration.");
		}
		match.zoom_out = read_positive(json, "zoom_out");

		// Zero skips the countdown or overview; these durations have no range check.
		match.countdown = json.number("countdown");
		match.overview = json.number("overview");

		const JsonValue text = json.object("countdown_text");
		match.countdown_font = text.string("font");
		match.countdown_colour = read_colour(text, "colour");
		match.countdown_shadow_colour = read_colour(text, "shadow_colour");
		match.countdown_shadow_offset = read_vector(text, "shadow_offset");
		match.countdown_scale = text.number("scale");

		const JsonValue controls = json.object("controls_text");
		match.controls_font = controls.string("font");
		match.controls_colour = read_colour(controls, "colour");
		match.controls_shadow_colour = read_colour(controls, "shadow_colour");
		match.controls_shadow_offset = read_vector(controls, "shadow_offset");
		match.controls_y = read_number_in(controls, "y", 0.0f, 1.0f);

		const JsonValue sounds = json.object("sounds");
		match.countdown_sound = read_sound(sounds, "countdown");
		match.go_sound = read_sound(sounds, "go");
		match.time_up_sound = read_sound(sounds, "time_up");
		match.last_ten_sound = read_sound(sounds, "last_ten");
		match.zoom_out_sound = read_sound(sounds, "zoom_out");
		match.room_tone_sound = read_sound(sounds, "room_tone");
		match.hover_sound = read_sound(sounds, "hover");
		match.click_sound = read_sound(sounds, "click");
		match.sigh_sound = read_sound(sounds, "sigh");
		match.chair_sound = read_sound(sounds, "chair");
		match.glass_in_sound = read_sound(sounds, "glass_in");
		match.close_sound = read_sound(sounds, "close");
		match.celebrate_sound = read_sound(sounds, "celebrate");
		match.slump_sound = read_sound(sounds, "slump");
		match.colour_in_sound = read_sound(sounds, "colour_in");
		match.skip_sound = read_sound(sounds, "skip");
		match.undo_sound = read_sound(sounds, "undo");

		const JsonValue opening = json.object("opening");
		match.opening.settle = json_non_negative(opening, "settle");
		match.opening.preview = read_positive(opening, "preview");
		match.opening.undo = json_non_negative(opening, "undo");
		match.opening.leave = read_positive(opening, "leave");
		match.opening.push = read_positive(opening, "push");
		const JsonValue reveal = json.object("reveal");
		match.reveal.walk_in = read_positive(reveal, "walk_in");
		match.reveal.hover = read_positive(reveal, "hover");
		match.reveal.decide = read_positive(reveal, "decide");
		match.reveal.show = read_positive(reveal, "show");
		match.reveal.leave = read_positive(reveal, "leave");
		match.reveal.react = read_positive(reveal, "react");
		if (match.opening.push > match.countdown)
		{
			throw std::runtime_error(opening.where() +
				": 'push' happens inside the countdown, so it cannot be longer.");
		}

		return match;
	}

	PaintTuning read_paint(const JsonValue& root)
	{
		const JsonValue json = root.object("paint");

		PaintTuning paint;

		// Positive tile lengths are required to derive finite strip counts.
		paint.size = Vector2F(
			read_positive(json.object("size"), "x"),
			read_positive(json.object("size"), "y"));

		paint.thickness = json.number("thickness");
		paint.sheet = json.string("sheet");
		paint.frame = json.string("frame");

		const JsonValue splash = json.object("splash");
		paint.splash_sheet = splash.string("sheet");
		paint.splash_animation = splash.string("animation");

		paint.sound_bank = json.string("sound_bank");
		const JsonValue choices = json.array("splash_sounds");
		if (choices.size() == 0)
		{
			throw std::runtime_error(choices.where() +
				": at least one splash sound is required.");
		}
		paint.splash_sounds.reserve(choices.size());
		for (size_t i = 0; i < choices.size(); ++i)
		{
			paint.splash_sounds.push_back(read_sound(choices.at(i)));
		}
		paint.capture_sound = read_sound(json, "capture_sound");
		paint.dud_sound = read_sound(json, "dud_sound");
		const JsonValue impacts = json.object("impact_sounds");
		const auto impact = [&](ProjectileType type, const char* key)
		{
			paint.impact_sounds[static_cast<size_t>(type)] = read_sound(impacts, key);
		};
		impact(ProjectileType::spray, "spray");
		impact(ProjectileType::jet, "jet");
		impact(ProjectileType::rolling, "rolling");
		impact(ProjectileType::mist, "mist");
		impact(ProjectileType::ball, "ball");
		paint.min_impact_interval = read_positive(json, "min_impact_interval");
		paint.min_capture_interval = read_positive(json, "min_capture_interval");
		return paint;
	}

	PlayerMovementSounds read_movement_sounds(const JsonValue& player)
	{
		const JsonValue json = player.object("movement_sounds");
		PlayerMovementSounds sounds;
		const auto read_choices = [&json](const char* key)
		{
			const JsonValue choices = json.array(key);
			if (choices.size() == 0)
			{
				throw std::runtime_error(choices.where() +
					": at least one movement sound is required.");
			}
			std::vector<SoundTuning> result;
			result.reserve(choices.size());
			for (size_t i = 0; i < choices.size(); ++i)
			{
				result.push_back(read_sound(choices.at(i)));
			}
			return result;
		};
		sounds.land = read_choices("land");
		sounds.step = read_choices("step");
		sounds.step_painted = read_choices("step_painted");
		sounds.skid = read_sound(json, "skid");
		sounds.landing_full_speed = read_positive(json, "landing_full_speed");
		sounds.landing_min_gain = read_number_in(json, "landing_min_gain", 0.0f, 1.0f);
		sounds.stride_distance = read_positive(json, "stride_distance");
		sounds.min_step_interval = read_positive(json, "min_step_interval");
		sounds.skid_min_speed = read_positive(json, "skid_min_speed");
		sounds.skid_repeat_delay = read_positive(json, "skid_repeat_delay");
		return sounds;
	}

	PlayerTuning read_player(const JsonValue& root)
	{
		const JsonValue json = root.object("player");

		PlayerTuning player;
		player.size = read_vector(json, "size");
		player.max_velocity = read_vector(json, "max_velocity");
		player.x_initial_velocity = json.number("x_initial_velocity");
		player.x_acceleration = json.number("x_acceleration");
		player.x_deacceleration = json.number("x_deacceleration");
		player.gravity = json.number("gravity");
		player.ramp_gravity_multiplier = json.number("ramp_gravity_multiplier");
		player.jump_launch_velocity = json.number("jump_launch_velocity");
		player.jump_acceleration = json.number("jump_acceleration");
		player.jump_max_time = json.number("jump_max_time");
		player.jump_buffer_time = read_number_in(json, "jump_buffer_time", 0.0f, 1.0f);
		player.ledge_grace_time = read_number_in(json, "ledge_grace_time", 0.0f, 1.0f);
		player.drop_down_velocity = json.number("drop_down_velocity");
		player.drop_down_analog_threshold =
			json.number("drop_down_analog_threshold");
		player.starting_health = read_positive(json, "starting_health");
		player.health_regen_delay = json.number("health_regen_delay");
		player.health_regen_rate = json.number("health_regen_rate");
		player.respawn_delay = json.number("respawn_delay");
		player.damage_sound_delay = json.number("damage_sound_delay");

		player.sound_bank = json.string("sound_bank");

		const JsonValue sounds = json.object("sounds");
		player.damage_sound = read_sound(sounds, "damage");
		player.death_sound = read_sound(sounds, "death");
		player.jump_sound = read_sound(sounds, "jump");
		player.respawn_sound = read_sound(sounds, "respawn");
		const JsonValue redraw = json.array("redraw_sounds");
		if (redraw.size() != player.redraw_sounds.size())
		{
			throw std::runtime_error(redraw.where() +
				": a separate redraw sound is required for each of the four players.");
		}
		for (size_t i = 0; i < player.redraw_sounds.size(); ++i)
		{
			player.redraw_sounds[i] = read_sound(redraw.at(i));
			for (size_t previous = 0; previous < i; ++previous)
			{
				if (player.redraw_sounds[i].name == player.redraw_sounds[previous].name)
				{
					throw std::runtime_error(redraw.at(i).where() +
						": each redraw sound must name a separate effect instance.");
				}
			}
		}
		player.movement_sounds = read_movement_sounds(json);

		const JsonValue animations = json.object("animations");
		const std::string sheet = animations.string("sheet");
		const std::string uniform = animations.string("uniform");
		player.idle = read_animation(animations, "idle", sheet, uniform);
		player.walk = read_animation(animations, "walk", sheet, uniform);
		player.running = read_animation(animations, "running", sheet, uniform);
		player.jumping = read_animation(animations, "jumping", sheet, uniform);
		player.falling = read_animation(animations, "falling", sheet, uniform);

		return player;
	}

	WeaponLoopSounds read_loop_sounds(const JsonValue& json)
	{
		WeaponLoopSounds sounds;
		if (!json.has("loop_sounds"))
		{
			return sounds;
		}

		const JsonValue block = json.object("loop_sounds");

		const auto read_team = [&block](const char* key,
			std::array<std::string, 4>& into)
		{
			const JsonValue names = block.array(key);
			if (names.size() != into.size())
			{
				throw std::runtime_error(names.where() +
					": a loop sound is needed for each of the four players, "
					"and this names " + std::to_string(names.size()) + ".");
			}
			for (size_t i = 0; i < into.size(); i++)
			{
				into[i] = names.at(i).as_string();
			}
		};

		read_team("team_a", sounds.team_a);
		read_team("team_b", sounds.team_b);
		sounds.present = true;
		return sounds;
	}

	WeaponTuning read_weapon(const JsonValue& weapons, const char* key)
	{
		const JsonValue json = weapons.object(key);

		WeaponTuning weapon;
		WeaponDetails& details = weapon.details;
		details.offset = read_vector(json, "offset");
		details.size = read_vector(json, "size");
		details.nozzle_offset = read_vector(json, "nozzle_offset");
		details.shoot_interval = json.number("shoot_interval");
		details.starting_vel_length = json.number("starting_vel_length");
		details.ammo_usage = json.number("ammo_usage");
		details.sheet_name = json.string("sheet");
		details.frame_name = json.string("frame");
		details.proj_type = read_projectile_type(json, "projectile");
		details.sound_bank_name = json.string("sound_bank");

		// Use the shared volume validation even though WeaponDetails stores separate fields.
		const SoundTuning shoot_sound = read_sound(json, "shoot_sound");
		details.shoot_sound_name = shoot_sound.name;
		details.shoot_sound_volume = shoot_sound.volume;

		if (json.has("add_player_velocity"))
		{
			weapon.relative.add_vel =
				read_add_player_velocity(json, "add_player_velocity");
			weapon.relative.player_vel_amount =
				json.number("player_velocity_amount");
		}

		weapon.loop_sounds = read_loop_sounds(json);
		const JsonValue feedback = json.object("feedback_sounds");
		weapon.feedback_sounds.empty = read_sound(feedback, "empty");
		weapon.feedback_sounds.recovered = read_sound(feedback, "recovered");
		weapon.feedback_sounds.empty_repeat_delay = read_positive(feedback, "empty_repeat_delay");
		const auto optional_sound = [&feedback](const char* name) -> std::optional<SoundTuning>
		{
			return feedback.has(name) ? std::optional<SoundTuning>(read_sound(feedback, name)) : std::nullopt;
		};
		weapon.feedback_sounds.stop = optional_sound("stop");
		weapon.feedback_sounds.ready = optional_sound("ready");
		weapon.feedback_sounds.lift = optional_sound("lift");
		weapon.feedback_sounds.settle = optional_sound("settle");
		return weapon;
	}

	ProjectileTuning read_projectile(const JsonValue& projectiles,
		const char* key)
	{
		const JsonValue json = projectiles.object(key);

		ProjectileTuning projectile;
		ProjectileDetails& details = projectile.details;
		details.gravity = json.number("gravity");
		details.wind_resistance = json.number("wind_resistance");
		details.rotat_origin = read_rotation_origin(json, "rotation_origin");
		details.delete_timer = json.number("delete_timer");
		details.player_damage = json.number("player_damage");
		details.size = read_vector(json, "size");
		details.col_rect_size = read_vector(json, "collision_size");
		details.sheet_name = json.string("sheet");
		details.animation_strip_name = json.string("animation");

		if (json.has("diffusion"))
		{
			const JsonValue diffusion = json.object("diffusion");
			projectile.diffusion.type = read_diffusion_type(diffusion, "type");
			projectile.diffusion.start_time = diffusion.number("start_time");
			projectile.diffusion.end_time = diffusion.number("end_time");
			projectile.diffusion.end_scale =
				read_vector(diffusion, "end_scale");
		}

		return projectile;
	}
}

GameTuning::GameTuning(const char* path)
{
	const JsonDocument document = read_json_file(path);
	this->source_path_ = path;
	const JsonValue root = document.root();

	this->input_ = read_input(root);
	this->match_ = read_match(root);
	this->paint_ = read_paint(root);
	this->player_ = read_player(root);

	const JsonValue weapons = root.object("weapons");
	this->ammo_regen_delay_ = weapons.number("ammo_regen_delay");
	this->ammo_regen_rate_ = weapons.number("ammo_regen_rate");
	this->starting_ammo_ = read_positive(weapons, "starting_ammo");

	const auto add_weapon = [this, &weapons](WeaponType type, const char* key)
	{
		const size_t index = static_cast<size_t>(type);
		this->weapons_[index] = read_weapon(weapons, key);
		this->weapon_tuned_[index] = true;
	};

	add_weapon(WeaponType::sprayer, "sprayer");
	add_weapon(WeaponType::sniper, "sniper");
	add_weapon(WeaponType::roller, "roller");
	add_weapon(WeaponType::mister, "mister");
	add_weapon(WeaponType::bucket, "bucket");

	const JsonValue projectiles = root.object("projectiles");
	const Vector2F max_velocity = read_vector(projectiles, "max_velocity");

	const auto add_projectile = [this, &projectiles, &max_velocity](
		ProjectileType type, const char* key)
	{
		const size_t index = static_cast<size_t>(type);
		this->projectiles_[index] = read_projectile(projectiles, key);
		this->projectiles_[index].max_velocity = max_velocity;
		this->projectile_tuned_[index] = true;
	};

	add_projectile(ProjectileType::spray, "spray");
	add_projectile(ProjectileType::jet, "jet");
	add_projectile(ProjectileType::rolling, "rolling");
	add_projectile(ProjectileType::mist, "mist");
	add_projectile(ProjectileType::ball, "ball");
}

void GameTuning::validate_resources(const RenderResources& render,
	const AudioResources& audio) const
{
	const ResourceValidation check(this->source_path_, render, audio);
	check.font("match.countdown_text.font", this->match_.countdown_font);
	check.font("match.controls_text.font", this->match_.controls_font);
	check.frame("paint.frame", this->paint_.sheet, this->paint_.frame);
	check.animation("paint.splash.animation", this->paint_.splash_sheet,
		this->paint_.splash_animation);
	for (size_t i = 0; i < this->paint_.splash_sounds.size(); ++i)
	{
		check.wave("paint.splash_sounds[" + std::to_string(i) + "].name",
			this->paint_.sound_bank, this->paint_.splash_sounds[i].name);
	}
	check.wave("paint.capture_sound.name", this->paint_.sound_bank,
		this->paint_.capture_sound.name);
	check.wave("paint.dud_sound.name", this->paint_.sound_bank,
		this->paint_.dud_sound.name);
	const auto impact = [&](ProjectileType type, const char* key)
	{
		check.wave(std::string("paint.impact_sounds.") + key + ".name",
			this->paint_.sound_bank,
			this->paint_.impact_sounds[static_cast<size_t>(type)].name);
	};
	impact(ProjectileType::spray, "spray");
	impact(ProjectileType::jet, "jet");
	impact(ProjectileType::rolling, "rolling");
	impact(ProjectileType::mist, "mist");
	impact(ProjectileType::ball, "ball");

	const auto animation = [&](const char* key, const PlayerAnimationTuning& clip)
	{
		const std::string prefix = std::string("player.animations.") + key;
		check.animation(prefix + ".strip", clip.sheet, clip.strip);
		check.frame("player.animations.uniform", clip.sheet, clip.uniform_texture);
	};
	animation("idle", this->player_.idle);
	animation("walk", this->player_.walk);
	animation("running", this->player_.running);
	animation("jumping", this->player_.jumping);
	animation("falling", this->player_.falling);
	check.wave("player.sounds.damage.name", this->player_.sound_bank,
		this->player_.damage_sound.name);
	check.wave("player.sounds.death.name", this->player_.sound_bank,
		this->player_.death_sound.name);
	check.wave("player.sounds.jump.name", this->player_.sound_bank,
		this->player_.jump_sound.name);
	check.wave("player.sounds.respawn.name", this->player_.sound_bank,
		this->player_.respawn_sound.name);
	for (size_t i = 0; i < this->player_.redraw_sounds.size(); ++i)
	{
		check.effect("player.redraw_sounds[" + std::to_string(i) + "].name",
			this->player_.sound_bank, this->player_.redraw_sounds[i].name);
	}
	const auto movement_choices = [&](const char* key,
		const std::vector<SoundTuning>& choices)
	{
		for (size_t i = 0; i < choices.size(); ++i)
		{
			check.wave(std::string("player.movement_sounds.") + key + "[" +
				std::to_string(i) + "].name", this->player_.sound_bank, choices[i].name);
		}
	};
	movement_choices("land", this->player_.movement_sounds.land);
	movement_choices("step", this->player_.movement_sounds.step);
	movement_choices("step_painted", this->player_.movement_sounds.step_painted);
	check.wave("player.movement_sounds.skid.name", this->player_.sound_bank,
		this->player_.movement_sounds.skid.name);

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
		check.wave(prefix + ".shoot_sound.name", details.sound_bank_name,
			details.shoot_sound_name);
		check.wave(prefix + ".feedback_sounds.empty.name", details.sound_bank_name,
			entry.feedback_sounds.empty.name);
		check.wave(prefix + ".feedback_sounds.recovered.name", details.sound_bank_name,
			entry.feedback_sounds.recovered.name);
		const auto feedback_sound = [&](const char* key, const std::optional<SoundTuning>& sound)
		{
			if (sound.has_value())
			{
				check.wave(prefix + ".feedback_sounds." + key + ".name",
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
				check.effect(prefix + ".loop_sounds.team_a[" + std::to_string(index) + "]",
					details.sound_bank_name, entry.loop_sounds.team_a[index]);
				check.effect(prefix + ".loop_sounds.team_b[" + std::to_string(index) + "]",
					details.sound_bank_name, entry.loop_sounds.team_b[index]);
			}
		}
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
	check.effect("match.sounds.countdown.name", bank, this->match_.countdown_sound.name);
	check.effect("match.sounds.go.name", bank, this->match_.go_sound.name);
	check.effect("match.sounds.time_up.name", bank, this->match_.time_up_sound.name);
	check.effect("match.sounds.last_ten.name", bank, this->match_.last_ten_sound.name);
	check.effect("match.sounds.zoom_out.name", bank, this->match_.zoom_out_sound.name);
	check.effect("match.sounds.room_tone.name", bank, this->match_.room_tone_sound.name);
	check.wave("match.sounds.hover.name", bank, this->match_.hover_sound.name);
	check.wave("match.sounds.click.name", bank, this->match_.click_sound.name);
	check.effect("match.sounds.sigh.name", bank, this->match_.sigh_sound.name);
	check.effect("match.sounds.chair.name", bank, this->match_.chair_sound.name);
	check.effect("match.sounds.glass_in.name", bank, this->match_.glass_in_sound.name);
	check.wave("match.sounds.close.name", bank, this->match_.close_sound.name);
	check.effect("match.sounds.celebrate.name", bank, this->match_.celebrate_sound.name);
	check.effect("match.sounds.slump.name", bank, this->match_.slump_sound.name);
	check.effect("match.sounds.colour_in.name", bank, this->match_.colour_in_sound.name);
	check.effect("match.sounds.skip.name", bank, this->match_.skip_sound.name);
	check.effect("match.sounds.undo.name", bank, this->match_.undo_sound.name);
}

const MatchTuning& GameTuning::match() const
{
	return this->match_;
}

const PaintTuning& GameTuning::paint() const
{
	return this->paint_;
}

const InputTuning& GameTuning::input() const
{
	return this->input_;
}

const PlayerTuning& GameTuning::player() const
{
	return this->player_;
}

float GameTuning::ammo_regen_delay() const
{
	return this->ammo_regen_delay_;
}
float GameTuning::ammo_regen_rate() const
{
	return this->ammo_regen_rate_;
}
float GameTuning::starting_ammo() const
{
	return this->starting_ammo_;
}

const WeaponTuning& GameTuning::weapon(WeaponType type) const
{
	const size_t index = static_cast<size_t>(type);
	if (index >= WEAPON_COUNT || !this->weapon_tuned_[index])
	{
		throw std::out_of_range("weapon " + std::to_string(index) +
			" has no tuning: ./tuning.json describes the five weapons a "
			"player can pick, and this is not one of them.");
	}
	return this->weapons_[index];
}

const ProjectileTuning& GameTuning::projectile(ProjectileType type) const
{
	const size_t index = static_cast<size_t>(type);
	if (index >= PROJECTILE_COUNT || !this->projectile_tuned_[index])
	{
		throw std::out_of_range("projectile " + std::to_string(index) +
			" has no tuning in ./tuning.json.");
	}
	return this->projectiles_[index];
}
