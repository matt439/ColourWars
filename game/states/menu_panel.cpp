#include "game/pch.h"
#include "game/states/menu_panel.h"
#include "game/states/menu_focus_motion.h"
#include "game/play_settings.h"
#include "game/objects/crude_drawing.h"

using namespace labrador;
using namespace mattmath;

MenuJoinAccent::MenuJoinAccent(const std::string& name, const RectangleF& rectangle,
	RenderResources* resources, const MenuStyle* style, const MenuFocusMotion* motion,
	const PlaySettings* settings) :
	UiTexture(name, style->sprite_sheet, style->fill_frame, rectangle, resources, style->highlight),
	style_(style), motion_(motion), settings_(settings)
{
}

void MenuJoinAccent::draw(DrawList& list) const
{
	if (this->hidden() || this->settings_->reduce_effects) { return; }
	const float press = this->motion_->press_fraction();
	const RectangleF gutter = this->rectangle();
	if (press <= 0.0f || gutter.width <= 0.0f || gutter.height <= 0.0f) { return; }
	const float width = std::min(this->style_->accent_width, gutter.width);
	const float height = gutter.height * press;
	const RectangleF mark(gutter.x + (gutter.width - width) * press,
		gutter.center().y - height / 2.0f, width, height);
	TextureObject::draw_with(list, mark, this->frame(), this->colour(),
		this->origin(), this->flip(), this->draw_rotation());
}

MenuStageAccent::MenuStageAccent(const std::string& name, const RectangleF& rectangle,
	RenderResources* resources, const MenuStyle* style, const MenuFocusMotion* motion,
	const PlaySettings* settings, float max_inward) :
	UiTexture(name, style->sprite_sheet, style->fill_frame, rectangle, resources, style->highlight),
	motion_(motion), settings_(settings), max_inward_(std::max(0.0f, max_inward))
{
}

void MenuStageAccent::draw(DrawList& list) const
{
	if (this->hidden()) { return; }
	RectangleF stripe = this->rectangle();
	if (!this->settings_->reduce_effects)
	{
		stripe.y += std::min(stripe.height *
			(this->motion_->arrow_offset() + this->motion_->press_fraction()), this->max_inward_);
	}
	TextureObject::draw_with(list, stripe, this->frame(), this->colour(),
		this->origin(), this->flip(), this->draw_rotation());
}

MenuChoiceIcon::MenuChoiceIcon(const std::string& name, const std::string& sheet,
	const std::string& frame, const RectangleF& rectangle, RenderResources* resources,
	const MenuFocusMotion* motion, const PlaySettings* settings) :
	UiTexture(name, sheet, frame, rectangle, resources), motion_(motion), settings_(settings)
{
}

void MenuChoiceIcon::set_motion_selected(bool selected)
{
	this->motion_selected_ = selected;
}

void MenuChoiceIcon::draw(DrawList& list) const
{
	if (this->hidden()) { return; }
	RectangleF artwork = this->rectangle();
	if (this->motion_selected_ && this->motion_ != nullptr &&
		(this->settings_ == nullptr || !this->settings_->reduce_effects))
	{
		const float inset = std::max(0.0f, std::min(artwork.width, artwork.height)) *
			this->motion_->selection_inset();
		artwork.inflate(-inset);
	}
	TextureObject::draw_with(list, artwork, this->frame(), this->colour(),
		this->origin(), this->flip(), this->draw_rotation());
}

MenuIcon::MenuIcon(const std::string& name, const std::string& frame,
	const RectangleF& rectangle, RenderResources* resources,
	const Colour& colour, const MenuStyle* style, const std::string& sheet_name,
	const std::string& fill_frame, const MenuFocusMotion* motion, const PlaySettings* settings) :
	UiTexture(name, sheet_name, frame, rectangle, resources, colour),
	style_(style), motion_(motion), settings_(settings),
	fill_frame_(this->sprite_sheet()->resolve_sprite_frame(fill_frame))
{
}

void MenuIcon::set_sprite_frame(const std::string& frame, const std::string& paint_frame,
	const std::string& supply_strip)
{
	UiTexture::set_sprite_frame(frame);
	this->paint_frame_ = paint_frame.empty() ? SpriteSheet::frame_handle{} :
		this->sprite_sheet()->resolve_sprite_frame(paint_frame);
	this->supply_strip_ = supply_strip.empty() ? SpriteSheet::strip_handle{} :
		this->sprite_sheet()->resolve_animation_strip(supply_strip);
}

void MenuIcon::set_paint_colour(const Colour& colour)
{
	this->paint_colour_ = colour;
}

void MenuIcon::draw(DrawList& list) const
{
	if (this->hidden())
	{
		return;
	}
	RectangleF tile = this->rectangle();
	tile.inflate(this->style_->icon_padding);
	RectangleF border = tile;
	border.inflate(this->style_->rule_height);
	if (this->motion_ != nullptr && (this->settings_ == nullptr || !this->settings_->reduce_effects))
	{
		// The selected tool and its paint masks remain fixed. Only the tile
		// contracts, and never far enough to cross the authored icon canvas.
		const float limit = std::max(0.0f, std::min(this->style_->icon_padding.x,
			this->style_->icon_padding.y));
		const float inset = std::clamp(std::min(this->rectangle().width, this->rectangle().height) *
			this->motion_->selection_inset(), 0.0f, limit);
		tile.inflate(-inset);
		border.inflate(-inset);
	}
	const SpriteSheet* sheet = this->sprite_sheet();
	sheet->draw(list, this->fill_frame_, border, this->colour());
	sheet->draw(list, this->fill_frame_, tile, this->style_->heading_colour);

	const RectangleI& source = sheet->sprite_frame(this->frame()).source_rectangle();
	const Vector2F source_size(static_cast<float>(source.width),
		static_cast<float>(source.height));
	const float scale = std::min(this->rectangle().width / source_size.x,
		this->rectangle().height / source_size.y);
	const Vector2F fitted_size = source_size * scale;
	const RectangleF artwork(this->rectangle().center() - fitted_size / 2.0f,
		fitted_size);
	TextureObject::draw_with(list, artwork, this->frame(), Colour::white,
		this->origin(), this->flip(), this->draw_rotation());
	if (this->paint_frame_.valid())
	{
		TextureObject::draw_with(list, artwork, this->paint_frame_, this->paint_colour_,
			this->origin(), this->flip(), this->draw_rotation());
	}
	if (this->supply_strip_.valid())
	{
		const AnimationStrip& supply = sheet->animation_strip(this->supply_strip_);
		sheet->draw(list, supply.frame_rect(supply.frame_count() - 1), artwork,
			this->paint_colour_, this->draw_rotation(), this->origin(), this->flip());
	}
}

MenuPaperArtwork::MenuPaperArtwork(const RectangleF& rectangle, RenderResources* resources,
	const MenuStyle* style, const MenuPaperArtPresentation* art, const FocusGroup* focus,
	const UiWidget* primary, const MenuFocusMotion* motion, const PlaySettings* settings) :
	UiTexture("box", style->sprite_sheet, style->fill_frame, rectangle, resources),
	art_(art), focus_(focus), primary_(primary), motion_(motion), settings_(settings)
{
}

void MenuPaperArtwork::draw(DrawList& list) const
{
	if (this->hidden()) { return; }
	list.set_filter(TextureFilter::point);
	this->draw_card(list, this->rectangle(), this->art_->paper);
	const UiWidget* focused = this->focus_->focused(0);
	const bool eligible = focused != nullptr && !focused->hidden() && this->focus_->enabled(focused);
	const bool animate = this->motion_ != nullptr &&
		(this->settings_ == nullptr || !this->settings_->reduce_effects);
	const float press = animate && eligible ? this->motion_->press_fraction() : 0.0f;

	// The continuation is recognisable even after navigating away. Its
	// card is part of the page rather than a colour-only focus change.
	if (this->primary_ != nullptr && !this->primary_->hidden())
	{
		RectangleF primary = this->primary_->bounds();
		primary.inflate(this->art_->focus_padding);
		this->draw_card(list, primary, this->art_->focus_fill,
			focused == this->primary_ ? press : 0.0f);
	}

	if (eligible)
	{
		RectangleF row = focused->bounds();
		row.inflate(this->art_->focus_padding);
		if (focused != this->primary_)
		{
			this->draw_card(list, row, this->art_->focus_fill, press);
		}
		// A detached, stepped arrow identifies the shared cursor without
		// relying on text colour, including on the always-present card.
		const float pen = this->art_->keyline_width;
		const float inward = animate ? this->motion_->arrow_offset() * pen : 0.0f;
		const Vector2F tip(row.x - pen * 3.0f + inward, row.center().y);
		this->fill(list, RectangleF(tip.x - pen * 4.0f, tip.y - pen,
			pen * 4.0f, pen * 2.0f), this->art_->ink);
		for (int step = 0; step < 3; ++step)
		{
			const float reach = static_cast<float>(step);
			this->fill(list, RectangleF(tip.x - reach * pen,
				tip.y - (reach + 1.0f) * pen, pen,
				(reach + 1.0f) * pen * 2.0f), this->art_->ink);
		}
	}
	// Other texture widgets in this layer also use the solid texel. Keep
	// point sampling; MenuPage selects linear filtering for its text layer.
}


void MenuPaperArtwork::fill(DrawList& list, const RectangleF& rectangle, const Colour& colour) const
{
	this->sprite_sheet()->draw(list, this->frame(), rectangle,
		CrudeDrawing::premultiplied(colour));
}

void MenuPaperArtwork::draw_card(DrawList& list, const RectangleF& bounds, const Colour& colour,
	float press_fraction) const
{
	const float pen = this->art_->keyline_width;
	const float step = pen * 2.0f;
	const float shadow = this->art_->shadow_offset;
	const RectangleF rest(bounds.x, bounds.y,
		bounds.width - shadow, bounds.height - shadow);
	// Depress only the paper face into its existing shadow. The complete
	// artwork envelope, labels and navigation bounds stay fixed.
	const RectangleF face(rest.position() + Vector2F(shadow * press_fraction), rest.size());
	const auto paper = [&](const RectangleF& box, const Colour& tint)
	{
		this->fill(list, RectangleF(box.x + step, box.y,
			box.width - step * 2.0f, box.height), tint);
		this->fill(list, RectangleF(box.x, box.y + step,
			box.width, box.height - step * 2.0f), tint);
	};
	paper(RectangleF(rest.position() + Vector2F(shadow), rest.size()), this->art_->shade);
	paper(face, this->art_->ink);
	const RectangleF inside(face.x + pen, face.y + pen,
		face.width - pen * 2.0f, face.height - pen * 2.0f);
	paper(inside, colour);
	for (int pass = 3; pass > 0; --pass)
	{
		Colour shade = this->art_->shade;
		shade.a *= 0.12f;
		const float width = pen * static_cast<float>(pass);
		this->fill(list, RectangleF(inside.x + step, inside.bottom() - width,
			inside.width - step * 2.0f, width), shade);
		this->fill(list, RectangleF(inside.right() - width, inside.y + step,
			width, inside.height - step * 2.0f), shade);
	}
}
