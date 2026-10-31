#include "game/pch.h"
#include "game/objects/level.h"
#include "game/objects/match_opening.h"

using namespace mattmath;
using namespace labrador;

// What Level keeps of the opening and the reveal (HYG#40): the state the
// story reads, the skip request and the forwarders. MatchOpening
// (match_opening.cpp) owns the monitor, the person, the beats and the
// figures' acting; Level builds it in level.cpp, adds its views in
// refresh_views and draws it in level_draw.cpp.

OpeningClock Level::opening_clock() const
{
	OpeningClock clock;
	clock.state = this->state_;
	clock.intro_countdown = this->intro_countdown_;
	clock.start_timer = this->start_timer_;
	clock.zoom_ratio = this->zoom_out_camera_ratio();
	return clock;
}

bool Level::intro_showing() const
{
	return MatchOpening::intro_showing(this->opening_clock());
}

bool Level::intro_playing() const
{
	return this->intro_showing();
}

bool Level::skip_requested() const
{
	// Anyone in the match: SeatCommandBuilder reads each participant's slot.
	return this->intro_skip_requested_;
}

void Level::skip_intro()
{
	this->opening_->skip(this->players_);
	// Straight to the split-screen countdown, from its start if it had not begun.
	this->state_ = LevelState::start_countdown;
	this->intro_countdown_ = false;
}

bool Level::revealed() const
{
	return this->opening_ != nullptr;
}

bool Level::reveal_showing() const
{
	return this->opening_ != nullptr && MatchOpening::reveal_showing(this->opening_clock());
}
