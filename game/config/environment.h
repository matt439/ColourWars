#pragma once

#include <cstdlib>
#include <memory>
#include <string>

// The value of an environment variable, empty when it is unset or empty.
// _dupenv_s rather than getenv, which MSVC deprecates as unsafe.
inline std::string environment_variable(const char* name)
{
	char* raw = nullptr;
	size_t length = 0;
	if (_dupenv_s(&raw, &length, name) != 0 || raw == nullptr) { return {}; }
	const std::unique_ptr<char, decltype(&std::free)> value(raw, &std::free);
	return std::string(value.get());
}
