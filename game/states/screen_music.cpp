#include "game/pch.h"
#include "game/states/screen_music.h"

ScreenMusic::ScreenMusic(const std::string& bank, const labrador::AudioResources* resources,
	const PlaySettings* preferences, const PresentationSound& music,
	const MusicMixPresentation& mix, bool loop) :
	SoundBankObject(bank, resources), preferences_(preferences), music_(music), mix_(mix)
{
	this->effect_ = this->resolve_effect(this->music_.name);
	this->play_effect(this->effect_, loop, 0.0f);
	this->envelope_.fade_to(1.0f, this->mix_.fade_in);
}

ScreenMusic::~ScreenMusic()
{
	this->stop_effect(this->effect_, true);
}

void ScreenMusic::apply_gain()
{
	this->set_effect_volume(this->effect_, this->music_.volume * this->envelope_.gain());
}

void ScreenMusic::update(float dt)
{
	if (dt < 0.0f || !std::isfinite(dt)) { return; }
	if (this->effect_state(this->effect_) != labrador::SoundState::playing) { return; }
	this->envelope_.advance(dt);
	this->apply_gain();
	if (this->leaving_ && this->finished()) { this->stop_effect(this->effect_, true); }
}

void ScreenMusic::suspend()
{
	this->pause_effect(this->effect_);
}

void ScreenMusic::resume()
{
	this->apply_gain();
	// A naturally finished one-shot and a completed exit stay finished.
	if (this->effect_state(this->effect_) == labrador::SoundState::paused)
	{
		this->resume_effect(this->effect_);
	}
}

void ScreenMusic::fade_out()
{
	this->leaving_ = true;
	this->envelope_.fade_to(0.0f, this->mix_.fade_out);
	if (this->finished()) { this->stop_effect(this->effect_, true); }
}

bool ScreenMusic::finished() const
{
	return this->effect_state(this->effect_) == labrador::SoundState::stopped ||
		(this->leaving_ && (this->envelope_.gain() == 0.0f ||
			this->music_.volume * this->preferences_->music_gain() == 0.0f));
}
