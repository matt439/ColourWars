#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_type.h"
#include "game/objects/blocked_shot_contact.h"
#include "game/objects/horizontal_motion.h"
#include "game/tuning.h"
#include "engine/render/animation_object.h"
#include "engine/core/moving_object.h"
#include "engine/collision/collision_object.h"
#include "game/objects/collision_object_type.h"
#include "engine/render/colour.h"
#include <span>
#include <optional>
#include <vector>

class PaintAudio;

// The tuning's sheet and animation strip as one resolved pair. Load-time.
labrador::RenderResources::SheetStrip resolve_projectile_strip(
	const ProjectileTuning& tuning, const labrador::RenderResources& render_resources);

// Abstract paint projectile: shared lifetime, team filtering and swept contacts;
// concrete subclasses supply their shape, movement update and drawing.
class Projectile : public labrador::MovingObject,
	public labrador::AnimationObject, public labrador::CollisionObject
{
public:
	// Borrowed tuning must outlive the projectile. main() owns the immutable
	// GameTuning, avoiding a copy of the asset names on each spawn. strip is
	// the tuning's sheet and animation strip, resolved once by whoever spawns
	// it (resolve_projectile_strip), so a spawn resolves no name.
	Projectile(const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		ProjectileType type,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip strip,
		const ProjectileTuning* tuning,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override = 0;
	void draw(labrador::DrawList& draw_list) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	const mattmath::Shape* shape() const override = 0;
	labrador::CollisionLayer layer() const override;
	labrador::CollisionMask mask() const override;
	labrador::CollisionTag tag() const override;
	void on_contact(const labrador::CollisionObject& other,
		const mattmath::Vector2F& normal, float penetration) override;
	bool for_deletion() const override;
	void set_for_deletion(bool for_deletion) override;

	float delete_timer() const;
	// Paint-owner index; device-less players have a separate owner by view.
	int player_num() const;
	ProjectileType projectile_type() const { return this->type_; }

	// A muzzle can extend through cover even when its owner cannot. Check
	// the launch volume before Scene defers its first collision sweep.
	void resolve_launch_obstruction(const mattmath::Vector2F& origin,
		std::span<labrador::CollisionObject* const> objects);
	// Resolve a flying projectile at its first impact along this tick's travel,
	// before the scene dispatches endpoint contacts in object-list order.
	// Rollers keep their one-step, multiple-contact behaviour.
	void resolve_movement_contacts(
		std::span<labrador::CollisionObject* const> objects);
	// Completed moving Roller contacts suppress only duplicate endpoint paint.
	// IDs are Level-local values, never borrowed collision-object pointers.
	bool moving_paint_completed(HorizontalMotionId id) const;
	// Cosmetic results accumulate across both contact callbacks and all surfaces
	// touched by a roller. Flush only after that contact batch is complete.
	void note_paint_contact(bool painted, bool captured) const;
	void flush_paint_audio(PaintAudio& audio);
	// Consume after both callbacks classify the hit, before the projectile is
	// retired. Wet tiles already have an impact; bare cover needs a detached cue.
	std::optional<BlockedShotContact> take_blocked_contact();
protected:
	virtual void set_rectangle(const mattmath::RectangleF& rectangle) = 0;

	// This projectile kind's whole record, borrowed. Never null.
	const ProjectileTuning& tuning() const;

	const ProjectileDetails& details() const;

	// Integrator velocity limit shared by all projectile kinds.
	const mattmath::Vector2F& max_velocity() const;

	PlayerTeam team() const;

	CollisionObjectType collision_type() const;

	const labrador::Colour& team_colour() const;
	ProjectileType type() const;

	float timer() const;
	void alter_timer(float time);

	// Return this step's displacement for the subclass to apply.
	virtual mattmath::Vector2F update_movement(float gravity,
		float wind_resistance, float dt);
private:
	// Borrowed, never null; the builder requires a valid tuning record.
	const ProjectileTuning* tuning_ = nullptr;

	float timer_ = 0.0f;

	// Allow one update before lifetime retirement so every spawn receives a collision sweep.
	bool first_update_done_ = false;
	mutable bool geometry_contact_ = false;
	mutable bool painted_contact_ = false;
	mutable bool captured_contact_ = false;
	std::optional<BlockedShotContact> blocked_contact_;
	mutable bool wet_contact_ = false;
	mattmath::RectangleF previous_rectangle_ = mattmath::RectangleF::ZERO;
	// One complete target span consumes each positive actual projectile update.
	bool moving_contact_ready_ = false;
	std::vector<HorizontalMotionId> moving_paint_completed_;


	void resolve_sweep(const mattmath::RectangleF& start,
		const mattmath::RectangleF& finish,
		std::span<labrador::CollisionObject* const> objects, bool launch);
	void resolve_fixed_slab_sweep(const mattmath::RectangleF& start,
		const mattmath::RectangleF& finish,
		std::span<labrador::CollisionObject* const> objects, bool launch);

    void resolve_translating_contacts(const mattmath::RectangleF& start,
        const mattmath::RectangleF& finish,
        std::span<labrador::CollisionObject* const> objects, bool moving_ready);
    void resolve_rolling_translating_contacts(
        std::span<labrador::CollisionObject* const> objects);
	int player_num_ = -1;
	labrador::Colour team_colour_ = labrador::Colour::gray;
	ProjectileType type_ = ProjectileType::spray;

	PlayerTeam team_ = PlayerTeam::none;
	bool for_deletion_ = false;

};

// Abstract specialization that adds diffusion while retaining Projectile's
// collision and lifetime contract. Spray and mist provide the concrete shapes.
class DiffusingProjectile : public Projectile
{
public:
	DiffusingProjectile(const mattmath::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		ProjectileType type,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip strip,
		const ProjectileTuning* tuning,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const mattmath::Vector2F& origin = mattmath::Vector2F::ZERO,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);
	~DiffusingProjectile() override = default;


	void update(float dt) override = 0;
	void draw(labrador::DrawList& draw_list) const override = 0;
	mattmath::RectangleF bounds() const override = 0;

	const mattmath::Shape* shape() const override = 0;
protected:
	// The sprite follows the collision rectangle's centre, but has its own
	// authored size. linear_size changes only the sprite; the combined mode
	// also scales the collision rectangle around that same centre.
	mattmath::RectangleF sprite_rectangle(
		const mattmath::RectangleF& collision_rectangle) const;
	void diffuse_collision_rectangle(mattmath::RectangleF& rectangle) const;
private:
	// Read diffusion settings from the borrowed projectile tuning record.
	const DiffusionDetails& diffusion_details() const;
	mattmath::Vector2F calculate_diffusion_scale() const;
};
