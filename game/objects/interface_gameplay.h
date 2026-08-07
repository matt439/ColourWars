#ifndef INTERFACEGAMEPLAY_H
#define INTERFACEGAMEPLAY_H

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
    const static MattMath::Vector2F TOP_RIGHT_BOX_SIZE = { 0.15f, 0.07f };
    constexpr float BOX_FILL_BORDER = 0.003f;
    const static MattMath::Vector2F TIMER_OFFSET = { 0.01f, 0.05f };
    const static std::string TIMER_FONT = "gill_sans_mt_bold_48";
    const static MattMath::Colour HEALTH_COLOUR = colour_consts::GREEN;
    const static MattMath::Vector2F TIMER_SHADOW_OFFSET = { 2.0f, 2.0f };

    const static MattMath::Colour TIMER_COLOUR = colour_consts::GRAY;
    const static MattMath::Colour TIMER_SHADOW_COLOUR = colour_consts::BLACK;
    constexpr float TIMER_SCALE_FACTOR = 1920.0f;

    const static std::string SPRITE_SHEET = "sprite_sheet_1";
    const static std::string EMPTY_BOX_FRAME = "health_box";
    const static std::string FILL_FRAME = "pixel";

    const static MattMath::Vector2F RESPAWN_TIMER_OFFSET = { 0.05f, 0.1f };
    const static std::string RESPAWN_TIMER_FONT = "gill_sans_mt_bold_144";
    const static MattMath::Vector2F RESPAWN_TIMER_SHADOW_OFFSET = { 2.0f, 2.0f };
    const static MattMath::Colour RESPAWN_TIMER_COLOUR = colour_consts::WHITE;
    const static MattMath::Colour RESPAWN_TIMER_SHADOW_COLOUR = colour_consts::BLACK;
}

struct InterfaceDraw
{
    MattMath::Vector2I resolution = { -1, -1 };
    float health = 0.0f;
    float ammo = 0.0f;
    player_team team = player_team::A;
    float timer = 0.0f;
    MattMath::Colour team_colour = colour_consts::GRAY;
};

class InterfaceGameplay : public Drawer
{
public:
    InterfaceGameplay(RenderResources* render_resources, const float* dt);

	void draw_gameplay_interface(DirectX::SpriteBatch* sprite_batch,
        const MattMath::Vector2F& resolution,
        float health,
        float ammo,
        float timer,
        const MattMath::Colour& team_colour,
        ID3D11SamplerState* sampler_state,
        float respawn_timer,
        bool show_respawn_timer);

private:
    void draw_ammo(DirectX::SpriteBatch* sprite_batch,
        const MattMath::Vector2F& resolution,
        float ammo,
        const MattMath::Colour& team_colour,
        const MattMath::Vector2F& position) const;
    void draw_health(DirectX::SpriteBatch* sprite_batch,
        const MattMath::Vector2F& resolution,
        float health,
        const MattMath::Vector2F& position) const;
    void draw_timer(DirectX::SpriteBatch* sprite_batch,
        const MattMath::Vector2F& resolution,
        float timer) const;

    void draw_respawn_timer(DirectX::SpriteBatch* sprite_batch,
        const MattMath::Vector2F& resolution,
        float timer) const;

    // The HUD always draws out of the same sheet, with the same two frames and
    // the same two fonts. All five names are resolved in the constructor, so
    // the four draw_* calls below - which run every frame, for every split
    // screen viewport - index rather than search. Declaration order matters:
    // the frames are resolved against _sheet.
    RenderResources::SpriteSheetHandle _sheet;
    SpriteSheet::frame_handle _empty_box_frame;
    SpriteSheet::frame_handle _fill_frame;
    RenderResources::FontHandle _timer_font;
    RenderResources::FontHandle _respawn_timer_font;
};
#endif // !INTERFACEGAMEPLAY_H
