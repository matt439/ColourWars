#include "game/pch.h"
#include "game/tunnelling_limits.h"
#include "engine/collision/collision_layer.h"
#include "engine/collision/tunnelling.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/level_object_builder.h"
#include <algorithm>
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
		for (int index = 0; index < static_cast<int>(ProjectileType::count); ++index)
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
	// in one step on each axis, the tuning keys that travel comes from, and
	// what it collides with.
	struct DiscreteMover
	{
		std::string name;
		std::string speed_keys_x;
		std::string speed_keys_y;
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
		// Player::update clamps to max_velocity on each axis, with two ways
		// past it: the first step of input sets x to x_initial_velocity
		// unclamped, and a drop-through adds drop_down_velocity to y after
		// the clamp. Its farthest step on each axis is the larger of the
		// first pair and the sum of the second.
		const Vector2F player_speed(
			std::max(std::abs(player.max_velocity.x), std::abs(player.x_initial_velocity)),
			std::abs(player.max_velocity.y) + std::abs(player.drop_down_velocity));
		return {
			{ "a player",
				"player.max_velocity.x (or player.x_initial_velocity, if that is larger)",
				"player.max_velocity.y (or player.drop_down_velocity, which a drop-through "
					"adds to it)",
				player.size, player_speed * seconds_per_step,
				collision_layer(player_type), collision_mask(player_type) },
			{ "a rolling projectile", "projectiles.max_velocity.x", "projectiles.max_velocity.y",
				tuning.projectile(ProjectileType::rolling).details.col_rect_size, Vector2F::ZERO,
				collision_layer(roller_type), collision_mask(roller_type) },
		};
	}

	void check_axis(const JsonValue& entry, const DiscreteMover& mover,
		float seconds_per_step, const char* axis, const char* dimension,
		const std::string& speed_keys, float travel, float mover_extent, float object_extent)
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
			", so it could pass through between two steps. Lower " + speed_keys +
			" or thicken the object.");
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
			check_axis(entry, mover, seconds_per_step, "horizontally", "width",
				mover.speed_keys_x, mover.travel.x, mover.extent.x, box.width);
			check_axis(entry, mover, seconds_per_step, "vertically", "height",
				mover.speed_keys_y, mover.travel.y, mover.extent.y, box.height);
		}
	}
}
