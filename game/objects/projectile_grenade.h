#pragma once

#include "game/objects/projectile.h"
#include "game/objects/round_shot_shape.h"
#include "engine/math/circle.h"
#include "engine/render/colour.h"

// A sub's paint grenade (ROAD#11): a paint ball that flies like the bucket's
// and dies as any flying shot does, on its first contact with cover or an
// enemy, or in the air when its body's delete_timer runs out. Retiring is
// what makes it a grenade: take_burst hands back a ring of drops, each a
// ProjectileBall on the sub's burst record, centred where it stopped.
class ProjectileGrenade final : public Projectile
{
public:
	ProjectileGrenade() = delete;
	// `rectangle`'s position is the centre to throw from, as for a ball.
	// Both records and both strips are the sub's, borrowed for the match.
	ProjectileGrenade(const labrador::RectangleF& rectangle,
		const labrador::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip body_strip,
		const ProjectileTuning* body,
		labrador::RenderResources::SheetStrip burst_strip,
		const ProjectileTuning* burst,
		int burst_count,
		float burst_speed,
		RoundShotShape shape = round_shot_shape());

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	labrador::RectangleF bounds() const override;
	const labrador::Shape* shape() const override;

	// Once, after it is marked for deletion: burst_count drops spaced evenly
	// around the circle, the first straight up, each at burst_speed.
	std::vector<std::unique_ptr<Projectile>> take_burst() override;
	bool burst() const { return this->burst_; }
	// What it was built with (ROAD#44), which its drops are built with too.
	RoundShotShape round_shape() const { return this->shape_mode_; }

protected:
	// The burst is the grenade's answer to bare cover, so it never duds.
	bool duds_on_bare_cover() const override { return false; }
	std::optional<float> sweep_radius() const override;

private:
	void set_rectangle(const labrador::RectangleF& rectangle) override;
	// The disc inscribed in rectangle_, kept with it; shape() only in circle mode.
	void sync_circle();

	labrador::RectangleF rectangle_ = labrador::RectangleF::zero;
	labrador::Circle circle_;
	RoundShotShape shape_mode_ = RoundShotShape::rectangle;
	labrador::RenderResources* render_resources_ = nullptr;
	labrador::RenderResources::SheetStrip burst_strip_;
	const ProjectileTuning* burst_tuning_ = nullptr;
	int burst_count_ = 0;
	float burst_speed_ = 0.0f;
	bool burst_ = false;
};
