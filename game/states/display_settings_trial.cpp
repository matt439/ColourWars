#include "game/pch.h"
#include "game/states/display_settings_trial.h"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <utility>

DisplaySettingsTrial::DisplaySettingsTrial(Save* save, DisplaySettingsChoice candidate,
	bool prior_actual_fullscreen, Apply apply, float timeout) :
	save_(save), candidate_(candidate),
	prior_resolution_(save != nullptr ? save->resolution() : labrador::ScreenResolution::s_1920_1080),
	prior_fullscreen_(prior_actual_fullscreen), apply_(std::move(apply)), remaining_(timeout)
{
	if (save == nullptr || !this->apply_ || !std::isfinite(timeout) || timeout < 1.0f || timeout > 60.0f ||
		static_cast<int>(candidate.resolution) < 0 || candidate.resolution > labrador::ScreenResolution::max ||
		candidate.palette < TeamColourPair::random || candidate.palette >= TeamColourPair::count)
	{
		throw std::invalid_argument("Invalid display settings trial.");
	}
}

DisplaySettingsTrial::~DisplaySettingsTrial()
{
	this->cancel();
}

void DisplaySettingsTrial::begin()
{
	if (this->started_ || this->result_) { return; }
	this->started_ = true;
	this->armed_ = true;
	this->applying_ = true;
	try
	{
		this->apply_(this->candidate_.resolution, this->candidate_.fullscreen);
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "display trial apply failed: %s\n", error.what());
		this->apply_failed_ = true;
	}
	catch (...)
	{
		std::fprintf(stderr, "display trial apply failed: unknown error\n");
		this->apply_failed_ = true;
	}
	this->applying_ = false;
	if (this->cancel_requested_ || this->apply_failed_) { this->cancel(); }
}

void DisplaySettingsTrial::advance(float seconds)
{
	if (!this->armed_ || this->applying_ || !std::isfinite(seconds) || seconds <= 0.0f) { return; }
	this->remaining_ = std::max(0.0f, this->remaining_ - seconds);
	if (this->remaining_ == 0.0f) { this->cancel(); }
}

void DisplaySettingsTrial::cancel() noexcept
{
	if (this->applying_)
	{
		this->cancel_requested_ = true;
		return;
	}
	if (!this->armed_)
	{
		if (!this->started_ && !this->result_) { this->result_ = DisplayTrialResult::reverted; }
		return;
	}
	// Disarm before calling the target: a size/focus message can reenter us.
	this->armed_ = false;
	this->result_ = this->apply_failed_ ? DisplayTrialResult::apply_failed : DisplayTrialResult::reverted;
	try
	{
		this->apply_(this->prior_resolution_, this->prior_fullscreen_);
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "display trial rollback failed: %s\n", error.what());
		this->result_ = DisplayTrialResult::rollback_failed;
	}
	catch (...)
	{
		std::fprintf(stderr, "display trial rollback failed: unknown error\n");
		this->result_ = DisplayTrialResult::rollback_failed;
	}
}

void DisplaySettingsTrial::keep()
{
	if (!this->armed_ || this->applying_) { return; }
	this->armed_ = false;
	this->result_ = DisplayTrialResult::save_failed;
	try
	{
		if (this->save_->set_display_settings_and_save(this->candidate_.resolution,
			this->candidate_.fullscreen, this->candidate_.palette))
		{
			this->result_ = DisplayTrialResult::kept;
		}
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "display trial save failed: %s\n", error.what());
	}
	catch (...)
	{
		std::fprintf(stderr, "display trial save failed: unknown error\n");
	}
}

std::optional<DisplayTrialResult> DisplaySettingsTrial::result() const
{
	return this->result_;
}

float DisplaySettingsTrial::remaining() const
{
	return this->remaining_;
}
