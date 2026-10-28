#pragma once

#include "engine/render/render_resources.h"
#include "engine/render/colour.h"
#include "engine/math/vector2f.h"
#include "engine/math/rectanglef.h"
#include "game/presentation.h"
#include "game/objects/player_team.h"
#include "game/objects/player_badge.h"


class InterfaceGameplay
{
public:
    // The presentation is borrowed and outlives every level: main() owns it.
    InterfaceGameplay(labrador::RenderResources* render_resources,
        const HudPresentation* look, const PlayerIdentityPresentation* identity);

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
        bool reduced_effects = false,
        bool clock_running = true) const;

    // The same, with the identity box's second line given ("Bot: paint")
    // instead of the device the input slot names.
	void draw_gameplay_interface(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        int player_index,
        const std::wstring& device_label,
        PlayerTeam team,
        float health,
        float ammo,
        float timer,
        const labrador::Colour& team_colour,
        bool reduced_effects = false,
        bool clock_running = true) const;

    // The same, with the identity box's whole text already built by
    // identity_text. Level keeps one per view so the draw path allocates nothing.
    // `low_paint` tags the PAINT label (POL#38); it blinks with the tool's
    // simulated low-paint age unless effects are reduced. Untimed practice
    // passes show_clock false and the clock panel is not drawn at all.
    void draw_gameplay_interface_prepared(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        int player_index,
        const std::wstring& identity,
        float health,
        float ammo,
        float timer,
        const labrador::Colour& team_colour,
        bool reduced_effects = false,
        bool clock_running = true,
        bool low_paint = false,
        float low_paint_elapsed = 0.0f,
        bool show_clock = true) const;

    // "P1  /  TEAM A\n" followed by the device label: the identity box's text.
    static std::wstring identity_text(int player_index, PlayerTeam team,
        const std::wstring& device_label);

private:
    // hud.paper.keyline_width at this HUD scale, never below one device
    // pixel: a thinner rectangle can fall between pixel centres and vanish.
    float paper_pen(float scale) const;
    void draw_paper_panel(labrador::DrawList& draw_list,
        const mattmath::RectangleF& face, float scale) const;
    void draw_meter(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float amount,
        const labrador::Colour& colour,
        const mattmath::Vector2F& position,
        const std::wstring& label,
        float scale,
        bool tagged = false) const;
    void draw_timer(labrador::DrawList& draw_list,
        const mattmath::Vector2F& resolution,
        float timer,
        float scale,
        bool reduced_effects,
        bool clock_running) const;
    void draw_closing_clock_panel(labrador::DrawList& draw_list,
        const mattmath::RectangleF& text_bounds,
        float scale,
        float beat_age,
        bool reduced_effects) const;

    labrador::RenderResources* render_resources() const;

    // Borrowed services and presentation; both outlive the HUD.
    labrador::RenderResources* render_resources_ = nullptr;
    const HudPresentation* look_ = nullptr;
    const PlayerIdentityPresentation* identity_ = nullptr;
    PlayerBadge player_badge_;

    // Resolve assets at construction. Declare sheet_ first because frame handles
    // are resolved against it.
    labrador::RenderResources::SpriteSheetHandle sheet_;
    labrador::SpriteSheet::frame_handle fill_frame_;
    labrador::FontHandle meter_label_font_;
    labrador::FontHandle timer_font_;
    labrador::FontHandle player_identity_font_;
};
