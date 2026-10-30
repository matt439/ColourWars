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
// has always had: A / LT jump, RT shoots, Start pauses. They are stated only
// here; default_action_bindings converts them.
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

// The keyboard seat's rebindable actions, in page and save order. Mouse aim,
// F1 debug, intro skip and menus stay fixed; Escape always pauses and Down
// always drops beside the choices below.
enum class KeyboardAction : std::uint8_t { move_left, move_right, jump, drop, shoot, pause, count };
inline constexpr std::size_t keyboard_action_count = static_cast<std::size_t>(KeyboardAction::count);

// One desktop control: a key, or (Shoot only) a mouse button. Both none is
// "unbound", valid only for the optional extra Pause key.
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
	std::array<PadProfile, input_slots::pad_count> pads{};
	void validate() const;
	// Only the six keyboard choices: each in its action's domain, none twice.
	void validate_keyboard() const;
	bool operator==(const InputBindings&) const = default;
};

// This first domain is explicitly A-Z and Space, independent of Key ordinals.
// Names are labrador::key_name's tokens ("a", "space"), restricted to that
// domain: the spelling is the engine's file format. Labels are this game's.
// Conversion outside this domain throws invalid_argument. Conflict checks use
// other default gameplay keyboard sources, excluding Jump and intro skip.
bool keyboard_jump_key_supported(labrador::Key key);
std::array<labrador::Key, 27> keyboard_jump_keys();
// Against the default keyboard choices, as the Keyboard Jump slice defined it.
std::optional<GameAction> keyboard_jump_conflict(labrador::Key key);
std::string_view input_key_name(labrador::Key key);
std::wstring_view input_key_label(labrador::Key key);
labrador::Key input_key_from_name(std::string_view name);

// The keyboard actions slice (GAP#4.2). Every action's keys are the A-Z/Space
// domain above; Shoot also takes the five mouse buttons below, and Pause may be
// none. Save tokens are the key names, "mouse_left" ... "mouse_x2" and "none";
// labels are "A", "Space", "Left click", "Mouse 4", "None". Other values and
// spellings throw invalid_argument.
std::array<labrador::MouseButton, 5> shoot_mouse_buttons();
bool keyboard_control_supported(KeyboardAction action, const DesktopControl& control);
std::string_view desktop_control_name(const DesktopControl& control);
std::wstring_view desktop_control_label(const DesktopControl& control);
DesktopControl desktop_control_from_name(std::string_view name);
// "move_left", "move_right", "jump", "drop", "shoot", "pause".
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

// A validated copy with the keyboard choices written into Move left/right,
// Jump, Drop (beside Down), Shoot and Pause (beside Escape).
ActionBindings effective_action_bindings(const InputBindings& preferences);
// Also replace the selected physical slot's Jump, Shoot and Pause sources.
// A trigger used for Shoot keeps the shoot threshold; on Jump or Pause it uses
// the jump threshold. Invalid slots throw out_of_range.
ActionBindings effective_action_bindings(const InputBindings& preferences, int pad_slot);

// One pad profile as the save spells it: { "jump": ["a", "lt"], "shoot":
// ["rt"], "pause": ["start"] }. Reading validates and throws runtime_error or
// invalid_argument; profiles (game/player_profiles.h) share this spelling.
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
