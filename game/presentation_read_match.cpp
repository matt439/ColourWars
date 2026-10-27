#include "game/pch.h"
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

// Readers for the in-match and visual sections of presentation.json: HUD,
// identity, feedback, figure, respawn, opening and the debug overlay.

using namespace labrador;
using namespace mattmath;
using namespace content_value;
using namespace presentation_read;

namespace
{
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

		const JsonValue identity = json.object("player_identity");
		hud.player_identity_font = identity.string("font");
		hud.player_identity_offset = read_vector_unchecked(identity, "offset");
		hud.player_identity_shadow_offset = read_vector_unchecked(identity, "shadow_offset");
		hud.player_identity_colour = read_colour(identity, "colour");
		hud.player_identity_shadow_colour = read_colour(identity, "shadow_colour");
		hud.player_identity_background_colour = read_colour(identity, "background_colour");
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

	std::array<IdleDripPresentation, 5> read_idle_drips(const JsonValue& root)
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
		std::array<IdleDripPresentation, 5> tools;
		for (IdleDripPresentation& tool : tools)
		{
			tool = drip;
			tool.enabled = false;
		}
		const JsonValue profiles = json.object("tools");
		const auto read_tool = [&](WeaponType type, const char* name)
		{
			const JsonValue profile = profiles.object(name);
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
		read_tool(WeaponType::sprayer, "sprayer");
		read_tool(WeaponType::sniper, "sniper");
		read_tool(WeaponType::bucket, "bucket");
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
		const std::array<const char*, 5> names = { "sprayer", "sniper", "roller", "mister", "bucket" };
		for (size_t index = 0; index < names.size(); ++index)
		{
			const Vector2F size = read_positive_vector(sizes, names[index]);
			if (feedback.keyline_width * 2.0f >= std::min(size.x, size.y))
			{
				throw std::runtime_error(sizes.object(names[index]).where() +
					": 'keyline_width' must leave visible fill inside the muzzle size.");
			}
			feedback.muzzle_sizes[index] = size;
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
		const std::array<const char*, 5> names = { "sprayer", "sniper", "roller", "mister", "bucket" };
		for (size_t index = 0; index < names.size(); ++index)
		{
			const JsonValue tool = tools.object(names[index]);
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
		const std::array<const char*, 5> keys = { "sprayer", "sniper", "roller", "mister", "bucket" };
		for (size_t index = 0; index < keys.size(); ++index)
		{
			const JsonValue grip = grips.object(keys[index]);
			figure.grips[index] = { read_vector(grip, "rear"), read_vector(grip, "front") };
		}
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
			const JsonValue points = stroke_json.array("points");
			if (points.size() < 2)
			{
				throw std::runtime_error(points.where() + ": a stroke needs at least two points.");
			}
			for (size_t at = 0; at < points.size(); ++at)
			{
				const JsonValue point_json = points.at(at);
				const Vector2F point(point_json.number("x"), point_json.number("y"));
				// The pen's whole nib stays on the canvas.
				if (!(point.x >= 0.0f && point.y >= 0.0f &&
					point.x <= last_nib.x && point.y <= last_nib.y))
				{
					throw std::runtime_error(point_json.where() +
						": the pen leaves the canvas at this point.");
				}
				if (!stroke.points.empty())
				{
					const Vector2F step = point - stroke.points.back();
					if ((step.x == 0.0f) == (step.y == 0.0f))
					{
						throw std::runtime_error(point_json.where() + ": the pen draws only "
							"horizontal and vertical lines, and never stands still.");
					}
				}
				stroke.points.push_back(point);
			}
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
			if (hardware.x < opening.glass.right() && opening.glass.x < hardware.right() &&
				hardware.y < opening.glass.bottom() && opening.glass.y < hardware.bottom())
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
		const JsonValue station = json.object("station_label");
		debug.station_label.font = station.string("font");
		debug.station_label.scale = read_positive(station, "scale");
		debug.station_label.offset = read_vector_unchecked(station, "offset");
		debug.station_label.colour = read_colour(station, "colour");
		return debug;
	}
}
