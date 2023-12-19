#include "pch.h"
#include "CollisionDetector.h"

////using namespace MattMath;
////using namespace EricsonMath;
//
//bool CollisionDetector::possibly_colliding_with(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	if (this->possibly_colliding_with_type(collider->
//		get_collidable_object_details(),
//		collidee->get_collidable_object_types()))
//	{
//		return this->possibly_colliding_with_aabb(collider, collidee);
//	}
//	return false;
//}
//
//bool CollisionDetector::is_colliding_with(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	// check the shape types of the collider and collidee
//	collidable_object_shape collidee_shape = collidee->get_collidable_object_shape();
//	switch (collider->get_collidable_object_shape())
//	{
//	case collidable_object_shape::RECTANGLE:
//		switch (collidee_shape)
//		{
//		case collidable_object_shape::RECTANGLE:
//			return this->rectangles_colliding(collider, collidee);
//		case collidable_object_shape::RECTANGLE_ROTATED:
//			std::exception("Not implemented");
//		case collidable_object_shape::CIRCLE:
//			return this->rectangle_colliding_with_circle(collider, collidee);
//		case collidable_object_shape::TRIANGLE:
//			return this->rectangle_colliding_with_triangle(collider, collidee);
//		default:
//			return false;
//		}
//	case collidable_object_shape::RECTANGLE_ROTATED:
//		std::exception("Not implemented");
//	case collidable_object_shape::CIRCLE:
//		switch (collidee_shape)
//		{
//		case collidable_object_shape::RECTANGLE:
//			return this->circle_colliding_with_rectangle(collider, collidee);
//		case collidable_object_shape::RECTANGLE_ROTATED:
//			std::exception("Not implemented");
//		case collidable_object_shape::CIRCLE:
//			return this->circles_colliding(collider, collidee);
//		case collidable_object_shape::TRIANGLE:
//			return this->circle_colliding_with_triangle(collider, collidee);
//		default:
//			return false;
//		}
//	case collidable_object_shape::TRIANGLE:
//		switch (collidee_shape)
//		{
//		case collidable_object_shape::RECTANGLE:
//			return this->triangle_colliding_with_rectangle(collider, collidee);
//		case collidable_object_shape::RECTANGLE_ROTATED:
//			std::exception("Not implemented");
//		case collidable_object_shape::CIRCLE:
//			return this->triangle_colliding_with_circle(collider, collidee);
//		case collidable_object_shape::TRIANGLE:
//			return this->triangles_colliding(collider, collidee);
//		default:
//			return false;
//		}
//	default:
//		return false;
//	}
//}
//
//bool CollisionDetector::possibly_colliding_with_type(
//	const std::vector<collidee_object_detail>& collider,
//	const std::vector<collidable_object_type>& collidee) const
//{
//	for (const collidee_object_detail& collider_detail : collider)
//	{
//		for (const collidable_object_type& collidee_type : collidee)
//		{
//			if (collider_detail.colidee_type == collidee_type)
//			{
//				return true;
//			}
//		}
//	}
//	return false;
//}
//
//bool CollisionDetector::possibly_colliding_with_aabb(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	return collider->get_collision_aabb().intersects(
//		collidee->get_collision_aabb());
//}
//bool CollisionDetector::rectangles_colliding(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const IRectangle*>(collider)->get_rectangle().
//			intersects(
//				dynamic_cast<const IRectangle*>(collidee)->get_rectangle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::circles_colliding(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const ICircle*>(collider)->get_circle().
//			intersects(
//				dynamic_cast<const ICircle*>(collidee)->get_circle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::triangles_colliding(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const ITriangle*>(collider)->get_triangle().
//			intersects(
//				dynamic_cast<const ITriangle*>(collidee)->get_triangle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::rectangle_colliding_with_circle(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const IRectangle*>(collider)->get_rectangle().
//			intersects(
//				dynamic_cast<const ICircle*>(collidee)->get_circle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::rectangle_colliding_with_triangle(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const IRectangle*>(collider)->get_rectangle().
//			intersects(
//				dynamic_cast<const ITriangle*>(collidee)->get_triangle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::circle_colliding_with_triangle(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const ICircle*>(collider)->get_circle().
//			intersects(
//				dynamic_cast<const ITriangle*>(collidee)->get_triangle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::circle_colliding_with_rectangle(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const ICircle*>(collider)->get_circle().
//			intersects(
//				dynamic_cast<const IRectangle*>(collidee)->get_rectangle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::triangle_colliding_with_rectangle(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const ITriangle*>(collider)->get_triangle().
//			intersects(
//				dynamic_cast<const IRectangle*>(collidee)->get_rectangle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}
//bool CollisionDetector::triangle_colliding_with_circle(
//	const ICollidableWithOthers* collider,
//	const ICollidable* collidee) const
//{
//	try
//	{
//		return dynamic_cast<const ITriangle*>(collider)->get_triangle().
//			intersects(
//				dynamic_cast<const ICircle*>(collidee)->get_circle());
//	}
//	catch (std::exception& e)
//	{
//		std::cout << e.what() << std::endl;
//		return false;
//	}
//}