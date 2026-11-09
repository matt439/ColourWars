#pragma once

#include "game/objects/projectile.h"
#include "engine/render/colour.h"

// A sub's sprinkler (ROAD#11): a paint ball in flight, like the grenade, until
// its body first meets cover. There it sticks: it stops, touches nothing
// (its mask empties, so neither cover nor players contact it), deals no
// damage, and every `interval` seconds for `lifetime` it sprays a fan of
// drops over its top half, each a ProjectileBall on the sub's burst record.
// Enemy shots break it: Level hands it one hit for each that reaches it
// (takes_hits), and the last of `hits` retires it. Meeting an enemy player
// before cover, or the body's delete_timer running out in the air, retires
// it as any flying shot, unplanted.
class ProjectileSprinkler final : public Projectile
{
public:
	ProjectileSprinkler() = delete;
	// `rectangle`'s position is the centre to throw from, as for a ball. The
	// sub's records and strips are borrowed for the match.
	ProjectileSprinkler(const labrador::RectangleF& rectangle,
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

	// The spray due this tick, if any: burst_count drops spread evenly from
	// straight left to straight right across the top, and every other spray
	// turned so its drops fill the gaps between the last one's.
	std::vector<std::unique_ptr<Projectile>> take_burst() override;

	bool planted() const { return this->planted_; }
	int sprays() const { return this->sprays_; }
	int hits_left() const { return this->hits_left_; }

protected:
	void resolve_travel(std::span<labrador::CollisionObject* const> objects) override;
	// It sticks to bare cover rather than splashing, so it never duds.
	bool duds_on_bare_cover() const override { return false; }

private:
	void set_rectangle(const labrador::RectangleF& rectangle) override;

	labrador::RectangleF rectangle_ = labrador::RectangleF::zero;
	labrador::RenderResources* render_resources_ = nullptr;
	labrador::RenderResources::SheetStrip spray_strip_;
	const SubTuning* sub_ = nullptr;
	bool planted_ = false;
	// Seconds planted left, and until the next spray (the first is at once).
	float life_left_ = 0.0f;
	float next_spray_ = 0.0f;
	int sprays_ = 0;
	int hits_left_ = 0;
};
