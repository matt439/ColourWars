#include "game/pch.h"
#include "game/content_value.h"

#include <stdexcept>

using namespace labrador;
using namespace mattmath;

namespace
{
	[[noreturn]] void reject(const JsonValue& parent, const char* key, float value,
		const char* rule)
	{
		throw std::runtime_error(parent.where() + ": '" + key + "' must be " + rule +
			", and is " + content_value::number_text(value) + ".");
	}
}

std::string content_value::number_text(float value)
{
	std::string text = std::to_string(value);
	const size_t last = text.find_last_not_of('0');
	if (text.find('.') != std::string::npos && last != std::string::npos)
	{
		text.erase(text[last] == '.' ? last : last + 1);
	}
	return text;
}

float content_value::read_finite(const JsonValue& parent, const char* key)
{
	const float value = parent.number(key);
	if (!std::isfinite(value))
	{
		reject(parent, key, value, "finite");
	}
	return value;
}

float content_value::read_non_negative(const JsonValue& parent, const char* key)
{
	const float value = parent.number(key);
	if (!std::isfinite(value) || !(value >= 0.0f))
	{
		reject(parent, key, value, "finite and not negative");
	}
	return value;
}

float content_value::read_positive(const JsonValue& parent, const char* key)
{
	const float value = parent.number(key);
	if (!std::isfinite(value) || !(value > 0.0f))
	{
		reject(parent, key, value, "finite and greater than zero");
	}
	return value;
}

float content_value::read_number_in(const JsonValue& parent, const char* key,
	float low, float high)
{
	const float value = parent.number(key);
	if (!(value >= low && value <= high))
	{
		throw std::runtime_error(parent.where() + ": '" + key + "' is " +
			number_text(value) + ", which is outside [" +
			number_text(low) + ", " + number_text(high) + "].");
	}
	return value;
}

float content_value::read_unit(const JsonValue& parent, const char* key)
{
	return read_number_in(parent, key, 0.0f, 1.0f);
}

Vector2F content_value::read_vector(const JsonValue& parent, const char* key)
{
	const JsonValue vector = parent.object(key);
	return Vector2F(read_finite(vector, "x"), read_finite(vector, "y"));
}

Vector2F content_value::read_vector_unchecked(const JsonValue& parent, const char* key)
{
	const JsonValue vector = parent.object(key);
	return Vector2F(vector.number("x"), vector.number("y"));
}

Vector2F content_value::read_positive_vector(const JsonValue& parent, const char* key)
{
	const JsonValue vector = parent.object(key);
	return Vector2F(read_positive(vector, "x"), read_positive(vector, "y"));
}
