#include "game/pch.h"
#include "game/objects/paint_tile.h"

using namespace DirectX;
using namespace mattmath;
using namespace paint_tile_consts;
using namespace artattack;

PaintTile::PaintTile(const RectangleF& rectangle,
	const std::string& sheet_name,
	const std::string& frame_name,
	RenderResources* render_resources,
	const TeamColour& team_colours,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteFlip flip,
	float layer_depth) :
	TextureObject(sheet_name, frame_name, render_resources,
		color, rotation, origin, flip, layer_depth),
	rectangle_(rectangle),
	team_colours_(team_colours)
{
	this->splash_ = PaintTileSplash(
		SPLASH_RECTANGLE, SPLASH_SPRITE_SHEET_NAME, SPLASH_ANIMATION_STRIP_NAME,
		render_resources);

	// The splash is always centred on the tile and the tile never moves, so
	// this is set once here rather than re-assigned on every draw call.
	this->splash_.set_rectangle_center(rectangle.center());
}

void PaintTile::update(float dt)
{
	this->splash_.update(dt);
}
void PaintTile::draw(DrawList& draw_list) const
{
	// Pure read. StructurePaintable::draw walks every tile it owns, and is
	// itself entered by every render worker at once, so assigning the tile's
	// and the splash's colour here was an unsynchronised write to state shared
	// by all of them.
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
	return this->rectangle_.area();
}
PlayerTeam PaintTile::team() const
{
	return this->team_;
}
void PaintTile::paint(PlayerTeam team)
{
	// The team came off the projectile once, at the structure, instead of
	// being re-derived from a ten-way enum comparison per tile - which ran
	// for every tile of the structure that a projectile touched, and
	// recovered the same answer every time.
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