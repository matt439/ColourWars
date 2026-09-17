#include "game/pch.h"
#include "game/session.h"

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
int Session::team_a_wins() const { return this->team_a_wins_; }
int Session::team_b_wins() const { return this->team_b_wins_; }

std::wstring Session::score_string() const
{
	return L"Wins: A " + std::to_wstring(this->team_a_wins_) + L" - B " +
		std::to_wstring(this->team_b_wins_) + L"  |  First to " +
		std::to_wstring(this->best_of_ / 2 + 1);
}

std::wstring Session::result_string() const
{
	if (this->complete())
	{
		return this->team_a_wins_ > this->team_b_wins_ ?
			L"Team A wins the series!" : L"Team B wins the series!";
	}
	return this->round_winner_ == PlayerTeam::none ?
		L"Draw - no win awarded" : L"Series continues";
}

std::wstring Session::continue_string() const
{
	if (!this->is_series()) return L"Restart";
	if (this->complete()) return L"Play Series Again";
	return this->round_winner_ == PlayerTeam::none ? L"Replay Draw" : L"Next Round";
}
