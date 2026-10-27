#include "game/pch.h"
#include "game/ai/match_bot_ballistics.h"
#include <cmath>
#include <filesystem>
#include <iostream>

namespace
{
	void require(bool condition, const char* message)
	{
		if (!condition) { throw std::runtime_error(message); }
	}

	void require_near(float actual, float expected, float tolerance, const char* message)
	{
		require(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance, message);
	}

	void check_shipped_tools(const GameTuning& tuning)
	{
		for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper,
			WeaponType::roller, WeaponType::mister, WeaponType::bucket })
		{
			const WeaponTuning& tool = tuning.weapon(type);
			const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
			const float reach = match_bot_ballistics::horizontal_reach(tool, projectile);
			require(std::isfinite(reach) && reach >= 0.0f,
				"A shipped tool has invalid bot projectile reach");
			if (type == WeaponType::sprayer || type == WeaponType::bucket)
			{
				// Current neutral launch speeds/gravity imply 484 and 256 units.
				// The old one-second heuristic allowed these tools to stop firing
				// from beyond even their ideal maximum range.
				require_near(reach, type == WeaponType::sprayer ? 484.0f : 256.0f, 0.01f,
					"A gravity tool retained an impossible one-second horizontal range");
			}
			else if (type == WeaponType::sniper)
			{
				require_near(reach, 2000.0f, 0.1f, "Sniper's no-gravity local range changed");
			}
			else if (type == WeaponType::mister)
			{
				require_near(reach, 300.0f, 0.02f, "Mister's no-gravity local range changed");
			}
			else
			{
				require_near(reach, 0.0f, 0.0f,
					"Roller's stationary contact volume gained projectile travel");
			}
		}
	}

	void check_limits(const GameTuning& tuning)
	{
		WeaponTuning tool = tuning.weapon(WeaponType::sprayer);
		ProjectileTuning projectile = tuning.projectile(tool.details.proj_type);
		tool.details.starting_vel_length = 600.0f;
		projectile.details.gravity = 1000.0f;
		projectile.details.wind_resistance = 0.0f;
		projectile.details.delete_timer = 5.0f;
		require_near(match_bot_ballistics::horizontal_reach(tool, projectile), 360.0f, 0.01f,
			"Gravity did not bound ideal same-height range");
		tool.details.starting_vel_length = -600.0f;
		require_near(match_bot_ballistics::horizontal_reach(tool, projectile), 360.0f, 0.01f,
			"A reversed launch changed the magnitude of ballistic reach");
		projectile.details.gravity = -1000.0f;
		require_near(match_bot_ballistics::horizontal_reach(tool, projectile), 360.0f, 0.01f,
			"Upward gravity changed the magnitude of same-height ballistic reach");
		tool.details.starting_vel_length = 600.0f;
		projectile.details.gravity = 1000.0f;
		projectile.details.delete_timer = 0.1f;
		const float short_lived = match_bot_ballistics::horizontal_reach(tool, projectile);
		require(short_lived >= 49.0f && short_lived <= 60.1f,
			"A short lifetime was ignored by the gravity range cap");
		projectile.details.delete_timer = 5.0f;
		projectile.details.wind_resistance = 1000.0f;
		const float drag_limited = match_bot_ballistics::horizontal_reach(tool, projectile);
		require(drag_limited < short_lived,
			"Wind snap-to-zero was ignored by the gravity range cap");
		tool.details.starting_vel_length = 0.0f;
		require_near(match_bot_ballistics::horizontal_reach(tool, projectile), 0.0f, 0.0f,
			"A zero-speed gravity projectile gained travel");
	}
}

int wmain(int argc, wchar_t** argv)
{
	try
	{
		require(argc == 2, "Usage: bot_ballistics <staged-content-directory>");
		const std::filesystem::path content(argv[1]);
		const GameTuning tuning((content / "tuning.json").string().c_str());
		check_shipped_tools(tuning); check_limits(tuning);
		std::cout << "PASS bot projectile reach: shipped five tools, gravity, reversed launch, lifetime, drag and stationary contact volume\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL " << error.what() << '\n'; return 1;
	}
}
