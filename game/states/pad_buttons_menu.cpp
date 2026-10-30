#include "game/pch.h"
#include "game/states/pad_buttons_menu.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

PadButtonsMenu::PadButtonsMenu(MenuContext* context, int player) : SettingsPanelPage(context, player) {}

void PadButtonsMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->save()->input_bindings();
	if (input_slots::is_pad(this->opening_slot())) { this->slot_ = this->opening_slot(); }
	else
	{
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			if (this->data()->gamepads()->connected(slot)) { this->slot_ = slot; break; }
		}
	}
	const ControlsSettingsPresentation& look = this->panel_look();
	const MenuStyle style = this->panel_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, style.disabled });
	this->set_prompts(MenuPrompt::sticks);
	this->build_panel(style);
	const auto& copy = this->text().pad_buttons;
	this->add_text("heading", copy.heading, style.heading_font, look.heading_position);
	const Vector2F value_limit(look.controls_position.x - look.art.focus_padding.x * 2.0f -
		look.art.shadow_offset - this->calculate_widget_position(1, 0).x,
		look.layout.spacing.y - look.art.focus_padding.y * 2.0f);
	const auto row = [&](const char* name, const std::wstring& label, int index, OptionRow::Adjust adjust)
	{
		return std::make_unique<OptionRow>(name, label, this->calculate_widget_position(0, index),
			this->calculate_widget_position(1, index), style, look.art.ink, this->render_resources(),
			std::move(adjust), &look.art, value_limit);
	};
	this->pad_ = row("pad", this->text().item(MenuItem::pad), 0, [this](Direction direction)
	{
		this->slot_ = std::clamp(this->slot_ + (direction == Direction::right ? 1 : -1),
			0, input_slots::pad_count - 1);
		this->refresh_values();
	});
	const std::array<const char*, entry_count> names{ "jump", "jump_alternate", "shoot",
		"shoot_alternate", "pause", "pause_alternate" };
	for (size_t entry = 0; entry < entry_count; ++entry)
	{
		this->entries_[entry] = row(names[entry], copy.row(entry), static_cast<int>(entry + 1),
			[this, entry](Direction direction) { this->choose(entry, direction); });
	}
	this->text_container_->add_child(this->pad_.get());
	this->focus_.add(this->pad_.get());
	for (const auto& option : this->entries_)
	{
		this->text_container_->add_child(option.get());
		this->focus_.add(option.get());
	}
	const auto action = [&](const char* name, const std::wstring& label, int index, Button::Action activate)
	{
		const Vector2F position = this->calculate_widget_position(0, index);
		UiText* widget = this->add_text(name, label, style.item_font, position);
		fit_text(*widget, RectangleF(position, Vector2F(
			look.controls_position.x - look.art.focus_padding.x * 2.0f - look.art.shadow_offset - position.x,
			look.layout.spacing.y - look.art.focus_padding.y * 2.0f)));
		this->focus_.add(widget, std::move(activate));
	};
	action("reset", copy.reset, 7, [this] { this->reset_pending(); });
	action("apply", this->text().item(MenuItem::apply), 8, [this] { this->apply_settings(); });
	action("back", this->text().item(MenuItem::back), 9, [this] { this->close(); });
	this->status_limit_ = this->status_limit(style);
	this->help_ = this->add_text("help", copy.help, look.help_font, look.controls_position);
	fit_text(*this->help_, RectangleF(look.controls_position, look.controls_size));
	this->status_ = this->add_text("status", L"", look.help_font, this->status_limit_.position());
	this->refresh_values();
	this->set_status(copy.pending_status);
}

void PadButtonsMenu::set_status(const std::wstring& value, bool warning)
{
	this->status_->set_text(value);
	this->status_->set_colour(warning ? this->presentation().main_menu().options_warning_colour :
		this->presentation().controls_settings().art.focus_ink);
	fit_text(*this->status_, this->status_limit_);
}

void PadButtonsMenu::refresh_values()
{
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (!this->edited_[slot]) { this->selection_.pads[slot] = this->save()->input_bindings().pads[slot]; }
	}
	// A pad wearing a profile edits the profile, so the row names it.
	const int worn = this->save()->worn_profile(this->slot_);
	this->pad_->set_value(this->text().worn_profile_label(this->text().device_label(this->slot_),
		worn < 0 ? std::string_view() : std::string_view(this->save()->profiles().list[static_cast<size_t>(worn)].name)));
	const PadProfile& profile = this->pending_profile();
	for (size_t entry = 0; entry < entry_count; ++entry)
	{
		const PadActionChoice& choice = profile.actions[entry / 2];
		this->entries_[entry]->set_value(std::wstring(pad_control_label(entry % 2 == 0 ? choice.primary : choice.alternate)));
	}
}

void PadButtonsMenu::choose(size_t entry, Direction direction)
{
	const PadAction action = static_cast<PadAction>(entry / 2);
	const bool alternate = entry % 2 == 1;
	PadProfile& profile = this->selection_.pads[this->slot_];
	PadControl& current = alternate ? profile[action].alternate : profile[action].primary;
	// Primaries cycle the eleven controls; alternates also offer None first.
	std::vector<PadControl> choices;
	if (alternate) { choices.push_back(PadControl::none); }
	for (const PadControl control : pad_controls()) { choices.push_back(control); }
	const int step = direction == Direction::right ? 1 : -1;
	int index = static_cast<int>(std::distance(choices.begin(), std::find(choices.begin(), choices.end(), current)));
	bool skipped = false;
	for (index += step; index >= 0 && index < static_cast<int>(choices.size()); index += step)
	{
		if (!pad_control_conflict(profile, action, alternate, choices[static_cast<size_t>(index)])) { break; }
		skipped = true;
	}
	if (index >= 0 && index < static_cast<int>(choices.size()))
	{
		current = choices[static_cast<size_t>(index)];
		this->edited_[this->slot_] = true;
	}
	this->refresh_values();
	this->set_status(skipped ? this->text().pad_buttons.conflict_status : this->text().pad_buttons.pending_status, skipped);
}

void PadButtonsMenu::reset_pending()
{
	this->selection_.pads[this->slot_] = PadProfile{};
	this->edited_[this->slot_] = true;
	this->refresh_values();
	this->set_status(this->text().pad_buttons.pending_status);
	this->play_wave(this->confirm_sound_);
}

void PadButtonsMenu::apply_settings()
{
	InputBindings applied = this->save()->input_bindings();
	for (int slot = 0; slot < input_slots::pad_count; ++slot)
	{
		if (this->edited_[slot]) { applied.pads[slot] = this->selection_.pads[slot]; }
	}
	const bool saved = this->save()->set_input_bindings_and_save(applied);
	if (saved) { this->edited_.fill(false); }
	this->refresh_values();
	this->play_wave(this->confirm_sound_);
	this->set_status(saved ? this->text().pad_buttons.saved_status : this->text().status.active_save_failed, !saved);
}

void PadButtonsMenu::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	this->refresh_values();
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->opening_slot() >= 0) { inputs = { this->opening_slot_input(inputs) }; }
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::back) { this->close(); return; }
		if (input.action == MenuInputAction::proceed)
		{
			if (this->activate_focus() == Activation::ran) { return; }
		}
		else if (input.direction == Direction::left || input.direction == Direction::right)
		{
			const OptionRow* option = dynamic_cast<const OptionRow*>(this->focus_.focused(0));
			if (option != nullptr && this->focus_.enabled(option))
			{
				option->adjust(input.direction);
				this->press_focus();
				this->play_wave(this->direction_sound_);
			}
		}
		else if (this->focus_.move(0, input.direction)) { this->play_wave(this->direction_sound_); }
	}
}
