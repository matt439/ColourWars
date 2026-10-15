#include "game/pch.h"
#include "game/objects/debug_text.h"
#include <cmath>
#include <cstdio>

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

DebugText::DebugText(RenderResources* render_resources,
    const ResolutionManager* resolution_manager,
    const DebugOverlayPresentation* look) :
    render_resources_(render_resources),
    resolution_manager_(resolution_manager),
    look_(look),
    font_(render_resources->resolve_sprite_font(look->font))
{
	
}

RenderResources* DebugText::render_resources() const
{
    return this->render_resources_;
}


std::wstring DebugText::frame_rate_text(double frame_ms)
{
    if (!(frame_ms > 0.0) || !std::isfinite(frame_ms)) { return L"render fps: n/a"; }
    const double fps = 1000.0 / frame_ms;
    if (!std::isfinite(fps)) { return L"render fps: n/a"; }
    char text[64]{};
    const int size = std::snprintf(text, sizeof(text), "render fps: %.1f", fps);
    return size > 0 && size < static_cast<int>(sizeof(text)) ? widen(text) : L"render fps: n/a";
}

void DebugText::draw_debug_info(DrawList& draw_list,
    const Player* player, int num_projectiles, float simulation_dt, double frame_ms) const
{
    const DebugOverlayPresentation& look = *this->look_;

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

    lines.push_back(L"sim dt: " + std::to_wstring(simulation_dt));
    lines.push_back(this->frame_rate_text(frame_ms));
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

    Vector2F text_pos = look.position;

    // Draw in pane-local coordinates; the camera is displayed as data, not applied as a transform.
    draw_list.set_filter(TextureFilter::linear);

    for (size_t i = 0; i < lines.size(); i++)
    {
        const Vector2F shadow_pos = text_pos + look.shadow_offset;

        draw_list.draw_text(this->font_, lines[i], shadow_pos,
            look.shadow_colour, look.shadow_scale, 0.0f, Vector2F::ZERO,
            0.0f);

        draw_list.draw_text(this->font_, lines[i], text_pos, look.colour,
            look.font_scale, 0.0f, Vector2F::ZERO, 0.0f);

        text_pos.y += look.line_spacing;
    }

    draw_list.set_filter(TextureFilter::point);
}
