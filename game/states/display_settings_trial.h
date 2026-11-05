#pragma once

#include "game/save/save.h"

#include <functional>
#include <optional>

struct DisplaySettingsChoice
{
	// The save's own defaults (save.h), so a choice built field by field
	// never carries an indeterminate value.
	labrador::ScreenResolution resolution = labrador::ScreenResolution::s_1920_1080;
	bool fullscreen = false;
	TeamColourPair palette = TeamColourPair::random;
	// Saved on Keep with the rest, like the palette; not part of the trial.
	bool vsync = true;
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
