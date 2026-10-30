#include "game/pch.h"
#include "game/presentation_validation.h"
#include "game/build_info.h"
#include "game/content_value.h"

#include "engine/assets/json.h"
#include "engine/render/text_encoding.h"
#include "engine/render/resolution_manager.h"
#include "game/drawable_text.h"
#include "game/input_slots.h"
#include "game/player_profiles.h"
#include "game/objects/player_badge.h"
#include "game/resource_validation.h"
#include "game/controls_text.h"
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
using namespace presentation_validation;

FontHandle Presentation::Validation::validate_artwork_resources()
{
	check.font("hud.meter_label_font", this->presentation_.hud_.meter_label_font);
	check.font("hud.player_identity.font", this->presentation_.hud_.player_identity_font);
	check.frame("hud.fill_frame", this->presentation_.hud_.sprite_sheet, this->presentation_.hud_.fill_frame);
	check.frame("movement_feedback.fill_frame", this->presentation_.movement_feedback_.sprite_sheet,
		this->presentation_.movement_feedback_.fill_frame);
	check.frame("combat_feedback.fill_frame", this->presentation_.combat_feedback_.sprite_sheet,
		this->presentation_.combat_feedback_.fill_frame);
	check.frame("weapon_feedback.fill_frame", this->presentation_.weapon_feedback_.sprite_sheet,
		this->presentation_.weapon_feedback_.fill_frame);
	check.frame("blocked_shot.fill_frame", this->presentation_.blocked_shot_.sprite_sheet,
		this->presentation_.blocked_shot_.fill_frame);
	check.frame("idle_drip.fill_frame", this->presentation_.idle_drip().sprite_sheet,
		this->presentation_.idle_drip().fill_frame);
	check.frame("figure.rest_arms_frame", this->presentation_.figure_.sprite_sheet, this->presentation_.figure_.rest_arms_frame);
	check.frame("figure.rest_arms_paint_frame", this->presentation_.figure_.sprite_sheet,
		this->presentation_.figure_.rest_arms_paint_frame);
	check.frame("figure.sleeves_frame", this->presentation_.figure_.sprite_sheet, this->presentation_.figure_.sleeves_frame);
	check.frame("figure.sleeves_paint_frame", this->presentation_.figure_.sprite_sheet,
		this->presentation_.figure_.sleeves_paint_frame);
	check.frame("tool_motion.mist_tail.fill_frame", this->presentation_.tool_motion_.mist_sheet,
		this->presentation_.tool_motion_.mist_frame);
	check.frame("respawn_redraw.pen_frame", this->presentation_.respawn_redraw_.sprite_sheet,
		this->presentation_.respawn_redraw_.pen_frame);
	check.frame("opening.fill_frame", this->presentation_.opening_.sprite_sheet,
		this->presentation_.opening_.fill_frame);
	check.sheet("opening.palette_sheet", this->presentation_.opening_.palette_sheet);
	const SpriteSheet* palette_sheet = render.sprite_sheet(this->presentation_.opening_.palette_sheet);
	for (size_t index = 0; index < this->presentation_.opening_.palette_frames.size(); ++index)
	{
		const std::string key = "opening.palette_frames[" + std::to_string(index) + "]";
		const std::string& name = this->presentation_.opening_.palette_frames[index];
		check.frame(key, this->presentation_.opening_.palette_sheet, name);
		const SpriteFrame& swatch = palette_sheet->sprite_frame(palette_sheet->resolve_sprite_frame(name));
		const RectangleI& source = swatch.source_rectangle();
		if (source.width <= 0 || source.height <= 0 || source.width != source.height ||
			swatch.origin() != Vector2F::ZERO)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				" must name a positive square swatch with zero authored origin.");
		}
	}
	check.font("opening.percent_font", this->presentation_.opening_.percent_font);
	const Vector2F heading_size = render.measure_text(
		render.resolve_sprite_font(this->presentation_.menu_.detail_font), L"GET READY");
	const float heading_scale = std::min(1.0f,
		(this->presentation_.match_.countdown_size.x - this->presentation_.match_.controls_padding.x * 2.0f) /
			(heading_size.x + std::abs(this->presentation_.menu_.detail_shadow_offset.x)));
	if ((heading_size.y + std::abs(this->presentation_.menu_.detail_shadow_offset.y)) * heading_scale +
		this->presentation_.match_.controls_padding.y * 3.0f >= this->presentation_.match_.countdown_size.y)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": match.countdown_size must leave room below the GET READY heading.");
	}

	for (const SoundReference& sound : this->presentation_.sound_references())
	{
		check.sound(sound);
	}

	// Non-menu primitive drawing and developer overlays still use this texel.
	check.frame("built-in primitive frame 'pixel'", "sprite_sheet_1", "pixel");
	const WeaponSelectArtworkPresentation& artwork = this->presentation_.main_menu_.weapon_select_art;
	check.frame("main_menu.weapon_select.fill_frame", artwork.sheet, artwork.fill_frame);
	const SpriteSheet* weapon_sheet = render.sprite_sheet(artwork.sheet);
	const float name_width = this->presentation_.main_menu_.weapon_description_x_offset;
	if (!std::isfinite(name_width) || name_width <= 0.0f)
	{
		throw std::runtime_error(this->presentation_.source_path_ +
			": main_menu.weapon_description.x_offset must be finite and positive for the weapon name column.");
	}
	const FontHandle name_font = render.resolve_sprite_font(this->presentation_.main_menu_.weapon_description_font);
	for (size_t index = 0; index < artwork.choices.size(); ++index)
	{
		const WeaponChoicePresentation& choice = artwork.choices[index];
		const std::string key = std::string("main_menu.weapon_select.choices.") + WEAPON_NAMES[index];
		drawable_text::check_font_can_draw(&render, this->presentation_.main_menu_.weapon_description_font,
			choice.name, this->presentation_.source_path_ + ": " + key + ".name");
		const Vector2F name_size = render.measure_text(name_font, choice.name);
		if (!std::isfinite(name_size.x) ||
			name_size.x + std::abs(this->presentation_.menu_.detail_shadow_offset.x) > name_width)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				".name must fit the name column before main_menu.weapon_description.x_offset.");
		}
		check.frame(key + ".frame", artwork.sheet, choice.frame);
		const SpriteFrame& body = weapon_sheet->sprite_frame(weapon_sheet->resolve_sprite_frame(choice.frame));
		if (body.source_rectangle().width <= 0 || body.source_rectangle().height <= 0)
		{
			throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
				".frame must have positive source dimensions.");
		}
		if (!choice.paint_frame.empty())
		{
			check.frame(key + ".paint_frame", artwork.sheet, choice.paint_frame);
			const SpriteFrame& paint = weapon_sheet->sprite_frame(
				weapon_sheet->resolve_sprite_frame(choice.paint_frame));
			if (body.source_rectangle().size() != paint.source_rectangle().size() || body.origin() != paint.origin())
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
					".paint_frame must match its body dimensions and origin.");
			}
		}
		if (!choice.supply_strip.empty())
		{
			check.animation(key + ".supply_strip", artwork.sheet, choice.supply_strip);
			const AnimationStrip& supply = weapon_sheet->animation_strip(
				weapon_sheet->resolve_animation_strip(choice.supply_strip));
			if (supply.frame_count() <= 0 || body.origin() != Vector2F::ZERO)
			{
				throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
					".supply_strip needs at least one frame and a body with zero authored origin.");
			}
			for (int frame = 0; frame < supply.frame_count(); ++frame)
			{
				if (supply.frame_rect(frame).size() != body.source_rectangle().size())
				{
					throw std::runtime_error(this->presentation_.source_path_ + ": " + key +
						".supply_strip must retain its body dimensions for every frame.");
				}
			}
		}
	}

	return name_font;
}
