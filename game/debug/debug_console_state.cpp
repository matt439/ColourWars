#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_console_state.h"
#include "engine/core/state_context.h"
#include "game/debug/debug_director_impl.h"
#include "game/game_data.h"

DebugConsoleState::DebugConsoleState(GameData* data, DebugDirectorImpl* director) :
	data_(data), director_(director)
{
	this->director_->set_console_open(true);
}

DebugConsoleState::~DebugConsoleState()
{
	this->director_->set_console_open(false);
}

void DebugConsoleState::init()
{
	this->view_.init(*this->data_);
}

void DebugConsoleState::close()
{
	if (!this->closing_)
	{
		this->closing_ = true;
		this->context()->pop();
	}
}

void DebugConsoleState::update(float dt)
{
	if (this->closing_)
	{
		return;
	}
	if (this->view_.update(*this->data_->keyboard(), dt, *this->director_) ==
		DebugConsoleView::Outcome::close)
	{
		this->close();
	}
}

void DebugConsoleState::draw(labrador::Renderer& renderer) const
{
	this->view_.draw(renderer, *this->data_, *this->director_);
}
#endif
