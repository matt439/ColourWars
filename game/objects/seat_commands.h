#pragma once

#include "game/objects/player_input_data.h"
#include "game/objects/slot_input.h"
#include <vector>

namespace labrador
{
	class Mouse;
	class ViewportManager;
}
class Level;
class Player;

// One logic step's input for a Level, by participant in roster order.
struct SeatCommands
{
	std::vector<PlayerInputData> seats;
	// A participant's slot asked to skip the intro (Level::request_intro_skip).
	bool skip_intro = false;
};

// The one place device slots become seats. Each participant takes the command
// of the device slot it plays from (Player::player_num), so a bot in P2 does
// not shift P3; a seat with no device slot - a bot, an idle human dummy - gets
// neutral input, which bots and seat filters then replace. A keyboard/mouse
// seat's aim is resolved here, from its player's centre on its own pane to the
// cursor. GameLevel owns one; probes that drive a Level from slot-indexed
// input use one too.
class SeatCommandBuilder
{
public:
	// Both borrowed and outliving the builder. Without a mouse, keyboard aim
	// stays unrequested.
	SeatCommandBuilder(const labrador::Mouse* mouse,
		const labrador::ViewportManager* viewports);

	// `slots` is indexed by device slot (input_slots). Read the level after its
	// refresh_layout, so a camera refitted to a new window size is the one aimed
	// through. Reads only: neither the level nor the slots change.
	SeatCommands build(const Level& level, const std::vector<SlotInput>& slots) const;

private:
	// The camera uses positive uniform scale, so normalising the screen-space
	// direction gives the same aim direction as normalising in world space.
	// `view` is the human pane, independent of participant and device slot.
	void resolve_mouse_aim(const Player& player, int view, PlayerInputData& input) const;

	const labrador::Mouse* mouse_ = nullptr;
	const labrador::ViewportManager* viewports_ = nullptr;
};
