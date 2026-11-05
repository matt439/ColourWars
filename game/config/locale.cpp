#include "game/pch.h"
#include "game/config/locale.h"
#include "game/config/drawable_text.h"
#include "game/config/environment.h"

#include "engine/core/file_path.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

using namespace labrador;

namespace
{
	constexpr const char* index_file = "locales.json";

	// The base a build without locales.json has: presentation.json's own
	// language, as the shipped index's "base" also names it.
	LocaleEntry base_without_index()
	{
		return { "en", "English", "", false };
	}

	// Full precision, so a number written back out spells the literal it was
	// read from and the engine's own parse of the composed text reads the
	// same float the base file gives it.
	constexpr unsigned parse_flags = rapidjson::kParseFullPrecisionFlag;

	rapidjson::Document parse(std::string_view text, const std::string& source)
	{
		rapidjson::Document document;
		document.Parse<parse_flags>(text.data(), text.size());
		if (document.HasParseError())
		{
			throw std::runtime_error("'" + source + "' is not valid JSON: " +
				rapidjson::GetParseError_En(document.GetParseError()) +
				" (offset " + std::to_string(document.GetErrorOffset()) + ")");
		}
		return document;
	}

	bool starts_with(std::string_view text, std::string_view prefix)
	{
		return text.substr(0, prefix.size()) == prefix;
	}

	std::string read_text(const std::filesystem::path& path, const std::string& source)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			throw std::runtime_error("'" + source + "' cannot be opened.");
		}
		std::string text{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		if (file.bad())
		{
			throw std::runtime_error("'" + source + "' could not be read.");
		}
		// A translator's editor may save a UTF-8 byte order mark; it is not
		// part of the JSON (gap audit L12).
		if (starts_with(text, "\xEF\xBB\xBF"))
		{
			text.erase(0, 3);
		}
		return text;
	}

	bool ends_with(std::string_view text, std::string_view suffix)
	{
		return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
	}

	bool is_comment(std::string_view key)
	{
		return starts_with(key, "_");
	}

	// BCP 47 in the shape this game needs: a two- or three-letter language,
	// then subtags of two to eight letters or digits ("en", "pt-BR",
	// "qps-ploc"). The tag is also the file name, so nothing else is allowed.
	bool is_tag(std::string_view tag)
	{
		size_t start = 0;
		bool first = true;
		while (true)
		{
			const size_t end = std::min(tag.find('-', start), tag.size());
			const std::string_view part = tag.substr(start, end - start);
			if (part.size() < 2 || part.size() > (first ? 3u : 8u))
			{
				return false;
			}
			for (const char c : part)
			{
				const bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
				const bool digit = c >= '0' && c <= '9';
				if (!letter && !(digit && !first))
				{
					return false;
				}
			}
			if (end == tag.size())
			{
				return true;
			}
			start = end + 1;
			first = false;
		}
	}

	// Where a key sits, as Presentation's own diagnostics spell it
	// ("menu_text.title.start", "team_palette.pairs[2].label"), and the same
	// path with indices dropped, which is what the text rule matches.
	struct KeyPath
	{
		std::vector<std::string> parts;
		std::string text;
	};

	KeyPath child_path(const KeyPath& parent, const std::string& key)
	{
		KeyPath path = parent;
		path.parts.push_back(key);
		path.text += (path.text.empty() ? "" : ".") + key;
		return path;
	}

	KeyPath element_path(const KeyPath& parent, size_t index)
	{
		KeyPath path = parent;
		path.parts.push_back("[]");
		path.text += "[" + std::to_string(index) + "]";
		return path;
	}

	// The keys of presentation.json that are copy rather than identifiers,
	// colours, frame or sound names. Everything else is the shell's look and
	// stays the same in every language; a locale that names it is refused.
	bool is_text(const std::vector<std::string>& parts)
	{
		const auto is = [&](std::initializer_list<const char*> shape)
		{
			if (parts.size() != shape.size()) { return false; }
			size_t index = 0;
			for (const char* part : shape)
			{
				if (std::string_view(part) != "*" && parts[index] != part) { return false; }
				++index;
			}
			return true;
		};
		return (parts.size() >= 2 && parts[0] == "menu_text") ||
			is({ "main_menu", "weapon_descriptions", "*" }) ||
			is({ "main_menu", "weapon_select", "choices", "*", "name" }) ||
			is({ "main_menu", "credits", "text" }) ||
			is({ "team_palette", "pairs", "[]", "label" });
	}

	// A key that names a sprite font: "font", "<anything>_font", or any
	// member of a "fonts" object. These are what a locale's "fonts" table
	// substitutes.
	bool is_font_key(const std::string& parent, const std::string& key)
	{
		return key == "font" || ends_with(key, "_font") || parent == "fonts";
	}

	void collect_fonts(const rapidjson::Value& value, const std::string& parent,
		std::set<std::string>& fonts)
	{
		if (value.IsObject())
		{
			for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member)
			{
				const std::string key(member->name.GetString(), member->name.GetStringLength());
				if (is_comment(key)) { continue; }
				if (member->value.IsString() && is_font_key(parent, key))
				{
					fonts.emplace(member->value.GetString(), member->value.GetStringLength());
				}
				collect_fonts(member->value, key, fonts);
			}
		}
		else if (value.IsArray())
		{
			for (const auto& element : value.GetArray())
			{
				collect_fonts(element, parent, fonts);
			}
		}
	}

	void substitute_fonts(rapidjson::Value& value, const std::string& parent,
		const std::map<std::string, std::string>& fonts, rapidjson::Document::AllocatorType& allocator)
	{
		if (value.IsObject())
		{
			for (auto member = value.MemberBegin(); member != value.MemberEnd(); ++member)
			{
				const std::string key(member->name.GetString(), member->name.GetStringLength());
				if (is_comment(key)) { continue; }
				if (member->value.IsString() && is_font_key(parent, key))
				{
					const auto found = fonts.find(std::string(member->value.GetString(),
						member->value.GetStringLength()));
					if (found != fonts.end())
					{
						member->value.SetString(found->second.c_str(),
							static_cast<rapidjson::SizeType>(found->second.size()), allocator);
					}
					continue;
				}
				substitute_fonts(member->value, key, fonts, allocator);
			}
		}
		else if (value.IsArray())
		{
			for (auto& element : value.GetArray())
			{
				substitute_fonts(element, parent, fonts, allocator);
			}
		}
	}

	const char* type_name(const rapidjson::Value& value)
	{
		if (value.IsObject()) { return "an object"; }
		if (value.IsArray()) { return "an array"; }
		if (value.IsString()) { return "text"; }
		if (value.IsNumber()) { return "a number"; }
		if (value.IsBool()) { return "true or false"; }
		return "null";
	}

	// Lays one locale's "presentation" tree over the composed document. The
	// overlay may only name what presentation.json has, with the same shape,
	// and may only replace text. An array is matched element by element; a
	// null element leaves that element as it is.
	void overlay(rapidjson::Value& base, const rapidjson::Value& over, const KeyPath& path,
		const std::string& source, rapidjson::Document::AllocatorType& allocator)
	{
		const std::string where = "'" + source + "': presentation" +
			(path.text.empty() ? std::string() : "." + path.text);
		if (over.IsObject())
		{
			if (!base.IsObject())
			{
				throw std::runtime_error(where + " is an object, but presentation.json has " +
					type_name(base) + " there.");
			}
			for (auto member = over.MemberBegin(); member != over.MemberEnd(); ++member)
			{
				const std::string key(member->name.GetString(), member->name.GetStringLength());
				if (is_comment(key)) { continue; }
				const auto target = base.FindMember(member->name);
				if (target == base.MemberEnd())
				{
					throw std::runtime_error(where + " has '" + key +
						"', which presentation.json does not.");
				}
				overlay(target->value, member->value, child_path(path, key), source, allocator);
			}
			return;
		}
		if (over.IsArray())
		{
			if (!base.IsArray() || base.Size() != over.Size())
			{
				throw std::runtime_error(where + " must be an array of the same length as "
					"presentation.json's (null keeps an element).");
			}
			for (rapidjson::SizeType index = 0; index < over.Size(); ++index)
			{
				if (over[index].IsNull()) { continue; }
				overlay(base[index], over[index], element_path(path, index), source, allocator);
			}
			return;
		}
		if (!over.IsString())
		{
			throw std::runtime_error(where + " is " + type_name(over) +
				"; a locale changes only text.");
		}
		if (!base.IsString())
		{
			throw std::runtime_error(where + " is text, but presentation.json has " +
				type_name(base) + " there.");
		}
		if (!is_text(path.parts))
		{
			throw std::runtime_error(where + " is not copy; a locale changes only text "
				"(menu_text, weapon names and descriptions, team pair labels and the credits).");
		}
		base.SetString(over.GetString(), over.GetStringLength(), allocator);
	}

	struct LocaleFile
	{
		std::string source;
		rapidjson::Document document;
		std::map<std::string, std::string> fonts;
	};

	LocaleFile read_locale_file(const std::filesystem::path& path)
	{
		LocaleFile file;
		file.source = path_to_utf8(path);
		file.document = parse(read_text(path, file.source), file.source);
		const rapidjson::Document& root = file.document;
		if (!root.IsObject())
		{
			throw std::runtime_error("'" + file.source + "' must be a JSON object.");
		}
		for (auto member = root.MemberBegin(); member != root.MemberEnd(); ++member)
		{
			const std::string key(member->name.GetString(), member->name.GetStringLength());
			if (is_comment(key) || key == "presentation" || key == "fonts") { continue; }
			throw std::runtime_error("'" + file.source + "' has '" + key +
				"'; a locale file has only \"presentation\" and \"fonts\".");
		}
		const auto presentation = root.FindMember("presentation");
		if (presentation == root.MemberEnd() || !presentation->value.IsObject())
		{
			throw std::runtime_error("'" + file.source + "': presentation must be an object.");
		}
		const auto fonts = root.FindMember("fonts");
		if (fonts != root.MemberEnd())
		{
			if (!fonts->value.IsObject())
			{
				throw std::runtime_error("'" + file.source + "': fonts must be an object.");
			}
			for (auto font = fonts->value.MemberBegin(); font != fonts->value.MemberEnd(); ++font)
			{
				const std::string from(font->name.GetString(), font->name.GetStringLength());
				if (is_comment(from)) { continue; }
				if (!font->value.IsString() || font->value.GetStringLength() == 0)
				{
					throw std::runtime_error("'" + file.source + "': fonts." + from +
						" must name a sprite font.");
				}
				file.fonts[from] = std::string(font->value.GetString(), font->value.GetStringLength());
			}
		}
		return file;
	}

	std::string required_string(const rapidjson::Value& object, const char* key,
		const std::string& where)
	{
		const auto member = object.FindMember(key);
		if (member == object.MemberEnd() || !member->value.IsString())
		{
			throw std::runtime_error(where + "." + key + " must be text.");
		}
		return std::string(member->value.GetString(), member->value.GetStringLength());
	}

	LocaleEntry read_entry(const rapidjson::Value& value, const std::string& where, bool base)
	{
		if (!value.IsObject())
		{
			throw std::runtime_error(where + " must be an object.");
		}
		LocaleEntry entry;
		entry.tag = required_string(value, "tag", where);
		if (!is_tag(entry.tag))
		{
			throw std::runtime_error(where + ".tag '" + entry.tag +
				"' is not a language tag such as en, pt-BR or qps-ploc.");
		}
		entry.name = required_string(value, "name", where);
		if (entry.name.empty())
		{
			throw std::runtime_error(where + ".name must not be empty.");
		}
		drawable_text::widen_drawable(entry.name, where + ".name");
		const auto fallback = value.FindMember("fallback");
		const auto test = value.FindMember("test");
		if (base)
		{
			if (fallback != value.MemberEnd() || test != value.MemberEnd())
			{
				throw std::runtime_error(where + " is presentation.json itself; it has no "
					"fallback and is not a test locale.");
			}
			return entry;
		}
		if (fallback != value.MemberEnd())
		{
			if (!fallback->value.IsString())
			{
				throw std::runtime_error(where + ".fallback must be a listed tag.");
			}
			entry.fallback.assign(fallback->value.GetString(), fallback->value.GetStringLength());
		}
		if (test != value.MemberEnd())
		{
			if (!test->value.IsBool())
			{
				throw std::runtime_error(where + ".test must be true or false.");
			}
			entry.test = test->value.GetBool();
		}
		return entry;
	}
}

struct Locales::Impl
{
	std::vector<LocaleEntry> entries;
	// One per entry; the base's is empty.
	std::vector<std::unique_ptr<LocaleFile>> files;
};

Locales::Locales() : impl_(std::make_unique<Impl>())
{
	this->impl_->entries.push_back(base_without_index());
	this->impl_->files.push_back(nullptr);
}

Locales::~Locales() = default;
Locales::Locales(Locales&&) noexcept = default;
Locales& Locales::operator=(Locales&&) noexcept = default;

Locales Locales::read(const std::string& directory)
{
	const std::filesystem::path folder = path_from_utf8(directory);
	const std::filesystem::path index_path = folder / index_file;
	std::error_code error;
	if (!std::filesystem::exists(index_path, error))
	{
		return Locales();
	}

	const std::string source = path_to_utf8(index_path);
	const rapidjson::Document index = parse(read_text(index_path, source), source);
	const std::string where = "'" + source + "': ";
	if (!index.IsObject())
	{
		throw std::runtime_error("'" + source + "' must be a JSON object.");
	}
	const auto base = index.FindMember("base");
	const auto list = index.FindMember("locales");
	if (base == index.MemberEnd())
	{
		throw std::runtime_error(where + "base is missing.");
	}
	if (list == index.MemberEnd() || !list->value.IsArray())
	{
		throw std::runtime_error(where + "locales must be an array.");
	}

	Locales result;
	Impl& impl = *result.impl_;
	impl.entries.clear();
	impl.files.clear();
	impl.entries.push_back(read_entry(base->value, where + "base", true));
	impl.files.push_back(nullptr);

	for (rapidjson::SizeType index_number = 0; index_number < list->value.Size(); ++index_number)
	{
		const std::string at = where + "locales[" + std::to_string(index_number) + "]";
		LocaleEntry entry = read_entry(list->value[index_number], at, false);
		if (result.find(entry.tag) != nullptr)
		{
			throw std::runtime_error(at + ".tag '" + entry.tag + "' is listed twice.");
		}
		if (entry.fallback.empty())
		{
			entry.fallback = result.base().tag;
		}
		// Only the base or a locale listed above: the chain always ends, and
		// reads top to bottom.
		if (result.find(entry.fallback) == nullptr)
		{
			throw std::runtime_error(at + ".fallback '" + entry.fallback +
				"' must be the base or a locale listed above it.");
		}
		impl.files.push_back(std::make_unique<LocaleFile>(
			read_locale_file(folder / (entry.tag + ".json"))));
		impl.entries.push_back(std::move(entry));
	}
	return result;
}

const std::vector<LocaleEntry>& Locales::entries() const
{
	return this->impl_->entries;
}

const LocaleEntry& Locales::base() const
{
	return this->impl_->entries.front();
}

const LocaleEntry* Locales::find(std::string_view tag) const
{
	for (const LocaleEntry& entry : this->impl_->entries)
	{
		if (entry.tag == tag) { return &entry; }
	}
	return nullptr;
}

std::vector<const LocaleEntry*> Locales::chain(std::string_view tag) const
{
	std::vector<const LocaleEntry*> result;
	const LocaleEntry* entry = this->find(tag);
	if (entry == nullptr)
	{
		throw std::invalid_argument("'" + std::string(tag) + "' is not a listed locale.");
	}
	while (entry != nullptr)
	{
		result.push_back(entry);
		entry = entry->fallback.empty() ? nullptr : this->find(entry->fallback);
	}
	return result;
}

std::string Locales::compose(std::string_view tag, std::string_view base_text,
	const std::string& base_name) const
{
	const std::vector<const LocaleEntry*> chain = this->chain(tag);
	if (chain.front() == &this->base())
	{
		return std::string(base_text);
	}

	rapidjson::Document document = parse(base_text, base_name);
	if (!document.IsObject())
	{
		throw std::runtime_error("'" + base_name + "' must be a JSON object.");
	}
	std::set<std::string> base_fonts;
	collect_fonts(document, "", base_fonts);

	// Furthest fallback first, so the nearer locale wins each string and font.
	std::map<std::string, std::string> fonts;
	for (auto entry = chain.rbegin(); entry != chain.rend(); ++entry)
	{
		const size_t position = static_cast<size_t>(*entry - this->impl_->entries.data());
		const LocaleFile* file = this->impl_->files[position].get();
		if (file == nullptr) { continue; }
		overlay(document, file->document["presentation"], KeyPath{}, file->source,
			document.GetAllocator());
		for (const auto& [from, to] : file->fonts)
		{
			if (base_fonts.count(from) == 0)
			{
				throw std::runtime_error("'" + file->source + "': fonts." + from +
					" is not a font presentation.json uses.");
			}
			fonts[from] = to;
		}
	}
	substitute_fonts(document, "", fonts, document.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	document.Accept(writer);
	return std::string(buffer.GetString(), buffer.GetSize());
}

JsonDocument Locales::presentation(std::string_view tag, std::string_view base_text,
	const std::string& base_name) const
{
	const std::string text = this->compose(tag, base_text, base_name);
	const bool base = tag == this->base().tag;
	return parse_json(text, base ? base_name : base_name + " (locale " + std::string(tag) + ")");
}

JsonDocument Locales::presentation(std::string_view tag, const std::string& base_path) const
{
	if (tag == this->base().tag)
	{
		// Byte for byte the file, through the engine's own reader.
		return read_json_file(base_path.c_str());
	}
	return this->presentation(tag, read_text(path_from_utf8(base_path), base_path), base_path);
}

void Locales::check_structure(std::string_view base_text, const std::string& base_name) const
{
	for (const LocaleEntry& entry : this->impl_->entries)
	{
		std::ignore = this->compose(entry.tag, base_text, base_name);
	}
}

void Locales::check_structure(const std::string& base_path) const
{
	if (this->impl_->entries.size() > 1)
	{
		this->check_structure(read_text(path_from_utf8(base_path), base_path), base_path);
	}
}

ActiveLocale::ActiveLocale(Locales locales, std::string_view requested) :
	locales_(std::move(locales))
{
	const LocaleEntry* entry = requested.empty() ? nullptr : this->locales_.find(requested);
	if (!requested.empty() && entry == nullptr)
	{
		std::fprintf(stderr, "locale: '%.*s' is not listed in locales/locales.json; "
			"using '%s'.\n", static_cast<int>(requested.size()), requested.data(),
			this->locales_.base().tag.c_str());
	}
	this->tag_ = entry != nullptr ? entry->tag : this->locales_.base().tag;
}

const Locales& ActiveLocale::locales() const
{
	return this->locales_;
}

const LocaleEntry& ActiveLocale::entry() const
{
	return *this->locales_.find(this->tag_);
}

const std::string& ActiveLocale::tag() const
{
	return this->tag_;
}

bool ActiveLocale::is_base() const
{
	return this->tag_ == this->locales_.base().tag;
}

JsonDocument ActiveLocale::presentation(const std::string& base_path) const
{
	return this->locales_.presentation(this->tag_, base_path);
}

JsonDocument ActiveLocale::presentation(std::string_view base_text, const std::string& base_name) const
{
	return this->locales_.presentation(this->tag_, base_text, base_name);
}

std::string locale_selection::requested_from_environment()
{
	return environment_variable("COLOURWARS_LOCALE");
}
