#pragma once

#include "game/objects/player_team.h"
#include "engine/render/render_resources.h"
#include "engine/render/colour.h"
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
    const static artattack::Colour HEALTH_COLOUR = artattack::Colour::green;
    const static mattmath::Vector2F TIMER_SHADOW_OFFSET = { 2.0f, 2.0f };

    const static artattack::Colour TIMER_COLOUR = artattack::Colour::gray;
    const static artattack::Colour TIMER_SHADOW_COLOUR = artattack::Colour::black;
    constexpr float TIMER_SCALE_FACTOR = 1920.0f;

    const static std::string SPRITE_SHEET = "sprite_sheet_1";
    const static std::string EMPTY_BOX_FRAME = "health_box";
    const static std::string FILL_FRAME = "pixel";

    const static mattmath::Vector2F RESPAWN_TIMER_OFFSET = { 0.05f, 0.1f };
    const static std::string RESPAWN_TIMER_FONT = "gill_sans_mt_bold_144";
    const static mattmath::Vector2F RESPAWN_TIMER_SHADOW_OFFSET = { 2.0f, 2.0f };
    const static artattack::Colour RESPAWN_TIMER_COLOUR = artattack::Colour::white;
    const static artattack::Colour RESPAWN_TIMER_SHADOW_COLOUR = artattack::Colour::black;
}

struct InterfaceDraw
{
    mattmath::Vector2I resolution = { -1, -1 };
    float health = 0.0f;
    float ammo = 0.0f;
    PlayerTeam team = PlayerTeam::a;
    float timer = 0.0f;
    artattack::Colour team_colour = artattack::Colour::gray;
};

class InterfaceGameplay
{
public:
    explicit InterfaceGameplay(artattack::RenderResources* render_resources);

    // const for the same reason GameObject::draw is. There is one
    // InterfaceGameplay for the whole level, and every render worker calls this
    // on it at once with its own view's health, ammo and timer - so the HUD is
    // the most tempting place in the level to stash per-view state on a shared
    // object, and the only reason it is safe is that all of it arrives as
    // parameters. Level holds this by unique_ptr, which does not pass its
    // constness on, so this has to say const itself to mean anything.
	void draw_gameplay_interface(artattack::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float health,
        float ammo,
        float timer,
        const artattack::Colour& team_colour,
        float respawn_timer,
        bool show_respawn_timer) const;

private:
    void draw_ammo(artattack::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float ammo,
        const artattack::Colour& team_colour,
        const mattmath::Vector2F& position) const;
    void draw_health(artattack::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float health,
        const mattmath::Vector2F& position) const;
    void draw_timer(artattack::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float timer) const;

    void draw_respawn_timer(artattack::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float timer) const;

    artattack::RenderResources* render_resources() const;

    // Borrowed, not owned: the services outlive every object that snapshots
    // them. Held here rather than inherited from a base class whose only
    // other members were three helpers nothing called.
    artattack::RenderResources* render_resources_ = nullptr;

    // The HUD always draws out of the same sheet, with the same two frames and
    // the same two fonts. All five names are resolved in the constructor, so
    // the four draw_* calls below - which run every frame, for every split
    // screen viewport - index rather than search. Declaration order matters:
    // the frames are resolved against sheet_.
    artattack::RenderResources::SpriteSheetHandle sheet_;
    artattack::SpriteSheet::frame_handle empty_box_frame_;
    artattack::SpriteSheet::frame_handle fill_frame_;
    artattack::FontHandle timer_font_;
    artattack::FontHandle respawn_timer_font_;
};
