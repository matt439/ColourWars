#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_log.h"
#include "engine/render/text_encoding.h"
#include <filesystem>

void DebugLog::write(DebugSeverity severity, std::string_view text)
{
	std::size_t start = 0;
	while (start <= text.size())
	{
		const std::size_t end = text.find('\n', start);
		const std::string_view line = text.substr(start,
			end == std::string_view::npos ? std::string_view::npos : end - start);

		this->lines_.push_back({ severity, std::string(line) });
		while (this->lines_.size() > capacity)
		{
			this->lines_.pop_front();
		}

		const char* prefix = severity == DebugSeverity::error ? "error: " :
			severity == DebugSeverity::warning ? "warning: " : "";
		std::string mirrored = "[console] ";
		mirrored += prefix;
		mirrored += line;
		mirrored += '\n';
		if (this->mirror_to_debugger_)
		{
			OutputDebugStringW(labrador::widen(mirrored).c_str());
		}
		if (this->transcript_.is_open())
		{
			this->transcript_ << mirrored.substr(10);
			this->transcript_.flush();
		}

		if (end == std::string_view::npos)
		{
			break;
		}
		start = end + 1;
	}
}

void DebugLog::info(std::string_view text)
{
	this->write(DebugSeverity::info, text);
}

void DebugLog::warning(std::string_view text)
{
	this->write(DebugSeverity::warning, text);
}

void DebugLog::error(std::string_view text)
{
	this->write(DebugSeverity::error, text);
}

const std::deque<DebugLogLine>& DebugLog::lines() const
{
	return this->lines_;
}

void DebugLog::clear()
{
	this->lines_.clear();
}

bool DebugLog::set_transcript(const std::string& path)
{
	this->transcript_.close();
	this->transcript_path_.clear();
	if (path.empty())
	{
		return true;
	}
	this->transcript_.open(std::filesystem::path(labrador::widen(path)),
		std::ios::out | std::ios::trunc | std::ios::binary);
	if (!this->transcript_.is_open())
	{
		return false;
	}
	this->transcript_path_ = path;
	// What was logged before the file existed (the intro, autoexec.cfg, the
	// launch statements before +logfile) heads it, so a transcript is whole.
	for (const DebugLogLine& line : this->lines_)
	{
		this->transcript_ << (line.severity == DebugSeverity::error ? "error: " :
			line.severity == DebugSeverity::warning ? "warning: " : "") << line.text << '\n';
	}
	this->transcript_.flush();
	return true;
}

const std::string& DebugLog::transcript_path() const
{
	return this->transcript_path_;
}

void DebugLog::set_mirror_to_debugger(bool mirror)
{
	this->mirror_to_debugger_ = mirror;
}
#endif
