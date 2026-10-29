#include "game/pch.h"
#include "game/states/main_menu.h"
#include "game/series_stage_rule.h"
#include "game/states/match_start.h"
#include "game/states/end_menu.h"
#include "game/build_info.h"
#include "game/objects/crude_drawing.h"
#include "game/drawable_text.h"
#include "game/carried/ui_text_drop_shadow.h"

using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

MainMenuPage::MainMenuPage(MenuContext* context, std::optional<Session> between_rounds) :
	MenuPage(context), between_rounds_(std::move(between_rounds))
{
	const MainMenuPresentation& look = this->look();

	this->error_sound_ = this->resolve_sound(look.error_sound);
	this->ready_sound_ = this->resolve_sound(look.ready_sound);
	this->unready_sound_ = this->resolve_wave(look.unready_sound.name);
}

bool MainMenuPage::debug_start_match(const MatchSetup& plan)
{
	// Between rounds this frame belongs to the match, which takes the plan.
	if (this->between_rounds_) { return false; }
	this->menu_context()->cancel_music_exit();
	*this->level_settings() = plan;
	this->context()->pop(MatchStart::developer);
	return true;
}

bool MainMenuPage::debug_dismiss(bool leaving_match)
{
	if (!this->between_rounds_)
	{
		return MenuPage::debug_dismiss(leaving_match);
	}
	this->menu_context()->cancel_music_exit();
	this->context()->pop(leaving_match ? EndMenuAction::exit : EndMenuAction::restart);
	return true;
}

const std::optional<Session>& MainMenuPage::between_rounds() const
{
	return this->between_rounds_;
}

void MainMenuPage::return_to_round_menu()
{
	this->context()->transition_to(
		std::make_unique<EndMenuInitial>(this->menu_context(), *this->between_rounds_));
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

void MainMenuPage::set_player_prompts(MenuPrompt prompts)
{
	this->player_prompts_ = prompts;
	this->set_prompts(prompts);
}

int MainMenuPage::next_host_row(int row, Direction direction, bool joining) const
{
	if (direction != Direction::up && direction != Direction::down) { return row; }
	const int step = direction == Direction::up ? -1 : 1;
	const auto& players = this->level_settings()->player_settings();
	for (int candidate = row + step; candidate >= 0 && candidate < this->player_count(); candidate += step)
	{
		const auto& seat = players[static_cast<size_t>(candidate)];
		if (joining ? candidate > 0 && (seat.controller == PlayerController::bot || seat.num < 0) :
			candidate == 0 || seat.controller == PlayerController::bot)
		{
			return candidate;
		}
	}
	return row;
}

void MainMenuPage::route_host_setup_input(std::vector<ProcessedMenuInput>& inputs, int& row)
{
	if (inputs.empty() || this->level_settings()->bot_count() == 0) { return; }
	const ProcessedMenuInput host = inputs[0];
	inputs[0] = { Direction::none, MenuInputAction::none, host.connected };
	if (host.action == MenuInputAction::none &&
		(host.direction == Direction::up || host.direction == Direction::down))
	{
		const int next = this->next_host_row(row, host.direction, false);
		if (next != row) { this->play_wave(this->direction_sound_); row = next; }
		return;
	}
	inputs[static_cast<size_t>(row)] = host;
}

std::unique_ptr<UiTexture> MainMenuPage::make_host_cursor() const
{
	const MenuStyle& style = this->menu_style();
	return std::make_unique<UiTexture>("host_row", style.sprite_sheet, style.fill_frame,
		RectangleF(0.0f, 0.0f, style.rule_height, style.rule_height), this->render_resources(),
		CrudeDrawing::premultiplied(this->look().art.focus_ink));
}

void MainMenuPage::place_host_cursor(UiTexture& cursor, const UiWidget& row) const
{
	const RectangleF bounds = row.bounds();
	const MenuStyle& style = this->menu_style();
	// An underline identifies the host's row independently of readiness tint.
	cursor.set_position(Vector2F(bounds.x, bounds.bottom() + style.rule_height));
	cursor.set_size(Vector2F(bounds.width, style.rule_height));
}

bool MainMenuPage::wait_for_players(
	const std::vector<ProcessedMenuInput>& inputs)
{
	// Focus loss can temporarily hide every pad. It does not change the roster.
	if (!this->context()->active())
	{
		this->reset_menu_motion();
		return true;
	}
	const bool missing = std::any_of(inputs.begin(), inputs.end(),
		[](const ProcessedMenuInput& input) { return !input.connected; });
	if (missing != this->waiting_for_players_)
	{
		this->waiting_for_players_ = missing;
		if (missing)
		{
			this->set_prompts(MenuPrompt::missing_player);
		}
		else
		{
			this->set_prompts(this->player_prompts_);
		}
	}
	if (missing)
	{
		this->reset_menu_motion();
		// Any available device may recover a setup whose owner disconnected.
		for (const ProcessedMenuInput& input : this->menu_inputs())
		{
			if (input.action == MenuInputAction::back)
			{
				this->play_wave(this->cancel_sound_);
				// Between rounds the roster is the series': keep it.
				if (this->between_rounds_)
				{
					this->return_to_round_menu();
				}
				else
				{
					this->context()->transition_to(
						std::make_unique<MainMenuPlayerCount>(this->menu_context()));
				}
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
	this->set_prompts(MenuPrompt::title);
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background",
		style.sprite_sheet, style.fill_frame, RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(), look.title_background_colour);
	this->title_ = std::make_unique<UiText>("title", this->text().title.wordmark,
		look.title_font, Vector2F(0.0f, look.title_y), this->render_resources(), look.art.ink);
	this->subtitle_ = std::make_unique<UiText>("subtitle", this->text().title.subtitle,
		style.detail_font, Vector2F(0.0f, look.title_subtitle_y), this->render_resources(), look.art.ink);
	this->start_ = std::make_unique<UiText>("start", this->text().title.start,
		style.item_font, Vector2F(0.0f, look.title_start_y), this->render_resources(), look.art.focus_ink);
	this->no_pad_ = std::make_unique<UiText>("no_pad", this->text().title.no_controller,
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
		this->render_resources(), &style, &look.art, &this->focus_, this->start_.get(),
		&this->focus_motion_, &this->save()->play_settings());

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

MainMenuHome::MainMenuHome(MenuContext* context, InitialFocus initial_focus) :
	MainMenuPage(context), initial_focus_(initial_focus)
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
			if (this->activate_focus() == Activation::ran)
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
	this->set_prompts(MenuPrompt::select_back);
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->background_ = std::make_unique<UiTexture>("background",
		style.sprite_sheet, style.fill_frame, RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(), CrudeDrawing::premultiplied(look.home_background_colour));
	this->heading_ = std::make_unique<UiText>("heading", this->text().heading(MenuHeading::home),
		style.heading_font, this->calculate_widget_position(0, 0), this->render_resources(), look.art.ink);
	this->subtitle_ = std::make_unique<UiText>("subtitle", this->text().home_subtitle,
		style.detail_font, look.home_subtitle_position, this->render_resources(), look.art.ink);
	this->play_ = std::make_unique<UiText>("play", this->text().item(MenuItem::play), style.item_font,
		this->calculate_widget_position(0, 2), this->render_resources(), look.art.focus_ink);
	this->options_ = std::make_unique<UiText>("options", this->text().item(MenuItem::options), style.item_font,
		this->calculate_widget_position(0, 3), this->render_resources(), look.art.ink);
	this->credits_ = std::make_unique<UiText>("credits", this->text().item(MenuItem::credits), style.item_font,
		this->calculate_widget_position(0, 4), this->render_resources(), look.art.ink);
	this->exit_ = std::make_unique<UiText>("exit", this->text().item(MenuItem::exit), style.item_font,
		this->calculate_widget_position(0, 5), this->render_resources(), look.art.ink);
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.home_box_size) / 2.0f,
			look.home_box_size),
		this->render_resources(), &style, &look.art, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());

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
			this->play_wave(this->confirm_sound_);
			this->context()->push(std::make_unique<MainMenuExitConfirmation>(this->menu_context()));
		});

	// Child pages return to their entry row; fresh entry and Mode return use Play.
	switch (this->initial_focus_)
	{
	case InitialFocus::options:
		this->focus_.set_focused(0, this->options_.get());
		break;
	case InitialFocus::credits:
		this->focus_.set_focused(0, this->credits_.get());
		break;
	case InitialFocus::play:
		break;
	}
}

#pragma endregion MainMenuHome

#pragma region MainMenuModeSelect

MainMenuModeSelect::MainMenuModeSelect(MenuContext* context) :
	MainMenuPage(context)
{

}

std::unique_ptr<OptionRow> MainMenuModeSelect::make_setup_row(const std::string& name, MenuItem item,
	int row, RenderResources* render, const MenuStyle& style, const MainMenuPresentation& look,
	const MenuText& text, OptionRow::Adjust on_adjust)
{
	MenuStyle row_style = style;
	row_style.highlight = look.art.focus_ink;
	row_style.unhighlight = look.art.ink;
	row_style.disabled = look.art.disabled_ink;
	row_style.shadow_colour = Colour{};
	const float y = look.mode_select_spacing.y * static_cast<float>(row);
	return std::make_unique<OptionRow>(name, text.item(item),
		style.layout.position + Vector2F(0.0f, y),
		style.layout.position + Vector2F(look.mode_select_spacing.x * 2.0f, y),
		row_style, look.art.ink, render, std::move(on_adjust));
}

std::unique_ptr<OptionRow> MainMenuModeSelect::make_duration_row(RenderResources* render,
	const MenuStyle& style, const MainMenuPresentation& look, const MenuText& text,
	OptionRow::Adjust on_adjust)
{
	return make_setup_row("duration", MenuItem::match_length, DURATION_ROW, render, style, look, text,
		std::move(on_adjust));
}

std::unique_ptr<OptionRow> MainMenuModeSelect::make_stage_rule_row(RenderResources* render,
	const MenuStyle& style, const MainMenuPresentation& look, const MenuText& text,
	OptionRow::Adjust on_adjust)
{
	return make_setup_row("series_stage", MenuItem::series_stage, STAGE_RULE_ROW, render, style, look, text,
		std::move(on_adjust));
}

bool MainMenuModeSelect::setup_row_fits(const OptionRow& row_widget,
	const RectangleF& panel, const MenuPaperArtPresentation& art, float row_spacing)
{
	const float pen = art.keyline_width;
	const float shade = art.shadow_offset;
	const RectangleF inside(panel.position() + Vector2F(pen * 3.0f),
		panel.size() - Vector2F(pen * 6.0f + shade));
	RectangleF row = row_widget.bounds();
	row.inflate(art.focus_padding);
	const RectangleF arrow(row.x - pen * 7.0f, row.center().y - pen * 3.0f,
		pen * 5.0f, pen * 6.0f);
	const RectangleF ornament = RectangleF::union_of(arrow, RectangleF::union_of(row,
		RectangleF(row.position() + Vector2F(shade), row.size())));
	return std::isfinite(row.x) && std::isfinite(row.y) && std::isfinite(row.width) && std::isfinite(row.height) &&
		row.width > 0.0f && row.height > 0.0f && std::isfinite(row_spacing) && row_spacing > 0.0f &&
		inside.contains(ornament) && std::max(row.height + shade, pen * 6.0f) <= row_spacing;
}

void MainMenuModeSelect::check_duration_row(const OptionRow& duration_row,
	const RectangleF& panel, const MenuPaperArtPresentation& art, float row_spacing)
{
	if (!setup_row_fits(duration_row, panel, art, row_spacing))
	{
		throw std::runtime_error("main_menu.mode_select_spacing: the complete duration row must fit main_menu.setup_box_size.");
	}
}

void MainMenuModeSelect::check_stage_rule_row(const OptionRow& stage_rule_row,
	const RectangleF& panel, const MenuPaperArtPresentation& art, float row_spacing)
{
	if (!setup_row_fits(stage_rule_row, panel, art, row_spacing))
	{
		throw std::runtime_error("main_menu.mode_select_spacing: the complete series stage row must fit main_menu.setup_box_size.");
	}
}

void MainMenuModeSelect::refresh_duration()
{
	const std::optional<float> seconds = this->level_settings()->duration();
	const std::wstring value = this->text().setting_duration(
		seconds.value_or(this->data()->tuning()->match().duration), !seconds.has_value());
	const size_t missing = this->render_resources()->first_unrenderable(this->duration_font_, value);
	if (missing != std::wstring::npos)
	{
		throw std::runtime_error(std::string("menu_text.values.") +
			(seconds ? "duration_minutes" : "duration_default") + " cannot draw character at index " +
			std::to_string(missing) + " with the captured duration font '" + this->duration_font_name_ + "'.");
	}
	this->duration_row_->set_value(value);
	// The row, font, spacing, shadow and paper were captured at construction.
	// Only the default's numeric tuning remains live through later refreshes.
	check_duration_row(*this->duration_row_, this->paper_->rectangle(), this->setup_art_, this->duration_row_spacing_);
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

Vector2F MainMenuModeSelect::practice_hint_position(const UiWidget& single_match,
	const MenuPaperArtPresentation& art)
{
	const RectangleF label = single_match.bounds();
	return Vector2F(label.right() + art.focus_padding.x * 2.0f + art.shadow_offset, label.y);
}

void MainMenuModeSelect::refresh_stage_rule()
{
	const std::wstring& value = this->text().series_stage_value(this->level_settings()->series_stage_rule());
	const size_t missing = this->render_resources()->first_unrenderable(this->duration_font_, value);
	if (missing != std::wstring::npos)
	{
		throw std::runtime_error("menu_text.values.series_stage cannot draw character at index " +
			std::to_string(missing) + " with the captured item font '" + this->duration_font_name_ + "'.");
	}
	this->stage_rule_row_->set_value(value);
	check_stage_rule_row(*this->stage_rule_row_, this->paper_->rectangle(), this->setup_art_,
		this->duration_row_spacing_);
}

void MainMenuModeSelect::cycle_stage_rule(Direction direction)
{
	constexpr std::array<SeriesStageRule, 3> rules{
		SeriesStageRule::same, SeriesStageRule::rotate, SeriesStageRule::losers_pick };
	const auto found = std::find(rules.begin(), rules.end(), this->level_settings()->series_stage_rule());
	size_t slot = found == rules.end() ? 0 : static_cast<size_t>(found - rules.begin());
	slot = direction == Direction::left ? (slot + rules.size() - 1) % rules.size() : (slot + 1) % rules.size();
	this->level_settings()->set_series_stage_rule(rules[slot]);
	this->refresh_stage_rule();
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
			if (this->activate_focus() == Activation::ran)
			{
				return;
			}
		}
		else if (this->focus_.move(0, input.direction))
		{
			this->play_wave(this->direction_sound_);
		}
		else if ((this->focus_.focused(0) == this->duration_row_.get() ||
			this->focus_.focused(0) == this->stage_rule_row_.get()) &&
			(input.direction == Direction::left || input.direction == Direction::right))
		{
			this->play_wave(this->direction_sound_);
			(this->focus_.focused(0) == this->duration_row_.get() ?
				this->duration_row_ : this->stage_rule_row_)->adjust(input.direction);
			this->press_focus();
		}
	}
}
void MainMenuModeSelect::init()
{
	const MainMenuPresentation& look = this->look();
	const MenuStyle& style = this->menu_style();
	this->setup_art_ = look.art;
	this->setup_art_.focus_padding = look.setup_focus_padding;
	this->setup_art_.shadow_offset = look.setup_shadow_offset;
	this->set_prompts(MenuPrompt::mode);

	// Every mode uses the same player setup and paint scoring.
	this->focus_.set_style({ look.art.focus_ink, look.art.ink, look.art.disabled_ink });

	this->set_widget_spacing(look.mode_select_spacing);

	this->background_ = std::make_unique<UiTexture>(
		"background",
		style.sprite_sheet,
		style.fill_frame,
		RectangleF(Vector2F::ZERO, DESIGN_RESOLUTION),
		this->render_resources(),
		CrudeDrawing::premultiplied(look.home_background_colour));

	// Invisible shadows retain the existing font extents and navigation
	// geometry while the paper's lettering uses plain, readable ink.
	this->heading_ = std::make_unique<colourwars::UiTextDropShadow>(
		"heading",
		this->text().heading(MenuHeading::mode),
		style.heading_font,
		this->calculate_widget_position(0, 0),
		this->render_resources(),
		look.art.ink,
		Colour{},
		style.heading_shadow_offset);

	this->standard_ = std::make_unique<colourwars::UiTextDropShadow>(
		"standard",
		this->text().item(MenuItem::single_match),
		style.item_font,
		this->calculate_widget_position(0, 2),
		this->render_resources(),
		look.art.focus_ink,
		Colour{},
		style.item_shadow_offset);

	this->best_of_three_ = std::make_unique<colourwars::UiTextDropShadow>(
		"best_of_three",
		this->text().item(MenuItem::best_of_three),
		style.item_font,
		this->calculate_widget_position(0, 3),
		this->render_resources(),
		look.art.ink,
		Colour{},
		style.item_shadow_offset);

	this->best_of_five_ = std::make_unique<colourwars::UiTextDropShadow>(
		"best_of_five",
		this->text().item(MenuItem::best_of_five),
		style.item_font,
		this->calculate_widget_position(0, 4),
		this->render_resources(),
		look.art.ink,
		Colour{},
		style.item_shadow_offset);

	this->practice_ = std::make_unique<colourwars::UiTextDropShadow>(
		"practice",
		this->text().solo_practice_hint,
		style.detail_font,
		practice_hint_position(*this->standard_, this->setup_art_),
		this->render_resources(),
		look.art.ink,
		Colour{},
		style.item_shadow_offset);

	this->back_ = std::make_unique<colourwars::UiTextDropShadow>(
		"back",
		this->text().item(MenuItem::back),
		style.item_font,
		this->calculate_widget_position(0, BACK_ROW),
		this->render_resources(),
		look.art.ink,
		Colour{},
		style.item_shadow_offset);

	this->duration_font_name_ = style.item_font;
	this->duration_font_ = this->render_resources()->resolve_sprite_font(this->duration_font_name_);
	this->duration_row_spacing_ = look.mode_select_spacing.y;
	this->duration_row_ = make_duration_row(this->render_resources(), style, look, this->text(),
		[this](Direction direction) { this->cycle_duration(direction); });
	this->stage_rule_row_ = make_stage_rule_row(this->render_resources(), style, look, this->text(),
		[this](Direction direction) { this->cycle_stage_rule(direction); });
	this->paper_ = std::make_unique<MenuPaperArtwork>(
		RectangleF((Vector2F(DESIGN_RESOLUTION.x, style.footer_y) - look.setup_box_size) / 2.0f,
			look.setup_box_size),
		this->render_resources(), &style, &this->setup_art_, &this->focus_, nullptr,
		&this->focus_motion_, &this->save()->play_settings());
	this->refresh_duration();
	this->refresh_stage_rule();

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->background_.get());
	this->texture_container_->add_child(this->paper_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->standard_.get());
	this->text_container_->add_child(this->best_of_three_.get());
	this->text_container_->add_child(this->best_of_five_.get());
	this->text_container_->add_child(this->duration_row_.get());
	this->text_container_->add_child(this->stage_rule_row_.get());
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
	this->focus_.add(this->stage_rule_row_.get(), [this]
		{
			this->play_wave(this->direction_sound_);
			this->cycle_stage_rule(Direction::right);
		});

	this->focus_.add(this->back_.get(), [this]
		{
			this->play_wave(this->cancel_sound_);
			this->context()->transition_to(
				std::make_unique<MainMenuHome>(this->menu_context()));
		});
	if (this->level_settings()->best_of() == 3)
	{
		this->focus_.set_focused(0, this->best_of_three_.get());
	}
	else if (this->level_settings()->best_of() == 5)
	{
		this->focus_.set_focused(0, this->best_of_five_.get());
	}

}

#pragma endregion MainMenuModeSelect
