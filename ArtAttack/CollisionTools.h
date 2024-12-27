#ifndef  COLLISION_TOOLS_H
#define COLLISION_TOOLS_H

#include "MattMath.h"
#include "shape_type.h"

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

/**
     *  This enumeration contains a list of directions that an
     *  object can collide with another object. For example, a
     *  collider object which is colliding with another object
     *  on its top edge would have a collision direction of TOP.
     *
     *  @constant TOP           The top of the object
     *  @constant BOTTOM        The bottom of the object
     *  @constant LEFT          The left of the object
     *  @constant RIGHT         The right of the object
     *  @constant TOP_LEFT      The top left of the object
     *  @constant TOP_RIGHT     The top right of the object
     *  @constant BOTTOM_LEFT   The bottom left of the object
     *  @constant BOTTOM_RIGHT  The bottom right of the object
     *  @constant NONE          No collision
     */
enum collision_direction
{
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    NONE,
};

class CollisionTools
{
public:
	CollisionTools() = default;
    collision_direction calculate_object_collision_direction(const MattMath::Shape* collider,
        const MattMath::Shape* collidee);

    bool resolve_object_collision(MattMath::Shape* collider, const MattMath::Shape* collidee,
        collision_direction direction);

private:
    shape_movement_direction opposite_direction(shape_movement_direction direction);
    shape_movement_direction direction_from_collision(collision_direction direction);

    void move_object_by_direction(MattMath::Shape* obj,
        shape_movement_direction direction, const MattMath::Vector2F& amount);

    void move_object_by_direction_relative_to_size(MattMath::Shape* obj,
        shape_movement_direction direction, float relative_amount = 1.0f);

    bool bracket_object_collision(bool colliding, int i, MattMath::Shape* collider,
        shape_movement_direction collider_direction);

    bool bracket_object_collision_generic(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        shape_movement_direction collider_direction, int iterations);

    collision_direction compare_point_collision_depth_horizontal(
        const MattMath::Point2F& collider, const MattMath::Point2F& collidee);

    collision_direction compare_point_collision_depth_vertical(
        const MattMath::Point2F& collider, const MattMath::Point2F& collidee);

    collision_direction calculate_containing_collision_direction(
        const MattMath::RectangleF& collider, const MattMath::RectangleF& collidee);

    collision_direction rectangle_rectangle_collision_direction(
        const MattMath::RectangleF& collider, const MattMath::RectangleF& collidee);

    void resolve_object_AABB_collision(MattMath::Shape* collider,
        const MattMath::Shape* collidee,
        collision_direction direction);
};

#endif // ! COLLISION_TOOLS_H
