#pragma once

#include <string_view>

// Screenshot callers supply one sanitized ASCII stem or directory component.
namespace debug_file_name
{
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
}
