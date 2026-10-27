#pragma once

#include "game/action_bindings.h"
#include "game/input_slots.h"
#include "game/save_section.h"
#include <cstdint>
#include <optional>
#include <string_view>

// The physical pad controls a gameplay action may take. none is only valid as
// an alternate. Back stays the debug control and the D-pad stays menu-only.
enum class PadControl : std::uint8_t
{
	none, a, b, x, y, left_shoulder, right_shoulder, left_trigger, right_trigger,
	left_stick, right_stick, start
};

// The rebindable pad actions, in page and save order. Movement, aim and the
// stick drop stay on the sticks; debug, intro skip and menus stay fixed.
enum class PadAction : std::uint8_t { jump, shoot, pause, count };
inline constexpr std::size_t pad_action_count = static_cast<std::size_t>(PadAction::count);

struct PadActionChoice
{
	PadControl primary = PadControl::none;
	PadControl alternate = PadControl::none;
	bool operator==(const PadActionChoice&) const = default;
};

// One physical XInput slot's choices. The defaults are the controls the game
// has always had: A / LT jump, RT shoots, Start pauses.
struct PadProfile
{
	std::array<PadActionChoice, pad_action_count> actions{ {
		{ PadControl::a, PadControl::left_trigger },
		{ PadControl::right_trigger, PadControl::none },
		{ PadControl::start, PadControl::none }
	} };
	PadActionChoice& operator[](PadAction action) { return this->actions.at(static_cast<std::size_t>(action)); }
	const PadActionChoice& operator[](PadAction action) const { return this->actions.at(static_cast<std::size_t>(action)); }
	// Every action needs a primary control, and no control serves two entries.
	void validate() const;
	bool operator==(const PadProfile&) const = default;
};

// Gameplay preferences: the shared keyboard Jump key and one pad profile per
// physical XInput slot. Menu/intro aliases stay fixed.
struct InputBindings
{
	labrador::Key keyboard_jump = labrador::Key::space;
	std::array<PadProfile, input_slots::pad_count> pads{};
	void validate() const;
	bool operator==(const InputBindings&) const = default;
};

// This first domain is explicitly A-Z and Space, independent of Key ordinals.
// Names are labrador::key_name's tokens ("a", "space"), restricted to that
// domain: the spelling is the engine's file format. Labels are this game's.
// Conversion outside this domain throws invalid_argument. Conflict checks use
// other default gameplay keyboard sources, excluding Jump and intro skip.
bool keyboard_jump_key_supported(labrador::Key key);
std::array<labrador::Key, 27> keyboard_jump_keys();
std::optional<GameAction> keyboard_jump_conflict(labrador::Key key);
std::string_view input_key_name(labrador::Key key);
std::wstring_view input_key_label(labrador::Key key);
labrador::Key input_key_from_name(std::string_view name);

// The eleven bindable controls in page order (none excluded). Canonical save
// tokens are "none", "a", "lb", "lt", "ls", "start" and so on; labels are what
// a player is shown. Other values and spellings throw invalid_argument.
std::array<PadControl, 11> pad_controls();
bool pad_control_supported(PadControl control);
std::string_view pad_control_name(PadControl control);
std::wstring_view pad_control_label(PadControl control);
PadControl pad_control_from_name(std::string_view name);
// "jump", "shoot", "pause": the save keys.
std::string_view pad_action_name(PadAction action);
// The action whose other entry in `profile` already uses `control`, ignoring
// the entry being edited (action, alternate). none never conflicts.
std::optional<PadAction> pad_control_conflict(const PadProfile& profile,
	PadAction action, bool alternate, PadControl control);

// A validated copy with only the gameplay Jump keyboard source replaced.
ActionBindings effective_action_bindings(const InputBindings& preferences);
// Also replace the selected physical slot's Jump, Shoot and Pause sources.
// A trigger used for Shoot keeps the shoot threshold; on Jump or Pause it uses
// the jump threshold. Invalid slots throw out_of_range.
ActionBindings effective_action_bindings(const InputBindings& preferences, int pad_slot);

class InputBindingsSection final : public SaveSection
{
public:
	const char* key() const override { return "input_bindings"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
