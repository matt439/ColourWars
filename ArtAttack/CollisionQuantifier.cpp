#include "pch.h"
#include "CollisionQuantifier.h"

//using namespace MattMath;
//
//rectangles_collision_quantifier
//	CollisionQuantifier::get_rectangles_collision_quantifier(
//	IRectangle* rect1, IRectangle* rect2)
//{
//	rectangles_collision_quantifier result;
//	RectangleF rect1_bounds = rect1->get_rectangle();
//	RectangleF rect2_bounds = rect2->get_rectangle();
//
//	result.intersection = RectangleF::intersection(rect1_bounds, rect2_bounds);
//	result.rect1_center = rect1_bounds.get_center();
//	result.rect2_center = rect2_bounds.get_center();
//	result.centers_separation = result.rect1_center - result.rect2_center;
//	result.rect1_contains_rect2 = rect1_bounds.contains(rect2_bounds);
//	result.rect2_contains_rect1 = rect2_bounds.contains(rect1_bounds);
//
//	return result;
//}