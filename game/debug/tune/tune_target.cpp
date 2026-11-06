#include "game/pch.h"
#include "game/debug/dev_tools.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/tune/tune_target.h"
#include "engine/app/application.h"
#include "engine/assets/json.h"
#include "game/game_data.h"
#include "game/config/presentation/presentation.h"
#include "game/config/content_validation.h"
#include "game/save/save.h"
#include "game/states/menu_content_validation.h"
#include "game/config/tuning.h"

namespace
{
	bool can_check_levels(const GameData* context)
	{
		return context != nullptr && context->application() != nullptr &&
			context->level_infos() != nullptr && context->stages() != nullptr &&
			context->settings_session() != nullptr;
	}

	bool can_check_resources(const GameData* context)
	{
		return context != nullptr && context->application() != nullptr;
	}

	void check_levels(const GameTuning& tuning, const Presentation& presentation,
		const GameData& context)
	{
		validate_shell_content(tuning, presentation, context.render_resources(),
			context.audio_resources(), *context.level_infos(), *context.stages(),
			context.settings_session()->play_settings());
	}
}

TuningTarget::TuningTarget(GameTuning* live, const Presentation* presentation) :
	live_(live), presentation_(presentation)
{
}

TuningTarget::~TuningTarget() = default;

void TuningTarget::validate(std::string_view candidate_text, const std::string& source_name,
	const GameData* context, bool checks_levels)
{
	this->candidate_.reset();
	auto candidate = std::make_unique<GameTuning>(
		labrador::parse_json(candidate_text, source_name));
	if (checks_levels && can_check_levels(context))
	{
		check_levels(*candidate, *this->presentation_, *context);
	}
	else if (can_check_resources(context))
	{
		// Startup's checks of numbers against the loaded resources, without
		// building every stage.
		candidate->validate_resources(*context->render_resources(), *context->audio_resources());
		validate_mode_durations(context->render_resources(), *candidate, *this->presentation_);
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

void PresentationTarget::validate(std::string_view candidate_text, const std::string& source_name,
	const GameData* context, bool checks_levels)
{
	this->candidate_.reset();
	// An edit of presentation.json is judged, and applied, in the run's
	// language: the locale's text still lies over the edited base.
	const ActiveLocale* locale = context != nullptr ? context->locale() : nullptr;
	auto candidate = std::make_unique<Presentation>(locale != nullptr ?
		locale->presentation(candidate_text, source_name) :
		labrador::parse_json(candidate_text, source_name));
	if (checks_levels && can_check_levels(context))
	{
		// The candidate is the base text, so its stage_names must still be
		// the levels' display_names.
		check_stage_names_agree(labrador::parse_json(candidate_text, source_name).root(),
			*context->stages());
		check_levels(*this->tuning_, *candidate, *context);
	}
	else if (can_check_resources(context))
	{
		// Frame sizes against each frame's native texels, and the countdown
		// box against its heading: startup's checks that need the resources.
		candidate->validate_resources(*context->render_resources(), *context->audio_resources());
		validate_controls_reminders(*candidate, *context->render_resources());
		validate_mode_durations(context->render_resources(), *this->tuning_, *candidate);
		validate_mode_stage_rules(context->render_resources(), *candidate);
		validate_mode_variants(context->render_resources(), *candidate);
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
