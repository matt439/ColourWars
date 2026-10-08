#pragma once

#include "engine/render/render_resources.h"
#include "engine/render/colour.h"
#include "engine/math/vector2f.h"
#include "game/presentation.h"
#include "game/objects/player_team.h"


class InterfaceGameplay
{
public:
    // The presentation is borrowed and outlives every level: main() owns it.
    InterfaceGameplay(labrador::RenderResources* render_resources,
        const HudPresentation* look);

    // Shared by render workers. Keep per-view state in parameters and locals;
    // const on the owning unique_ptr does not make its pointee const.
	void draw_gameplay_interface(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        int player_index,
        int input_slot,
        PlayerTeam team,
        float health,
        float ammo,
        float timer,
        const labrador::Colour& team_colour,
        float respawn_timer,
        bool show_respawn_timer) const;

private:
    void draw_ammo(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float ammo,
        const labrador::Colour& team_colour,
        const mattmath::Vector2F& position) const;
    void draw_health(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float health,
        const mattmath::Vector2F& position) const;
    void draw_timer(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float timer) const;

    void draw_respawn_timer(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float timer) const;

    labrador::RenderResources* render_resources() const;

    // Borrowed services and presentation; both outlive the HUD.
    labrador::RenderResources* render_resources_ = nullptr;
    const HudPresentation* look_ = nullptr;

    // Resolve assets at construction. Declare sheet_ first because frame handles
    // are resolved against it.
    labrador::RenderResources::SpriteSheetHandle sheet_;
    labrador::SpriteSheet::frame_handle empty_box_frame_;
    labrador::SpriteSheet::frame_handle fill_frame_;
    labrador::FontHandle timer_font_;
    labrador::FontHandle respawn_timer_font_;
    labrador::FontHandle player_identity_font_;
};
