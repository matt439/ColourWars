#include "game/pch.h"
#include "game/states/game_states.h"

using namespace labrador;

// GameLevel's DebugMatchTarget adapters, which serve only the developer
// tools. The match step they wrap is GameLevel::step_match (game_states.cpp).

const MatchSetup& GameLevel::debug_settings() const
{
	return this->settings_;
}

void GameLevel::debug_rebuild(const MatchSetup& plan, bool reset_session)
{
	this->start_ = MatchStart::developer;
	this->settings_ = plan;
	if (reset_session)
	{
		this->session_ = Session(plan.player_count() == 1 ? 1 : plan.best_of());
	}
	this->build_and_enter_level();
}

void GameLevel::debug_adopt(const MatchSetup& plan, bool reset_session)
{
	this->start_ = MatchStart::developer;
	this->settings_ = plan;
	if (reset_session)
	{
		this->session_ = Session(plan.player_count() == 1 ? 1 : plan.best_of());
	}
}

void GameLevel::debug_leave()
{
	this->leave_match(EndMenuAction::exit);
}

const Level* GameLevel::debug_level() const
{
	return this->level_.get();
}

bool GameLevel::debug_level_finished() const
{
	return this->level_->state() == LevelState::finished;
}

void GameLevel::debug_apply_gap(const RoundTape::Gap& gap)
{
	RoundTape::apply_gap(*this->level_, gap);
}

void GameLevel::debug_sync_audio()
{
	this->data()->debug().sync_background_audio(this->context()->active());
	this->sync_rumble();
	if (this->results_audio_)
	{
		if (this->context()->active() && !this->data()->debug().match_audio_paused())
		{
			this->results_audio_->resume();
		}
		else { this->results_audio_->suspend(); }
	}
	if (this->audio_may_play())
	{
		this->level_->resume();
	}
	else if (this->covered_ || (!this->context()->active() &&
		!this->data()->debug().background_updates_allowed()))
	{
		this->suspend_level();
		this->data()->debug().match_suspended(*this);
	}
	else
	{
		// Only the debug clock is paused: no menu or focus loss, so the players
		// keep their jump state and a paused or stepped match (a replay among
		// them) plays out as it would have run.
		this->level_->pause_audio();
	}
}

void GameLevel::debug_background_changed()
{
	if (this->level_ == nullptr || this->context()->active()) { return; }
	this->data()->debug().sync_background_audio(false);
	this->stop_rumble();
	if (this->results_audio_) { this->results_audio_->suspend(); }
	if (this->resume_audio_) { this->resume_audio_->stop(); }
	if (!this->covered_ && !this->pause_menu_open_ &&
		this->data()->debug().background_updates_allowed())
	{
		// A previous ordinary focus interruption is already recorded; enabling
		// the policy releases that hold, never an existing resume countdown.
		this->background_continuing_ = true;
		this->focus_interrupted_ = false;
		this->level_->pause_audio();
	}
	else if (this->background_continuing_)
	{
		this->background_continuing_ = false;
		this->focus_interrupted_ = this->level_->state() == LevelState::active;
		this->suspend_level();
		this->data()->debug().match_suspended(*this);
	}
}

std::vector<std::string> GameLevel::debug_status() const
{
	std::vector<std::string> lines;
	for (int view = 0; view < this->level_->player_count(); ++view)
	{
		const Player& player = this->level_->player(view);
		const mattmath::Vector2F centre = player.center();
		char position[64] = {};
		std::snprintf(position, sizeof(position), "at (%.0f, %.0f) ",
			static_cast<double>(centre.x), static_cast<double>(centre.y));
		lines.push_back(position + player.player_move_state_string() +
			(player.state() == PlayerState::dead ? " dead" : ""));
	}
	return lines;
}

void GameLevel::debug_set_overlays(const LevelOverlays& overlays)
{
	this->level_->set_overlays(overlays);
}

void GameLevel::debug_set_player_text(bool showing)
{
	this->level_->set_players_showing_debug(showing);
}

void GameLevel::debug_set_cheats(bool damage_immune, bool infinite_ammo)
{
	this->tape_.set_cheats(damage_immune, infinite_ammo);
	this->level_->set_player_cheats(damage_immune, infinite_ammo);
}

std::vector<LevelStation> GameLevel::debug_stations() const
{
	return this->data()->level_infos()->get(this->settings_.stage())->stations();
}

// A player moved between steps is a round the tape cannot build again.
bool GameLevel::debug_place(int seat, const mattmath::Vector2F& top_left)
{
	this->tape_.spoil();
	return this->level_->place_player(seat, top_left);
}

bool GameLevel::debug_respawn(int seat)
{
	this->tape_.spoil();
	return this->level_->respawn_player(seat);
}

bool GameLevel::debug_kill(int seat)
{
	this->tape_.spoil();
	return this->level_->kill_player(seat);
}

MatchPaint GameLevel::debug_paint() const
{
	const LevelEndInfo info = this->level_->level_end_info();
	MatchPaint paint;
	paint.team_a = info.team_a_area;
	paint.team_b = info.team_b_area;
	paint.total = info.total_area;
	// Convert roster positions to the same owners used by their projectiles.
	for (const SeatSetup& player : this->settings_.player_settings())
	{
		const int owner = paint_owners::index(player.num, static_cast<int>(paint.seats.size()));
		paint.seats.push_back(owner >= 0 ? info.player_areas.at(static_cast<size_t>(owner)) : 0.0f);
	}
	return paint;
}

bool GameLevel::debug_countdown_over() const
{
	return this->level_->match_started();
}

bool GameLevel::debug_perf_ready() const
{
	// A typed capture closes its developer console before sampling. The
	// director checks focus/clock/coverage; this answers the gameplay gates.
	return this->level_->state() == LevelState::active && !this->pause_menu_open_ &&
		!this->resume_countdown_.active();
}

void GameLevel::debug_set_seat_labels(const std::vector<std::wstring>& labels)
{
	if (this->level_ != nullptr)
	{
		this->level_->set_seat_labels(labels);
	}
}

std::vector<mattmath::RectangleF> GameLevel::debug_screen_obstacles() const
{
	return this->level_ != nullptr ? this->level_->opening_cards() :
		std::vector<mattmath::RectangleF>();
}
