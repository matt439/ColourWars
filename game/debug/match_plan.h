#pragma once

#include "game/objects/stage.h"
#include "game/states/menu_level_settings.h"
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// A partial match request built by console statements and launch arguments.
// Only the request is stored: device claims and random choices are resolved
// when the plan is applied, after the pads have been polled.
struct MatchRequest
{
	struct WeaponChoice
	{
		int seat = -1; // 0-based; -1 means every seat
		std::string weapon; // a weapon name or "random"
	};

	std::optional<std::string> stage; // a stage key, or random/next/prev
	std::optional<int> players;
	std::vector<WeaponChoice> weapons;
	std::optional<PlayerVisualMode> visual;
	bool restart = false;

	bool empty() const;
	std::string describe() const;
};

struct MatchPlan
{
	bool ok = false;
	std::string error;
	MenuLevelSettings settings;

	// A new roster, stage or best-of starts a new session; a restart does not.
	bool reset_session = false;
};

// Turns a request into complete MenuLevelSettings. Pure: it takes the stage
// list, which pads are connected and a random source as values, so the
// acceptance case can drive it without a window or a pad.
class MatchPlanner
{
public:
	using ConnectedPads = std::array<bool, 4>;
	using Random = std::function<unsigned int()>;

	explicit MatchPlanner(const StageList* stages);

	// `current` is the running match's settings, or nullptr when starting a
	// match from the menus (one keyboard/mouse player on the first stage).
	MatchPlan resolve(const MenuLevelSettings* current, const MatchRequest& request,
		const ConnectedPads& pads, const Random& random) const;

	// Seats take connected pads in slot order, then keyboard/mouse; seats
	// left over are idle dummies (-1), which get neutral input and are never
	// reported as a disconnected pad. With no pad, seat 0 is keyboard/mouse.
	static void assign_devices(MenuLevelSettings& settings, int players,
		const ConnectedPads& pads);

	// Weapon names accepted by "weapon": the five primaries and random.
	static std::vector<std::string> weapon_names();
	static bool weapon_from_name(const std::string& name, WeaponType& weapon);
	static const char* weapon_name(WeaponType weapon);

	// The console's names for PlayerVisualMode: original, rotating, posed.
	static const char* visual_name(PlayerVisualMode mode);

	// Validates a stage word now, so a typo fails when typed.
	bool is_stage_word(const std::string& word) const;
	std::vector<std::string> stage_words() const;

private:
	const StageList* stages_ = nullptr;
};
