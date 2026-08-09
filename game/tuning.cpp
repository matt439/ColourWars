#include "game/pch.h"
#include "game/tuning.h"

#include "engine/assets/json.h"

#include <optional>
#include <stdexcept>

using namespace artattack;
using namespace mattmath;

namespace
{
	// Every read below goes through JsonValue, so a missing key, a wrong type
	// or a hand-edit that broke the file is a startup failure naming the file,
	// the position in it and the key. The four helpers here are for the
	// vocabulary JsonValue cannot check on its own: a name that has to be one
	// of a fixed set. Each rejects with where() so an unknown name reads the
	// same as a missing one (T6).

	Vector2F read_vector(const JsonValue& parent, const char* key)
	{
		const JsonValue vector = parent.object(key);
		return Vector2F(vector.number("x"), vector.number("y"));
	}

	projectile_type read_projectile_type(const JsonValue& parent,
		const char* key)
	{
		const std::string name = parent.string(key);

		if (name == "spray") { return SPRAY; }
		if (name == "jet") { return JET; }
		if (name == "rolling") { return ROLLING; }
		if (name == "mist") { return MIST; }
		if (name == "ball") { return BALL; }

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

	SoundTuning read_sound(const JsonValue& parent, const char* key)
	{
		const JsonValue sound = parent.object(key);

		SoundTuning tuning;
		tuning.name = sound.string("name");
		tuning.volume = sound.number("volume");
		return tuning;
	}

	// The sheet and the uniform texture are the same for every clip, so they
	// are written once above the clips rather than five times inside them.
	PlayerAnimationTuning read_animation(const JsonValue& parent,
		const char* key, const std::string& sheet, const std::string& uniform)
	{
		const JsonValue clip = parent.object(key);

		PlayerAnimationTuning tuning;
		tuning.sheet = sheet;
		tuning.uniform_texture = uniform;
		tuning.strip = clip.string("strip");
		tuning.frame_time =
			clip.has("frame_time") ? clip.number("frame_time") : -1.0f;
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

	MatchTuning read_match(const JsonValue& root)
	{
		const JsonValue json = root.object("match");

		MatchTuning match;
		match.duration = json.number("duration");
		match.countdown = json.number("countdown");
		match.zoom_out = json.number("zoom_out");
		match.overview = json.number("overview");

		const JsonValue text = json.object("countdown_text");
		match.countdown_font = text.string("font");
		match.countdown_colour = read_colour(text, "colour");
		match.countdown_shadow_colour = read_colour(text, "shadow_colour");
		match.countdown_shadow_offset = read_vector(text, "shadow_offset");
		match.countdown_scale = text.number("scale");

		const JsonValue sounds = json.object("sounds");
		match.countdown_sound = read_sound(sounds, "countdown");
		match.zoom_out_sound = read_sound(sounds, "zoom_out");

		return match;
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
		player.drop_down_velocity = json.number("drop_down_velocity");
		player.drop_down_analog_threshold =
			json.number("drop_down_analog_threshold");
		player.starting_health = json.number("starting_health");
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

		const JsonValue shoot_sound = json.object("shoot_sound");
		details.shoot_sound_name = shoot_sound.string("name");
		details.shoot_sound_volume = shoot_sound.number("volume");

		if (json.has("add_player_velocity"))
		{
			weapon.relative.add_vel =
				read_add_player_velocity(json, "add_player_velocity");
			weapon.relative.player_vel_amount =
				json.number("player_velocity_amount");
		}

		weapon.loop_sounds = read_loop_sounds(json);
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
	const JsonValue root = document.root();

	this->match_ = read_match(root);
	this->player_ = read_player(root);

	const JsonValue weapons = root.object("weapons");
	this->ammo_regen_delay_ = weapons.number("ammo_regen_delay");
	this->ammo_regen_rate_ = weapons.number("ammo_regen_rate");
	this->starting_ammo_ = weapons.number("starting_ammo");

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
		projectile_type type, const char* key)
	{
		const size_t index = static_cast<size_t>(type);
		this->projectiles_[index] = read_projectile(projectiles, key);
		this->projectiles_[index].max_velocity = max_velocity;
		this->projectile_tuned_[index] = true;
	};

	add_projectile(SPRAY, "spray");
	add_projectile(JET, "jet");
	add_projectile(ROLLING, "rolling");
	add_projectile(MIST, "mist");
	add_projectile(BALL, "ball");
}

const MatchTuning& GameTuning::match() const
{
	return this->match_;
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

const ProjectileTuning& GameTuning::projectile(projectile_type type) const
{
	const size_t index = static_cast<size_t>(type);
	if (index >= PROJECTILE_COUNT || !this->projectile_tuned_[index])
	{
		throw std::out_of_range("projectile " + std::to_string(index) +
			" has no tuning in ./tuning.json.");
	}
	return this->projectiles_[index];
}
