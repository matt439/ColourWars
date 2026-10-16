#pragma once

#include "engine/ui/widget.h"
#include "engine/ui/focus.h"
#include "game/presentation.h"

class MenuFocusMotion;
struct PlaySettings;

// Accepted joins/leaves briefly mark the unused gutter beside a player row.
class MenuJoinAccent final : public labrador::UiTexture
{
public:
	MenuJoinAccent(const std::string& name, const mattmath::RectangleF& rectangle,
		labrador::RenderResources* resources, const MenuStyle* style,
		const MenuFocusMotion* motion, const PlaySettings* settings);
	void draw(labrador::DrawList& list) const override;
private:
	const MenuStyle* style_;
	const MenuFocusMotion* motion_;
	const PlaySettings* settings_;
};

// The briefing stripe settles inside the padding above its unchanged text.
class MenuStageAccent final : public labrador::UiTexture
{
public:
	MenuStageAccent(const std::string& name, const mattmath::RectangleF& rectangle,
		labrador::RenderResources* resources, const MenuStyle* style,
		const MenuFocusMotion* motion, const PlaySettings* settings, float max_inward);
	void draw(labrador::DrawList& list) const override;
private:
	const MenuFocusMotion* motion_;
	const PlaySettings* settings_;
	float max_inward_;
};

// Selected team artwork contracts inside its authored canvas. Logical bounds
// and all neighbouring identity/readiness widgets retain their geometry.
class MenuChoiceIcon final : public labrador::UiTexture
{
public:
	MenuChoiceIcon(const std::string& name, const std::string& sheet,
		const std::string& frame, const mattmath::RectangleF& rectangle,
		labrador::RenderResources* resources, const MenuFocusMotion* motion,
		const PlaySettings* settings);
	void set_motion_selected(bool selected);
	void draw(labrador::DrawList& list) const override;
private:
	const MenuFocusMotion* motion_;
	const PlaySettings* settings_;
	bool motion_selected_ = false;
};

// Fit weapon artwork inside a light tile; readiness colours the border.
class MenuIcon final : public labrador::UiTexture
{
public:
	MenuIcon(const std::string& name, const std::string& frame,
		const mattmath::RectangleF& rectangle, labrador::RenderResources* resources,
		const labrador::Colour& colour, const MenuStyle* style,
		const std::string& sheet_name, const std::string& fill_frame,
		const MenuFocusMotion* motion = nullptr, const PlaySettings* settings = nullptr,
		const MenuPaperArtPresentation* paper_art = nullptr);
	// An empty paint name is an icon with no team-coloured region.
	void set_sprite_frame(const std::string& frame, const std::string& paint_frame = "",
		const std::string& supply_strip = "");
	void set_paint_colour(const labrador::Colour& colour);
	// Resting tile and border include the complete optional paper envelope.
	mattmath::RectangleF artwork_bounds() const;
	void draw(labrador::DrawList& list) const override;
private:
	const MenuStyle* style_;
	const MenuFocusMotion* motion_;
	const PlaySettings* settings_;
	const MenuPaperArtPresentation* paper_art_;
	labrador::SpriteSheet::frame_handle fill_frame_;
	labrador::SpriteSheet::frame_handle paint_frame_;
	labrador::SpriteSheet::strip_handle supply_strip_;
	labrador::Colour paint_colour_ = labrador::Colour::white;
};

// Shared Paint.exe modal artwork. Focus, widgets and motion are borrowed from
// the page, preferences from Save. The primary card stays visible off focus.
class MenuPaperArtwork final : public labrador::UiTexture
{
public:
	MenuPaperArtwork(const mattmath::RectangleF& rectangle,
		labrador::RenderResources* resources, const MenuStyle* style, const MenuPaperArtPresentation* art,
		const labrador::FocusGroup* focus, const labrador::UiWidget* primary = nullptr,
		const MenuFocusMotion* motion = nullptr, const PlaySettings* settings = nullptr);
	void draw(labrador::DrawList& list) const override;
private:
	void fill(labrador::DrawList& list, const mattmath::RectangleF& rectangle,
		const labrador::Colour& colour) const;
	void draw_card(labrador::DrawList& list, const mattmath::RectangleF& bounds,
		const labrador::Colour& colour, float press_fraction = 0.0f) const;
	const MenuPaperArtPresentation* art_;
	const labrador::FocusGroup* focus_;
	const labrador::UiWidget* primary_;
	const MenuFocusMotion* motion_;
	const PlaySettings* settings_;
};
