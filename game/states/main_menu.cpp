#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/build_info.h"
#include <sstream>

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

MainMenuPage::MainMenuPage(MenuContext* context) :
	MenuPage(context)
{
	const MainMenuPresentation& look = this->look();

	this->error_sound_ = this->resolve_wave(look.error_sound);
	this->ready_sound_ = this->resolve_wave(look.ready_sound);
	this->unready_sound_ = this->resolve_wave(look.unready_sound.name);
}

bool MainMenuPage::debug_start_match(const MenuLevelSettings& plan)
{
	this->menu_context()->cancel_music_exit();
	*this->level_settings() = plan;
	this->context()->pop();
	return true;
}

const MainMenuPresentation& MainMenuPage::look() const
{
	return this->presentation().main_menu();
}

int MainMenuPage::player_count() const
{ 
	return this->level_settings()->
		player_count();
}

std::vector<int> MainMenuPage::connected_slots() const
{
	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	std::vector<int> result;
	for (int slot = 0; slot < static_cast<int>(inputs.size()); slot++)
	{
		if (inputs[slot].connected)
		{
			result.push_back(slot);
		}
	}
	return result;
}

void MainMenuPage::set_player_prompts(
	const std::vector<PromptBar::Prompt>& prompts)
{
	this->player_prompts_ = prompts;
	this->set_prompts(prompts);
}

bool MainMenuPage::wait_for_players(
	const std::vector<ProcessedMenuInput>& inputs)
{
	// Focus loss can temporarily hide every pad. It does not change the roster.
	if (!this->context()->active())
	{
		return true;
	}
	const bool missing = std::any_of(inputs.begin(), inputs.end(),
		[](const ProcessedMenuInput& input) { return !input.connected; });
	if (missing != this->waiting_for_players_)
	{
		this->waiting_for_players_ = missing;
		if (missing)
		{
			this->set_prompts({
				{ L"", L"Reconnect controller" },
				{ L"B", L"Change players" } });
		}
		else
		{
			this->set_prompts(this->player_prompts_);
		}
	}
	if (missing)
	{
		// Any available device may recover a setup whose owner disconnected.
		for (const ProcessedMenuInput& input : this->menu_inputs())
		{
			if (input.action == MenuInputAction::back)
			{
				this->play_wave(this->cancel_sound_);
				this->context()->transition_to(
					std::make_unique<MainMenuPlayerCount>(this->menu_context()));
				break;
			}
		}
	}
	return missing;
}

#pragma region MainMenuTitle

MainMenuTitle::MainMenuTitle(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuTitle::update_page(float /*dt*/)
{
	if (this->context()->active())
	{
		this->no_pad_->set_hidden(!this->connected_slots().empty());
	}

	const std::vector<ProcessedMenuInput> inputs = this->menu_inputs();
	for (int i = 0; i < inputs.size(); i++)
	{
		if (inputs[i].action == MenuInputAction::proceed)
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(std::make_unique<MainMenuHome>(
				this->menu_context()));
			return;
		}
	}
}
void MainMenuTitle::init()
{
	this->set_prompts({ { L"A", L"Start" } });
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background",
		"sprite_sheet_1", "pixel", RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(), look.title_background_colour);
	this->title_ = std::make_unique<UiText>("title", L"COLOUR\nWARS",
		look.title_font, Vector2F(0.0f, look.title_y), this->render_resources(), look.art.ink);
	this->subtitle_ = std::make_unique<UiText>("subtitle", L"MAKE A MESS. MAKE IT YOURS.",
		style.detail_font, Vector2F(0.0f, look.title_subtitle_y), this->render_resources(), look.art.ink);
	this->start_ = std::make_unique<UiText>("start", L"LET'S PLAY",
		style.item_font, Vector2F(0.0f, look.title_start_y), this->render_resources(), look.art.focus_ink);
	this->no_pad_ = std::make_unique<UiText>("no_pad", L"Connect a controller",
		style.item_font, Vector2F(0.0f, look.title_no_pad_y), this->render_resources(), look.art.ink, true);
	this->build_id_ = std::make_unique<UiText>("build_id", build_info::label,
		this->presentation().prompt_bar().font, Vector2F(0.0f, look.build_id_y),
		this->render_resources(), look.art.ink);

	// Fit the real font metrics, keeping the authored vertical rhythm.
	for (UiText* text : { this->title_.get(), this->subtitle_.get(), this->start_.get(),
		this->no_pad_.get(), this->build_id_.get() })
	{
		text->set_position(Vector2F(this->calculate_center_position(text->bounds().width,
			DESIGN_RESOLUTION.x), text->bounds().y));
	}
	this->focus_.add(this->start_.get());
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.title_box_size) / 2.0f,
			look.title_box_size),
		this->render_resources(), &look.art, &this->focus_, this->start_.get());

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->title_.get());
	this->text_container_->add_child(this->subtitle_.get());
	this->text_container_->add_child(this->start_.get());
	this->text_container_->add_child(this->no_pad_.get());
	this->text_container_->add_child(this->build_id_.get());
}

#pragma endregion MainMenuTitle

#pragma region MainMenuHome

MainMenuHome::MainMenuHome(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuHome::update_page(float /*dt*/)
{
	// Read focus for each device so later inputs see earlier navigation
	// from the same frame.
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuTitle>(this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			// Stop after one activation: every action here leaves the page.
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
	}
}
void MainMenuHome::init()
{
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->set_widget_position(look.home_position);
	this->set_widget_spacing(look.home_spacing);
	this->set_prompts({ { L"A", L"Select" }, { L"B", L"Back" } });
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background",
		"sprite_sheet_1", "pixel", RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(), look.home_background_colour);
	this->heading_ = std::make_unique<UiText>("heading", L"Make your mark.",
		style.heading_font, this->calculate_widget_position(0, 0), this->render_resources(), look.art.ink);
	this->subtitle_ = std::make_unique<UiText>("subtitle", L"Paint the arena. Claim the win.",
		style.detail_font, look.home_subtitle_position, this->render_resources(), look.art.ink);
	this->play_ = std::make_unique<UiText>("play", L"Play", style.item_font,
		this->calculate_widget_position(0, 2), this->render_resources(), look.art.focus_ink);
	this->options_ = std::make_unique<UiText>("options", L"Options", style.item_font,
		this->calculate_widget_position(0, 3), this->render_resources(), look.art.ink);
	this->credits_ = std::make_unique<UiText>("credits", L"Credits", style.item_font,
		this->calculate_widget_position(0, 4), this->render_resources(), look.art.ink);
	this->exit_ = std::make_unique<UiText>("exit", L"Exit", style.item_font,
		this->calculate_widget_position(0, 5), this->render_resources(), look.art.ink);
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.home_box_size) / 2.0f,
			look.home_box_size),
		this->render_resources(), &look.art, &this->focus_, this->play_.get());

	this->texture_container_ = std::make_unique<UiContainer>("texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());
	this->text_container_ = std::make_unique<UiContainer>("text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->subtitle_.get());
	this->text_container_->add_child(this->play_.get());
	this->text_container_->add_child(this->options_.get());
	this->text_container_->add_child(this->credits_.get());
	this->text_container_->add_child(this->exit_.get());

	// Navigation uses widget bounds in design space, not declaration order.
	this->focus_.add(this->play_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuModeSelect>(this->menu_context()));
		});
	this->focus_.add(this->options_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuOptions>(this->menu_context()));
		});
	this->focus_.add(this->credits_.get(), [this]
		{
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuCredits>(this->menu_context()));
		});
	this->focus_.add(this->exit_.get(), [this]
		{
			this->menu_context()->finish_with_music([application = this->data()->application()]
			{
				application->quit();
			});
		});

}

#pragma endregion MainMenuHome

#pragma region MainMenuModeSelect

MainMenuModeSelect::MainMenuModeSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

void MainMenuModeSelect::refresh_duration()
{
	const std::optional<float> seconds = this->level_settings()->duration();
	std::wostringstream label;
	label << seconds.value_or(this->data()->tuning()->match().duration) / 60.0f << L" min";
	const std::wstring time = label.str();
	this->duration_row_->set_value(seconds ? time : L"Default (" + time + L")");
}

void MainMenuModeSelect::cycle_duration(Direction direction)
{
	const std::vector<float>& choices = this->data()->tuning()->match().duration_choices;
	const std::optional<float> current = this->level_settings()->duration();
	size_t slot = 0;
	for (size_t index = 0; index < choices.size(); ++index)
	{
		if (current && *current == choices[index]) { slot = index + 1; break; }
	}
	const size_t count = choices.size() + 1;
	slot = direction == Direction::left ? (slot + count - 1) % count : (slot + 1) % count;
	this->level_settings()->set_duration(slot == 0 ? std::nullopt :
		std::optional<float>(choices[slot - 1]));
	this->refresh_duration();
}

void MainMenuModeSelect::update_page(float /*dt*/)
{
	for (const ProcessedMenuInput& input : this->menu_inputs())
	{
		if (input.action == MenuInputAction::back)
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(std::make_unique<MainMenuHome>(
				this->menu_context()));
			return;
		}
		if (input.action == MenuInputAction::proceed)
		{
			if (this->focus_.activate(0) == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
		else if (this->focus_.focused(0) == this->duration_row_.get() &&
			(input.direction == Direction::left || input.direction == Direction::right))
		{
			this->play_wave(this->direction_sound_);
			this->duration_row_->adjust(input.direction);
		}
	}
}
void MainMenuModeSelect::init()
{
	this->set_prompts({
		{ L"A", L"Select" },
		{ L"Left/Right", L"Match length" },
		{ L"B", L"Back" } });

	// Every mode uses the same player setup and paint scoring.
	this->focus_.set_style({ this->menu_style().highlight,
		this->menu_style().unhighlight,
		this->menu_style().disabled });

	this->set_widget_spacing(this->look().mode_select_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		this->look().backdrop_sheet,
		this->look().backdrop_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		this->look().play_background_colour);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"heading",
		L"Mode Select",
		this->menu_style().heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().heading_shadow_offset);

	this->standard_ = std::make_unique<UiTextDropShadow>(
		"standard",
		L"Single Match",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		this->menu_style().highlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->best_of_three_ = std::make_unique<UiTextDropShadow>(
		"best_of_three",
		L"Best of 3",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->best_of_five_ = std::make_unique<UiTextDropShadow>(
		"best_of_five",
		L"Best of 5",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->practice_ = std::make_unique<UiTextDropShadow>(
		"practice",
		L"Solo practice: choose Single Match",
		this->menu_style().detail_font,
		this->calculate_widget_position(0, 7),
		this->render_resources(),
		this->menu_style().heading_colour,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->back_ = std::make_unique<UiTextDropShadow>(
		"back",
		L"Back",
		this->menu_style().item_font,
		this->calculate_widget_position(0, 6),
		this->render_resources(),
		this->menu_style().unhighlight,
		this->menu_style().shadow_colour,
		this->menu_style().item_shadow_offset);

	this->duration_row_ = std::make_unique<OptionRow>("duration", L"Match length",
		this->calculate_widget_position(0, 5), this->calculate_widget_position(2, 5),
		this->menu_style(), this->look().options_value_colour, this->render_resources(),
		[this](Direction direction) { this->cycle_duration(direction); });
	this->refresh_duration();

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->standard_.get());
	this->text_container_->add_child(this->best_of_three_.get());
	this->text_container_->add_child(this->best_of_five_.get());
	this->text_container_->add_child(this->duration_row_.get());
	this->text_container_->add_child(this->practice_.get());
	this->text_container_->add_child(this->back_.get());

	const auto choose = [this](int best_of)
		{
			this->level_settings()->set_best_of(best_of);
			this->play_wave(this->confirm_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuPlayerCount>(this->menu_context()));
		};
	this->focus_.add(this->standard_.get(), [choose] { choose(1); });
	this->focus_.add(this->best_of_three_.get(), [choose] { choose(3); });
	this->focus_.add(this->best_of_five_.get(), [choose] { choose(5); });
	this->focus_.add(this->duration_row_.get(), [this]
		{
			this->play_wave(this->direction_sound_);
			this->cycle_duration(Direction::right);
		});

	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});

}

#pragma endregion MainMenuModeSelect
