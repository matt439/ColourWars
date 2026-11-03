#include "game/pch.h"
#include "game/objects/level.h"
#include "game/objects/countdown_cards.h"

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

std::optional<Vector2F> Level::tool_change_place() const
{
	const Player& player = this->player(this->participant_for_view(0));
	if (player.state() != PlayerState::alive) { return std::nullopt; }
	return player.rectangle().position();
}

void Level::enter_after_tool_change(const std::optional<Vector2F>& place)
{
	this->start_without_countdown();
	if (place) { this->place_player(this->participant_for_view(0), *place); }
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
		return RectangleF::zero;
	}
	return this->lessons_.card(this->lessons_.active());
}
