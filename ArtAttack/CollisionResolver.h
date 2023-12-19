#ifndef COLLISIONRESOLVER_H
#define COLLISIONRESOLVER_H

//#include "ICollidableWithOthers.h"
//#include <vector>
//#include <memory>
//#include <algorithm>
//#include "CollisionDetector.h"
//#include "CollisionQuantifier.h"
//
//class CollisionResolver
//{
//private:
//	std::unique_ptr<CollisionDetector> _collision_detector = nullptr;
//	std::unique_ptr<CollisionQuantifier> _collision_quantifier = nullptr;
//	//std::unique_ptr<CollisionDetector> get_detector() const;
//	void sort_colliders(std::vector<std::unique_ptr<ICollidableWithOthers>>*
//		colliders) const;
//
//	void sort_collidables(std::vector<ICollidable*> collidables,
//		const std::vector<Vector2F>& intersection_depths) const;
//
//	void resolve_collision(ICollidableWithOthers* collider,
//		ICollidable* collidee) const;
//
//	void resolve_rectangles_colliding(ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//	void resolve_circles_colliding(ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//	void resolve_triangles_colliding(ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//
//	void resolve_rectangle_colliding_with_circle(
//		ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//	void resolve_rectangle_colliding_with_triangle(
//		ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//
//	void resolve_circle_colliding_with_triangle(
//		ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//	void resolve_circle_colliding_with_rectangle(
//		ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//
//	void resolve_triangle_colliding_with_rectangle(
//		ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//	void resolve_triangle_colliding_with_circle(
//		ICollidableWithOthers* collider,
//		ICollidable* collidee, collision_movement_outcome movement) const;
//
//	void apply_effects(ICollidableWithOthers* collider,
//		ICollidable* collidee,
//		const std::vector<collision_effect_detail>& effects) const;
//
//	collision_movement_outcome prioritise_and_select_movement(
//		const std::vector<collision_movement_outcome>& effects) const;
//
//public:
//	CollisionResolver();
//	void resolve_collisions(
//		std::vector<std::unique_ptr<ICollidableWithOthers>>* colliders,
//		std::vector<std::unique_ptr<ICollidable>>* collidables) const;
//};
#endif // !COLLISIONRESOLVER_H
