#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#include <stdexcept>

#if COLOURWARS_DEV_TOOLS
#include <deque>
#include <fstream>
#include <iterator>
#endif

namespace debug_console_acceptance
{
	void require(bool condition, const std::string& message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

#if COLOURWARS_DEV_TOOLS
	bool log_contains(const DebugLog& log, const std::string& text, DebugSeverity severity)
	{
		for (const DebugLogLine& line : log.lines())
		{
			if (line.severity == severity && line.text.find(text) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	const StageList& test_stages()
	{
		static const StageList stages = {
			{ "movement_course", "Movement Course", "icon", false },
			{ "x", "X", "icon", true },
		};
		return stages;
	}

	// A match's settings with `players` seats on stage x (seat 0 keyboard/mouse).
	MenuLevelSettings planned(int players)
	{
		MatchRequest roster;
		roster.stage = "x";
		roster.players = players;
		unsigned int seed = 11;
		const MatchPlan plan = MatchPlanner(&test_stages()).resolve(nullptr, roster,
			MatchPlanner::ConnectedPads{}, [&seed] { return seed++; });
		require(plan.ok, "The test roster resolves: " + plan.error);
		return plan.settings;
	}

	std::size_t row_index(const std::vector<DebugMenuRow>& rows, const std::string& label)
	{
		for (std::size_t i = 0; i < rows.size(); ++i)
		{
			if (rows[i].label == label) { return i; }
		}
		throw std::runtime_error("The debug menu has no row '" + label + "'");
	}

	std::vector<std::string> labels(const std::vector<DebugMenuRow>& rows)
	{
		std::vector<std::string> result;
		for (const DebugMenuRow& row : rows) { result.push_back(row.label); }
		return result;
	}

	std::string read_text(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		require(stream.good(), "Could not read " + path.string());
		return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
	}

	void write_text(const std::filesystem::path& path, const std::string& text)
	{
		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		stream.close();
		require(!stream.fail(), "Could not write " + path.string());
	}

	std::string to_lf(std::string text)
	{
		std::string result;
		for (std::size_t i = 0; i < text.size(); ++i)
		{
			if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
			{
				continue;
			}
			result += text[i];
		}
		return result;
	}

	std::size_t count_of(const std::string& text, const std::string& what)
	{
		std::size_t count = 0;
		for (std::size_t at = text.find(what); at != std::string::npos; at = text.find(what, at + 1))
		{
			++count;
		}
		return count;
	}

	std::size_t log_index(const DebugLog& log, const std::string& text)
	{
		const std::deque<DebugLogLine>& lines = log.lines();
		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			if (lines[i].text.find(text) != std::string::npos) { return i; }
		}
		return std::string::npos;
	}

	bool any_line(const DebugLog& log, const std::string& text)
	{
		return log_index(log, text) != std::string::npos;
	}

	std::vector<labrador::Colour> colour_values(const Presentation& presentation)
	{
		return { presentation.movement_feedback().colour, presentation.movement_feedback().highlight,
			presentation.movement_feedback().keyline, presentation.debug_overlay().colour,
			presentation.debug_overlay().shadow_colour, presentation.debug_overlay().station_label.colour,
			presentation.main_menu().home_background_colour, presentation.main_menu().team_select.selected,
			presentation.main_menu().team_select.unselected, presentation.main_menu().weapon_select.selected,
			presentation.main_menu().weapon_select.unselected, presentation.weapon_feedback().highlight,
			presentation.weapon_feedback().keyline, presentation.player_identity().ink_colour,
			presentation.player_identity().background_colour, presentation.combat_feedback().highlight,
			presentation.combat_feedback().keyline, presentation.blocked_shot().highlight,
			presentation.blocked_shot().keyline, presentation.idle_drip().keyline,
			presentation.respawn_redraw().pen_colour, presentation.opening().cursor_art.ink,
			presentation.opening().cursor_art.fill, presentation.opening().silhouette_art.ink,
			presentation.opening().silhouette_art.rim, presentation.opening().silhouette_art.shade,
			presentation.opening().reveal_art.ink, presentation.opening().reveal_art.paper,
			presentation.opening().reveal_art.shade, presentation.opening().cursor_art.shade,
			presentation.opening().cursor_art.shadow, presentation.opening().desktop_colour,
			presentation.opening().monitor_art.power_light, presentation.opening().program_colour,
			presentation.opening().face, presentation.opening().sweat, presentation.opening().glass_colour, presentation.opening().silhouette, presentation.opening().scribble, presentation.hud().health_colour, presentation.hud().meter_track_colour, presentation.opening().crack, presentation.figure().arm_colour, presentation.figure().hand_colour, presentation.figure().quiet.face_colour, presentation.hud().timer_colour, presentation.hud().timer_shadow_colour };
	}
#endif
}
