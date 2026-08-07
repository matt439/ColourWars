#include "game/pch.h"
#include "game/objects/paint_tile.h"

using namespace DirectX;
using namespace mattmath;
using namespace paint_tile_consts;

PaintTile::PaintTile(const RectangleF& rectangle,
	const std::string& sheet_name,
	const std::string& frame_name,
	RenderResources* render_resources,
	const TeamColour& team_colours,
	const float* dt,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	TextureObject(sheet_name, frame_name, render_resources,
		color, rotation, origin, effects, layer_depth),
	rectangle_(rectangle),
	team_colours_(team_colours),
	dt_(dt)
{
	this->splash_ = PaintTileSplash(
		dt, SPLASH_RECTANGLE, SPLASH_SPRITE_SHEET_NAME, SPLASH_ANIMATION_STRIP_NAME,
		render_resources);

	// The splash is always centred on the tile and the tile never moves, so
	// this is set once here rather than re-assigned on every draw call.
	this->splash_.set_rectangle_center(rectangle.get_center());
}

void PaintTile::update()
{
	this->splash_.update();
}
void PaintTile::draw(SpriteBatch* sprite_batch, const Camera& camera)
{
	// Pure read. StructurePaintable::draw walks every tile it owns, and is
	// itself entered by every render worker at once, so assigning the tile's
	// and the splash's colour here was an unsynchronised write to state shared
	// by all of them.
	if (this->team_ == PlayerTeam::none)
	{
		return;
	}
	const Colour tint = this->team_colours_.get_team_colour(this->team_);

	this->splash_.draw_with_colour(sprite_batch, camera, tint);

	this->TextureObject::draw_with(sprite_batch, this->rectangle_, camera,
		this->get_frame(), tint, this->get_origin(),
		this->get_effects(), this->get_draw_rotation());
}
void PaintTile::draw(SpriteBatch* sprite_batch)
{
	this->draw(sprite_batch, Camera::DEFAULT_CAMERA);
}
float PaintTile::get_area() const
{
	return this->rectangle_.get_area();
}
PlayerTeam PaintTile::get_team() const
{
	return this->team_;
}
bool PaintTile::is_colliding(const ICollisionGameObject* other) const
{
    // aabb check
    if (!this->get_shape()->AABB_intersects(other->get_shape()))
    {
        return false;
    }
    else // AABBs are intersecting
    {
        // if the other object is a rectangle, then we have a collision
        // since the AABB check passed
        ShapeType other_shape_type = other->get_shape()->get_shape_type();
        if (other_shape_type == ShapeType::rectangle)
        {
            return true;
        }
    }

    // narrow phase check
    if (this->get_shape()->intersects(other->get_shape()))
    {
        return true;
    }

    return false;
}
const Shape* PaintTile::get_shape() const
{
    return &this->rectangle_;
}
void PaintTile::on_collision(const ICollisionGameObject* other)
{
	CollisionObjectType other_type = other->get_collision_object_type();
	PlayerTeam other_team;
	if (other_type == CollisionObjectType::projectile_spray_team_a ||
		other_type == CollisionObjectType::projectile_jet_team_a ||
		other_type == CollisionObjectType::projectile_rolling_team_a ||
		other_type == CollisionObjectType::projectile_ball_team_a ||
		other_type == CollisionObjectType::projectile_mist_team_a)
	{
		other_team = PlayerTeam::a;
	}
	else if (other_type == CollisionObjectType::projectile_spray_team_b ||
		other_type == CollisionObjectType::projectile_jet_team_b ||
		other_type == CollisionObjectType::projectile_rolling_team_b ||
		other_type == CollisionObjectType::projectile_ball_team_b ||
		other_type == CollisionObjectType::projectile_mist_team_b)
	{
		other_team = PlayerTeam::b;
	}
	else
	{
		throw std::exception("PaintTile::on_collision() - other_type is not a projectile");
	}
	this->team_ = other_team;
	this->splash_.reset_and_play();
}
CollisionObjectType PaintTile::get_collision_object_type() const
{
	return CollisionObjectType::paint_tile;
}
bool PaintTile::get_for_deletion() const
{
	return false;
}
bool PaintTile::is_visible_in_viewport(const RectangleF& view) const
{
	return this->rectangle_.intersects(view);
}

PaintTileSplash::PaintTileSplash(const float* dt,
	const RectangleF& rectangle,
	const std::string& sheet_name,
	const std::string& animation_strip_name,
	RenderResources* render_resources,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	AnimationObject(dt, sheet_name, animation_strip_name,
		render_resources,
		color, rotation, origin, effects, layer_depth),
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
void PaintTileSplash::update()
{
	this->AnimationObject::update();
}
void PaintTileSplash::draw(SpriteBatch* sprite_batch, const Camera& camera)
{
	this->AnimationObject::draw(sprite_batch, this->rectangle_, camera);
}
void PaintTileSplash::draw_with_colour(SpriteBatch* sprite_batch,
	const Camera& camera, const Colour& colour) const
{
	this->AnimationObject::draw_with(sprite_batch, this->rectangle_, camera,
		colour, this->get_effects());
}
void PaintTileSplash::draw(SpriteBatch* sprite_batch)
{
	this->AnimationObject::draw(sprite_batch, this->rectangle_);
}
bool PaintTileSplash::is_visible_in_viewport(const RectangleF& view) const
{
	return this->rectangle_.intersects(view);
}