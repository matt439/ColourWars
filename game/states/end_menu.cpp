#include "game/pch.h"
#include "game/states/end_menu.h"
#include "game/objects/crude_drawing.h"

using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

namespace
{
	// This page owns its paper treatment. Other menus retain their existing
	// panels and chrome, and focus remains the engine's shared-cursor workflow.
	class EndMenuArtwork final : public UiTexture
	{
	public:
		EndMenuArtwork(const RectangleF& rectangle, RenderResources* resources,
			const EndMenuArtPresentation* art, const FocusGroup* focus,
			const UiWidget* primary) :
			UiTexture("box", "sprite_sheet_1", "pixel", rectangle, resources),
			art_(art), focus_(focus), primary_(primary)
		{
		}

		void draw(DrawList& list) const override
		{
			if (this->hidden()) { return; }
			list.set_filter(TextureFilter::point);
			this->draw_card(list, this->rectangle(), this->art_->paper);

			// The continuation is recognisable even after navigating away. Its
			// card is part of the page rather than a colour-only focus change.
			RectangleF primary = this->primary_->bounds();
			primary.inflate(this->art_->focus_padding);
			this->draw_card(list, primary, this->art_->focus_fill);

			const UiWidget* focused = this->focus_->focused(0);
			if (focused != nullptr && !focused->hidden())
			{
				RectangleF row = focused->bounds();
				row.inflate(this->art_->focus_padding);
				if (focused != this->primary_)
				{
					this->draw_card(list, row, this->art_->focus_fill);
				}
				// A detached, stepped arrow identifies the shared cursor without
				// relying on text colour, including on the always-present card.
				const float pen = this->art_->keyline_width;
				const Vector2F tip(row.x - pen * 3.0f, row.center().y);
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
			list.set_filter(TextureFilter::linear);
		}

	private:
		void fill(DrawList& list, const RectangleF& rectangle, const Colour& colour) const
		{
			this->sprite_sheet()->draw(list, this->frame(), rectangle,
				CrudeDrawing::premultiplied(colour));
		}

		void draw_card(DrawList& list, const RectangleF& bounds, const Colour& colour) const
		{
			const float pen = this->art_->keyline_width;
			const float step = pen * 2.0f;
			const float shadow = this->art_->shadow_offset;
			const RectangleF face(bounds.x, bounds.y,
				bounds.width - shadow, bounds.height - shadow);
			const auto paper = [&](const RectangleF& box, const Colour& tint)
			{
				this->fill(list, RectangleF(box.x + step, box.y,
					box.width - step * 2.0f, box.height), tint);
				this->fill(list, RectangleF(box.x, box.y + step,
					box.width, box.height - step * 2.0f), tint);
			};
			paper(RectangleF(face.position() + Vector2F(shadow), face.size()), this->art_->shade);
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

		const EndMenuArtPresentation* art_;
		const FocusGroup* focus_;
		const UiWidget* primary_;
	};
}

EndMenuPage::EndMenuPage(MenuContext* context) :
	MenuPage(context)
{

}

const EndMenuPresentation& EndMenuPage::look() const
{
	return this->presentation().end_menu();
}

void EndMenuPage::finish(EndMenuAction action) const
{
	this->context()->pop(action);
}

bool EndMenuPage::debug_dismiss(bool leaving_match)
{
	this->finish(leaving_match ? EndMenuAction::exit : EndMenuAction::restart);
	return true;
}

EndMenuInitial::EndMenuInitial(MenuContext* context, const Session& session) :
	EndMenuPage(context), session_(session)
{

}

void EndMenuInitial::update_page(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void EndMenuInitial::init()
{
	this->set_prompts({
		{ L"A", L"Select" } });

	const MenuStyle& style = this->menu_style();
	const EndMenuPresentation& look = this->look();
	const bool ongoing = this->session_.is_series() && !this->session_.complete();

	this->focus_.set_style({ look.art.focus_ink, look.art.ink });

	this->set_widget_position(look.layout.position);
	this->set_widget_size(look.layout.size);
	this->set_widget_spacing(look.layout.spacing);

	this->heading_ = std::make_unique<UiText>(
		"heading",
		this->session_.is_series() ? (this->session_.complete() ?
			L"Series Complete" : L"Round Complete") : L"Match Complete",
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.ink);

	this->heading_->set_position(Vector2F(
		this->calculate_center_position(this->heading_->bounds().width, DESIGN_RESOLUTION.x),
		look.heading_y));

	this->change_teams_ = std::make_unique<UiText>(
		"change_teams",
		L"Change Teams",
		style.item_font,
		this->calculate_widget_position(0, 1),
		this->render_resources(),
		look.art.ink);

	this->change_weapons_ = std::make_unique<UiText>(
		"change_weapons",
		L"Change Weapons",
		style.item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		look.art.ink);

	this->change_level_ = std::make_unique<UiText>(
		"change_level",
		L"Change Level",
		style.item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		look.art.ink);

	this->restart_ = std::make_unique<UiText>(
		"restart",
		this->session_.continue_string(),
		style.item_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.focus_ink);

	this->exit_ = std::make_unique<UiText>(
		"exit",
		L"Exit to Main Menu",
		style.item_font,
		this->calculate_widget_position(0, ongoing ? 1 : 4),
		this->render_resources(),
		look.art.ink);

	this->box_ = std::make_unique<EndMenuArtwork>(
		RectangleF(Vector2F::ZERO, look.box_size), this->render_resources(),
		&look.art, &this->focus_, this->restart_.get());
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	// Paint the opaque page after the shared chrome's focus backing, then
	// its labels. This leaves the shared footer and control legend untouched.
	this->text_container_->add_child(this->box_.get());
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->restart_.get());
	this->text_container_->add_child(this->change_teams_.get());
	this->text_container_->add_child(this->change_weapons_.get());
	this->text_container_->add_child(this->change_level_.get());
	this->text_container_->add_child(this->exit_.get());

	const auto choose = [this](EndMenuAction action, bool confirm)
		{
			this->play_wave(confirm ? this->confirm_sound_ : this->cancel_sound_);
			this->finish(action);
		};

	this->change_teams_->set_hidden(ongoing);
	this->change_weapons_->set_hidden(ongoing);
	this->change_level_->set_hidden(ongoing);
	this->focus_.add(this->restart_.get(),
		[choose] { choose(EndMenuAction::restart, true); });
	if (!ongoing)
	{
		this->focus_.add(this->change_teams_.get(),
			[choose] { choose(EndMenuAction::change_teams, true); });
		this->focus_.add(this->change_weapons_.get(),
			[choose] { choose(EndMenuAction::change_weapons, true); });
		this->focus_.add(this->change_level_.get(),
			[choose] { choose(EndMenuAction::change_level, true); });
	}
	this->focus_.add(this->exit_.get(),
		[choose] { choose(EndMenuAction::exit, false); });
	if (this->session_.is_series() && this->session_.complete())
	{
		this->play_wave(this->resolve_wave(look.series_sound.name), look.series_sound.volume);
	}
}
