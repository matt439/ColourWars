#include "game/pch.h"
#include "game/debug/dev_tools.h"
#include "game/debug/debug_tokenizer.h"

namespace
{
	bool starts_statement(const std::wstring& argument)
	{
		return argument.size() >= 2 && argument[0] == L'+' &&
			((argument[1] >= L'a' && argument[1] <= L'z') ||
				(argument[1] >= L'A' && argument[1] <= L'Z'));
	}
}

// Release reads launch arguments only to say they are ignored.
bool has_launch_statements(const std::vector<std::wstring>& arguments)
{
	return std::any_of(arguments.begin(), arguments.end(), starts_statement);
}

#if COLOURWARS_DEV_TOOLS
namespace
{
	// UTF-16 to UTF-8; the tokenizer then rejects anything outside ASCII.
	std::string narrow(const std::wstring& text)
	{
		if (text.empty())
		{
			return {};
		}
		const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(),
			static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		std::string result(static_cast<std::size_t>(std::max(size, 0)), ' ');
		if (size > 0)
		{
			WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
				result.data(), size, nullptr, nullptr);
		}
		return result;
	}
}

DebugTokens tokenize(std::string_view statement)
{
	DebugTokens result;
	std::string word;
	bool in_word = false;
	bool quoted = false;

	for (std::size_t i = 0; i < statement.size(); ++i)
	{
		const char c = statement[i];
		const unsigned char byte = static_cast<unsigned char>(c);
		if (byte < 0x20 || byte > 0x7e)
		{
			if (c == '\t' && !quoted)
			{
				// Tab separates like a space.
			}
			else
			{
				result.ok = false;
				result.error = "only printable ASCII is accepted";
				result.words.clear();
				return result;
			}
		}

		if (quoted)
		{
			if (c == '\\' && i + 1 < statement.size() && statement[i + 1] == '"')
			{
				word += '"';
				++i;
			}
			else if (c == '"')
			{
				quoted = false;
			}
			else
			{
				word += c;
			}
			continue;
		}

		if (c == ' ' || c == '\t')
		{
			if (in_word)
			{
				result.words.push_back(word);
				word.clear();
				in_word = false;
			}
		}
		else if (c == '"')
		{
			quoted = true;
			in_word = true;
		}
		else
		{
			word += c;
			in_word = true;
		}
	}

	if (quoted)
	{
		result.ok = false;
		result.error = "unterminated quote";
		result.words.clear();
		return result;
	}
	if (in_word)
	{
		result.words.push_back(word);
	}
	return result;
}

std::vector<std::string> split_statements(std::string_view line)
{
	std::vector<std::string> statements;
	std::string current;
	bool quoted = false;
	for (std::size_t i = 0; i < line.size(); ++i)
	{
		const char c = line[i];
		if (quoted && c == '\\' && i + 1 < line.size() && line[i + 1] == '"')
		{
			current += c;
			current += line[++i];
			continue;
		}
		if (c == '"')
		{
			quoted = !quoted;
		}
		if (c == ';' && !quoted)
		{
			statements.push_back(current);
			current.clear();
			continue;
		}
		current += c;
	}
	statements.push_back(current);

	std::erase_if(statements, [](const std::string& statement)
		{
			return statement.find_first_not_of(" \t") == std::string::npos;
		});
	return statements;
}

std::string quote_word(const std::string& word)
{
	if (!word.empty() && word.find_first_of(" \t\";") == std::string::npos)
	{
		return word;
	}
	std::string quoted = "\"";
	for (const char c : word)
	{
		if (c == '"')
		{
			quoted += '\\';
		}
		quoted += c;
	}
	quoted += '"';
	return quoted;
}

std::vector<std::string> parse_launch_arguments(
	const std::vector<std::wstring>& arguments)
{
	std::vector<std::string> statements;
	bool in_statement = false;
	for (const std::wstring& argument : arguments)
	{
		if (starts_statement(argument))
		{
			statements.push_back(narrow(argument.substr(1)));
			in_statement = true;
		}
		else if (in_statement)
		{
			statements.back() += ' ';
			statements.back() += quote_word(narrow(argument));
		}
	}
	return statements;
}
#endif
