#pragma once

#include <string>

namespace labrador { class JsonValue; }
struct SaveData;

// Streams the save file's JSON. The RapidJSON writer behind it stays in
// save.cpp, so a section names only what it writes.
class SaveWriter
{
public:
	virtual ~SaveWriter() = default;
	virtual void key(const char* name) = 0;
	virtual void integer(int value) = 0;
	virtual void number(float value) = 0;
	virtual void boolean(bool value) = 0;
	virtual void string(const std::string& value) = 0;
	virtual void begin_object() = 0;
	virtual void end_object() = 0;
	virtual void begin_array() = 0;
	virtual void end_array() = 0;
};

// One self-contained part of the save. Save reads every section from the same
// root object and catches each on its own, so a malformed section falls back
// to its defaults and leaves the others' values in place.
class SaveSection
{
public:
	virtual ~SaveSection() = default;
	// Names the section in diagnostics. For a section stored under one nested
	// root member it is that member's key; the original preferences keep
	// their flat root keys and use a descriptive name.
	virtual const char* key() const = 0;
	// Restores this section's defaults in `data`, after a failed read.
	virtual void reset(SaveData& data) const = 0;
	// Reads what this section owns from the save's root object. Recovers each
	// field independently where it can; anything it throws resets the section.
	virtual void read(const labrador::JsonValue& root, SaveData& data) const = 0;
	// Writes this section's root members (keys and values, not the root's braces).
	virtual void write(SaveWriter& writer, const SaveData& data) const = 0;
};
