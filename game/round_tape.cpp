#include "game/pch.h"
#include "game/round_tape.h"
#include "game/objects/level.h"

void RoundTape::start(unsigned int seed, const MatchSetup& settings)
{
	this->clear();
	this->seed_ = seed;
	this->settings_ = settings;
	this->started_ = true;
}

void RoundTape::clear()
{
	*this = RoundTape();
}

void RoundTape::spoil()
{
	this->spoiled_ = true;
	// Nothing reads a spoiled tape again; a long round's inputs need not wait
	// for the next build to be released.
	this->seats_ = {};
	this->dts_ = {};
	this->gaps_ = {};
}

void RoundTape::set_cheats(bool damage_immune, bool infinite_ammo)
{
	if (damage_immune == this->damage_immune_ && infinite_ammo == this->infinite_ammo_) { return; }
	if (!this->seats_.empty()) { this->spoil(); return; }
	this->damage_immune_ = damage_immune;
	this->infinite_ammo_ = infinite_ammo;
}

bool RoundTape::playable() const
{
	return this->started_ && !this->spoiled_ && !this->seats_.empty();
}

int RoundTape::steps() const
{
	return static_cast<int>(this->seats_.size());
}

const std::vector<PlayerInputData>& RoundTape::seats(int step) const
{
	return this->seats_.at(static_cast<std::size_t>(step));
}

float RoundTape::dt(int step) const
{
	return this->dts_.at(static_cast<std::size_t>(step));
}

int RoundTape::step_before_end(float seconds) const
{
	// A millisecond's tolerance, so eight seconds of 1/60 s float steps is
	// 480 steps however the sum rounds.
	const double wanted = static_cast<double>(seconds) - 0.001;
	double total = 0.0;
	int step = this->steps();
	while (step > 0 && total < wanted)
	{
		--step;
		total += static_cast<double>(this->dts_[static_cast<std::size_t>(step)]);
	}
	return step;
}

void RoundTape::record_step(const std::vector<PlayerInputData>& seats, float dt)
{
	if (!this->started_ || this->spoiled_) { return; }
	this->seats_.push_back(seats);
	this->dts_.push_back(dt);
}

void RoundTape::record_suspend()
{
	if (!this->started_ || this->spoiled_) { return; }
	this->gaps_.push_back({ this->steps(), false, {} });
}

void RoundTape::record_resume(const std::vector<PlayerInputData>& seats)
{
	if (!this->started_ || this->spoiled_) { return; }
	this->gaps_.push_back({ this->steps(), true, seats });
}

void RoundTape::apply_gaps(Level& level, int step) const
{
	for (const Gap& gap : this->gaps_)
	{
		if (gap.step != step) { continue; }
		if (gap.resume)
		{
			level.prepare_resume(gap.seats);
		}
		else
		{
			// Its jump buffers only: the replay's voices are not paused.
			level.interrupt_players();
		}
	}
}

void RoundTapeRecorder::prepare(SeatInputFilter* next, float dt)
{
	this->next_ = next;
	this->dt_ = dt;
}

void RoundTapeRecorder::filter_seats(std::vector<PlayerInputData>& seats, const Level& level)
{
	if (this->next_ != nullptr) { this->next_->filter_seats(seats, level); }
	this->tape_->record_step(seats, this->dt_);
}

bool RoundTapeRecorder::replaces_bot_inputs() const
{
	return this->next_ != nullptr && this->next_->replaces_bot_inputs();
}

void RoundTapePlayer::filter_seats(std::vector<PlayerInputData>& seats, const Level& /*level*/)
{
	if (this->finished()) { return; }
	const std::vector<PlayerInputData>& recorded = this->tape_->seats(this->step_);
	if (recorded.size() == seats.size()) { seats = recorded; }
	++this->step_;
}

bool RoundTapePlayer::finished() const
{
	return this->step_ >= this->tape_->steps();
}
