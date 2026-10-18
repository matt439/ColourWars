#include "game/pch.h"
#include "game/objects/drifting_visual.h"
#include "engine/render/sprite_geometry.h"

#include <numbers>

using namespace labrador;
using namespace mattmath;

DriftingVisual::DriftingVisual(const std::string& sheet_name, const std::string& frame_name,
	const RectangleF& anchor, RenderResources* render_resources,
	const PlaySettings* preferences, const Colour& colour,
	float distance, float period, float phase) :
	TextureObject(sheet_name, frame_name, render_resources, colour),
	preferences_(preferences), anchor_(anchor), distance_(distance),
	period_(period), phase_(phase)
{
	if (!std::isfinite(distance) || distance <= 0.0f || distance > 256.0f)
	{
		throw std::invalid_argument("drift.distance must be finite, greater than zero and at most 256");
	}
	if (!std::isfinite(period) || period < 1.0f || period > 3600.0f)
	{
		throw std::invalid_argument("drift.period must be finite and between 1 and 3600 seconds");
	}
	if (!std::isfinite(phase) || phase < 0.0f || phase >= 1.0f)
	{
		throw std::invalid_argument("drift.phase must be finite, at least zero and less than one");
	}
	const SpriteFrame& frame = this->sprite_sheet()->sprite_frame(this->frame());
	this->anchor_bounds_ = sprite_world_bounds(anchor, frame.source_rectangle(),
		0.0f, frame.origin());
	const RectangleF error = sprite_world_bounds(RectangleF::ZERO,
		frame.source_rectangle(), 0.0f, frame.origin(), 1.0f);
	this->pixel_padding_ = Vector2F(-error.x, -error.y);
	this->refresh_offset();
}

void DriftingVisual::refresh_offset()
{
	// Four world units per pencil cell, including motion. A bounded double
	// clock avoids losing small ticks late in a match or overflowing on a hitch.
	const double angle = (this->elapsed_ / this->period_ + this->phase_) *
		2.0 * std::numbers::pi;
	this->offset_ = static_cast<float>(std::round(
		static_cast<double>(this->distance_) * std::sin(angle) / 4.0) * 4.0);
}

void DriftingVisual::update(float dt)
{
	if (!std::isfinite(dt) || dt <= 0.0f ||
		(this->preferences_ != nullptr && this->preferences_->reduce_effects))
	{
		return;
	}
	this->elapsed_ = std::fmod(this->elapsed_ + static_cast<double>(dt), this->period_);
	this->refresh_offset();
}

float DriftingVisual::visible_offset() const
{
	return this->preferences_ != nullptr && this->preferences_->reduce_effects
		? 0.0f : this->offset_;
}

RectangleF DriftingVisual::rectangle() const
{
	RectangleF result = this->anchor_;
	result.x += this->visible_offset();
	return result;
}

void DriftingVisual::draw(DrawList& draw_list) const
{
	this->TextureObject::draw(draw_list, this->rectangle());
}

RectangleF DriftingVisual::bounds() const
{
	RectangleF result = this->anchor_bounds_;
	result.x += this->visible_offset();
	return result;
}

RectangleF DriftingVisual::cull_bounds(float units_per_pixel) const
{
	// Include the resting pose and both extremes, even if Reduce effects is
	// applied while paused. Non-cell distances can round outwards by half a cell.
	const float excursion = std::ceil(this->distance_ / 4.0f) * 4.0f;
	const Vector2F padding = this->pixel_padding_ * units_per_pixel;
	return RectangleF(this->anchor_bounds_.x - excursion - padding.x,
		this->anchor_bounds_.y - padding.y,
		this->anchor_bounds_.width + 2.0f * (excursion + padding.x),
		this->anchor_bounds_.height + 2.0f * padding.y);
}
