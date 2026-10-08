#include "game/pch.h"
#include "game/states/results_menu.h"

using namespace DirectX;
using namespace mattmath;
using namespace menu_consts;
using namespace labrador;

ResultsMenuPage::ResultsMenuPage(MenuContext* context,
	const LevelEndInfo& end_info) :
	MenuPage(context),
	level_end_info_(end_info)
{
	this->winner_sound_ = this->resolve_wave(this->look().winner_sound.name);
	this->fill_sound_ = this->resolve_effect(this->look().fill_sound.name);
}

const ResultsMenuPresentation& ResultsMenuPage::look() const
{
	return this->presentation().results_menu();
}

LevelEndInfo ResultsMenuPage::level_end_info() const
{
	return this->level_end_info_;
}

ResultsMenuInitial::ResultsMenuInitial(MenuContext* context,
	const LevelEndInfo& end_info, const Session& session) :
	ResultsMenuPage(context, end_info), session_(session)
{

}

void ResultsMenuInitial::update(float dt)
{
	const ResultsMenuPresentation& look = this->look();

	std::vector<ProcessedMenuInput> menu_inputs = this->menu_inputs();
	

	if (this->delay_timer_ < look.team_fill_delay)
	{
		this->delay_timer_ += dt;
	}
	else
	{		
		this->update_fill_box();
		if (this->fill_timer_ >= look.team_fill_time)
		{
			if (!this->winner_sound_played_)
			{
				this->winner_sound_played_ = true;
				this->stop_effect(this->fill_sound_, true);
				this->play_wave(this->winner_sound_, look.winner_sound.volume);
			}
			// Reveal percentages only for teams that participated.
			if (this->level_end_info().team_a_played)
			{
				this->team_a_percentage_->set_hidden(false);
			}
			if (this->level_end_info().team_b_played)
			{
				this->team_b_percentage_->set_hidden(false);
			}
			this->winner_->set_hidden(false);
			for (const std::unique_ptr<UiTextDropShadow>& detail : this->details_)
			{
				detail->set_hidden(false);
			}
			
			if (this->show_results_timer_ >= look.show_results_delay)
			{
				this->proceed_->set_hidden(false);
				int continue_input = this->check_for_continue_input(menu_inputs);

				if (continue_input != -1)
				{
					this->play_wave(this->confirm_sound_);

					this->context()->pop();
				}
			}
			this->show_results_timer_ += dt;
		}
		this->fill_timer_ += dt;
	}
}

void ResultsMenuInitial::init()
{
	const MenuStyle& style = this->menu_style();
	const ResultsMenuPresentation& look = this->look();

	const LevelEndInfo end_info = this->level_end_info();

	this->box_ = std::make_unique<UiTexture>(
		"box",
		"sprite_sheet_1",
		"pixel",
		RectangleF(Vector2F::ZERO, look.box_size),
		this->render_resources(),
		look.box_colour);
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"title",
		this->session_.is_series() ? L"Round " +
			std::to_wstring(this->session_.round()) : L"Results",
		style.heading_font,
		look.heading_position,
		this->render_resources(),
		look.heading_colour,
		style.shadow_colour,
		style.heading_shadow_offset);

	this->fill_box_ = std::make_unique<UiTexture>(
		"fill_box",
		"sprite_sheet_1",
		"results_menu_fill_box",
		RectangleF(Vector2F::ZERO, look.fill_box_size),
		this->render_resources(),
		look.box_colour);
	auto fill_box_position = Vector2F(
		this->calculate_center_position(
		this->fill_box_->rectangle().width, DESIGN_RESOLUTION.x),
		this->box_->rectangle().position().y + look.fill_box_y_offset);
	this->fill_box_->set_position(fill_box_position);

	this->team_a_fill_ = std::make_unique<UiTexture>(
		"team_a_fill",
		"sprite_sheet_1",
		"results_menu_team_a_fill",
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, look.team_fill_size.y)),
		this->render_resources(),
		this->level_end_info().team_colours.team_a);
	this->team_a_fill_->set_position(this->fill_box_->rectangle().position() +
		look.team_fill_border);

	this->team_b_fill_ = std::make_unique<UiTexture>(
		"team_b_fill",
		"sprite_sheet_1",
		"results_menu_team_b_fill",
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, look.team_fill_size.y)),
		this->render_resources(),
		this->level_end_info().team_colours.team_b);

	this->team_a_percentage_ = std::make_unique<UiTextDropShadow>(
		"team_a_percentage",
		end_info.team_a_percentage_string(),
		style.detail_font,
		this->team_a_fill_->rectangle().position(),
		this->render_resources(),
		look.percentage_colour,
		style.shadow_colour,
		style.detail_shadow_offset,
		true);

	this->team_b_percentage_ = std::make_unique<UiTextDropShadow>(
		"team_b_percentage",
		end_info.team_b_percentage_string(),
		style.detail_font,
		Vector2F::ZERO,
		this->render_resources(),
		look.percentage_colour,
		style.shadow_colour,
		style.detail_shadow_offset,
		true);
	this->team_b_percentage_->set_position(Vector2F(
		this->calculate_center_position(this->widget_size().x, DESIGN_RESOLUTION.x) +
		look.team_b_percentage_x_offset,
		this->calculate_team_b_fill_top_right_position().y));

	this->winner_ = std::make_unique<UiTextDropShadow>(
		"winner",
		end_info.winning_team_string(),
		style.item_font,
		look.winner_position,
		this->render_resources(),
		look.heading_colour,
		style.shadow_colour,
		style.detail_shadow_offset,
		true);

	this->proceed_ = std::make_unique<UiTextDropShadow>(
		"proceed",
		L"Press A/Enter to proceed",
		style.detail_font,
		look.proceed_position,
		this->render_resources(),
		look.heading_colour,
		style.shadow_colour,
		style.detail_shadow_offset,
		true);

	// Fixed A/B marks label the bars throughout the fill animation.
	const RectangleF fill_box_rect = this->fill_box_->rectangle();
	const float mark_y = fill_box_rect.position().y +
		(fill_box_rect.height - look.team_mark_size.y) / 2.0f;

	this->team_a_mark_ = std::make_unique<UiTexture>(
		"team_a_mark",
		"sprite_sheet_1",
		"team_select_a",
		RectangleF(Vector2F(
			fill_box_rect.position().x - look.team_mark_gap -
				look.team_mark_size.x,
			mark_y),
			look.team_mark_size),
		this->render_resources(),
		Colour::white);

	this->team_b_mark_ = std::make_unique<UiTexture>(
		"team_b_mark",
		"sprite_sheet_1",
		"team_select_b",
		RectangleF(Vector2F(
			fill_box_rect.position().x + fill_box_rect.width +
				look.team_mark_gap,
			mark_y),
			look.team_mark_size),
		this->render_resources(),
		Colour::white);

	// Hide bars and marks for absent teams, regardless of animated width.
	this->team_a_fill_->set_hidden(!end_info.team_a_played);
	this->team_b_fill_->set_hidden(!end_info.team_b_played);
	this->team_a_mark_->set_hidden(!end_info.team_a_played);
	this->team_b_mark_->set_hidden(!end_info.team_b_played);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());
	this->texture_container_->add_child(this->fill_box_.get());
	this->texture_container_->add_child(this->team_a_fill_.get());
	this->texture_container_->add_child(this->team_b_fill_.get());
	this->texture_container_->add_child(this->team_a_mark_.get());
	this->texture_container_->add_child(this->team_b_mark_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->team_a_percentage_.get());
	this->text_container_->add_child(this->team_b_percentage_.get());
	this->text_container_->add_child(this->winner_.get());
	this->text_container_->add_child(this->proceed_.get());

	const auto add_detail = [this, &style, &look](const std::wstring& text,
		const Vector2F& position, const std::string& font)
		{
			this->details_.push_back(std::make_unique<UiTextDropShadow>(
				"detail_" + std::to_string(this->details_.size()), text, font,
				position, this->render_resources(), look.heading_colour,
				style.shadow_colour, style.detail_shadow_offset, true));
			this->text_container_->add_child(this->details_.back().get());
		};
	if (this->session_.is_series())
	{
		add_detail(this->session_.score_string(),
			look.session_score_position, style.detail_font);
		add_detail(this->session_.result_string(),
			look.session_result_position, style.detail_font);
	}
	add_detail(L"Surface held (% of arena)",
		look.contribution_position, look.contribution_font);
	const std::vector<MenuPlayerSettings> players =
		this->level_settings()->player_settings();
	for (size_t i = 0; i < players.size(); ++i)
	{
		const MenuPlayerSettings& player = players[i];
		add_detail(this->level_settings()->player_label(static_cast<int>(i)) +
			(player.team == PlayerTeam::a ? L" | Team A | " : L" | Team B | ") +
			end_info.player_percentage_string(player.num),
			look.contribution_position + Vector2F(0.0f,
				look.contribution_spacing * static_cast<float>(i + 1)), look.contribution_font);
	}

	this->play_effect(this->fill_sound_, true, look.fill_sound.volume);
}


void ResultsMenuInitial::update_fill_box() const
{
	this->update_team_a_fill();
	this->update_team_b_fill();
}

float ResultsMenuInitial::fill_time_ratio() const
{
	const ResultsMenuPresentation& look = this->look();

	return this->fill_timer_ / look.team_fill_time;
}

void ResultsMenuInitial::update_team_a_fill() const
{
	const ResultsMenuPresentation& look = this->look();

	float score = this->level_end_info().team_a_ratio();
	float max_width = look.team_fill_size.x * score;
	float fill = this->fill_time_ratio();
	if (fill > 1.0f)
	{
		fill = 1.0f;
	}
	float width = fill * max_width;
	this->team_a_fill_->set_width(width);
}

void ResultsMenuInitial::update_team_b_fill() const
{
	const ResultsMenuPresentation& look = this->look();

	float score = this->level_end_info().team_b_ratio();
	float max_width = look.team_fill_size.x * score;
	float fill = this->fill_time_ratio();
	if (fill > 1.0f)
	{
		fill = 1.0f;
	}
	float width = fill * max_width;
	this->team_b_fill_->set_width(width);
	this->team_b_fill_->set_position_from_top_right_origin(
		this->calculate_team_b_fill_top_right_position());
}

Vector2F ResultsMenuInitial::calculate_team_b_fill_top_right_position() const
{
	const ResultsMenuPresentation& look = this->look();

	Vector2F team_b_top_right_position = this->fill_box_->rectangle().position() +
		Vector2F(this->fill_box_->rectangle().width, 0.0f) +
			Vector2F(-look.team_fill_border.x,
				look.team_fill_border.y);
	return team_b_top_right_position;
}

int ResultsMenuInitial::check_for_continue_input(
	const std::vector<ProcessedMenuInput>& menu_input)
{
	for (int i = 0; i < menu_input.size(); i++)
	{
		if (menu_input[i].action == MenuInputAction::proceed)
		{
			return i;
		}
	}
	return -1;
}
