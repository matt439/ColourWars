#include "game/pch.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/stick_settings_menu.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

ControlsSettingsMenu::ControlsSettingsMenu(MenuContext* context, int player) :
	PauseMenuPage(context, player)
{
	this->volume_preview_sound_ = this->resolve_wave(
		this->presentation().controls_settings().volume_preview_sound.name);
}

void ControlsSettingsMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->save()->play_settings();
	const ControlsSettingsPresentation& look = this->presentation().controls_settings();
	MenuStyle style = this->menu_style();
	style.item_font = look.item_font;
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.ink });
	this->set_prompts(MenuPrompt::controls);
	this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet, style.fill_frame,
		RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
		this->render_resources(), this->presentation().main_menu().home_background_colour);
	const RectangleF paper_bounds(
		(Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f,
		look.box_size);
	this->paper_ = std::make_unique<MenuPaperArtwork>(paper_bounds,
		this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());
	const Vector2F value_limit(look.controls_position.x - look.art.focus_padding.x * 2.0f -
		look.art.shadow_offset - this->calculate_widget_position(1, 0).x,
		look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
	this->texture_container_ = std::make_unique<UiContainer>("textures");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text");
	const auto add_text = [this](const char* name, const std::wstring& value,
		const std::string& font, const Vector2F& position, const Colour& colour)
	{
		this->text_.push_back(std::make_unique<UiText>(name, value, font,
			position, this->render_resources(), colour));
		UiText* widget = this->text_.back().get();
		this->text_container_->add_child(widget);
		return widget;
	};
	add_text("heading", this->text().heading(MenuHeading::controls_audio), style.heading_font,
		look.heading_position, look.art.ink);
	const auto make_row = [this, &style, &look, &value_limit](const char* name, const std::wstring& label,
		int row, int& value, int minimum, int maximum, int step)
	{
		return std::make_unique<OptionRow>(name, label,
			this->calculate_widget_position(0, row), this->calculate_widget_position(1, row),
			style, look.art.ink, this->render_resources(),
			[this, &value, minimum, maximum, step](Direction direction)
			{
				value = std::clamp(value + (direction == Direction::right ? step : -step),
					minimum, maximum);
				this->refresh_values();
				this->set_status(this->text().status.controls_pending,
					this->presentation().controls_settings().art.focus_ink);
			}, &look.art, value_limit);
	};
	UiText* sticks = add_text("sticks", this->text().item(MenuItem::pad_sticks), style.item_font,
		this->calculate_widget_position(0, 0), look.art.ink);
	this->focus_.add(sticks, [this]
	{
		this->play_wave(this->confirm_sound_);
		this->input()->reset_actions();
		this->context()->push(std::make_unique<StickSettingsMenu>(this->menu_context(), this->pausing_player()));
	});
	this->music_ = make_row("music", this->text().item(MenuItem::music_volume),
		1, this->selection_.music_volume, 0, 100, 10);
	this->sfx_ = make_row("sfx", this->text().item(MenuItem::sfx_volume),
		2, this->selection_.sfx_volume, 0, 100, 10);
	this->view_shake_ = std::make_unique<OptionRow>("view_shake", this->text().item(MenuItem::view_shake),
		this->calculate_widget_position(0, 3), this->calculate_widget_position(1, 3),
		style, look.art.ink, this->render_resources(),
		[this](Direction direction)
		{
			this->selection_.view_shake = direction == Direction::right;
			this->refresh_values();
			this->set_status(this->text().status.controls_pending,
				this->presentation().controls_settings().art.focus_ink);
		}, &look.art, value_limit);
	this->aim_marker_ = std::make_unique<OptionRow>("aim_marker", this->text().item(MenuItem::aim_marker),
		this->calculate_widget_position(0, 4), this->calculate_widget_position(1, 4),
		style, look.art.ink, this->render_resources(),
		[this](Direction direction)
		{
			this->selection_.aim_marker = direction == Direction::right;
			this->refresh_values();
			this->set_status(this->text().status.controls_pending,
				this->presentation().controls_settings().art.focus_ink);
		}, &look.art, value_limit);
	this->reduce_effects_ = std::make_unique<OptionRow>("reduce_effects", this->text().item(MenuItem::reduce_effects),
		this->calculate_widget_position(0, 5), this->calculate_widget_position(1, 5),
		style, look.art.ink, this->render_resources(),
		[this](Direction direction)
		{
			this->selection_.reduce_effects = direction == Direction::right;
			this->refresh_values();
			this->set_status(this->text().status.controls_pending,
				this->presentation().controls_settings().art.focus_ink);
		}, &look.art, value_limit);
	for (OptionRow* row : { this->music_.get(), this->sfx_.get(),
		this->view_shake_.get(), this->aim_marker_.get(), this->reduce_effects_.get() })
	{
		this->text_container_->add_child(row);
		this->focus_.add(row);
	}
	this->refresh_values();
	UiText* apply = add_text("apply", this->text().item(MenuItem::apply), style.item_font,
		this->calculate_widget_position(0, 6), look.art.ink);
	UiText* back = add_text("back", this->text().item(MenuItem::back), style.item_font,
		this->calculate_widget_position(0, 7), look.art.ink);
	this->status_ = add_text("status", L"", look.help_font, look.status_position, look.art.focus_ink);
	this->set_status(this->text().status.controls_pending, look.art.focus_ink);
	this->focus_.add(apply, [this] { this->apply_settings(); });
	this->focus_.add(back, [this] { this->close(); });
	UiText* controls = add_text("controls", this->text().help.controls,
		look.help_font, look.controls_position, look.art.ink);
	const RectangleF help = controls->bounds();
	controls->set_scale(std::min({ 1.0f, look.controls_size.x / help.width,
		look.controls_size.y / help.height }));
}

void ControlsSettingsMenu::set_status(const std::wstring& text, const Colour& colour)
{
	this->status_->set_text(text);
	this->status_->set_colour(colour);
	this->status_->set_scale(1.0f);
	const ControlsSettingsPresentation& look = this->presentation().controls_settings();
	const float available = this->paper_->rectangle().right() - look.art.shadow_offset -
		look.art.focus_padding.x - look.status_position.x;
	if (this->status_->bounds().width > available)
	{
		this->status_->set_scale(available / this->status_->bounds().width);
	}
}

void ControlsSettingsMenu::refresh_values()
{
	this->music_->set_value(std::to_wstring(this->selection_.music_volume) + L"%");
	this->sfx_->set_value(std::to_wstring(this->selection_.sfx_volume) + L"%");
	this->view_shake_->set_value(this->selection_.view_shake ? this->text().values.on : this->text().values.off);
	this->aim_marker_->set_value(this->selection_.aim_marker ? this->text().values.on : this->text().values.off);
	this->reduce_effects_->set_value(this->selection_.reduce_effects ? this->text().values.on : this->text().values.off);
}

void ControlsSettingsMenu::apply_settings()
{
	// The child commits pad profiles independently of these shared preferences.
	this->selection_.sticks = this->save()->play_settings().sticks;
	const bool saved = this->save()->set_play_settings_and_save(this->selection_);
	// Apply the saved gain through the owner so an in-progress fade survives.
	this->menu_context()->update_music(0.0f);
	this->play_wave(this->confirm_sound_);
	this->set_status(saved ? this->text().status.controls_saved :
		this->text().status.active_save_failed,
		saved ? this->presentation().controls_settings().art.focus_ink :
		this->presentation().main_menu().options_warning_colour);
}

void ControlsSettingsMenu::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}

bool ControlsSettingsMenu::debug_dismiss(bool /*leaving_match*/)
{
	this->close();
	return true;
}

void ControlsSettingsMenu::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->pausing_player() >= 0)
	{
		inputs = { this->pausing_player_input(inputs) };
	}
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::back)
		{
			this->close();
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->activate_focus() == Activation::ran)
			{
				return;
			}
		}
		else if (input.direction == Direction::left || input.direction == Direction::right)
		{
			const OptionRow* row = dynamic_cast<const OptionRow*>(this->focus_.focused(0));
			if (row != nullptr && this->focus_.enabled(row))
			{
				const int previous_sfx = this->selection_.sfx_volume;
				row->adjust(input.direction);
				this->press_focus();
				if (row == this->sfx_.get())
				{
					if (this->selection_.sfx_volume != previous_sfx)
					{
						// Audition the pending value even when the saved setting is mute.
						// MenuPage::play_wave would multiply by the applied gain again.
						SoundBankObject::play_wave(this->volume_preview_sound_,
							this->presentation().controls_settings().volume_preview_sound.volume *
							this->selection_.sfx_gain());
					}
				}
				else
				{
					this->play_wave(this->direction_sound_);
				}
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
