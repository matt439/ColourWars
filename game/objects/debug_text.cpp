#include "game/pch.h"
#include "game/objects/debug_text.h"

using namespace DirectX;
using namespace mattmath;
using namespace debug_text_consts;
using namespace artattack;

DebugText::DebugText(RenderResources* render_resources,
    const ResolutionManager* resolution_manager) :
    Drawer(render_resources),
    resolution_manager_(resolution_manager),
    font_(render_resources->resolve_sprite_font(DEBUG_FONT))
{
	
}


void DebugText::draw_debug_info(SpriteBatch* sprite_batch,
    const Player* player, int num_projectiles, float dt) const
{
    const PlayerInputData& input = player->input();
    
    RectangleF rect = player->rectangle();
    Vector2F center = player->center();
    Vector2F vel = player->velocity();
    Camera camera = player->camera();
    Vector2F resolution = this->resolution_manager_->resolution_vec();
    Viewport viewport = player->viewport();
    float x_movement = player->input_x_movement();
    Vector2F shoot_direction = player->
        input_shoot_direction();
    float shoot_angle = input.shoot_angle;
    Vector2F left_stick = input.left_analog_stick;
    Vector2F right_stick = input.right_analog_stick;
    bool primary_shooting = player->
        input_primary_shoot();
    bool jump_pressed = player->
		input_jump_pressed();
    bool jump_held = player->
        input_jump_held();
    PlayerState state = player->state();

    std::vector<std::wstring> lines;

    lines.push_back(L"rect: " + std::to_wstring(rect.x) + L", " +
        std::to_wstring(rect.y) + L", " + std::to_wstring(rect.width) + L", " +
        std::to_wstring(rect.height));
    lines.push_back(L"center: " + std::to_wstring(center.x) + L", " +
        std::to_wstring(center.y));
    lines.push_back(L"vel: " + std::to_wstring(vel.x) + L", " +
        std::to_wstring(vel.y));
    lines.push_back(L"camera: " + std::to_wstring(camera.translation.x) + L", " +
        std::to_wstring(camera.translation.y) + L", " + std::to_wstring(camera.scale));
    lines.push_back(L"resolution: " + std::to_wstring(resolution.x) + L", " +
        std::to_wstring(resolution.y));
    lines.push_back(L"viewport: " + std::to_wstring(viewport.x) + L", " +
        std::to_wstring(viewport.y) + L", " +
        std::to_wstring(viewport.width) + L", " +
        std::to_wstring(viewport.height));
    lines.push_back(L"x-input: " + std::to_wstring(x_movement));
    lines.push_back(L"shoot-direction: " + std::to_wstring(shoot_direction.x) +
        L", " + std::to_wstring(shoot_direction.y));
    lines.push_back(L"shoot-angle: " + std::to_wstring(shoot_angle));
    lines.push_back(L"left-stick: " + std::to_wstring(left_stick.x) + L", " +
        std::to_wstring(left_stick.y));
    lines.push_back(L"right-stick: " + std::to_wstring(right_stick.x) + L", " +
        std::to_wstring(right_stick.y));

    std::wstring primary_shooting_string = L"primary-shooting: ";
    if (primary_shooting)
    {
        primary_shooting_string += L"true";
    }
    else
    {
        primary_shooting_string += L"false";
    }
    lines.push_back(primary_shooting_string);

    lines.push_back(L"dt: " + std::to_wstring(dt));
    lines.push_back(L"fps: " + std::to_wstring(1.0f / dt));
    lines.push_back(L"proj_count: " + std::to_wstring(num_projectiles));
    lines.push_back(L"jump_pressed: " + std::to_wstring(jump_pressed));
    lines.push_back(L"jump_held: " + std::to_wstring(jump_held));

    std::wstring move_state_string = L"move_state: ";
	move_state_string += widen(player->player_move_state_string());

    lines.push_back(move_state_string);

    std::wstring state_string = L"state: ";
    switch (state)
    {
    case PlayerState::alive:
        state_string += L"alive";
        break;
    case PlayerState::dead:
        state_string += L"dead";
        break;
    };
    lines.push_back(state_string);

    Vector2F text_pos = DEBUG_POSITION;

    SpriteFont* sprite_font = this->render_resources()->sprite_font(
        this->font_);

    sprite_batch->Begin();

    for (size_t i = 0; i < lines.size(); i++)
    {
        Vector2F shadow_pos = text_pos + DEBUG_SHADOW_OFFSET;
        
        sprite_font->DrawString(
            sprite_batch,
            lines[i].c_str(),
            shadow_pos.xm_vector(),
            DEBUG_SHADOW_COLOR.xm_vector(),
            0.0f,
            Vector2F::ZERO.xm_vector(),
            DEBUG_SHADOW_SCALE);

        sprite_font->DrawString(
            sprite_batch,
            lines[i].c_str(),
            text_pos.xm_vector(),
            DEBUG_COLOR.xm_vector(),
            0.0f,
            Vector2F::ZERO.xm_vector(),
            DEBUG_FONT_SCALE);

        text_pos.y += DEBUG_LINE_SPACING;
    }

    sprite_batch->End();
}
