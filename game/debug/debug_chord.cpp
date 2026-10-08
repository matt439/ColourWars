#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_chord.h"

namespace
{
	// Thirty frames of 1/60 s sum to just under 0.5f in single precision.
	constexpr float hold_tolerance = 1e-4f;
}

bool DebugChord::update(const Pads& pads, float dt)
{
	bool fire = false;
	for (std::size_t slot = 0; slot < this->sessions_.size(); ++slot)
	{
		Session& session = this->sessions_[slot];
		const Pad& pad = pads[slot];
		session.was_active = session.active;

		const bool both = pad.connected && pad.back && pad.start;
		const bool neither = !pad.connected || (!pad.back && !pad.start);
		if (!session.active && both)
		{
			session = Session{ true, false, false, 0.0f };
		}
		else if (session.active && neither)
		{
			session.active = false;
			session.fired = false;
			session.held = 0.0f;
			continue;
		}

		if (session.active && both)
		{
			session.held += dt;
			if (!session.fired && session.held >= hold_seconds - hold_tolerance)
			{
				session.fired = true;
				fire = true;
			}
		}
	}
	return fire;
}

bool DebugChord::suppressing(int slot) const
{
	if (slot < 0 || slot >= static_cast<int>(this->sessions_.size()))
	{
		return false;
	}
	const Session& session = this->sessions_[static_cast<std::size_t>(slot)];
	return session.active || session.was_active;
}

bool DebugChord::any_suppressing() const
{
	for (int slot = 0; slot < static_cast<int>(this->sessions_.size()); ++slot)
	{
		if (this->suppressing(slot))
		{
			return true;
		}
	}
	return false;
}

void DebugChord::reset()
{
	this->sessions_ = {};
}
#endif
