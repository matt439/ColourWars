#include "game/pch.h"
#include "game/debug/debug_director.h"
#include "game/debug/debug_tokenizer.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_director_impl.h"

using namespace labrador;

DebugDirector::DebugDirector() = default;

DebugDirector::DebugDirector(GameTuning* tuning, Presentation* presentation)
{
	if (tuning != nullptr && presentation != nullptr)
	{
		this->impl_ = std::make_unique<DebugDirectorImpl>(tuning, presentation);
	}
}

DebugDirector::~DebugDirector() = default;

DebugDirector& DebugDirector::inert()
{
	static DebugDirector director;
	return director;
}

void DebugDirector::attach(Application* application, GameData* data)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->attach(application, data);
	}
}

void DebugDirector::run_launch_arguments(const std::vector<std::wstring>& arguments)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->run_launch_statements(parse_launch_arguments(arguments));
	}
}

void DebugDirector::attach_match(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->attach_match(match);
	}
}

void DebugDirector::detach_match(const DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->detach_match(match);
	}
}

void DebugDirector::before_level_build(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->before_level_build(match);
	}
}

void DebugDirector::after_level_build(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->after_level_build(match);
	}
}

void DebugDirector::filter_match_inputs(std::vector<SlotInput>& inputs)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->filter_match_inputs(inputs);
	}
}

bool DebugDirector::before_match_update(DebugMatchTarget& match, float dt, bool active)
{
	return this->impl_ != nullptr && this->impl_->before_match_update(match, dt, active);
}

int DebugDirector::match_steps(const std::vector<SlotInput>& inputs, float dt)
{
	return this->impl_ != nullptr ? this->impl_->match_steps(inputs, dt) : 1;
}

bool DebugDirector::before_match_step(DebugMatchTarget& match, int run, MatchStep& step)
{
	if (this->impl_ == nullptr)
	{
		step.seats = nullptr;
		return true;
	}
	return this->impl_->before_match_step(match, run, step);
}

bool DebugDirector::after_match_step(DebugMatchTarget& match)
{
	return this->impl_ == nullptr || this->impl_->after_match_step(match);
}

void DebugDirector::end_match_steps(DebugMatchTarget& match, int runs)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->end_match_steps(match, runs);
	}
}

bool DebugDirector::match_audio_paused() const
{
	return this->impl_ != nullptr && (this->impl_->clock().paused() ||
		this->impl_->debug_menu_open() || this->impl_->blocking_console_open());
}

bool DebugDirector::background_scripts() const
{
	return this->impl_ != nullptr && this->impl_->background_scripts();
}

bool DebugDirector::background_updates_allowed() const
{
	return this->impl_ != nullptr && this->impl_->background_updates_allowed();
}

void DebugDirector::sync_background_audio(bool active)
{
	if (this->impl_ != nullptr) { this->impl_->sync_background_audio(active); }
}

void DebugDirector::match_suspended(DebugMatchTarget& match)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->match_suspended(match);
	}
}

void DebugDirector::match_resumed(DebugMatchTarget& match, const std::vector<PlayerInputData>& seats)
{
	if (this->impl_ != nullptr)
	{
		this->impl_->match_resumed(match, seats);
	}
}

bool DebugDirector::drives_slot(int slot) const
{
	return this->impl_ != nullptr && this->impl_->drives_slot(slot);
}

OpeningMode DebugDirector::opening_mode(OpeningMode chosen) const
{
	return this->impl_ != nullptr ? this->impl_->opening_mode(chosen) : chosen;
}

bool DebugDirector::before_page_update(DebugPageTarget& page, float dt, bool active)
{
	return this->impl_ != nullptr && this->impl_->before_page_update(page, dt, active);
}

void DebugDirector::begin_keyboard_capture()
{
	if (this->impl_ != nullptr) { this->impl_->begin_keyboard_capture(); }
}

void DebugDirector::capture_input_frame(DebugPageTarget& page)
{
	if (this->impl_ != nullptr) { this->impl_->capture_input_frame(page); }
}

void DebugDirector::end_keyboard_capture()
{
	if (this->impl_ != nullptr) { this->impl_->end_keyboard_capture(); }
}

bool DebugDirector::keyboard_jump_collision(Key key) const
{
	return this->impl_ != nullptr && this->impl_->keyboard_jump_collision(key);
}

void DebugDirector::draw_status(Renderer& renderer) const
{
	if (this->impl_ != nullptr && this->impl_->data() != nullptr)
	{
		this->impl_->draw_status(renderer);
	}
}

void DebugDirector::begin_match_draw() const
{
	if (this->impl_ != nullptr)
	{
		this->impl_->begin_match_draw();
	}
}

double DebugDirector::frame_time_ms() const
{
	return this->impl_ != nullptr ? this->impl_->perf().current_frame_ms() : 0.0;
}

#else

#include <cstdio>

// Release: no implementation exists; the type is complete only so the
// unique_ptr member can be destroyed.
class DebugDirectorImpl
{
};

DebugDirector::DebugDirector() = default;

DebugDirector::DebugDirector(GameTuning*, Presentation*)
{
}

DebugDirector::~DebugDirector() = default;

DebugDirector& DebugDirector::inert()
{
	static DebugDirector director;
	return director;
}

void DebugDirector::run_launch_arguments(const std::vector<std::wstring>& arguments)
{
	if (has_launch_statements(arguments))
	{
		std::fputs("developer arguments ignored: Release build\n", stderr);
	}
}

#endif
