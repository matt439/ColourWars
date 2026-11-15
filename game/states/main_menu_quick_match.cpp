#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/objects/crude_drawing.h"
using namespace labrador;

void MainMenuQuickMatch::accept(bool customise)
{
    MatchSetup proposed;
    proposed.set_player_count(2);
    proposed.set_player_bot(1, BotDifficulty::normal);
    proposed.begin_input_claims(this->activating_slot_);
    proposed.set_player_team(0, PlayerTeam::a);
    proposed.set_player_team(1, PlayerTeam::b);
    proposed.set_screen_layout(ScreenLayout::one_player);
    *this->level_settings() = proposed;
    this->go_to(customise ? MainMenuScreen::mode_select : MainMenuScreen::stage_select);
}

void MainMenuQuickMatch::update_page(float)
{
    const auto inputs = this->menu_inputs();
    for(size_t i=0;i<inputs.size();++i)
    {
        const auto& input=inputs[i];
        if(input.action==MenuInputAction::back) { this->go_to(MainMenuScreen::home); return; }
        if(input.action==MenuInputAction::proceed)
        {
            this->activating_slot_=static_cast<int>(i);
            if(this->activate_focus()==Activation::ran) return;
        }
        else this->focus_.move(0,input.direction);
    }
}

void MainMenuQuickMatch::init()
{
    const auto& style=this->menu_style(); const auto& look=this->look(); const auto& text=this->text();
    this->set_prompts(MenuPrompt::select_back);
    this->focus_.set_style({look.art.focus_ink,look.art.ink});
    this->background_=std::make_unique<UiTexture>("background",style.sprite_sheet,style.fill_frame,
        menu_geometry::design_area(),this->render_resources(),CrudeDrawing::premultiplied(look.home_background_colour));
    this->heading_=std::make_unique<UiText>("heading",text.quick_title,style.heading_font,
        look.credits_heading_position,this->render_resources(),look.art.ink);
    std::wstring body = text.quick_body;
    const auto token = body.find(L"{duration}");
    if(token != std::wstring::npos) body.replace(token, 10,
        text.setting_duration(this->data()->tuning()->match().duration, false));
    this->body_=std::make_unique<UiText>("body",body,style.detail_font,
        look.credits_position,this->render_resources(),look.art.ink);
    this->start_=std::make_unique<UiText>("start",text.quick_start,style.item_font,Vector2F(180,650),this->render_resources(),look.art.ink);
    this->customise_=std::make_unique<UiText>("customise",text.quick_customise,style.item_font,Vector2F(180,740),this->render_resources(),look.art.ink);
    this->back_=std::make_unique<UiText>("back",text.item(MenuItem::back),style.item_font,Vector2F(180,830),this->render_resources(),look.art.ink);
    this->paper_=std::make_unique<MenuPaperArtwork>(menu_geometry::footer_paper(look.credits_box_size,style.footer_y),
        this->render_resources(),&style,&look.art,&this->focus_,nullptr,&this->focus_motion_,&this->settings_session()->play_settings());
    this->body_->set_scale(menu_geometry::fit_scale(this->body_->bounds().size(),Vector2F(
        menu_geometry::paper_text_right(this->paper_->rectangle(),look.art)-look.credits_position.x,
        650-look.art.focus_padding.y-look.credits_position.y)));
    this->texture_container_=std::make_unique<UiContainer>("texture");
    this->texture_container_->add_child(this->background_.get()); this->texture_container_->add_child(this->paper_.get());
    this->text_container_=std::make_unique<UiContainer>("text");
    for(auto* item:{this->heading_.get(),this->body_.get(),this->start_.get(),this->customise_.get(),this->back_.get()}) this->text_container_->add_child(item);
    this->focus_.add(this->start_.get(),[this]{this->accept(false);});
    this->focus_.add(this->customise_.get(),[this]{this->accept(true);});
    this->focus_.add(this->back_.get(),[this]{this->go_to(MainMenuScreen::home);});
}
