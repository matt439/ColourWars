#pragma once

#include "game/objects/player_input_data.h"
#include "game/states/menu_level_settings.h"
#include <string>
#include <vector>

// What the debug director may ask of a running match. GameLevel implements
// it privately; the director only reaches it through its own hooks, which
// GameLevel calls at the top of its update, when no Level::update is on the
// call stack.
class DebugMatchTarget
{
public:
	virtual ~DebugMatchTarget() = default;

	virtual const MenuLevelSettings& debug_settings() const = 0;

	// Builds a fresh level from `plan` through the match's one build path,
	// which calls DebugDirector::before_level_build between destroying the old
	// level and building the new one. reset_session starts a new session.
	virtual void debug_rebuild(const MenuLevelSettings& plan, bool reset_session) = 0;

	// Leaves the match for the main menu's home page.
	virtual void debug_leave() = 0;

	virtual void debug_update_level(const std::vector<PlayerInputData>& inputs,
		float dt) = 0;
	virtual bool debug_level_finished() const = 0;

	// Pauses the level's voices while the debug clock is paused, the match is
	// covered or the window is inactive; resumes them otherwise.
	virtual void debug_sync_audio() = 0;

	// One line per view: device, position, move state.
	virtual std::vector<std::string> debug_status() const = 0;

protected:
	DebugMatchTarget() = default;
	DebugMatchTarget(const DebugMatchTarget&) = default;
	DebugMatchTarget& operator=(const DebugMatchTarget&) = default;
};
