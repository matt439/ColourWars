#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_registry.h"
#include "game/debug/debug_log.h"
#include "game/debug/debug_tokenizer.h"

namespace
{
	std::string common_prefix(const std::vector<std::string>& words)
	{
		if (words.empty())
		{
			return {};
		}
		std::string prefix = words.front();
		for (const std::string& word : words)
		{
			std::size_t length = 0;
			while (length < prefix.size() && length < word.size() &&
				prefix[length] == word[length])
			{
				++length;
			}
			prefix.resize(length);
		}
		return prefix;
	}

	std::vector<std::string> variable_names(const DebugRegistry& registry)
	{
		std::vector<std::string> names;
		for (const DebugEntry* entry : registry.entries())
		{
			if (entry->is_variable())
			{
				names.push_back(entry->name());
			}
		}
		return names;
	}

	std::vector<std::string> entry_names(const DebugRegistry& registry)
	{
		std::vector<std::string> names;
		for (const DebugEntry* entry : registry.entries())
		{
			names.push_back(entry->name());
		}
		return names;
	}
}

DebugRegistry::DebugRegistry()
{
	this->add_builtins();
}

void DebugRegistry::add(std::unique_ptr<DebugEntry> entry)
{
	const std::string name = entry->name();
	if (!this->entries_.emplace(name, std::move(entry)).second)
	{
		throw std::logic_error("debug registry: duplicate name '" + name + "'");
	}
}

DebugEntry* DebugRegistry::find(std::string_view name) const
{
	const auto found = this->entries_.find(name);
	return found == this->entries_.end() ? nullptr : found->second.get();
}

DebugVariable* DebugRegistry::variable(std::string_view name) const
{
	DebugEntry* entry = this->find(name);
	return entry != nullptr && entry->is_variable() ?
		static_cast<DebugVariable*>(entry) : nullptr;
}

std::vector<const DebugEntry*> DebugRegistry::entries() const
{
	std::vector<const DebugEntry*> result;
	for (const auto& entry : this->entries_)
	{
		result.push_back(entry.second.get());
	}
	return result;
}

const std::vector<std::string>& DebugRegistry::category_order()
{
	static const std::vector<std::string> order = {
		"match", "player", "view", "time", "tuning", "replay", "content", "console" };
	return order;
}

std::vector<std::string> DebugRegistry::categories() const
{
	std::vector<std::string> result;
	for (const std::string& category : category_order())
	{
		if (!this->entries(category).empty())
		{
			result.push_back(category);
		}
	}
	return result;
}

std::vector<const DebugEntry*> DebugRegistry::entries(std::string_view category) const
{
	std::vector<const DebugEntry*> result;
	for (const auto& entry : this->entries_)
	{
		const DebugMenuTraits& traits = entry.second->menu();
		if (traits.form != DebugMenuTraits::Form::hidden && traits.category == category)
		{
			result.push_back(entry.second.get());
		}
	}
	return result;
}

DebugCompletion DebugRegistry::complete(std::string_view line) const
{
	DebugCompletion completion;
	completion.line = std::string(line);

	// Complete the last statement only.
	const std::size_t statement_start = line.find_last_of(';') == std::string_view::npos ?
		0 : line.find_last_of(';') + 1;
	const std::size_t word_start = line.find_last_of(" \t") == std::string_view::npos ||
		line.find_last_of(" \t") < statement_start ?
		statement_start : line.find_last_of(" \t") + 1;
	const std::string_view word = line.substr(word_start);
	const DebugTokens before = tokenize(line.substr(statement_start, word_start - statement_start));
	if (!before.ok)
	{
		return completion;
	}

	std::vector<std::string> pool;
	if (before.words.empty())
	{
		pool = entry_names(*this);
	}
	else
	{
		const std::string& head = before.words.front();
		const std::size_t argument = before.words.size() - 1;
		if (head == "get" || head == "set")
		{
			if (argument == 0)
			{
				pool = variable_names(*this);
			}
			else if (head == "set" && argument == 1)
			{
				if (const DebugVariable* variable = this->variable(before.words[1]))
				{
					pool = variable->suggestions(0);
				}
			}
		}
		else if (head == "help")
		{
			if (argument == 0)
			{
				pool = entry_names(*this);
			}
		}
		else if (const DebugEntry* entry = this->find(head))
		{
			// Choices are callables over live game state; one that throws
			// completes nothing rather than reaching the console's update.
			try
			{
				const std::vector<std::string> previous(before.words.begin() + 1,
					before.words.end());
				pool = entry->completions(previous, word);
			}
			catch (const std::exception&)
			{
				pool.clear();
			}
		}
	}

	std::vector<std::string> matches;
	for (const std::string& candidate : pool)
	{
		if (candidate.compare(0, word.size(), word) == 0 &&
			std::find(matches.begin(), matches.end(), candidate) == matches.end())
		{
			matches.push_back(candidate);
		}
	}
	if (matches.empty())
	{
		return completion;
	}

	const std::string head = std::string(line.substr(0, word_start));
	if (matches.size() == 1)
	{
		// A path segment ("tuning.player.") continues the same word.
		const char last = matches.front().empty() ? ' ' : matches.front().back();
		completion.line = head + matches.front() + (last == '.' || last == '[' ? "" : " ");
		return completion;
	}
	std::sort(matches.begin(), matches.end());
	completion.line = head + common_prefix(matches);
	completion.candidates = matches;
	return completion;
}

DebugStatus DebugRegistry::execute(std::string_view line, DebugLog& log)
{
	DebugStatus combined;
	for (const std::string& statement : split_statements(line))
	{
		DebugStatus status;
		try
		{
			status = this->execute_statement(statement, log);
		}
		catch (const std::exception& e)
		{
			status = DebugStatus::failure(std::string("exception: ") + e.what());
		}
		if (!status.message.empty())
		{
			log.write(status.ok ? DebugSeverity::info : DebugSeverity::error, status.message);
		}
		if (!status.ok && combined.ok)
		{
			combined.message = status.message;
		}
		combined.ok = combined.ok && status.ok;
		combined.close_console = combined.close_console || status.close_console;
	}
	return combined;
}

DebugStatus DebugRegistry::check(std::string_view statement) const
{
	const DebugTokens tokens = tokenize(statement);
	if (!tokens.ok)
	{
		return DebugStatus::failure(tokens.error);
	}
	if (tokens.words.empty())
	{
		return {};
	}
	const DebugEntry* entry = this->find(tokens.words.front());
	if (entry == nullptr)
	{
		return DebugStatus::failure("unknown command '" + tokens.words.front() + "'");
	}
	const std::size_t given = tokens.words.size() - 1;
	if (entry->is_variable())
	{
		return given <= 1 ? DebugStatus::success() :
			DebugStatus::failure("usage: " + entry->usage());
	}
	const DebugCommand* command = static_cast<const DebugCommand*>(entry);
	const std::vector<DebugArgSpec>& specs = command->args();
	const std::size_t required = static_cast<std::size_t>(std::count_if(
		specs.begin(), specs.end(), [](const DebugArgSpec& spec) { return !spec.optional; }));
	if (given < required || (!command->variadic() && given > specs.size()))
	{
		return DebugStatus::failure("usage: " + command->usage());
	}
	return DebugStatus::success();
}

DebugStatus DebugRegistry::execute_statement(const std::string& statement, DebugLog& log)
{
	const DebugTokens tokens = tokenize(statement);
	if (!tokens.ok)
	{
		return DebugStatus::failure(tokens.error);
	}
	if (tokens.words.empty())
	{
		return {};
	}

	const std::string& name = tokens.words.front();
	DebugEntry* entry = this->find(name);
	if (entry == nullptr)
	{
		return DebugStatus::failure("unknown command '" + name + "' (try help)");
	}

	std::vector<std::string> rest(tokens.words.begin() + 1, tokens.words.end());

	// Quake shorthand: "feedback" reads, "feedback off" assigns.
	if (entry->is_variable())
	{
		DebugVariable* variable = static_cast<DebugVariable*>(entry);
		if (rest.empty())
		{
			return DebugStatus::success(variable->name() + " = " + variable->value_text());
		}
		if (rest.size() > 1)
		{
			return DebugStatus::failure("usage: " + variable->usage());
		}
		return variable->assign(rest.front());
	}

	DebugCommand* command = static_cast<DebugCommand*>(entry);
	const std::vector<DebugArgSpec>& specs = command->args();
	const std::size_t required = static_cast<std::size_t>(std::count_if(
		specs.begin(), specs.end(), [](const DebugArgSpec& spec) { return !spec.optional; }));
	if (rest.size() < required || (!command->variadic() && rest.size() > specs.size()))
	{
		return DebugStatus::failure("usage: " + command->usage());
	}
	return command->run(DebugArgs(std::move(rest)), log);
}

void DebugRegistry::add_builtins()
{
	this->add(std::make_unique<LambdaCommand>("set", "assign a variable",
		std::vector<DebugArgSpec>{ { "variable", false, {} }, { "value", false, {} } },
		[this](const DebugArgs& args, DebugLog&)
		{
			DebugVariable* variable = this->variable(args.text(0));
			if (variable == nullptr)
			{
				return DebugStatus::failure("no variable '" + args.text(0) + "' (try list)");
			}
			DebugStatus status = variable->assign(args.text(1));
			if (status.ok && status.message.empty())
			{
				status.message = variable->name() + " = " + variable->value_text();
			}
			return status;
		}));

	this->add(std::make_unique<LambdaCommand>("get", "print a variable",
		std::vector<DebugArgSpec>{ { "variable", false, {} } },
		[this](const DebugArgs& args, DebugLog&)
		{
			const DebugVariable* variable = this->variable(args.text(0));
			if (variable == nullptr)
			{
				return DebugStatus::failure("no variable '" + args.text(0) + "' (try list)");
			}
			return DebugStatus::success(variable->name() + " = " + variable->value_text() +
				"  (" + std::string(variable->type_name()) + ", default " +
				variable->default_text() + ")");
		}));

	this->add(std::make_unique<LambdaCommand>("list", "list variables with their values",
		std::vector<DebugArgSpec>{ { "prefix", true, {} } },
		[this](const DebugArgs& args, DebugLog& log)
		{
			const std::string prefix = args.has(0) ? args.text(0) : std::string();
			int shown = 0;
			for (const DebugEntry* entry : this->entries())
			{
				if (!entry->is_variable() || entry->name().compare(0, prefix.size(), prefix) != 0)
				{
					continue;
				}
				const DebugVariable* variable = static_cast<const DebugVariable*>(entry);
				log.info("  " + variable->name() + " = " + variable->value_text() +
					"   " + variable->help());
				++shown;
			}
			return shown == 0 ? DebugStatus::failure("no variables match '" + prefix + "'") :
				DebugStatus::success();
		}));

	this->add(std::make_unique<LambdaCommand>("help", "list commands, or describe one",
		std::vector<DebugArgSpec>{ { "name", true, [this] { return entry_names(*this); } } },
		[this](const DebugArgs& args, DebugLog& log)
		{
			if (args.has(0))
			{
				const DebugEntry* entry = this->find(args.text(0));
				if (entry == nullptr)
				{
					return DebugStatus::failure("no command or variable '" + args.text(0) + "'");
				}
				log.info(entry->usage());
				return DebugStatus::success("  " + entry->help());
			}
			for (const DebugEntry* entry : this->entries())
			{
				log.info("  " + entry->usage() + "  - " + entry->help());
			}
			return DebugStatus::success("keys: ` console, F10/F11 or pad Back+Start debug menu, "
				"F2 collision, F3 camera, F4 paint, F5 restart, F6 pause, F7 step, "
				"F8 visual, F9 feedback (default binds: 'binds' lists them, 'bind' changes them)");
		}));
}
#endif
