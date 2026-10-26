#pragma once

#include "game/action_bindings.h"
#include "game/input_slots.h"
#include "game/save_section.h"
#include <optional>
#include <string_view>

// Gameplay Jump preferences: the shared keyboard key and one face button per
// physical XInput slot. Menu/intro aliases and the LT Jump source stay fixed.
struct InputBindings
{
	labrador::Key keyboard_jump = labrador::Key::space;
	std::array<labrador::GamepadButton, input_slots::pad_count> pad_jump = []
	{
		std::array<labrador::GamepadButton, input_slots::pad_count> result{};
		result.fill(labrador::GamepadButton::a);
		return result;
	}();
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

// Canonical save tokens are a/b/x/y; labels are A/B/X/Y. Other buttons and
// noncanonical spellings throw invalid_argument, independently of menu roles.
bool pad_jump_button_supported(labrador::GamepadButton button);
std::string_view input_button_name(labrador::GamepadButton button);
std::wstring_view input_button_label(labrador::GamepadButton button);
labrador::GamepadButton input_button_from_name(std::string_view name);

// A validated copy with only the gameplay Jump keyboard source replaced.
ActionBindings effective_action_bindings(const InputBindings& preferences);
// Also replace the selected physical slot's gameplay Jump face button. The LT
// alias and other actions remain unchanged. Invalid slots throw out_of_range.
ActionBindings effective_action_bindings(const InputBindings& preferences, int pad_slot);

class InputBindingsSection final : public SaveSection
{
public:
	const char* key() const override { return "input_bindings"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
