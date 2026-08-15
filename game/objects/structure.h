#pragma once

#include "engine/collision/collision_object.h"
#include "engine/render/texture_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/render/colour.h"

class Structure : public labrador::TextureObject, public labrador::CollisionObject
{
public:
	Structure() = default;
	// Takes the collision shape, rather than borrowing and secretly copying
	// it. The old signature was a `const Shape*` that the constructor cloned,
	// which is why mattmath::Shape carried a polymorphic clone() for this one
	// call site - and why every caller passed the address of a stack local
	// whose lifetime looked, at the call, like it had to outlive the object.
	Structure(const std::string& sheet_name,
		const std::string& frame_name,
		const mattmath::RectangleF& sprite_rectangle,
		std::unique_ptr<mattmath::Shape> collision_shape,
		labrador::RenderResources* render_resources,
		CollisionObjectType collision_type,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
	labrador::CollisionLayer layer() const override;
	labrador::CollisionMask mask() const override;
	labrador::CollisionTag tag() const override;

	// Level geometry does not respond to being hit; it is what everything
	// else responds to. StructurePaintable overrides this to take paint.
	void on_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;

	bool for_deletion() const override;
protected:
	const mattmath::RectangleF& rectangle() const;
	CollisionObjectType collision_type() const;
private:
	CollisionObjectType collision_type_ =
		CollisionObjectType::structure;
	mattmath::RectangleF sprite_rectangle_ = mattmath::RectangleF::ZERO;
	std::unique_ptr<mattmath::Shape> collision_shape_ = nullptr;
};
