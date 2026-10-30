#pragma once

struct UserStageScan;

// The state that owns a menu frame, as the pages above it see it: GameMenu
// for the setup flow, GameLevel for the menus over a match. A MenuContext
// with no owner (a probe, the developer tools' detached context) plays no
// music and has no stage folder.
class MenuOwner
{
public:
	virtual ~MenuOwner() = default;

	// The owner's screen music, ticked by the top covering page. A zero `dt`
	// starts a requested fade at the current gain without advancing it.
	virtual void update_menu_music(float dt) = 0;
	// A page lost focus or was covered; the owner's music waits for its next tick.
	virtual void suspend_menu_music() {}

	// ROAD#16: rereads the player's stage folder. Only GameMenu has one to
	// lend, so the stage list holds still under a running match or series.
	virtual void rescan_stages() {}
	// The retained result of the last rescan; reading it never scans.
	virtual const UserStageScan* stage_scan() const { return nullptr; }
protected:
	MenuOwner() = default;
	MenuOwner(const MenuOwner&) = default;
	MenuOwner& operator=(const MenuOwner&) = default;
};
