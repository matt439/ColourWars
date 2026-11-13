#pragma once

#include "game/input/action_bindings.h"
#include "game/input/input_slots.h"
#include "game/save/save_section.h"
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

// The physical pad controls a gameplay action may take. none is only valid as
// an alternate. Back stays reserved for the Debug-only panel and the D-pad stays
// menu-only.
enum class PadControl : std::uint8_t
{
	none, a, b, x, y, left_shoulder, right_shoulder, left_trigger, right_trigger,
	left_stick, right_stick, start
};

// The rebindable pad actions, in page and save order. Movement, aim and the
// stick drop stay on the sticks; debug, intro skip and menus stay fixed. Sub
// (ROAD#11) takes one control and no alternate: Pad Buttons has room for one
// more row, not two.
enum class PadAction : std::uint8_t { jump, shoot, pause, sub, count };
inline constexpr std::size_t pad_action_count = static_cast<std::size_t>(PadAction::count);
// Whether the action may take a second control (all but Sub).
constexpr bool pad_action_has_alternate(PadAction action) { return action != PadAction::sub; }
// The Pad Buttons rows: each action's primary, then its alternate where it
// has one. Out-of-range indices throw out_of_range.
struct PadEntry
{
	PadAction action = PadAction::jump;
	bool alternate = false;
};
inline constexpr std::size_t pad_entry_count = pad_action_count * 2 - 1;
constexpr PadEntry pad_entry(std::size_t index)
{
	std::size_t at = 0;
	for (std::size_t action = 0; action < pad_action_count; ++action)
	{
		const PadAction value = static_cast<PadAction>(action);
		if (at++ == index) { return { value, false }; }
		if (pad_action_has_alternate(value) && at++ == index) { return { value, true }; }
	}
	throw std::out_of_range("Unknown pad entry");
}

struct PadActionChoice
{
	PadControl primary = PadControl::none;
	PadControl alternate = PadControl::none;
	bool operator==(const PadActionChoice&) const = default;
};

// One physical XInput slot's choices. The defaults are the controls the game
// has always had: A / LT jump, RT shoots, Start pauses, and RB throws the sub.
// They are stated only here; default_action_bindings converts them. The
// controls_help acceptance case requires the README and HOW-TO-PLAY tables to
// name them (HYG#102).
struct PadProfile
{
	std::array<PadActionChoice, pad_action_count> actions{ {
		{ PadControl::a, PadControl::left_trigger },
		{ PadControl::right_trigger, PadControl::none },
		{ PadControl::start, PadControl::none },
		{ PadControl::right_shoulder, PadControl::none }
	} };
	PadActionChoice& operator[](PadAction action) { return this->actions.at(static_cast<std::size_t>(action)); }
	const PadActionChoice& operator[](PadAction action) const { return this->actions.at(static_cast<std::size_t>(action)); }
	// Every action needs a primary control, Sub has no alternate, and no
	// control serves two entries.
	void validate() const;
	bool operator==(const PadProfile&) const = default;
};

// The keyboard seat's rebindable actions, in page and save order. Mouse aim,
// F1 debug (Debug builds only), intro skip and menus stay fixed; Escape always pauses and Down
// always drops beside the choices below.
enum class KeyboardAction : std::uint8_t { move_left, move_right, jump, drop, shoot, pause, sub, count };
inline constexpr std::size_t keyboard_action_count = static_cast<std::size_t>(KeyboardAction::count);
// Shoot and Sub may take a mouse button as well as a key.
constexpr bool keyboard_action_takes_mouse(KeyboardAction action)
{
	return action == KeyboardAction::shoot || action == KeyboardAction::sub;
}

// One desktop control: a key, or (Shoot and Sub only) a mouse button. Both
// none is "unbound", valid only for the optional extra Pause key.
struct DesktopControl
{
	labrador::Key key = labrador::Key::none;
	labrador::MouseButton button = labrador::MouseButton::none;
	bool operator==(const DesktopControl&) const = default;
};

// Gameplay preferences: the shared keyboard keys and one pad profile per
// physical XInput slot. Menu/intro aliases stay fixed. A default-constructed
// one is the game's default controls, which default_action_bindings reads.
struct InputBindings
{
	labrador::Key keyboard_jump = player_input_consts::jump_key;
	labrador::Key keyboard_move_left = player_input_consts::move_left_key;
	labrador::Key keyboard_move_right = player_input_consts::move_right_key;
	labrador::Key keyboard_drop = player_input_consts::drop_key;
	DesktopControl keyboard_shoot{ labrador::Key::none, player_input_consts::shoot_button };
	// In addition to the fixed Escape.
	labrador::Key keyboard_pause = labrador::Key::none;
	// The sub weapon (ROAD#11).
	DesktopControl keyboard_sub{ labrador::Key::none, labrador::MouseButton::right };
	std::array<PadProfile, input_slots::pad_count> pads{};
	void validate() const;
	// Only the seven keyboard choices: each in its action's domain, none twice.
	void validate_keyboard() const;
	bool operator==(const InputBindings&) const = default;
};

// This first domain is explicitly A-Z and Space, independent of Key ordinals.
// Names are labrador::key_name's tokens ("a", "space"), restricted to that
// domain: the spelling is the engine's file format. A legend is the letter
// printed on the key, the same in every language; Space has none (an empty
// view). Player-facing labels are controls_text's, which take the words (Space,
// None, Start, the mouse buttons) from menu_text.controls.labels.
// Conversion outside this domain throws invalid_argument. Conflict checks use
// other default gameplay keyboard sources, excluding Jump and intro skip.
bool keyboard_key_supported(labrador::Key key);
std::array<labrador::Key, 27> keyboard_keys();
// Against the default keyboard choices, as the Keyboard Jump slice defined it.
std::optional<GameAction> keyboard_jump_conflict(labrador::Key key);
std::string_view input_key_name(labrador::Key key);
std::wstring_view input_key_legend(labrador::Key key);
labrador::Key input_key_from_name(std::string_view name);

// The keyboard actions slice (GAP#4.2). Every action's keys are the A-Z/Space
// domain above; Shoot and Sub also take the five mouse buttons below, and Pause
// may be none. Save tokens are the key names, "mouse_left" ... "mouse_x2" and "none";
// what a player is shown is controls_text::desktop_label's. Other values and
// spellings throw invalid_argument; desktop_control_name also refuses a control
// that is both a key and a button.
std::array<labrador::MouseButton, 5> shoot_mouse_buttons();
bool keyboard_control_supported(KeyboardAction action, const DesktopControl& control);
std::string_view desktop_control_name(const DesktopControl& control);
DesktopControl desktop_control_from_name(std::string_view name);
// "move_left", "move_right", "jump", "drop", "shoot", "pause", "sub".
std::string_view keyboard_action_name(KeyboardAction action);
GameAction keyboard_game_action(KeyboardAction action);
DesktopControl keyboard_control(const InputBindings& bindings, KeyboardAction action);
// Throws invalid_argument for a control outside the action's domain.
void set_keyboard_control(InputBindings& bindings, KeyboardAction action, const DesktopControl& control);
// The other action in `bindings` already using `control`; none never conflicts.
std::optional<KeyboardAction> keyboard_control_conflict(const InputBindings& bindings,
	KeyboardAction action, const DesktopControl& control);
// Whether the two preferences' keyboard choices are identical.
bool same_keyboard(const InputBindings& left, const InputBindings& right);

// The eleven bindable controls in page order (none excluded). Canonical save
// tokens are "none", "a", "lb", "lt", "ls", "start" and so on; legends are what
// is printed on the button ("A", "LB"), empty for none and Start, whose words
// controls_text::pad_label takes from the locale. Other values and spellings
// throw invalid_argument.
std::array<PadControl, 11> pad_controls();
bool pad_control_supported(PadControl control);
std::string_view pad_control_name(PadControl control);
std::wstring_view pad_control_legend(PadControl control);
PadControl pad_control_from_name(std::string_view name);
// "jump", "shoot", "pause", "sub": the save keys.
std::string_view pad_action_name(PadAction action);
// The action whose other entry in `profile` already uses `control`, ignoring
// the entry being edited (action, alternate). none never conflicts.
std::optional<PadAction> pad_control_conflict(const PadProfile& profile,
	PadAction action, bool alternate, PadControl control);

// A validated copy with the keyboard choices written into Move left/right,
// Jump, Drop (beside Down), Shoot, Pause (beside Escape) and Sub.
ActionBindings effective_action_bindings(const InputBindings& preferences);
// Also replace the selected physical slot's Jump, Shoot, Pause and Sub sources.
// A trigger used for Shoot keeps the shoot threshold; on Jump or Pause it uses
// the jump threshold. Invalid slots throw out_of_range.
ActionBindings effective_action_bindings(const InputBindings& preferences, int pad_slot);

// One pad profile as the save spells it: { "jump": ["a", "lt"], "shoot":
// ["rt"], "pause": ["start"], "sub": ["rb"] }. A profile written before Sub
// (save version 9) has no "sub": it reads as RB, or the first control the
// others leave free in pad_controls order after RB and LB. Reading validates
// and throws runtime_error or invalid_argument; profiles
// (game/save/player_profiles.h) share this spelling.
PadProfile read_pad_profile(const labrador::JsonValue& entry);
void write_pad_profile(SaveWriter& writer, const PadProfile& profile);

class InputBindingsSection final : public SaveSection
{
public:
	const char* key() const override { return "input_bindings"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
