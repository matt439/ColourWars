#pragma once

// Engine-free rules for a player's own stage file (ROAD#16), shared by the
// game's folder scan (game/stage_loading.cpp, directory_consts) and the level
// editor's Save to My Stages, so the editor refuses exactly the names the
// game would. Standard library only.
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace user_stage_contract
{
	// The per-user folder under %LOCALAPPDATA%: ColourWars\stages, beside the
	// save. Keep the game folder's name stable so existing saves stay reachable.
	inline constexpr char game_folder_name[] = "ColourWars";
	inline constexpr char stage_folder_name[] = "stages";

	inline std::filesystem::path folder_under(const std::filesystem::path& local_app_data)
	{
		return local_app_data / game_folder_name / stage_folder_name;
	}

	// The file name, without .json, is the stage's id: 1 to 64 ASCII letters,
	// digits, '_' or '-', not starting with '-' (the registration helper's
	// character rule, so the file can later be registered unchanged). Windows
	// reserved device stems are refused in every ASCII spelling.
	inline constexpr std::size_t max_name_length = 64;

	inline bool usable_name(std::string_view name)
	{
		if (name.empty() || name.size() > max_name_length || name.front() == '-')
		{
			return false;
		}
		if (!std::all_of(name.begin(), name.end(), [](char character)
		{
			const unsigned char value = static_cast<unsigned char>(character);
			return value < 0x80 && (std::isalnum(value) || character == '_' || character == '-');
		}))
		{
			return false;
		}
		if (name.size() == 3 || name.size() == 4)
		{
			const auto ascii_upper = [](char character)
			{
				return character >= 'a' && character <= 'z'
					? static_cast<char>(character - 'a' + 'A') : character;
			};
			const char prefix[] = {ascii_upper(name[0]), ascii_upper(name[1]), ascii_upper(name[2])};
			const std::string_view stem(prefix, 3);
			if (name.size() == 3 && (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL"))
			{
				return false;
			}
			if (name.size() == 4 && (stem == "COM" || stem == "LPT") && name[3] >= '1' && name[3] <= '9')
			{
				return false;
			}
		}
		return true;
	}

	// What a refusal says about a name usable_name rejects.
	inline std::string name_rule()
	{
		return "the file name must be 1 to " + std::to_string(max_name_length) +
			" ASCII letters, digits, '_' or '-', not starting with '-', and must not be a reserved Windows name"
			" (CON, PRN, AUX, NUL, COM1-COM9 or LPT1-LPT9)";
	}

	// Stage ids collide without case: a user stage cannot take a shipped
	// stage's name, or another user stage's, in any spelling.
	inline bool same_name(std::string_view first, std::string_view second)
	{
		return first.size() == second.size() && std::equal(first.begin(), first.end(),
			second.begin(), [](char a, char b)
			{
				return std::tolower(static_cast<unsigned char>(a)) ==
					std::tolower(static_cast<unsigned char>(b));
			});
	}
}
