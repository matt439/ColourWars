#ifndef PLAYERCONSTS_H
#define PLAYERCONSTS_H

#include "player_team.h"
#include "wep_type.h"
#include "Colour.h"

struct player_animation_info
{
	std::string sprite_sheet = "";
    std::string animation = "";
    std::string uniform_texture = "";

    // if frame_time is FLT_MIN, then the frame_time will be set to the default
    float frame_time = FLT_MIN;
};

namespace player_consts
{
    const MattMath::Vector2F DEFAULT_STARTING_POSITION = { 0.0f, 0.0f };
    const MattMath::Vector2F DEFAULT_SIZE = { 52.0f, 120.0f };
    const MattMath::RectangleF DEFAULT_BOUNDS = { DEFAULT_STARTING_POSITION, DEFAULT_SIZE };
    const MattMath::RectangleF DEFAULT_COLLISION_BOUNDS = { 0.0f, 0.0f, 50.0f, 50.0f };
    const MattMath::Vector2F STARTING_DX = { 0.0f, 0.0f };
    const MattMath::Vector2F STARTING_VELOCITY = { 0.0f, 0.0f };
    const MattMath::Vector2F MAX_VELOCITY = { 800.0f, 1900.0f };
    constexpr float X_INITIAL_VELOCITY = 100.0f;
    constexpr float X_ACCELERATION = 900.0f;
    constexpr float X_DEACCELERATION = 2500.0f;
    constexpr float GRAVITY = 2000.0f;
    constexpr float JUMP_LAUNCH_VELOCITY = -600.0f;
    constexpr float JUMP_MAX_TIME = 0.35f;
    constexpr float JUMP_POWER = 0.5f;
    constexpr float STARTING_HEALTH = 1.0f;
    constexpr float HEALTH_REGEN_DELAY = 5.0f;
    constexpr float HEALTH_REGEN_RATE = 1.0f;
    constexpr float DAMAGE_SOUND_DELAY = 0.25f;
    constexpr float DROP_DOWN_VELOCITY = 100.0f;
    constexpr float DROP_DOWN_ANALOG_THRESHOLD = 0.8f;

    constexpr float JUMP_ACCELERATION = -1300.0f;
    constexpr float RESPAWN_DELAY = 3.0f;
    constexpr int DEFAULT_PLAYER_NUM = 0;
    const player_team DEFAULT_TEAM = player_team::A;
    const MattMath::Colour DEFAULT_TEAM_COLOUR = colour_consts::GRAY;
    const wep_type DEFAULT_PRIMARY = wep_type::SPRAYER;
    const wep_type DEFAULT_SECONDARY = wep_type::GRENADE_STD;
    const std::string DEFAULT_SHEET = "sprite_sheet_1";
    const std::string DEFAULT_ANIMATION_STRIP = "player_right_walk";
    const std::string DEFAULT_UNIFORM_TEXTURE = "player_right_walk_uniform";

    const MattMath::Colour DEFAULT_PLAYER_COLOUR = colour_consts::BURLYWOOD;

    const std::string SOUND_BANK = "sound_bank_1";
    const std::string DAMAGE_SOUND = "SplittingFireWood";
    constexpr float DAMAGE_SOUND_VOLUME = 0.1f;
    const std::string DEATH_SOUND = "DeathImpactYellSingle";
    constexpr float DEATH_SOUND_VOLUME = 1.0f;
    const std::string JUMP_SOUND = "CartoonBoing";
    constexpr float JUMP_SOUND_VOLUME = 0.2f;
    const std::string RESPAWN_SOUND = "ThunderCrack";
    constexpr float RESPAWN_SOUND_VOLUME = 0.2f;

    constexpr float SPRAY_DAMAGE = 0.01f;
    constexpr float JET_DAMAGE = 0.334f;
    constexpr float ROLLING_DAMAGE = 0.015f;
    constexpr float BALL_DAMAGE = 0.201f;
    constexpr float MIST_DAMAGE = 0.01f;

    const static std::string COUNTDOWN_TEXT = "3";
    const static std::string COUNTDOWN_FONT_NAME = "gill_sans_mt_bold_72";
    const static MattMath::Colour COUNTDOWN_COLOUR = colour_consts::DARK_GRAY;
    const static MattMath::Colour COUNTDOWN_SHADOW_COLOUR = colour_consts::BLACK;
    constexpr float COUNTDOWN_SCALE = 1.0f;
    const static MattMath::Vector2F COUNTDOWN_SHADOW_OFFSET = { 2.0f, 2.0f };
    constexpr float COUNTDOWN_TEXT_WIDTH = 400.0f;
    constexpr float COUNTDOWN_TEXT_HEIGHT = 600.0f;

    const player_animation_info DEFAULT_ANIMATION_INFO =
    {
		DEFAULT_SHEET,
		DEFAULT_ANIMATION_STRIP,
		DEFAULT_UNIFORM_TEXTURE,
        FLT_MIN
	};
    const player_animation_info ANIMATION_INFO_WALK =
    {
        DEFAULT_SHEET,
        "player_right_walk",
        "player_right_walk_uniform",
        FLT_MIN
    };
    const player_animation_info ANIMATION_INFO_RUNNING =
    {
        DEFAULT_SHEET,
        "player_right_walk",
        "player_right_walk_uniform",
        0.05f
    };
    const player_animation_info ANIMATION_INFO_IDLE =
    {
		DEFAULT_SHEET,
		"player_right_idle",
		"player_right_walk_uniform",
        FLT_MIN
	};
    const player_animation_info ANIMATION_INFO_JUMPING =
    {
        DEFAULT_SHEET,
        "player_right_jumping",
        "player_right_walk_uniform",
        FLT_MIN
    };
    const player_animation_info ANIMATION_INFO_FALLING =
    {
        DEFAULT_SHEET,
        "player_right_falling",
        "player_right_walk_uniform",
        FLT_MIN
    };
}

#endif // !PLAYERCONSTS_H
