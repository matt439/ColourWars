#pragma once

#include "game/action_bindings.h"
#include "game/save_section.h"
#include <optional>
#include <string_view>

// The shared keyboard player's saved preference. Menu aliases, introductory
// skip and pad controls remain the defaults owned by ActionBindings.
struct InputBindings
{
	labrador::Key keyboard_jump = labrador::Key::space;
	void validate() const;
	bool operator==(const InputBindings&) const = default;
};

// This first domain is explicitly A-Z and Space, independent of Key ordinals.
// Names are strict lowercase save tokens; labels name physical key positions.
// Conversion outside this domain throws invalid_argument. Conflict checks use
// other default gameplay keyboard sources, excluding Jump and intro skip.
bool keyboard_jump_key_supported(labrador::Key key);
std::array<labrador::Key, 27> keyboard_jump_keys();
std::optional<GameAction> keyboard_jump_conflict(labrador::Key key);
std::string_view input_key_name(labrador::Key key);
std::wstring_view input_key_label(labrador::Key key);
labrador::Key input_key_from_name(std::string_view name);

// A validated copy with only the gameplay Jump keyboard source replaced.
ActionBindings effective_action_bindings(const InputBindings& preferences);

class InputBindingsSection final : public SaveSection
{
public:
	const char* key() const override { return "input_bindings"; }
	void reset(SaveData& data) const override;
	void read(const labrador::JsonValue& root, SaveData& data) const override;
	void write(SaveWriter& writer, const SaveData& data) const override;
};
