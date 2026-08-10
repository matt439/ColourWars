#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_type.h"
#include "game/tuning.h"
#include "engine/render/animation_object.h"
#include "engine/core/moving_object.h"
#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/render/colour.h"

class Projectile : public artattack::MovingObject,
	public artattack::AnimationObject, public artattack::CollisionObject
{
public:
	// `tuning` is borrowed and must outlive this projectile. It points into
	// the GameTuning main() owns, which outlives the Application and therefore
	// every level, so this holds for every projectile that can exist.
	//
	// Borrowed rather than copied because a sprayer spawns one of these per
	// frame per firing player, and ProjectileDetails carries two std::strings:
	// copying the record was two heap allocations on the spawn path for a
	// value that is identical for every projectile of the type and never
	// changes after startup (T8).
	Projectile(const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const artattack::Colour& team_colour,
		projectile_type type,
		artattack::RenderResources* render_resources,
		const ProjectileTuning* tuning,
		const artattack::Colour& color = artattack::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override = 0;
	void draw(artattack::DrawList& draw_list) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	const mattmath::Shape* shape() const override = 0;
	artattack::CollisionLayer layer() const override;
	artattack::CollisionMask mask() const override;
	artattack::CollisionTag tag() const override;
	void on_contact(const artattack::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;
	bool for_deletion() const override;
	void set_for_deletion(bool for_deletion) override;

	float delete_timer() const;
protected:
	// This projectile kind's whole record, borrowed. Never null.
	const ProjectileTuning& tuning() const;

	const ProjectileDetails& details() const;

	// The clamp every projectile's integrator applies. One number for all five
	// kinds, which is why it is not in `details`.
	const mattmath::Vector2F& max_velocity() const;

	PlayerTeam team() const;

	CollisionObjectType collision_type() const;

	int player_num() const;
	const artattack::Colour& team_colour() const;
	projectile_type type() const;

	float timer() const;
	void alter_timer(float time);

	// Returns this step's displacement rather than leaving it on a member for
	// the caller to read back. Only the four subclasses immediately below the
	// call ever wanted it, and a member that is written and read inside one
	// call chain is a return value that has been talked out of being one.
	virtual mattmath::Vector2F update_movement(float gravity,
		float wind_resistance, float dt);
private:
	// Never null: every constructor takes one and the builder reads it out of
	// GameTuning, which throws rather than answering with a missing record.
	const ProjectileTuning* tuning_ = nullptr;

	float timer_ = 0.0f;

	// False until update_movement has run once. See the comment there: the
	// lifetime test cannot fire on the update that precedes this projectile's
	// first collision sweep.
	bool first_update_done_ = false;

	int player_num_ = -1;
	artattack::Colour team_colour_ = artattack::Colour::gray;
	projectile_type type_ = SPRAY;

	PlayerTeam team_ = PlayerTeam::none;
	bool for_deletion_ = false;

};

class DiffusingProjectile : public Projectile
{
public:
	DiffusingProjectile(const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const artattack::Colour& team_colour,
		projectile_type type,
		artattack::RenderResources* render_resources,
		const ProjectileTuning* tuning,
		const artattack::Colour& color = artattack::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		artattack::SpriteFlip flip = artattack::SpriteFlip::none,
		float layer_depth = 0.0f);
	~DiffusingProjectile() override = default;


	void update(float dt) override = 0;
	void draw(artattack::DrawList& draw_list) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	const mattmath::Shape* shape() const override = 0;
protected:
	mattmath::Vector2F calculate_diffusion_size() const;
private:
	// Read off the same borrowed record the base class holds, rather than
	// copied beside it.
	const DiffusionDetails& diffusion_details() const;
	const mattmath::Vector2F& base_size() const;
};
