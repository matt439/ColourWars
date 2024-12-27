#include "pch.h"
#include "CollisionTools.h"

using namespace MattMath;

sprite_movement_direction CollisionTools::opposite_direction(
    sprite_movement_direction direction)
{
    switch (direction)
    {
    case sprite_movement_direction::UP:
        return sprite_movement_direction::DOWN;
    case sprite_movement_direction::DOWN:
        return sprite_movement_direction::UP;
    case sprite_movement_direction::LEFT:
        return sprite_movement_direction::RIGHT;
    case sprite_movement_direction::RIGHT:
        return sprite_movement_direction::LEFT;
    case sprite_movement_direction::UP_LEFT:
        return sprite_movement_direction::DOWN_RIGHT;
    case sprite_movement_direction::UP_RIGHT:
        return sprite_movement_direction::DOWN_LEFT;
    case sprite_movement_direction::DOWN_LEFT:
        return sprite_movement_direction::UP_RIGHT;
    case sprite_movement_direction::DOWN_RIGHT:
        return sprite_movement_direction::UP_LEFT;
    };
    return sprite_movement_direction::NONE;
}

sprite_movement_direction CollisionTools::direction_from_collision(
    collision_direction direction)
{
    switch (direction)
    {
    case collision_direction::TOP:
        return sprite_movement_direction::DOWN;
    case collision_direction::BOTTOM:
        return sprite_movement_direction::UP;
    case collision_direction::LEFT:
        return sprite_movement_direction::RIGHT;
    case collision_direction::RIGHT:
        return sprite_movement_direction::LEFT;
    case collision_direction::TOP_LEFT:
        return sprite_movement_direction::DOWN_RIGHT;
    case collision_direction::TOP_RIGHT:
        return sprite_movement_direction::DOWN_LEFT;
    case collision_direction::BOTTOM_LEFT:
        return sprite_movement_direction::UP_RIGHT;
    case collision_direction::BOTTOM_RIGHT:
        return sprite_movement_direction::UP_LEFT;
    };
    return sprite_movement_direction::NONE;
}

void CollisionTools::move_object_by_direction(Shape* obj,
    sprite_movement_direction direction, const Vector2F& amount)
{
    if (amount.x == 0.0f && amount.y == 0.0f)
    {
        return;
    }

    switch (direction)
    {
    case sprite_movement_direction::UP:
        obj->offset(Vector2F(0.0f, -amount.y));
        break;
    case sprite_movement_direction::DOWN:
        obj->offset(Vector2F(0.0f, amount.y));
        break;
    case sprite_movement_direction::LEFT:
        obj->offset(Vector2F(-amount.x, 0.0f));
        break;
    case sprite_movement_direction::RIGHT:
        obj->offset(Vector2F(amount.x, 0.0f));
        break;
    case sprite_movement_direction::UP_LEFT:
        obj->offset(Vector2F(-amount.x, -amount.y));
        break;
    case sprite_movement_direction::UP_RIGHT:
        obj->offset(Vector2F(amount.x, -amount.y));
        break;
    case sprite_movement_direction::DOWN_LEFT:
        obj->offset(Vector2F(-amount.x, amount.y));
        break;
    default: // sprite_movement_direction::DOWN_RIGHT:
        obj->offset(Vector2F(amount.x, amount.y));
        break;
    };
}

void CollisionTools::move_object_by_direction_relative_to_size(Shape* obj,
    sprite_movement_direction direction, float relative_amount)
{
    RectangleF obj_aabb = obj->get_bounding_box();

    double relative_width = obj_aabb.width * relative_amount;
    double relative_height = obj_aabb.height * relative_amount;

    switch (direction)
    {
    case sprite_movement_direction::UP:
        move_object_by_direction(obj, sprite_movement_direction::UP,
            Vector2F(0.0f, relative_height));
        break;
    case sprite_movement_direction::DOWN:
        move_object_by_direction(obj, sprite_movement_direction::DOWN,
            Vector2F(0.0f, relative_height));
        break;
    case sprite_movement_direction::LEFT:
        move_object_by_direction(obj, sprite_movement_direction::LEFT,
            Vector2F(relative_width, 0.0f));
        break;
    case sprite_movement_direction::RIGHT:
        move_object_by_direction(obj, sprite_movement_direction::RIGHT,
            Vector2F(relative_width, 0.0f));
        break;
    case sprite_movement_direction::UP_LEFT:
        move_object_by_direction(obj, sprite_movement_direction::UP_LEFT,
            Vector2F(relative_width, relative_height));
        break;
    case sprite_movement_direction::UP_RIGHT:
        move_object_by_direction(obj, sprite_movement_direction::UP_RIGHT,
            Vector2F(relative_width, relative_height));
        break;
    case sprite_movement_direction::DOWN_LEFT:
        move_object_by_direction(obj, sprite_movement_direction::DOWN_LEFT,
            Vector2F(relative_width, relative_height));
        break;
    default: // sprite_movement_direction::DOWN_RIGHT:
        move_object_by_direction(obj, sprite_movement_direction::DOWN_RIGHT,
            Vector2F(relative_width, relative_height));
        break;
    };
}

/**
 * Moves the object back and forth with decreasing step size for
 * the given number of iterations.
*/
bool CollisionTools::bracket_object_collision(bool colliding, int i, Shape* collider,
    sprite_movement_direction collider_direction)
{
    RectangleF collider_aabb = collider->get_bounding_box();

    if (colliding)
    {
        move_object_by_direction_relative_to_size(collider, collider_direction,
            1.0f / pow(ITERATION_POWER, static_cast<float>(i)));
    }
    else if (i == 1) // no collision in the first iteration
    {
        return false;
    }
    else
    {
        move_object_by_direction_relative_to_size(collider,
            opposite_direction(collider_direction),
            1.0f / pow(ITERATION_POWER, static_cast<float>(i)));
    }
    return true;
}

bool CollisionTools::bracket_object_collision_generic(Shape* collider, const Shape* collidee,
    sprite_movement_direction collider_direction, int iterations)
{
    RectangleF collider_aabb = collider->get_bounding_box();

    for (int i = 1; i <= iterations; i++)
    {
        if (!bracket_object_collision(collider->intersects(collidee),
            i, collider, collider_direction))
        {
            return false;
        }
    }
    return true;
}

collision_direction CollisionTools::compare_point_collision_depth_horizontal(
    const Point2F& collider, const Point2F& collidee)
{
    if (collider.x < collidee.x)
    {
        return collision_direction::RIGHT;
    }
    return collision_direction::LEFT;
}

collision_direction CollisionTools::compare_point_collision_depth_vertical(
    const Point2F& collider, const Point2F& collidee)
{
    if (collider.y < collidee.y)
    {
        return collision_direction::BOTTOM;
    }
    return collision_direction::TOP;
}

collision_direction CollisionTools::calculate_containing_collision_direction(
    const RectangleF& collider, const RectangleF& collidee)
{
    // calculate the direction of the greatest distance between the two sprites
	Point2F collider_center = collider.get_center();
	Point2F collidee_center = collidee.get_center();

    float x_distance = collider_center.x - collidee_center.x;
    if (x_distance < 0.0f)
    {
        x_distance *= -1.0f;
    }

    float y_distance = collider_center.y - collidee_center.y;
    if (y_distance < 0.0f)
    {
        y_distance *= -1.0f;
    }

    if (x_distance < y_distance)
    {
        return compare_point_collision_depth_horizontal(collider_center, collidee_center);
    }
    return compare_point_collision_depth_vertical(collider_center, collidee_center);
}

collision_direction CollisionTools::rectangle_rectangle_collision_direction(
    const RectangleF& collider, const RectangleF& collidee)
{
    Segment collider_top_edge = collider.get_top_edge();
	Segment collider_left_edge = collider.get_left_edge();
	Segment collider_right_edge = collider.get_right_edge();
	Segment collider_bottom_edge = collider.get_bottom_edge();

	bool left_edge = collidee.intersects(collider_left_edge);
	bool right_edge = collidee.intersects(collider_right_edge);
	bool top_edge = collidee.intersects(collider_top_edge);
	bool bottom_edge = collidee.intersects(collider_bottom_edge);

    if (left_edge && right_edge && top_edge && bottom_edge) // collidee is equal size of collider
    {
        return calculate_containing_collision_direction(collider, collidee);
    }
    if ((left_edge && right_edge && top_edge) || (top_edge &&
        !(left_edge || right_edge || bottom_edge)))
    {
        return collision_direction::TOP;
    }
    if ((left_edge && right_edge && bottom_edge) || (bottom_edge &&
        !(left_edge || right_edge || top_edge)))
    {
        return collision_direction::BOTTOM;
    }
    if ((top_edge && bottom_edge && right_edge) || (right_edge &&
        !(left_edge || top_edge || bottom_edge)))
    {
        return collision_direction::RIGHT;
    }
    if ((top_edge && bottom_edge && left_edge) || (left_edge &&
        !(right_edge || top_edge || bottom_edge)))
    {
        return collision_direction::LEFT;
    }
    if (left_edge && right_edge)
    {
        // check if the collider is more to the left or right of the collidee
        return compare_point_collision_depth_horizontal(collider.get_center(),
            collidee.get_center());
    }
    if (top_edge && bottom_edge)
    {
        // check if the collider is more to the top or bottom of the collidee
        return compare_point_collision_depth_vertical(collider.get_center(),
            collidee.get_center());
    }
    if (left_edge && top_edge)
    {
        return collision_direction::TOP_LEFT;
    }
    if (left_edge && bottom_edge)
    {
        return collision_direction::BOTTOM_LEFT;
    }
    if (right_edge && top_edge)
    {
        return collision_direction::TOP_RIGHT;
    }
    if (right_edge && bottom_edge)
    {
        return collision_direction::BOTTOM_RIGHT;
    }

    // collider contains collidee or collidee contains collider
    return calculate_containing_collision_direction(collider, collidee);
}

collision_direction CollisionTools::calculate_object_collision_direction(const Shape* collider,
    const Shape* collidee)
{
    if (!collider->intersects(collidee))
    {
        return collision_direction::NONE;
    }

    return rectangle_rectangle_collision_direction(collider->get_bounding_box(),
        collidee->get_bounding_box());
}

void CollisionTools::resolve_object_AABB_collision(Shape* collider,
    const Shape* collidee,
    collision_direction direction)
{
    // get the intersection rectangle
    RectangleF inter = RectangleF::intersection(collider->get_bounding_box(),
        collidee->get_bounding_box());

    Vector2F amount = { inter.width, inter.height };

    move_object_by_direction(collider, direction_from_collision(direction), amount);
}

bool CollisionTools::resolve_object_collision(Shape* collider, const Shape* collidee,
    collision_direction direction)
{
    // check if the sprites are colliding
    if (direction == collision_direction::NONE || !collider->intersects(collidee))
    {
        return false;
    }

    if (collider->get_shape_type() == shape_type::RECTANGLE &&
        collidee->get_shape_type() == shape_type::RECTANGLE)
    {
        resolve_object_AABB_collision(collider, collidee, direction);
    }
    else // one or both of the sprites are using pixel collision
    {
        bracket_object_collision_generic(collider, collidee,
            direction_from_collision(direction), BRACKET_ITERATIONS);
    }

    return true;
}