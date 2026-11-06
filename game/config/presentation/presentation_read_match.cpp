#include "game/pch.h"
#include "game/config/presentation/presentation_read.h"
#include "game/config/content_value.h"

#include "engine/assets/json.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Readers for the in-match, visual and audio sections of presentation.json:
// HUD, identity, feedback, figure, respawn, opening, camera, music mix, the
// match's countdown and controls cards and the debug overlay.

using namespace labrador;
using namespace content_value;
using namespace presentation_read;

namespace
{
	// A position in a list, which the caller checks against the list.
	size_t read_index(const JsonValue& parent, const char* key)
	{
		const int value = parent.integer(key);
		if (value < 0)
		{
			throw std::runtime_error(parent.where() + ": '" + key + "' must not be negative.");
		}
		return static_cast<size_t>(value);
	}

	// A pen path: two or more points, each step horizontal or vertical. With
	// a last nib position, every point keeps the pen's whole nib on the canvas
	// from the origin to it; a slip's edit has none, because where its nib may
	// go is the respawn canvas's to say, so Presentation checks it.
	std::vector<Vector2F> read_pen_points(const JsonValue& parent,
		const std::optional<Vector2F>& last_nib = std::nullopt)
	{
		const JsonValue points = parent.array("points");
		if (points.size() < 2)
		{
			throw std::runtime_error(points.where() + ": a stroke needs at least two points.");
		}
		std::vector<Vector2F> path;
		for (size_t at = 0; at < points.size(); ++at)
		{
			const JsonValue point_json = points.at(at);
			const Vector2F point(point_json.number("x"), point_json.number("y"));
			if (last_nib.has_value() && !(point.x >= 0.0f && point.y >= 0.0f &&
				point.x <= last_nib->x && point.y <= last_nib->y))
			{
				throw std::runtime_error(point_json.where() +
					": the pen leaves the canvas at this point.");
			}
			if (!path.empty())
			{
				const Vector2F step = point - path.back();
				if ((step.x == 0.0f) == (step.y == 0.0f))
				{
					throw std::runtime_error(point_json.where() + ": the pen draws only "
						"horizontal and vertical lines, and never stands still.");
				}
			}
			path.push_back(point);
		}
		return path;
	}

	// A gameplay plaque (gate permission, ferry direction): a glyph keyline
	// that leaves room inside the plaque, and ink and paper that read apart
	// in monochrome.
	void check_plaque(const JsonValue& json, const Colour& ink, const Colour& paper,
		const Vector2F& size, float keyline_width)
	{
		const auto brightness = [](const Colour& colour)
		{
			return 0.2126f * colour.r + 0.7152f * colour.g + 0.0722f * colour.b;
		};
		if (keyline_width * 4.0f >= std::min(size.x, size.y) ||
			ink.a != 1.0f || paper.a != 1.0f ||
			std::abs(brightness(ink) - brightness(paper)) < 0.5f)
		{
			throw std::runtime_error(json.where() +
				": keyline_width must leave glyph room within size; ink and paper must be opaque "
				"with at least 0.5 monochrome brightness separation.");
		}
	}

	FigureSlipsPresentation read_figure_slips(const JsonValue& json, const Vector2F& canvas_size)
	{
		FigureSlipsPresentation slips;
		slips.band_frame = json.identifier("band_frame");
		const JsonValue band = json.object("band");
		slips.band = RectangleF(band.number("x"), band.number("y"),
			read_positive(band, "width"), read_positive(band, "height"));
		// A facing-left figure mirrors its canvas, so the band must cover the
		// same texels either way for one band rectangle to serve both.
		if (!(slips.band.x >= 0.0f && slips.band.y >= 0.0f &&
			slips.band.right() <= canvas_size.x && slips.band.bottom() <= canvas_size.y &&
			slips.band.x + slips.band.right() == canvas_size.x))
		{
			throw std::runtime_error(band.where() +
				": the band must sit in the canvas, centred across its width.");
		}
		slips.two_slip_chance = read_unit(json, "two_slip_chance");
		const JsonValue list = json.array("list");
		for (size_t index = 0; index < list.size(); ++index)
		{
			const JsonValue slip_json = list.at(index);
			FigureSlip slip;
			slip.id = slip_json.identifier("id");
			if (slip.id.empty() || slip.id.find_first_of("+.") != std::string::npos)
			{
				throw std::runtime_error(slip_json.where() +
					": a slip id names its band frames, so it needs no '+' or '.'.");
			}
			for (const FigureSlip& earlier : slips.list)
			{
				if (earlier.id == slip.id)
				{
					throw std::runtime_error(slip_json.where() + ": slip '" + slip.id + "' is listed twice.");
				}
			}
			const JsonValue edits = slip_json.array("strokes");
			for (size_t at = 0; at < edits.size(); ++at)
			{
				const JsonValue edit_json = edits.at(at);
				FigureSlipStroke edit;
				// after and remove choose the edit, so a misspelt one must fail.
				if (edit_json.has("after"))
				{
					require_known_keys(edit_json, { "after", "part", "points" });
					edit.edit = FigureSlipStroke::Edit::insert_after;
					edit.stroke = read_index(edit_json, "after");
					edit.part = edit_json.string("part");
					edit.points = read_pen_points(edit_json);
				}
				else
				{
					require_known_keys(edit_json, { "stroke", "remove", "points" });
					edit.stroke = read_index(edit_json, "stroke");
					if (edit_json.has("remove") && edit_json.boolean("remove"))
					{
						edit.edit = FigureSlipStroke::Edit::remove;
					}
					else
					{
						edit.points = read_pen_points(edit_json);
					}
				}
				slip.strokes.push_back(std::move(edit));
			}
			slips.list.push_back(std::move(slip));
		}
		const JsonValue exclusive = json.array("exclusive");
		for (size_t index = 0; index < exclusive.size(); ++index)
		{
			const JsonValue pair = exclusive.at(index);
			std::array<size_t, 2> indices{};
			for (size_t side = 0; side < 2 && pair.size() == 2; ++side)
			{
				const std::string id = pair.at(side).as_identifier();
				const auto found = std::find_if(slips.list.begin(), slips.list.end(),
					[&id](const FigureSlip& slip) { return slip.id == id; });
				if (found == slips.list.end())
				{
					throw std::runtime_error(pair.where() + ": no slip is called '" + id + "'.");
				}
				indices[side] = static_cast<size_t>(found - slips.list.begin());
			}
			if (pair.size() != 2 || indices[0] == indices[1])
			{
				throw std::runtime_error(pair.where() + ": an exclusive pair names two different slips.");
			}
			slips.exclusive.push_back({ std::min(indices[0], indices[1]), std::max(indices[0], indices[1]) });
		}
		return slips;
	}

	MovementBurstPresentation read_movement_burst(const JsonValue& parent,
		const char* key)
	{
		const JsonValue json = parent.object(key);
		return { read_positive(json, "duration"), read_positive(json, "spread"),
			read_positive(json, "lift"), read_positive(json, "particle_size") };
	}
	std::vector<Colour> read_colours(const JsonValue& parent, const char* key)
	{
		const JsonValue array = parent.array(key);
		if (array.size() == 0)
		{
			throw std::runtime_error(array.where() + ": '" + key + "' needs a colour.");
		}
		std::vector<Colour> colours;
		for (size_t index = 0; index < array.size(); ++index)
		{
			colours.push_back(colour_from_text(array.at(index), array.at(index).as_string()));
		}
		return colours;
	}
}

namespace presentation_read
{
	HudPresentation read_hud(const JsonValue& root)
	{
		const JsonValue json = root.object("hud");

		HudPresentation hud;
		const JsonValue paper = json.object("paper");
		hud.paper.ink = read_colour(paper, "ink");
		hud.paper.paper = read_colour(paper, "paper");
		hud.paper.shade = read_colour(paper, "shade");
		hud.paper.keyline_width = read_positive(paper, "keyline_width");
		hud.paper.shadow_offset = paper.number("shadow_offset");
		if (hud.paper.keyline_width > 4.0f ||
			!std::isfinite(hud.paper.shadow_offset) || hud.paper.shadow_offset < 0.0f ||
			hud.paper.shadow_offset > 12.0f || hud.paper.ink.a != 1.0f ||
			hud.paper.paper.a != 1.0f || hud.paper.ink == hud.paper.paper ||
			hud.paper.shade.a <= 0.0f)
		{
			throw std::runtime_error(paper.where() +
				": keyline_width <= 4, shadow_offset from 0 to 12, distinct opaque ink/paper "
				"and visible shade required.");
		}
		hud.meter_label_font = json.string("meter_label_font");
		hud.meter_track_colour = read_colour(json, "meter_track_colour");
		hud.meter_label_gap = read_positive(json, "meter_label_gap");
		hud.reference_height = read_positive(json, "reference_height");
		// Margins and the meter box are fractions of the player's view.
		hud.right_margin = read_unit(json, "right_margin");
		hud.ammo_top_margin = read_unit(json, "ammo_top_margin");
		hud.health_top_margin = read_unit(json, "health_top_margin");
		hud.box_size = read_positive_vector(json, "box_size");
		hud.box_fill_border = read_non_negative(json, "box_fill_border");
		hud.health_colour = read_colour(json, "health_colour");

		hud.sprite_sheet = json.string("sprite_sheet");
		hud.fill_frame = json.string("fill_frame");

		const JsonValue timer = json.object("timer");
		hud.timer_font = timer.string("font");
		hud.timer_offset = read_vector_unchecked(timer, "offset");
		hud.timer_shadow_offset = read_vector_unchecked(timer, "shadow_offset");
		hud.timer_colour = read_colour(timer, "colour");
		hud.timer_shadow_colour = read_colour(timer, "shadow_colour");
		hud.timer_reference_width = read_positive(timer, "reference_width");

		const JsonValue closing = json.object("closing_clock");
		ClosingClockPresentation& clock = hud.closing_clock;
		clock.ink = read_colour(closing, "ink");
		clock.paper = read_colour(closing, "paper");
		clock.shade = read_colour(closing, "shade");
		clock.text = read_colour(closing, "text");
		clock.accent = read_colour(closing, "accent");
		clock.keyline_width = read_positive(closing, "keyline_width");
		clock.shadow_offset = closing.number("shadow_offset");
		clock.tick_reach = read_positive(closing, "tick_reach");
		clock.beat_duration = read_positive(closing, "beat_duration");
		if (clock.keyline_width > 8.0f || !std::isfinite(clock.shadow_offset) ||
			clock.shadow_offset < 0.0f || clock.shadow_offset > 16.0f ||
			clock.tick_reach > 24.0f || clock.beat_duration > 0.5f ||
			clock.ink.a != 1.0f || clock.paper.a != 1.0f || clock.text.a != 1.0f ||
			clock.accent.a != 1.0f || clock.shade.a <= 0.0f)
		{
			throw std::runtime_error(closing.where() +
				": keyline_width <= 8, shadow_offset from 0 to 16, tick_reach <= 24, "
				"beat_duration <= 0.5, opaque ink/paper/text/accent and visible shade required.");
		}

		const JsonValue low_paint = json.object("low_paint");
		hud.low_paint.tag = read_colour(low_paint, "tag");
		hud.low_paint.text = read_colour(low_paint, "text");
		hud.low_paint.padding = read_vector(low_paint, "padding");
		hud.low_paint.blink_period = read_positive(low_paint, "blink_period");
		if (hud.low_paint.padding.x < 0.0f || hud.low_paint.padding.y < 0.0f ||
			hud.low_paint.padding.x > 16.0f || hud.low_paint.padding.y > 16.0f ||
			hud.low_paint.blink_period < 0.2f || hud.low_paint.blink_period > 2.0f ||
			hud.low_paint.tag.a != 1.0f || hud.low_paint.text.a != 1.0f ||
			hud.low_paint.tag == hud.low_paint.text)
		{
			throw std::runtime_error(low_paint.where() +
				": padding from 0 to 16, blink_period from 0.2 to 2 and distinct opaque "
				"tag/text required.");
		}

		const JsonValue marker = json.object("cursor_marker");
		CursorMarkerPresentation& cursor = hud.cursor_marker;
		cursor.enabled = marker.boolean("enabled");
		cursor.ink = read_colour(marker, "ink");
		cursor.paper = read_colour(marker, "paper");
		cursor.size = read_positive(marker, "size");
		cursor.ink_width = read_positive(marker, "ink_width");
		cursor.paper_width = read_positive(marker, "paper_width");
		// The hollow centre must stay open around the hotspot.
		if (cursor.size > 96.0f || cursor.ink_width > 8.0f || cursor.paper_width > 8.0f ||
			(cursor.ink_width * 2.0f + cursor.paper_width) * 2.0f >= cursor.size * 0.5f ||
			cursor.ink.a != 1.0f || cursor.paper.a != 1.0f || cursor.ink == cursor.paper)
		{
			throw std::runtime_error(marker.where() +
				": size <= 96 with ink/paper widths <= 8 leaving at least half the marquee open, "
				"and distinct opaque ink/paper required.");
		}

		const JsonValue teammate = json.object("teammate_marker");
		TeammateMarkerPresentation& ally = hud.teammate_marker;
		ally.enabled = teammate.boolean("enabled");
		ally.ink = read_colour(teammate, "ink");
		ally.size = read_positive_vector(teammate, "size");
		ally.keyline_width = read_positive(teammate, "keyline_width");
		ally.badge_inset = read_non_negative(teammate, "badge_inset");
		ally.pointer_length = read_positive(teammate, "pointer_length");
		ally.pointer_width = read_positive(teammate, "pointer_width");
		ally.margin = read_non_negative(teammate, "margin");
		ally.fade_in = read_non_negative(teammate, "fade_in");
		// The badge needs room inside the keyline, and the pointer must read
		// as narrower than the chip it leaves.
		const float ally_short = std::min(ally.size.x, ally.size.y);
		if (ally.size.x < 16.0f || ally.size.y < 16.0f || ally.size.x > 128.0f || ally.size.y > 128.0f ||
			ally.keyline_width > 8.0f || ally.badge_inset > 16.0f ||
			(ally.keyline_width + ally.badge_inset) * 2.0f >= ally_short * 0.5f ||
			ally.pointer_length > 48.0f || ally.pointer_width * 2.0f > ally_short ||
			ally.pointer_width <= ally.keyline_width ||
			ally.margin > 64.0f || ally.fade_in > 1.0f || ally.ink.a != 1.0f)
		{
			throw std::runtime_error(teammate.where() +
				": size from 16 to 128, keyline_width <= 8 and badge_inset <= 16 leaving at least half the "
				"chip for the badge, pointer_length <= 48, pointer_width wider than the keyline and at most "
				"half the chip, margin <= 64, fade_in <= 1 and an opaque ink required.");
		}

		const JsonValue identity = json.object("player_identity");
		hud.player_identity_font = identity.string("font");
		hud.player_identity_offset = read_vector_unchecked(identity, "offset");
		hud.player_identity_shadow_offset = read_vector_unchecked(identity, "shadow_offset");
		hud.player_identity_colour = read_colour(identity, "colour");
		hud.player_identity_shadow_colour = read_colour(identity, "shadow_colour");
		hud.player_identity_padding = read_vector_unchecked(identity, "padding");
		hud.player_identity_reference_width = read_positive(identity, "reference_width");


		return hud;
	}

	PlayerIdentityPresentation read_player_identity(const JsonValue& root)
	{
		const JsonValue json = root.object("player_identity");
		PlayerIdentityPresentation identity;
		identity.enabled = json.boolean("enabled");
		identity.world_size = read_positive_vector(json, "world_size");
		identity.world_offset = read_vector(json, "world_offset");
		identity.bot_label_size = read_positive_vector(json, "bot_label_size");
		identity.bot_label_gap = read_non_negative(json, "bot_label_gap");
		identity.emphasis_duration = read_positive(json, "emphasis_duration");
		identity.emphasis_scale = read_positive(json, "emphasis_scale");
		identity.ui_size = read_positive_vector(json, "ui_size");
		identity.ui_gap = read_positive(json, "ui_gap");
		identity.background_colour = read_colour(json, "background_colour");
		identity.ink_colour = read_colour(json, "ink_colour");
		identity.border = read_positive(json, "border");
		if (identity.emphasis_scale < 1.0f || identity.emphasis_scale > 2.0f ||
			identity.border * 2.0f >= std::min(identity.ui_size.x, identity.ui_size.y) ||
			identity.background_colour.a != 1.0f || identity.ink_colour.a != 1.0f ||
			identity.background_colour == identity.ink_colour)
		{
			throw std::runtime_error(json.where() +
				": emphasis_scale must be 1..2, border must leave room for the glyph, "
				"and badge colours must be distinct and opaque.");
		}
		return identity;
	}

	GatePermissionPresentation read_gate_permission(const JsonValue& root)
	{
		const JsonValue json = root.object("gate_permission");
		GatePermissionPresentation cue;
		cue.ink = read_colour(json, "ink");
		cue.paper = read_colour(json, "paper");
		cue.size = read_positive_vector(json, "size");
		cue.inset = read_non_negative(json, "inset");
		cue.keyline_width = read_positive(json, "keyline_width");
		check_plaque(json, cue.ink, cue.paper, cue.size, cue.keyline_width);
		return cue;

	}

	FerryDirectionPresentation read_ferry_direction(const JsonValue& root)
	{
		const JsonValue json = root.object("ferry_direction");
		FerryDirectionPresentation cue;
		cue.ink = read_colour(json, "ink");
		cue.paper = read_colour(json, "paper");
		cue.size = read_positive_vector(json, "size");
		cue.gap = read_non_negative(json, "gap");
		cue.keyline_width = read_positive(json, "keyline_width");
		cue.lift_width = read_positive(json, "lift_width");
		check_plaque(json, cue.ink, cue.paper, cue.size, cue.keyline_width);
		return cue;

	}

	std::array<IdleDripPresentation, tuned_weapon_count> read_idle_drips(const JsonValue& root)
	{
		const JsonValue json = root.object("idle_drip");
		IdleDripPresentation drip;
		drip.enabled = json.boolean("enabled");
		drip.sprite_sheet = json.identifier("sprite_sheet");
		drip.fill_frame = json.identifier("fill_frame");
		drip.keyline = read_colour(json, "keyline");
		drip.grow_time = read_positive(json, "grow_time");
		drip.stretch_time = read_positive(json, "stretch_time");
		drip.lifetime = read_positive(json, "lifetime");
		drip.gravity = read_positive(json, "gravity");
		drip.keyline_width = read_positive(json, "keyline_width");
		std::array<IdleDripPresentation, tuned_weapon_count> tools;
		for (IdleDripPresentation& tool : tools)
		{
			tool = drip;
			tool.enabled = false;
		}
		const JsonValue profiles = json.object("tools");
		const auto read_tool = [&](WeaponType type)
		{
			const JsonValue profile = profiles.object(weapon_name(type));
			IdleDripPresentation& tool = tools.at(static_cast<size_t>(type));
			tool.enabled = drip.enabled;
			tool.interval_min = read_positive(profile, "interval_min");
			tool.interval_max = read_positive(profile, "interval_max");
			tool.size = read_positive(profile, "size");
			tool.stretch = read_positive(profile, "stretch");
			tool.nozzle_inset = read_positive(profile, "nozzle_inset");
			tool.nozzle_underside = read_positive(profile, "nozzle_underside");
			if (tool.interval_max < tool.interval_min ||
				tool.keyline_width * 2.0f >= tool.size)
			{
				throw std::runtime_error(profile.where() +
					": intervals must be ordered and keylines must leave visible paint.");
			}
		};
		read_tool(WeaponType::sprayer);
		read_tool(WeaponType::sniper);
		read_tool(WeaponType::bucket);
		return tools;
	}

	WeaponFeedbackPresentation read_weapon_feedback(const JsonValue& root)
	{
		const JsonValue json = root.object("weapon_feedback");
		WeaponFeedbackPresentation feedback;
		feedback.enabled = json.boolean("enabled");
		feedback.sprite_sheet = json.identifier("sprite_sheet");
		feedback.fill_frame = json.identifier("fill_frame");
		feedback.keyline = read_colour(json, "keyline");
		feedback.highlight = read_colour(json, "highlight");
		feedback.muzzle_duration = read_positive(json, "muzzle_duration");
		feedback.empty_duration = read_positive(json, "empty_duration");
		feedback.recovered_duration = read_positive(json, "recovered_duration");
		feedback.keyline_width = read_positive(json, "keyline_width");
		feedback.status_size = read_positive(json, "status_size");
		feedback.status_offset = read_positive(json, "status_offset");
		feedback.aim_marker_distance = read_positive(json, "aim_marker_distance");
		feedback.aim_marker_size = read_positive(json, "aim_marker_size");
		feedback.aim_marker_stroke = read_positive(json, "aim_marker_stroke");
		if (feedback.aim_marker_distance > 128.0f || feedback.aim_marker_size > 32.0f ||
			feedback.aim_marker_distance <= feedback.aim_marker_size ||
			feedback.aim_marker_stroke > feedback.aim_marker_size / 4.0f ||
			feedback.keyline_width >= feedback.aim_marker_stroke)
		{
			throw std::runtime_error(json.where() +
				": aim marker must stay near the nozzle with distinct, visible strokes "
				"(distance <= 128, size <= 32, distance > size, keyline < stroke <= size / 4).");
		}
		if (feedback.keyline_width * 2.0f >= feedback.status_size)
		{
			throw std::runtime_error(json.where() +
				": 'keyline_width' must leave visible fill inside 'status_size'.");
		}
		const JsonValue sizes = json.object("muzzle_sizes");
		for (size_t index = 0; index < tuned_weapon_count; ++index)
		{
			const char* const name = weapon_names[index];
			const Vector2F size = read_positive_vector(sizes, name);
			if (feedback.keyline_width * 2.0f >= std::min(size.x, size.y))
			{
				throw std::runtime_error(sizes.object(name).where() +
					": 'keyline_width' must leave visible fill inside the muzzle size.");
			}
			feedback.muzzle_sizes[index] = size;
		}
		const JsonValue low = json.object("low_paint");
		feedback.low_paint_size = read_positive_vector(low, "size");
		feedback.low_paint_offset = read_vector(low, "offset");
		feedback.low_paint_blink_period = read_positive(low, "blink_period");
		feedback.low_paint_dim_alpha = read_unit(low, "dim_alpha");
		if (feedback.keyline_width * 2.0f >= std::min(feedback.low_paint_size.x, feedback.low_paint_size.y) ||
			std::max(feedback.low_paint_size.x, feedback.low_paint_size.y) > feedback.status_size * 2.0f ||
			std::abs(feedback.low_paint_offset.x) > 64.0f || std::abs(feedback.low_paint_offset.y) > 64.0f ||
			feedback.low_paint_blink_period < 0.2f || feedback.low_paint_blink_period > 2.0f ||
			feedback.low_paint_dim_alpha >= 1.0f)
		{
			throw std::runtime_error(low.where() +
				": size must leave visible fill and stay within twice 'status_size', "
				"offset within 64 of the status slot, blink_period from 0.2 to 2 and "
				"dim_alpha below 1 required.");
		}
		return feedback;
	}

	BlockedShotPresentation read_blocked_shot(const JsonValue& root)
	{
		const JsonValue json = root.object("blocked_shot");
		BlockedShotPresentation contact;
		contact.enabled = json.boolean("enabled");
		contact.sprite_sheet = json.identifier("sprite_sheet");
		contact.fill_frame = json.identifier("fill_frame");
		contact.highlight = read_colour(json, "highlight");
		contact.keyline = read_colour(json, "keyline");
		contact.duration = read_positive(json, "duration");
		contact.repeat_delay = read_positive(json, "repeat_delay");
		contact.width = read_positive(json, "width");
		contact.reach = read_positive(json, "reach");
		contact.bead_size = read_positive(json, "bead_size");
		contact.stroke_width = read_positive(json, "stroke_width");
		contact.keyline_width = read_positive(json, "keyline_width");
		if (contact.duration > 0.4f || contact.repeat_delay < 0.05f ||
			contact.repeat_delay > 1.0f || contact.width > 48.0f || contact.reach > 48.0f ||
			contact.bead_size > contact.width * 0.5f ||
			contact.stroke_width > contact.width * 0.25f ||
			contact.keyline_width * 2.0f >=
				std::min(contact.stroke_width, contact.bead_size * 0.6f))
		{
			throw std::runtime_error(json.where() +
				": blocked contacts need duration <= 0.4 seconds, repeat_delay in [0.05, 1], "
				"width/reach <= 48, bead_size <= width / 2, stroke_width <= width / 4 "
				"and keylines that leave visible strokes and beads.");
		}
		return contact;
	}

	ToolMotionPresentation read_tool_motion(const JsonValue& root)
	{
		const JsonValue json = root.object("tool_motion");
		ToolMotionPresentation motion;
		motion.enabled = json.boolean("enabled");
		motion.shot_pose_split = read_positive(json, "shot_pose_split");
		motion.roller_travel_per_turn = read_positive(json, "roller_travel_per_turn");
		motion.roller_lift_time = read_positive(json, "roller_lift_time");
		motion.roller_settle_time = read_positive(json, "roller_settle_time");
		motion.hose_travel_per_sway = read_positive(json, "hose_travel_per_sway");
		motion.hose_settle_time = read_positive(json, "hose_settle_time");
		motion.bucket_travel_per_swing = read_positive(json, "bucket_travel_per_swing");
		motion.bucket_settle_time = read_positive(json, "bucket_settle_time");
		const JsonValue tools = json.object("tools");
		for (size_t index = 0; index < tuned_weapon_count; ++index)
		{
			const JsonValue tool = tools.object(weapon_names[index]);
			motion.tools[index] = { read_positive(tool, "shot_recovery"),
				read_positive(tool, "valve_time") };
		}
		const JsonValue mist = json.object("mist_tail");
		motion.mist_sheet = mist.identifier("sprite_sheet");
		motion.mist_frame = mist.identifier("fill_frame");
		motion.mist_tail_duration = read_positive(mist, "duration");
		motion.mist_tail_distance = read_positive(mist, "distance");
		motion.mist_tail_size = read_positive_vector(mist, "size");
		motion.mist_tail_opacity = read_positive(mist, "opacity");
		if (motion.shot_pose_split >= 1.0f || motion.mist_tail_opacity > 1.0f)
		{
			throw std::runtime_error(json.where() +
				": shot_pose_split must be less than one and mist opacity at most one.");
		}
		return motion;
	}

	FigurePresentation read_figure(const JsonValue& root)
	{
		const JsonValue json = root.object("figure");
		FigurePresentation figure;
		figure.sprite_sheet = json.identifier("sprite_sheet");
		figure.rest_arms_frame = json.identifier("rest_arms_frame");
		figure.rest_arms_paint_frame = json.identifier("rest_arms_paint_frame");
		figure.sleeves_frame = json.identifier("sleeves_frame");
		figure.sleeves_paint_frame = json.identifier("sleeves_paint_frame");
		figure.canvas_size = read_positive_vector(json, "canvas_size");
		figure.body_inset = read_non_negative(json, "body_inset");
		if (!(figure.body_width() > 0.0f))
		{
			throw std::runtime_error(json.where() + ": 'body_inset' must leave the hitbox inside 'canvas_size'.");
		}
		figure.rear_shoulder = read_vector(json, "rear_shoulder");
		figure.front_shoulder = read_vector(json, "front_shoulder");
		figure.arm_width = read_positive(json, "arm_width");
		figure.elbow_drop = read_positive(json, "elbow_drop");
		figure.hand_size = read_positive(json, "hand_size");
		figure.hand_edge = read_positive(json, "hand_edge");
		figure.arm_colour = read_colour(json, "arm_colour");
		figure.hand_colour = read_colour(json, "hand_colour");
		const JsonValue quiet = json.object("quiet");
		auto& motion = figure.quiet;
		motion.enabled = quiet.boolean("enabled");
		motion.settle_time = read_positive(quiet, "settle_time");
		motion.breath_period = read_positive(quiet, "breath_period");
		motion.breath_height = read_positive(quiet, "breath_height");
		motion.shift_period = read_positive(quiet, "shift_period");
		motion.shift_distance = read_positive(quiet, "shift_distance");
		motion.blink_min_interval = read_positive(quiet, "blink_min_interval");
		motion.blink_max_interval = read_positive(quiet, "blink_max_interval");
		motion.blink_duration = read_positive(quiet, "blink_duration");
		motion.lean_distance = read_positive(quiet, "lean_distance");
		motion.lean_acceleration = read_positive(quiet, "lean_acceleration");
		motion.lean_settle_time = read_positive(quiet, "lean_settle_time");
		motion.eyelid_height = read_positive(quiet, "eyelid_height");
		motion.face_colour = read_colour(quiet, "face_colour");
		if (motion.blink_max_interval < motion.blink_min_interval ||
			motion.blink_duration >= motion.blink_min_interval ||
			motion.breath_height >= figure.canvas_size.y / 10.0f ||
			motion.lean_distance + motion.shift_distance >= figure.body_width() / 4.0f)
		{
			throw std::runtime_error(quiet.where() +
				": quiet motion must stay modest and blink intervals must be ordered.");
		}
		const JsonValue eyes = quiet.array("eyes");
		if (eyes.size() != motion.eyes.size())
		{
			throw std::runtime_error(eyes.where() + ": needs the two authored eye rectangles.");
		}
		for (size_t index = 0; index < motion.eyes.size(); ++index)
		{
			const JsonValue eye = eyes.at(index);
			const RectangleF rectangle(eye.number("x"), eye.number("y"),
				read_positive(eye, "width"), read_positive(eye, "height"));
			if (!(rectangle.x >= 0.0f && rectangle.y >= 0.0f &&
				rectangle.right() <= figure.canvas_size.x && rectangle.bottom() <= figure.canvas_size.y &&
				motion.eyelid_height < rectangle.height))
			{
				throw std::runtime_error(eye.where() + ": eye must stay in the canvas and leave room for its lid.");
			}
			motion.eyes[index] = rectangle;
		}
		if (figure.hand_edge * 2.0f >= figure.hand_size)
		{
			throw std::runtime_error(json.where() + ": 'hand_edge' must leave an interior in 'hand_size'.");
		}
		const JsonValue grips = json.object("grips");
		for (size_t index = 0; index < tuned_weapon_count; ++index)
		{
			const JsonValue grip = grips.object(weapon_names[index]);
			figure.grips[index] = { read_vector(grip, "rear"), read_vector(grip, "front") };
		}
		figure.slips = read_figure_slips(json.object("slips"), figure.canvas_size);
		return figure;
	}

	RespawnRedrawPresentation read_respawn_redraw(const JsonValue& root)
	{
		const JsonValue json = root.object("respawn_redraw");

		RespawnRedrawPresentation redraw;
		redraw.sprite_sheet = json.identifier("sprite_sheet");
		redraw.pen_frame = json.identifier("pen_frame");
		redraw.pen_colour = read_colour(json, "pen_colour");
		redraw.canvas_size = read_positive_vector(json, "canvas_size");
		redraw.pen_width = read_positive(json, "pen_width");
		// Fractions of the respawn wait.
		redraw.draw_from = read_unit(json, "draw_from");
		redraw.fill_at = read_unit(json, "fill_at");
		if (!(redraw.draw_from < redraw.fill_at))
		{
			throw std::runtime_error(json.where() + ": 'draw_from' must come before 'fill_at'.");
		}
		redraw.camera_hold = read_non_negative(json, "camera_hold");

		const JsonValue strokes = json.array("strokes");
		if (strokes.size() == 0)
		{
			throw std::runtime_error(strokes.where() + ": a figure needs at least one stroke.");
		}
		const Vector2F last_nib = redraw.canvas_size - Vector2F(redraw.pen_width);
		for (size_t index = 0; index < strokes.size(); ++index)
		{
			const JsonValue stroke_json = strokes.at(index);
			RespawnStroke stroke;
			stroke.part = stroke_json.string("part");
			stroke.points = read_pen_points(stroke_json, last_nib);
			redraw.strokes.push_back(std::move(stroke));
		}
		return redraw;
	}

	OpeningPresentation read_opening(const JsonValue& root)
	{
		const JsonValue json = root.object("opening");

		OpeningPresentation opening;
		opening.sprite_sheet = json.identifier("sprite_sheet");
		opening.fill_frame = json.identifier("fill_frame");
		opening.bezel_shades = read_colours(json, "bezel_shades");
		opening.bezel_shadow = read_colour(json, "bezel_shadow");
		const JsonValue art = json.object("monitor_art");
		MonitorArtPresentation& monitor = opening.monitor_art;
		monitor.ink = read_colour(art, "ink");
		monitor.highlight = read_colour(art, "highlight");
		monitor.power_light = read_colour(art, "power_light");
		monitor.pen_width = read_unit(art, "pen_width");
		monitor.corner_step = read_unit(art, "corner_step");
		monitor.shadow_width = read_unit(art, "shadow_width");
		monitor.vent_count = art.integer("vent_count");
		if (monitor.pen_width <= 0.0f || monitor.pen_width > 0.02f ||
			monitor.corner_step < monitor.pen_width || monitor.corner_step > 0.05f ||
			monitor.shadow_width <= 0.0f || monitor.shadow_width > 0.1f ||
			monitor.vent_count < 1 || monitor.vent_count > 12)
		{
			throw std::runtime_error(art.where() +
				": monitor pen, corner and shadow widths must fit the bezel, "
				"and 'vent_count' must be from 1 to 12.");
		}
		const auto read_monitor_rectangle = [&art](const char* key)
		{
			const JsonValue value = art.object(key);
			const RectangleF rectangle(read_unit(value, "x"), read_unit(value, "y"),
				read_unit(value, "width"), read_unit(value, "height"));
			if (rectangle.width <= 0.0f || rectangle.height <= 0.0f ||
				rectangle.right() > 1.0f || rectangle.bottom() > 1.0f)
			{
				throw std::runtime_error(value.where() + ": monitor artwork must be on the bezel.");
			}
			return rectangle;
		};
		monitor.vent_area = read_monitor_rectangle("vent_area");
		monitor.power_button = read_monitor_rectangle("power_button");
		monitor.power_indicator = read_monitor_rectangle("power_indicator");
		const JsonValue glass = json.object("glass");
		opening.glass = RectangleF(read_unit(glass, "x"), read_unit(glass, "y"),
			read_unit(glass, "width"), read_unit(glass, "height"));
		if (opening.glass.width <= 0.0f || opening.glass.height <= 0.0f ||
			opening.glass.right() > 1.0f || opening.glass.bottom() > 1.0f)
		{
			throw std::runtime_error(glass.where() + ": the glass must be on the screen.");
		}
		for (const RectangleF& hardware : { monitor.vent_area, monitor.power_button,
			monitor.power_indicator })
		{
			if (overlaps(hardware, opening.glass))
			{
				throw std::runtime_error(art.where() + ": monitor hardware must stay outside the glass.");
			}
		}
		opening.glass_colour = read_colour(json, "glass_colour");
		opening.program_colour = read_colour(json, "program_colour");
		opening.desktop_colour = read_colour(json, "desktop_colour");
		opening.palette_height = read_unit(json, "palette_height");
		opening.palette_sheet = json.identifier("palette_sheet");
		const JsonValue palette = json.array("palette_frames");
		if (opening.palette_sheet.empty() || palette.size() == 0)
		{
			throw std::runtime_error(json.where() +
				": 'palette_sheet' and 'palette_frames' must not be empty.");
		}
		for (size_t index = 0; index < palette.size(); ++index)
		{
			const JsonValue entry = palette.at(index);
			const std::string frame = entry.as_identifier();
			if (frame.empty())
			{
				throw std::runtime_error(entry.where() + ": palette frame must not be empty.");
			}
			opening.palette_frames.push_back(frame);
		}
		opening.slot_outline = read_colour(json, "slot_outline");
		opening.sticky_notes = read_colours(json, "sticky_notes");
		opening.stickers = read_colours(json, "stickers");
		opening.scribble = read_colour(json, "scribble");
		opening.coffee = read_colour(json, "coffee");
		opening.crack = read_colour(json, "crack");
		opening.silhouette = read_colour(json, "silhouette");
		const JsonValue person_art = json.object("silhouette_art");
		SilhouetteArtPresentation& person = opening.silhouette_art;
		person.ink = read_colour(person_art, "ink");
		person.rim = read_colour(person_art, "rim");
		person.shade = read_colour(person_art, "shade");
		person.pen_width = read_unit(person_art, "pen_width");
		person.shade_width = read_unit(person_art, "shade_width");
		if (person.pen_width <= 0.0f || person.pen_width > 0.025f ||
			person.shade_width < person.pen_width || person.shade_width > 0.08f)
		{
			throw std::runtime_error(person_art.where() +
				": silhouette pen and shade widths must fit its coarse contour.");
		}
		const JsonValue cursor_art = json.object("cursor_art");
		CursorArtPresentation& cursor = opening.cursor_art;
		cursor.ink = read_colour(cursor_art, "ink");
		cursor.fill = read_colour(cursor_art, "fill");
		cursor.shade = read_colour(cursor_art, "shade");
		cursor.shadow = read_colour(cursor_art, "shadow");
		cursor.shadow_offset = read_unit(cursor_art, "shadow_offset");
		cursor.click_duration = read_positive(cursor_art, "click_duration");
		cursor.click_reach = read_unit(cursor_art, "click_reach");
		if (cursor.shadow_offset > 0.2f || cursor.click_duration <= 0.0f ||
			cursor.click_duration > 0.5f || cursor.click_reach <= 0.0f || cursor.click_reach > 0.6f)
		{
			throw std::runtime_error(cursor_art.where() +
				": cursor shadow/click must stay local; 'click_duration' must be in (0, 0.5].");
		}
		opening.adult_height = read_unit(json, "adult_height");
		opening.child_height = read_unit(json, "child_height");
		opening.cursor_size = read_unit(json, "cursor_size");
		opening.preview_strength = read_unit(json, "preview_strength");
		opening.figure_grey = read_colour(json, "figure_grey");
		opening.percent_font = json.identifier("percent_font");
		opening.percent_height = read_unit(json, "percent_height");
		const JsonValue reveal_art = json.object("reveal_art");
		RevealArtPresentation& reveal = opening.reveal_art;
		reveal.ink = read_colour(reveal_art, "ink");
		reveal.paper = read_colour(reveal_art, "paper");
		reveal.shade = read_colour(reveal_art, "shade");
		reveal.keyline_width = read_positive(reveal_art, "keyline_width");
		reveal.padding = read_unit(reveal_art, "padding");
		reveal.shadow_offset = read_unit(reveal_art, "shadow_offset");
		reveal.tick_length = read_unit(reveal_art, "tick_length");
		reveal.beat_duration = read_positive(reveal_art, "beat_duration");
		if (reveal.keyline_width > 0.06f || reveal.padding < 0.08f || reveal.padding > 0.18f ||
			reveal.shadow_offset > 0.1f || reveal.tick_length > 0.2f || reveal.beat_duration > 0.5f)
		{
			throw std::runtime_error(reveal_art.where() +
				": keyline_width must be in (0, 0.06], padding in [0.08, 0.18], "
				"shadow_offset in [0, 0.1], tick_length in [0, 0.2] and beat_duration in (0, 0.5].");
		}
		const JsonValue acting = json.object("acting");
		opening.face = read_colour(acting, "face");
		opening.sweat = read_colour(acting, "sweat");
		opening.hop = read_unit(acting, "hop");
		opening.shake = read_unit(acting, "shake");
		opening.slump = read_unit(acting, "slump");
		if (opening.adult_height <= 0.0f || opening.child_height <= 0.0f ||
			opening.cursor_size <= 0.0f || opening.palette_height <= 0.0f ||
			opening.percent_height <= 0.0f)
		{
			throw std::runtime_error(json.where() + ": 'adult_height', 'child_height', "
				"'cursor_size', 'palette_height' and 'percent_height' must be more than zero.");
		}
		return opening;
	}

	MusicMixPresentation read_music_mix(const JsonValue& root)
	{
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
		MusicMixPresentation mix;
		mix.fade_in = music_duration("fade_in");
		mix.fade_out = music_duration("fade_out");
		mix.duck_time = music_duration("duck_time");
		mix.restore_time = music_duration("restore_time");
		mix.pause_gain = read_unit(music_mix, "pause_gain");
		return mix;
	}

	CameraPresentation read_camera(const JsonValue& root)
	{
		const JsonValue camera = root.object("camera");
		// Three keys are optional, so a misspelt one must fail rather than be skipped.
		require_known_keys(camera, { "reference_height", "minimum_world_width",
			"horizontal_lead_seconds", "horizontal_lead_max_distance", "shake" });
		CameraPresentation framing;
		framing.reference_height = read_positive(camera, "reference_height");
		const auto camera_option = [&](const char* key)
		{
			return camera.has(key) ? read_non_negative(camera, key) : 0.0f;
		};
		framing.minimum_world_width = camera_option("minimum_world_width");
		framing.horizontal_lead_seconds = camera_option("horizontal_lead_seconds");
		framing.horizontal_lead_max_distance = camera_option("horizontal_lead_max_distance");
		const JsonValue shake = camera.object("shake");
		ViewShakePresentation& kick = framing.shake;
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
		return framing;
	}

	MovementFeedbackPresentation read_movement_feedback(const JsonValue& root)
	{
		const JsonValue movement = root.object("movement_feedback");
		MovementFeedbackPresentation feedback;
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
		return feedback;
	}

	CombatFeedbackPresentation read_combat_feedback(const JsonValue& root)
	{
		const JsonValue combat = root.object("combat_feedback");
		CombatFeedbackPresentation contact;
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
		return contact;
	}
	MatchPresentation read_match(const JsonValue& root)
	{
		const JsonValue match = root.object("match");
		MatchPresentation intro;
		intro.reference_size = read_positive_vector(match, "reference_size");
		intro.countdown_size = read_positive_vector(match, "countdown_size");
		intro.countdown_center_y = read_positive(match, "countdown_center_y");
		if (intro.countdown_center_y >= 1.0f)
		{
			throw std::runtime_error(match.where() + ": countdown_center_y must be less than one.");
		}
		intro.controls_margin = read_positive_vector(match, "controls_margin");
		intro.controls_padding = read_positive_vector(match, "controls_padding");
		intro.controls_gap = read_positive(match, "controls_gap");
		intro.stage_entry_size = read_positive_vector(match, "stage_entry_size");
		intro.stage_entry_top = read_positive(match, "stage_entry_top");
		const JsonValue controls_text = match.object("controls_text");
		require_known_keys(controls_text, { "font", "colour", "shadow_colour", "shadow_offset", "y" });
		require_known_keys(controls_text.object("shadow_offset"), { "x", "y" });
		intro.controls_text.font = controls_text.string("font");
		intro.controls_text.colour = read_colour(controls_text, "colour");
		intro.controls_text.shadow_colour = read_colour(controls_text, "shadow_colour");
		intro.controls_text.shadow_offset = read_vector(controls_text, "shadow_offset");
		intro.controls_text.y = read_unit(controls_text, "y");
		const JsonValue countdown_art = match.object("countdown_art");
		CountdownArtPresentation& signal = intro.countdown_art;
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
		return intro;
	}

	DebugOverlayPresentation read_debug_overlay(const JsonValue& root)
	{
		const JsonValue json = root.object("debug_overlay");

		DebugOverlayPresentation debug;
		debug.font = json.string("font");
		debug.font_scale = read_positive(json, "font_scale");
		debug.line_spacing = read_positive(json, "line_spacing");
		debug.position = read_vector_unchecked(json, "position");
		debug.colour = read_colour(json, "colour");
		debug.shadow_colour = read_colour(json, "shadow_colour");
		debug.shadow_offset = read_vector_unchecked(json, "shadow_offset");
		debug.shadow_scale = read_positive(json, "shadow_scale");
		debug.menu_value_colour = read_colour(json, "menu_value_colour");
		const JsonValue station = json.object("station_label");
		debug.station_label.font = station.string("font");
		debug.station_label.scale = read_positive(station, "scale");
		debug.station_label.offset = read_vector_unchecked(station, "offset");
		debug.station_label.colour = read_colour(station, "colour");
		return debug;
	}
}
