#include "game/pch.h"
#include "game/objects/paint_tile.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

PaintTile::PaintTile(const RectangleF& rectangle,
	const PaintTuning* paint,
	RenderResources* render_resources,
	const TeamColour& team_colours,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	TextureObject(paint->sheet, paint->frame, render_resources,
		color, rotation, origin, flip, layer_depth),
	rectangle_(rectangle),
	team_colours_(team_colours),
	// Extend the splash by the tile's depth on both sides.
	splash_(RectangleF(0.0f, 0.0f,
		paint->thickness * 2.0f, paint->thickness * 2.0f),
		paint->splash_sheet, paint->splash_animation, render_resources)
{
	// The tile is stationary, so the splash centre is fixed at construction.
	this->splash_.set_rectangle_center(rectangle.center());
}

void PaintTile::update(float dt)
{
	this->splash_.update(dt);
}
void PaintTile::draw(DrawList& draw_list) const
{
	// Pass tint as a draw parameter: render workers share this tile.
	if (this->team_ == PlayerTeam::none)
	{
		return;
	}
	const Colour tint = this->team_colours_.team_colour(this->team_);

	this->splash_.draw_with_colour(draw_list, tint);

	this->TextureObject::draw_with(draw_list, this->rectangle_,
		this->frame(), tint, this->origin(),
		this->flip(), this->draw_rotation());
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
void PaintTile::paint(PlayerTeam team, int player_num)
{
	// Friendly repaint retains the capturing player's credit; enemy repaint transfers it.
	if (this->team_ != team)
	{
		this->player_num_ = player_num;
	}
	this->team_ = team;
	this->splash_.reset_and_play();
}
RectangleF PaintTile::bounds() const
{
	return this->rectangle_;
}

PaintTileSplash::PaintTileSplash(
	const RectangleF& rectangle,
	const std::string& sheet_name,
	const std::string& animation_strip_name,
	RenderResources* render_resources,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	AnimationObject(sheet_name, animation_strip_name,
		render_resources,
		color, rotation, origin, flip, layer_depth),
	rectangle_(rectangle)
{

}

void PaintTileSplash::set_rectangle_center(const Vector2F& center)
{
	this->rectangle_.set_position_at_center(center);
}
void PaintTileSplash::reset_and_play()
{
	this->reset();
	this->play();
}
void PaintTileSplash::set_colour(const Colour& colour)
{
	this->AnimationObject::set_colour(colour);
}
void PaintTileSplash::update(float dt)
{
	this->AnimationObject::update(dt);
}
void PaintTileSplash::draw(DrawList& draw_list) const
{
	this->AnimationObject::draw(draw_list, this->rectangle_);
}
void PaintTileSplash::draw_with_colour(DrawList& draw_list,
	const Colour& colour) const
{
	this->AnimationObject::draw_with(draw_list, this->rectangle_,
		colour, this->flip());
}
RectangleF PaintTileSplash::bounds() const
{
	return this->rectangle_;
}
