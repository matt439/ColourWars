#pragma once

#include "engine/render/texture_object.h"
#include "game/objects/collider.h"
#include "game/objects/collision_object_type.h"
#include "game/objects/player_team.h"
#include "engine/render/colour.h"
#include "game/objects/terrain_material.h"
#include <optional>

class FixedSlabGeometry;
class TeamGateStructurePaintable;
class TranslatingStructurePaintable;

class Structure : public labrador::TextureObject, public Collider
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
	// Chosen by the caller's path during construction/update, never by zoom
	// and never mutated while Scene renders its views on parallel workers.
	void set_miniature(bool miniature) { this->miniature_ = miniature; }
	bool miniature() const { return this->miniature_; }
	// Explicit legacy painter retained for independent renderer references.
	// Runtime views select their mode before calling the ordinary draw().
	virtual void draw_miniature(labrador::DrawList& draw_list) const;
	mattmath::RectangleF bounds() const override;

	const mattmath::Shape* shape() const override;
	labrador::CollisionLayer layer() const override;
	labrador::CollisionMask mask() const override;
	labrador::CollisionTag tag() const override;

	bool for_deletion() const override;
	// The Tool Props drawing, or null for the legacy stretched frame.
	const TerrainMaterial* material() const;

	Structure* structure() override { return this; }
	const Structure* structure() const override { return this; }

	// How a player stands on it. The level file chooses these through the
	// collision type; they are the structure's own answers, not its tag's.
	// A one-way platform, landed on only from above.
	bool jump_through() const;
	// A right-angled slope, which a player walks up rather than into.
	bool ramp() const;
	// For a ramp, whether it climbs towards +x.
	bool ramp_rises_right() const;
	// The fixed sloping slab's geometry, or null for anything else.
	virtual const FixedSlabGeometry* slab_geometry() const { return nullptr; }
	// Neither a ramp nor a slab: its top is its bounding box's top edge.
	bool flat() const;

	// The paint a player standing at world x reads, for the surfaces that
	// hold paint on top. Bare structures answer none.
	virtual PlayerTeam top_paint_team(float x) const;
	virtual std::optional<labrador::Colour> top_paint_colour(float x) const;
	// Whether that top paint lies along its flat top edge, read where the
	// feet meet it. A slab's paint lies along its slope and is read from its
	// own support line instead.
	virtual bool holds_flat_top_paint() const { return false; }

	// Whether a player of team walks through it now. Only an open gate does.
	virtual bool passable_for(PlayerTeam team) const;
	// Where a shot centred at shot_centre, stopped by this cover, marks the
	// cover's face. normal is the shot's contact normal, from the shot into
	// the cover.
	virtual mattmath::Vector2F cover_point(const mattmath::Vector2F& shot_centre,
		const mattmath::Vector2F& normal) const;

	// The roles Level borrows for per-step work. Null unless this is one.
	virtual TranslatingStructurePaintable* mover() { return nullptr; }
	virtual const TranslatingStructurePaintable* mover() const { return nullptr; }
	virtual TeamGateStructurePaintable* gate() { return nullptr; }
	virtual const TeamGateStructurePaintable* gate() const { return nullptr; }

protected:
	const mattmath::RectangleF& rectangle() const;
	CollisionObjectType collision_type() const;
	// Tell the receiver it touched a structure. A structure's own handlers
	// keep the defaults, so bare geometry stays fixed on contact; a painted
	// one answers projectiles (StructurePaintable).
	void touch(Collider& receiver,
		const mattmath::Vector2F& normal, float penetration) const override;
private:
	bool miniature_ = false;
	CollisionObjectType collision_type_ =
		CollisionObjectType::structure;
	mattmath::RectangleF sprite_rectangle_ = mattmath::RectangleF::ZERO;
	std::unique_ptr<mattmath::Shape> collision_shape_ = nullptr;
	std::unique_ptr<const TerrainMaterial> material_ = nullptr;
};
