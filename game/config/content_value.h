#pragma once

#include "engine/assets/json.h"
#include "engine/math/vector2f.h"

#include <string>

// Range-checked reads of authored numbers, shared by every content reader
// (tuning.json, presentation.json and the level definitions). JsonValue
// already fails loudly on a missing key or a wrong type; these add the value
// check, so a number outside what the code can use comes back as a sentence
// naming the file, the object and the key, instead of a divide by zero three
// screens later.
//
// Bounds state what the code needs or what a field means (a delay cannot be
// negative, a gain is 0..1). They never fence in today's balance: tuning is
// provisional, and a bound that only records the current number is a rebuild
// waiting to happen. A rule that compares two fields is written where both are
// read, at the end of that reader.
//
// Every comparison is written so a non-finite value fails it. JSON has no NaN,
// but a literal past float's range (1e100) arrives as infinity.
namespace content_value
{
	// The value as a diagnostic shows it: without trailing decimal zeroes.
	std::string number_text(float value);

	float read_finite(const labrador::JsonValue& parent, const char* key);
	// Zero is allowed: a delay of zero skips the wait.
	float read_non_negative(const labrador::JsonValue& parent, const char* key);
	float read_positive(const labrador::JsonValue& parent, const char* key);
	// Inclusive at both ends.
	float read_number_in(const labrador::JsonValue& parent, const char* key,
		float low, float high);
	// A gain, fraction or threshold: [0, 1].
	float read_unit(const labrador::JsonValue& parent, const char* key);

	// An {"x", "y"} object. Both components must be finite.
	mattmath::Vector2F read_vector(const labrador::JsonValue& parent, const char* key);
	// The same object as authored, with no check here. For a position or
	// offset a later layout rule checks, so the failure names the screen or
	// text it breaks rather than only the number.
	mattmath::Vector2F read_vector_unchecked(const labrador::JsonValue& parent,
		const char* key);
	// Both components finite and greater than zero: an extent or a cap.
	mattmath::Vector2F read_positive_vector(const labrador::JsonValue& parent,
		const char* key);
}
