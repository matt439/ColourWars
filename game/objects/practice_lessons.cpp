#include "game/pch.h"
#include "game/objects/practice_lessons.h"
#include "game/objects/paper_card.h"
#include "game/objects/player.h"
#include "game/presentation.h"
#include "game/tuning.h"
#include "game/drawable_text.h"
#include "engine/math/intersects.h"
#include "engine/render/renderer.h"
#include <algorithm>
#include <string>

using namespace mattmath;
using namespace labrador;

PracticeLessons::PracticeLessons(std::vector<LevelLesson> lessons, const GameTuning* tuning,
	const Presentation* presentation, RenderResources* render_resources) :
	tuning_(tuning),
	presentation_(presentation),
	lessons_(std::move(lessons))
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
		this->cards_.push_back(std::move(card));
	}
}

const LevelLesson& PracticeLessons::lesson(int index) const
{
	return this->lessons_.at(static_cast<size_t>(index));
}

const RectangleF& PracticeLessons::card(int index) const
{
	return this->cards_.at(static_cast<size_t>(index)).card;
}

void PracticeLessons::centre(const PaperCard& paper)
{
	const Vector2F resolution = paper.resolution();
	if (this->cards_.empty() || !(resolution.x > 0.0f) || !(resolution.y > 0.0f))
	{
		return;
	}
	const MatchPresentation& look = this->presentation_->match();
	const Vector2F heading_shadow = this->presentation_->menu().detail_shadow_offset;
	const Vector2F text_shadow = this->tuning_->match().controls_shadow_offset;
	const float scale = paper.scale();
	const Vector2F margin = look.controls_margin * scale;
	const Vector2F padding = look.controls_padding * scale;
	for (LessonCard& lesson : this->cards_)
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

void PracticeLessons::update(const Player& focus)
{
	this->active_ = -1;
	if (focus.state() != PlayerState::alive) { return; }
	const Vector2F centre = focus.center();
	for (size_t index = 0; index < this->lessons_.size(); ++index)
	{
		if (rectangle_point_intersect(this->lessons_[index].area, centre))
		{
			this->active_ = static_cast<int>(index);
			return;
		}
	}
}

void PracticeLessons::draw(DrawList& draw_list, const PaperCard& paper) const
{
	const LessonCard& lesson = this->cards_.at(static_cast<size_t>(this->active_));
	draw_list.set_filter(TextureFilter::point);
	paper.draw(draw_list, lesson.card, paper.scale());
	draw_list.set_filter(TextureFilter::linear);
	lesson.title->draw(draw_list);
	lesson.text->draw(draw_list);
	draw_list.set_filter(TextureFilter::point);
}
