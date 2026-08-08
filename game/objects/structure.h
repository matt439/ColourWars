#pragma once

#include "engine/collision/collision_object.h"
#include "engine/render/texture_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/render/colour.h"

class Structure : public artattack::TextureObject, public artattack::CollisionObject
{
public:
	Structure() = default;
	Structure(const std::string& sheet_name,
		const std::string& frame_name,
		const mattmath::RectangleF& sprite_rectangle,
		const mattmath::Shape* collision_shape,
		artattack::RenderResources* render_resources,
		CollisionObjectType collision_type,
		const artattack::Colour& color = artattack::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override;
	void draw(artattack::DrawList& draw_list) const override;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
	artattack::CollisionLayer layer() const override;
	artattack::CollisionMask mask() const override;
	artattack::CollisionTag tag() const override;

	// Level geometry does not respond to being hit; it is what everything
	// else responds to. StructurePaintable overrides this to take paint.
	void on_contact(const artattack::CollisionObject& other,
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
