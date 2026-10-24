#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/tune_scope.h"
#include <charconv>
#include <cmath>

namespace
{
	struct ScopeRow
	{
		const char* pattern;
		TuneScope scope;
	};

	// The string allowlist and its consumer audit, before any numeric wildcard.
	constexpr ScopeRow colour_rows[] = {
		// MovementFeedback reads its borrowed record when drawing. Its colour
		// is the fallback for unpainted dust; paint flecks keep their stored tint.
		{ "presentation.movement_feedback.colour", TuneScope::live },
		{ "presentation.movement_feedback.highlight", TuneScope::live },
		{ "presentation.movement_feedback.keyline", TuneScope::live },
		// DebugText reads its borrowed look on every draw; Level refreshes the
		// existing station labels before refresh_layout's same-size early return.
		{ "presentation.debug_overlay.colour", TuneScope::live },
		{ "presentation.debug_overlay.shadow_colour", TuneScope::live },
		{ "presentation.debug_overlay.station_label.colour", TuneScope::live },
		// MainMenuHome::init and Mode Select copy the tint into their UiTexture
		// (as do the other main-menu pages when each page is built).
		{ "presentation.main_menu.backgrounds.home", TuneScope::next_screen },
		// Active, connected Team Select updates read both tints into FocusGroup
		// during visual refresh. Frozen draws retain the previous refresh's
		// colours; the page's atlas and frame names remain captured.
		{ "presentation.main_menu.team_select.selected", TuneScope::live },
		{ "presentation.main_menu.team_select.unselected", TuneScope::live },
		// Active, connected Weapon Select visual refresh reads both tile tints.
		// Frozen draws or missing claimed devices retain the last refresh;
		// catalog, names, descriptions, fonts and page paper remain captured.
		{ "presentation.main_menu.weapon_select.selected", TuneScope::live },
		{ "presentation.main_menu.weapon_select.unselected", TuneScope::live },
		// WeaponFeedback reads both borrowed colours during each cue and aim
		// marker draw. Cue state, team tint and captured sheet/fill stay intact.
		{ "presentation.weapon_feedback.highlight", TuneScope::live },
		{ "presentation.weapon_feedback.keyline", TuneScope::live },
		// PlayerBadge reads this borrowed ink on every world/HUD/menu draw.
		// Its existing reader requires opaque ink distinct from the background.
		{ "presentation.player_identity.ink_colour", TuneScope::live },
		// The same borrowed badge record supplies opaque paper on every draw;
		// complete candidate validation keeps it distinct from the current ink.
		{ "presentation.player_identity.background_colour", TuneScope::live },
		// CombatFeedback reads both borrowed tints on each draw of an existing
		// hurt accent or detached burst; timers and captured fill stay intact.
		{ "presentation.combat_feedback.highlight", TuneScope::live },
		{ "presentation.combat_feedback.keyline", TuneScope::live },
		// BlockedShotFeedback reads both borrowed tints on each draw. Existing
		// contacts, owner repeat budgets and the captured sheet/fill stay intact.
		{ "presentation.blocked_shot.highlight", TuneScope::live },
		{ "presentation.blocked_shot.keyline", TuneScope::live },
		// IdleDrip reads the common keyline through its borrowed tool record on
		// every attached/falling draw; captured paint and sheet/fill stay intact.
		{ "presentation.idle_drip.keyline", TuneScope::live },
		// RespawnRedraw reads the borrowed pen tint on each partial-path draw;
		// player progress, ordered strokes and captured sheet/frame stay intact.
		{ "presentation.respawn_redraw.pen_colour", TuneScope::live },
		// ScreenCursor reads these borrowed tints for the resting arrow and
		// reveal click ticks on every draw; sheet/frame and draw inputs stay intact.
		{ "presentation.opening.cursor_art.ink", TuneScope::live },
		{ "presentation.opening.cursor_art.fill", TuneScope::live },
		// Silhouette reads these borrowed inks on every visible story draw;
		// body fill, persona/pose inputs and captured sheet/frame stay intact.
		{ "presentation.opening.silhouette_art.ink", TuneScope::live },
		{ "presentation.opening.silhouette_art.rim", TuneScope::live },
		{ "presentation.opening.silhouette_art.shade", TuneScope::live },
		// RevealArt reads these borrowed card tints on every draw; team text,
		// caller inputs and captured font/sheet/frame keep their existing policy.
		{ "presentation.opening.reveal_art.ink", TuneScope::live },
		{ "presentation.opening.reveal_art.paper", TuneScope::live },
		{ "presentation.opening.reveal_art.shade", TuneScope::live },
		// ScreenCursor reads the remaining borrowed arrow materials on every
		// draw; ink/fill, click inputs and captured sheet/frame stay intact.
		{ "presentation.opening.cursor_art.shade", TuneScope::live },
		{ "presentation.opening.cursor_art.shadow", TuneScope::live },
		// Monitor reads this borrowed tint each time the closed program shows
		// the desktop; persona, swatches and captured sheet/frame stay intact.
		{ "presentation.opening.desktop_colour", TuneScope::live },
		// Monitor reads this borrowed indicator ink on every front-details draw;
		// captured persona, sheet/frame and all indicator geometry stay intact.
		{ "presentation.opening.monitor_art.power_light", TuneScope::live },
		// Monitor reads this borrowed program paper on every draw; callers may
		// cover it later, while persona and captured resources keep their policy.
		{ "presentation.opening.program_colour", TuneScope::live },
		// FigureActing reads the borrowed eye paint and sweat on each applicable
		// draw; caller pose/time, redraw strokes and captured sheet/fill stay intact.
		{ "presentation.opening.acting.face", TuneScope::live },
		{ "presentation.opening.acting.sweat", TuneScope::live },
		// Monitor reads the same borrowed glass tint for its main glass, palette,
		// picker and close button; persona and captured resources stay intact.
		{ "presentation.opening.glass_colour", TuneScope::live },
		// Silhouette reads its borrowed body tint for every ordered mass fill;
		// persona, pose inputs and captured resources stay intact.
		{ "presentation.opening.silhouette", TuneScope::live },
		// Monitor reads this borrowed note ink for every ordered scribble dab;
		// captured note positions, colours, tilts and resources stay intact.
		{ "presentation.opening.scribble", TuneScope::live },
		// InterfaceGameplay reads the borrowed health tint on every meter draw;
		// caller health/ammo, the track and captured sheet/fonts stay intact.
		{ "presentation.hud.health_colour", TuneScope::live },
		// Both HUD meters borrow the track tint on each draw; their ordered
		// health/team fill prefixes and captured sheet/fonts remain unchanged.
		{ "presentation.hud.meter_track_colour", TuneScope::live },
		// Existing Monitors read the borrowed thin crack tint per segment;
		// captured paths, broad ink order, geometry and resources remain fixed.
		{ "presentation.opening.crack", TuneScope::live },
		// FigureAttachments reads these borrowed tints for live arms, lids and
		// hands; captured rest/sleeve layers, geometry and resources stay fixed.
		{ "presentation.figure.arm_colour", TuneScope::live },
		{ "presentation.figure.hand_colour", TuneScope::live },
		// Existing FigureAttachments read this borrowed eye paint-out on each
		// blink draw; lid ink, quiet timing and captured layers stay intact.
		{ "presentation.figure.quiet.face_colour", TuneScope::live },
		// Existing HUDs read this borrowed foreground on normal timer draws;
		// closing-clock text, shadow and captured font handles stay separate.
		{ "presentation.hud.timer.colour", TuneScope::live },
		// The same retained HUD reads its normal-clock shadow per draw;
		// foreground, closing signal and captured font remain independent.
		{ "presentation.hud.timer.shadow_colour", TuneScope::live },
		// The retained HUD reads this opaque text on its closing-clock branch;
		// full candidate validation preserves the reader's alpha-one contract.
		{ "presentation.hud.closing_clock.text", TuneScope::live },
	};

	// First match wins; evidence is the reader of each value. "live" means a
	// pointer into the live object is read on every step (or on the event the
	// value is about: a respawn, a refill, an animation change).
	constexpr ScopeRow scope_rows[] = {
		// Input readers borrow GameTuning and read thresholds each frame.
		{ "tuning.input.**", TuneScope::live },
		// Copied into the player's rectangle by PlayerBuilder.
		{ "tuning.player.size.*", TuneScope::rebuild },
		// Player::player_tuning() every step; frame_time at each animation change.
		{ "tuning.player.**", TuneScope::live },
		// Level reads the volume each time it plays them.
		{ "tuning.match.sounds.*.volume", TuneScope::live },
		// The opening's and the reveal's beats are read every update (level_intro.cpp).
		{ "tuning.match.opening.**", TuneScope::live },
		{ "tuning.match.reveal.**", TuneScope::live },
		// Timers and texts are copied by the Level constructor.
		{ "tuning.match.**", TuneScope::rebuild },
		// Tiles and their thickness are built with the level.
		{ "tuning.paint.**", TuneScope::rebuild },
		// Weapon reads tuning_ / weapon_tuning_ through pointers (details() per
		// call, ammo regen per step, starting ammo per refill).
		{ "tuning.weapons.**", TuneScope::live },
		// ProjectileBuilder sizes the rectangle when a shot is made.
		{ "tuning.projectiles.*.size.*", TuneScope::new_objects },
		{ "tuning.projectiles.*.collision_size.*", TuneScope::new_objects },
		// ProjectileMist constructs its velocity fan once, when the shot is made.
		{ "tuning.projectiles.mist.spread_step", TuneScope::new_objects },
		// A projectile keeps a pointer to its ProjectileTuning record.
		{ "tuning.projectiles.**", TuneScope::live },
		// MovementFeedback, RespawnRedraw, DebugText and InterfaceGameplay keep pointers;
		// refresh_player_cameras reads the camera block every update.
		{ "presentation.movement_feedback.**", TuneScope::live },
		// WeaponFeedback borrows its numeric presentation record.
		{ "presentation.weapon_feedback.**", TuneScope::live },
		// FigureAttachments borrows the geometry that the numeric tuner can edit.
		{ "presentation.figure.**", TuneScope::live },
		{ "presentation.respawn_redraw.**", TuneScope::live },
		// Monitor, Silhouette and ScreenCursor keep a pointer; Level reads figure_grey.
		{ "presentation.opening.**", TuneScope::live },
		{ "presentation.debug_overlay.**", TuneScope::live },
		{ "presentation.hud.**", TuneScope::live },
		{ "presentation.camera.**", TuneScope::live },
		// Level::refresh_layout returns early unless the window size changed.
		{ "presentation.match.**", TuneScope::rebuild },
		// Menu pages read their look when they are built.
		{ "presentation.**", TuneScope::next_screen },
	};

	std::vector<std::string_view> segments(std::string_view text)
	{
		std::vector<std::string_view> result;
		std::size_t start = 0;
		while (start <= text.size())
		{
			const std::size_t dot = text.find('.', start);
			if (dot == std::string_view::npos)
			{
				result.push_back(text.substr(start));
				break;
			}
			result.push_back(text.substr(start, dot - start));
			start = dot + 1;
		}
		return result;
	}

	bool match_segments(const std::vector<std::string_view>& pattern, std::size_t p,
		const std::vector<std::string_view>& path, std::size_t q)
	{
		if (p == pattern.size())
		{
			return q == path.size();
		}
		if (pattern[p] == "**")
		{
			// At least one segment, so "x.**" never matches "x" itself.
			for (std::size_t next = q + 1; next <= path.size(); ++next)
			{
				if (match_segments(pattern, p + 1, path, next))
				{
					return true;
				}
			}
			return false;
		}
		if (q == path.size())
		{
			return false;
		}
		if (pattern[p] != "*" && pattern[p] != path[q])
		{
			return false;
		}
		return match_segments(pattern, p + 1, path, q + 1);
	}

	std::string shortest(float value)
	{
		char buffer[64] = {};
		const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
		return std::string(buffer, result.ptr);
	}
}

const char* tune_scope_name(TuneScope scope)
{
	switch (scope)
	{
	case TuneScope::live: return "live";
	case TuneScope::new_objects: return "new shots";
	case TuneScope::next_screen: return "next screen";
	case TuneScope::rebuild:
	default: return "rebuild";
	}
}

const char* tune_scope_note(TuneScope scope)
{
	switch (scope)
	{
	case TuneScope::live: return "(live)";
	case TuneScope::new_objects: return "(applies to new shots)";
	case TuneScope::next_screen: return "(applies when the screen is next opened)";
	case TuneScope::rebuild:
	default: return "(applies on rebuild: restart, or add 'restart' to the tune)";
	}
}

bool tune_pattern_matches(std::string_view pattern, std::string_view path)
{
	return match_segments(segments(pattern), 0, segments(path), 0);
}

TuneScope tune_scope_of(std::string_view path)
{
	for (const ScopeRow& row : colour_rows)
	{
		if (path == row.pattern) { return row.scope; }
	}
	for (const ScopeRow& row : scope_rows)
	{
		if (tune_pattern_matches(row.pattern, path))
		{
			return row.scope;
		}
	}
	return TuneScope::rebuild;
}

bool tune_colour_path(std::string_view path)
{
	for (const ScopeRow& row : colour_rows)
	{
		if (path == row.pattern) { return true; }
	}
	return false;
}

std::vector<std::string> tune_scope_patterns()
{
	std::vector<std::string> patterns;
	for (const ScopeRow& row : colour_rows)
	{
		patterns.push_back(row.pattern);
	}
	for (const ScopeRow& row : scope_rows)
	{
		patterns.push_back(row.pattern);
	}
	return patterns;
}

bool tune_integer_token(std::string_view token)
{
	return token.find_first_of(".eE") == std::string_view::npos;
}

double tune_step(double value, bool integer_token)
{
	const double magnitude = std::fabs(value);
	double step = magnitude > 0.0 ?
		std::pow(10.0, std::floor(std::log10(magnitude)) - 1.0) : 0.1;
	if (integer_token)
	{
		step = std::max(1.0, std::round(step));
	}
	return step;
}

std::string tune_format(double value, std::string_view previous)

{
	const bool integral = std::isfinite(value) && std::floor(value) == value &&
		std::fabs(value) < 1e15;
	if (tune_integer_token(previous) && integral)
	{
		char buffer[32] = {};
		const auto result = std::to_chars(buffer, buffer + sizeof(buffer),
			static_cast<long long>(value));
		return std::string(buffer, result.ptr);
	}
	// The readers narrow every number to float: write the float's shortest
	// round-trip form, so what the file says is what the game holds.
	std::string text = shortest(static_cast<float>(value));
	if (text.find_first_of(".eE") == std::string::npos)
	{
		text += ".0";
	}
	return text;
}

std::vector<std::string> tune_children(const std::vector<std::string>& paths,
	std::string_view prefix)
{
	std::vector<std::string> children;
	for (const std::string& path : paths)
	{
		if (path.size() <= prefix.size() || path.compare(0, prefix.size(), prefix) != 0)
		{
			continue;
		}
		const std::string_view rest = std::string_view(path).substr(prefix.size());
		const std::size_t cut = rest.find_first_of(".[");
		const std::string child = cut == std::string_view::npos ? path :
			std::string(prefix) + std::string(rest.substr(0, cut + 1));
		if (std::find(children.begin(), children.end(), child) == children.end())
		{
			children.push_back(child);
		}
	}
	return children;
}
#endif
