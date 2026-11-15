#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/objects/crude_drawing.h"
#include "game/input/controls_text.h"
#include <array>

using namespace labrador;

MainMenuHowToPlay::MainMenuHowToPlay(MenuContext* context) : MainMenuPage(context) {}

void MainMenuHowToPlay::refresh()
{
    const auto& words = this->text();
    std::wstring copy = this->page_ == 0 ? words.learn_start : words.learn_finish;
    if (this->page_ == 1)
    {
        constexpr std::array<int, 4> pads{0, 1, 2, 3};
        const auto& bindings = this->settings_session()->input_bindings();
        copy = words.learn_controls + L"\n" + words.controls.keyboard_heading + L"\n" +
            controls_text::keyboard_controls_reminder(words.controls, bindings) + L"\n" +
            words.controls.gamepad_heading + L"\n" +
            controls_text::pad_controls_reminder(words.controls, bindings, pads);
    }
    this->body_->set_text(copy);
    this->body_->set_scale(1.0f);
    const auto& look = this->look();
    this->body_->set_scale(menu_geometry::fit_scale(this->body_->bounds().size(),
        Vector2F(menu_geometry::paper_text_right(this->paper_->rectangle(), look.art) -
            look.credits_position.x, look.credits_back_position.y - look.art.focus_padding.y -
            look.credits_position.y)));
}

void MainMenuHowToPlay::update_page(float)
{
    for (const auto& input : this->menu_inputs())
    {
        if (input.action == MenuInputAction::back)
        {
            this->play_wave(this->cancel_sound_);
            this->go_to(MainMenuRoute::home(MainMenuHome::InitialFocus::how_to_play));
            return;
        }
        if (input.action == MenuInputAction::proceed)
        {
            if (this->activate_focus() == Activation::ran) return;
        }
        else if (this->focus_.move(0, input.direction)) this->play_wave(this->direction_sound_);
    }
}

void MainMenuHowToPlay::init()
{
    const auto& style = this->menu_style(); const auto& look = this->look();
    this->set_prompts(MenuPrompt::select_back);
    this->focus_.set_style({look.art.focus_ink, look.art.ink});
    this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet,
        style.fill_frame, menu_geometry::design_area(), this->render_resources(),
        CrudeDrawing::premultiplied(look.home_background_colour));
    this->heading_ = std::make_unique<UiText>("heading", this->text().item(MenuItem::how_to_play),
        style.heading_font, look.credits_heading_position, this->render_resources(), look.art.ink);
    this->body_ = std::make_unique<UiText>("learn_body", L"", style.detail_font,
        look.credits_position, this->render_resources(), look.art.ink);
    this->next_ = std::make_unique<UiText>("next", this->text().learn_next, style.item_font,
        look.credits_back_position, this->render_resources(), look.art.focus_ink);
    this->back_ = std::make_unique<UiText>("back", this->text().item(MenuItem::back), style.item_font,
        look.credits_back_position, this->render_resources(), look.art.ink);
    this->paper_ = std::make_unique<MenuPaperArtwork>(
        menu_geometry::footer_paper(look.credits_box_size, style.footer_y), this->render_resources(),
        &style, &look.art, &this->focus_, nullptr, &this->focus_motion_,
        &this->settings_session()->play_settings());
    this->back_->set_position(Vector2F(menu_geometry::paper_text_right(
        this->paper_->rectangle(), look.art) - this->back_->bounds().width,
        look.credits_back_position.y));
    this->texture_container_ = std::make_unique<UiContainer>("texture_container");
    this->texture_container_->add_child(this->background_.get());
    this->texture_container_->add_child(this->paper_.get());
    this->text_container_ = std::make_unique<UiContainer>("text_container");
    for (auto* widget : {this->heading_.get(), this->body_.get(), this->next_.get(), this->back_.get()})
        this->text_container_->add_child(widget);
    this->focus_.add(this->next_.get(), [this]
        { this->play_wave(this->confirm_sound_); this->page_ = (this->page_ + 1) % 3; this->refresh(); });
    this->focus_.add(this->back_.get(), [this]
        { this->play_wave(this->cancel_sound_); this->go_to(MainMenuRoute::home(MainMenuHome::InitialFocus::how_to_play)); });
    this->refresh();
}
