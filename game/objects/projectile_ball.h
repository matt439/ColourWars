#pragma once

#include "game/objects/projectile.h"
#include "game/objects/round_shot_shape.h"
#include "engine/math/circle.h"
#include "engine/render/colour.h"

class ProjectileBall final : public Projectile
{
public:
	ProjectileBall() = delete;
	ProjectileBall(const labrador::RectangleF& rectangle,
		const labrador::Vector2F& velocity,
		PlayerTeam team,
		int player_num,
		const labrador::Colour& team_colour,
		labrador::RenderResources* render_resources,
		labrador::RenderResources::SheetStrip strip,
		const ProjectileTuning* tuning,
		float rotation = 0.0f,
		const labrador::Vector2F& origin = labrador::Vector2F::zero,
		labrador::SpriteFlip flip = labrador::SpriteFlip::none,
		float layer_depth = 0.0f,
		RoundShotShape shape = round_shot_shape());

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	labrador::RectangleF bounds() const override;

	const labrador::Shape* shape() const override;
	// What it was built with (ROAD#44); a grenade's drops take its own.
	RoundShotShape round_shape() const { return this->shape_mode_; }
protected:
	std::optional<float> sweep_radius() const override;
private:
	void set_rectangle(const labrador::RectangleF& rectangle) override;
	// The disc inscribed in rectangle_, kept with it; shape() only in circle mode.
	void sync_circle();
	labrador::RectangleF rectangle_ = labrador::RectangleF::zero;
	labrador::Circle circle_;
	RoundShotShape shape_mode_ = RoundShotShape::rectangle;
};
