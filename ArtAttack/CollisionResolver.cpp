#include "pch.h"
#include "CollisionResolver.h"

//using namespace MattMath;
//
//CollisionResolver::CollisionResolver()
//{
//	this->_collision_detector = std::make_unique<CollisionDetector>();
//	this->_collision_quantifier = std::make_unique<CollisionQuantifier>();
//}
//void CollisionResolver::resolve_collisions(
//	std::vector<std::unique_ptr<ICollidableWithOthers>>* colliders,
//	std::vector<std::unique_ptr<ICollidable>>* collidables) const
//{
//	this->sort_colliders(colliders);
//
//	for (size_t i = 0; i < colliders->size(); i++)
//	{
//		ICollidableWithOthers* collider = colliders->at(i).get();
//		std::vector<ICollidable*> collidables_to_check;
//		for (size_t j = i + 1; j < colliders->size(); j++)
//		{
//			//if (i == j)
//			//{
//			//	continue;
//			//}
//			if (this->_collision_detector->possibly_colliding_with(
//				collider, colliders->at(j).get()))
//			{
//				collidables_to_check.push_back(colliders->at(j).get());
//			}
//		}
//		for (size_t j = 0; j < collidables->size(); j++)
//		{
//			if (this->_collision_detector->possibly_colliding_with(
//				collider, collidables->at(j).get()))
//			{
//				collidables_to_check.push_back(collidables->at(j).get());
//			}
//		}
//		std::vector<ICollidable*> confirmed_colliding;
//		for (size_t j = 0; j < collidables_to_check.size(); j++)
//		{
//			if (this->_collision_detector->is_colliding_with(
//				collider, collidables_to_check[j]))
//			{
//				confirmed_colliding.push_back(collidables_to_check[j]);
//			}
//		}
//
//		std::vector<Vector2F> intersection_depths;
//		intersection_depths = this->_collision_quantifier->
//			get_intersection_depths(
//				collider, confirmed_colliding);
//
//		this->sort_collidables(confirmed_colliding, intersection_depths);
//
//		for (size_t j = 0; j < confirmed_colliding.size(); j++)
//		{
//			if (j == 0)
//			{
//				this->resolve_collision(collider,
//					confirmed_colliding[j]);
//				continue;
//			}
//			if (this->_collision_detector->possibly_colliding_with(
//				collider, confirmed_colliding[j]))
//			{
//				if (this->_collision_detector->is_colliding_with(
//					collider, confirmed_colliding[j]))
//				{
//					this->resolve_collision(collider,
//						confirmed_colliding[j]);
//				}
//			}
//		}
//	}
//}
//
//void CollisionResolver::resolve_collision(ICollidableWithOthers* collider,
//	ICollidable* collidee) const
//{
//	const std::vector<collidable_object_type>& collidee_types =
//		collidee->get_collidable_object_types();
//
//	const std::vector<collidee_object_detail>& collider_details =
//		collider->get_collidable_object_details();
//
//	std::vector<collision_effect_detail> effects;
//	std::vector<collision_movement_outcome> movement_outcomes;
//
//	for (size_t i = 0; i < collidee_types.size(); i++)
//	{
//		for (size_t j = 0; j < collider_details.size(); j++)
//		{
//			if (collidee_types[i] == collider_details[j].colidee_type)
//			{
//				movement_outcomes.push_back(collider_details[j].movement_outcome);
//				for (size_t k = 0; k < collider_details[j].effects.size(); k++)
//				{
//					effects.push_back(collider_details[j].effects[k]);
//				}
//			}
//		}
//	}
//	if (movement_outcomes.size() == 0 && effects.size() == 0)
//	{
//		const collidee_object_detail& default_detail =
//			collider->get_default_collidee_object_detail();
//		movement_outcomes.push_back(default_detail.movement_outcome);
//		effects = default_detail.effects;
//	}
//	this->apply_effects(collider, collidee, effects);
//
//	collision_movement_outcome movement =
//		this->prioritise_and_select_movement(movement_outcomes);
//	
//	// check the shape types of the collider and collidee
//	collidable_object_shape collidee_shape = collidee->get_collidable_object_shape();
//	switch (collider->get_collidable_object_shape())
//	{
//	case collidable_object_shape::RECTANGLE:
//		switch (collidee_shape)
//		{
//		case collidable_object_shape::RECTANGLE:
//			this->resolve_rectangles_colliding(collider,
//				collidee, movement);
//		case collidable_object_shape::RECTANGLE_ROTATED:
//			std::exception("Not implemented");
//		case collidable_object_shape::CIRCLE:
//			this->resolve_rectangle_colliding_with_circle(collider,
//				collidee, movement);
//		case collidable_object_shape::TRIANGLE:
//			this->resolve_rectangle_colliding_with_triangle(collider,
//				collidee, movement);
//		default:
//			std::exception("Incorrect shape type");
//		}
//	case collidable_object_shape::RECTANGLE_ROTATED:
//		std::exception("Not implemented");
//	case collidable_object_shape::CIRCLE:
//		switch (collidee_shape)
//		{
//		case collidable_object_shape::RECTANGLE:
//			this->resolve_circle_colliding_with_rectangle(collider,
//				collidee, movement);
//		case collidable_object_shape::RECTANGLE_ROTATED:
//			std::exception("Not implemented");
//		case collidable_object_shape::CIRCLE:
//			this->resolve_circles_colliding(collider, collidee, movement);
//		case collidable_object_shape::TRIANGLE:
//			this->resolve_circle_colliding_with_triangle(collider,
//				collidee, movement);
//		default:
//			std::exception("Incorrect shape type");
//		}
//	case collidable_object_shape::TRIANGLE:
//		switch (collidee_shape)
//		{
//		case collidable_object_shape::RECTANGLE:
//			this->resolve_triangle_colliding_with_rectangle(collider,
//				collidee, movement);
//		case collidable_object_shape::RECTANGLE_ROTATED:
//			std::exception("Not implemented");
//		case collidable_object_shape::CIRCLE:
//			this->resolve_triangle_colliding_with_circle(collider,
//				collidee, movement);
//		case collidable_object_shape::TRIANGLE:
//			this->resolve_triangles_colliding(collider, collidee, movement);
//		default:
//			std::exception("Incorrect shape type");
//		}
//	default:
//		std::exception("Incorrect shape type");
//	}
//}
//
//void CollisionResolver::sort_colliders(
//	std::vector<std::unique_ptr<ICollidableWithOthers>>* colliders) const
//{
//	std::sort(colliders->begin(), colliders->end(),
//		[](const std::unique_ptr<ICollidableWithOthers>& a,
//			const std::unique_ptr<ICollidableWithOthers>& b)
//	{
//		return a->get_priority_level() < b->get_priority_level();
//	});
//}
//
//// Sorts collidables by their intersection area with the collider.
//// Smallest first.
//void CollisionResolver::sort_collidables(
//	std::vector<ICollidable*> collidables,
//	const std::vector<Vector2F>& intersection_depths) const
//{
//	if (collidables.size() != intersection_depths.size())
//	{
//		std::exception("Incorrect number of intersection depths");
//	}
//	std::sort(collidables.begin(), collidables.end(),
//		[&intersection_depths](const ICollidable* a,
//			const ICollidable* b)
//	{
//		for (size_t i = 0; i < intersection_depths.size(); i++)
//		{
//			if (intersection_depths[i].x == 0 && intersection_depths[i].y == 0)
//			{
//				continue;
//			}
//			if (intersection_depths[i].x < intersection_depths[i].y)
//			{
//				return true;
//			}
//			else if (intersection_depths[i].x > intersection_depths[i].y)
//			{
//				return false;
//			}
//		}
//		return false;
//	});
//}
//
//void CollisionResolver::resolve_rectangles_colliding(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	IRectangle* collider_rect = nullptr;
//	IRectangle* collidee_rect = nullptr;
//	try
//	{
//		IRectangle* collider_rect =
//			dynamic_cast<IRectangle*>(collider);
//		IRectangle* collidee_rect =
//			dynamic_cast<IRectangle*>(collidee);
//	}
//	catch (std::bad_cast e)
//	{
//		std::exception("Incorrect shape type");
//	}
//
//	rectangles_collision_quantifier quantifier =
//		this->_collision_quantifier->get_rectangles_collision_quantifier(
//			collider_rect, collidee_rect);
//
//	switch (movement)
//	{
//	case collision_movement_outcome::NONE:
//		break;
//	case collision_movement_outcome::STOP_VERTICAL:
//		collider_rect->offset(0.0f, quantifier.intersection_depth.y);
//		collider->zero_velocity_y();
//		break;
//	case collision_movement_outcome::STOP_HORIZONTAL:
//		collider_rect->offset(quantifier.intersection_depth.x, 0.0f);
//		collider->zero_velocity_x();
//		break;
//	case collision_movement_outcome::REBOUND_VERTICAL:
//		collider_rect->offset(0.0f, quantifier.intersection_depth.y);
//		collider->reverse_velocity_y();
//		break;
//	case collision_movement_outcome::REBOUND_HORIZONTAL:
//		collider_rect->offset(quantifier.intersection_depth.x, 0.0f);
//		collider->reverse_velocity_x();
//		break;
//	case collision_movement_outcome::STOP:
//		collider_rect->offset(quantifier.intersection_depth.x,
//			quantifier.intersection_depth.y);
//		collider->zero_velocity();
//		break;
//	}
//
//
//}
//void CollisionResolver::resolve_circles_colliding(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//void CollisionResolver::resolve_triangles_colliding(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//void CollisionResolver::resolve_rectangle_colliding_with_circle(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//void CollisionResolver::resolve_rectangle_colliding_with_triangle(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//void CollisionResolver::resolve_circle_colliding_with_triangle(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//void CollisionResolver::resolve_circle_colliding_with_rectangle(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//void CollisionResolver::resolve_triangle_colliding_with_rectangle(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//void CollisionResolver::resolve_triangle_colliding_with_circle(
//	ICollidableWithOthers* collider,
//	ICollidable* collidee, collision_movement_outcome movement) const
//{
//	std::exception("Not implemented");
//}
//
//void CollisionResolver::apply_effects(ICollidableWithOthers* collider,
//	ICollidable* collidee,
//	const std::vector<collision_effect_detail>& effects) const
//{
//	for (size_t i = 0; i < effects.size(); i++)
//	{
//
//	}
//}
//
//collision_movement_outcome CollisionResolver::prioritise_and_select_movement(
//	const std::vector<collision_movement_outcome>& effects) const
//{
//	collision_movement_outcome outcome = collision_movement_outcome::NONE;
//	for (size_t i = 0; i < effects.size(); i++)
//	{
//		if (static_cast<int>(effects[i]) > static_cast<int>(outcome))
//		{
//			outcome = effects[i];
//		}
//	}
//	return outcome;
//}