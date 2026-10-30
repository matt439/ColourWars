#include "game/pch.h"
#include "game/objects/level.h"
#include "game/input_slots.h"
#include "game/controls_text.h"
#include "game/objects/team_gate_structure_paintable.h"

using namespace mattmath;
using namespace labrador;

void Level::build_opening_frame(RenderResources* render_resources)
{
	// Admit the builder's pending objects before the first frame is drawn.
	this->scene_->end_tick();
	this->admit_horizontal_structures();
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		Structure* structure = Collider::of(*object).structure();
		if (TeamGateStructurePaintable* gate = structure != nullptr ? structure->gate() : nullptr)
		{
			this->team_gates_.push_back(gate);
			this->team_gates_view_.push_back(gate);
		}
	}
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (Collider::of(*object).projectile() == nullptr)
		{
			this->projectile_targets_.push_back(object);
		}
	}
	this->projectile_count_ = this->count_projectiles();

	// The picture is the stage's authored overview and everyone in it, with
	// room around the spawns, inside the stage; its halves meet mid-stage.
	RectangleF picture = this->zoom_out_finish_bounds_;
	for (const Player* player : this->players_)
	{
		picture = RectangleF::union_of(picture, player->rectangle());
	}
	const float margin = picture.width * 0.03f;
	picture = RectangleF(picture.x - margin, picture.y - margin,
		picture.width + margin * 2.0f, picture.height + margin * 2.0f);
	const std::optional<RectangleF>& stage = this->scene_->bounds();
	const RectangleF limit = stage.value_or(this->camera_bounds_);
	const float left = std::max(picture.x, limit.x);
	const float top = std::max(picture.y, limit.y);
	const float right = std::min(picture.right(), limit.right());
	const float bottom = std::min(picture.bottom(), limit.bottom());
	this->picture_bounds_ = right > left && bottom > top ?
		RectangleF(left, top, right - left, bottom - top) : this->camera_bounds_;
	this->picture_middle_ = limit.x + limit.width / 2.0f;

	// Use the configured countdown for the opening beat.
	const MatchTuning& match = this->tuning_->match();
	const MatchPresentation& controls = this->presentation_->match();
	this->controls_look_ = { controls.reference_size, controls.controls_margin, controls.controls_padding,
		this->presentation_->menu().detail_shadow_offset, match.controls_shadow_offset, controls.controls_gap, match.controls_y };
	this->countdown_label_ = match.countdown > 0.0f ?
		std::to_wstring(static_cast<int>(std::ceil(match.countdown))) : L"GO!";
	this->countdown_art_ = std::make_unique<CountdownArt>(
		&this->presentation_->match(), &this->presentation_->opening(), render_resources);
	const MenuStyle& menu = this->presentation_->menu();
	this->countdown_heading_ = std::make_unique<colourwars::TextDropShadow>(
		L"GET READY", menu.detail_font, Vector2F::ZERO,
		render_resources, this->presentation_->match().countdown_art.ink,
		this->presentation_->match().countdown_art.paper,
		menu.detail_shadow_offset);
	this->resume_heading_ = std::make_unique<colourwars::TextDropShadow>(
		L"RESUMING", menu.detail_font, Vector2F::ZERO, render_resources,
		this->presentation_->match().countdown_art.ink,
		this->presentation_->match().countdown_art.paper, menu.detail_shadow_offset);
	this->replay_heading_ = std::make_unique<colourwars::TextDropShadow>(
		L"REPLAY", menu.detail_font, Vector2F::ZERO, render_resources,
		this->presentation_->match().countdown_art.ink,
		this->presentation_->match().countdown_art.paper, menu.detail_shadow_offset);
	this->centre_countdown_text();

	// Keep these reminders synchronized with PlayerInput bindings, showing only
	// the devices in this match. They are shared by all panes, not repeated per player.
	bool anyone_on_keyboard = false;
	bool anyone_on_gamepad = false;
	for (const Player* player : this->players_)
	{
		if (input_slots::is_keyboard_and_mouse(player->player_num()))
		{
			anyone_on_keyboard = true;
		}
		else if (input_slots::is_pad(player->player_num()))
		{
			anyone_on_gamepad = true;
			this->gamepad_slots_.push_back(player->player_num());
		}
	}

	const auto add_reminder = [this, render_resources, &match, &menu](
		const std::wstring& heading, const std::wstring& text)
	{
		ControlsReminder reminder;
		reminder.heading = std::make_unique<colourwars::TextDropShadow>(
			heading, menu.detail_font, Vector2F::ZERO, render_resources,
			menu.highlight, menu.shadow_colour, menu.detail_shadow_offset);
		reminder.text = std::make_unique<colourwars::TextDropShadow>(
			text, match.controls_font, Vector2F::ZERO, render_resources,
			match.controls_colour, match.controls_shadow_colour,
			match.controls_shadow_offset);
		this->controls_reminders_.push_back(std::move(reminder));
	};

	if (anyone_on_gamepad)
	{
		const InputBindings bindings = this->input_bindings_ ? *this->input_bindings_ : InputBindings{};
		add_reminder(L"GAMEPAD", controls_text::pad_controls_reminder(bindings, this->gamepad_slots_));
		this->gamepad_controls_ = this->controls_reminders_.back().text.get();
		this->gamepad_pads_ = bindings.pads;
	}
	if (anyone_on_keyboard)
	{
		const InputBindings bindings = this->input_bindings_ ? *this->input_bindings_ : InputBindings{};
		this->keyboard_controls_words_ = controls_text::keyboard_controls_reminder(bindings);
		add_reminder(L"KEYBOARD + MOUSE", this->keyboard_controls_words_);
		this->keyboard_controls_ = this->controls_reminders_.back().text.get();
	}
	this->centre_controls_text();

	this->refresh_layout();
}

void Level::refresh_keyboard_controls()
{
	bool changed = false;
	if (this->gamepad_controls_ && this->input_bindings_ && this->gamepad_pads_ != this->input_bindings_->pads)
	{
		this->gamepad_controls_->set_text(controls_text::pad_controls_reminder(*this->input_bindings_, this->gamepad_slots_));
		this->gamepad_pads_ = this->input_bindings_->pads;
		changed = true;
	}
	if (this->keyboard_controls_)
	{
		const std::wstring words = controls_text::keyboard_controls_reminder(
			this->input_bindings_ ? *this->input_bindings_ : InputBindings{});
		if (this->keyboard_controls_words_ != words)
		{
			this->keyboard_controls_->set_text(words);
			this->keyboard_controls_words_ = words;
			changed = true;
		}
	}
	if (changed) { this->centre_controls_text(); }
}

std::wstring_view Level::gamepad_controls_text() const
{ return this->gamepad_controls_ ? std::wstring_view(this->gamepad_controls_->text()) : std::wstring_view{}; }

RectangleF Level::gamepad_controls_text_bounds() const
{
	if (this->state_ == LevelState::start_countdown)
	{
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{ if (reminder.text.get() == this->gamepad_controls_) { return reminder.text_bounds; } }
	}
	return RectangleF::ZERO;
}

RectangleF Level::gamepad_controls_card_bounds() const
{
	if (this->state_ == LevelState::start_countdown)
	{
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{ if (reminder.text.get() == this->gamepad_controls_) { return reminder.card; } }
	}
	return RectangleF::ZERO;
}

std::wstring_view Level::keyboard_controls_text() const
{ return this->keyboard_controls_ ? std::wstring_view(this->keyboard_controls_->text()) : std::wstring_view{}; }

RectangleF Level::keyboard_controls_text_bounds() const
{
	if (this->state_ == LevelState::start_countdown)
	{
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{ if (reminder.text.get() == this->keyboard_controls_) { return reminder.text_bounds; } }
	}
	return RectangleF::ZERO;
}

RectangleF Level::keyboard_controls_card_bounds() const
{
	if (this->state_ == LevelState::start_countdown)
	{
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{ if (reminder.text.get() == this->keyboard_controls_) { return reminder.card; } }
	}
	return RectangleF::ZERO;
}

void Level::set_countdown_text(const std::wstring& text)
{
	this->countdown_label_ = text;
}

void Level::centre_countdown_text()
{
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const MatchPresentation& look = this->presentation_->match();
	const MenuStyle& menu = this->presentation_->menu();
	const float scale = this->opening_scale();
	const Vector2F card_size = look.countdown_size * scale;
	this->countdown_card_ = RectangleF(
		Vector2F((resolution.x - card_size.x) / 2.0f,
			resolution.y * look.countdown_center_y - card_size.y / 2.0f),
		card_size);
	const Vector2F padding = look.controls_padding * scale;
	const RectangleF heading_bounds = RectangleF::union_of(
		this->countdown_heading_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->countdown_heading_->text_bounds_at(menu.detail_shadow_offset, 1.0f));
	const float heading_scale = std::min(scale,
		(card_size.x - padding.x * 2.0f) / heading_bounds.width);
	this->countdown_heading_->set_scale(heading_scale);
	this->countdown_heading_->set_shadow_scale(heading_scale);
	this->countdown_heading_->set_shadow_offset(menu.detail_shadow_offset * heading_scale);
	this->countdown_heading_->set_position(Vector2F(
		(resolution.x - heading_bounds.width * heading_scale) / 2.0f,
		this->countdown_card_.y + padding.y) - heading_bounds.position() * heading_scale);
	const RectangleF resume_bounds = RectangleF::union_of(
		this->resume_heading_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->resume_heading_->text_bounds_at(menu.detail_shadow_offset, 1.0f));
	const float resume_scale = std::min(heading_scale,
		(card_size.x - padding.x * 2.0f) / resume_bounds.width);
	this->resume_heading_->set_scale(resume_scale);
	this->resume_heading_->set_shadow_scale(resume_scale);
	this->resume_heading_->set_shadow_offset(menu.detail_shadow_offset * resume_scale);
	this->resume_heading_->set_position(Vector2F(
		(resolution.x - resume_bounds.width * resume_scale) / 2.0f,
		this->countdown_card_.y + padding.y) - resume_bounds.position() * resume_scale);
	// The replay's card holds only its heading, at the top of the screen,
	// where it covers the least of the round it labels.
	const RectangleF replay_bounds = RectangleF::union_of(
		this->replay_heading_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->replay_heading_->text_bounds_at(menu.detail_shadow_offset, 1.0f));
	this->replay_heading_->set_scale(heading_scale);
	this->replay_heading_->set_shadow_scale(heading_scale);
	this->replay_heading_->set_shadow_offset(menu.detail_shadow_offset * heading_scale);
	const Vector2F replay_size = replay_bounds.size() * heading_scale + padding * 2.0f;
	this->replay_card_ = RectangleF(Vector2F((resolution.x - replay_size.x) / 2.0f, padding.y),
		replay_size);
	this->replay_heading_->set_position(this->replay_card_.position() + padding -
		replay_bounds.position() * heading_scale);
	const float number_top = this->countdown_card_.y + padding.y * 2.0f +
		heading_bounds.height * heading_scale;
	this->countdown_number_box_ = RectangleF(
		this->countdown_card_.x + padding.x, number_top,
		card_size.x - padding.x * 2.0f,
		this->countdown_card_.bottom() - padding.y - number_top);
	this->centre_stage_entry();
}

void Level::centre_stage_entry()
{
	const MatchPresentation& look = this->presentation_->match();
	const MenuStyle& menu = this->presentation_->menu();
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const float scale = this->opening_scale();
	const Vector2F size = look.stage_entry_size * scale;
	this->stage_entry_card_ = RectangleF(
		(resolution.x - size.x) / 2.0f, look.stage_entry_top * scale, size.x, size.y);
	const Vector2F padding = look.controls_padding * scale;
	const float shade = look.countdown_art.shadow_offset * scale;
	const RectangleF inside(this->stage_entry_card_.position() + padding,
		size - padding * 2.0f - Vector2F(shade));
	const RectangleF bounds = RectangleF::union_of(
		this->stage_entry_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->stage_entry_->text_bounds_at(menu.heading_shadow_offset, 1.0f));
	// Fit both dimensions, including a font's side bearings and the shadow.
	// This also handles authored multi-line names without clipping the paper.
	const float text_scale = std::min({ scale,
		inside.width / std::max(bounds.width, 1.0f),
		inside.height / std::max(bounds.height, 1.0f) });
	this->stage_entry_->set_scale(text_scale);
	this->stage_entry_->set_shadow_scale(text_scale);
	this->stage_entry_->set_shadow_offset(menu.heading_shadow_offset * text_scale);
	this->stage_entry_text_bounds_ = RectangleF(
		inside.center() - bounds.size() * text_scale / 2.0f, bounds.size() * text_scale);
	this->stage_entry_->set_position(this->stage_entry_text_bounds_.position() -
		bounds.position() * text_scale);
}

std::wstring_view Level::stage_entry_name() const
{
	return this->stage_entry_->text();
}

RectangleF Level::stage_entry_bounds() const
{
	return this->state_ == LevelState::start_countdown && !this->stage_entry_name().empty() ?
		this->stage_entry_card_ : RectangleF::ZERO;
}

RectangleF Level::stage_entry_text_bounds() const
{
	return this->stage_entry_bounds().width > 0.0f ?
		this->stage_entry_text_bounds_ : RectangleF::ZERO;
}

void Level::centre_controls_text()
{
	if (this->controls_reminders_.empty())
	{
		return;
	}
	// Fit a pair of equally sized cards from authored measurements on every resize.
	// All positions stay in fullscreen pixel space, independent of split-screen panes.
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const ControlsReminderLook& look = this->controls_look_;
	const float scale = std::min(resolution.x / look.reference_size.x, resolution.y / look.reference_size.y);
	const Vector2F margin = look.margin * scale;
	const Vector2F padding = look.padding * scale;
	const float gap = look.gap * scale;
	const float count = static_cast<float>(this->controls_reminders_.size());
	float content_width = 0.0f;
	float heading_height = 0.0f;
	float text_height = 0.0f;
	for (const ControlsReminder& reminder : this->controls_reminders_)
	{
		const RectangleF heading = RectangleF::union_of(
			reminder.heading->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.heading->text_bounds_at(look.heading_shadow, 1.0f));
		const RectangleF text = RectangleF::union_of(
			reminder.text->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.text->text_bounds_at(look.text_shadow, 1.0f));
		content_width = std::max({ content_width, heading.width, text.width });
		heading_height = std::max(heading_height, heading.height);
		text_height = std::max(text_height, text.height);
	}
	const float column_width = (resolution.x - margin.x * 2.0f - gap * (count - 1.0f)) / count;
	const float text_scale = std::min(scale,
		(column_width - padding.x * 2.0f) / content_width);
	const Vector2F card_size(content_width * text_scale + padding.x * 2.0f,
		(heading_height + text_height) * text_scale + padding.y * 3.0f);
	const float total_width = card_size.x * count + gap * (count - 1.0f);
	float x = (resolution.x - total_width) / 2.0f;
	for (ControlsReminder& reminder : this->controls_reminders_)
	{
		reminder.card = RectangleF(Vector2F(x,
			std::min(resolution.y * look.y,
				resolution.y - margin.y - card_size.y)), card_size);
		const RectangleF heading = RectangleF::union_of(
			reminder.heading->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.heading->text_bounds_at(look.heading_shadow, 1.0f));
		const RectangleF text = RectangleF::union_of(
			reminder.text->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.text->text_bounds_at(look.text_shadow, 1.0f));
		reminder.heading->set_scale(text_scale);
		reminder.heading->set_shadow_scale(text_scale);
		reminder.heading->set_shadow_offset(look.heading_shadow * text_scale);
		reminder.heading->set_position(reminder.card.position() + padding -
			heading.position() * text_scale);
		reminder.text->set_scale(text_scale);
		reminder.text->set_shadow_scale(text_scale);
		reminder.text->set_shadow_offset(look.text_shadow * text_scale);
		const Vector2F text_position = reminder.card.position() +
			Vector2F(padding.x, padding.y * 2.0f + heading_height * text_scale) -
			text.position() * text_scale;
		reminder.text->set_position(text_position);
		reminder.text_bounds = RectangleF::union_of(reminder.text->text_bounds_at(text_position, text_scale),
			reminder.text->text_bounds_at(text_position + look.text_shadow * text_scale, text_scale));
		x += card_size.x + gap;
	}
}

float Level::opening_scale() const
{
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const Vector2F reference = this->presentation_->match().reference_size;
	return std::min(resolution.x / reference.x, resolution.y / reference.y);
}

std::vector<RectangleF> Level::opening_cards() const
{
	std::vector<RectangleF> cards;
	if (this->state_ != LevelState::start_countdown)
	{
		return cards;
	}
	cards.push_back(this->countdown_card_);
	for (const ControlsReminder& reminder : this->controls_reminders_)
	{
		cards.push_back(reminder.card);
	}
	if (!this->stage_entry_name().empty()) { cards.push_back(this->stage_entry_card_); }
	return cards;
}

void Level::draw_resume_countdown(Renderer& renderer, float remaining) const
{
	if (!(remaining > 0.0f) || this->state_ != LevelState::active) { return; }
	DrawList list = renderer.view(renderer.view_count() - 1);
	list.set_viewport(this->viewport_manager_->fullscreen_viewport());
	list.set_camera(Camera::DEFAULT_CAMERA);
	list.set_filter(TextureFilter::point);
	this->countdown_art_->draw_card(list, this->countdown_card_, this->opening_scale());
	const float duration = this->presentation_->pause_menu().resume_countdown;
	const float beat_age = std::min(duration - remaining, std::ceil(remaining) - remaining);
	this->countdown_art_->draw(list, this->countdown_number_box_,
		std::to_wstring(static_cast<int>(std::ceil(remaining))), beat_age,
		this->preferences_->reduce_effects);
	list.set_filter(TextureFilter::linear);
	this->resume_heading_->draw(list);
	list.set_filter(TextureFilter::point);
}

void Level::draw_replay_banner(Renderer& renderer) const
{
	DrawList list = renderer.view(renderer.view_count() - 1);
	list.set_viewport(this->viewport_manager_->fullscreen_viewport());
	list.set_camera(Camera::DEFAULT_CAMERA);
	list.set_filter(TextureFilter::point);
	this->countdown_art_->draw_card(list, this->replay_card_, this->opening_scale());
	list.set_filter(TextureFilter::linear);
	this->replay_heading_->draw(list);
	list.set_filter(TextureFilter::point);
}

void Level::draw_countdown_text(DrawList& draw_list) const
{
	const bool getting_ready = this->state_ == LevelState::start_countdown;
	if (getting_ready)
	{
		const float scale = this->opening_scale();
		draw_list.set_filter(TextureFilter::point);
		this->countdown_art_->draw_card(draw_list, this->countdown_card_, scale);
		if (!this->stage_entry_name().empty())
		{
			this->countdown_art_->draw_card(draw_list, this->stage_entry_card_, scale);
		}
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{
			this->draw_opening_card(draw_list, reminder.card, scale);
		}
	}
	const float beat_age = this->start_timer_ > 0.0f ?
		std::min(this->tuning_->match().countdown - this->start_timer_,
			std::ceil(this->start_timer_) - this->start_timer_) : -this->start_timer_;
	draw_list.set_filter(TextureFilter::point);
	this->countdown_art_->draw(draw_list, this->countdown_number_box_,
		this->countdown_label_, beat_age, this->preferences_->reduce_effects);
	// Keep the shared controls text on its existing font treatment.
	draw_list.set_filter(TextureFilter::linear);

	// Clear the cards when movement begins. GO! keeps its existing one-second beat.
	if (getting_ready)
	{
		this->countdown_heading_->draw(draw_list);
		this->stage_entry_->draw(draw_list);
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{
			reminder.heading->draw(draw_list);
			reminder.text->draw(draw_list);
		}
	}

	draw_list.set_filter(TextureFilter::point);
}

void Level::draw_opening_card(DrawList& draw_list,
	const RectangleF& rectangle, float scale) const
{
	const MenuStyle& menu = this->presentation_->menu();
	const SpriteSheet* sheet = this->render_resources_->sprite_sheet(this->overlay_sheet_);
	sheet->draw(draw_list, this->overlay_fill_,
		RectangleF(rectangle.position() + menu.panel_shadow_offset * scale, rectangle.size()),
		menu.scrim_colour);
	sheet->draw(draw_list, this->overlay_fill_, rectangle, menu.panel_edge_colour);
	const float border = menu.rule_height * scale;
	sheet->draw(draw_list, this->overlay_fill_, RectangleF(
		rectangle.x + border, rectangle.y + border,
		rectangle.width - border * 2.0f, rectangle.height - border * 2.0f),
		menu.focus_panel_colour);
	sheet->draw(draw_list, this->overlay_fill_, RectangleF(
		rectangle.x + border, rectangle.y + border,
		rectangle.width - border * 2.0f, menu.accent_width * scale), menu.highlight);
}
