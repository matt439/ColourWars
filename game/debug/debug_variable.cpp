#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_variable.h"
#include "game/debug/debug_command.h"
#include <cstdio>

DebugStatus DebugStatus::success(std::string message, bool close_console)
{
	DebugStatus status;
	status.message = std::move(message);
	status.close_console = close_console;
	return status;
}

DebugStatus DebugStatus::failure(std::string message)
{
	DebugStatus status;
	status.ok = false;
	status.message = std::move(message);
	return status;
}

DebugEntry::DebugEntry(std::string name, std::string help) :
	name_(std::move(name)), help_(std::move(help))
{
}

const std::string& DebugEntry::name() const
{
	return this->name_;
}

const std::string& DebugEntry::help() const
{
	return this->help_;
}

const DebugMenuTraits& DebugEntry::menu() const
{
	return this->menu_;
}

void DebugEntry::set_menu(DebugMenuTraits traits)
{
	this->menu_ = std::move(traits);
}

DebugVariable::DebugVariable(std::string name, std::string help, std::string default_text) :
	DebugEntry(std::move(name), std::move(help)), default_text_(std::move(default_text))
{
	DebugMenuTraits traits;
	traits.form = DebugMenuTraits::Form::variable;
	this->set_menu(std::move(traits));
}

std::string DebugVariable::step_word(int direction) const
{
	return direction >= 0 ? "next" : "prev";
}

DebugStatus DebugVariable::step(int direction)
{
	return this->assign(this->step_word(direction));
}

const std::string& DebugVariable::default_text() const
{
	return this->default_text_;
}

std::string DebugVariable::usage() const
{
	std::string words;
	for (const std::string& word : this->accepted_words())
	{
		words += words.empty() ? "" : "|";
		words += word;
	}
	return this->name() + " [" + words + "]";
}

std::vector<std::string> DebugVariable::suggestions(std::size_t argument) const
{
	return argument == 0 ? this->accepted_words() : std::vector<std::string>();
}

BoolVariable::BoolVariable(std::string name, std::string help, bool default_value,
	std::function<bool()> get, std::function<DebugStatus(bool)> set) :
	DebugVariable(std::move(name), std::move(help), default_value ? "on" : "off"),
	get_(std::move(get)), set_(std::move(set))
{
}

std::string BoolVariable::value_text() const
{
	return this->get_() ? "on" : "off";
}

bool BoolVariable::parse(std::string_view text, bool current, bool& ok)
{
	ok = true;
	if (text == "1" || text == "on" || text == "true" || text == "yes")
	{
		return true;
	}
	if (text == "0" || text == "off" || text == "false" || text == "no")
	{
		return false;
	}
	if (text == "toggle")
	{
		return !current;
	}
	ok = false;
	return current;
}

DebugStatus BoolVariable::assign(std::string_view text)
{
	bool ok = false;
	const bool value = parse(text, this->get_(), ok);
	if (!ok)
	{
		return DebugStatus::failure(this->name() + ": '" + std::string(text) +
			"' is not on/off/1/0/toggle");
	}
	return this->set_(value);
}

std::vector<std::string> BoolVariable::accepted_words() const
{
	return { "on", "off", "toggle" };
}

std::string BoolVariable::step_word(int /*direction*/) const
{
	return "toggle";
}

ChoiceVariable::ChoiceVariable(std::string name, std::string help,
	std::string default_value, std::vector<std::string> choices,
	std::vector<std::pair<std::string, std::string>> aliases,
	std::function<std::string()> get,
	std::function<DebugStatus(const std::string&)> set) :
	DebugVariable(std::move(name), std::move(help), std::move(default_value)),
	choices_(std::move(choices)), aliases_(std::move(aliases)),
	get_(std::move(get)), set_(std::move(set))
{
}

std::string ChoiceVariable::value_text() const
{
	return this->get_();
}

std::string ChoiceVariable::resolve(std::string_view text) const
{
	for (const std::string& choice : this->choices_)
	{
		if (choice == text)
		{
			return choice;
		}
	}
	for (const auto& alias : this->aliases_)
	{
		if (alias.first == text)
		{
			return alias.second;
		}
	}
	if ((text == "next" || text == "prev") && !this->choices_.empty())
	{
		const std::string current = this->get_();
		const auto found = std::find(this->choices_.begin(), this->choices_.end(), current);
		const int count = static_cast<int>(this->choices_.size());
		int index = found == this->choices_.end() ? 0 :
			static_cast<int>(found - this->choices_.begin());
		index = (index + (text == "next" ? 1 : count - 1)) % count;
		return this->choices_[static_cast<std::size_t>(index)];
	}
	return {};
}

DebugStatus ChoiceVariable::assign(std::string_view text)
{
	const std::string choice = this->resolve(text);
	if (choice.empty())
	{
		return DebugStatus::failure(this->name() + ": '" + std::string(text) +
			"' is not one of " + this->usage());
	}
	return this->set_(choice);
}

std::vector<std::string> ChoiceVariable::suggestions(std::size_t argument) const
{
	return argument == 0 ? this->choices_ : std::vector<std::string>();
}

std::vector<std::string> ChoiceVariable::accepted_words() const
{
	std::vector<std::string> words = this->choices_;
	words.push_back("next");
	words.push_back("prev");
	return words;
}
NumberVariable::NumberVariable(std::string name, std::string help, float default_value,
	float minimum, float maximum, std::vector<float> steps,
	std::function<float()> get, std::function<DebugStatus(float)> set) :
	DebugVariable(std::move(name), std::move(help), format(default_value)),
	minimum_(minimum), maximum_(maximum), steps_(std::move(steps)),
	get_(std::move(get)), set_(std::move(set))
{
	std::sort(this->steps_.begin(), this->steps_.end());
}

std::string NumberVariable::format(float value)
{
	char buffer[32] = {};
	std::snprintf(buffer, sizeof(buffer), "%g", static_cast<double>(value));
	return buffer;
}

std::string NumberVariable::value_text() const
{
	return format(this->get_());
}

bool NumberVariable::resolve(std::string_view text, float current, float& value) const
{
	if (text == "next" || text == "prev")
	{
		value = current;
		if (text == "next")
		{
			const auto above = std::upper_bound(this->steps_.begin(), this->steps_.end(), current);
			value = above != this->steps_.end() ? *above : current;
		}
		else
		{
			const auto below = std::lower_bound(this->steps_.begin(), this->steps_.end(), current);
			value = below != this->steps_.begin() ? *(below - 1) : current;
		}
		return true;
	}
	return DebugArgs::parse_float(text, value) && value >= this->minimum_ && value <= this->maximum_;
}

DebugStatus NumberVariable::assign(std::string_view text)
{
	float value = 0.0f;
	if (!this->resolve(text, this->get_(), value))
	{
		return DebugStatus::failure(this->name() + ": expected " + format(this->minimum_) + "-" +
			format(this->maximum_) + ", got '" + std::string(text) + "'");
	}
	return this->set_(value);
}

std::vector<std::string> NumberVariable::accepted_words() const
{
	std::vector<std::string> words;
	for (const float step : this->steps_)
	{
		words.push_back(format(step));
	}
	words.push_back("next");
	words.push_back("prev");
	return words;
}

std::vector<std::string> NumberVariable::suggestions(std::size_t argument) const
{
	std::vector<std::string> words;
	if (argument == 0)
	{
		for (const float step : this->steps_)
		{
			words.push_back(format(step));
		}
	}
	return words;
}
#endif
