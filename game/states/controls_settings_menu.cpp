#include "game/pch.h"
#include "game/states/controls_settings_menu.h"
#include "game/states/stick_settings_menu.h"
#include "game/states/keyboard_jump_menu.h"
#include "game/states/pad_buttons_menu.h"
#include "game/states/profiles_menu.h"
#include "game/controls_text.h"
#include "game/sound_groups.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

ControlsSettingsMenu::ControlsSettingsMenu(MenuContext* context, int player) :
	SettingsPanelPage(context, player)
{
	this->volume_preview_sound_ = this->resolve_wave(
		this->presentation().controls_settings().volume_preview_sound.name);
}

void ControlsSettingsMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->save()->play_settings();
	const ControlsSettingsPresentation& look = this->panel_look();
	const MenuStyle style = this->panel_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.ink });
	this->set_prompts(MenuPrompt::controls);
	this->build_panel(style);
	const Vector2F value_limit(look.controls_position.x - look.art.focus_padding.x * 2.0f -
		look.art.shadow_offset - this->calculate_widget_position(1, 0).x,
		look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
	const auto fit_action = [&look](UiText* widget)
	{
		const RectangleF bounds = widget->bounds();
		widget->set_scale(std::min({ 1.0f,
			(look.controls_position.x - look.art.focus_padding.x * 2.0f - look.art.shadow_offset - bounds.x) / bounds.width,
			(look.layout.spacing.y - look.art.focus_padding.y * 2.0f) / bounds.height }));
	};
	this->add_text("heading", this->text().heading(MenuHeading::controls_audio), style.heading_font,
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
	UiText* sticks = this->add_text("sticks", this->text().item(MenuItem::pad_sticks), style.item_font,
		this->calculate_widget_position(0, 0), look.art.ink);
	this->focus_.add(sticks, [this]
	{
		this->play_wave(this->confirm_sound_);
		this->input()->reset_actions();
		this->context()->push(std::make_unique<StickSettingsMenu>(this->menu_context(), this->opening_slot()));
	});
	UiText* keyboard = this->add_text("keyboard_jump", this->text().item(MenuItem::keyboard_jump), style.item_font,
		this->calculate_widget_position(0, 1), look.art.ink);
	fit_action(keyboard);
	this->focus_.add(keyboard, [this]
	{
		this->play_wave(this->confirm_sound_);
		this->input()->reset_actions();
		this->context()->push(std::make_unique<KeyboardJumpMenu>(this->menu_context(), this->opening_slot()));
	});
	UiText* pad_buttons = this->add_text("pad_buttons", this->text().item(MenuItem::pad_buttons), style.item_font,
		this->calculate_widget_position(0, 2), look.art.ink);
	fit_action(pad_buttons);
	this->focus_.add(pad_buttons, [this]
	{
		this->play_wave(this->confirm_sound_);
		this->input()->reset_actions();
		this->context()->push(std::make_unique<PadButtonsMenu>(this->menu_context(), this->opening_slot()));
	});
	UiText* profiles = this->add_text("profiles", this->text().item(MenuItem::profiles), style.item_font,
		this->calculate_widget_position(0, 3), look.art.ink);
	fit_action(profiles);
	this->focus_.add(profiles, [this]
	{
		this->play_wave(this->confirm_sound_);
		this->input()->reset_actions();
		this->context()->push(std::make_unique<ProfilesMenu>(this->menu_context(), this->opening_slot()));
	});
	this->music_ = make_row("music", this->text().item(MenuItem::music_volume),
		4, this->selection_.music_volume, 0, 100, 10);
	this->sfx_ = make_row("sfx", this->text().item(MenuItem::sfx_volume),
		5, this->selection_.sfx_volume, 0, 100, 10);
	this->view_shake_ = std::make_unique<OptionRow>("view_shake", this->text().item(MenuItem::view_shake),
		this->calculate_widget_position(0, 6), this->calculate_widget_position(1, 6),
		style, look.art.ink, this->render_resources(),
		[this](Direction direction)
		{
			this->selection_.view_shake = direction == Direction::right;
			this->refresh_values();
			this->set_status(this->text().status.controls_pending,
				this->presentation().controls_settings().art.focus_ink);
		}, &look.art, value_limit);
	this->aim_marker_ = std::make_unique<OptionRow>("aim_marker", this->text().item(MenuItem::aim_marker),
		this->calculate_widget_position(0, 7), this->calculate_widget_position(1, 7),
		style, look.art.ink, this->render_resources(),
		[this](Direction direction)
		{
			this->selection_.aim_marker = direction == Direction::right;
			this->refresh_values();
			this->set_status(this->text().status.controls_pending,
				this->presentation().controls_settings().art.focus_ink);
		}, &look.art, value_limit);
	this->reduce_effects_ = std::make_unique<OptionRow>("reduce_effects", this->text().item(MenuItem::reduce_effects),
		this->calculate_widget_position(0, 8), this->calculate_widget_position(1, 8),
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
	UiText* apply = this->add_text("apply", this->text().item(MenuItem::apply), style.item_font,
		this->calculate_widget_position(0, 9), look.art.ink);
	fit_action(apply);
	UiText* back = this->add_text("back", this->text().item(MenuItem::back), style.item_font,
		this->calculate_widget_position(0, 10), look.art.ink);
	this->status_ = this->add_text("status", L"", look.help_font, look.status_position, look.art.focus_ink);
	this->set_status(this->text().status.controls_pending, look.art.focus_ink);
	this->focus_.add(apply, [this] { this->apply_settings(); });
	this->focus_.add(back, [this] { this->close(); });
	this->controls_ = this->add_text("controls", L"",
		look.help_font, look.controls_position, look.art.ink);
	this->controls_limit_ = RectangleF(look.controls_position, look.controls_size);
	this->refresh_help();
}

void ControlsSettingsMenu::refresh_help()
{
	// MenuPage captured this copy when it was constructed. Refresh only the
	// effective keys, retaining the page's authored template, font and geometry.
	this->controls_->set_text(controls_text::controls_help(this->text().help.controls,
		this->save()->input_bindings()));
	this->controls_->set_scale(1.0f);
	const RectangleF bounds = this->controls_->bounds();
	this->controls_->set_scale(std::min({ 1.0f, this->controls_limit_.width / bounds.width,
		this->controls_limit_.height / bounds.height }));
}

void ControlsSettingsMenu::on_resume()
{
	SettingsPanelPage::on_resume();
	this->input()->reset_actions();
	this->refresh_help();
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
	this->music_->set_value(this->text().setting_percentage(this->selection_.music_volume));
	this->sfx_->set_value(this->text().setting_percentage(this->selection_.sfx_volume));
	this->view_shake_->set_value(this->selection_.view_shake ? this->text().values.on : this->text().values.off);
	this->aim_marker_->set_value(this->selection_.aim_marker ? this->text().values.on : this->text().values.off);
	this->reduce_effects_->set_value(this->selection_.reduce_effects ? this->text().values.on : this->text().values.off);
}

void ControlsSettingsMenu::apply_settings()
{
	// The child commits pad profiles independently of these shared preferences.
	this->selection_.sticks = this->save()->play_settings().sticks;
	this->selection_.pad_rumble = this->save()->play_settings().pad_rumble;
	this->selection_.pad_comfort = this->save()->play_settings().pad_comfort;
	const bool saved = this->save()->set_play_settings_and_save(this->selection_);
	// The settings in memory changed whether or not the file did. The groups
	// reach every playing and paused voice, so a music fade carries on.
	sound_groups::apply(*this->data()->audio_resources(), this->save()->play_settings());
	this->play_wave(this->confirm_sound_);
	this->set_status(saved ? this->text().status.controls_saved :
		this->text().status.active_save_failed,
		saved ? this->presentation().controls_settings().art.focus_ink :
		this->presentation().main_menu().options_warning_colour);
}

void ControlsSettingsMenu::update_page(float /*dt*/)
{
	if (!this->context()->active())
	{
		return;
	}
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->opening_slot() >= 0)
	{
		inputs = { this->opening_slot_input(inputs) };
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
						// The preview wave is in no sound group, because the SFX group
						// still carries the applied value.
						colourwars::SoundBankObject::play_wave(this->volume_preview_sound_,
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
