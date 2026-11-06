#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

// The developer tools' one hash and its one spelling. A replay header records
// the content files' FNV-1a (TuneDocument::hash, the director's file hashes),
// the match fingerprint chains it over the state, and every hash a replay or
// command writes is 16 lower-case hex digits. Debug-only in use; it includes
// only the standard library.

// FNV-1a, 64-bit, continued from `hash` (the offset basis by default).
inline std::uint64_t fnv1a(std::string_view bytes, std::uint64_t hash = 0xcbf29ce484222325ull)
{
	for (const char c : bytes)
	{
		hash ^= static_cast<unsigned char>(c);
		hash *= 0x100000001b3ull;
	}
	return hash;
}

// `value` as exactly 16 lower-case hex digits.
inline std::string hex64(std::uint64_t value)
{
	char buffer[20] = {};
	std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value));
	return buffer;
}
