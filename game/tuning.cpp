#include "game/pch.h"
#include "game/tuning.h"
#include "game/content_value.h"

#include "engine/assets/json.h"

#include <optional>
#include <cmath>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;
using namespace content_value;

namespace
{
	// Enum parsers use JsonValue's source context for unknown names.

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

	SoundTuning read_sound(const JsonValue& sound)
	{
		SoundTuning tuning;
		tuning.name = sound.string("name");

		// Reject authored volumes outside [0, 1] before SoundBank clamps them.
		tuning.volume = read_unit(sound, "volume");
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
		tuning.paint_strip = clip.identifier("paint_strip");
		tuning.frame_time = clip.has("frame_time")
			? std::optional<float>(read_positive(clip, "frame_time"))
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

	InputTuning read_input(const JsonValue& root)
	{
		const JsonValue json = root.object("input");
		InputTuning input;
		input.trigger_shoot_threshold = read_unit(json, "trigger_shoot_threshold");
		input.trigger_jump_threshold = read_unit(json, "trigger_jump_threshold");
		input.movement_deadzone = read_unit(json, "movement_deadzone");
		input.menu_engage_threshold = read_unit(json, "menu_engage_threshold");
		input.menu_release_threshold = read_unit(json, "menu_release_threshold");
		if (input.movement_deadzone >= 1.0f || input.menu_release_threshold <= 0.0f ||
			input.menu_release_threshold >= input.menu_engage_threshold)
		{
			throw std::runtime_error(json.where() +
				": movement_deadzone must be below 1; menu thresholds must satisfy 0 < release < engage <= 1.");
		}
		return input;
	}

	BotTuning read_bots(const JsonValue& root)
	{
		const JsonValue json = root.object("bots");
		BotTuning bots;
		bots.sight_distance = read_positive(json, "sight_distance");
		bots.reserve_low = read_unit(json, "reserve_low");
		bots.reserve_resume = read_unit(json, "reserve_resume");
		if (bots.reserve_resume <= bots.reserve_low)
		{
			throw std::runtime_error(json.where() + ": reserve_resume must exceed reserve_low.");
		}
		const auto steps = [](const JsonValue& value, const char* key)
		{
			const int count = value.integer(key);
			if (count < 1 || count > 3600)
			{
				throw std::runtime_error(value.where() + ": " + key + " must be between 1 and 3600 steps.");
			}
			return count;
		};
		bots.stuck_jump_steps = steps(json, "stuck_jump_steps");
		bots.stuck_turn_steps = steps(json, "stuck_turn_steps");
		bots.recovery_steps = steps(json, "recovery_steps");
		bots.progress_window_steps = steps(json, "progress_window_steps");
		bots.progress_distance = read_positive(json, "progress_distance");
		bots.finish_dwell_steps = steps(json, "finish_dwell_steps");
		bots.deliberate_shot_interval = read_positive(json, "deliberate_shot_interval");
		if (bots.stuck_turn_steps <= bots.stuck_jump_steps)
		{
			throw std::runtime_error(json.where() + ": stuck_turn_steps must exceed stuck_jump_steps.");
		}
		const char* names[]{ "easy", "normal", "hard" };
		for (size_t index = 0; index < bots.difficulties.size(); ++index)
		{
			const JsonValue entry = json.object(names[index]);
			BotDifficultyTuning& difficulty = bots.difficulties[index];
			difficulty.reaction_steps = steps(entry, "reaction_steps");
			difficulty.replan_steps = steps(entry, "replan_steps");
			difficulty.territory_candidates = entry.integer("territory_candidates");
			if (difficulty.territory_candidates < 1 || difficulty.territory_candidates > 256)
			{
				throw std::runtime_error(entry.where() + ": territory_candidates must be between 1 and 256.");
			}
			difficulty.aim_error_radians = read_number_in(entry, "aim_error_radians", 0.0f, PI);
		}
		return bots;
	}

	RumbleProfile read_rumble(const JsonValue& root)
	{
		const JsonValue json = root.object("rumble");
		RumbleProfile profile;
		constexpr const char* names[]{ "hit", "elimination", "hard_landing", "dry_fire" };
		for (size_t index = 0; index < profile.size(); ++index)
		{
			const JsonValue pulse = json.object(names[index]);
			profile[index] = { { read_unit(pulse, "low_frequency"), read_unit(pulse, "high_frequency") },
				read_number_in(pulse, "duration", 0.0f, maximum_rumble_pulse_seconds) };
		}
		return profile;
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

		// Zero skips the overview; the countdown holds the opening push, so is
		// checked against it below. Both are timers that run before or after
		// the match, so neither is compared with its duration.
		match.countdown = read_non_negative(json, "countdown");
		match.overview = read_non_negative(json, "overview");

		const JsonValue controls = json.object("controls_text");
		match.controls_font = controls.string("font");
		match.controls_colour = read_colour(controls, "colour");
		match.controls_shadow_colour = read_colour(controls, "shadow_colour");
		match.controls_shadow_offset = read_vector(controls, "shadow_offset");
		match.controls_y = read_unit(controls, "y");

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
		match.opening.settle = read_non_negative(opening, "settle");
		match.opening.preview = read_positive(opening, "preview");
		match.opening.undo = read_non_negative(opening, "undo");
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
		paint.size = read_positive_vector(json, "size");

		paint.thickness = read_positive(json, "thickness");
		paint.sheet = json.string("sheet");
		paint.frame = json.string("frame");

		const JsonValue splash = json.object("splash");
		paint.splash_sheet = splash.string("sheet");
		const JsonValue animations = splash.object("animations");
		const auto splash_animation = [&](ProjectileType type, const char* key)
		{
			paint.splash_animations[static_cast<size_t>(type)] = animations.string(key);
		};
		splash_animation(ProjectileType::spray, "spray");
		splash_animation(ProjectileType::jet, "jet");
		splash_animation(ProjectileType::rolling, "rolling");
		splash_animation(ProjectileType::mist, "mist");
		splash_animation(ProjectileType::ball, "ball");
		paint.splash_animation = paint.splash_animations[static_cast<size_t>(ProjectileType::spray)];

		const JsonValue visual = json.object("visual");
		paint.edge_width = read_positive(visual, "edge_width");
		paint.wet_duration = read_positive(visual, "wet_duration");
		paint.wet_highlight = read_unit(visual, "wet_highlight");
		paint.settle_depth = read_positive(visual, "settle_depth");
		paint.splash_scale = read_positive(visual, "splash_scale");
		if (!(paint.edge_width < paint.thickness))
		{
			throw std::runtime_error(visual.where() +
				": 'edge_width' must leave visible team paint inside the tile thickness.");
		}
		const JsonValue runoff = visual.object("runoff");
		paint.runoff.delay = read_unit(runoff, "delay");
		const auto positive_runoff = [&](const char* key, float high)
		{
			const float value = read_number_in(runoff, key, 0.0f, high);
			if (!(value > 0.0f))
			{
				throw std::runtime_error(runoff.where() + ": '" + key +
					"' must be greater than zero.");
			}
			return value;
		};
		paint.runoff.duration = positive_runoff("duration", 2.0f);
		paint.runoff.distance = positive_runoff("distance", 64.0f);
		paint.runoff.width = positive_runoff("width", 16.0f);
		paint.runoff.bead_height = positive_runoff("bead_height", 32.0f);
		paint.runoff.trail_length = positive_runoff("trail_length", 64.0f);
		paint.runoff.trail_width = positive_runoff("trail_width", 16.0f);
		paint.runoff.keyline_width = positive_runoff("keyline_width", 8.0f);
		paint.runoff.face_inset = read_number_in(runoff, "face_inset", 0.0f, 16.0f);
		paint.runoff.shade = read_unit(runoff, "shade");
		paint.runoff.highlight = read_unit(runoff, "highlight");
		if (!(paint.runoff.width + paint.runoff.face_inset <=
			paint.thickness - paint.edge_width))
		{
			throw std::runtime_error(runoff.where() +
				": 'width' and 'face_inset' must fit inside thickness minus edge_width.");
		}
		if (!(paint.runoff.trail_width <= paint.runoff.width))
		{
			throw std::runtime_error(runoff.where() +
				": 'trail_width' must not exceed width.");
		}
		if (!(paint.runoff.keyline_width * 2.0f < paint.runoff.width &&
			paint.runoff.keyline_width * 2.0f < paint.runoff.bead_height))
		{
			throw std::runtime_error(runoff.where() +
				": 'keyline_width' must leave visible paint inside width and bead_height.");
		}

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
		sounds.landing_min_gain = read_unit(json, "landing_min_gain");
		sounds.stride_distance = read_positive(json, "stride_distance");
		sounds.min_step_interval = read_positive(json, "min_step_interval");
		sounds.skid_min_speed = read_positive(json, "skid_min_speed");
		sounds.skid_repeat_delay = read_positive(json, "skid_repeat_delay");
		return sounds;
	}

    PaintUnderfootTuning read_paint_underfoot(const JsonValue& player)
    {
        PaintUnderfootTuning result;
        if (!player.has("paint_underfoot")) { return result; }
        const JsonValue json = player.object("paint_underfoot");
        result.enabled = json.boolean("enabled");
        const auto at_least_one = [&](const char* key)
        {
            const float value = read_finite(json, key);
            if (!(value >= 1.0f))
            { throw std::runtime_error(json.where() + ": '" + key + "' must be at least 1."); }
            return value;
        };
        result.own_speed_multiplier = at_least_one("own_speed_multiplier");
        result.own_ammo_regen_multiplier = at_least_one("own_ammo_regen_multiplier");
        result.enemy_speed_multiplier = read_unit(json, "enemy_speed_multiplier");
        if (!(result.enemy_speed_multiplier > 0.0f))
        { throw std::runtime_error(json.where() + ": 'enemy_speed_multiplier' must be greater than zero."); }
        result.enemy_ammo_regen_multiplier = read_unit(json, "enemy_ammo_regen_multiplier");
        return result;
    }

    void validate_underfoot_rates(const JsonValue& json, const PlayerTuning& player)
    {
        if (!json.has("paint_underfoot")) { return; }
        const JsonValue effect = json.object("paint_underfoot");
        const auto speed = [&](float multiplier, const char* multiplier_key)
        {
            const auto rate = [&](float base, const char* key, bool may_be_zero)
            {
                const float derived = base * multiplier;
                if (!std::isfinite(derived) || !(may_be_zero ? derived >= 0.0f : derived > 0.0f))
                { throw std::runtime_error(effect.where() + ": '" + multiplier_key +
                    "' times player." + key + " must be finite and retain a usable magnitude."); }
            };
            rate(player.x_initial_velocity, "x_initial_velocity", false);
            rate(player.x_acceleration, "x_acceleration", true);
            rate(player.x_deacceleration, "x_deacceleration", false);
            rate(player.max_velocity.x, "max_velocity.x", false);
            if (!std::isfinite(player.x_deacceleration * multiplier + player.x_acceleration * multiplier))
            { throw std::runtime_error(effect.where() + ": '" + multiplier_key + "' produces non-finite horizontal braking."); }
        };
        speed(player.paint_underfoot.own_speed_multiplier, "own_speed_multiplier");
        speed(player.paint_underfoot.enemy_speed_multiplier, "enemy_speed_multiplier");
    }
	PlayerTuning read_player(const JsonValue& root)
	{
		const JsonValue json = root.object("player");

		PlayerTuning player;
		// A zero, negative or non-finite extent or speed cap would leave the
		// per-stage anti-tunnelling bound (game/tunnelling_limits.h) meaning
		// nothing, so the tuning half of that check is refusing one at load.
		player.size = read_positive_vector(json, "size");
		player.max_velocity = read_positive_vector(json, "max_velocity");
		// The three horizontal rates are magnitudes: Player::update takes their
		// direction from the stick. A zero first step could never leave rest,
		// and braking divides by x_deacceleration (plus x_acceleration, which
		// may be zero for a constant run) and is what stops a released stick.
		player.x_initial_velocity = read_positive(json, "x_initial_velocity");
		player.x_acceleration = read_non_negative(json, "x_acceleration");
		player.x_deacceleration = read_positive(json, "x_deacceleration");
		player.paint_underfoot = read_paint_underfoot(json);
		validate_underfoot_rates(json, player);
		// A multiplier on gravity; a negative one would turn it round on a ramp.
		player.ramp_gravity_multiplier = read_non_negative(json, "ramp_gravity_multiplier");
		// The vertical rates carry a direction in their sign (y is down), so
		// only finiteness is required of them.
		player.gravity = read_finite(json, "gravity");
		player.jump_launch_velocity = read_finite(json, "jump_launch_velocity");
		player.jump_acceleration = read_finite(json, "jump_acceleration");
		// Zero leaves a jump with its launch velocity and no held boost.
		player.jump_max_time = read_non_negative(json, "jump_max_time");
		player.jump_buffer_time = read_number_in(json, "jump_buffer_time", 0.0f, 1.0f);
		player.ledge_grace_time = read_number_in(json, "ledge_grace_time", 0.0f, 1.0f);
		player.drop_down_velocity = read_finite(json, "drop_down_velocity");
		// Compared with a stick axis, which is -1..1.
		player.drop_down_analog_threshold = read_unit(json, "drop_down_analog_threshold");
		player.starting_health = read_positive(json, "starting_health");
		player.health_regen_delay = read_non_negative(json, "health_regen_delay");
		player.health_regen_rate = read_non_negative(json, "health_regen_rate");
		player.respawn_delay = read_non_negative(json, "respawn_delay");
		player.damage_sound_delay = read_non_negative(json, "damage_sound_delay");

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
		player.takeoff = read_animation(animations, "takeoff", sheet, uniform);
		player.land = read_animation(animations, "land", sheet, uniform);
		player.fire = read_animation(animations, "fire", sheet, uniform);
		player.hurt = read_animation(animations, "hurt", sheet, uniform);
		player.elimination = read_animation(animations, "elimination", sheet, uniform);

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

	WeaponTuning read_weapon(const JsonValue& weapons, const char* key, WeaponType type)
	{
		const JsonValue json = weapons.object(key);

		WeaponTuning weapon;
		WeaponDetails& details = weapon.details;
		details.offset = read_vector(json, "offset");
		details.size = read_vector(json, "size");
		details.nozzle_offset = read_vector(json, "nozzle_offset");
		// Zero fires every update; the sprayer and roller are authored so.
		details.shoot_interval = read_non_negative(json, "shoot_interval");
		details.starting_vel_length = read_non_negative(json, "starting_vel_length");
		// Not compared with weapons.starting_ammo: a tank too small for one
		// shot is a tool that reads as empty, which is a state, not a crash.
		details.ammo_usage = read_non_negative(json, "ammo_usage");
		details.sheet_name = json.string("sheet");
		details.frame_name = json.string("frame");
		details.proj_type = read_projectile_type(json, "projectile");
		details.sound_bank_name = json.string("sound_bank");
		if (type == WeaponType::roller)
		{
			weapon.aim_tilt = read_number_in(json, "aim_tilt", 0.0f, PI_OVER_2);
			if (!(weapon.aim_tilt < PI_OVER_2))
			{
				throw std::runtime_error(json.where() +
					": 'aim_tilt' must be less than pi/2 radians to keep left and right facing distinct.");
			}
		}

		// Use the shared volume validation even though WeaponDetails stores separate fields.
		const SoundTuning shoot_sound = read_sound(json, "shoot_sound");
		details.shoot_sound_name = shoot_sound.name;
		details.shoot_sound_volume = shoot_sound.volume;

		if (json.has("add_player_velocity"))
		{
			weapon.relative.add_vel =
				read_add_player_velocity(json, "add_player_velocity");
			weapon.relative.player_vel_amount =
				read_finite(json, "player_velocity_amount");
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
		weapon.feedback_sounds.low = optional_sound("low");
		return weapon;
	}

	ProjectileTuning read_projectile(const JsonValue& projectiles,
		const char* key, ProjectileType type, float wind_stop_factor)
	{
		const JsonValue json = projectiles.object(key);

		ProjectileTuning projectile;
		ProjectileDetails& details = projectile.details;
		details.gravity = read_finite(json, "gravity");
		details.wind_resistance = read_non_negative(json, "wind_resistance");
		projectile.wind_stop_factor = wind_stop_factor;
		if (!std::isfinite(details.wind_resistance * wind_stop_factor))
		{
			throw std::runtime_error(json.where() +
				": 'wind_resistance' times 'projectiles.wind_stop_factor' must produce a finite horizontal stop speed.");
		}
		if (type == ProjectileType::mist)
		{
			// The two outer pellets stay within a half-turn forward fan.
			projectile.spread_step = read_number_in(json, "spread_step", 0.0f, PI / 4.0f);
		}
		details.rotat_origin = read_rotation_origin(json, "rotation_origin");
		// A shot that is deleted the update it is made never exists.
		details.delete_timer = read_positive(json, "delete_timer");
		details.player_damage = read_non_negative(json, "player_damage");
		details.size = read_positive_vector(json, "size");
		details.col_rect_size = read_positive_vector(json, "collision_size");
		details.sheet_name = json.string("sheet");
		details.animation_strip_name = json.string("animation");

		if (json.has("diffusion"))
		{
			const JsonValue diffusion = json.object("diffusion");
			projectile.diffusion.type = read_diffusion_type(diffusion, "type");
			projectile.diffusion.start_time = read_non_negative(diffusion, "start_time");
			projectile.diffusion.end_time = read_non_negative(diffusion, "end_time");
			projectile.diffusion.end_scale = read_positive_vector(diffusion, "end_scale");
			// Equal times are a step to end_scale; a reversed pair would skip
			// the growth the block describes.
			if (!(projectile.diffusion.start_time <= projectile.diffusion.end_time))
			{
				throw std::runtime_error(diffusion.where() + ": 'start_time' is " +
					number_text(projectile.diffusion.start_time) + ", after 'end_time' (" +
					number_text(projectile.diffusion.end_time) + ").");
			}
		}

		return projectile;
	}
}

GameTuning::GameTuning(const char* path) :
	GameTuning(read_json_file(path))
{
}

GameTuning::GameTuning(const JsonDocument& document)
{
	this->source_path_ = document.source_path();
	const JsonValue root = document.root();

	this->input_ = read_input(root);
	this->match_ = read_match(root);
	this->paint_ = read_paint(root);
	this->player_ = read_player(root);
	this->rumble_ = read_rumble(root);
	this->bots_ = read_bots(root);

	const JsonValue weapons = root.object("weapons");
	this->ammo_regen_delay_ = read_non_negative(weapons, "ammo_regen_delay");
	this->ammo_regen_rate_ = read_non_negative(weapons, "ammo_regen_rate");
	this->starting_ammo_ = read_positive(weapons, "starting_ammo");
	{
		const JsonValue low = weapons.object("low_paint");
		this->low_paint_.supply_fraction = read_unit(low, "supply_fraction");
		this->low_paint_.shots = read_finite(low, "shots");
		// Two shots' paint guarantees the shot that crosses the line leaves one.
		if (!(this->low_paint_.shots >= 2.0f))
		{
			throw std::runtime_error(low.where() + ": 'shots' must be at least 2, so a warned tool can still fire.");
		}
		this->low_paint_.rearm_fraction = read_number_in(low, "rearm_fraction", 0.0f, 1.0f);
	}
    if (root.object("player").has("paint_underfoot"))
    {
        const JsonValue effect = root.object("player").object("paint_underfoot");
        const auto recovery = [&](float multiplier, const char* key)
        {
            if (!std::isfinite(this->ammo_regen_rate_ * multiplier))
            { throw std::runtime_error(effect.where() + ": '" + key + "' times weapons.ammo_regen_rate must be finite."); }
        };
        recovery(this->player_.paint_underfoot.own_ammo_regen_multiplier, "own_ammo_regen_multiplier");
        recovery(this->player_.paint_underfoot.enemy_ammo_regen_multiplier, "enemy_ammo_regen_multiplier");
    }

	const auto add_weapon = [this, &weapons](WeaponType type, const char* key)
	{
		const size_t index = static_cast<size_t>(type);
		this->weapons_[index] = read_weapon(weapons, key, type);
		this->weapon_tuned_[index] = true;
	};

	add_weapon(WeaponType::sprayer, "sprayer");
	add_weapon(WeaponType::sniper, "sniper");
	add_weapon(WeaponType::roller, "roller");
	add_weapon(WeaponType::mister, "mister");
	add_weapon(WeaponType::bucket, "bucket");

	const JsonValue projectiles = root.object("projectiles");
	// A per-axis speed cap: a zero or negative one has no meaning.
	const Vector2F max_velocity = read_positive_vector(projectiles, "max_velocity");
	const float wind_stop_factor = read_non_negative(projectiles, "wind_stop_factor");

	const auto add_projectile = [this, &projectiles, &max_velocity, wind_stop_factor](
		ProjectileType type, const char* key)
	{
		const size_t index = static_cast<size_t>(type);
		this->projectiles_[index] = read_projectile(projectiles, key, type, wind_stop_factor);
		this->projectiles_[index].max_velocity = max_velocity;
		this->projectile_tuned_[index] = true;
	};

	add_projectile(ProjectileType::spray, "spray");
	add_projectile(ProjectileType::jet, "jet");
	add_projectile(ProjectileType::rolling, "rolling");
	add_projectile(ProjectileType::mist, "mist");
	add_projectile(ProjectileType::ball, "ball");
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

const RumbleProfile& GameTuning::rumble() const
{
	return this->rumble_;
}

const BotTuning& GameTuning::bots() const
{
	return this->bots_;
}

const BotDifficultyTuning& GameTuning::bot(BotDifficulty difficulty) const
{
	if (difficulty != BotDifficulty::easy && difficulty != BotDifficulty::normal &&
		difficulty != BotDifficulty::hard)
	{
		throw std::out_of_range("Unknown bot difficulty");
	}
	return this->bots_.difficulties[static_cast<size_t>(difficulty)];
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

const LowPaintTuning& GameTuning::low_paint() const
{
	return this->low_paint_;
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
