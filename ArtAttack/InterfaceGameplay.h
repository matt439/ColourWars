#ifndef INTERFACEGAMEPLAY_H
#define INTERFACEGAMEPLAY_H

#include "Drawer.h"
#include "player_team.h"
#include "ResourceManager.h"
#include "colour.h"
#include "MattMath.h"

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
    InterfaceGameplay(DirectX::SpriteBatch* sprite_batch,
        ResourceManager* resource_manager, const float* dt);

	void draw_gameplay_interface(const MattMath::Vector2F& resolution,
        float health,
        float ammo,
        float timer,
        const MattMath::Colour& team_colour,
        ID3D11SamplerState* sampler_state,
        float respawn_timer,
        bool show_respawn_timer);

private:
    void draw_ammo(const MattMath::Vector2F& resolution,
        float ammo,
        const MattMath::Colour& team_colour,
        const MattMath::Vector2F& position) const;
    void draw_health(const MattMath::Vector2F& resolution,
        float health,
        const MattMath::Vector2F& position) const;
    void draw_timer(const MattMath::Vector2F& resolution,
        float timer) const;

    void draw_respawn_timer(const MattMath::Vector2F& resolution,
        float timer) const;
};
#endif // !INTERFACEGAMEPLAY_H
