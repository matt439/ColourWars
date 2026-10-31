#include "game/pch.h"
#include "game/objects/level.h"
#include "game/objects/countdown_cards.h"
#include "game/objects/match_opening.h"

using namespace mattmath;
using namespace labrador;

void Level::build_opening_frame()
{
	// Admit the builder's pending objects before the first frame is drawn.
	this->scene_->end_tick();
	this->structures_.admit(*this->scene_, this->players_, this->tuning_->player());
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
	if (this->opening_ != nullptr)
	{
		this->opening_->set_picture(right > left && bottom > top ?
			RectangleF(left, top, right - left, bottom - top) : this->camera_bounds_,
			limit.x + limit.width / 2.0f);
	}

	// The countdown's cards and a controls reminder for each device in play.
	this->cards_->build(this->players_, this->paper_);

	this->refresh_layout();
}

std::wstring_view Level::gamepad_controls_text() const
{ return this->cards_->gamepad_controls_text(); }

RectangleF Level::gamepad_controls_text_bounds() const
{
	return this->state_ == LevelState::start_countdown ?
		this->cards_->gamepad_controls_text_bounds() : RectangleF::ZERO;
}

RectangleF Level::gamepad_controls_card_bounds() const
{
	return this->state_ == LevelState::start_countdown ?
		this->cards_->gamepad_controls_card_bounds() : RectangleF::ZERO;
}

std::wstring_view Level::keyboard_controls_text() const
{ return this->cards_->keyboard_controls_text(); }

RectangleF Level::keyboard_controls_text_bounds() const
{
	return this->state_ == LevelState::start_countdown ?
		this->cards_->keyboard_controls_text_bounds() : RectangleF::ZERO;
}

RectangleF Level::keyboard_controls_card_bounds() const
{
	return this->state_ == LevelState::start_countdown ?
		this->cards_->keyboard_controls_card_bounds() : RectangleF::ZERO;
}

std::wstring_view Level::stage_entry_name() const
{
	return this->cards_->stage_entry_name();
}

RectangleF Level::stage_entry_bounds() const
{
	return this->state_ == LevelState::start_countdown && !this->stage_entry_name().empty() ?
		this->cards_->stage_entry_card() : RectangleF::ZERO;
}

RectangleF Level::stage_entry_text_bounds() const
{
	return this->stage_entry_bounds().width > 0.0f ?
		this->cards_->stage_entry_text_bounds() : RectangleF::ZERO;
}

std::vector<RectangleF> Level::opening_cards() const
{
	if (this->state_ != LevelState::start_countdown)
	{
		return {};
	}
	return this->cards_->cards();
}

void Level::draw_resume_countdown(Renderer& renderer, float remaining) const
{
	if (!(remaining > 0.0f) || this->state_ != LevelState::active) { return; }
	DrawList list = renderer.view(renderer.view_count() - 1);
	list.set_viewport(this->viewport_manager_->fullscreen_viewport());
	list.set_camera(Camera::DEFAULT_CAMERA);
	this->cards_->draw_resume(list, this->paper_, remaining, this->preferences_->reduce_effects);
}

void Level::draw_replay_banner(Renderer& renderer) const
{
	DrawList list = renderer.view(renderer.view_count() - 1);
	list.set_viewport(this->viewport_manager_->fullscreen_viewport());
	list.set_camera(Camera::DEFAULT_CAMERA);
	this->cards_->draw_replay_banner(list, this->paper_);
}

void Level::draw_countdown_text(DrawList& draw_list) const
{
	const bool getting_ready = this->state_ == LevelState::start_countdown;
	const float beat_age = this->start_timer_ > 0.0f ?
		std::min(this->tuning_->match().countdown - this->start_timer_,
			std::ceil(this->start_timer_) - this->start_timer_) : -this->start_timer_;
	this->cards_->draw(draw_list, this->paper_, getting_ready, beat_age,
		this->preferences_->reduce_effects);
}
