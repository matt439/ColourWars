#include "game/pch.h"
#include "game/states/stick_settings_menu.h"
#include "game/match_setup.h"
#include "game/objects/stick_response.h"
#include "game/objects/crude_drawing.h"
#include <algorithm>
#include <cmath>

using namespace labrador;
using namespace mattmath;

// Snapshot-only drawing: repeated draws and resize never poll or change input.
class StickResponsePreview final : public UiTexture
{
public:
	StickResponsePreview(RenderResources* resources, const MenuStyle* style, const ControlsSettingsPresentation* look) :
		UiTexture("stick_preview", style->sprite_sheet, style->fill_frame,
			RectangleF(look->controls_position, look->controls_size), resources), look_(look)
	{
	}
	void set_response(const Vector2F& left, const Vector2F& right,
		const StickResponse& response, float movement_deadzone, float aim_deadzone)
	{
		this->left_ = left;
		this->right_ = right;
		this->response_ = response;
		this->movement_deadzone_ = movement_deadzone;
		this->aim_deadzone_ = aim_deadzone;
	}
	void draw(DrawList& list) const override
	{
		if (this->hidden()) { return; }
		const StickPreviewPresentation& view = this->look_->stick_preview;
		const MenuPaperArtPresentation& art = this->look_->art;
		const auto fill = [&](const RectangleF& box, const Colour& colour)
		{
			this->draw_with(list, box, this->frame(), colour, Vector2F::ZERO, SpriteFlip::none, 0.0f);
		};
		const float line = view.line_width;
		const auto track = [&](const RectangleF& box)
		{
			fill(box, art.ink);
			fill(RectangleF(box.x + line, box.y + line, box.width - line * 2.0f,
				box.height - line * 2.0f), art.paper);
		};
		track(view.raw_movement);
		const RectangleF& raw = view.raw_movement;
		const float half = (raw.width - view.marker_size) / 2.0f;
		fill(RectangleF(raw.center().x - half * this->movement_deadzone_, raw.y + line,
			half * this->movement_deadzone_ * 2.0f, raw.height - line * 2.0f), art.focus_fill);
		fill(RectangleF(raw.center().x + this->left_.x * half - line / 2.0f,
			raw.y, line, raw.height), art.ink);
		track(view.movement_response);
		const RectangleF& output = view.movement_response;
		const float travel = this->response_.movement * (output.width / 2.0f - line);
		if (travel != 0.0f)
		{
			fill(RectangleF(output.center().x + std::min(travel, 0.0f), output.y + line,
				std::abs(travel), output.height - line * 2.0f), art.ink);
		}
		fill(RectangleF(output.center().x - line / 2.0f, output.y, line, output.height), art.ink);
		track(view.aim);
		const Vector2F centre = view.aim.center();
		const float radius = (view.aim.width - view.marker_size) / 2.0f;
		// Rasterise the radial cutoff with bounded, texel-sized steps.
		constexpr int points = 96;
		for (int point = 0; point < points; ++point)
		{
			const float angle = static_cast<float>(point) * 6.283185307f / static_cast<float>(points);
			const Vector2F position = centre + Vector2F(std::cos(angle), std::sin(angle)) *
				(radius * this->aim_deadzone_);
			fill(RectangleF(position - Vector2F(line / 2.0f), Vector2F(line)), art.ink);
		}
		const Vector2F raw_point = centre + this->right_ * radius;
		fill(RectangleF(raw_point - Vector2F(view.marker_size / 2.0f), Vector2F(view.marker_size)), art.ink);
		fill(RectangleF(raw_point - Vector2F(view.marker_size / 2.0f - line),
			Vector2F(view.marker_size - line * 2.0f)), art.paper);
		if (this->response_.aim != Vector2F::ZERO)
		{
			const Vector2F active = centre + this->response_.aim.normalized() * radius;
			fill(RectangleF(active - Vector2F(view.marker_size / 2.0f), Vector2F(view.marker_size)), art.ink);
		}
	}
private:
	const ControlsSettingsPresentation* look_;
	Vector2F left_;
	Vector2F right_;
	StickResponse response_;
	float movement_deadzone_ = 0.0f;
	float aim_deadzone_ = 0.0f;
};

namespace
{
	int rounded_percent(float value)
	{
		return static_cast<int>(std::lround(value * 100.0f));
	}
}

StickSettingsMenu::StickSettingsMenu(MenuContext* context, int player) : SettingsPanelPage(context, player) {}
StickSettingsMenu::~StickSettingsMenu() = default;

void StickSettingsMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->settings_session()->play_settings();
	this->rumble_edited_.fill(false);
	this->comfort_edited_.fill(false);
	if (input_slots::is_pad(this->opening_slot())) { this->slot_ = this->opening_slot(); }
	else
	{
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			if (this->data()->gamepads()->connected(this->preview_device(slot))) { this->slot_ = slot; break; }
		}
	}
	const ControlsSettingsPresentation& look = this->panel_look();
	const MenuStyle style = this->panel_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.ink });
	this->set_prompts(MenuPrompt::sticks);
	this->build_panel(style);
	this->preview_ = std::make_unique<StickResponsePreview>(this->render_resources(), &style, &look);
	this->texture_container_->add_child(this->preview_.get());
	this->add_text("heading", this->text().heading(MenuHeading::pad_sticks), style.heading_font, look.heading_position);
	const Vector2F value_limit(
		menu_geometry::settings_action_width(look, this->calculate_widget_position(1, 0).x),
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
	const auto adjust_deadzone = [this](bool movement, Direction direction)
	{
		StickSettings& settings = this->selection_.sticks[static_cast<size_t>(this->slot_)];
		int& value = movement ? settings.movement_deadzone : settings.aim_deadzone;
		const float effective = movement ? this->selection_.movement_deadzone(this->slot_,
			this->data()->tuning()->input().movement_deadzone) : this->selection_.aim_deadzone(this->slot_);
		value = std::clamp(static_cast<int>(std::lround(effective * 100.0f)) +
			(direction == Direction::right ? 5 : -5), 0, 90);
		this->refresh_values();
		this->set_status(this->text().status.sticks_pending);
	};
	this->movement_ = row("movement", this->text().item(MenuItem::move_deadzone), 1,
		[adjust_deadzone](Direction direction) { adjust_deadzone(true, direction); });
	this->aim_ = row("aim", this->text().item(MenuItem::aim_deadzone), 2,
		[adjust_deadzone](Direction direction) { adjust_deadzone(false, direction); });
	this->rumble_ = row("rumble", this->text().item(MenuItem::rumble), 3,
		[this](Direction direction)
		{
			const size_t slot = static_cast<size_t>(this->slot_);
			this->selection_.pad_rumble[slot] = direction == Direction::right;
			this->rumble_edited_[slot] = true;
			this->refresh_values();
			this->set_status(this->text().status.sticks_pending);
		});
	// Off, Shared, On: Left leans to Off and Right to On, as on the shared rows.
	const auto comfort_row = [this, &row](const char* name, MenuItem item, int index,
		SeatSwitch SeatComfort::* field)
	{
		return row(name, this->text().item(item), index, [this, field](Direction direction)
		{
			const size_t slot = static_cast<size_t>(this->slot_);
			SeatSwitch& value = this->selection_.pad_comfort[slot].*field;
			value = static_cast<SeatSwitch>(std::clamp(static_cast<int>(value) +
				(direction == Direction::right ? 1 : -1), static_cast<int>(SeatSwitch::off),
				static_cast<int>(SeatSwitch::on)));
			this->comfort_edited_[slot] = true;
			this->refresh_values();
			this->set_status(this->text().status.sticks_pending);
		});
	};
	this->view_shake_ = comfort_row("view_shake", MenuItem::view_shake, 4, &SeatComfort::view_shake);
	this->aim_marker_ = comfort_row("aim_marker", MenuItem::aim_marker, 5, &SeatComfort::aim_marker);
	this->reduce_effects_ = comfort_row("reduce_effects", MenuItem::reduce_effects, 6, &SeatComfort::reduce_effects);
	for (OptionRow* option : { this->pad_.get(), this->movement_.get(), this->aim_.get(), this->rumble_.get(),
		this->view_shake_.get(), this->aim_marker_.get(), this->reduce_effects_.get() })
	{
		this->text_container_->add_child(option);
		this->focus_.add(option);
	}
	UiText* apply = this->add_text("apply", this->text().item(MenuItem::apply), style.item_font,
		this->calculate_widget_position(0, 7));
	const RectangleF apply_bounds = apply->bounds();
	apply->set_scale(std::min({ 1.0f,
		menu_geometry::settings_action_width(look, apply_bounds.x) / apply_bounds.width,
		(look.layout.spacing.y - look.art.focus_padding.y * 2.0f) / apply_bounds.height }));
	this->focus_.add(apply, [this] { this->apply_settings(); });
	this->focus_.add(this->add_text("back", this->text().item(MenuItem::back), style.item_font,
		this->calculate_widget_position(0, 8)), [this] { this->close(); });
	this->status_ = this->add_text("status", L"", look.help_font, look.status_position);
	this->connection_ = this->add_text("connection", L"", look.help_font, look.controls_position);
	const float label_height = this->render_resources()->measure_text(
		this->render_resources()->resolve_sprite_font(look.help_font), L"Move").y;
	this->raw_movement_ = this->add_text("raw_movement", L"", look.help_font,
		look.stick_preview.raw_movement.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y));
	this->movement_response_ = this->add_text("movement_response", L"", look.help_font,
		look.stick_preview.movement_response.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y));
	this->add_text("aim_label", this->text().help.aim_cutoff, look.help_font,
		look.stick_preview.aim.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y));
	this->aim_readout_ = this->add_text("aim_response", L"", look.help_font, look.stick_preview.aim_readout_position);
	UiText* association = this->add_text("association",
		this->text().help.pad_association,
		look.help_font, look.stick_preview.association_position);
	const Vector2F available = look.controls_position + look.controls_size - look.stick_preview.association_position;
	association->set_scale(std::min({ 1.0f, available.x / association->bounds().width,
		available.y / association->bounds().height }));
	this->set_status(this->text().status.sticks_pending);
	this->refresh_values();
}

void StickSettingsMenu::set_status(const std::wstring& text, bool warning)
{
	this->status_->set_text(text);
	const ControlsSettingsPresentation& look = this->presentation().controls_settings();
	this->status_->set_colour(warning ? this->presentation().main_menu().options_warning_colour : look.art.focus_ink);
	const float available = menu_geometry::paper_text_right(this->paper_->rectangle(), look.art) -
		look.status_position.x;
	this->status_->set_scale(1.0f);
	this->status_->set_scale(std::min(1.0f, available / this->status_->bounds().width));
}

void StickSettingsMenu::refresh_values()
{
	// A pad wearing a profile edits the profile, so the row names it.
	const int worn = this->settings_session()->worn_profile(this->slot_);
	this->pad_->set_value(this->text().worn_profile_label(this->text().device_label(this->slot_),
		worn < 0 ? std::string_view() : std::string_view(this->save()->profiles().list[static_cast<size_t>(worn)].name)));
	this->movement_->set_value(this->text().setting_percentage(rounded_percent(this->selection_.movement_deadzone(this->slot_,
		this->data()->tuning()->input().movement_deadzone))));
	this->aim_->set_value(this->text().setting_percentage(rounded_percent(this->selection_.aim_deadzone(this->slot_))));
	this->rumble_->set_value(this->selection_.rumble_enabled(this->slot_) ? this->text().values.on : this->text().values.off);
	const SeatComfort& comfort = this->selection_.seat_comfort(this->slot_);
	const auto comfort_value = [this](SeatSwitch value, bool shared)
	{
		return value == SeatSwitch::shared ? this->text().setting_shared(shared) :
			(value == SeatSwitch::on ? this->text().values.on : this->text().values.off);
	};
	this->view_shake_->set_value(comfort_value(comfort.view_shake, this->selection_.view_shake));
	this->aim_marker_->set_value(comfort_value(comfort.aim_marker, this->selection_.aim_marker));
	this->reduce_effects_->set_value(comfort_value(comfort.reduce_effects, this->selection_.reduce_effects));
	this->refresh_preview();
}

// A recovered seat keeps its logical profile while its input arrives on another
// device. Unpaused Options read physical slots even if the shared setup kept a mapping.
int StickSettingsMenu::preview_device(int slot) const
{
	return this->opening_slot() >= 0 ? this->level_settings()->device_slot(slot) : slot;
}

void StickSettingsMenu::refresh_preview()
{
	const GamepadState& pad = this->data()->gamepads()->state(this->preview_device(this->slot_));
	const Vector2F left = pad.connected ? pad.left_stick : Vector2F::ZERO;
	const Vector2F right = pad.connected ? pad.right_stick : Vector2F::ZERO;
	const float authored = this->data()->tuning()->input().movement_deadzone;
	const StickResponse response = stick_response(left, right, authored, this->selection_, this->slot_);
	this->preview_->set_response(left, right, response, this->selection_.movement_deadzone(this->slot_, authored),
		this->selection_.aim_deadzone(this->slot_));
	// Each readout's text follows only its integer inputs, so it is rebuilt
	// when they change; the empty records force the first write.
	const std::array<int, 2> connection = { this->slot_, pad.connected ? 1 : 0 };
	if (this->shown_connection_ != connection)
	{
		this->connection_->set_text(this->text().stick_connection(this->slot_, pad.connected));
		this->shown_connection_ = connection;
	}
	const int raw_movement = rounded_percent(left.x);
	if (this->shown_raw_movement_ != raw_movement)
	{
		this->raw_movement_->set_text(this->text().stick_raw_movement(raw_movement));
		this->shown_raw_movement_ = raw_movement;
	}
	const int movement_response = rounded_percent(response.movement);
	if (this->shown_movement_response_ != movement_response)
	{
		this->movement_response_->set_text(this->text().stick_movement_response(movement_response));
		this->shown_movement_response_ = movement_response;
	}
	const bool aim_active = !(response.aim == Vector2F::ZERO);
	const Vector2F aim = response.aim.normalized();
	const int raw_x = rounded_percent(right.x), raw_y = rounded_percent(right.y);
	const std::array<int, 5> aim_inputs = { aim_active ? 1 : 0, raw_x, raw_y,
		aim_active ? rounded_percent(aim.x) : 0, aim_active ? rounded_percent(aim.y) : 0 };
	if (this->shown_aim_ != aim_inputs)
	{
		this->aim_readout_->set_text(aim_active ?
			this->text().stick_aim_active(raw_x, raw_y, aim_inputs[3], aim_inputs[4]) :
			this->text().stick_aim_inactive(raw_x, raw_y));
		this->shown_aim_ = aim_inputs;
	}
}

void StickSettingsMenu::apply_settings()
{
	// Other preference pages may have pending edits; commit only this page's scope.
	PlaySettings applied = this->settings_session()->play_settings();
	applied.sticks = this->selection_.sticks;
	for (size_t slot = 0; slot < this->rumble_edited_.size(); ++slot)
	{
		if (this->rumble_edited_[slot]) { applied.pad_rumble[slot] = this->selection_.pad_rumble[slot]; }
		if (this->comfort_edited_[slot]) { applied.pad_comfort[slot] = this->selection_.pad_comfort[slot]; }
	}
	const bool saved = this->settings_session()->set_play_settings_and_save(applied);
	this->selection_.pad_rumble = applied.pad_rumble;
	this->selection_.pad_comfort = applied.pad_comfort;
	this->rumble_edited_.fill(false);
	this->comfort_edited_.fill(false);
	this->refresh_values();
	this->play_wave(this->confirm_sound_);
	this->set_status(saved ? this->text().status.sticks_saved : this->text().status.active_save_failed, !saved);
}

void StickSettingsMenu::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	this->refresh_preview();
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
