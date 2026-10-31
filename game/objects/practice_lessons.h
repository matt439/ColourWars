#pragma once

#include "game/objects/level_station.h"
#include "game/carried/text_drop_shadow.h"
#include "engine/math/rectanglef.h"
#include <memory>
#include <vector>

namespace labrador
{
	class DrawList;
	class RenderResources;
}
class GameTuning;
class Presentation;
class Player;
class PaperCard;

// Untimed solo practice's lessons (POL#19, HYG#40): the lessons the stations
// carry, one prepared card per lesson so a draw only chooses among them, and
// which lesson the player is standing in. Empty, it is a level that is not
// practice. The look and the controls text are read through the borrowed
// tuning and presentation each layout, never copied.
class PracticeLessons
{
public:
	PracticeLessons() = default;
	// Builds a card per lesson; a title or text that cannot be drawn throws
	// from drawable_text::widen_drawable, naming the lesson's station.
	PracticeLessons(std::vector<LevelLesson> lessons, const GameTuning* tuning,
		const Presentation* presentation, labrador::RenderResources* render_resources);

	bool empty() const { return this->lessons_.empty(); }
	int count() const { return static_cast<int>(this->lessons_.size()); }
	const LevelLesson& lesson(int index) const;
	// The lesson whose area holds the focus player's centre, as of the last
	// update, else -1.
	int active() const { return this->active_; }
	// Lesson `index`'s card in window pixels, as of the last centre.
	const mattmath::RectangleF& card(int index) const;

	// Bottom centre of the window, clear of the HUD along the top. Every
	// card is laid out on resize, so choosing one never measures text.
	void centre(const PaperCard& paper);
	// Practice is solo, so the one human is the one being taught.
	void update(const Player& focus);
	// The active lesson's card; there must be one.
	void draw(labrador::DrawList& draw_list, const PaperCard& paper) const;

private:
	struct LessonCard
	{
		std::unique_ptr<colourwars::TextDropShadow> title;
		std::unique_ptr<colourwars::TextDropShadow> text;
		mattmath::RectangleF card;
	};
	const GameTuning* tuning_ = nullptr;
	const Presentation* presentation_ = nullptr;
	std::vector<LevelLesson> lessons_;
	std::vector<LessonCard> cards_;
	int active_ = -1;
};
