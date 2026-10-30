#include "game/pch.h"
#include "game/states/profiles_menu.h"
#include "game/player_control.h"
#include <algorithm>

using namespace labrador;
using namespace mattmath;

namespace
{
	std::wstring widened(std::string_view name)
	{
		return std::wstring(name.begin(), name.end());
	}
}

ProfilesMenu::ProfilesMenu(MenuContext* context, int player) : SettingsPanelPage(context, player) {}

void ProfilesMenu::init()
{
	this->input()->reset_actions();
	this->pressing_slot_ = this->opening_slot();
	// Open on the profile the opening pad wears, else the first.
	this->profile_ = this->save()->worn_profile(this->opening_slot());
	if (this->profile_ < 0 && !this->save()->profiles().list.empty()) { this->profile_ = 0; }
	const ControlsSettingsPresentation& look = this->panel_look();
	const MenuStyle style = this->panel_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, style.disabled });
	this->set_prompts(MenuPrompt::profiles);
	this->build_panel(style);
	const ProfilesMenuText& copy = this->text().profiles;
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
	this->profile_row_ = row("profile", copy.profile, 0, [this](Direction direction) { this->change_profile(direction); });
	this->name_row_ = row("name", copy.name, 1, [](Direction) {});
	this->boost_row_ = row("boost", copy.boost, 2, [this](Direction direction) { this->change_boost(direction); });
	this->wearer_row_ = row("worn_by", copy.worn_by, 3, [this](Direction direction) { this->change_wearer(direction); });
	for (OptionRow* option : { this->profile_row_.get(), this->name_row_.get(), this->boost_row_.get(), this->wearer_row_.get() })
	{
		this->text_container_->add_child(option);
	}
	this->focus_.add(this->profile_row_.get());
	this->focus_.add(this->name_row_.get(), [this] { this->begin_name_edit(); });
	this->focus_.add(this->boost_row_.get());
	this->focus_.add(this->wearer_row_.get());
	const auto action = [&](const char* name, const std::wstring& label, int index, Button::Action activate)
	{
		const Vector2F position = this->calculate_widget_position(0, index);
		UiText* widget = this->add_text(name, label, style.item_font, position);
		fit_text(*widget, RectangleF(position, Vector2F(
			look.controls_position.x - look.art.focus_padding.x * 2.0f - look.art.shadow_offset - position.x,
			look.layout.spacing.y - look.art.focus_padding.y * 2.0f)));
		this->focus_.add(widget, std::move(activate));
		return widget;
	};
	this->new_ = action("new_profile", copy.new_profile, 4, [this] { this->add_profile(); });
	this->delete_ = action("delete_profile", copy.delete_profile, 5, [this] { this->delete_profile(); });
	action("back", this->text().item(MenuItem::back), 6, [this] { this->close(); });
	this->status_limit_ = this->status_limit(style);
	this->help_ = this->add_text("help", copy.help, look.help_font, look.controls_position);
	fit_text(*this->help_, RectangleF(look.controls_position, look.controls_size));
	this->status_ = this->add_text("status", L"", look.help_font, this->status_limit_.position());
	this->refresh_values();
	if (this->profile_ < 0) { this->focus_.set_focused(0, this->new_); }
	this->set_status(copy.idle_status);
}

void ProfilesMenu::set_status(const std::wstring& value, bool warning)
{
	this->status_->set_text(value);
	this->status_->set_colour(warning ? this->presentation().main_menu().options_warning_colour :
		this->presentation().controls_settings().art.focus_ink);
	fit_text(*this->status_, this->status_limit_);
}

void ProfilesMenu::report_save(bool saved, const std::wstring& success)
{
	this->set_status(saved ? success : this->text().status.active_save_failed, !saved);
}

void ProfilesMenu::refresh_values()
{
	const PlayerProfiles& profiles = this->save()->profiles();
	const ProfilesMenuText& copy = this->text().profiles;
	if (this->profile_ >= static_cast<int>(profiles.list.size()))
	{
		this->profile_ = static_cast<int>(profiles.list.size()) - 1;
	}
	const bool any = this->profile_ >= 0;
	for (const UiWidget* widget : { static_cast<const UiWidget*>(this->profile_row_.get()),
		static_cast<const UiWidget*>(this->name_row_.get()), static_cast<const UiWidget*>(this->boost_row_.get()),
		static_cast<const UiWidget*>(this->wearer_row_.get()), static_cast<const UiWidget*>(this->delete_) })
	{
		this->focus_.set_enabled(widget, any);
	}
	this->focus_.set_enabled(this->new_, !profiles.full());
	if (!any)
	{
		this->profile_row_->set_value(copy.none);
		for (OptionRow* option : { this->name_row_.get(), this->boost_row_.get(), this->wearer_row_.get() })
		{
			option->set_value(L"");
		}
		return;
	}
	const PlayerProfile& profile = profiles.list[static_cast<size_t>(this->profile_)];
	this->profile_row_->set_value(widened(profile.name));
	if (this->name_edit_)
	{
		// The cursor's character sits in brackets; spaces show as underscores.
		std::wstring shown;
		for (size_t index = 0; index < this->name_edit_->buffer.size(); ++index)
		{
			const wchar_t character = this->name_edit_->buffer[index] == ' ' ? L'_' :
				static_cast<wchar_t>(this->name_edit_->buffer[index]);
			if (index == this->name_edit_->cursor) { shown += L'['; shown += character; shown += L']'; }
			else { shown += character; }
		}
		this->name_row_->set_value(shown);
	}
	else
	{
		this->name_row_->set_value(widened(profile.name));
	}
	this->boost_row_->set_value(this->text().handicap_label(profile.boost));
	const int wearer = this->save()->profile_wearer(this->profile_);
	this->wearer_row_->set_value(wearer < 0 ? copy.nobody : this->text().device_label(wearer));
}

void ProfilesMenu::change_profile(Direction direction)
{
	const int count = static_cast<int>(this->save()->profiles().list.size());
	if (count == 0) { return; }
	this->profile_ = std::clamp(this->profile_ + (direction == Direction::right ? 1 : -1), 0, count - 1);
	this->delete_armed_ = false;
	this->refresh_values();
	this->set_status(this->text().profiles.idle_status);
}

void ProfilesMenu::change_boost(Direction direction)
{
	if (this->profile_ < 0) { return; }
	const int boost = std::clamp(this->save()->profiles().list[static_cast<size_t>(this->profile_)].boost +
		(direction == Direction::right ? 1 : -1), 0, handicaps::count - 1);
	this->report_save(this->save()->set_profile_boost_and_save(this->profile_, boost),
		this->text().profiles.saved_status);
	this->refresh_values();
}

void ProfilesMenu::change_wearer(Direction direction)
{
	if (this->profile_ < 0) { return; }
	// Nobody, then Pad 1-4. Moving a profile takes it off its old pad; the
	// new pad takes off whatever it wore.
	const int wearer = this->save()->profile_wearer(this->profile_);
	const int next = std::clamp(wearer + (direction == Direction::right ? 1 : -1), -1, input_slots::pad_count - 1);
	if (next == wearer) { return; }
	if (wearer >= 0) { this->save()->wear_profile(wearer, -1); }
	if (next >= 0)
	{
		this->save()->wear_profile(next, -1);
		this->save()->wear_profile(next, this->profile_);
	}
	this->refresh_values();
	this->set_status(this->text().profiles.idle_status);
}

void ProfilesMenu::add_profile()
{
	this->delete_armed_ = false;
	if (this->save()->profiles().full())
	{
		this->set_status(this->text().profiles.full_status, true);
		return;
	}
	bool saved = false;
	this->profile_ = this->save()->add_profile_and_save(this->save()->profiles().next_free_name(),
		this->pressing_slot_, saved);
	this->refresh_values();
	this->report_save(saved, this->text().profiles.saved_status);
	// A new profile is named straight away.
	this->focus_.set_focused(0, this->name_row_.get());
	this->begin_name_edit();
}

void ProfilesMenu::delete_profile()
{
	if (this->profile_ < 0) { return; }
	if (!this->delete_armed_)
	{
		this->delete_armed_ = true;
		this->play_wave(this->direction_sound_);
		this->set_status(this->text().profiles.delete_confirm_status, true);
		return;
	}
	this->delete_armed_ = false;
	const bool saved = this->save()->delete_profile_and_save(this->profile_);
	this->play_wave(this->confirm_sound_);
	this->refresh_values();
	if (this->profile_ < 0) { this->focus_.set_focused(0, this->new_); }
	this->report_save(saved, this->text().profiles.deleted_status);
}

void ProfilesMenu::begin_name_edit()
{
	if (this->profile_ < 0) { return; }
	this->delete_armed_ = false;
	this->name_edit_ = NameEdit{ this->save()->profiles().list[static_cast<size_t>(this->profile_)].name, 0 };
	this->set_prompts(MenuPrompt::profile_name);
	this->play_wave(this->confirm_sound_);
	this->refresh_values();
	this->set_status(this->text().profiles.editing_status);
}

void ProfilesMenu::finish_name_edit(bool keep)
{
	const std::string buffer = this->name_edit_->buffer;
	if (keep)
	{
		std::string name;
		try { name = player_profiles::normalized_name(buffer); }
		catch (const std::invalid_argument&) { this->play_wave(this->cancel_sound_); return; }
		const int existing = this->save()->profiles().find(name);
		if (existing >= 0 && existing != this->profile_)
		{
			this->play_wave(this->cancel_sound_);
			this->set_status(this->text().profiles.name_taken_status, true);
			return;
		}
		this->name_edit_.reset();
		this->report_save(this->save()->rename_profile_and_save(this->profile_, name),
			this->text().profiles.saved_status);
		this->play_wave(this->confirm_sound_);
	}
	else
	{
		this->name_edit_.reset();
		this->play_wave(this->cancel_sound_);
		this->set_status(this->text().profiles.idle_status);
	}
	this->set_prompts(MenuPrompt::profiles);
	this->input()->reset_actions();
	this->refresh_values();
}

void ProfilesMenu::update_name_edit(const ProcessedMenuInput& input)
{
	NameEdit& edit = *this->name_edit_;
	if (input.action == MenuInputAction::back) { this->finish_name_edit(false); return; }
	if (input.action == MenuInputAction::proceed) { this->finish_name_edit(true); return; }
	const std::string_view characters = player_profiles::name_characters();
	switch (input.direction)
	{
	case Direction::up:
	case Direction::down:
	{
		if (edit.cursor >= edit.buffer.size()) { edit.buffer.resize(edit.cursor + 1, ' '); }
		const size_t count = characters.size();
		const size_t at = characters.find(edit.buffer[edit.cursor]);
		const size_t current = at == std::string_view::npos ? 0 : at;
		edit.buffer[edit.cursor] = characters[(current + (input.direction == Direction::up ? 1 : count - 1)) % count];
		break;
	}
	case Direction::left:
		if (edit.cursor == 0) { return; }
		--edit.cursor;
		break;
	case Direction::right:
		if (edit.cursor + 1 >= player_profiles::max_name_length) { return; }
		++edit.cursor;
		if (edit.cursor >= edit.buffer.size()) { edit.buffer.resize(edit.cursor + 1, ' '); }
		break;
	default:
		return;
	}
	this->play_wave(this->direction_sound_);
	this->refresh_values();
}

void ProfilesMenu::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	std::vector<int> slots;
	if (this->opening_slot() >= 0)
	{
		inputs = { this->opening_slot_input(inputs) };
		slots = { this->opening_slot() };
	}
	else
	{
		for (int slot = 0; slot < static_cast<int>(inputs.size()); ++slot) { slots.push_back(slot); }
	}
	for (size_t index = 0; index < inputs.size(); ++index)
	{
		const ProcessedMenuInput& input = inputs[index];
		if (input.action != MenuInputAction::none || input.direction != Direction::none)
		{
			this->pressing_slot_ = slots[index];
		}
		if (this->name_edit_)
		{
			this->update_name_edit(input);
			continue;
		}
		if (input.action == MenuInputAction::back) { this->close(); return; }
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.focused(0) != this->delete_) { this->delete_armed_ = false; }
			if (this->activate_focus() == Activation::ran) { return; }
		}
		else if (input.direction == Direction::left || input.direction == Direction::right)
		{
			const OptionRow* option = dynamic_cast<const OptionRow*>(this->focus_.focused(0));
			if (option != nullptr && this->focus_.enabled(option) && option != this->name_row_.get())
			{
				option->adjust(input.direction);
				this->press_focus();
				this->play_wave(this->direction_sound_);
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->delete_armed_ = false;
			this->play_wave(this->direction_sound_);
		}
	}
}
