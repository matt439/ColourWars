#ifndef  COLLISION_TOOLS_H
#define COLLISION_TOOLS_H

#include "MattMath.h"
#include "shape_type.h"
#include "CollisionDirection.h"

constexpr int BRACKET_ITERATIONS = 40;
constexpr float ITERATION_POWER = 1.5f;

enum class shape_movement_direction
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    UP_LEFT,
    UP_RIGHT,
    DOWN_LEFT,
    DOWN_RIGHT,
    NONE,
};

class CollisionTools
{
public:
	CollisionTools() = default;
    static collision_direction calculate_object_collision_direction(
        const MattMath::Shape* collider,
        const MattMath::Shape* collidee);

    static bool resolve_object_collision(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
		collision_direction direction, MattMath::Vector2F& amount);

    static bool resolve_object_collision(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        collision_direction direction);

	static MattMath::Vector2F calculate_object_collision_depth(
		const MattMath::Shape* collider,
		const MattMath::Shape* collidee,
		collision_direction direction);

private:
    static shape_movement_direction opposite_direction(shape_movement_direction direction);
    static shape_movement_direction direction_from_collision(collision_direction direction);

    static void move_object_by_direction(MattMath::Shape* obj,
        shape_movement_direction direction, const MattMath::Vector2F& amount);

    static void move_object_by_direction_relative_to_size(MattMath::Shape* obj,
        shape_movement_direction direction, float relative_amount = 1.0f);

    static bool bracket_object_collision(bool colliding, int i, MattMath::Shape* collider,
        shape_movement_direction collider_direction);

    static bool bracket_object_collision_generic(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        shape_movement_direction collider_direction, int iterations);

    static collision_direction compare_point_collision_depth_horizontal(
        const MattMath::Point2F& collider, const MattMath::Point2F& collidee);

    static collision_direction compare_point_collision_depth_vertical(
        const MattMath::Point2F& collider, const MattMath::Point2F& collidee);

    static collision_direction calculate_containing_collision_direction(
        const MattMath::RectangleF& collider, const MattMath::RectangleF& collidee);

    static collision_direction rectangle_rectangle_collision_direction(
        const MattMath::RectangleF& collider, const MattMath::RectangleF& collidee);

    static void resolve_object_AABB_collision(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        collision_direction direction);
};

#endif // ! COLLISION_TOOLS_H
