#pragma once

#include "engine/assets/json.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

// The locale axis (GAP#L11): which language the shell's copy is in.
//
// presentation.json is the base locale and holds every string. A locale is a
// partial copy of presentation.json's text, kept beside it in
// locales/<tag>.json and listed in locales/locales.json; at startup the chosen
// locale's file, and each locale it falls back to, are laid over the base
// before Presentation reads it. Everything Presentation already checks - the
// tokens a template must carry, that every character is in the font that
// draws it, that the composed line fits its box - then judges the translation
// exactly as it judges the shipped English. docs/localisation.md is the
// contract: the file format, what a locale may and may not change, fallback,
// font coverage and numeric conventions.
struct LocaleEntry
{
	// A BCP 47 language tag ("fr", "pt-BR", "qps-ploc"); the file is <tag>.json.
	std::string tag;
	// The language's name in itself, as a language picker would show it.
	std::string name;
	// The locale a missing string comes from: the base, or one listed above
	// this one. Empty only for the base.
	std::string fallback;
	// A test locale (pseudo-localisation) is selectable by COLOURWARS_LOCALE
	// but never offered to players.
	bool test = false;
};

class Locales
{
public:
	// Only the base locale, as when a build has no locales/locales.json.
	Locales();
	~Locales();
	Locales(Locales&&) noexcept;
	Locales& operator=(Locales&&) noexcept;

	// Reads `directory`/locales.json and every file it lists (UTF-8 path).
	// No index is not an error: the build has only the base locale. A broken
	// index or locale file throws naming the file and key.
	static Locales read(const std::string& directory);

	// The base locale first, then the index's locales in order.
	const std::vector<LocaleEntry>& entries() const;
	const LocaleEntry& base() const;
	// Exact, case-sensitive tag; nullptr when not listed.
	const LocaleEntry* find(std::string_view tag) const;
	// `tag`, then what it falls back to, ending with the base. Throws
	// invalid_argument for a tag that is not listed.
	std::vector<const LocaleEntry*> chain(std::string_view tag) const;

	// presentation.json's text with `tag`'s chain laid over it, as JSON text.
	// Throws runtime_error naming the locale file and key when a locale
	// names a key presentation.json lacks, replaces something that is not
	// text, or changes a JSON type. The base tag returns `base_text` itself.
	std::string compose(std::string_view tag, std::string_view base_text,
		const std::string& base_name) const;
	// The composed document, named "<base_name> (locale <tag>)" so every
	// Presentation diagnostic says which locale it was reading; the base tag
	// keeps `base_name`.
	labrador::JsonDocument presentation(std::string_view tag, std::string_view base_text,
		const std::string& base_name) const;
	labrador::JsonDocument presentation(std::string_view tag, const std::string& base_path) const;

	// Composes every listed locale over `base_text` and throws the first
	// structural problem. Text, fonts and fit are Presentation's to judge, per
	// locale; the locales acceptance case does that for all of them.
	void check_structure(std::string_view base_text, const std::string& base_name) const;
	void check_structure(const std::string& base_path) const;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

// The locale one run uses: the requested tag when it is listed, otherwise the
// base, with one stderr line naming the unknown tag.
class ActiveLocale
{
public:
	ActiveLocale(Locales locales, std::string_view requested);

	const Locales& locales() const;
	const LocaleEntry& entry() const;
	const std::string& tag() const;
	bool is_base() const;

	// The active locale's presentation document, as Locales::presentation.
	labrador::JsonDocument presentation(const std::string& base_path) const;
	labrador::JsonDocument presentation(std::string_view base_text,
		const std::string& base_name) const;

private:
	Locales locales_;
	std::string tag_;
};

namespace locale_selection
{
	// The COLOURWARS_LOCALE environment variable, or empty for the base.
	// A player-facing language choice will join it here; until a second
	// shipped language exists, this is how a locale is chosen.
	std::string requested_from_environment();
}
