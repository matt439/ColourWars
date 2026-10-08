#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "engine/render/text_encoding.h"
#include "game/debug/debug_console_view.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_keys.h"
#include "game/debug/debug_script.h"
#include "game/debug/debug_tokenizer.h"
#include "game/debug/replay.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/objects/level.h"
#include "game/source_paths.h"
#include <bit>
#include <cmath>
#include <ctime>
#include <fstream>

// Phase 4: scripts (exec, wait, echo), binds, history, the live console,
// screenshot folders and the perf overlay.

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

	std::string read_file(const std::filesystem::path& path, std::size_t limit, bool& too_big)
	{
		too_big = false;
		std::error_code error;
		const std::uintmax_t size = std::filesystem::file_size(path, error);
		if (!error && size > limit)
		{
			too_big = true;
			return {};
		}
		std::ifstream stream(path, std::ios::binary);
		return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	}

	// Atomically: a temporary file beside the target, then a replacing move.
	bool write_file(const std::filesystem::path& path, const std::string& text)
	{
		std::error_code ignored;
		std::filesystem::create_directories(path.parent_path(), ignored);
		// Per process: two Debug instances on one profile never share a
		// half-written temporary (the last whole file wins).
		std::filesystem::path temporary = path;
		temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
		{
			std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
			if (!stream)
			{
				return false;
			}
			stream << text;
			if (!stream.flush())
			{
				return false;
			}
		}
		if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
		{
			std::filesystem::remove(temporary, ignored);
			return false;
		}
		return true;
	}

	std::string upper(std::string text)
	{
		for (char& c : text)
		{
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		}
		return text;
	}

	std::string local_time_stamp()
	{
		const std::time_t now = std::time(nullptr);
		std::tm local{};
		localtime_s(&local, &now);
		char buffer[32] = {};
		std::strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", &local);
		return buffer;
	}

	// "review-{time}" -> "review-20261009-031500"; each '/' segment kept to
	// [A-Za-z0-9_-]. Empty when the text has no usable segment; `error` says
	// why a text is refused.
	std::string clean_directory(std::string text, std::string& error)
	{
		const std::string time = local_time_stamp();
		for (std::size_t at = text.find("{time}"); at != std::string::npos; at = text.find("{time}"))
		{
			text.replace(at, 6, time);
		}
		std::vector<std::string> segments;
		std::string segment;
		const auto flush = [&segments, &segment, &error]
			{
				if (segment == "..")
				{
					error = "'..' is not allowed";
				}
				else if (!segment.empty() && segment != ".")
				{
					for (char& c : segment)
					{
						const bool allowed = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
							(c >= '0' && c <= '9') || c == '_' || c == '-';
						c = allowed ? c : '_';
					}
					segments.push_back(segment);
				}
				segment.clear();
			};
		for (const char c : text)
		{
			if (c == '/' || c == '\\')
			{
				flush();
			}
			else
			{
				segment += c;
			}
		}
		flush();
		if (segments.size() > 3)
		{
			error = "at most three folders";
		}
		if (!error.empty())
		{
			return {};
		}
		std::string joined;
		for (const std::string& part : segments)
		{
			joined += (joined.empty() ? "" : "/") + part;
		}
		return joined;
	}

	// The first // line of a script, for exec.list.
	std::string summary(const std::filesystem::path& path)
	{
		std::ifstream stream(path, std::ios::binary);
		std::string line;
		for (int i = 0; i < 8 && std::getline(stream, line); ++i)
		{
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			const std::size_t start = line.find_first_not_of(" \t\xEF\xBB\xBF");
			if (start != std::string::npos && line.compare(start, 2, "//") == 0)
			{
				const std::size_t text = line.find_first_not_of(" /", start);
				return text == std::string::npos ? std::string() : line.substr(text);
			}
		}
		return {};
	}
}

// ---------------------------------------------------------------------------
// Directories and persistence

void DebugDirectorImpl::set_user_directory(std::filesystem::path directory)
{
	this->user_override_ = std::move(directory);
}

std::optional<std::filesystem::path> DebugDirectorImpl::user_directory() const
{
	if (!this->user_override_.empty())
	{
		return this->user_override_;
	}
	if (this->application_ != nullptr)
	{
		return std::filesystem::path(labrador::widen(directory_consts::debug_directory()));
	}
	return std::nullopt;
}

void DebugDirectorImpl::set_script_directories(std::vector<std::filesystem::path> directories)
{
	this->script_override_ = std::move(directories);
}

std::vector<std::filesystem::path> DebugDirectorImpl::script_directories() const
{
	if (!this->script_override_.empty())
	{
		return this->script_override_;
	}
	std::vector<std::filesystem::path> directories{
		std::filesystem::path(labrador::widen(source_paths::scripts_directory)) };
	if (const std::optional<std::filesystem::path> user = this->user_directory())
	{
		directories.push_back(*user);
	}
	return directories;
}

void DebugDirectorImpl::load_persistent()
{
	if (this->persistence_loaded_)
	{
		return;
	}
	const std::optional<std::filesystem::path> directory = this->user_directory();
	if (!directory)
	{
		return;
	}
	this->persistence_loaded_ = true;
	this->load_binds(*directory);

	bool too_big = false;
	const std::filesystem::path history = *directory / L"history.txt";
	std::error_code ignored;
	if (std::filesystem::is_regular_file(history, ignored))
	{
		const std::string text = read_file(history, 1024 * 1024, too_big);
		std::vector<std::string> lines;
		std::size_t start = 0;
		while (start < text.size())
		{
			std::size_t end = text.find('\n', start);
			end = end == std::string::npos ? text.size() : end;
			std::string line = text.substr(start, end - start);
			start = end + 1;
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}
			const bool printable = std::all_of(line.begin(), line.end(), [](char c)
				{
					return static_cast<unsigned char>(c) >= 0x20 && static_cast<unsigned char>(c) <= 0x7e;
				});
			if (printable)
			{
				lines.push_back(line);
			}
		}
		this->field_.set_history(lines);
	}
}

void DebugDirectorImpl::load_binds(const std::filesystem::path& directory)
{
	const std::filesystem::path path = directory / L"binds.cfg";
	std::error_code ignored;
	if (!std::filesystem::is_regular_file(path, ignored))
	{
		return;
	}
	bool too_big = false;
	const std::string text = read_file(path, debug_script::max_bytes, too_big);
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
	if (!write_file(path, text))
	{
		this->log_.warning("binds: cannot write " + labrador::path_to_utf8(path) +
			"; the change holds for this session");
	}
}

void DebugDirectorImpl::save_history()
{
	const std::optional<std::filesystem::path> directory = this->user_directory();
	if (!directory)
	{
		return;
	}
	std::string text;
	for (const std::string& line : this->field_.history())
	{
		text += line + "\n";
	}
	const std::filesystem::path path = *directory / L"history.txt";
	if (!write_file(path, text) && !this->history_write_failed_)
	{
		this->history_write_failed_ = true;
		this->log_.warning("history: cannot write " + labrador::path_to_utf8(path) +
			"; the history holds for this session");
	}
}

// ---------------------------------------------------------------------------
// Scripts

DebugStatus DebugDirectorImpl::exec_script(const std::string& name,
	const std::vector<std::string>& args)
{
	std::string file;
	if (!debug_script::file_name(name, file))
	{
		return DebugStatus::failure("exec: '" + name + "' is not a script name ([A-Za-z0-9_-], "
			"optionally .cfg; not a path)");
	}
	std::filesystem::path found;
	std::string searched;
	for (const std::filesystem::path& directory : this->script_directories())
	{
		std::error_code ignored;
		searched += (searched.empty() ? "" : ", ") + labrador::path_to_utf8(directory);
		if (std::filesystem::is_regular_file(directory / labrador::widen(file), ignored))
		{
			found = directory / labrador::widen(file);
			break;
		}
	}
	if (found.empty())
	{
		return DebugStatus::failure("exec: no " + file + " in " + searched + " (try exec.list)");
	}
	bool too_big = false;
	const std::string text = read_file(found, debug_script::max_bytes, too_big);
	if (too_big)
	{
		return DebugStatus::failure("exec: " + file + " is larger than " +
			std::to_string(debug_script::max_bytes) + " bytes");
	}
	const ParsedScript parsed = parse_script(file, text, args);
	if (!parsed.ok)
	{
		return DebugStatus::failure("exec: " + parsed.error);
	}
	const bool nested = this->scripts_.executing();
	DebugScriptRunner::Frame frame;
	frame.name = file;
	frame.statements = parsed.statements;
	const DebugStatus pushed = this->scripts_.push(std::move(frame));
	if (!pushed.ok)
	{
		return pushed;
	}
	if (nested)
	{
		return DebugStatus::success();
	}
	// Typed or bound: the console closes and the hooks run it from the next
	// frame (a live console's hook runs it this frame).
	return DebugStatus::success("exec " + file + ": " + std::to_string(parsed.statements.size()) +
		" statement(s)", true);
}

DebugStatus DebugDirectorImpl::wait_command(const DebugArgs& args)
{
	if (!this->scripts_.executing())
	{
		return DebugStatus::failure("wait: only in a script (exec, autoexec.cfg or launch "
			"arguments)");
	}
	const DebugWaitContext context = this->wait_context();
	const std::string& word = args.text(0);
	if (word == "match")
	{
		if (!context.in_match && !context.build_pending)
		{
			return DebugStatus::failure("wait match: no match is running or queued (stage first)");
		}
		return this->scripts_.set_wait(std::make_unique<MatchWait>());
	}
	if (word == "replay")
	{
		if (!context.replay_active)
		{
			return DebugStatus::failure("wait replay: no replay is armed or playing");
		}
		return this->scripts_.set_wait(std::make_unique<ReplayWait>());
	}
	if (word == "record")
	{
		if (!context.recording)
		{
			return DebugStatus::failure("wait record: nothing is recording or armed");
		}
		return this->scripts_.set_wait(std::make_unique<RecordWait>());
	}
	if (word == "frames")
	{
		int frames = 0;
		if (!args.has(1) || !DebugArgs::parse_int(args.text(1), frames) || frames < 1 ||
			frames > 1000000)
		{
			return DebugStatus::failure("wait frames: expected 1-1000000");
		}
		return this->scripts_.set_wait(std::make_unique<FramesWait>(
			context.frames + frames, frames));
	}
	if (args.has(1))
	{
		return DebugStatus::failure("usage: wait <updates> | <seconds>s | match | frames <n> | "
			"replay | record");
	}

	long long updates = 0;
	if (!word.empty() && word.back() == 's')
	{
		float seconds = 0.0f;
		if (!DebugArgs::parse_float(std::string_view(word).substr(0, word.size() - 1), seconds) ||
			seconds <= 0.0f || seconds > 3600.0f)
		{
			return DebugStatus::failure("wait: expected seconds in (0, 3600], got '" + word + "'");
		}
		// The engine's step is fixed; before the first update, 1/60.
		const float dt = this->last_dt_bits_ != 0 ?
			std::bit_cast<float>(this->last_dt_bits_) : 1.0f / 60.0f;
		updates = std::max(1L, std::lround(seconds / dt));
	}
	else
	{
		int count = 0;
		if (!DebugArgs::parse_int(word, count) || count < 1 || count > 1000000)
		{
			return DebugStatus::failure("wait: expected 1-1000000 updates, <x>s, match, frames <n>, "
				"replay or record; got '" + word + "'");
		}
		updates = count;
	}
	if (!context.in_match && !context.build_pending)
	{
		return DebugStatus::failure("wait: no match is running or queued (stage first, or "
			"wait frames)");
	}
	return this->scripts_.set_wait(std::make_unique<UpdatesWait>(
		context.updates + updates, updates));
}

std::string DebugDirectorImpl::rebuild_note() const
{
	return this->staging_ || this->debug_menu_ != nullptr ?
		": queued; rebuilds when the debug menu closes" : ": rebuilding the match";
}

// ---------------------------------------------------------------------------
// Drawing

void DebugDirectorImpl::begin_match_draw()
{
	if (!this->capturing_)
	{
		this->perf_.begin_draw(perf_now_ms());
	}
}

void DebugDirectorImpl::draw_status(labrador::Renderer& renderer)
{
	if (!this->capturing_)
	{
		this->perf_.end_draw(perf_now_ms());
	}
	const Level* level = this->match_ != nullptr ? this->match_->debug_level() : nullptr;
	if (level != nullptr)
	{
		this->perf_.set_counts(level->collision_object_count(), level->projectile_count());
	}
	const GameData& data = *this->data_;
	using Anchor = DebugConsoleView::Anchor;
	// Bottom corners, clear of the opening cards and of each other: the paint
	// panel, the perf panel above it, then the strip bottom right (or above
	// both when a narrow window cannot hold them on one row).
	std::vector<mattmath::RectangleF> avoid = this->match_ != nullptr ?
		this->match_->debug_screen_obstacles() : std::vector<mattmath::RectangleF>();
	const mattmath::RectangleF paint = DebugConsoleView::draw_panel(renderer, data,
		this->paint_lines(), Anchor::bottom_left, avoid);
	avoid.push_back(paint);
	if (this->overlay_perf_)
	{
		avoid.push_back(DebugConsoleView::draw_perf(renderer, data, this->perf_, avoid));
	}
	if (!this->console_open() && (this->screenshot_strip_ || !this->capturing_))
	{
		const std::string strip = this->status_strip();
		DebugConsoleView::draw_panel(renderer, data,
			strip.empty() ? std::vector<std::string>() : std::vector<std::string>{ strip },
			Anchor::bottom_right, avoid);
	}
	if (this->live_open_ && this->live_view_ != nullptr)
	{
		this->live_view_->draw(renderer, data, *this);
	}
}

// ---------------------------------------------------------------------------
// Commands

void DebugDirectorImpl::register_script_commands()
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

	const auto script_names = [this]
		{
			std::vector<std::string> names;
			for (const std::filesystem::path& directory : this->script_directories())
			{
				std::error_code error;
				for (std::filesystem::directory_iterator it(directory, error), end;
					!error && it != end; it.increment(error))
				{
					if (it->path().extension() == L".cfg")
					{
						const std::string stem = labrador::path_to_utf8(it->path().stem());
						std::string file;
						if (debug_script::file_name(stem, file) &&
							std::find(names.begin(), names.end(), stem) == names.end())
						{
							names.push_back(stem);
						}
					}
				}
			}
			std::sort(names.begin(), names.end());
			return names;
		};

	add("exec", "run a script of console lines: exec <name> [args for $1..$9] "
		"(scripts/debug, then %LOCALAPPDATA%/ColourWars/debug)",
		{ { "name", false, script_names } },
		[this](Args args, DebugLog&)
		{
			std::vector<std::string> rest;
			for (std::size_t i = 1; i < args.size(); ++i)
			{
				rest.push_back(args.text(i));
			}
			return this->exec_script(args.text(0), rest);
		}, traits("console", Form::hidden), true);

	add("exec.stop", "stop every running script", {},
		[this](Args, DebugLog&)
		{
			if (!this->scripts_.running())
			{
				return DebugStatus::success("exec.stop: no script is running");
			}
			this->scripts_.stop("exec.stop", this->log_);
			return DebugStatus::success();
		}, traits("console", Form::run));

	add("exec.list", "the scripts exec can run, with their first comment line", {},
		[this, script_names](Args, DebugLog& log)
		{
			int count = 0;
			for (const std::string& name : script_names())
			{
				for (const std::filesystem::path& directory : this->script_directories())
				{
					const std::filesystem::path path = directory / labrador::widen(name + ".cfg");
					std::error_code ignored;
					if (std::filesystem::is_regular_file(path, ignored))
					{
						const std::string text = summary(path);
						log.info("  " + name + (text.empty() ? std::string() : "  - " + text));
						++count;
						break;
					}
				}
			}
			std::string where;
			for (const std::filesystem::path& directory : this->script_directories())
			{
				where += (where.empty() ? "" : ", ") + labrador::path_to_utf8(directory);
			}
			return DebugStatus::success(std::to_string(count) + " script(s) in " + where);
		}, traits("console", Form::run));

	add("wait", "in a script: wait <updates> | <seconds>s | match | frames <n> | replay | record",
		{ { "n|<x>s|match|frames|replay|record", false, []
			{
				return std::vector<std::string>{ "match", "frames", "replay", "record" };
			} },
			{ "frames", true, {} } },
		[this](Args args, DebugLog&) { return this->wait_command(args); },
		traits("console", Form::hidden));

	add("echo", "print a line (a marker in a script's transcript)",
		{ { "text", true, {} } },
		[](Args args, DebugLog& log)
		{
			std::string text;
			for (std::size_t i = 0; i < args.size(); ++i)
			{
				text += (i == 0 ? "" : " ") + args.text(i);
			}
			log.info(text);
			return DebugStatus::success();
		}, traits("console", Form::hidden), true);

	// Binds -----------------------------------------------------------------

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

	add("unbind", "unbind <key|all>: all clears every key, the F2-F9 defaults too",
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

	add("binds", "list the key table (F2-F9 by default)", {},
		[list_binds](Args, DebugLog& log) { return list_binds(log); },
		traits("console", Form::run));

	add("binds.reset", "the default F2-F9 keys again; deletes binds.cfg", {},
		[this](Args, DebugLog&)
		{
			this->binds_.reset();
			// The defaults delete binds.cfg; never while binds.cfg is being read
			// (a file that starts with binds.reset must not delete itself).
			this->save_binds();
			return DebugStatus::success("binds.reset: F2-F9 are the defaults again");
		}, traits("console", Form::run));

	// The live console ------------------------------------------------------

	registry.add(std::make_unique<BoolVariable>("console.live",
		"the console over a running match: the match keeps going, the keyboard seat is "
		"idle while it is open (off: the console freezes the world)", false,
		[this] { return this->live_console_; },
		[this](bool on)
		{
			this->live_console_ = on;
			if (!on)
			{
				this->close_live_console("console.live 0");
			}
			return DebugStatus::success(std::string("console.live = ") + (on ? "on" : "off") +
				(on ? " (opens with ` over a match)" : ""));
		}));

	// Screenshots -----------------------------------------------------------

	add("screenshot.dir", "a folder under screens/ for the next screenshots ({time} is the "
		"local time); off for screens/ itself",
		{ { "folder|off", true, [] { return std::vector<std::string>{ "off" }; } } },
		[this](Args args, DebugLog&)
		{
			const std::string root = directory_consts::screenshot_directory();
			if (!args.has(0))
			{
				return DebugStatus::success("screenshot.dir = " + (this->screenshot_dir_.empty() ?
					std::string("off (") + root + ")" : this->screenshot_dir_));
			}
			if (args.text(0) == "off" || args.text(0).empty())
			{
				this->screenshot_dir_.clear();
				return DebugStatus::success("screenshot.dir off: " + root);
			}
			std::string error;
			const std::string cleaned = clean_directory(args.text(0), error);
			if (!error.empty() || cleaned.empty())
			{
				return DebugStatus::failure("screenshot.dir: " + (error.empty() ?
					std::string("no folder name in '") + args.text(0) + "'" : error));
			}
			this->screenshot_dir_ = cleaned;
			return DebugStatus::success("screenshot.dir = " + cleaned + " (" + root + "/" +
				cleaned + ")");
		}, traits("content", Form::hidden));

	auto strip = std::make_unique<BoolVariable>("screenshot.strip",
		"whether a screenshot shows the debug status strip (off for A/B pictures that "
		"differ only in the world)", true,
		[this] { return this->screenshot_strip_; },
		[this](bool on)
		{
			this->screenshot_strip_ = on;
			return DebugStatus::success(std::string("screenshot.strip = ") + (on ? "on" : "off"));
		});
	strip->set_menu(traits("content", Form::hidden));
	registry.add(std::move(strip));

	// Perf ------------------------------------------------------------------

	add("perf", "frame, update and draw time (CPU, min / avg / max) and object counts", {},
		[this](Args, DebugLog& log)
		{
			if (this->perf_.size() == 0)
			{
				return DebugStatus::success("perf: no match frame measured yet");
			}
			for (const std::string& line : this->perf_.lines())
			{
				log.info(line);
			}
			return DebugStatus::success();
		}, traits("view", Form::hidden));

	add("perf.reset", "forget the measured frames", {},
		[this](Args, DebugLog&)
		{
			this->perf_.reset();
			return DebugStatus::success("perf: reset");
		}, traits("view", Form::hidden));
}
#endif
