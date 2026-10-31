#include "game/pch.h"
#include "game/objects/level.h"
#include "game/objects/countdown_cards.h"

using namespace mattmath;
using namespace labrador;

// Level's untimed solo practice (POL#19). PracticeLessons owns the lessons
// the stations carry and the paper card that shows the one the player is
// standing in; Level forwards to it, and only shows a card while the match is
// live. Whether a match is practice is the builder's choice; Level only sees
// the lessons.

bool Level::practice() const
{
	return !this->lessons_.empty();
}

void Level::start_without_countdown()
{
	if (this->state_ != LevelState::start_countdown || this->countdown_started_) { return; }
	this->start_timer_ = 0.0f;
	this->cards_->set_label(L"GO!");
}

int Level::active_lesson() const
{
	return this->lessons_.active();
}

int Level::lesson_count() const
{
	return this->lessons_.count();
}

const LevelLesson& Level::lesson(int index) const
{
	return this->lessons_.lesson(index);
}

RectangleF Level::lesson_card_bounds() const
{
	if (this->state_ != LevelState::active || this->lessons_.active() < 0)
	{
		return RectangleF::ZERO;
	}
	return this->lessons_.card(this->lessons_.active());
}
