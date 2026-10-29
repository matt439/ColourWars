#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/core/file_path.h"
#include "engine/render/text_encoding.h"
#include "game/debug/debug_console_view.h"
#include "game/debug/debug_director_impl.h"
#include "game/debug/debug_files.h"
#include "game/debug/debug_script.h"
#include "game/debug/replay.h"
#include "game/directory_consts.h"
#include "game/game_data.h"
#include "game/objects/level.h"
#include "game/source_paths.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string_view>

// Phase 4: scripts (exec, wait, echo), history, the live console, screenshot
// folders and the perf overlay. Binds are in bind_commands.cpp.

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

	std::string local_time_stamp()
	{
		const std::time_t now = std::time(nullptr);
		std::tm local{};
		localtime_s(&local, &now);
		char buffer[32] = {};
		std::strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", &local);
		return buffer;
	}

	bool reserved_directory_segment(std::string_view segment)
	{
		if (segment.size() != 3 && segment.size() != 4) return false;
		const auto upper = [](char value)
		{
			return value >= 'a' && value <= 'z' ? static_cast<char>(value - 'a' + 'A') : value;
		};
		const char prefix[] = { upper(segment[0]), upper(segment[1]), upper(segment[2]) };
		const std::string_view stem(prefix, 3);
		return (segment.size() == 3 && (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL")) ||
			(segment.size() == 4 && (stem == "COM" || stem == "LPT") && segment[3] >= '1' && segment[3] <= '9');
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
		for (const std::string& part : segments)
		{
			if (reserved_directory_segment(part))
			{
				error = "reserved Windows folder name: " + part;
				return {};
			}
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
		const std::string text = debug_files::read_file(history, 1024 * 1024, too_big);
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
	if (!debug_files::write_file(path, text) && !this->history_write_failed_)
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
	const std::string text = debug_files::read_file(found, debug_script::max_bytes, too_big);
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

void DebugDirectorImpl::break_perf_interval()
{
	// A page/focus/screenshot transition can arrive before the next draw.
	// Preserve an exact already-completed final sample before breaking, but
	// never wait for or invent the phases of a frame still in flight.
	if (this->application_ != nullptr)
	{
		this->perf_.break_interval(this->application_->frame_timings());
	}
	else { this->perf_.break_interval(); }
}

DebugStatus DebugDirectorImpl::start_perf_capture(const DebugArgs& args)
{
	int frames = 0;
	if (!DebugArgs::parse_int(args.text(0), frames) || frames < 1 ||
		frames > static_cast<int>(DebugPerf::max_capture_frames))
	{
		return DebugStatus::failure("perf.capture: expected 1-36000 rendered intervals");
	}
	const std::string& label = args.text(1);
	if (label.empty() || label.size() > 64 || !std::all_of(label.begin(), label.end(), [](char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') || c == '-' || c == '_';
	}))
	{
		return DebugStatus::failure("perf.capture: label must use 1-64 letters, digits, '-' or '_'");
	}
	if (this->perf_capture_path_ || this->perf_.capture_active() || this->perf_.capture_ready())
	{
		return DebugStatus::failure("perf.capture: another capture is pending");
	}
	if (this->application_ == nullptr || !this->application_->active() || this->match_ == nullptr ||
		!this->clock_.is_default() || this->debug_menu_ != nullptr || this->live_open_ ||
		!this->match_->debug_perf_ready() || this->rebuild_queued() ||
		this->application_->depth() != (this->console_open_ ? 3 : 2))
	{
		return DebugStatus::failure("perf.capture: needs a focused live match at timescale 1 with no rebuild queued");
	}
	const auto directory = this->user_directory();
	if (!directory) { return DebugStatus::failure("perf.capture: no developer output directory"); }
	const mattmath::Vector2I size = this->application_->resolution_manager()->resolution_ivec();
	this->perf_width_ = size.x;
	this->perf_height_ = size.y;
	this->break_perf_interval();
	if (!this->perf_.start_capture(static_cast<std::size_t>(frames)))
	{
		return DebugStatus::failure("perf.capture: could not start recording");
	}
	this->perf_capture_path_ = *directory / L"perf" / labrador::widen(label);
	return DebugStatus::success("perf.capture: recording " + std::to_string(frames) +
		" rendered intervals (CPU wall time, diagnostic only)", true);
}

void DebugDirectorImpl::finish_perf_capture()
{
	if (!this->perf_capture_path_) { return; }
	// Update hooks run after the previous Application publish and before a
	// scripted quit. Pair its last sample here; no extra match draw is needed.
	if (this->application_ != nullptr)
	{
		this->perf_.observe_frame_timings(this->application_->frame_timings());
	}
	if (!this->perf_.capture_ready()) { return; }
	std::filesystem::path csv_path = *this->perf_capture_path_;
	csv_path += L".csv";
	std::filesystem::path json_path = *this->perf_capture_path_;
	json_path += L".json";
	std::ostringstream csv;
	csv.imbue(std::locale::classic());
	csv << std::setprecision(17) << "frame,application_frame,frame_ms,update_ms,draw_ms,begin_frame_ms,submit_ms,end_frame_ms,updates,objects,projectiles\n";
	std::size_t index = 0;
	std::size_t paired = 0;
	for (const DebugPerf::Frame& frame : this->perf_.captured_frames())
	{
		csv << ++index << ',' << frame.application_frame << ',' << frame.frame_ms << ',' << frame.update_ms << ',' << frame.draw_ms;
		if (frame.application_frame != 0)
		{
			csv << ',' << frame.begin_frame_ms << ',' << frame.submit_ms << ',' << frame.end_frame_ms;
			++paired;
		}
		else { csv << ",,,"; }
		csv << ',' << frame.updates << ',' << frame.objects << ',' << frame.projectiles << '\n';
	}
	const bool complete = this->perf_.capture_complete() && paired == index;
	const std::string reason = this->perf_.capture_complete() && !complete ?
		"unpaired_engine_frame" : this->perf_.capture_reason();
	const std::string json = "{\n  \"schema_version\": 2,\n  \"requested_frames\": " +
		std::to_string(this->perf_.capture_target()) + ",\n  \"captured_frames\": " + std::to_string(index) +
		",\n  \"complete\": " + (complete ? "true" : "false") +
		",\n  \"reason\": \"" + reason +
		"\",\n  \"timing_scope\": \"match_draw_begin_to_begin_cpu_wall_ms\",\n"
		"  \"engine_timing_scope\": \"matched_completed_application_frame_cpu_wall_ms\",\n"
		"  \"engine_paired_frames\": " + std::to_string(paired) + "\n}\n";
	const bool written = debug_files::write_file(csv_path, csv.str()) && debug_files::write_file(json_path, json);
	this->perf_capture_path_.reset();
	this->perf_.discard_capture();
	this->break_perf_interval();
	if (written)
	{
		this->log_.info(std::string("perf.capture: ") + (complete ? "complete " : "interrupted ") +
			std::to_string(index) + " intervals: " + labrador::path_to_utf8(csv_path));
	}
	else { this->log_.error("perf.capture: could not write CSV/JSON output"); }
}

// ---------------------------------------------------------------------------
// Drawing

void DebugDirectorImpl::begin_match_draw()
{
	// A covered match still draws, sometimes several times between updates.
	// It and an inactive window must never create a pacing interval.
	if (this->capturing_ || this->application_ == nullptr || !this->application_->active() ||
		this->application_->depth() != 2 || this->debug_menu_ != nullptr ||
		this->console_open_)
	{
		this->break_perf_interval();
		return;
	}
	const mattmath::Vector2I size = this->application_->resolution_manager()->resolution_ivec();
	if (this->perf_.capture_active() && (!this->clock_.is_default() || this->live_open_ ||
		this->match_ == nullptr || !this->match_->debug_perf_ready() ||
		size.x != this->perf_width_ || size.y != this->perf_height_))
	{
		this->break_perf_interval();
	}
	this->perf_.begin_draw(perf_now_ms(), this->application_->frame_timings());
}

void DebugDirectorImpl::draw_status(labrador::Renderer& renderer)
{
	const Level* level = this->match_ != nullptr ? this->match_->debug_level() : nullptr;
	if (level != nullptr)
	{
		this->perf_.set_counts(level->collision_object_count(), level->projectile_count());
	}
	if (!this->capturing_) { this->perf_.end_draw(perf_now_ms()); }
	const GameData& data = *this->data_;
	using Anchor = DebugConsoleView::Anchor;
	// Bottom corners, clear of the opening cards and of each other: the paint
	// panel, the perf panel above it, then the strip bottom right (or above
	// both when a narrow window cannot hold them on one row).
	std::vector<mattmath::RectangleF> avoid = this->match_ != nullptr ?
		this->match_->debug_screen_obstacles() : std::vector<mattmath::RectangleF>();
	// Resolved once here for every panel below, not once per panel.
	const DebugConsoleView::OverlayHandles handles =
		DebugConsoleView::OverlayHandles::resolve(data);
	const mattmath::RectangleF paint = DebugConsoleView::draw_panel(renderer, data, handles,
		this->paint_lines(), Anchor::bottom_left, avoid);
	avoid.push_back(paint);
	if (this->overlay_perf_)
	{
		avoid.push_back(DebugConsoleView::draw_perf(renderer, data, handles, this->perf_, avoid));
	}
	if (!this->console_open() && (this->screenshot_strip_ || !this->capturing_))
	{
		const std::string strip = this->status_strip();
		DebugConsoleView::draw_panel(renderer, data, handles,
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
					const auto extension = it->path().extension().native();
					std::error_code kind_error;
					if (CompareStringOrdinal(extension.c_str(), -1, L".cfg", -1, TRUE) == CSTR_EQUAL &&
						it->is_regular_file(kind_error))
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

	// Binds (bind_commands.cpp), in the place they always had.
	this->register_bind_commands();

	// The live console ------------------------------------------------------

	registry.add(std::make_unique<BoolVariable>("script.background",
		"session-only inactive script/bot/replay updates; physical input, audio and rumble "
		"stay inactive; hidden, minimised and power-suspended windows still stop", false,
		[this] { return this->background_scripts_; },
		[this](bool on)
		{
			this->background_scripts_ = on;
			if (on && this->application_ != nullptr && this->data_ != nullptr &&
				!this->data_->keyboard()->focused())
			{
				this->sync_background_audio(false);
			}
			this->background_changed();
			return DebugStatus::success(std::string("script.background = ") + (on ? "on" : "off") +
				(on && !this->background_updates_allowed() ? " (window gate closed)" : ""));
		}));

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

	add("perf", "frame, update and draw time (CPU, min / avg / max), object counts and the last level build", {},
		[this](Args, DebugLog& log)
		{
			if (this->last_build_)
			{
				char ms[32];
				std::snprintf(ms, sizeof(ms), "%.1f", this->last_build_->ms);
				log.info(std::string("build ") + ms + " ms  " + this->last_build_->stage + "  " +
					std::to_string(this->last_build_->players) + "P");
			}
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
			if (this->perf_capture_path_) { return DebugStatus::failure("perf: a capture is pending"); }
			this->perf_.reset();
			return DebugStatus::success("perf: reset");
		}, traits("view", Form::hidden));

	add("perf.capture", "capture 1-36000 rendered match intervals, then export CSV and JSON; script barrier",
		{ { "frames", false, {} }, { "label", false, {} } },
		[this](Args args, DebugLog&) { return this->start_perf_capture(args); },
		traits("view", Form::hidden));
}
#endif
