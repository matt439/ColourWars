#pragma once

#include "game/config/play_settings.h"
#include "engine/core/game_object.h"
#include "engine/render/texture_object.h"

// A quiet backdrop drawing, moved once per world tick and shared by every
// view. The authored rectangle is the centre of a bounded horizontal drift;
// its phase is content, never a draw from the match's random stream.
class DriftingVisual final : public labrador::GameObject, public labrador::TextureObject
{
public:
	DriftingVisual(const std::string& sheet_name, const std::string& frame_name,
		const labrador::RectangleF& anchor, labrador::RenderResources* render_resources,
		const PlaySettings* preferences, const labrador::Colour& colour,
		float distance, float period, float phase);

	void update(float dt) override;
	void draw(labrador::DrawList& draw_list) const override;
	labrador::RectangleF rectangle() const;
	labrador::RectangleF bounds() const override;
	labrador::RectangleF cull_bounds(float units_per_pixel) const override;

private:
	const PlaySettings* preferences_;
	labrador::RectangleF anchor_;
	labrador::RectangleF anchor_bounds_;
	labrador::Vector2F pixel_padding_;
	float distance_;
	double period_;
	double phase_;
	double elapsed_ = 0.0;
	float offset_ = 0.0f;

	void refresh_offset();
	float visible_offset() const;
};
