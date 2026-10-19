#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/tune_target.h"
#include "engine/app/application.h"
#include "game/content_validation.h"
#include "game/game_data.h"
#include "game/presentation.h"
#include "game/save.h"
#include "game/states/main_menu.h"
#include "game/tuning.h"

namespace
{
	bool can_check_levels(const GameData* context)
	{
		return context != nullptr && context->application() != nullptr &&
			context->level_infos() != nullptr && context->stages() != nullptr &&
			context->save() != nullptr;
	}

	bool can_check_resources(const GameData* context)
	{
		return context != nullptr && context->application() != nullptr;
	}

	void check_levels(const GameTuning& tuning, const Presentation& presentation,
		const GameData& context)
	{
		validate_game_content(tuning, presentation, context.render_resources(),
			context.audio_resources(), *context.level_infos(), *context.stages(),
			context.save()->play_settings());
	}
}

TuningTarget::TuningTarget(GameTuning* live, const Presentation* presentation) :
	live_(live), presentation_(presentation)
{
}

TuningTarget::~TuningTarget() = default;

void TuningTarget::validate(const std::string& candidate_file, const GameData* context,
	bool checks_levels)
{
	this->candidate_.reset();
	auto candidate = std::make_unique<GameTuning>(candidate_file.c_str());
	if (checks_levels && can_check_levels(context))
	{
		check_levels(*candidate, *this->presentation_, *context);
	}
	else if (can_check_resources(context))
	{
		// Startup's checks of numbers against the loaded resources, without
		// building every stage.
		candidate->validate_resources(*context->render_resources(), *context->audio_resources());
		MainMenuModeSelect::validate_durations(context->render_resources(), *candidate, *this->presentation_);
	}
	this->candidate_ = std::move(candidate);
}

void TuningTarget::apply(bool)
{
	if (this->candidate_ != nullptr)
	{
		*this->live_ = *this->candidate_;
		this->candidate_.reset();
	}
}

PresentationTarget::PresentationTarget(Presentation* live, const GameTuning* tuning) :
	live_(live), tuning_(tuning)
{
}

PresentationTarget::~PresentationTarget() = default;

void PresentationTarget::validate(const std::string& candidate_file, const GameData* context,
	bool checks_levels)
{
	this->candidate_.reset();
	auto candidate = std::make_unique<Presentation>(candidate_file.c_str());
	if (checks_levels && can_check_levels(context))
	{
		check_levels(*this->tuning_, *candidate, *context);
	}
	else if (can_check_resources(context))
	{
		// Frame sizes against each frame's native texels, and the countdown
		// box against its heading: startup's checks that need the resources.
		candidate->validate_resources(*context->render_resources(), *context->audio_resources());
		MainMenuModeSelect::validate_durations(context->render_resources(), *this->tuning_, *candidate);
	}
	this->candidate_ = std::move(candidate);
}

void PresentationTarget::apply(bool keep_live_toggle)
{
	if (this->candidate_ == nullptr)
	{
		return;
	}
	if (keep_live_toggle)
	{
		// F9 is a live switch, not a file edit: a tune of anything else keeps it.
		this->candidate_->set_movement_feedback_enabled(
			this->live_->movement_feedback().enabled);
	}
	*this->live_ = *this->candidate_;
	this->candidate_.reset();
}
#endif
