#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_text_field.h"

bool DebugKeyRepeat::fire(bool held, bool pressed, float dt)
{
	if (pressed)
	{
		this->held_time_ = 0.0f;
		this->next_fire_ = delay;
		return true;
	}
	if (!held)
	{
		this->held_time_ = 0.0f;
		this->next_fire_ = 0.0f;
		return false;
	}
	this->held_time_ += dt;
	if (this->next_fire_ > 0.0f && this->held_time_ >= this->next_fire_ - 1.0e-5f)
	{
		this->next_fire_ += interval;
		return true;
	}
	return false;
}

void DebugTextField::apply(const DebugKeyEvents& events)
{
	if (events.clear_line)
	{
		this->text_.clear();
		this->caret_ = 0;
	}

	for (const char c : events.typed)
	{
		const unsigned char byte = static_cast<unsigned char>(c);
		if (byte < 0x20 || byte > 0x7e || this->text_.size() >= max_length)
		{
			continue;
		}
		this->text_.insert(this->text_.begin() + static_cast<std::ptrdiff_t>(this->caret_), c);
		++this->caret_;
	}

	if (events.backspace && this->caret_ > 0)
	{
		this->text_.erase(this->caret_ - 1, 1);
		--this->caret_;
	}
	if (events.del && this->caret_ < this->text_.size())
	{
		this->text_.erase(this->caret_, 1);
	}
	if (events.left && this->caret_ > 0)
	{
		--this->caret_;
	}
	if (events.right && this->caret_ < this->text_.size())
	{
		++this->caret_;
	}
	if (events.home)
	{
		this->caret_ = 0;
	}
	if (events.end)
	{
		this->caret_ = this->text_.size();
	}
	if (events.up)
	{
		this->browse(+1);
	}
	if (events.down)
	{
		this->browse(-1);
	}
}

void DebugTextField::browse(int direction)
{
	const int count = static_cast<int>(this->history_.size());
	if (count == 0)
	{
		return;
	}
	if (this->history_index_ == -1)
	{
		if (direction < 0)
		{
			return;
		}
		this->draft_ = this->text_;
	}
	const int next = std::clamp(this->history_index_ + direction, -1, count - 1);
	this->history_index_ = next;
	this->text_ = next == -1 ? this->draft_ :
		this->history_[static_cast<std::size_t>(count - 1 - next)];
	this->caret_ = this->text_.size();
}

const std::string& DebugTextField::text() const
{
	return this->text_;
}

std::size_t DebugTextField::caret() const
{
	return this->caret_;
}

void DebugTextField::set_text(std::string text)
{
	if (text.size() > max_length)
	{
		text.resize(max_length);
	}
	this->text_ = std::move(text);
	this->caret_ = this->text_.size();
}

std::string DebugTextField::submit()
{
	std::string line = std::move(this->text_);
	this->text_.clear();
	this->caret_ = 0;
	this->history_index_ = -1;
	this->draft_.clear();
	if (line.find_first_not_of(' ') != std::string::npos &&
		(this->history_.empty() || this->history_.back() != line))
	{
		this->history_.push_back(line);
		if (this->history_.size() > history_capacity)
		{
			this->history_.erase(this->history_.begin());
		}
	}
	return line;
}

const std::vector<std::string>& DebugTextField::history() const
{
	return this->history_;
}

void DebugTextField::set_history(const std::vector<std::string>& lines)
{
	this->history_.clear();
	for (const std::string& line : lines)
	{
		if (line.find_first_not_of(' ') == std::string::npos || line.size() > max_length ||
			(!this->history_.empty() && this->history_.back() == line))
		{
			continue;
		}
		this->history_.push_back(line);
	}
	if (this->history_.size() > history_capacity)
	{
		this->history_.erase(this->history_.begin(), this->history_.end() -
			static_cast<std::ptrdiff_t>(history_capacity));
	}
	this->history_index_ = -1;
	this->draft_.clear();
}
#endif
