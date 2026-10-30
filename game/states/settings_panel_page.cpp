#include "game/pch.h"
#include "game/states/settings_panel_page.h"
#include "game/objects/crude_drawing.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

SettingsPanelPage::SettingsPanelPage(MenuContext* context, int opening_slot) :
	MenuPage(context), opening_slot_(opening_slot)
{
}

bool SettingsPanelPage::debug_dismiss(bool /*leaving_match*/)
{
	this->close();
	return true;
}

const ControlsSettingsPresentation& SettingsPanelPage::panel_look() const
{
	return this->presentation().controls_settings();
}

int SettingsPanelPage::opening_slot() const
{
	return this->opening_slot_;
}

ProcessedMenuInput SettingsPanelPage::opening_slot_input(
	const std::vector<ProcessedMenuInput>& inputs) const
{
	return menu_input_for_owner(inputs, this->opening_slot_, *this->level_settings());
}

MenuStyle SettingsPanelPage::panel_style() const
{
	MenuStyle style = this->menu_style();
	style.item_font = this->panel_look().item_font;
	return style;
}

void SettingsPanelPage::build_panel(const MenuStyle& style)
{
	const ControlsSettingsPresentation& look = this->panel_look();
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet, style.fill_frame,
		menu_geometry::design_area(), this->render_resources(),
		CrudeDrawing::premultiplied(this->presentation().main_menu().home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(menu_geometry::footer_paper(look.box_size, style.footer_y),
		this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->settings_session()->play_settings());
	this->texture_container_ = std::make_unique<UiContainer>("textures");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text");
}

UiText* SettingsPanelPage::add_text(const char* name, const std::wstring& value,
	const std::string& font, const Vector2F& position)
{
	return this->add_text(name, value, font, position, this->panel_look().art.ink);
}

UiText* SettingsPanelPage::add_text(const char* name, const std::wstring& value,
	const std::string& font, const Vector2F& position, const Colour& colour)
{
	this->text_.push_back(std::make_unique<UiText>(name, value, font, position,
		this->render_resources(), colour));
	UiText* widget = this->text_.back().get();
	this->text_container_->add_child(widget);
	return widget;
}

void SettingsPanelPage::fit_text(UiText& widget, const RectangleF& available)
{
	widget.set_scale(1.0f);
	const RectangleF bounds = widget.bounds();
	if (bounds.width > 0.0f && bounds.height > 0.0f)
	{
		widget.set_scale(std::min({ 1.0f, available.width / bounds.width,
			available.height / bounds.height }));
	}
}

RectangleF SettingsPanelPage::status_limit(const MenuStyle& style) const
{
	const ControlsSettingsPresentation& look = this->panel_look();
	return RectangleF(look.status_position, Vector2F(
		menu_geometry::paper_text_right(this->paper_->rectangle(), look.art) - look.status_position.x,
		style.footer_y - look.status_position.y));
}

void SettingsPanelPage::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}
