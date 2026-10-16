#pragma once

#include "game/save.h"

#include <functional>
#include <optional>

struct DisplaySettingsChoice
{
	labrador::ScreenResolution resolution;
	bool fullscreen;
	TeamColourPair palette;
};

enum class DisplayTrialResult
{
	reverted,
	kept,
	save_failed,
	apply_failed,
	rollback_failed
};

// Owns just a display transaction. The target changes this application's
// window; Save remains untouched until Keep, including on timeout/unwind.
class DisplaySettingsTrial
{
public:
	using Apply = std::function<void(labrador::ScreenResolution, bool)>;
	DisplaySettingsTrial(Save* save, DisplaySettingsChoice candidate,
		bool prior_actual_fullscreen, Apply apply, float timeout);
	~DisplaySettingsTrial();
	DisplaySettingsTrial(const DisplaySettingsTrial&) = delete;
	DisplaySettingsTrial& operator=(const DisplaySettingsTrial&) = delete;

	void begin();
	void advance(float seconds);
	void cancel() noexcept;
	void keep();
	std::optional<DisplayTrialResult> result() const;
	float remaining() const;

private:
	Save* save_;
	DisplaySettingsChoice candidate_;
	labrador::ScreenResolution prior_resolution_;
	bool prior_fullscreen_;
	Apply apply_;
	float remaining_;
	std::optional<DisplayTrialResult> result_;
	bool started_ = false;
	bool armed_ = false;
	bool applying_ = false;
	bool cancel_requested_ = false;
	bool apply_failed_ = false;
};
