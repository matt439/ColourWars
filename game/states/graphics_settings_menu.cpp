#include "game/pch.h"
#include "game/states/graphics_settings_menu.h"
#include "game/config/crt_trial.h"
#include "game/game_data.h"

using namespace labrador;

GraphicsSettingsMenu::GraphicsSettingsMenu(MenuContext* context, int opening_slot) :
    SettingsPanelPage(context, opening_slot)
{
}

void GraphicsSettingsMenu::init()
{
    this->input()->reset_actions();
    this->selection_ = this->settings_session()->play_settings().crt_monitor;
    const auto& look = this->panel_look();
    const auto& copy = this->text().graphics;
    const MenuStyle style = this->panel_style();
    this->focus_.set_style({look.art.focus_ink, look.art.ink, look.art.ink});
    this->set_prompts(MenuPrompt::controls);
    this->build_panel(style);
    this->add_text("heading", copy.heading, style.heading_font, look.heading_position);
    const Vector2F value_limit(
        menu_geometry::settings_action_width(look, this->calculate_widget_position(1, 0).x),
        look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
    this->crt_ = std::make_unique<OptionRow>("crt_monitor", this->text().item(MenuItem::crt_monitor),
        this->calculate_widget_position(0, 0), this->calculate_widget_position(1, 0),
        style, look.art.ink, this->render_resources(), [this](Direction direction)
        {
            this->selection_ = direction == Direction::right;
            this->refresh_value();
            this->set_status(this->text().graphics.pending);
        }, &look.art, value_limit);
    this->text_container_->add_child(this->crt_.get());
    this->focus_.add(this->crt_.get());
    this->refresh_value();

    const crt_trial::Runtime* runtime = this->data()->crt_runtime();
    const bool enabled = runtime != nullptr && runtime->enabled();
    const bool overridden = runtime != nullptr && runtime->overridden();
    const auto& launch = overridden ? (enabled ? copy.override_on : copy.override_off) :
        (enabled ? copy.launch_on : copy.launch_off);
    UiText* active = this->add_text("crt_launch", launch, look.help_font,
        this->calculate_widget_position(0, 2));
    fit_text(*active, RectangleF(active->bounds().position(), Vector2F(
        menu_geometry::settings_action_width(look, active->bounds().x), look.layout.spacing.y)));
    UiText* help = this->add_text("graphics_help", copy.help, look.help_font, look.controls_position);
    fit_text(*help, RectangleF(look.controls_position, look.controls_size));
    UiText* apply = this->add_text("apply", this->text().item(MenuItem::apply), style.item_font,
        this->calculate_widget_position(0, 4));
    UiText* back = this->add_text("back", this->text().item(MenuItem::back), style.item_font,
        this->calculate_widget_position(0, 5));
    for (UiText* action : {apply, back})
    {
        fit_text(*action, RectangleF(action->bounds().position(), Vector2F(
            menu_geometry::settings_action_width(look, action->bounds().x),
            look.layout.spacing.y - look.art.focus_padding.y * 2.0f)));
    }
    this->focus_.add(apply, [this] { this->apply_settings(); });
    this->focus_.add(back, [this] { this->pop_page(); });
    this->status_ = this->add_text("status", L"", look.help_font, look.status_position, look.art.focus_ink);
    this->set_status(copy.pending);
}

void GraphicsSettingsMenu::refresh_value()
{
    this->crt_->set_value(this->selection_ ? this->text().values.on : this->text().values.off);
}

void GraphicsSettingsMenu::set_status(const std::wstring& text, bool warning)
{
    this->status_->set_text(text);
    this->status_->set_colour(warning ? this->presentation().main_menu().options_warning_colour :
        this->panel_look().art.focus_ink);
    fit_text(*this->status_, this->status_limit(this->panel_style()));
}

void GraphicsSettingsMenu::apply_settings()
{
    const bool saved = this->settings_session()->set_crt_monitor_and_save(this->selection_);
    this->play_wave(this->confirm_sound_);
    const auto* runtime = this->data()->crt_runtime();
    const bool overridden = runtime != nullptr && runtime->overridden();
    this->set_status(saved ? (overridden ? this->text().graphics.saved_override : this->text().graphics.saved) :
        this->text().graphics.save_failed, !saved);
}

void GraphicsSettingsMenu::update_page(float /*dt*/)
{
    if (!this->context()->active()) { return; }
    std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
    if (this->opening_slot() >= 0) { inputs = {this->opening_slot_input(inputs)}; }
    for (const auto& input : inputs)
    {
        if (input.action == MenuInputAction::back) { this->pop_page(); return; }
        if (input.action == MenuInputAction::proceed)
        {
            if (this->activate_focus() == Activation::ran) { return; }
        }
        else if (input.direction == Direction::left || input.direction == Direction::right)
        {
            const auto* row = dynamic_cast<const OptionRow*>(this->focus_.focused(0));
            if (row != nullptr && this->focus_.enabled(row))
            {
                row->adjust(input.direction);
                this->press_focus();
                this->play_wave(this->direction_sound_);
            }
        }
        else if (this->focus_.move(0, input.direction)) { this->play_wave(this->direction_sound_); }
    }
}
