#include "game/pch.h"
#include "game/states/stick_settings_menu.h"
#include "game/objects/stick_response.h"
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

	std::wstring percent(float value)
	{
		return std::to_wstring(rounded_percent(value)) + L"%";
	}
}

StickSettingsMenu::StickSettingsMenu(MenuContext* context, int player) : PauseMenuPage(context, player) {}
StickSettingsMenu::~StickSettingsMenu() = default;

void StickSettingsMenu::init()
{
	this->input()->reset_actions();
	this->selection_ = this->save()->play_settings();
	if (input_slots::is_pad(this->pausing_player())) { this->slot_ = this->pausing_player(); }
	else
	{
		for (int slot = 0; slot < input_slots::pad_count; ++slot)
		{
			if (this->data()->gamepads()->connected(slot)) { this->slot_ = slot; break; }
		}
	}
	const ControlsSettingsPresentation& look = this->presentation().controls_settings();
	MenuStyle style = this->menu_style();
	style.item_font = look.item_font;
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.ink });
	this->set_prompts(MenuPrompt::sticks);
	this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet, style.fill_frame,
		RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), this->render_resources(),
		this->presentation().main_menu().home_background_colour);
	this->paper_ = std::make_unique<MenuPaperArtwork>(RectangleF(
		(Vector2F(menu_consts::DESIGN_RESOLUTION.x, style.footer_y) - look.box_size) / 2.0f,
		look.box_size), this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());
	this->preview_ = std::make_unique<StickResponsePreview>(this->render_resources(), &style, &look);
	this->texture_container_ = std::make_unique<UiContainer>("textures");
	for (UiObject* object : { static_cast<UiObject*>(this->background_.get()),
		static_cast<UiObject*>(this->paper_.get()), static_cast<UiObject*>(this->preview_.get()) })
	{
		this->texture_container_->add_child(object);
	}
	this->text_container_ = std::make_unique<UiContainer>("text");
	const auto add_text = [&](const char* name, const std::wstring& value,
		const std::string& font, const Vector2F& position)
	{
		this->text_.push_back(std::make_unique<UiText>(name, value, font, position,
			this->render_resources(), look.art.ink));
		UiText* text = this->text_.back().get();
		this->text_container_->add_child(text);
		return text;
	};
	add_text("heading", this->text().heading(MenuHeading::pad_sticks), style.heading_font, look.heading_position);
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
	for (OptionRow* option : { this->pad_.get(), this->movement_.get(), this->aim_.get() })
	{
		this->text_container_->add_child(option);
		this->focus_.add(option);
	}
	this->focus_.add(add_text("apply", this->text().item(MenuItem::apply), style.item_font,
		this->calculate_widget_position(0, 3)), [this] { this->apply_settings(); });
	this->focus_.add(add_text("back", this->text().item(MenuItem::back), style.item_font,
		this->calculate_widget_position(0, 4)), [this] { this->close(); });
	this->status_ = add_text("status", L"", look.help_font, look.status_position);
	this->connection_ = add_text("connection", L"", look.help_font, look.controls_position);
	const float label_height = this->render_resources()->measure_text(
		this->render_resources()->resolve_sprite_font(look.help_font), L"Move").y;
	this->raw_movement_ = add_text("raw_movement", L"", look.help_font,
		look.stick_preview.raw_movement.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y));
	this->movement_response_ = add_text("movement_response", L"", look.help_font,
		look.stick_preview.movement_response.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y));
	add_text("aim_label", this->text().help.aim_cutoff, look.help_font,
		look.stick_preview.aim.position() - Vector2F(0.0f, label_height + look.art.focus_padding.y));
	this->aim_readout_ = add_text("aim_response", L"", look.help_font, look.stick_preview.aim_readout_position);
	UiText* association = add_text("association",
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
	const float available = this->paper_->rectangle().right() - look.art.shadow_offset -
		look.art.focus_padding.x - look.status_position.x;
	this->status_->set_scale(1.0f);
	this->status_->set_scale(std::min(1.0f, available / this->status_->bounds().width));
}

void StickSettingsMenu::refresh_values()
{
	this->pad_->set_value(input_slots::device_name(this->slot_));
	this->movement_->set_value(percent(this->selection_.movement_deadzone(this->slot_,
		this->data()->tuning()->input().movement_deadzone)));
	this->aim_->set_value(percent(this->selection_.aim_deadzone(this->slot_)));
	this->refresh_preview();
}

void StickSettingsMenu::refresh_preview()
{
	const GamepadState& pad = this->data()->gamepads()->state(this->slot_);
	const Vector2F left = pad.connected ? pad.left_stick : Vector2F::ZERO;
	const Vector2F right = pad.connected ? pad.right_stick : Vector2F::ZERO;
	const float authored = this->data()->tuning()->input().movement_deadzone;
	const StickResponse response = stick_response(left, right, authored, this->selection_, this->slot_);
	this->preview_->set_response(left, right, response, this->selection_.movement_deadzone(this->slot_, authored),
		this->selection_.aim_deadzone(this->slot_));
	this->connection_->set_text(this->text().stick_connection(this->slot_, pad.connected));
	this->raw_movement_->set_text(this->text().stick_raw_movement(rounded_percent(left.x)));
	this->movement_response_->set_text(this->text().stick_movement_response(rounded_percent(response.movement)));
	const Vector2F aim = response.aim.normalized();
	const int raw_x = rounded_percent(right.x), raw_y = rounded_percent(right.y);
	this->aim_readout_->set_text(response.aim == Vector2F::ZERO ?
		this->text().stick_aim_inactive(raw_x, raw_y) :
		this->text().stick_aim_active(raw_x, raw_y, rounded_percent(aim.x), rounded_percent(aim.y)));
}

void StickSettingsMenu::apply_settings()
{
	// Other preference pages may have pending edits; commit only this page's scope.
	PlaySettings applied = this->save()->play_settings();
	applied.sticks = this->selection_.sticks;
	const bool saved = this->save()->set_play_settings_and_save(applied);
	this->play_wave(this->confirm_sound_);
	this->set_status(saved ? this->text().status.sticks_saved : this->text().status.active_save_failed, !saved);
}

void StickSettingsMenu::close()
{
	this->play_wave(this->cancel_sound_);
	this->input()->reset_actions();
	this->context()->pop();
}

bool StickSettingsMenu::debug_dismiss(bool /*leaving_match*/) { this->close(); return true; }

void StickSettingsMenu::update_page(float /*dt*/)
{
	if (!this->context()->active()) { return; }
	this->refresh_preview();
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->pausing_player() >= 0) { inputs = { this->pausing_player_input(inputs) }; }
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
