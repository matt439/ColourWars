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
		// A projectile keeps a pointer to its ProjectileTuning record.
		{ "tuning.projectiles.**", TuneScope::live },
		// MovementFeedback, RespawnRedraw, DebugText and InterfaceGameplay keep pointers;
		// refresh_player_cameras reads the camera block every update.
		{ "presentation.movement_feedback.**", TuneScope::live },
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
	for (const ScopeRow& row : scope_rows)
	{
		if (tune_pattern_matches(row.pattern, path))
		{
			return row.scope;
		}
	}
	return TuneScope::rebuild;
}

std::vector<std::string> tune_scope_patterns()
{
	std::vector<std::string> patterns;
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
