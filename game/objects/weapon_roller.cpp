#include "game/pch.h"
#include "game/objects/weapon_roller.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;


WeaponRoller::WeaponRoller(const GameTuning* tuning,
    PlayerTeam team,
    int player_num,
    int sound_index,
    const Colour& team_colour,
    WeaponType type,
    const Vector2F& player_center,
    RenderResources* render_resources,
    const AudioResources* audio_resources,
    const PlaySettings* preferences,
    const Colour& color,
    float rotation,
    const Vector2F& origin,
    SpriteFlip flip,
    float layer_depth) :
    Weapon(tuning,
        team, player_num, sound_index, team_colour, type, player_center,
        render_resources, audio_resources, preferences, color, rotation, origin, flip, layer_depth)
{

}

void WeaponRoller::update_movement_and_rotation(PlayerInputData /*input*/,
    const Vector2F& player_center,
    const Vector2F& /*player_velocity*/,
    bool player_facing_right)
{
    if (player_facing_right)  
    {
        this->set_rotation(PI / 4.0f);
    }
    else
    {
        this->set_rotation(3.0f * PI / 4.0f);
    }
    this->set_player_center(player_center);

    if (facing_left(this->rotation()))
    {
        this->set_invert_y(true);
    }
    else
    {
        this->set_invert_y(false);
    }
}

labrador::Colour WeaponRoller::paint_colour() const
{
    return this->shooting_this_update_
        ? this->team_colour()
        : labrador::Colour::white;
}
