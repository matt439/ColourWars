#pragma once

#include "game/objects/projectile.h"
#include "engine/render/colour.h"

// A sub that sticks where it lands (ROAD#11): the sprinkler and the paint
// wall. In flight it is a paint ball on the sub's body record, until that
// body first meets cover. There it plants: it stops, touches nothing (its
// mask empties, so neither cover nor players contact it), deals no damage,
// and every `interval` seconds for `lifetime` it releases drops, each a
// ProjectileBall on the sub's burst record, laid out by the concrete class
// (spray). Enemy shots that reach it are spent on it: Level hands it one hit
// for each (takes_hits), and the last of `hits` retires it. Meeting an enemy
// player before cover, or the body's delete_timer running out in the air,
// retires it as any flying shot, unplanted.
class ProjectilePlanted : public Projectile
{
public:
	ProjectilePlanted() = delete;
	// `rectangle`'s position is the centre to release from, as for a ball.
	// The sub's records and strips are borrowed for the match.
	ProjectilePlanted(const labrador::RectangleF& rectangle,
		const labrador::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip body_strip,
		labrador::RenderResources::SheetStrip spray_strip,
		const SubTuning* sub);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	labrador::RectangleF bounds() const override;
	const labrador::Shape* shape() const override;
	labrador::CollisionMask mask() const override;

	// The first cover it meets plants it; once planted, nothing reaches it.
	void on_structure_contact(const Structure& other,
		const labrador::Vector2F& normal, float penetration) override;
	float player_damage() const override;
	bool takes_hits() const override;
	void take_hit() override;

	// The drops due this tick, if any: the first at once on planting, then
	// one release each interval while it stands.
	std::vector<std::unique_ptr<Projectile>> take_burst() override;

	bool planted() const { return this->planted_; }
	int sprays() const { return this->sprays_; }
	int hits_left() const { return this->hits_left_; }

protected:
	void resolve_travel(std::span<labrador::CollisionObject* const> objects) override;
	// It sticks to bare cover rather than splashing, so it never duds.
	bool duds_on_bare_cover() const override { return false; }

	// The drops of the release'th release (counted from 0), each made with drop.
	virtual std::vector<std::unique_ptr<Projectile>> spray(int release) const = 0;
	// One drop of the sub's burst record centred at centre, moving at velocity.
	std::unique_ptr<Projectile> drop(const labrador::Vector2F& centre,
		const labrador::Vector2F& velocity) const;
	const SubTuning& sub() const { return *this->sub_; }

private:
	void set_rectangle(const labrador::RectangleF& rectangle) override;

	labrador::RectangleF rectangle_ = labrador::RectangleF::zero;
	labrador::RenderResources* render_resources_ = nullptr;
	labrador::RenderResources::SheetStrip spray_strip_;
	const SubTuning* sub_ = nullptr;
	bool planted_ = false;
	// Seconds planted left, and until the next release (the first is at once).
	float life_left_ = 0.0f;
	float next_spray_ = 0.0f;
	int sprays_ = 0;
	int hits_left_ = 0;
};
