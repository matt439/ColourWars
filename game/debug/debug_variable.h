#pragma once

#include "game/debug/debug_entry.h"
#include <functional>
#include <utility>

// A named value with a type, a current value and a default. Variables are
// views onto game state, so they are bound to getter/setter callables.
class DebugVariable : public DebugEntry
{
public:
	bool is_variable() const override { return true; }
	std::string usage() const override;
	std::vector<std::string> suggestions(std::size_t argument) const override;

	virtual std::string_view type_name() const = 0;
	virtual std::string value_text() const = 0;
	const std::string& default_text() const;

	// Parse, validate and set. Never throws for bad text; the setter may.
	virtual DebugStatus assign(std::string_view text) = 0;

	// The words this variable accepts, for completion and errors.
	virtual std::vector<std::string> accepted_words() const = 0;

protected:
	DebugVariable(std::string name, std::string help, std::string default_text);

private:
	std::string default_text_;
};

// on/off; accepts 1/0, on/off, true/false, yes/no and toggle.
class BoolVariable final : public DebugVariable
{
public:
	BoolVariable(std::string name, std::string help, bool default_value,
		std::function<bool()> get, std::function<DebugStatus(bool)> set);

	std::string_view type_name() const override { return "bool"; }
	std::string value_text() const override;
	DebugStatus assign(std::string_view text) override;
	std::vector<std::string> accepted_words() const override;

	// The bool reading of a word, or false in `ok` when it is not one.
	static bool parse(std::string_view text, bool current, bool& ok);

private:
	std::function<bool()> get_;
	std::function<DebugStatus(bool)> set_;
};

// One of a fixed list of words. Aliases map other spellings onto a choice;
// "next" and "prev" step through the list, wrapping.
class ChoiceVariable final : public DebugVariable
{
public:
	ChoiceVariable(std::string name, std::string help, std::string default_value,
		std::vector<std::string> choices,
		std::vector<std::pair<std::string, std::string>> aliases,
		std::function<std::string()> get,
		std::function<DebugStatus(const std::string&)> set);

	std::string_view type_name() const override { return "choice"; }
	std::string value_text() const override;
	DebugStatus assign(std::string_view text) override;
	std::vector<std::string> accepted_words() const override;

	// Completes to the choices only, so "visual p" is "posed", not "prev".
	std::vector<std::string> suggestions(std::size_t argument) const override;

	// The canonical choice for a word (choice, alias, next or prev), or empty.
	std::string resolve(std::string_view text) const;

private:
	std::vector<std::string> choices_;
	std::vector<std::pair<std::string, std::string>> aliases_;
	std::function<std::string()> get_;
	std::function<DebugStatus(const std::string&)> set_;
};
