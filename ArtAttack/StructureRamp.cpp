#include "pch.h"
//#include "StructureRamp.h"
//
//using namespace DirectX;
//using namespace MattMath;
//
//StructureRamp::StructureRamp(const std::string& sheet_name,
//	const std::string& frame_name,
//	const MattMath::TriangleRightAxisAligned& triangle,
//	ResourceManager* resource_manager,
//	collision_object_type collision_type,
//	const Colour& color,
//	float rotation,
//	const Vector2F& origin,
//	SpriteEffects effects,
//	float layer_depth) :
//	Structure(sheet_name, frame_name, triangle.get_bounding_box(), &triangle, resource_manager,
//		collision_type, color, rotation, origin, effects, layer_depth)
//{
//
//}
//
//float StructureRamp::get_gradient() const
//{
//	return static_cast<const TriangleRightAxisAligned*>(
//		this->get_shape())->get_hypotenuse_gradient();
//}