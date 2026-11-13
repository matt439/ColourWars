#pragma once

#include "game/objects/player_team.h"
#include "game/objects/projectile_type.h"
#include "game/objects/blocked_shot_contact.h"
#include "game/objects/horizontal_motion.h"
#include "game/objects/projectile_target_flags.h"
#include "game/config/tuning.h"
#include "game/carried/animation_object.h"
#include "game/carried/moving_object.h"
#include "engine/collision/collision_object.h"
#include "game/objects/collider.h"
#include "game/objects/collision_object_type.h"
#include "engine/render/colour.h"
#include <memory>
#include <span>
#include <optional>
#include <vector>

class PaintAudio;
class ProjectilePlanted;

// The tuning's sheet and animation strip as one resolved pair. Load-time.
labrador::RenderResources::SheetStrip resolve_projectile_strip(
	const ProjectileTuning& tuning, const labrador::RenderResources& render_resources);

// Abstract paint projectile: shared lifetime, team filtering and swept contacts;
// concrete subclasses supply their shape, movement update and drawing, and
// override the contact rules a flying shot has (ProjectileRolling does).
class Projectile : public colourwars::MovingObject,
	public colourwars::AnimationObject, public Collider
{
public:
	// Borrowed tuning must outlive the projectile. main() owns the immutable
	// GameTuning, avoiding a copy of the asset names on each spawn. strip is
	// the tuning's sheet and animation strip, resolved once by whoever spawns
	// it (resolve_projectile_strip), so a spawn resolves no name.
	Projectile(const labrador::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		ProjectileType type,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip strip,
		const ProjectileTuning* tuning,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const labrador::Vector2F& origin = labrador::Vector2F::zero,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);

	void update(float dt) override = 0;
	void draw(labrador::DrawList& draw_list) const override = 0;
	labrador::RectangleF bounds() const override = 0;

	const labrador::Shape* shape() const override = 0;
	labrador::CollisionLayer layer() const override;
	labrador::CollisionMask mask() const override;
	labrador::CollisionTag tag() const override;
	// A flying shot dies on its first contact with flat cover or an enemy,
	// marking where cover stopped it.
	void on_structure_contact(const Structure& other,
		const labrador::Vector2F& normal, float penetration) override;
	void on_player_contact(const Player& other,
		const labrador::Vector2F& normal, float penetration) override;
	bool for_deletion() const override;
	void set_for_deletion(bool for_deletion) override;

	Projectile* projectile() override { return this; }
	const Projectile* projectile() const override { return this; }

	float delete_timer() const;
	// Paint-owner index; device-less players have a separate owner by view.
	int player_num() const;
	// The painting team; layer() follows it.
	PlayerTeam team() const;
	ProjectileType projectile_type() const { return this->type_; }
	// Health a hit removes, from this projectile's own record: its kind's,
	// or a sub's for the grenade and its drops, which share the ball's kind.
	// A planted sprinkler touches no one, so it overrides this to zero.
	virtual float player_damage() const { return this->tuning_->details.player_damage; }
	// What this tick's travel covered: last tick's shape to this one's. What
	// Level tests the roller's unswept volume against, after the tick's
	// contacts, for the objects shots can break (takes_hits).
	labrador::RectangleF travel_bounds() const;
	// Whether enemy shots break this one now: a planted sprinkler or wall.
	// Level passes each one to every shot's swept travel as a breakable
	// (resolve_movement_contacts), so an enemy shot whose path reaches it
	// before cover or a player is spent there and calls take_hit. A roller's
	// volume, which is not swept, is struck by its travel_bounds after the
	// tick's contacts instead (Level::strike_breakables).
	virtual bool takes_hits() const { return false; }
	virtual void take_hit() {}
	// This shot as a planted sub that stands and takes hits now (takes_hits),
	// or null: what the bots' planted-sub query lists (REV#21).
	virtual const ProjectilePlanted* planted_sub() const { return nullptr; }

	// Where a launch from an owner centred at owner_centre is swept from:
	// the owner's centre, for every shot and sub but the wall, which is set
	// down beside its owner and so is swept sideways at its own height.
	virtual labrador::Vector2F launch_origin(const labrador::Vector2F& owner_centre) const
	{ return owner_centre; }
	// A muzzle can extend through cover even when its owner cannot. Check
	// the launch volume before Scene defers its first collision sweep.
	// known, here and below, is what objects answers about movers and fixed
	// slabs when the caller already asked (a Level, of its own targets). It
	// must be the answers for exactly this list; without it the sweep asks.
	void resolve_launch_obstruction(const labrador::Vector2F& origin,
		std::span<labrador::CollisionObject* const> objects,
		std::optional<ProjectileTargetFlags> known = std::nullopt);
	// Resolve this tick's travel (resolve_travel) before the scene dispatches
	// endpoint contacts in object-list order, once per actual update.
	// breakables are the projectiles enemy shots can break (takes_hits, asked
	// again for each as the sweep reaches it). A swept shot stops at the first
	// of them its path enters before cover or a player, spends itself there
	// and hands it one hit. Cover wins a tied entry and a breakable beats a
	// player on one; an empty list leaves the sweep exactly as it was.
	void resolve_movement_contacts(
		std::span<labrador::CollisionObject* const> objects,
		std::optional<ProjectileTargetFlags> known = std::nullopt,
		std::span<Projectile* const> breakables = {});
	// Completed moving Roller contacts suppress only duplicate endpoint paint.
	// IDs are Level-local values, never borrowed collision-object pointers.
	bool moving_paint_completed(HorizontalMotionId id) const;
	// Cosmetic results accumulate across both contact callbacks and all surfaces
	// touched by a roller. Flush only after that contact batch is complete.
	void note_paint_contact(bool painted, bool captured);
	// note_paint_contact from a surface answering this projectile's contact,
	// where the projectile is the contact's other party and so arrives const
	// (labrador::CollisionObject::on_contact). Every projectile is owned
	// non-const by its scene or a probe, so the write is defined. This is the
	// one place the game writes through a contact's other party; engine
	// report 33 asks for a contact that can carry the answer back.
	static void answer_paint_contact(const Projectile& toucher,
		bool painted, bool captured);
	void flush_paint_audio(PaintAudio& audio);
	// Consume after both callbacks classify the hit, before the projectile is
	// retired. Wet tiles already have an impact; bare cover needs a detached cue.
	std::optional<BlockedShotContact> take_blocked_contact();
	// What this projectile releases this tick: a sub's grenade bursts into
	// drops as it retires (ProjectileGrenade), a planted sprinkler sprays on
	// its schedule (ProjectileSprinkler). Empty for every tool's shot.
	// Called once a tick, after its deletion is settled for the tick, by
	// Level for a shot in flight and by Player for one its launch stopped;
	// each child still needs its launch resolved from where it starts.
	virtual std::vector<std::unique_ptr<Projectile>> take_burst() { return {}; }
protected:
	virtual void set_rectangle(const labrador::RectangleF& rectangle) = 0;
	void touch(Collider& receiver,
		const labrador::Vector2F& normal, float penetration) const override;

	// A flying shot stops at its first impact along this tick's travel, if
	// its kind is swept (is_swept). Rollers keep their one-step volume.
	// known and breakables are resolve_movement_contacts' arguments, passed
	// through.
	virtual void resolve_travel(std::span<labrador::CollisionObject* const> objects,
		std::optional<ProjectileTargetFlags> known,
		std::span<Projectile* const> breakables);
	// Whether a launch volume crossing an enemy between owner and muzzle hits
	// them, as it hits cover there.
	virtual bool launch_hits_players() const { return true; }
	// Whether retiring after touching only bare cover sounds a dud.
	virtual bool duds_on_bare_cover() const { return true; }

	// Set by each positive update until Level resolves its travel.
	bool moving_contact_ready() const { return this->moving_contact_ready_; }
	// Paint every moving surface the one-step volume crosses in the surface's
	// own frame, once each (moving_paint_completed).
	void resolve_rolling_translating_contacts(
		std::span<labrador::CollisionObject* const> objects);

	// This projectile kind's whole record, borrowed. Never null.
	const ProjectileTuning& tuning() const;

	const ProjectileDetails& details() const;

	// Integrator velocity limit shared by all projectile kinds.
	const labrador::Vector2F& max_velocity() const;

	CollisionObjectType collision_type() const;

	const labrador::Colour& team_colour() const;

	float timer() const;
	void alter_timer(float time);

	// Return this step's displacement for the subclass to apply.
	virtual labrador::Vector2F update_movement(float gravity,
		float wind_resistance, float dt);
private:
	// Borrowed, never null; the builder requires a valid tuning record.
	const ProjectileTuning* tuning_ = nullptr;

	float timer_ = 0.0f;

	// Allow one update before lifetime retirement so every spawn receives a collision sweep.
	bool first_update_done_ = false;
	bool geometry_contact_ = false;
	bool painted_contact_ = false;
	bool captured_contact_ = false;
	std::optional<BlockedShotContact> blocked_contact_;
	bool wet_contact_ = false;
	labrador::RectangleF previous_rectangle_ = labrador::RectangleF::zero;
	// One complete target span consumes each positive actual projectile update.
	bool moving_contact_ready_ = false;
	std::vector<HorizontalMotionId> moving_paint_completed_;


	// fixed_slabs is whether objects holds a fixed slab, when the caller knows
	// it for this exact list; the one-target calls inside the sweeps pass none,
	// and no breakables, which have already lost to that target.
	void resolve_sweep(const labrador::RectangleF& start,
		const labrador::RectangleF& finish,
		std::span<labrador::CollisionObject* const> objects, bool launch,
		std::optional<bool> fixed_slabs = std::nullopt,
		std::span<Projectile* const> breakables = {});
	void resolve_fixed_slab_sweep(const labrador::RectangleF& start,
		const labrador::RectangleF& finish,
		std::span<labrador::CollisionObject* const> objects, bool launch,
		std::span<Projectile* const> breakables = {});

    void resolve_translating_contacts(const labrador::RectangleF& start,
        const labrador::RectangleF& finish,
        std::span<labrador::CollisionObject* const> objects, bool moving_ready,
        std::span<Projectile* const> breakables);
	// Spend this shot on target, which its travel from start to finish enters
	// over interval (a breakable that won the sweep): stop it just inside the
	// face it entered, as cover stops a shot, retire it and hand target a hit.
	void strike_breakable(const labrador::RectangleF& start,
		const labrador::RectangleF& finish, Projectile& target,
		const std::pair<float, float>& interval);
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
	DiffusingProjectile(const labrador::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		ProjectileType type,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip strip,
		const ProjectileTuning* tuning,
		const labrador::Colour& color = labrador::Colour::white,
		float rotation = 0.0f,
		const labrador::Vector2F& origin = labrador::Vector2F::zero,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f);
	~DiffusingProjectile() override = default;


	void update(float dt) override = 0;
	void draw(labrador::DrawList& draw_list) const override = 0;
	labrador::RectangleF bounds() const override = 0;

	const labrador::Shape* shape() const override = 0;
protected:
	// The sprite follows the collision rectangle's centre, but has its own
	// authored size. linear_size changes only the sprite; the combined mode
	// also scales the collision rectangle around that same centre.
	labrador::RectangleF sprite_rectangle(
		const labrador::RectangleF& collision_rectangle) const;
	void diffuse_collision_rectangle(labrador::RectangleF& rectangle) const;
private:
	// Read diffusion settings from the borrowed projectile tuning record.
	const DiffusionDetails& diffusion_details() const;
	labrador::Vector2F calculate_diffusion_scale() const;
};
