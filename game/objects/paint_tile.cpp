#include "game/pch.h"
#include "game/objects/paint_tile.h"
#include "game/objects/fixed_slab_geometry.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

PaintSplashClips PaintSplashClips::resolve(const PaintTuning& paint,
	const RenderResources& render_resources)
{
	PaintSplashClips clips;
	clips.initial = render_resources.resolve_sheet_strip(paint.splash_sheet, paint.splash_animation);
	for (size_t type = 0; type < clips.by_type.size(); ++type)
	{
		clips.by_type[type] = render_resources.resolve_sheet_strip(paint.splash_sheet,
			paint.splash_animations[type]);
	}
	return clips;
}

PaintTile::PaintTile(const RectangleF& rectangle,
	const PaintTuning* paint,
	const PaintSplashClips* splash_clips,
	RenderResources* render_resources,
	const TeamColour& team_colours,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth,
	bool along_x, bool leading) :
	TextureObject(paint->sheet, paint->frame, render_resources,
		color, rotation, origin, flip, layer_depth),
	rectangle_(rectangle),
	team_colours_(team_colours),
	paint_(paint),
	splash_clips_(splash_clips),
	wet_age_(paint->wet_duration),
	along_x_(along_x),
	leading_(leading),
	// Each tile owns at most one airborne splash; repeated contacts restart it.
	splash_(RectangleF(0.0f, 0.0f,
		paint->thickness * paint->splash_scale,
		paint->thickness * paint->splash_scale),
		splash_clips->initial, render_resources, Colour::white,
		along_x ? (leading ? 0.0f : PI) : (leading ? -PI / 2.0f : PI / 2.0f))
{
	// The tile is stationary, so the splash centre is fixed at construction.
	this->splash_.set_rectangle_center(rectangle.center());
}

void PaintTile::update(float dt)
{
	if (!(dt > 0.0f)) { return; }
	this->wet_age_ = std::min(this->wet_age_ + dt, this->paint_->wet_duration);
	this->splash_.update(dt);
}
void PaintTile::draw(DrawList& draw_list) const
{
	this->draw_splash(draw_list);
	this->draw_surface(draw_list);
}

void PaintTile::draw_splash(DrawList& draw_list) const
{
	if (this->team_ == PlayerTeam::none) { return; }
	this->splash_.draw_with_colour(draw_list,
		this->team_colours_.team_colour(this->team_));
}

void PaintTile::draw_surface(DrawList& draw_list) const
{
	// Pass tint as a draw parameter: render workers share this tile.
	if (this->team_ == PlayerTeam::none)
	{
		return;
	}
	const Colour tint = this->team_colours_.team_colour(this->team_);

	this->TextureObject::draw_with(draw_list, this->rectangle_,
		this->frame(), tint, this->origin(),
		this->flip(), this->draw_rotation());

	// A single inward edge follows the whole exposed face. Never border the
	// short ends of individual four-unit tiles: that would turn paint into a grid.
	const float depth = this->along_x_ ? this->rectangle_.height : this->rectangle_.width;
	const float edge = std::min(this->paint_->edge_width, depth);
	this->TextureObject::draw_with(draw_list, this->face_band(depth - edge, edge),
		this->frame(), Colour::black, Vector2F::ZERO, SpriteFlip::none, 0.0f);

	if (this->wet_age_ < this->paint_->wet_duration)
	{
		const float progress = this->wet_age_ / this->paint_->wet_duration;
		const float colour_depth = depth - edge;
		const float shine_depth = std::min(this->paint_->settle_depth, colour_depth);
		if (shine_depth > 0.0f)
		{
			const float offset = std::min(this->paint_->settle_depth * progress,
				colour_depth - shine_depth);
			const float amount = this->paint_->wet_highlight * (1.0f - progress);
			const Colour shine(tint.r + (1.0f - tint.r) * amount,
				tint.g + (1.0f - tint.g) * amount,
				tint.b + (1.0f - tint.b) * amount, 1.0f);
			this->TextureObject::draw_with(draw_list, this->face_band(offset, shine_depth),
				this->frame(), shine, Vector2F::ZERO, SpriteFlip::none, 0.0f);
		}
	}
}

void PaintTile::draw_splash_rigid(DrawList& draw_list,
	const FixedSlabGeometry& geometry) const
{
	if (this->team_ == PlayerTeam::none) { return; }
	this->splash_.draw_with_colour_rigid(draw_list,
		this->team_colours_.team_colour(this->team_), geometry);
}

void PaintTile::draw_surface_rigid(DrawList& draw_list,
	const FixedSlabGeometry& geometry) const
{
	this->draw_surface_base_rigid(draw_list, geometry, this->rectangle_);
	this->draw_surface_wet_rigid(draw_list, geometry);
}

void PaintTile::draw_rectangle_rigid(DrawList& draw_list,
	const FixedSlabGeometry& geometry, const RectangleF& local, const Colour& tint) const
{
	const Vector2F anchor = geometry.to_world(local.top_left());
	// The local rectangle starts at this exact rigid-frame point. A raw
	// source rect deliberately supplies no authored sprite-frame pivot.
	this->sprite_sheet()->draw(draw_list,
		this->sprite_sheet()->sprite_frame(this->frame()).source_rectangle(),
		RectangleF(anchor, local.size()), tint, geometry.tangent().angle(), Vector2F::ZERO,
		SpriteFlip::none, this->layer_depth());
}

void PaintTile::draw_surface_base_rigid(DrawList& draw_list,
	const FixedSlabGeometry& geometry, const RectangleF& local_band) const
{
	if (this->team_ == PlayerTeam::none) { return; }
	const Colour tint = this->team_colours_.team_colour(this->team_);
	this->draw_rectangle_rigid(draw_list, geometry, local_band, tint);
	const float depth = this->along_x_ ? local_band.height : local_band.width;
	const float edge = std::min(this->paint_->edge_width, depth);
	const RectangleF inner = this->along_x_
		? RectangleF(local_band.x, this->leading_ ? local_band.bottom() - edge : local_band.top(),
			local_band.width, edge)
		: RectangleF(this->leading_ ? local_band.right() - edge : local_band.left(), local_band.y,
			edge, local_band.height);
	this->draw_rectangle_rigid(draw_list, geometry, inner, Colour::black);
}

void PaintTile::draw_surface_wet_rigid(DrawList& draw_list,
	const FixedSlabGeometry& geometry) const
{
	if (this->team_ == PlayerTeam::none) { return; }
	const Colour tint = this->team_colours_.team_colour(this->team_);
	const float depth = this->along_x_ ? this->rectangle_.height : this->rectangle_.width;
	const float edge = std::min(this->paint_->edge_width, depth);
	if (this->wet_age_ < this->paint_->wet_duration)
	{
		const float progress = this->wet_age_ / this->paint_->wet_duration;
		const float colour_depth = depth - edge;
		const float shine_depth = std::min(this->paint_->settle_depth, colour_depth);
		if (shine_depth > 0.0f)
		{
			const float offset = std::min(this->paint_->settle_depth * progress,
				colour_depth - shine_depth);
			const float amount = this->paint_->wet_highlight * (1.0f - progress);
			this->draw_rectangle_rigid(draw_list, geometry, this->face_band(offset, shine_depth), Colour(
				tint.r + (1.0f - tint.r) * amount,
				tint.g + (1.0f - tint.g) * amount,
				tint.b + (1.0f - tint.b) * amount, 1.0f));
		}
	}
}

RectangleF PaintTile::face_band(float offset, float depth) const
{
	RectangleF band = this->rectangle_;
	if (this->along_x_)
	{
		band.y = this->leading_ ? this->rectangle_.top() + offset :
			this->rectangle_.bottom() - offset - depth;
		band.height = depth;
	}
	else
	{
		band.x = this->leading_ ? this->rectangle_.left() + offset :
			this->rectangle_.right() - offset - depth;
		band.width = depth;
	}
	return band;
}
float PaintTile::area() const
{
	return this->rectangle_.width * this->rectangle_.height;
}
PlayerTeam PaintTile::team() const
{
	return this->team_;
}
int PaintTile::player_num() const
{
	return this->player_num_;
}
void PaintTile::paint(PlayerTeam team, int player_num, ProjectileType type)
{
	// Friendly repaint retains the capturing player's credit; enemy repaint transfers it.
	if (this->team_ != team)
	{
		this->player_num_ = player_num;
	}
	this->team_ = team;
	this->wet_age_ = 0.0f;
	this->splash_.reset_and_play(this->splash_clips_->by_type.at(static_cast<size_t>(type)));
}
RectangleF PaintTile::bounds() const
{
	return this->rectangle_;
}

RectangleF PaintTile::visual_bounds() const
{
	return this->splash_.active() ? RectangleF::union_of(this->rectangle_,
		this->splash_.bounds()) : this->rectangle_;
}

PaintTileSplash::PaintTileSplash(
	const RectangleF& rectangle,
	RenderResources::SheetStrip clip,
	RenderResources* render_resources,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	colourwars::AnimationObject(clip, render_resources,
		color, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{
	this->read_clip(clip);
	// Construction must not start a decorative impact before paint arrives.
	this->stop();
}

void PaintTileSplash::read_clip(RenderResources::SheetStrip clip)
{
	this->rigid_clip_ = clip;
	// The object already holds clip.sheet(), so this is two indexed reads.
	const AnimationStrip& strip = this->sprite_sheet()->animation_strip(clip.strip());
	this->frame_time_ = strip.frame_time();
	this->frame_count_ = strip.frame_count();
	const RectangleI& source = strip.frame_rect(0);
	this->set_origin(Vector2F(static_cast<float>(source.width) / 2.0f,
		static_cast<float>(source.height) / 2.0f));
}

void PaintTileSplash::set_rectangle_center(const Vector2F& center)
{
	this->rectangle_.set_position_at_center(center);
}
void PaintTileSplash::reset_and_play()
{
	this->age_ = 0.0f;
	this->active_ = true;
	this->reset();
	this->play();
}
void PaintTileSplash::reset_and_play(RenderResources::SheetStrip clip)
{
	this->set_animation_strip_and_reset(clip);
	this->read_clip(clip);
	this->reset_and_play();
}
bool PaintTileSplash::active() const
{
	return this->active_;
}
void PaintTileSplash::set_colour(const Colour& colour)
{
	this->colourwars::AnimationObject::set_colour(colour);
}
void PaintTileSplash::update(float dt)
{
	if (!(dt > 0.0f) || !this->active_) { return; }
	this->age_ += dt;
	if (this->age_ >= this->frame_time_ * this->frame_count_)
	{
		this->active_ = false;
		this->stop();
		return;
	}
	// Select by elapsed time so a long tick also retires/skips frames correctly.
	this->set_frame_index(std::min(static_cast<int>(this->age_ / this->frame_time_),
		this->frame_count_ - 1));
}
void PaintTileSplash::draw(DrawList& draw_list) const
{
	if (!this->active_) { return; }
	this->colourwars::AnimationObject::draw(draw_list,
		RectangleF(this->rectangle_.center(), this->rectangle_.size()));
}
void PaintTileSplash::draw_with_colour(DrawList& draw_list,
	const Colour& colour) const
{
	if (!this->active_) { return; }
	this->colourwars::AnimationObject::draw_with(draw_list,
		RectangleF(this->rectangle_.center(), this->rectangle_.size()),
		colour, this->flip());
}
void PaintTileSplash::draw_with_colour_rigid(DrawList& draw_list,
	const Colour& colour, const FixedSlabGeometry& geometry) const
{
	if (!this->active_) { return; }
	const AnimationStrip& strip = this->sprite_sheet()->animation_strip(this->rigid_clip_.strip());
	const int index = std::min(static_cast<int>(this->age_ / this->frame_time_),
		this->frame_count_ - 1);
	this->sprite_sheet()->draw(draw_list, strip.frame_rect(index),
		RectangleF(geometry.to_world(this->rectangle_.center()), this->rectangle_.size()),
		colour, geometry.tangent().angle() + this->draw_rotation(), this->origin(),
		this->flip(), this->layer_depth());
}
RectangleF PaintTileSplash::bounds() const
{
	return this->rectangle_;
}
