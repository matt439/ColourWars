#include "game/pch.h"
#include "game/debug/debug_background.h"

#if COLOURWARS_DEV_TOOLS
#include <CommCtrl.h>
#include <utility>

bool background_window_allows(const DebugBackgroundState& state)
{
	return state.visible && !state.minimized && !state.power_suspended;
}

void DebugBackgroundInput::filter_seats(std::vector<PlayerInputData>& seats, const Level& level)
{
	for (PlayerInputData& input : seats)
	{
		const bool connected = input.connected;
		input = PlayerInputData();
		input.connected = connected;
	}
	if (this->next_ != nullptr) { this->next_->filter_seats(seats, level); }
}

class DebugBackgroundWindow::Impl
{
public:
	~Impl()
	{
		if (this->window != nullptr)
		{
			RemoveWindowSubclass(this->window, &Impl::observe, reinterpret_cast<UINT_PTR>(this));
		}
	}

	HWND window = nullptr;
	DebugBackgroundState facts;
	std::function<void()> changed;

	static LRESULT CALLBACK observe(HWND window, UINT message, WPARAM wparam,
		LPARAM lparam, UINT_PTR, DWORD_PTR reference)
	{
		Impl& self = *reinterpret_cast<Impl*>(reference);
		bool changed = false;
		if (message == WM_NCDESTROY)
		{
			// Application dies before the director. No later destructor or
			// query may use the window, or the Application that used to own it.
			self.window = nullptr;
			self.changed = {};
		}
		else if (message == WM_SIZE)
		{
			self.facts.minimized = wparam == SIZE_MINIMIZED;
			changed = true;
		}
		else if (message == WM_SHOWWINDOW)
		{
			changed = true;
		}
		else if (message == WM_POWERBROADCAST)
		{
			if (wparam == PBT_APMSUSPEND)
			{
				self.facts.power_suspended = true;
				changed = true;
			}
			else if (wparam == PBT_APMRESUMEAUTOMATIC || wparam == PBT_APMRESUMESUSPEND)
			{
				self.facts.power_suspended = false;
				changed = true;
			}
		}
		// Publish the hard gate before the engine handles its deactivation.
		if (changed && self.changed) { self.changed(); }
		return DefSubclassProc(window, message, wparam, lparam);
	}
};

DebugBackgroundWindow::DebugBackgroundWindow() : impl_(std::make_unique<Impl>()) {}
DebugBackgroundWindow::~DebugBackgroundWindow() = default;

bool DebugBackgroundWindow::attach(void* window, std::function<void()> changed)
{
	this->impl_ = std::make_unique<Impl>();
	const HWND handle = static_cast<HWND>(window);
	if (handle == nullptr || !IsWindow(handle)) { return false; }
	this->impl_->window = handle;
	this->impl_->facts.minimized = IsIconic(handle) != FALSE;
	this->impl_->changed = std::move(changed);
	if (!SetWindowSubclass(handle, &Impl::observe, reinterpret_cast<UINT_PTR>(this->impl_.get()),
		reinterpret_cast<DWORD_PTR>(this->impl_.get())))
	{
		this->impl_->window = nullptr;
		this->impl_->changed = {};
		return false;
	}
	return true;
}

DebugBackgroundState DebugBackgroundWindow::state() const
{
	if (this->impl_->window == nullptr) { return {}; }
	DebugBackgroundState result = this->impl_->facts;
	result.visible = IsWindowVisible(this->impl_->window) != FALSE;
	return result;
}
#endif
