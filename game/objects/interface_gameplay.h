#pragma once

#include "engine/render/render_resources.h"
#include "engine/render/colour.h"
#include "engine/math/matt_math.h"
#include "game/presentation.h"


class InterfaceGameplay
{
public:
    // The presentation is borrowed and outlives every level: main() owns it.
    InterfaceGameplay(artattack::RenderResources* render_resources,
        const HudPresentation* look);

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
    const HudPresentation* look_ = nullptr;

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
