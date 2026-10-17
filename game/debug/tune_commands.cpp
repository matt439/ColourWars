#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/content_root.h"
#include "engine/core/file_path.h"
#include "engine/render/text_encoding.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/replay.h"
#include "game/debug/tune_document.h"
#include "game/debug/tune_scope.h"
#include "game/debug/tune_target.h"
#include "game/presentation.h"
#include "game/source_paths.h"
#include <charconv>
#include <cmath>
#include <fstream>
#include <process.h>

// tune, tune.diff, tune.reset, tune.save and tune.find: live edits of the
// numbers and booleans in tuning.json and presentation.json, validated by the
// game's own readers. See docs/debug-console.md, "Live tuning".

namespace
{
	constexpr const char* file_names[2] = { "tuning", "presentation" };

	DebugMenuTraits tune_traits(DebugMenuTraits::Form form)
	{
		DebugMenuTraits traits;
		traits.category = "tuning";
		traits.form = form;
		return traits;
	}

	std::string read_bytes(const std::filesystem::path& path, bool& ok)
	{
		std::ifstream stream(path, std::ios::binary);
		ok = stream.good();
		return ok ? std::string(std::istreambuf_iterator<char>(stream),
			std::istreambuf_iterator<char>()) : std::string();
	}

	bool write_bytes(const std::filesystem::path& path, const std::string& bytes)
	{
		std::ofstream stream(path, std::ios::binary | std::ios::trunc);
		stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
		stream.close();
		return !stream.fail();
	}

	// Writes beside the target and renames over it, so a crash mid-write never
	// leaves half a content file. On failure the target is untouched and no
	// temporary is left in the content directory for the build to stage.
	bool replace_file(const std::filesystem::path& path, const std::string& bytes)
	{
		std::filesystem::path temporary = path;
		temporary += L".tmp";
		std::error_code ignored;
		if (!write_bytes(temporary, bytes))
		{
			std::filesystem::remove(temporary, ignored);
			return false;
		}
		if (MoveFileExW(temporary.c_str(), path.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0)
		{
			std::filesystem::remove(temporary, ignored);
			return false;
		}
		return true;
	}

	void replace_all(std::string& text, const std::string& from, const std::string& to)
	{
		if (from.empty())
		{
			return;
		}
		for (std::size_t at = text.find(from); at != std::string::npos;
			at = text.find(from, at + to.size()))
		{
			text.replace(at, from.size(), to);
		}
	}

	// Forward slashes throughout, as reload prints its directory.
	std::string shown(const std::filesystem::path& path)
	{
		const std::u8string text = path.lexically_normal().generic_u8string();
		return std::string(text.begin(), text.end());
	}

	bool parse_double(std::string_view text, double& value)
	{
		if (!text.empty() && text.front() == '+')
		{
			text.remove_prefix(1);
		}
		const char* end = text.data() + text.size();
		const auto result = std::from_chars(text.data(), end, value);
		return !text.empty() && result.ec == std::errc() && result.ptr == end &&
			std::isfinite(value);
	}

	// `tune` completes its path one segment at a time and its value with the
	// words that path accepts.
	class TuneCommand final : public DebugCommand
	{
	public:
		using Paths = std::function<std::vector<std::string>()>;
		using Token = std::function<std::string(const std::string&)>;
		using Body = std::function<DebugStatus(const DebugArgs&, DebugLog&)>;

		TuneCommand(Paths paths, Token token, Body body) :
			DebugCommand("tune", "print or set a number or boolean in tuning.json or "
				"presentation.json: tune <path> [value|next|prev|+=x|-=x|toggle] [restart]",
				{ { "path", false, {} }, { "value", true, {} }, { "restart", true, {} } }),
			paths_(std::move(paths)), token_(std::move(token)), body_(std::move(body))
		{
		}

		std::vector<std::string> completions(const std::vector<std::string>& previous,
			std::string_view partial) const override
		{
			if (previous.empty())
			{
				// Parent of the partial path: up to its last '.' or '['.
				const std::size_t cut = partial.find_last_of(".[");
				const std::string_view parent = cut == std::string_view::npos ?
					std::string_view() : partial.substr(0, cut + 1);
				return tune_children(this->paths_(), parent);
			}
			if (previous.size() == 1)
			{
				const std::string token = this->token_(previous.front());
				if (token == "true" || token == "false")
				{
					return { "on", "off", "toggle" };
				}
				return token.empty() ? std::vector<std::string>() :
					std::vector<std::string>{ "next", "prev" };
			}
			if (previous.size() == 2)
			{
				return { "restart" };
			}
			return {};
		}

		DebugStatus run(const DebugArgs& args, DebugLog& log) override
		{
			return this->body_(args, log);
		}

	private:
		Paths paths_;
		Token token_;
		Body body_;
	};
}

void DebugDirectorImpl::set_content_directories(std::filesystem::path source,
	std::filesystem::path staged)
{
	this->source_override_ = std::move(source);
	this->staged_override_ = std::move(staged);
	for (std::unique_ptr<TuneDocument>& document : this->tune_docs_)
	{
		document.reset();
	}
	this->tune_load_tried_ = false;
}

std::filesystem::path DebugDirectorImpl::source_content_directory() const
{
	return this->source_override_.empty() ?
		std::filesystem::path(labrador::widen(source_paths::content_directory)) :
		this->source_override_;
}

std::filesystem::path DebugDirectorImpl::staged_content_directory() const
{
	return this->staged_override_.empty() ?
		std::filesystem::path(labrador::widen(labrador::executable_directory())) :
		this->staged_override_;
}

void DebugDirectorImpl::load_tune_documents()
{
	this->tune_load_tried_ = true;
	for (int file = 0; file < 2; ++file)
	{
		const std::filesystem::path path = this->staged_content_directory() /
			labrador::widen(std::string(file_names[file]) + ".json");
		bool ok = false;
		std::string text = read_bytes(path, ok);
		if (!ok)
		{
			this->log_.warning("tune: cannot read " + shown(path));
			continue;
		}
		try
		{
			this->tune_docs_[static_cast<std::size_t>(file)] =
				std::make_unique<TuneDocument>(file_names[file], std::move(text));
		}
		catch (const std::exception& e)
		{
			this->log_.warning(std::string("tune: ") + e.what());
		}
	}
}

int DebugDirectorImpl::tune_file_index(std::string_view file) const
{
	for (int index = 0; index < 2; ++index)
	{
		if (file == file_names[index])
		{
			return index;
		}
	}
	return -1;
}

TuneDocument* DebugDirectorImpl::tune_document(std::string_view file)
{
	if (!this->tune_load_tried_)
	{
		this->load_tune_documents();
	}
	const int index = this->tune_file_index(file);
	return index < 0 ? nullptr : this->tune_docs_[static_cast<std::size_t>(index)].get();
}

std::vector<std::string> DebugDirectorImpl::tune_paths()
{
	std::vector<std::string> paths;
	for (const char* file : file_names)
	{
		if (const TuneDocument* document = this->tune_document(file))
		{
			const std::vector<std::string> some = document->prefixed_paths();
			paths.insert(paths.end(), some.begin(), some.end());
		}
	}
	return paths;
}

std::string DebugDirectorImpl::tune_token(const std::string& path)
{
	const std::size_t dot = path.find('.');
	if (dot == std::string::npos)
	{
		return {};
	}
	const TuneDocument* document = this->tune_document(std::string_view(path).substr(0, dot));
	if (document == nullptr)
	{
		return {};
	}
	const JsonLeaf* leaf = document->current().find(std::string_view(path).substr(dot + 1));
	return leaf == nullptr ? std::string() : std::string(document->current().token(*leaf));
}

std::vector<std::filesystem::path> stale_tune_directories(const std::filesystem::path& temp)
{
	std::vector<std::filesystem::path> result;
	std::error_code error;
	for (std::filesystem::directory_iterator it(temp, error), end; !error && it != end;
		it.increment(error))
	{
		const std::wstring name = it->path().filename().wstring();
		const std::wstring prefix = L"ColourWars-tune-";
		if (name.size() <= prefix.size() || name.compare(0, prefix.size(), prefix) != 0)
		{
			continue;
		}
		const bool digits = std::all_of(name.begin() + static_cast<std::ptrdiff_t>(prefix.size()),
			name.end(), [](wchar_t c) { return c >= L'0' && c <= L'9'; });
		std::error_code ignored;
		// Only an empty folder: removing one cannot delete anybody's data.
		if (digits && it->is_directory(ignored) && std::filesystem::is_empty(it->path(), ignored))
		{
			result.push_back(it->path());
		}
	}
	return result;
}

DebugStatus DebugDirectorImpl::tune_validate_and_apply(int file, const std::string& candidate,
	const std::string& edited_path)
{
	TuneDocument& document = *this->tune_docs_[static_cast<std::size_t>(file)];
	const std::string name = file_names[file];

	// The engine reads JSON only from a file (engine report 12). A file, not
	// a folder: earlier builds left empty ColourWars-tune-<pid> folders behind
	// (the folder's removal raced the reader's handle), swept once here.
	std::error_code ignored;
	const std::filesystem::path temp = std::filesystem::temp_directory_path(ignored);
	if (!this->tune_swept_)
	{
		this->tune_swept_ = true;
		for (const std::filesystem::path& stale : stale_tune_directories(temp))
		{
			std::filesystem::remove(stale, ignored);
		}
	}
	const std::filesystem::path path = temp /
		(L"ColourWars-tune-" + std::to_wstring(_getpid()) + L"-" + labrador::widen(name) + L".json");
	const auto remove_candidate = [&]
		{
			std::filesystem::remove(path, ignored);
		};
	if (!write_bytes(path, candidate))
	{
		remove_candidate();
		return DebugStatus::failure("tune: cannot write the candidate " +
			labrador::path_to_utf8(path));
	}
	const std::string utf8 = labrador::path_to_utf8(path);
	// Every edit runs the reader and, with an application, the resource checks
	// startup runs on that file; paint also reaches every stage's objects, and
	// the player's size and the speeds its per-step travel is taken from reach
	// every stage's tunnelling bound.
	const bool checks_levels = name == "tuning" &&
		(edited_path.empty() || edited_path.starts_with("paint.") ||
			edited_path.starts_with("player.size.") ||
			edited_path.starts_with("player.max_velocity.") ||
			edited_path == "player.x_initial_velocity" ||
			edited_path == "player.drop_down_velocity");
	try
	{
		this->tune_targets_[static_cast<std::size_t>(file)]->validate(utf8, this->data_,
			checks_levels);
	}
	catch (const std::exception& e)
	{
		remove_candidate();
		std::string message = e.what();
		replace_all(message, utf8, name + ".json");
		return DebugStatus::failure("tune: rejected: " + message);
	}
	remove_candidate();

	// F9 is a live switch; only an edit of the flag itself replaces it.
	const bool touches_toggle = name == "presentation" && (edited_path.empty() ?
		document.edits().count("movement_feedback.enabled") != 0 :
		edited_path == "movement_feedback.enabled");
	this->tune_targets_[static_cast<std::size_t>(file)]->apply(!touches_toggle);
	if (edited_path.empty())
	{
		document.rebase(candidate);
	}
	else
	{
		document.commit(candidate, edited_path);
	}
	return DebugStatus::success();
}

DebugStatus DebugDirectorImpl::tune_value(const std::string& path, const std::string& word,
	bool restart)
{
	const std::size_t dot = path.find('.');
	const int file = dot == std::string::npos ? -1 :
		this->tune_file_index(std::string_view(path).substr(0, dot));
	if (file < 0)
	{
		return DebugStatus::failure("tune: a path starts with tuning. or presentation. "
			"(Tab completes it)");
	}
	TuneDocument* document = this->tune_document(file_names[file]);
	if (document == nullptr)
	{
		return DebugStatus::failure(std::string("tune: ") + file_names[file] +
			".json could not be read at startup");
	}
	const std::string rest = path.substr(dot + 1);
	const JsonLeaf* leaf = document->current().find(rest);
	if (leaf == nullptr)
	{
		return DebugStatus::failure("tune: no number or boolean '" + path + "' (strings, colours "
			"and keys the file leaves out are not tunable; try tune.find)");
	}
	const std::string token(document->current().token(*leaf));
	const TuneScope scope = tune_scope_of(path);
	if (word.empty())
	{
		const auto edit = document->edits().find(rest);
		return DebugStatus::success(path + " = " + token +
			(edit != document->edits().end() ? "  (file: " + edit->second.baseline_token + ")" :
				std::string()) + "  [" + tune_scope_name(scope) + "]");
	}
	if (const auto refused = this->refuse_while_replaying("tune"))
	{
		return *refused;
	}
	if (this->has_pending_content())
	{
		return DebugStatus::failure("tune: a reload is queued; restart (or leave the match) first");
	}

	std::string next;
	if (leaf->kind == JsonLeaf::Kind::boolean)
	{
		bool ok = true;
		const bool value = BoolVariable::parse(word, token == "true", ok);
		if (!ok)
		{
			return DebugStatus::failure("tune: " + path + " is on or off (on, off, toggle)");
		}
		next = value ? "true" : "false";
	}
	else
	{
		double current = 0.0;
		parse_double(token, current);
		const bool integer = tune_integer_token(token);
		double value = 0.0;
		if (word == "next" || word == "prev")
		{
			const double step = tune_step(current, integer);
			value = word == "next" ? current + step : current - step;
		}
		else if (word.size() > 2 && (word.compare(0, 2, "+=") == 0 || word.compare(0, 2, "-=") == 0))
		{
			double delta = 0.0;
			if (!parse_double(std::string_view(word).substr(2), delta))
			{
				return DebugStatus::failure("tune: '" + word + "' is not +=number or -=number");
			}
			value = word[0] == '+' ? current + delta : current - delta;
		}
		else if (!parse_double(word, value))
		{
			return DebugStatus::failure("tune: '" + word + "' is not a number, next, prev, "
				"+=x or -=x");
		}
		next = tune_format(value, token);
	}
	if (next == token)
	{
		return DebugStatus::success(path + " = " + token + " (unchanged)");
	}

	const DebugStatus applied = this->tune_validate_and_apply(file,
		document->edited(*leaf, next), rest);
	if (!applied.ok)
	{
		return applied;
	}
	std::string message = "tune: " + path + " " + token + " -> " + next + " " +
		tune_scope_note(scope);
	if (restart)
	{
		if (this->match_ != nullptr)
		{
			this->request_.restart = true;
			return DebugStatus::success(message + "; restarting the match", true);
		}
		message += " (no match to restart)";
	}
	return DebugStatus::success(message);
}

DebugStatus DebugDirectorImpl::tune_reset(const std::string& what)
{
	if (const auto refused = this->refuse_while_replaying("tune.reset"))
	{
		return *refused;
	}
	if (this->has_pending_content())
	{
		return DebugStatus::failure("tune.reset: a reload is queued; restart (or leave the match) "
			"first");
	}
	std::vector<int> files;
	std::string path;
	if (what.empty() || what == "all")
	{
		files = { 0, 1 };
	}
	else if (this->tune_file_index(what) >= 0)
	{
		files = { this->tune_file_index(what) };
	}
	else
	{
		path = what;
	}

	if (!path.empty())
	{
		const std::size_t dot = path.find('.');
		const int file = dot == std::string::npos ? -1 :
			this->tune_file_index(std::string_view(path).substr(0, dot));
		TuneDocument* document = file < 0 ? nullptr : this->tune_document(file_names[file]);
		const std::string rest = dot == std::string::npos ? std::string() : path.substr(dot + 1);
		if (document == nullptr || document->edits().count(rest) == 0)
		{
			return DebugStatus::failure("tune.reset: '" + path + "' has no live edit (tune.diff "
				"lists them)");
		}
		const std::string was = document->edits().at(rest).current_token;
		const std::string back = document->edits().at(rest).baseline_token;
		const DebugStatus applied = this->tune_validate_and_apply(file,
			document->reverted(rest), rest);
		if (!applied.ok)
		{
			return applied;
		}
		return DebugStatus::success("tune.reset: " + path + " " + was + " -> " + back + " " +
			tune_scope_note(tune_scope_of(path)));
	}

	int reverted = 0;
	for (const int file : files)
	{
		TuneDocument* document = this->tune_document(file_names[file]);
		if (document == nullptr || document->edits().empty())
		{
			continue;
		}
		const int count = static_cast<int>(document->edits().size());
		const DebugStatus applied = this->tune_validate_and_apply(file,
			document->reverted(""), "");
		if (!applied.ok)
		{
			return applied;
		}
		reverted += count;
	}
	return DebugStatus::success(reverted == 0 ? std::string("tune.reset: no edits") :
		"tune.reset: " + std::to_string(reverted) + " value(s) back to the file's");
}

std::vector<std::string> DebugDirectorImpl::tune_diff_lines(const std::string& which)
{
	std::vector<std::string> lines;
	for (int file = 0; file < 2; ++file)
	{
		if (!which.empty() && which != "all" && which != file_names[file])
		{
			continue;
		}
		const TuneDocument* document = this->tune_document(file_names[file]);
		if (document == nullptr)
		{
			continue;
		}
		for (const auto& [path, edit] : document->edits())
		{
			const std::string full = std::string(file_names[file]) + "." + path;
			lines.push_back(full + "  " + edit.baseline_token + " -> " + edit.current_token +
				"  [" + tune_scope_name(tune_scope_of(full)) + "]");
		}
		if (file == 1)
		{
			const JsonLeaf* flag = document->current().find("movement_feedback.enabled");
			if (flag != nullptr)
			{
				const bool in_file = document->current().token(*flag) == "true";
				if (in_file != this->presentation_->movement_feedback().enabled)
				{
					lines.push_back(std::string("presentation.movement_feedback.enabled  live "
						"toggle (F9) is ") + (in_file ? "off" : "on") + ", not a file edit");
				}
			}
		}
	}
	return lines;
}

DebugStatus DebugDirectorImpl::tune_save(const std::string& which, bool force)
{
	// The save makes the file live, outside any recorded input; and a queued
	// reload would put the texts it read back over the saved values.
	if (const auto refused = this->refuse_while_replaying("tune.save"))
	{
		return *refused;
	}
	if (this->has_pending_content())
	{
		return DebugStatus::failure("tune.save: a reload is queued; restart (or leave the match) "
			"first");
	}
	std::vector<int> files;
	if (which.empty() || which == "all")
	{
		files = { 0, 1 };
	}
	else if (this->tune_file_index(which) >= 0)
	{
		files = { this->tune_file_index(which) };
	}
	else
	{
		return DebugStatus::failure("tune.save: tuning, presentation or all");
	}

	std::vector<std::string> done;
	bool failed = false;
	for (const int file : files)
	{
		TuneDocument* document = this->tune_document(file_names[file]);
		if (document == nullptr || document->edits().empty())
		{
			continue;
		}
		const std::string name = std::string(file_names[file]) + ".json";
		const std::filesystem::path source = this->source_content_directory() / labrador::widen(name);
		std::error_code error;
		if (!std::filesystem::is_regular_file(source, error))
		{
			this->log_.error("tune.save: no source tree at " +
				shown(this->source_content_directory()) +
				"; tune.save needs a Debug build made from a checkout");
			failed = true;
			continue;
		}
		bool ok = false;
		const std::string before = read_bytes(source, ok);
		if (!ok)
		{
			this->log_.error("tune.save: cannot read " + shown(source));
			failed = true;
			continue;
		}
		TuneSplice splice;
		try
		{
			splice = document->splice(before, force);
		}
		catch (const std::exception& e)
		{
			this->log_.error(std::string("tune.save: ") + e.what());
			failed = true;
			continue;
		}
		if (!splice.conflicts.empty())
		{
			for (const std::string& conflict : splice.conflicts)
			{
				this->log_.error("tune.save: conflict: " + conflict);
			}
			this->log_.error("tune.save: " + name + " not written; edit it by hand, reload, "
				"or 'tune.save " + file_names[file] + " force' to overwrite those values");
			failed = true;
			continue;
		}
		if (!replace_file(source, splice.text))
		{
			this->log_.error("tune.save: cannot write " + shown(source));
			failed = true;
			continue;
		}
		// The game's reader on the file as written, and every startup check
		// that reads it (resources and stages), so a save never leaves a file
		// the next launch refuses.
		try
		{
			this->tune_targets_[static_cast<std::size_t>(file)]->validate(
				shown(source), this->data_, true);
			this->tune_targets_[static_cast<std::size_t>(file)]->apply(true);
		}
		catch (const std::exception& e)
		{
			const std::string why = std::string("tune.save: the written file did not load (") +
				e.what() + ")";
			if (replace_file(source, before))
			{
				this->log_.error(why + "; the old bytes are back");
			}
			else
			{
				// Keep the old bytes somewhere a person can find them.
				std::filesystem::path backup = source;
				backup += L".bak";
				const bool kept = write_bytes(backup, before);
				this->log_.error(why + "; putting the old bytes back FAILED, so " + shown(source) +
					" holds the rejected text" + (kept ? "; the old file is in " + shown(backup) :
						std::string("; and they could not be kept beside it either")));
			}
			failed = true;
			continue;
		}
		// A relaunch without a rebuild reads the staged copy: keep it equal,
		// and whole if the write fails.
		const std::filesystem::path staged = this->staged_content_directory() / labrador::widen(name);
		if (!replace_file(staged, splice.text))
		{
			this->log_.warning("tune.save: could not refresh the staged " +
				shown(staged) + "; the next build copies it");
		}
		document->rebase(splice.text);
		done.push_back(std::to_string(splice.written) + " value(s) written to " +
			shown(source));
	}
	if (done.empty() && !failed)
	{
		return DebugStatus::success("tune.save: no edits to save");
	}
	for (std::size_t i = 0; i + 1 < done.size(); ++i)
	{
		this->log_.info("tune.save: " + done[i]);
	}
	if (failed)
	{
		return DebugStatus::failure(done.empty() ? "tune.save: nothing written" :
			"tune.save: " + done.back() + "; the rest failed");
	}
	return DebugStatus::success("tune.save: " + done.back());
}

void DebugDirectorImpl::register_tune_commands()
{
	using Args = const DebugArgs&;
	DebugRegistry& registry = this->registry_;

	auto tune = std::make_unique<TuneCommand>(
		[this] { return this->tune_paths(); },
		[this](const std::string& path) { return this->tune_token(path); },
		[this](Args args, DebugLog&)
		{
			const bool restart = args.has(2) && args.text(2) == "restart";
			if (args.has(2) && !restart)
			{
				return DebugStatus::failure("tune: the third word may only be restart");
			}
			return this->tune_value(args.text(0), args.has(1) ? args.text(1) : std::string(),
				restart);
		});
	tune->set_menu(tune_traits(DebugMenuTraits::Form::hidden));
	registry.add(std::move(tune));

	const auto add = [&registry](std::string name, std::string help,
		std::vector<DebugArgSpec> args, LambdaCommand::Body body, DebugMenuTraits menu)
		{
			auto command = std::make_unique<LambdaCommand>(std::move(name), std::move(help),
				std::move(args), std::move(body));
			command->set_menu(std::move(menu));
			registry.add(std::move(command));
		};
	const auto files = [] { return std::vector<std::string>{ "all", "tuning", "presentation" }; };

	add("tune.diff", "list the live tune edits (and the F9 toggle when it differs)",
		{ { "tuning|presentation", true, files } },
		[this](Args args, DebugLog& log)
		{
			const std::vector<std::string> lines = this->tune_diff_lines(
				args.has(0) ? args.text(0) : std::string());
			for (const std::string& line : lines)
			{
				log.info("  " + line);
			}
			return DebugStatus::success(lines.empty() ? std::string("tune.diff: no edits") :
				"tune.diff: " + std::to_string(lines.size()) + " line(s)");
		}, tune_traits(DebugMenuTraits::Form::run));

	add("tune.reset", "put live edits back to the file's values: a path, a file, or all",
		{ { "path|tuning|presentation|all", true, [this]
			{
				std::vector<std::string> words = { "all", "tuning", "presentation" };
				for (const char* file : file_names)
				{
					if (const TuneDocument* document = this->tune_document(file))
					{
						for (const auto& edit : document->edits())
						{
							words.push_back(std::string(file) + "." + edit.first);
						}
					}
				}
				return words;
			} } },
		[this](Args args, DebugLog&)
		{
			return this->tune_reset(args.has(0) ? args.text(0) : std::string());
		}, tune_traits(DebugMenuTraits::Form::run));

	add("tune.save", "write the live edits into the SOURCE content files (only those tokens), "
		"then reload them; force overwrites values changed on disk",
		{ { "tuning|presentation|all", true, files },
			{ "force", true, [] { return std::vector<std::string>{ "force" }; } } },
		[this](Args args, DebugLog&)
		{
			const bool force = (args.has(0) && args.text(0) == "force") ||
				(args.has(1) && args.text(1) == "force");
			const std::string which = args.has(0) && args.text(0) != "force" ? args.text(0) : "all";
			return this->tune_save(which, force);
		}, tune_traits(DebugMenuTraits::Form::run));

	add("tune.find", "list tunable paths containing a word",
		{ { "word", false, {} } },
		[this](Args args, DebugLog& log)
		{
			int shown = 0;
			for (const std::string& path : this->tune_paths())
			{
				if (path.find(args.text(0)) != std::string::npos)
				{
					log.info("  " + path + " = " + this->tune_token(path));
					++shown;
				}
			}
			return shown == 0 ? DebugStatus::failure("tune.find: nothing contains '" +
				args.text(0) + "'") : DebugStatus::success(std::to_string(shown) + " path(s)");
		}, tune_traits(DebugMenuTraits::Form::hidden));
}
#endif
