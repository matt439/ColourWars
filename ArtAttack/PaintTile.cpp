#include "pch.h"
#include "PaintTile.h"

using namespace DirectX;
using namespace MattMath;
using namespace paint_tile_consts;

PaintTile::PaintTile(const RectangleF& rectangle,
	const std::string& sheet_name,
	const std::string& frame_name,
	SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	const team_colour& team_colours,
	const float* dt,
	const Colour& color,
	float rotation,
	const Vector2F& origin,
	SpriteEffects effects,
	float layer_depth) :
	TextureObject(sheet_name, frame_name,
		sprite_batch, resource_manager,
		color, rotation, origin, effects, layer_depth),
	_rectangle(rectangle),
	_team_colours(team_colours),
	_dt(dt)
{
	//this->_splash = std::make_unique<PaintTileSplash>(
	//	dt, SPLASH_RECTANGLE, SPLASH_SPRITE_SHEET_NAME, SPLASH_ANIMATION_STRIP_NAME,
	//	sprite_batch,
	//	resource_manager, color);

	this->_splash = PaintTileSplash(
		dt, SPLASH_RECTANGLE, SPLASH_SPRITE_SHEET_NAME, SPLASH_ANIMATION_STRIP_NAME,
		sprite_batch,
		resource_manager);
}

void PaintTile::update()
{
	//if (this->_splash_timer < SPLASH_DURATION)
	//{

	//}
	//this->_splash_timer += *this->_dt;

	this->_splash.update();
}
void PaintTile::draw(const Camera& camera)
{
	switch (this->_team)
	{
	case player_team::A:
		this->set_colour(this->_team_colours.get_team_colour(player_team::A));
		this->_splash.set_colour(this->_team_colours.get_team_colour(player_team::A));
		break;
	case player_team::B:
		this->set_colour(this->_team_colours.get_team_colour(player_team::B));
		this->_splash.set_colour(this->_team_colours.get_team_colour(player_team::B));
		break;
	default: // player_team::NONE
		return;
	}
	this->_splash.set_rectangle_center(this->_rectangle.get_center());
	this->_splash.draw(camera);

	this->TextureObject::draw(this->_rectangle, camera);
	//if (this->_splash_timer < SPLASH_DURATION)
	//{
	//	this->_splash.set_rectangle_center(this->_rectangle.get_center());
	//	this->_splash.draw(camera);
	//}

}
void PaintTile::draw()
{
    switch (this->_team)
    {
	case player_team::A:
		this->set_colour(this->_team_colours.get_team_colour(player_team::A));
		this->_splash.set_colour(this->_team_colours.get_team_colour(player_team::A));
		break;
	case player_team::B:
		this->set_colour(this->_team_colours.get_team_colour(player_team::B));
		this->_splash.set_colour(this->_team_colours.get_team_colour(player_team::B));
		break;
	default: // player_team::NONE
		return;
	}
	this->_splash.set_rectangle_center(this->_rectangle.get_center());
	this->_splash.draw();

	this->TextureObject::draw(this->_rectangle);


}
float PaintTile::get_area() const
{
	return this->_rectangle.get_area();
}
player_team PaintTile::get_team() const
{
	return this->_team;
}
//void PaintTile::set_team(player_team team)
//{
//	this->_team = team;
//}
//const RectangleF& PaintTile::get_rectangle() const
//{
//	return this->_rectangle;
//}
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
        shape_type other_shape_type = other->get_shape()->get_shape_type();
        if (other_shape_type == shape_type::RECTANGLE)
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
    return &this->_rectangle;
}
void PaintTile::on_collision(const ICollisionGameObject* other)
{
	collision_object_type other_type = other->get_collision_object_type();
	player_team other_team;
	if (other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_JET_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_BALL_TEAM_A ||
		other_type == collision_object_type::PROJECTILE_MIST_TEAM_A)
	{
		other_team = player_team::A;
	}
	else if (other_type == collision_object_type::PROJECTILE_SPRAY_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_JET_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_ROLLING_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_BALL_TEAM_B ||
		other_type == collision_object_type::PROJECTILE_MIST_TEAM_B)
	{
		other_team = player_team::B;
	}
	else
	{
		throw std::exception("PaintTile::on_collision() - other_type is not a projectile");
	}
	this->_team = other_team;
	//this->_splash_timer = 0.0f;
	this->_splash.reset_and_play();
	//this->_sound_bank->play_wave(SOUND_NAME, SOUND_VOLUME);
}
collision_object_type PaintTile::get_collision_object_type() const
{
	return collision_object_type::PAINT_TILE;
}
bool PaintTile::get_for_deletion() const
{
	return false;
}
bool PaintTile::is_visible_in_viewport(const RectangleF& view) const
{
	return this->_rectangle.intersects(view);
}
//void play_splash_sound()
//{
//	this->_sound_bank = resource_manager->get_sound_bank(SOUND_BANK_NAME);
//}

void PaintTileSplash::set_rectangle_center(const Vector2F& center)
{
	this->_rectangle.set_position_at_center(center);
}
void PaintTileSplash::reset_and_play()
{
	this->AnimationObject::reset();
	this->AnimationObject::play();
}
void PaintTileSplash::set_colour(const Colour& colour)
{
	this->AnimationObject::set_colour(colour);
}
void PaintTileSplash::update()
{
	this->AnimationObject::update();
}
void PaintTileSplash::draw(const Camera& camera)
{
	this->AnimationObject::draw(this->_rectangle, camera);
}
void PaintTileSplash::draw()
{
	this->AnimationObject::draw(this->_rectangle);
}
bool PaintTileSplash::is_visible_in_viewport(const RectangleF& view) const
{
	return this->_rectangle.intersects(view);
}