#pragma once

#include <cstddef>
#include <string_view>

// The one rule for a name the developer tools put in a path: an ASCII stem or
// directory component of [A-Za-z0-9_-], never a reserved Windows device name.
// Callers that refuse a bad name ask valid_stem; callers that repair one
// replace each character stem_character refuses with '_' and then prefix a
// reserved_stem with '_'.
namespace debug_file_name
{
	// The longest stem a refusing caller (exec, perf.capture) accepts.
	inline constexpr std::size_t max_stem = 64;

	inline bool stem_character(char value)
	{
		return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
			(value >= '0' && value <= '9') || value == '_' || value == '-';
	}

	// CON, PRN, AUX, NUL, COM1-9 and LPT1-9, in any case.
	inline bool reserved_stem(std::string_view segment)
	{
		if (segment.size() != 3 && segment.size() != 4) return false;
		const auto upper = [](char value)
		{
			return value >= 'a' && value <= 'z' ? static_cast<char>(value - 'a' + 'A') : value;
		};
		const char prefix[] = { upper(segment[0]), upper(segment[1]), upper(segment[2]) };
		const std::string_view stem(prefix, 3);
		return (segment.size() == 3 && (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL")) ||
			(segment.size() == 4 && (stem == "COM" || stem == "LPT") && segment[3] >= '1' && segment[3] <= '9');
	}

	// 1-max_stem stem characters and not a reserved name.
	inline bool valid_stem(std::string_view stem)
	{
		if (stem.empty() || stem.size() > max_stem || reserved_stem(stem)) return false;
		for (const char value : stem)
		{
			if (!stem_character(value)) return false;
		}
		return true;
	}
}
