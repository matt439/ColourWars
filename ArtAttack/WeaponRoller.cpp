#include "pch.h"
#include "WeaponRoller.h"

using namespace DirectX;
using namespace MattMath;
using namespace weapon_consts;


WeaponRoller::WeaponRoller(player_team team,
    int player_num,
    const Colour& team_colour,
    wep_type type,
    const Vector2F& player_center,
    SpriteBatch* sprite_batch,
    ResourceManager* resource_manager,
    const float* dt,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteEffects effects,
    float layer_depth) :
    Weapon(DETAILS_ROLLER,
        team, player_num, team_colour, type, player_center,
        sprite_batch, resource_manager, dt,
        color, rotation, origin, effects, layer_depth)
{

}

void WeaponRoller::update_movement_and_rotation(PlayerInputData input,
    const Vector2F& player_center,
    const Vector2F& player_velocity,
    bool player_facing_right)
{
    if (player_facing_right)  
    {
        this->set_rotation(PI / 4.0f);
    }
    else //player facing left
    {
        this->set_rotation(3.0f * PI / 4.0f);
    }
    this->set_gun_player_aligned(true);
    

    this->set_player_center(player_center);

    //gun facing left
    if (facing_left(this->get_rotation()))
    {
        this->set_invert_y(true);
    }
    //gun facing right
    else
    {
        this->set_invert_y(false);
    }
}

void WeaponRoller::draw(const Camera& camera, bool debug)
{
    //draw weapon
    Vector2F draw_pos = this->get_draw_pos();
    auto draw_rectangle = RectangleF(draw_pos, this->get_details().size);

    Vector2F origin = this->calculate_sprite_origin(
        this->get_details().size, rotation_origin::LEFT_CENTER);

    SpriteEffects effects = SpriteEffects_None;
    bool invert_y = this->get_invert_y();
    bool invert_x = this->get_invert_x();
    if (invert_y && invert_x)
    {
        effects = SpriteEffects_FlipBoth;
    }
    else if (invert_y)
    {
        effects = SpriteEffects_FlipVertically;
    }
    else if (invert_x)
    {
        effects = SpriteEffects_FlipHorizontally;
    }

    TextureObject::set_element_name(this->get_details().frame_name);
    DrawObject::set_origin(origin);
    DrawObject::set_effects(effects);
    DrawObject::set_draw_rotation(this->get_rotation());

    if (this->_shooting_this_update)
	{
        DrawObject::set_colour(this->get_team_colour());
	}
    else
    {
        DrawObject::set_colour(colour_consts::WHITE);
    }

    TextureObject::draw(draw_rectangle, camera);

    if (debug)
    {
        // draw nozzle
        TextureObject::set_element_name("nozzle");
        RectangleF draw_rectangle_noz = this->get_nozzle_rectangle();
        Vector2F origin_noz = calculate_sprite_origin(
            get_nozzle_size(), rotation_origin::CENTER);

        TextureObject::set_origin(origin_noz);
        TextureObject::set_effects(SpriteEffects_None);
        TextureObject::set_draw_rotation(0.0f);

        TextureObject::draw(draw_rectangle_noz, camera);
    }
}
