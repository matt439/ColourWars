#include "game/pch.h"
#include "game/config/presentation/presentation_validation.h"
#include "game/config/drawable_text.h"
#include "game/objects/primitive_frame.h"
#include "game/objects/interface_gameplay.h"
#include "game/input/input_slots.h"
#include "game/save/player_profiles.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

using namespace labrador;
using namespace presentation_read;
using namespace presentation_validation;

FontHandle Presentation::Validation::validate_artwork_resources()
{
	check.font("hud.meter_label_font", this->presentation.hud_.meter_label_font);
	check.font("hud.player_identity.font", this->presentation.hud_.player_identity_font);
	check.font("match.controls_text.font", this->presentation.match_.controls_text.font);
	check.frame("hud.fill_frame", this->presentation.hud_.sprite_sheet, this->presentation.hud_.fill_frame);
	check.frame("movement_feedback.fill_frame", this->presentation.movement_feedback_.sprite_sheet,
		this->presentation.movement_feedback_.fill_frame);
	check.frame("combat_feedback.fill_frame", this->presentation.combat_feedback_.sprite_sheet,
		this->presentation.combat_feedback_.fill_frame);
	check.frame("weapon_feedback.fill_frame", this->presentation.weapon_feedback_.sprite_sheet,
		this->presentation.weapon_feedback_.fill_frame);
	check.frame("blocked_shot.fill_frame", this->presentation.blocked_shot_.sprite_sheet,
		this->presentation.blocked_shot_.fill_frame);
	check.frame("idle_drip.fill_frame", this->presentation.idle_drip().sprite_sheet,
		this->presentation.idle_drip().fill_frame);
	check.frame("figure.rest_arms_frame", this->presentation.figure_.sprite_sheet, this->presentation.figure_.rest_arms_frame);
	check.frame("figure.rest_arms_paint_frame", this->presentation.figure_.sprite_sheet,
		this->presentation.figure_.rest_arms_paint_frame);
	check.frame("figure.sleeves_frame", this->presentation.figure_.sprite_sheet, this->presentation.figure_.sleeves_frame);
	check.frame("figure.sleeves_paint_frame", this->presentation.figure_.sprite_sheet,
		this->presentation.figure_.sleeves_paint_frame);
	check.frame("tool_motion.mist_tail.fill_frame", this->presentation.tool_motion_.mist_sheet,
		this->presentation.tool_motion_.mist_frame);
	check.frame("respawn_redraw.pen_frame", this->presentation.respawn_redraw_.sprite_sheet,
		this->presentation.respawn_redraw_.pen_frame);
	check.frame("opening.fill_frame", this->presentation.opening_.sprite_sheet,
		this->presentation.opening_.fill_frame);
	check.sheet("opening.palette_sheet", this->presentation.opening_.palette_sheet);
	const SpriteSheet* palette_sheet = render.sprite_sheet(this->presentation.opening_.palette_sheet);
	for (size_t index = 0; index < this->presentation.opening_.palette_frames.size(); ++index)
	{
		const std::string key = "opening.palette_frames[" + std::to_string(index) + "]";
		const std::string& name = this->presentation.opening_.palette_frames[index];
		check.frame(key, this->presentation.opening_.palette_sheet, name);
		const SpriteFrame& swatch = palette_sheet->sprite_frame(palette_sheet->resolve_sprite_frame(name));
		const RectangleI& source = swatch.source_rectangle();
		if (source.width <= 0 || source.height <= 0 || source.width != source.height ||
			swatch.origin() != Vector2F::zero)
		{
			throw std::runtime_error(this->presentation.source_path_ + ": " + key +
				" must name a positive square swatch with zero authored origin.");
		}
	}
	check.font("opening.percent_font", this->presentation.opening_.percent_font);
	// CountdownCards scales each heading to the card's width, never above the
	// GET READY heading's scale, which also places the number box. RESUMING
	// shares the card; the replay card takes GET READY's scale and grows with
	// its own heading, so only its glyphs are checked here.
	const MenuCountdownText& countdown = this->presentation.menu_text_.countdown;
	const Vector2F& shadow = this->presentation.menu_.detail_shadow_offset;
	const float heading_room = this->presentation.match_.countdown_size.x -
		this->presentation.match_.controls_padding.x * 2.0f;
	float heading_scale = 1.0f;
	for (const auto& [key, text] : { std::pair<const char*, const std::wstring*>{ "get_ready", &countdown.get_ready },
		{ "resuming", &countdown.resuming }, { "replay", &countdown.replay } })
	{
		const std::string where = std::string("menu_text.countdown.") + key;
		const Vector2F heading_size = fixed_menu_size(where, *text, this->presentation.menu_.detail_font);
		const float fitted = std::min(heading_scale, heading_room / (heading_size.x + std::abs(shadow.x)));
		if (text == &countdown.get_ready) { heading_scale = fitted; }
		if (text == &countdown.replay) { continue; }
		if ((heading_size.y + std::abs(shadow.y)) * fitted +
			this->presentation.match_.controls_padding.y * 3.0f >= this->presentation.match_.countdown_size.y)
		{
			// Drawable text is printable ASCII, so each character narrows as is.
			std::string heading;
			for (const wchar_t character : *text) { heading += static_cast<char>(character); }
			throw std::runtime_error(this->presentation.source_path_ +
				": match.countdown_size must leave room below the " + heading + " heading (" + where + ").");
		}
	}

	// The controls reminders' headings. Each card is as wide as its widest
	// line and the cards scale together to fit the window (CountdownCards::
	// centre_controls_text), so only the glyphs and bounds are checked here;
	// the reminders themselves are measured in match.controls_text.font by
	// validate_controls_reminders (game/config/content_validation.h).
	const ControlsCopy& controls = this->presentation.menu_text_.controls;
	std::ignore = fixed_menu_size("menu_text.controls.keyboard_heading", controls.keyboard_heading,
		this->presentation.menu_.detail_font);
	std::ignore = fixed_menu_size("menu_text.controls.gamepad_heading", controls.gamepad_heading,
		this->presentation.menu_.detail_font);

	// The HUD's copy: the team labels in the identity box, which grows with
	// its text, and the meter labels, which must fit above their meter at
	// any pane size (the label scales by at most width / reference_width).
	const HudPresentation& hud = this->presentation.hud_;
	std::ignore = fixed_menu_size("menu_text.hud.team_a", hud.labels.team_a, hud.player_identity_font);
	std::ignore = fixed_menu_size("menu_text.hud.team_b", hud.labels.team_b, hud.player_identity_font);
	for (const auto& [key, text] : { std::pair<const char*, const std::wstring*>{ "health", &hud.labels.health },
		{ "paint", &hud.labels.paint } })
	{
		const std::string where = std::string("menu_text.hud.") + key;
		if (fixed_menu_size(where, *text, hud.meter_label_font).x >
			hud.box_size.x * hud.player_identity_reference_width)
		{
			throw std::runtime_error(this->presentation.source_path_ + ": " + where +
				" must fit above its meter (hud.box_size.x of hud.player_identity.reference_width).");
		}
	}

	for (const SoundReference& sound : this->presentation.sound_references())
	{
		check.sound(sound);
	}

	// Non-menu primitive drawing and developer overlays still use this texel.
	check.frame("built-in primitive frame 'pixel'", primitive_frame::sheet, primitive_frame::pixel);
	const WeaponSelectArtworkPresentation& artwork = this->presentation.main_menu_.weapon_select_art;
	check.frame("main_menu.weapon_select.fill_frame", artwork.sheet, artwork.fill_frame);
	const SpriteSheet* weapon_sheet = render.sprite_sheet(artwork.sheet);
	const float name_width = this->presentation.main_menu_.weapon_description_x_offset;
	if (!std::isfinite(name_width) || name_width <= 0.0f)
	{
		throw std::runtime_error(this->presentation.source_path_ +
			": main_menu.weapon_description.x_offset must be finite and positive for the weapon name column.");
	}
	const FontHandle name_font = render.resolve_sprite_font(this->presentation.main_menu_.weapon_description_font);
	for (size_t index = 0; index < artwork.choices.size(); ++index)
	{
		const WeaponChoicePresentation& choice = artwork.choices[index];
		const std::string key = std::string("main_menu.weapon_select.choices.") + weapon_names[index];
		drawable_text::check_font_can_draw(&render, this->presentation.main_menu_.weapon_description_font,
			choice.name, this->presentation.source_path_ + ": " + key + ".name");
		const Vector2F name_size = render.measure_text(name_font, choice.name);
		if (!std::isfinite(name_size.x) ||
			name_size.x + std::abs(this->presentation.menu_.detail_shadow_offset.x) > name_width)
		{
			throw std::runtime_error(this->presentation.source_path_ + ": " + key +
				".name must fit the name column before main_menu.weapon_description.x_offset.");
		}
		check.frame(key + ".frame", artwork.sheet, choice.frame);
		const SpriteFrame& body = weapon_sheet->sprite_frame(weapon_sheet->resolve_sprite_frame(choice.frame));
		if (body.source_rectangle().width <= 0 || body.source_rectangle().height <= 0)
		{
			throw std::runtime_error(this->presentation.source_path_ + ": " + key +
				".frame must have positive source dimensions.");
		}
		if (!choice.paint_frame.empty())
		{
			check.frame(key + ".paint_frame", artwork.sheet, choice.paint_frame);
			const SpriteFrame& paint = weapon_sheet->sprite_frame(
				weapon_sheet->resolve_sprite_frame(choice.paint_frame));
			if (body.source_rectangle().size() != paint.source_rectangle().size() || body.origin() != paint.origin())
			{
				throw std::runtime_error(this->presentation.source_path_ + ": " + key +
					".paint_frame must match its body dimensions and origin.");
			}
		}
		if (!choice.supply_strip.empty())
		{
			check.animation(key + ".supply_strip", artwork.sheet, choice.supply_strip);
			const AnimationStrip& supply = weapon_sheet->animation_strip(
				weapon_sheet->resolve_animation_strip(choice.supply_strip));
			if (supply.frame_count() <= 0 || body.origin() != Vector2F::zero)
			{
				throw std::runtime_error(this->presentation.source_path_ + ": " + key +
					".supply_strip needs at least one frame and a body with zero authored origin.");
			}
			for (int frame = 0; frame < supply.frame_count(); ++frame)
			{
				if (supply.frame_rect(frame).size() != body.source_rectangle().size())
				{
					throw std::runtime_error(this->presentation.source_path_ + ": " + key +
						".supply_strip must retain its body dimensions for every frame.");
				}
			}
		}
	}

	return name_font;
}

// Last phase: the HUD identity box with a worn profile's name (REV#35). After
// every other check, so a label's own diagnostics keep their precedence.
void Presentation::Validation::validate_hud_identity() const
{
	const HudPresentation& hud = this->presentation.hud_;
	// The identity box grows with its text: the badge line, the device and,
	// for a pad wearing a profile, its name, checked with the widest name.
	// The box must end before the clock's paper, measured on the widest
	// clock "00:00" with the closing clock's padding and ticks. The box's x
	// is a fraction of the pane and its size is at most pane width /
	// reference_width, so a full-width pane at that scale is the closest.
	const FontHandle identity_font = render.resolve_sprite_font(hud.player_identity_font);
	const std::wstring widest_name(player_profiles::max_name_length, L'W');
	float text_width = 0.0f;
	for (int player = 0; player < max_players; ++player)
	{
		for (const std::wstring* team : { &hud.labels.team_a, &hud.labels.team_b })
		{
			for (int slot = 0; slot < input_slots::count; ++slot)
			{
				const std::string key = std::string("menu_text.labels.") +
					(input_slots::is_pad(slot) ? "pad" : "keyboard_mouse") +
					" with menu_text.hud at the HUD identity box";
				const std::wstring identity = InterfaceGameplay::identity_text(player, *team,
					this->presentation.menu_text_.device_label(slot),
					input_slots::is_pad(slot) ? widest_name : std::wstring());
				drawable_text::check_font_can_draw(&render, hud.player_identity_font, identity,
					this->presentation.source_path_ + ": " + key);
				text_width = std::max(text_width, render.measure_text(identity_font, identity).x);
			}
		}
	}
	const PlayerIdentityPresentation& badge = this->presentation.player_identity_;
	const float width = hud.player_identity_reference_width;
	const float box_right = width * hud.player_identity_offset.x + hud.player_identity_padding.x + text_width +
		(badge.enabled ? badge.ui_size.x + badge.ui_gap : 0.0f);
	const float clock_width = render.measure_text(render.resolve_sprite_font(hud.timer_font), L"00:00").x *
		width / hud.timer_reference_width;
	const float clock_left = (width - clock_width) / 2.0f - width * hud.timer_offset.x -
		std::max(hud.player_identity_padding.x, hud.closing_clock.keyline_width * 3.0f) -
		hud.closing_clock.tick_reach;
	if (!std::isfinite(box_right) || !std::isfinite(clock_left) || box_right > clock_left)
	{
		throw std::runtime_error(this->presentation.source_path_ +
			": menu_text.labels with menu_text.hud at the HUD identity box must end before the clock's paper with a " +
			std::to_string(player_profiles::max_name_length) +
			"-character profile name (hud.player_identity.offset, padding and font).");
	}
}
