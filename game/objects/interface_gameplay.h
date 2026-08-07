#pragma once

#include "engine/render/drawer.h"
#include "game/objects/player_team.h"
#include "engine/render/render_resources.h"
#include "engine/math/colour.h"
#include "engine/math/matt_math.h"

namespace interface_consts
{
    constexpr float RIGHT_MARGIN = 0.18f;
    constexpr float AMMO_TOP_MARGIN = 0.15f;
    constexpr float HEALTH_TOP_MARGIN = 0.05f;
    const static mattmath::Vector2F TOP_RIGHT_BOX_SIZE = { 0.15f, 0.07f };
    constexpr float BOX_FILL_BORDER = 0.003f;
    const static mattmath::Vector2F TIMER_OFFSET = { 0.01f, 0.05f };
    const static std::string TIMER_FONT = "gill_sans_mt_bold_48";
    const static mattmath::Colour HEALTH_COLOUR = colour_consts::GREEN;
    const static mattmath::Vector2F TIMER_SHADOW_OFFSET = { 2.0f, 2.0f };

    const static mattmath::Colour TIMER_COLOUR = colour_consts::GRAY;
    const static mattmath::Colour TIMER_SHADOW_COLOUR = colour_consts::BLACK;
    constexpr float TIMER_SCALE_FACTOR = 1920.0f;

    const static std::string SPRITE_SHEET = "sprite_sheet_1";
    const static std::string EMPTY_BOX_FRAME = "health_box";
    const static std::string FILL_FRAME = "pixel";

    const static mattmath::Vector2F RESPAWN_TIMER_OFFSET = { 0.05f, 0.1f };
    const static std::string RESPAWN_TIMER_FONT = "gill_sans_mt_bold_144";
    const static mattmath::Vector2F RESPAWN_TIMER_SHADOW_OFFSET = { 2.0f, 2.0f };
    const static mattmath::Colour RESPAWN_TIMER_COLOUR = colour_consts::WHITE;
    const static mattmath::Colour RESPAWN_TIMER_SHADOW_COLOUR = colour_consts::BLACK;
}

struct InterfaceDraw
{
    mattmath::Vector2I resolution = { -1, -1 };
    float health = 0.0f;
    float ammo = 0.0f;
    PlayerTeam team = PlayerTeam::a;
    float timer = 0.0f;
    mattmath::Colour team_colour = colour_consts::GRAY;
};

class InterfaceGameplay : public Drawer
{
public:
    InterfaceGameplay(RenderResources* render_resources, const float* dt);

	void draw_gameplay_interface(DirectX::SpriteBatch* sprite_batch,
        const mattmath::Vector2F& resolution,
        float health,
        float ammo,
        float timer,
        const mattmath::Colour& team_colour,
        ID3D11SamplerState* sampler_state,
        float respawn_timer,
        bool show_respawn_timer);

private:
    void draw_ammo(DirectX::SpriteBatch* sprite_batch,
        const mattmath::Vector2F& resolution,
        float ammo,
        const mattmath::Colour& team_colour,
        const mattmath::Vector2F& position) const;
    void draw_health(DirectX::SpriteBatch* sprite_batch,
        const mattmath::Vector2F& resolution,
        float health,
        const mattmath::Vector2F& position) const;
    void draw_timer(DirectX::SpriteBatch* sprite_batch,
        const mattmath::Vector2F& resolution,
        float timer) const;

    void draw_respawn_timer(DirectX::SpriteBatch* sprite_batch,
        const mattmath::Vector2F& resolution,
        float timer) const;

    // The HUD always draws out of the same sheet, with the same two frames and
    // the same two fonts. All five names are resolved in the constructor, so
    // the four draw_* calls below - which run every frame, for every split
    // screen viewport - index rather than search. Declaration order matters:
    // the frames are resolved against sheet_.
    RenderResources::SpriteSheetHandle sheet_;
    SpriteSheet::frame_handle empty_box_frame_;
    SpriteSheet::frame_handle fill_frame_;
    RenderResources::FontHandle timer_font_;
    RenderResources::FontHandle respawn_timer_font_;
};
