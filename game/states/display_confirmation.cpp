#include "game/pch.h"
#include "game/states/display_confirmation.h"
#include "game/objects/crude_drawing.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

using namespace labrador;
using namespace mattmath;

namespace
{
	// Window callbacks may cancel reentrantly. They must never close this
	// frame until the outer display operation has finished returning.
	class TrialOperation
	{
	public:
		explicit TrialOperation(bool& applying) :
			applying_(applying), previous_(std::exchange(applying, true)) {}
		~TrialOperation() { this->applying_ = this->previous_; }
	private:
		bool& applying_;
		bool previous_;
	};
}

MainMenuDisplayConfirmation::MainMenuDisplayConfirmation(MenuContext* context,
	DisplaySettingsChoice candidate, std::optional<int> pausing_slot) :
	MenuPage(context), candidate_(candidate), pausing_slot_(pausing_slot),
	look_(context->data()->presentation()->pause_menu()),
	timeout_(context->data()->presentation()->main_menu().display_trial_seconds)
{
	if (this->pausing_slot_ &&
		((!input_slots::is_pad(*this->pausing_slot_) && !input_slots::is_keyboard_and_mouse(*this->pausing_slot_)) ||
			this->level_settings()->player_index_for_slot(*this->pausing_slot_) < 0))
	{
		throw std::invalid_argument("Display confirmation owner must be a valid roster input slot.");
	}
	this->look_.art = menu_geometry::display_art(this->look_,
		context->data()->presentation()->main_menu());
}

MainMenuDisplayConfirmation::~MainMenuDisplayConfirmation()
{
	// Roll back while the rest of this page still exists. The core callback
	// captures the application, never this child or its state context.
	this->destructing_ = true;
	this->trial_.reset();
}

void MainMenuDisplayConfirmation::init()
{
	this->input()->reset_actions();
	const MenuStyle& style = this->menu_style();
	const PauseMenuPresentation& look = this->look_;
	this->set_prompts(MenuPrompt::select_back);
	this->set_widget_position(look.layout.position);
	this->set_widget_spacing(look.layout.spacing);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background", style.sprite_sheet,
		style.fill_frame, menu_geometry::design_area(), this->render_resources(),
		CrudeDrawing::premultiplied(this->presentation().main_menu().home_background_colour));
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		menu_geometry::screen_paper(look.box_size),
		this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->settings_session()->play_settings());
	this->heading_ = std::make_unique<UiText>("heading", this->text().heading(MenuHeading::options),
		style.heading_font, Vector2F(0.0f, look.heading_y), this->render_resources(), look.art.ink);
	this->heading_->set_position(Vector2F(
		menu_geometry::centred_x(this->heading_->bounds().width), look.heading_y));
	this->question_ = std::make_unique<UiText>("question", this->text().confirmations.display,
		style.detail_font, Vector2F(0.0f, look.status_y), this->render_resources(), look.art.ink);
	this->question_->set_position(Vector2F(
		menu_geometry::centred_x(this->question_->bounds().width), look.status_y));
	this->keep_ = std::make_unique<UiText>("keep", this->text().item(MenuItem::keep), style.item_font,
		this->calculate_widget_position(0, 0), this->render_resources(), look.art.ink);
	this->revert_ = std::make_unique<UiText>("revert", this->text().item(MenuItem::revert), style.item_font,
		this->calculate_widget_position(0, 1), this->render_resources(), look.art.focus_ink);
	this->countdown_ = std::make_unique<UiText>("countdown", L"", style.detail_font,
		Vector2F(0.0f, this->calculate_widget_position(0, 2).y), this->render_resources(), look.art.ink);
	this->refresh_countdown();

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->question_.get());
	this->text_container_->add_child(this->keep_.get());
	this->text_container_->add_child(this->revert_.get());
	this->text_container_->add_child(this->countdown_.get());
	this->focus_.add(this->revert_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->cancel_trial();
		});
	this->focus_.add(this->keep_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			const TrialOperation operation(this->applying_);
			this->trial_->keep();
		});

	Application* application = this->data()->application();
	this->trial_ = std::make_unique<DisplaySettingsTrial>(this->save(), this->candidate_,
		application->fullscreen(), [application](ScreenResolution resolution, bool fullscreen)
		{
			if (application->window() == nullptr) { return; }
			std::exception_ptr failure;
			try { application->set_resolution(resolution); }
			catch (...) { failure = std::current_exception(); }
			try { application->set_fullscreen(fullscreen); }
			catch (...) { if (!failure) { failure = std::current_exception(); } }
			if (failure) { std::rethrow_exception(failure); }
		}, this->timeout_);
	{
		const TrialOperation operation(this->applying_);
		this->trial_->begin();
	}
	this->started_at_ = std::chrono::steady_clock::now();
	if (!this->context()->active()) { this->cancel_trial(); }
}

void MainMenuDisplayConfirmation::cancel_trial()
{
	if (this->destructing_) { return; }
	this->input()->reset_actions();
	this->reset_menu_motion();
	if (this->trial_)
	{
		const TrialOperation operation(this->applying_);
		this->trial_->cancel();
	}
}

bool MainMenuDisplayConfirmation::close_if_finished()
{
	if (this->covered_ || this->closing_ || this->applying_ || this->destructing_ || !this->trial_) { return false; }
	const std::optional<DisplayTrialResult> result = this->trial_->result();
	if (!result) { return false; }
	this->closing_ = true;
	this->input()->reset_actions();
	this->reset_menu_motion();
	this->context()->pop<DisplayTrialResult>(*result);
	return true;
}

void MainMenuDisplayConfirmation::refresh_countdown()
{
	const float remaining = this->trial_ ? this->trial_->remaining() : this->timeout_;
	// The text changes once a second; it is rebuilt only when the whole second does.
	const int seconds = static_cast<int>(std::ceil(remaining));
	if (this->shown_seconds_ == seconds)
	{
		return;
	}
	this->countdown_->set_text(this->text().display_countdown(seconds));
	this->shown_seconds_ = seconds;
	this->countdown_->set_position(Vector2F(
		menu_geometry::centred_x(this->countdown_->bounds().width),
		this->calculate_widget_position(0, 2).y));
}

void MainMenuDisplayConfirmation::update_page(float dt)
{
	if (this->close_if_finished()) { return; }
	if (!this->context()->active())
	{
		this->cancel_trial();
		this->close_if_finished();
		return;
	}
	if (std::isfinite(dt) && dt > 0.0f)
	{
		this->simulated_seconds_ += static_cast<double>(dt);
	}
	// Simulation ticks and real time are two views of one deadline. Comparing
	// their totals catches a stalled loop without charging the same time twice.
	const double wall_seconds = std::max(0.0,
		std::chrono::duration<double>(std::chrono::steady_clock::now() - this->started_at_).count());
	const double elapsed = std::max(this->simulated_seconds_, wall_seconds);
	const double advance = std::max(0.0, elapsed - this->advanced_seconds_);
	this->advanced_seconds_ = elapsed;
	if (advance > 0.0)
	{
		const TrialOperation operation(this->applying_);
		const float seconds = elapsed >= static_cast<double>(this->timeout_) ? this->trial_->remaining() :
			static_cast<float>(advance);
		this->trial_->advance(seconds);
	}
	if (this->close_if_finished()) { return; }
	this->refresh_countdown();
	std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	if (this->pausing_slot_)
	{
		inputs = { menu_input_for_owner(inputs, *this->pausing_slot_, *this->level_settings()) };
	}
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->cancel_trial();
			this->close_if_finished();
			return;
		}
	}
	for (const ProcessedMenuInput& input : inputs)
	{
		if (input.action == MenuInputAction::proceed && this->activate_focus() == Activation::ran)
		{
			this->close_if_finished();
			return;
		}
		if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
			return;
		}
	}
}

void MainMenuDisplayConfirmation::on_suspend()
{
	if (this->destructing_) { return; }
	this->covered_ = true;
	MenuPage::on_suspend();
	this->cancel_trial();
}

void MainMenuDisplayConfirmation::on_resume()
{
	if (this->destructing_) { return; }
	this->covered_ = false;
	MenuPage::on_resume();
	this->input()->reset_actions();
	this->close_if_finished();
}

void MainMenuDisplayConfirmation::on_deactivated()
{
	if (this->destructing_) { return; }
	MenuPage::on_deactivated();
	this->cancel_trial();
}

bool MainMenuDisplayConfirmation::debug_dismiss(bool /*leaving_match*/)
{
	if (this->destructing_) { return false; }
	this->cancel_trial();
	return this->close_if_finished();
}

void MainMenuDisplayConfirmation::debug_reset_input(int pad_slot)
{
	if (this->destructing_) { return; }
	MenuPage::debug_reset_input(pad_slot);
	this->cancel_trial();
}
