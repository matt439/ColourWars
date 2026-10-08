#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// One number or boolean in a JSON text, with its byte span. `path` is the
// keys from the root joined by '.', array elements written [i]
// ("weapons.sprayer.offset.x", "fonts[2]").
struct JsonLeaf
{
	enum class Kind
	{
		number,
		boolean,
	};

	std::string path;
	Kind kind = Kind::number;
	std::size_t begin = 0;
	std::size_t end = 0;
};

// The numbers and booleans of a JSON text and where each one is, so live
// tuning can name every value a content file holds and replace one token
// without touching a byte around it. Debug-only.
//
// Parsed by rapidjson's SAX reader with the flags the engine's reader uses
// (engine/assets/json.cpp), so it accepts exactly what the game accepts. A
// repeated key resolves to its first occurrence, as rapidjson's FindMember
// does. Strings, nulls and containers carry no span: live tuning edits only
// numbers and booleans (see docs/debug-console.md, "Live tuning"). The engine
// has no in-memory parse and no key enumeration of its own (engine report 12).
class JsonSpanDocument
{
public:
	// Throws std::runtime_error "<name>: line L, column C: <what>" when the
	// text is not JSON.
	JsonSpanDocument(std::string text, std::string name);

	const std::string& text() const { return this->text_; }
	const std::string& name() const { return this->name_; }

	// In file order.
	const std::vector<JsonLeaf>& leaves() const { return this->leaves_; }
	const JsonLeaf* find(std::string_view path) const;
	std::string_view token(const JsonLeaf& leaf) const;

	// The text with exactly that leaf's bytes replaced by `token`.
	std::string replaced(const JsonLeaf& leaf, std::string_view token) const;

	// String leaves seen (not spanned), for the acceptance case.
	std::size_t string_count() const { return this->string_count_; }

private:
	std::string text_;
	std::string name_;
	std::vector<JsonLeaf> leaves_;
	std::size_t string_count_ = 0;
};
