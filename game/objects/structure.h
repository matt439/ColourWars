#pragma once

#include "engine/collision/collision_object.h"
#include "engine/render/texture_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/render/colour.h"
#include <vector>

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
		float tile_world_units_per_texel = 0.0f);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
	labrador::CollisionLayer layer() const override;
	labrador::CollisionMask mask() const override;
	labrador::CollisionTag tag() const override;

	// Geometry remains fixed on contact; StructurePaintable overrides this to accept paint.
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

	// Only the visual is tiled. Collision and the paintable subclass continue
	// to use the original rectangle, including its exact partial edges.
	struct TextureTile
	{
		mattmath::RectangleI source;
		mattmath::RectangleF destination;
	};
	std::vector<TextureTile> texture_tiles_;
	void build_texture_tiles(float world_units_per_texel);
};
