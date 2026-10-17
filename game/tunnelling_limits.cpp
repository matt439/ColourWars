#include "game/pch.h"
#include "game/tunnelling_limits.h"
#include "engine/collision/collision_layer.h"
#include "engine/collision/tunnelling.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/level_object_builder.h"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

using namespace labrador;
using namespace mattmath;

namespace
{
	// The exemption in projectile_type.h, held to the code that relies on it:
	// a kind that is neither swept nor the stationary roller has no bound
	// below, so it must not compile until it is given one.
	constexpr bool every_unswept_kind_is_bounded()
	{
		for (int index = 0; index <= static_cast<int>(ProjectileType::ball); ++index)
		{
			const ProjectileType type = static_cast<ProjectileType>(index);
			if (!is_swept(type) && type != ProjectileType::rolling)
			{
				return false;
			}
		}
		return true;
	}
	static_assert(every_unswept_kind_is_bounded(),
		"A projectile kind is neither swept by Projectile::resolve_sweep nor the "
		"stationary roller; give it a tunnelling bound in check_stage_tunnelling.");

	// Something moved by endpoint sampling: its own extent, its farthest travel
	// in one step on each axis, and what it collides with.
	struct DiscreteMover
	{
		std::string name;
		std::string speed_key;
		Vector2F extent;
		Vector2F travel;
		CollisionLayer layer = 0u;
		CollisionMask mask = 0u;
	};

	std::string number(float value)
	{
		std::ostringstream text;
		text << value;
		return text.str();
	}

	std::vector<DiscreteMover> discrete_movers(const GameTuning& tuning,
		float seconds_per_step)
	{
		const PlayerTuning& player = tuning.player();
		// Team B's mask differs from team A's only in which paint it meets.
		const CollisionObjectType player_type = CollisionObjectType::player_team_a;
		// The roller paints one sweep from where it spawned and never moves.
		const CollisionObjectType roller_type = CollisionObjectType::projectile_rolling_team_a;
		return {
			{ "a player", "player.max_velocity", player.size,
				Vector2F(std::abs(player.max_velocity.x), std::abs(player.max_velocity.y)) *
					seconds_per_step,
				collision_layer(player_type), collision_mask(player_type) },
			{ "a rolling projectile", "projectiles.max_velocity",
				tuning.projectile(ProjectileType::rolling).details.col_rect_size, Vector2F::ZERO,
				collision_layer(roller_type), collision_mask(roller_type) },
		};
	}

	void check_axis(const JsonValue& entry, const DiscreteMover& mover,
		float seconds_per_step, const char* axis, const char* dimension,
		const char* component, float travel, float mover_extent, float object_extent)
	{
		if (!can_tunnel(travel, mover_extent, object_extent))
		{
			return;
		}
		throw std::runtime_error(entry.where() + ": " + mover.name +
			" at its speed cap travels " + number(travel) + " units " + axis +
			" in one " + number(seconds_per_step) + " s step, and against this object's " +
			dimension + " of " + number(object_extent) + " the engine's discrete bound "
			"(labrador::max_safe_displacement) is " +
			number(max_safe_displacement(mover_extent, object_extent)) +
			", so it could pass through between two steps. Lower " + mover.speed_key +
			"." + component + " or thicken the object.");
	}
}

void check_stage_tunnelling(const GameTuning& tuning,
	const LevelObjectBuilder& builder, const JsonValue& collision_objects,
	float seconds_per_step)
{
	const std::vector<DiscreteMover> movers = discrete_movers(tuning, seconds_per_step);
	const std::vector<std::unique_ptr<CollisionObject>> objects =
		builder.build_collision_objects(collision_objects, TeamColour{});
	for (size_t index = 0; index < objects.size(); ++index)
	{
		const CollisionObject& object = *objects[index];
		const JsonValue entry = collision_objects.at(index);
		const RectangleF box = object.shape()->bounding_box();
		for (const DiscreteMover& mover : movers)
		{
			if (!layers_collide(mover.layer, mover.mask, object.layer(), object.mask()))
			{
				continue;
			}
			check_axis(entry, mover, seconds_per_step, "horizontally", "width", "x",
				mover.travel.x, mover.extent.x, box.width);
			check_axis(entry, mover, seconds_per_step, "vertically", "height", "y",
				mover.travel.y, mover.extent.y, box.height);
		}
	}
}
