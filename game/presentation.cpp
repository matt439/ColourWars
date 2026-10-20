#include "game/pch.h"
#include "game/presentation.h"
#include "game/presentation_read.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/objects/player_badge.h"
#include "game/resource_validation.h"
#include "game/states/ui_player_badge.h"
#include "game/text_wrap.h"
#include "engine/render/sprite_geometry.h"

#include <cmath>
#include <initializer_list>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;

namespace
{
	MovementBurstPresentation read_movement_burst(const JsonValue& parent,
		const char* key)
	{
		const JsonValue json = parent.object(key);
		return { read_positive(json, "duration"), read_positive(json, "spread"),
			read_positive(json, "lift"), read_positive(json, "particle_size") };
	}

	bool is_hex_colour(const std::string& text)
	{
		if (text.length() != 6 && text.length() != 8)
		{
			return false;
		}
		return text.find_first_not_of("0123456789abcdefABCDEF") ==
			std::string::npos;
	}
}

namespace presentation_read
{
	Colour colour_from_text(const JsonValue& where, const std::string& text);

	// Accept named colours or RRGGBB/RRGGBBAA. Validate hex strings before
	// construction so a typo cannot silently become opaque black.
	Colour read_colour(const JsonValue& parent, const char* key)
	{
		return colour_from_text(parent, parent.string(key));
	}

	Colour colour_from_text(const JsonValue& parent, const std::string& text)
	{

		const std::optional<Colour> named = colour_from_name(text);
		if (named.has_value())
		{
			return *named;
		}

		if (is_hex_colour(text))
		{
			return Colour(text);
		}

		throw std::runtime_error(parent.where() + ": '" + text +
			"' is not a colour this engine knows by name, and is not six or "
			"eight hexadecimal digits.");
	}
}

const TeamPaletteEntry& TeamPalettePresentation::fixed_pair(TeamColourPair pair) const
{
	const char* key = TeamColourTools::palette_key(pair);
	for (const TeamPaletteEntry& entry : this->pairs)
	{
		if (entry.key == key) { return entry; }
	}
	throw std::invalid_argument("Missing fixed team palette");
}

const TeamPaletteEntry& TeamPalettePresentation::random_pair(size_t draw) const
{
	if (this->pairs.empty()) { throw std::invalid_argument("No team palette pairs to draw"); }
	return this->pairs[draw % this->pairs.size()];
}

const TeamPalettePresentation& Presentation::team_palette() const
{
	return this->team_palette_;
}

const WeaponChoicePresentation& WeaponSelectArtworkPresentation::choice(WeaponType type) const
{
	const size_t index = static_cast<size_t>(type);
	if (index >= this->choices.size())
	{
		throw std::out_of_range("Unsupported weapon selection type: " +
			std::to_string(static_cast<int>(type)));
	}
	return this->choices[index];
}

Presentation::Presentation(const char* path)
{
	const JsonDocument document = read_json_file(path);
	this->source_path_ = path;
	const JsonValue root = document.root();

	this->team_palette_ = read_team_palette(root);
	this->menu_ = read_menu_style(root);
	this->menu_text_ = read_menu_text(root);
	const JsonValue music_mix = root.object("music_mix");
	const auto music_duration = [&](const char* key)
	{
		const float value = read_positive(music_mix, key);
		if (value > 5.0f)
		{
			throw std::runtime_error(music_mix.where() + ": " + key + " must be at most 5 seconds.");
		}
		return value;
	};
	this->music_mix_.fade_in = music_duration("fade_in");
	this->music_mix_.fade_out = music_duration("fade_out");
	this->music_mix_.duck_time = music_duration("duck_time");
	this->music_mix_.restore_time = music_duration("restore_time");
	this->music_mix_.pause_gain = read_unit(music_mix, "pause_gain");
	this->main_menu_ = read_main_menu(root, this->menu_.footer_y);
	this->controls_settings_ = read_controls_settings(root, this->menu_.footer_y);
	this->pause_menu_ = read_pause_menu(root);
	this->end_menu_ = read_end_menu(root);
	this->results_menu_ = read_results_menu(root);
	this->prompt_bar_ = read_prompt_bar(root);
	this->hud_ = read_hud(root);
	this->player_identity_ = read_player_identity(root);
	const JsonValue camera = root.object("camera");
	this->camera_.reference_height = read_positive(camera, "reference_height");
	const auto camera_option = [&](const char* key)
	{
		return camera.has(key) ? read_non_negative(camera, key) : 0.0f;
	};
	this->camera_.minimum_world_width = camera_option("minimum_world_width");
	this->camera_.horizontal_lead_seconds = camera_option("horizontal_lead_seconds");
	this->camera_.horizontal_lead_max_distance = camera_option("horizontal_lead_max_distance");
	const JsonValue shake = camera.object("shake");
	ViewShakePresentation& kick = this->camera_.shake;
	kick.hurt_amplitude = read_positive(shake, "hurt_amplitude");
	kick.elimination_amplitude = read_positive(shake, "elimination_amplitude");
	kick.landing_amplitude = read_positive(shake, "landing_amplitude");
	kick.landing_min_speed = read_positive(shake, "landing_min_speed");
	kick.landing_full_speed = read_positive(shake, "landing_full_speed");
	kick.duration = read_positive(shake, "duration");
	kick.cycles = read_positive(shake, "cycles");
	kick.repeat_delay = read_positive(shake, "repeat_delay");
	if (kick.hurt_amplitude > 8.0f || kick.elimination_amplitude > 8.0f ||
		kick.landing_amplitude > 8.0f || kick.duration > 0.5f || kick.cycles > 3.0f ||
		kick.repeat_delay < kick.duration || kick.repeat_delay > 2.0f ||
		kick.landing_full_speed <= kick.landing_min_speed)
	{
		throw std::runtime_error(shake.where() + ": shake needs amplitudes <= 8 world units, "
			"duration <= 0.5 seconds, cycles <= 3, duration <= repeat_delay <= 2 seconds "
			"and landing_full_speed > landing_min_speed.");
	}
	const JsonValue movement = root.object("movement_feedback");
	MovementFeedbackPresentation& feedback = this->movement_feedback_;
	feedback.enabled = movement.boolean("enabled");
	feedback.sprite_sheet = movement.string("sprite_sheet");
	feedback.fill_frame = movement.string("fill_frame");
	feedback.colour = read_colour(movement, "colour");
	feedback.highlight = read_colour(movement, "highlight");
	feedback.pixel_size = read_positive(movement, "pixel_size");
	feedback.landing_min_speed = read_positive(movement, "landing_min_speed");
	feedback.full_impact_speed = read_positive(movement, "full_impact_speed");
	feedback.minimum_strength = read_positive(movement, "minimum_strength");
	feedback.takeoff = read_movement_burst(movement, "takeoff");
	feedback.landing = read_movement_burst(movement, "landing");
	feedback.step = read_movement_burst(movement, "step");
	feedback.skid = read_movement_burst(movement, "skid");
	feedback.keyline = read_colour(movement, "keyline");
	feedback.keyline_width = read_positive(movement, "keyline_width");
	if (feedback.minimum_strength > 1.0f ||
		feedback.full_impact_speed < feedback.landing_min_speed ||
		feedback.takeoff.particle_size < feedback.pixel_size ||
		feedback.landing.particle_size < feedback.pixel_size ||
		feedback.step.particle_size < feedback.pixel_size ||
		feedback.skid.particle_size < feedback.pixel_size ||
		feedback.keyline_width * 2.0f >=
			std::min(feedback.step.particle_size, feedback.skid.particle_size))
	{
		throw std::runtime_error(movement.where() +
			": strength must be at most one, full impact speed must cover minimum speed, "
			"particles must be at least one presentation pixel, and contact keylines "
			"must leave a visible particle interior.");
	}
	const JsonValue combat = root.object("combat_feedback");
	CombatFeedbackPresentation& contact = this->combat_feedback_;
	contact.enabled = combat.boolean("enabled");
	contact.sprite_sheet = combat.string("sprite_sheet");
	contact.fill_frame = combat.string("fill_frame");
	contact.highlight = read_colour(combat, "highlight");
	contact.keyline = read_colour(combat, "keyline");
	contact.hurt_duration = read_positive(combat, "hurt_duration");
	contact.hurt_repeat_delay = read_positive(combat, "hurt_repeat_delay");
	contact.hurt_width = read_positive(combat, "hurt_width");
	contact.hurt_reach = read_positive(combat, "hurt_reach");
	contact.elimination_duration = read_positive(combat, "elimination_duration");
	contact.elimination_spread = read_positive(combat, "elimination_spread");
	contact.elimination_lift = read_positive(combat, "elimination_lift");
	contact.particle_size = read_positive(combat, "particle_size");
	contact.keyline_width = read_positive(combat, "keyline_width");
	if (contact.hurt_repeat_delay < contact.hurt_duration ||
		contact.keyline_width * 2.0f >= std::min(contact.hurt_width, contact.particle_size))
	{
		throw std::runtime_error(combat.where() +
			": repeat delay must cover the hurt duration and keylines must leave visible fill.");
	}
	this->weapon_feedback_ = read_weapon_feedback(root);
	this->blocked_shot_ = read_blocked_shot(root);
	this->idle_drips_ = read_idle_drips(root);
	this->figure_ = read_figure(root);
	this->tool_motion_ = read_tool_motion(root);
	this->respawn_redraw_ = read_respawn_redraw(root);
	if (this->respawn_redraw_.canvas_size != this->figure_.canvas_size)
	{
		// The pen draws over the same overhanging canvas as the figure.
		throw std::runtime_error("respawn_redraw.canvas_size must equal figure.canvas_size.");
	}
	this->opening_ = read_opening(root);
	const JsonValue match = root.object("match");
	this->match_.reference_size = read_positive_vector(match, "reference_size");
	this->match_.countdown_size = read_positive_vector(match, "countdown_size");
	this->match_.countdown_center_y = read_positive(match, "countdown_center_y");
	if (this->match_.countdown_center_y >= 1.0f)
	{
		throw std::runtime_error(match.where() + ": countdown_center_y must be less than one.");
	}
	this->match_.controls_margin = read_positive_vector(match, "controls_margin");
	this->match_.controls_padding = read_positive_vector(match, "controls_padding");
	this->match_.controls_gap = read_positive(match, "controls_gap");
	this->match_.stage_entry_size = read_positive_vector(match, "stage_entry_size");
	this->match_.stage_entry_top = read_positive(match, "stage_entry_top");
	const JsonValue countdown_art = match.object("countdown_art");
	CountdownArtPresentation& signal = this->match_.countdown_art;
	signal.ink = read_colour(countdown_art, "ink");
	signal.paper = read_colour(countdown_art, "paper");
	signal.shade = read_colour(countdown_art, "shade");
	signal.number = read_colour(countdown_art, "number");
	signal.go = read_colour(countdown_art, "go");
	signal.keyline_width = read_positive(countdown_art, "keyline_width");
	signal.shadow_offset = countdown_art.number("shadow_offset");
	signal.beat_duration = read_positive(countdown_art, "beat_duration");
	signal.pulse = read_unit(countdown_art, "pulse");
	if (signal.keyline_width > 8.0f || !std::isfinite(signal.shadow_offset) ||
		signal.shadow_offset < 0.0f || signal.shadow_offset > 16.0f ||
		signal.beat_duration > 0.5f || signal.pulse > 0.15f)
	{
		throw std::runtime_error(countdown_art.where() +
			": keyline_width <= 8, shadow_offset <= 16, beat_duration <= 0.5 and pulse <= 0.15 required.");
	}
	const MatchPresentation& intro = this->match_;
	if (intro.stage_entry_size.x <= intro.controls_padding.x * 2.0f + signal.shadow_offset ||
		intro.stage_entry_size.y <= intro.controls_padding.y * 2.0f + signal.shadow_offset ||
		intro.stage_entry_size.x <= signal.keyline_width * 6.0f + signal.shadow_offset ||
		intro.stage_entry_size.y <= signal.keyline_width * 6.0f + signal.shadow_offset ||
		intro.stage_entry_size.x + intro.controls_margin.x * 2.0f > intro.reference_size.x ||
		intro.stage_entry_top + intro.stage_entry_size.y + intro.controls_gap >
			intro.reference_size.y * intro.countdown_center_y - intro.countdown_size.y / 2.0f)
	{
		throw std::runtime_error(match.where() +
			": stage_entry_size and stage_entry_top must fit above the countdown with room inside their padding.");
	}
	if (intro.countdown_size.x <= intro.controls_padding.x * 2.0f ||
		intro.countdown_size.y <= intro.controls_padding.y * 3.0f ||
		intro.countdown_size.x > intro.reference_size.x ||
		intro.countdown_size.y / 2.0f > intro.reference_size.y *
			std::min(intro.countdown_center_y, 1.0f - intro.countdown_center_y) ||
		intro.controls_margin.x * 2.0f + intro.controls_gap +
			intro.controls_padding.x * 4.0f >= intro.reference_size.x ||
		intro.controls_margin.y * 2.0f + intro.controls_padding.y * 3.0f >=
			intro.reference_size.y)
	{
		throw std::runtime_error(match.where() +
			": countdown and controls cards must fit the reference size with room inside their padding.");
	}
	this->debug_overlay_ = read_debug_overlay(root);
}

const MatchPresentation& Presentation::match() const
{
	return this->match_;
}

const CameraPresentation& Presentation::camera() const
{
	return this->camera_;
}

const MovementFeedbackPresentation& Presentation::movement_feedback() const
{
	return this->movement_feedback_;
}

const CombatFeedbackPresentation& Presentation::combat_feedback() const
{
	return this->combat_feedback_;
}

const WeaponFeedbackPresentation& Presentation::weapon_feedback() const
{
	return this->weapon_feedback_;
}

const BlockedShotPresentation& Presentation::blocked_shot() const
{
	return this->blocked_shot_;
}

const IdleDripPresentation& Presentation::idle_drip(WeaponType type) const
{
	return this->idle_drips_.at(static_cast<size_t>(type));
}

const RespawnRedrawPresentation& Presentation::respawn_redraw() const
{
	return this->respawn_redraw_;
}

const FigurePresentation& Presentation::figure() const
{
	return this->figure_;
}

const ToolMotionPresentation& Presentation::tool_motion() const
{
	return this->tool_motion_;
}

const OpeningPresentation& Presentation::opening() const
{
	return this->opening_;
}

const MenuStyle& Presentation::menu() const
{
	return this->menu_;
}

const MenuText& Presentation::menu_text() const
{
	return this->menu_text_;
}

const MusicMixPresentation& Presentation::music_mix() const
{
	return this->music_mix_;
}

const MainMenuPresentation& Presentation::main_menu() const
{
	return this->main_menu_;
}

const ControlsSettingsPresentation& Presentation::controls_settings() const
{
	return this->controls_settings_;
}

const PauseMenuPresentation& Presentation::pause_menu() const
{
	return this->pause_menu_;
}

const EndMenuPresentation& Presentation::end_menu() const
{
	return this->end_menu_;
}

const ResultsMenuPresentation& Presentation::results_menu() const
{
	return this->results_menu_;
}

const PromptBarPresentation& Presentation::prompt_bar() const
{
	return this->prompt_bar_;
}

const PlayerIdentityPresentation& Presentation::player_identity() const
{
	return this->player_identity_;
}

const HudPresentation& Presentation::hud() const
{
	return this->hud_;
}

const DebugOverlayPresentation& Presentation::debug_overlay() const
{
	return this->debug_overlay_;
}

void Presentation::set_movement_feedback_enabled(bool enabled)
{
	this->movement_feedback_.enabled = enabled;
}
