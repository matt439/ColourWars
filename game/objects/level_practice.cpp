#include "game/pch.h"
#include "game/objects/level.h"
#include "game/drawable_text.h"
#include "engine/math/intersects.h"

using namespace mattmath;
using namespace labrador;

// Level's untimed solo practice (POL#19): the lessons the stations carry and
// the paper card that shows the one the player is standing in. Whether a
// match is practice is the builder's choice; Level only sees the lessons.

bool Level::practice() const
{
	return !this->lessons_.empty();
}

void Level::start_without_countdown()
{
	if (this->state_ != LevelState::start_countdown || this->countdown_started_) { return; }
	this->start_timer_ = 0.0f;
	this->set_countdown_text(L"GO!");
}

int Level::active_lesson() const
{
	return this->active_lesson_;
}

int Level::lesson_count() const
{
	return static_cast<int>(this->lessons_.size());
}

const LevelLesson& Level::lesson(int index) const
{
	return this->lessons_.at(static_cast<size_t>(index));
}

RectangleF Level::lesson_card_bounds() const
{
	if (this->state_ != LevelState::active || this->active_lesson_ < 0)
	{
		return RectangleF::ZERO;
	}
	return this->lesson_cards_.at(static_cast<size_t>(this->active_lesson_)).card;
}

void Level::build_lesson_cards(RenderResources* render_resources)
{
	// The card is the controls reminder's: the menu's detail heading over the
	// match's controls text, so a lesson reads like the reminder it extends.
	const MenuStyle& menu = this->presentation_->menu();
	const MatchTuning& match = this->tuning_->match();
	for (const LevelLesson& lesson : this->lessons_)
	{
		const std::string where = "the lesson at station '" + lesson.station + "'";
		LessonCard card;
		card.title = std::make_unique<colourwars::TextDropShadow>(
			drawable_text::widen_drawable(lesson.title, where + ": title"),
			menu.detail_font, Vector2F::ZERO, render_resources,
			menu.highlight, menu.shadow_colour, menu.detail_shadow_offset);
		card.text = std::make_unique<colourwars::TextDropShadow>(
			drawable_text::widen_drawable(lesson.text, where + ": text"),
			match.controls_font, Vector2F::ZERO, render_resources,
			match.controls_colour, match.controls_shadow_colour, match.controls_shadow_offset);
		this->lesson_cards_.push_back(std::move(card));
	}
}

void Level::centre_lesson_cards()
{
	// Bottom centre of the window, clear of the HUD along the top. Every
	// card is laid out on resize, so choosing one never measures text.
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	if (this->lesson_cards_.empty() || !(resolution.x > 0.0f) || !(resolution.y > 0.0f))
	{
		return;
	}
	const MatchPresentation& look = this->presentation_->match();
	const Vector2F heading_shadow = this->presentation_->menu().detail_shadow_offset;
	const Vector2F text_shadow = this->tuning_->match().controls_shadow_offset;
	const float scale = this->opening_scale();
	const Vector2F margin = look.controls_margin * scale;
	const Vector2F padding = look.controls_padding * scale;
	for (LessonCard& lesson : this->lesson_cards_)
	{
		const RectangleF title = RectangleF::union_of(
			lesson.title->text_bounds_at(Vector2F::ZERO, 1.0f),
			lesson.title->text_bounds_at(heading_shadow, 1.0f));
		const RectangleF text = RectangleF::union_of(
			lesson.text->text_bounds_at(Vector2F::ZERO, 1.0f),
			lesson.text->text_bounds_at(text_shadow, 1.0f));
		const float content_width = std::max(title.width, text.width);
		const float room = resolution.x - margin.x * 2.0f - padding.x * 2.0f;
		const float text_scale = content_width > 0.0f && room > 0.0f ?
			std::min(scale, room / content_width) : scale;
		const Vector2F size(content_width * text_scale + padding.x * 2.0f,
			(title.height + text.height) * text_scale + padding.y * 3.0f);
		lesson.card = RectangleF(Vector2F((resolution.x - size.x) / 2.0f,
			resolution.y - margin.y - size.y), size);
		lesson.title->set_scale(text_scale);
		lesson.title->set_shadow_scale(text_scale);
		lesson.title->set_shadow_offset(heading_shadow * text_scale);
		lesson.title->set_position(lesson.card.position() + padding -
			title.position() * text_scale);
		lesson.text->set_scale(text_scale);
		lesson.text->set_shadow_scale(text_scale);
		lesson.text->set_shadow_offset(text_shadow * text_scale);
		lesson.text->set_position(lesson.card.position() +
			Vector2F(padding.x, padding.y * 2.0f + title.height * text_scale) -
			text.position() * text_scale);
	}
}

void Level::update_active_lesson()
{
	// Practice is solo, so the one human view is the one being taught.
	this->active_lesson_ = -1;
	if (this->lessons_.empty()) { return; }
	const Player& player = *this->players_.at(
		static_cast<size_t>(this->view_participants_.front()));
	if (player.state() != PlayerState::alive) { return; }
	const Vector2F centre = player.center();
	for (size_t index = 0; index < this->lessons_.size(); ++index)
	{
		if (rectangle_point_intersect(this->lessons_[index].area, centre))
		{
			this->active_lesson_ = static_cast<int>(index);
			return;
		}
	}
}

void Level::draw_lesson_card(DrawList& draw_list) const
{
	const LessonCard& lesson = this->lesson_cards_.at(static_cast<size_t>(this->active_lesson_));
	draw_list.set_filter(TextureFilter::point);
	this->draw_opening_card(draw_list, lesson.card, this->opening_scale());
	draw_list.set_filter(TextureFilter::linear);
	lesson.title->draw(draw_list);
	lesson.text->draw(draw_list);
	draw_list.set_filter(TextureFilter::point);
}
