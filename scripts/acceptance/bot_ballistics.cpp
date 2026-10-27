#include "game/pch.h"
#include "game/ai/match_bot_ballistics.h"
#include "scripts/acceptance/paint_surfaces/fixtures.h"
#include "game/objects/weapon_builder.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>

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

	void check_actual_flight(paint_surfaces_acceptance::World& world)
	{
		using namespace mattmath;
		const float dt = static_cast<float>(labrador::StepTimer::TicksToSeconds(
			labrador::StepTimer::TicksPerSecond / 60));
		int contacts = 0;
		for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper,
			WeaponType::mister, WeaponType::bucket })
		{
			for (const float side : { -1.0f, 1.0f })
			{
				for (const bool moving : { false, true })
				{
					const WeaponTuning& tool = world.tuning.weapon(type);
					const ProjectileTuning& projectile = world.tuning.projectile(tool.details.proj_type);
					const Vector2F shooter_velocity(side * (moving ? 120.0f : 0.0f), 0.0f);
					const Vector2F target(side * 280.0f, -35.0f);
					const Vector2F target_velocity(side * (moving ? 80.0f : 0.0f), moving ? -80.0f : 0.0f);
					const Vector2F aim = match_bot_ballistics::aim_direction(tool, projectile,
						shooter_velocity, target, target_velocity);
					require_near(aim.length(), 1.0f, 0.001f, "Bot flight search returned a non-unit aim");
					std::unique_ptr<Weapon> weapon = WeaponBuilder::build_weapon(&world.tuning, PlayerTeam::a,
						0, 0, labrador::Colour::white, type, {}, &world.render, &world.audio, &world.preferences);
					PlayerInputData input;
					input.primary_shoot = input.shoot_direction_requested = true;
					input.shoot_direction = aim; input.shoot_angle = aim.angle();
					std::vector<std::unique_ptr<Projectile>> shots;
					// Advance only the weapon's normal cooldown until it buys its first shot.
					for (int tick = 0; tick < 120 && shots.empty(); ++tick)
					{ shots = weapon->update_and_get_projectiles(input, {}, shooter_velocity, side > 0.0f, dt); }
					require(!shots.empty() && weapon->ammo() < world.tuning.starting_ammo(),
						"Actual bot aim flight did not buy an ordinary shot");
					// Mist is emitted center-out; the first pellet is its actual centerline.
					Projectile& shot = *shots.front();
					Vector2F previous = shot.shape()->bounding_box().center() - target;
					float closest = (std::numeric_limits<float>::max)();
					for (int tick = 0; tick < 60; ++tick)
					{
						shot.update(dt);
						if (shot.for_deletion()) { break; }
						const Vector2F current = shot.shape()->bounding_box().center() - target -
							target_velocity * (dt * static_cast<float>(tick + 1));
						const Vector2F delta = current - previous;
						const float along = delta.length_squared() > 0.0f ?
							std::clamp(-previous.dot(delta) / delta.length_squared(), 0.0f, 1.0f) : 0.0f;
						closest = std::min(closest, (previous + delta * along).length());
						previous = current;
					}
					std::cout << "MEASURE actual centerline " << static_cast<int>(type) << '/' << side << '/' << moving <<
						" miss=" << closest << '\n';
					require(closest < 2.0f, "Bot local aim missed a reachable stationary/moving target with the actual weapon projectile");
					++contacts;
				}
			}
		}
		require(contacts == 16, "Actual projectile aim matrix lost a mirrored/tool/motion case");
		const WeaponTuning& roller = world.tuning.weapon(WeaponType::roller);
		require(match_bot_ballistics::aim_direction(roller, world.tuning.projectile(roller.details.proj_type),
			{}, {}, {}).x == 1.0f, "Stationary contact aim did not handle coincident centers");
	}

	void check_finite_fallbacks(const GameTuning& tuning)
	{
		using namespace mattmath;
		for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
			WeaponType::mister, WeaponType::bucket })
		{
			const WeaponTuning& tool = tuning.weapon(type);
			const ProjectileTuning& projectile = tuning.projectile(tool.details.proj_type);
			for (const Vector2F target : { Vector2F::ZERO, Vector2F(1.0f, 0.0f),
				Vector2F(-1.0f, 0.0f), Vector2F(0.0f, -300.0f), Vector2F(0.0f, 300.0f),
				Vector2F(100000.0f, -100000.0f) })
			{
				const Vector2F aim = match_bot_ballistics::aim_direction(tool, projectile,
					{ -1200.0f, 500.0f }, target, { 2000.0f, -1000.0f });
				require_near(aim.length(), 1.0f, 0.001f,
					"Close, vertical, unreachable or coincident target produced invalid bot aim");
			}
		}
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
		check_finite_fallbacks(tuning);
		paint_surfaces_acceptance::World world(content);
		check_actual_flight(world);
		std::cout << "PASS bot projectile reach and 16 actual paid-shot centerlines: five tools, gravity, reversed launch, lifetime, drag, stationary contact volume, muzzle and inherited/target motion\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAIL " << error.what() << '\n'; return 1;
	}
}
