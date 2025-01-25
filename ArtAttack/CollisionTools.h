#ifndef  COLLISION_TOOLS_H
#define COLLISION_TOOLS_H

#include "MattMath.h"
#include "shape_type.h"

constexpr int BRACKET_ITERATIONS = 40;
constexpr float ITERATION_POWER = 1.5f;

class CollisionTools
{
public:
	CollisionTools() = default;
    static MattMath::Vector2F calculate_object_collision_direction(
        const MattMath::Shape* collider,
        const MattMath::Shape* collidee);

    static bool resolve_object_collision(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        const MattMath::Vector2F& collision_direction, MattMath::Vector2F& amount);

    static bool resolve_object_collision(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        const MattMath::Vector2F& collision_direction);

	static MattMath::Vector2F calculate_object_collision_depth(
		const MattMath::Shape* collider,
		const MattMath::Shape* collidee,
        const MattMath::Vector2F& collision_direction);

private:
    static MattMath::Vector2F opposite_direction(const MattMath::Vector2F& direction);

    static void move_object_by_direction_relative_to_size(MattMath::Shape* obj,
        const MattMath::Vector2F& movement_direction, float relative_amount = 1.0f);

    static bool bracket_object_collision(bool colliding, int i, MattMath::Shape* collider,
        const MattMath::Vector2F& collider_direction);

    static bool bracket_object_collision_generic(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        const MattMath::Vector2F& collider_direction, int iterations);

    static MattMath::Vector2F compare_point_collision_depth_horizontal(
        const MattMath::Point2F& collider, const MattMath::Point2F& collidee);

    static MattMath::Vector2F compare_point_collision_depth_vertical(
        const MattMath::Point2F& collider, const MattMath::Point2F& collidee);

    static MattMath::Vector2F calculate_containing_collision_direction(
        const MattMath::Shape* collider, const MattMath::Shape* collidee);

    static MattMath::Vector2F shape_shape_collision_direction(
        const MattMath::Shape* collider, const MattMath::Shape* collidee);

    static void resolve_object_AABB_collision(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        const MattMath::Vector2F& collision_direction);
};

#endif // ! COLLISION_TOOLS_H
