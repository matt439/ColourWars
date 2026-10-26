#pragma once

#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/objects/seat_input_filter.h"
#include <functional>
#include <memory>

// Ordinary deactivation may run developer scripts; hiding, minimising and
// power suspension never do. These facts do not change StateContext's focus.
struct DebugBackgroundState
{
	bool visible = false;
	bool minimized = false;
	bool power_suspended = false;
};

bool background_window_allows(const DebugBackgroundState& state);

// Level resolves device/camera mouse aim before this boundary. Clear that
// final physical baseline first, then let the usual bots/record/play filter
// supply or observe the effective input. Device presence remains unchanged.
class DebugBackgroundInput final : public SeatInputFilter
{
public:
	void set_next(SeatInputFilter* next) { this->next_ = next; }
	void filter_seats(std::vector<PlayerInputData>& seats, const Level& level) override;

private:
	SeatInputFilter* next_ = nullptr;
};

// Observes only our existing window, forwarding every message unchanged.
// Owns no Application: the director is destroyed after that borrowed object.
class DebugBackgroundWindow
{
public:
	DebugBackgroundWindow();
	~DebugBackgroundWindow();
	DebugBackgroundWindow(const DebugBackgroundWindow&) = delete;
	DebugBackgroundWindow& operator=(const DebugBackgroundWindow&) = delete;
	bool attach(void* window, std::function<void()> changed);
	DebugBackgroundState state() const;

private:
	class Impl;
	std::unique_ptr<Impl> impl_;
};
#endif
