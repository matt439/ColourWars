#pragma once

#include "engine/collision/collision_object.h"
#include "engine/render/texture_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/render/colour.h"
#include "game/objects/terrain_material.h"

class Structure : public labrador::TextureObject, public labrador::CollisionObject
{
public:
	Structure() = delete;
	// Take ownership of the collision shape; it must not be null.
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
		float layer_depth = 0.0f,
		std::unique_ptr<const TerrainMaterial> material = nullptr);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	// The same painter order, with strokes for the list's camera (DrawList::
	// camera()), drawn only by the miniature views.
	virtual void draw_miniature(labrador::DrawList& draw_list) const;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
	labrador::CollisionLayer layer() const override;
	labrador::CollisionMask mask() const override;
	labrador::CollisionTag tag() const override;

	// Geometry remains fixed on contact; StructurePaintable overrides this to accept paint.
	void on_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;

	bool for_deletion() const override;
	// The Tool Props drawing, or null for the legacy stretched frame.
	const TerrainMaterial* material() const;
protected:
	const mattmath::RectangleF& rectangle() const;
	CollisionObjectType collision_type() const;
private:
	CollisionObjectType collision_type_ =
		CollisionObjectType::structure;
	mattmath::RectangleF sprite_rectangle_ = mattmath::RectangleF::ZERO;
	std::unique_ptr<mattmath::Shape> collision_shape_ = nullptr;
	std::unique_ptr<const TerrainMaterial> material_ = nullptr;
};
