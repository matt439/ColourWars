#include "game/pch.h"
#include "game/session.h"
#include "game/presentation.h"

Session::Session(int best_of) : best_of_(best_of)
{
	if (best_of != 1 && best_of != 3 && best_of != 5)
	{
		throw std::invalid_argument("Session length must be 1, 3 or 5");
	}
}

void Session::finish_round(const LevelEndInfo& result)
{
	if (this->round_finished_ || this->complete())
	{
		return;
	}
	this->round_finished_ = true;
	this->round_winner_ = result.one_team_only() ? PlayerTeam::none : result.winning_team();
	if (this->round_winner_ == PlayerTeam::a) ++this->team_a_wins_;
	if (this->round_winner_ == PlayerTeam::b) ++this->team_b_wins_;
}

void Session::next_round()
{
	if (!this->round_finished_ || this->complete())
	{
		throw std::logic_error("No next round is available");
	}
	++this->round_;
	this->round_finished_ = false;
	this->round_winner_ = PlayerTeam::none;
}

bool Session::is_series() const { return this->best_of_ > 1; }
bool Session::complete() const
{
	return this->is_series() ?
		std::max(this->team_a_wins_, this->team_b_wins_) >= this->best_of_ / 2 + 1 :
		this->round_finished_;
}
int Session::round() const { return this->round_; }
PlayerTeam Session::round_winner() const { return this->round_winner_; }
int Session::team_a_wins() const { return this->team_a_wins_; }
int Session::team_b_wins() const { return this->team_b_wins_; }

std::wstring Session::score_string(const MenuResultsText& text) const
{
	return text.score_line(this->team_a_wins_, this->team_b_wins_, this->best_of_ / 2 + 1);
}

std::wstring Session::result_string(const MenuResultsText& text) const
{
	if (this->complete())
	{
		return this->team_a_wins_ > this->team_b_wins_ ?
			text.series_team_a_wins : text.series_team_b_wins;
	}
	return this->round_winner_ == PlayerTeam::none ?
		text.draw_no_win : text.series_continues;
}

std::wstring Session::continue_string(const MenuEndText& text) const
{
	if (!this->is_series()) return text.rematch;
	if (this->complete()) return text.play_series_again;
	return this->round_winner_ == PlayerTeam::none ? text.replay_draw : text.next_round;
}
