#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_script.h"
#include "game/debug/debug_tokenizer.h"

std::string DebugScriptStatement::where() const
{
	return this->line > 0 ? this->origin + ":" + std::to_string(this->line) : this->origin;
}

namespace debug_script
{
	bool file_name(std::string_view name, std::string& file)
	{
		std::string_view stem = name;
		if (stem.size() > 4 && stem.substr(stem.size() - 4) == ".cfg")
		{
			stem.remove_suffix(4);
		}
		if (stem.empty() || stem.size() > 64)
		{
			return false;
		}
		for (const char c : stem)
		{
			const bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') || c == '_' || c == '-';
			if (!allowed)
			{
				return false;
			}
		}
		file = std::string(stem) + ".cfg";
		return true;
	}

	std::string strip_comment(std::string_view line)
	{
		bool quoted = false;
		for (std::size_t i = 0; i < line.size(); ++i)
		{
			const char c = line[i];
			if (quoted && c == '\\' && i + 1 < line.size() && line[i + 1] == '"')
			{
				++i;
				continue;
			}
			if (c == '"')
			{
				quoted = !quoted;
			}
			else if (!quoted && c == '/' && i + 1 < line.size() && line[i + 1] == '/')
			{
				return std::string(line.substr(0, i));
			}
		}
		return std::string(line);
	}
}

namespace
{
	bool safe_argument(const std::string& argument)
	{
		return !argument.empty() && argument.find_first_of(";\" \t") == std::string::npos &&
			argument.find("//") == std::string::npos;
	}
}

ParsedScript parse_script(std::string_view origin, std::string_view text,
	const std::vector<std::string>& args)
{
	ParsedScript result;
	const std::string name(origin);
	const auto fail = [&result](std::string error)
		{
			result.ok = false;
			result.error = std::move(error);
			result.statements.clear();
			return result;
		};

	if (text.size() > debug_script::max_bytes)
	{
		return fail(name + ": " + std::to_string(text.size()) + " bytes; a script is at most " +
			std::to_string(debug_script::max_bytes));
	}
	for (std::size_t i = 0; i < args.size(); ++i)
	{
		if (!safe_argument(args[i]))
		{
			return fail(name + ": argument $" + std::to_string(i + 1) + " '" + args[i] +
				"' may not be empty or hold ';', '\"', '//' or spaces");
		}
	}
	if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
		static_cast<unsigned char>(text[1]) == 0xBB && static_cast<unsigned char>(text[2]) == 0xBF)
	{
		text.remove_prefix(3);
	}

	int line_number = 0;
	std::size_t start = 0;
	while (start <= text.size())
	{
		const std::size_t end = text.find('\n', start);
		std::string_view raw = text.substr(start, end == std::string_view::npos ?
			std::string_view::npos : end - start);
		start = end == std::string_view::npos ? text.size() + 1 : end + 1;
		++line_number;
		if (static_cast<std::size_t>(line_number) > debug_script::max_lines)
		{
			return fail(name + ": more than " + std::to_string(debug_script::max_lines) + " lines");
		}
		if (!raw.empty() && raw.back() == '\r')
		{
			raw.remove_suffix(1);
		}
		const std::string where = name + ":" + std::to_string(line_number);
		const std::string uncommented = debug_script::strip_comment(raw);

		// $1..$9, then $$; an argument the caller did not give is an error.
		std::string line;
		for (std::size_t i = 0; i < uncommented.size(); ++i)
		{
			const char c = uncommented[i];
			if (c != '$' || i + 1 >= uncommented.size())
			{
				line += c;
				continue;
			}
			const char next = uncommented[i + 1];
			if (next == '$')
			{
				line += '$';
				++i;
			}
			else if (next >= '1' && next <= '9')
			{
				const std::size_t index = static_cast<std::size_t>(next - '1');
				if (index >= args.size())
				{
					return fail(where + ": $" + std::string(1, next) + " has no argument (" +
						std::to_string(args.size()) + " given)");
				}
				line += args[index];
				++i;
			}
			else
			{
				line += c;
			}
		}

		for (const std::string& statement : split_statements(line))
		{
			const DebugTokens tokens = tokenize(statement);
			if (!tokens.ok)
			{
				return fail(where + ": " + tokens.error);
			}
			if (tokens.words.empty())
			{
				continue;
			}
			const std::size_t first = statement.find_first_not_of(" \t");
			const std::size_t last = statement.find_last_not_of(" \t");
			result.statements.push_back({ name, line_number,
				statement.substr(first, last - first + 1) });
		}
	}
	return result;
}
#endif
