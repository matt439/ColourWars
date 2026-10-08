#pragma once

#include "game/debug/dev_tools.h"
#include "game/debug/debug_targets.h"
#include <memory>
#include <string>
#include <vector>

class GameData;
class GameTuning;
class MenuPage;
class Presentation;
class DebugDirectorImpl;

namespace labrador
{
	class Application;
	class Renderer;
}

// The developer debug service: console, launch arguments, F-key binds, the
// debug clock and queued match requests. The states call its hooks; it does
// nothing in Release.
//
// Its layout is the same in every configuration (one pointer). In Debug the
// pointer owns a DebugDirectorImpl; in Release it is always null and the hooks
// below are inline no-ops, so no registry, console or command text is built.
class DebugDirector
{
public:
	// Borrows the live tuning and presentation, which `reload` replaces and
	// `feedback` edits. Both outlive Application, as this does.
	DebugDirector(GameTuning* tuning, Presentation* presentation);
	~DebugDirector();

	DebugDirector(const DebugDirector&) = delete;
	DebugDirector& operator=(const DebugDirector&) = delete;

	// A director with no implementation, for a GameData that was never given one.
	static DebugDirector& inert();

	bool enabled() const { return this->impl_ != nullptr; }

	// Called once the application and game data are complete.
	void attach(labrador::Application* application, GameData* data);

	// The process's arguments after the program name. Debug runs every
	// "+statement"; Release prints one stderr line when any are present.
	void run_launch_arguments(const std::vector<std::wstring>& arguments);

	// GameLevel::init calls this before building its first level, and the
	// destructor calls detach_match. Requests left over from the menus or from
	// an ended match (players/weapon stored without `stage`) are dropped here,
	// with a log line, so they never rebuild a match started another way.
	void attach_match(DebugMatchTarget& match);
	void detach_match(const DebugMatchTarget& match);

	// GameLevel's one build path calls this with no Level alive: it applies
	// content `reload` validated and consumes a queued restart, so a restart
	// from the pause or end menu does not build twice.
	void before_level_build();

	// The top of GameLevel::update. Opens the console, runs the F-key binds
	// and applies queued requests (rebuild, leave). True: the match has done
	// its work for this step and should return.
	bool before_match_update(DebugMatchTarget& match);

	// Replaces GameLevel's single Level::update with the debug clock's
	// 0..4 fixed-dt updates.
	void update_match(DebugMatchTarget& match,
		const std::vector<PlayerInputData>& inputs, float dt);

	// Whether the debug clock is paused, so the match's voices stay paused.
	bool match_audio_paused() const;

	// The top of every MenuPage::update. Opens the console and starts a match
	// queued by the console or by launch arguments. True: the page should not
	// run its own logic this frame.
	bool before_page_update(MenuPage& page);

	// GameLevel::draw, last: a small strip naming non-default debug state
	// (timescale, pause, visual, feedback) while the console is closed.
	void draw_status(labrador::Renderer& renderer) const;

	DebugDirectorImpl* impl() const { return this->impl_.get(); }

private:
	DebugDirector();
	std::unique_ptr<DebugDirectorImpl> impl_;
};

#if !COLOURWARS_DEV_TOOLS
inline void DebugDirector::attach(labrador::Application*, GameData*) {}
inline void DebugDirector::attach_match(DebugMatchTarget&) {}
inline void DebugDirector::detach_match(const DebugMatchTarget&) {}
inline void DebugDirector::before_level_build() {}
inline bool DebugDirector::before_match_update(DebugMatchTarget&) { return false; }
inline void DebugDirector::update_match(DebugMatchTarget& match,
	const std::vector<PlayerInputData>& inputs, float dt)
{
	match.debug_update_level(inputs, dt);
}
inline bool DebugDirector::match_audio_paused() const { return false; }
inline bool DebugDirector::before_page_update(MenuPage&) { return false; }
inline void DebugDirector::draw_status(labrador::Renderer&) const {}
#endif
