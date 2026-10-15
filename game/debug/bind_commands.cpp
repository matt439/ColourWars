#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_files.h"
#include "game/debug/debug_keys.h"
#include "game/debug/debug_script.h"
#include "game/debug/debug_tokenizer.h"
#include <cctype>

// Phase 4 binds: bind, unbind, binds and binds.reset, and binds.cfg in the
// debug user directory (only the differences from the default binds).

namespace
{
	using Form = DebugMenuTraits::Form;

	DebugMenuTraits traits(const char* category, Form form)
	{
		DebugMenuTraits result;
		result.category = category;
		result.form = form;
		return result;
	}

	std::string upper(std::string text)
	{
		for (char& c : text)
		{
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		}
		return text;
	}
}

// ---------------------------------------------------------------------------
// binds.cfg

void DebugDirectorImpl::load_binds(const std::filesystem::path& directory)
{
	const std::filesystem::path path = directory / L"binds.cfg";
	std::error_code ignored;
	if (!std::filesystem::is_regular_file(path, ignored))
	{
		return;
	}
	bool too_big = false;
	const std::string text = debug_files::read_file(path, debug_script::max_bytes, too_big);
	const ParsedScript parsed = parse_script("binds.cfg", text, {});
	if (too_big || !parsed.ok)
	{
		// The next bind would overwrite it: keep the unreadable file aside.
		const std::filesystem::path aside = directory / L"binds.cfg.bad";
		std::error_code moved;
		std::filesystem::rename(path, aside, moved);
		this->log_.warning("binds: " + (too_big ? std::string("binds.cfg is too large") :
			parsed.error) + "; keeping the default keys" + (moved ? std::string() :
			" (the file is kept as " + labrador::path_to_utf8(aside) + ")"));
		return;
	}
	// Each line runs through the registry, quietly: only a failure is news.
	DebugLog quiet;
	quiet.set_mirror_to_debugger(false);
	this->binds_loading_ = true;
	int applied = 0;
	for (const DebugScriptStatement& statement : parsed.statements)
	{
		const DebugTokens tokens = tokenize(statement.text);
		const std::string head = tokens.words.empty() ? std::string() : tokens.words.front();
		if (head != "bind" && head != "unbind" && head != "binds.reset")
		{
			this->log_.warning("binds: " + statement.where() + ": only bind and unbind lines "
				"are read");
			continue;
		}
		const DebugStatus status = this->registry_.execute(statement.text, quiet);
		if (!status.ok)
		{
			this->log_.warning("binds: " + statement.where() + ": " +
				(quiet.lines().empty() ? std::string("failed") : quiet.lines().back().text));
		}
		else
		{
			++applied;
		}
	}
	this->binds_loading_ = false;
	this->log_.info("binds: " + std::to_string(applied) + " change(s) from " +
		labrador::path_to_utf8(path));
}

void DebugDirectorImpl::save_binds()
{
	if (this->binds_loading_)
	{
		return;
	}
	const std::optional<std::filesystem::path> directory = this->user_directory();
	if (!directory)
	{
		return;
	}
	const std::filesystem::path path = *directory / L"binds.cfg";
	const std::string text = this->binds_.save_text();
	std::error_code ignored;
	if (this->binds_ == DebugBinds::defaults())
	{
		// Nothing differs: no file, so a later change to a default reaches
		// this profile too.
		std::filesystem::remove(path, ignored);
		return;
	}
	if (!debug_files::write_file(path, text))
	{
		this->log_.warning("binds: cannot write " + labrador::path_to_utf8(path) +
			"; the change holds for this session");
	}
}

// ---------------------------------------------------------------------------
// Commands

void DebugDirectorImpl::register_bind_commands()
{
	using Args = const DebugArgs&;
	DebugRegistry& registry = this->registry_;
	const auto add = [&registry](std::string name, std::string help,
		std::vector<DebugArgSpec> args, LambdaCommand::Body body, DebugMenuTraits menu,
		bool variadic = false)
		{
			auto command = std::make_unique<LambdaCommand>(std::move(name), std::move(help),
				std::move(args), std::move(body), variadic);
			command->set_menu(std::move(menu));
			registry.add(std::move(command));
		};

	const auto list_binds = [this](DebugLog& log)
		{
			for (const auto& [chord, line] : this->binds_.entries())
			{
				const std::string* original = DebugBinds::defaults().find(chord);
				const std::string name = upper(debug_keys::chord_name(chord));
				log.info("  " + name + std::string(name.size() < 10 ? 10 - name.size() : 1, ' ') +
					line + (original != nullptr && *original == line ? "" : "  *"));
			}
			for (const auto& [chord, line] : DebugBinds::defaults().entries())
			{
				if (this->binds_.find(chord) == nullptr)
				{
					log.info("  " + upper(debug_keys::chord_name(chord)) + "  (unbound; default " +
						line + ")");
				}
			}
			return DebugStatus::success(std::to_string(this->binds_.entries().size()) +
				" bind(s), matches only; * differs from the default. grave, F10 and F11 are reserved.");
		};
	const auto key_words = []
		{
			std::vector<std::string> words;
			for (const auto& [key, name] : debug_keys::names())
			{
				words.push_back(name);
			}
			return words;
		};

	add("bind", "bind <key> <line>: run a console line on a key in a match (ctrl+, shift+); "
		"bind <key> shows one; bind alone lists",
		{ { "key", true, key_words }, { "line", true, {} } },
		[this, list_binds](Args args, DebugLog& log)
		{
			if (!args.has(0))
			{
				return list_binds(log);
			}
			const std::optional<KeyChord> chord = debug_keys::parse(args.text(0));
			if (!chord)
			{
				return DebugStatus::failure("bind: unknown key '" + args.text(0) + "' (a-z, 0-9, "
					"f1-f12, numpad0-9, space, tab, ..., with ctrl+ or shift+)");
			}
			const std::string name = debug_keys::chord_name(*chord);
			if (!args.has(1))
			{
				const std::string* line = this->binds_.find(*chord);
				return DebugStatus::success(line != nullptr ? name + " = " + *line : name + " is not bound");
			}
			if (debug_keys::reserved(*chord))
			{
				return DebugStatus::failure("bind: " + name + " is reserved (" +
					(chord->key == labrador::Key::grave ? "the console" : "the debug menu") + ")");
			}
			std::string line;
			if (args.size() == 2)
			{
				line = args.text(1);
			}
			else
			{
				for (std::size_t i = 1; i < args.size(); ++i)
				{
					line += (i == 1 ? "" : " ") + quote_word(args.text(i));
				}
			}
			for (const std::string& statement : split_statements(line))
			{
				const DebugTokens tokens = tokenize(statement);
				if (!tokens.ok)
				{
					return DebugStatus::failure("bind: " + tokens.error);
				}
			}
			if (debug_keys::player_key(*chord) && !this->binds_loading_)
			{
				log.warning("bind: " + name + " also moves the keyboard player in a match");
			}
			this->binds_.bind(*chord, line);
			this->save_binds();
			return DebugStatus::success("bind " + name + " " + quote_word(line));
		}, traits("console", Form::hidden), true);

	add("unbind", "unbind <key|all>: all clears every key, including the defaults",
		{ { "key|all", false, key_words } },
		[this](Args args, DebugLog&)
		{
			if (args.text(0) == "all")
			{
				this->binds_.clear();
				this->save_binds();
				return DebugStatus::success("unbind all: no key is bound ('binds.reset' for the "
					"defaults)");
			}
			const std::optional<KeyChord> chord = debug_keys::parse(args.text(0));
			if (!chord)
			{
				return DebugStatus::failure("unbind: unknown key '" + args.text(0) + "'");
			}
			if (!this->binds_.unbind(*chord))
			{
				return DebugStatus::failure("unbind: " + debug_keys::chord_name(*chord) + " is not bound");
			}
			this->save_binds();
			return DebugStatus::success("unbind " + debug_keys::chord_name(*chord));
		}, traits("console", Form::hidden));

	add("binds", "list the key table (F2-F7 and F9 by default)", {},
		[list_binds](Args, DebugLog& log) { return list_binds(log); },
		traits("console", Form::run));

	add("binds.reset", "restore the default keys; deletes binds.cfg", {},
		[this](Args, DebugLog&)
		{
			this->binds_.reset();
			// The defaults delete binds.cfg; never while binds.cfg is being read
			// (a file that starts with binds.reset must not delete itself).
			this->save_binds();
			return DebugStatus::success("binds.reset: default keys restored");
		}, traits("console", Form::run));
}
#endif
