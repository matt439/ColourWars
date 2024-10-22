#include "pch.h"
#include "DebugText.h"

using namespace DirectX;
using namespace MattMath;
using namespace debug_text_consts;

void DebugText::draw_debug_info(const Player* player, int num_projectiles)
{
    const PlayerInputData& input = player->get_input();
    
    RectangleF rect = player->get_rectangle();
    Vector2F center = player->get_center();
    Vector2F vel = player->get_velocity();
    Camera camera = player->get_camera();
    Vector2F resolution = this->_resolution_manager->get_resolution_vec();
    Viewport viewport = player->get_viewport();
    float x_movement = player->get_input_x_movement();
    Vector2F shoot_direction = player->
        get_input_shoot_direction();
    float shoot_angle = input.shoot_angle;
    Vector2F left_stick = input.left_analog_stick;
    Vector2F right_stick = input.right_analog_stick;
    bool primary_shooting = player->
        get_input_primary_shoot();
    bool jump_pressed = player->
		get_input_jump_pressed();
    bool jump_held = player->
        get_input_jump_held();
    player_move_state move_state = player->get_move_state();
    player_state state = player->get_state();

    std::vector<std::string> lines;

    lines.push_back("rect: " + std::to_string(rect.x) + ", " +
        std::to_string(rect.y) + ", " + std::to_string(rect.width) + ", " +
        std::to_string(rect.height));
    lines.push_back("center: " + std::to_string(center.x) + ", " +
        std::to_string(center.y));
    lines.push_back("vel: " + std::to_string(vel.x) + ", " +
        std::to_string(vel.y));
    lines.push_back("camera: " + std::to_string(camera.translation.x) + ", " +
        std::to_string(camera.translation.y) + ", " + std::to_string(camera.scale));
    lines.push_back("resolution: " + std::to_string(resolution.x) + ", " +
        std::to_string(resolution.y));
    lines.push_back("viewport: " + std::to_string(viewport.x) + ", " +
        std::to_string(viewport.y) + ", " +
        std::to_string(viewport.width) + ", " +
        std::to_string(viewport.height));
    lines.push_back("x-input: " + std::to_string(x_movement));
    lines.push_back("shoot-direction: " + std::to_string(shoot_direction.x) +
        ", " + std::to_string(shoot_direction.y));
    lines.push_back("shoot-angle: " + std::to_string(shoot_angle));
    lines.push_back("left-stick: " + std::to_string(left_stick.x) + ", " +
        std::to_string(left_stick.y));
    lines.push_back("right-stick: " + std::to_string(right_stick.x) + ", " +
        std::to_string(right_stick.y));

    std::string primary_shooting_string = "primary-shooting: ";
    if (primary_shooting)
    {
        primary_shooting_string += "true";
    }
    else
    {
        primary_shooting_string += "false";
    }
    lines.push_back(primary_shooting_string);

    lines.push_back("dt: " + std::to_string(this->get_dt()));
    lines.push_back("fps: " + std::to_string(1.0f / this->get_dt()));
    lines.push_back("proj_count: " + std::to_string(num_projectiles));
    lines.push_back("jump_pressed: " + std::to_string(jump_pressed));
    lines.push_back("jump_held: " + std::to_string(jump_held));

    std::string move_state_string = "move_state: ";
    switch (move_state)
    {
    case player_move_state::ON_GROUND:
        move_state_string += "on_ground";
        break;
    case player_move_state::ON_DROP_DOWN_GROUND:
        move_state_string += "on_drop_down_ground";
		break;
    case player_move_state::ON_CEILING:
        move_state_string += "on_ceiling";
        break;
    case player_move_state::IN_AIR:
        move_state_string += "in_air";
        break;
    case player_move_state::JUMPING:
        move_state_string += "jumping";
        break;
    };
    lines.push_back(move_state_string);

    std::string state_string = "state: ";
    switch (state)
    {
    case player_state::ALIVE:
        state_string += "alive";
        break;
    case player_state::DEAD:
        state_string += "dead";
        break;
    };
    lines.push_back(state_string);

    Vector2F text_pos = DEBUG_POSITION;

    SpriteFont* sprite_font = this->get_resource_manager()->get_sprite_font(
        DEBUG_FONT);

    this->get_sprite_batch()->Begin();

    for (size_t i = 0; i < lines.size(); i++)
    {
        Vector2F shadow_pos = text_pos + DEBUG_SHADOW_OFFSET;
        
        sprite_font->DrawString(
            this->get_sprite_batch(),
            lines[i].c_str(),
            shadow_pos.get_xm_vector(),
            DEBUG_SHADOW_COLOR.get_xm_vector(),
            0.0f,
            Vector2F::ZERO.get_xm_vector(),
            DEBUG_SHADOW_SCALE);

        sprite_font->DrawString(
            this->get_sprite_batch(),
            lines[i].c_str(),
            text_pos.get_xm_vector(),
            DEBUG_COLOR.get_xm_vector(),
            0.0f,
            Vector2F::ZERO.get_xm_vector(),
            DEBUG_FONT_SCALE);

        text_pos.y += DEBUG_LINE_SPACING;
    }

    this->get_sprite_batch()->End();
}