#include "game/pch.h"
#include "game/states/menu_panel.h"
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
	this->winner_sound_ = this->resolve_wave(end_info.winning_team() == PlayerTeam::none
		? this->look().draw_sound.name : this->look().winner_sound.name);
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
	const LevelEndInfo& end_info, const Session& session, bool revealed,
	const RecordOutcome& record) :
	ResultsMenuPage(context, end_info), session_(session), revealed_(revealed),
	record_(record)
{

}

std::wstring ResultsMenuInitial::record_text(const RecordOutcome& record,
	const MenuRecordText& text)
{
	if (!record.recorded)
	{
		return std::wstring();
	}
	if (record.new_best && record.best)
	{
		return text.new_best + L" " + percentage_text(*record.best);
	}
	if (record.best)
	{
		return text.stage_best + L" " + percentage_text(*record.best);
	}
	return std::wstring();
}

bool ResultsMenuInitial::debug_dismiss(bool /*leaving_match*/)
{
	// The fill loop stops on its own only once the reveal completes.
	this->stop_effect(this->fill_sound_, true);
	this->context()->pop();
	return true;
}

void ResultsMenuInitial::update_page(float dt)
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
				this->play_wave(this->winner_sound_,
					this->level_end_info().winning_team() == PlayerTeam::none
						? look.draw_sound.volume : look.winner_sound.volume);
			}
			// Reveal percentages only for teams that participated.
			if (this->level_end_info().team_a_played)
			{
				this->team_a_percentage_panel_->set_hidden(false);
				this->team_a_percentage_->set_hidden(false);
			}
			if (this->level_end_info().team_b_played)
			{
				this->team_b_percentage_panel_->set_hidden(false);
				this->team_b_percentage_->set_hidden(false);
			}
			// The reveal clicked the winner; a draw still needs its line.
			this->winner_->set_hidden(this->revealed_ &&
				this->level_end_info().winning_team() != PlayerTeam::none);
			for (const std::unique_ptr<UiTextDropShadow>& detail : this->details_)
			{
				detail->set_hidden(false);
			}
			for (const auto& badge : this->player_badges_)
			{
				badge->set_hidden(false);
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

	this->box_ = std::make_unique<MenuPaperArtwork>(
		RectangleF(Vector2F::ZERO, look.box_size),
		this->render_resources(), &style, &look.art, &this->focus_);
	this->box_->set_position_at_center(DESIGN_RESOLUTION / 2.0f);
	const auto center_text = [](UiTextDropShadow& text, float y)
	{
		text.set_position(Vector2F((DESIGN_RESOLUTION.x - text.bounds().width) / 2.0f, y));
	};

	this->heading_ = std::make_unique<UiTextDropShadow>(
		"title",
		this->session_.is_series() ? this->text().results.round_title(this->session_.round()) :
			this->text().results.single_match_heading,
		style.heading_font,
		Vector2F::ZERO,
		this->render_resources(),
		look.art.ink,
		Colour(),
		Vector2F::ZERO);
	center_text(*this->heading_, look.heading_y);

	this->fill_box_ = std::make_unique<UiTexture>(
		"fill_box",
		style.sprite_sheet,
		style.fill_frame,
		RectangleF(Vector2F::ZERO, look.fill_box_size),
		this->render_resources(),
		look.art.ink);
	auto fill_box_position = Vector2F(
		this->calculate_center_position(
		this->fill_box_->rectangle().width, DESIGN_RESOLUTION.x),
		this->box_->rectangle().position().y + look.fill_box_y_offset);
	this->fill_box_->set_position(fill_box_position);
	this->fill_track_ = std::make_unique<UiTexture>(
		"fill_track", style.sprite_sheet, style.fill_frame,
		RectangleF(fill_box_position + Vector2F(look.art.keyline_width),
			look.fill_box_size - Vector2F(look.art.keyline_width * 2.0f)),
		this->render_resources(), look.track_colour);

	this->team_a_fill_ = std::make_unique<UiTexture>(
		"team_a_fill",
		style.sprite_sheet,
		style.fill_frame,
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, look.team_fill_size.y)),
		this->render_resources(),
		this->level_end_info().team_colours.team_a);
	this->team_a_fill_->set_position(this->fill_box_->rectangle().position() +
		look.team_fill_border);

	this->team_b_fill_ = std::make_unique<UiTexture>(
		"team_b_fill",
		style.sprite_sheet,
		style.fill_frame,
		RectangleF(Vector2F::ZERO,
			Vector2F(0.0f, look.team_fill_size.y)),
		this->render_resources(),
		this->level_end_info().team_colours.team_b);

	this->team_a_percentage_ = std::make_unique<UiTextDropShadow>(
		"team_a_percentage",
		end_info.team_a_percentage_string(this->text().results),
		style.detail_font,
		this->team_a_fill_->rectangle().position(),
		this->render_resources(),
		look.art.ink,
		Colour(),
		Vector2F::ZERO,
		true);

	this->team_b_percentage_ = std::make_unique<UiTextDropShadow>(
		"team_b_percentage",
		end_info.team_b_percentage_string(this->text().results),
		style.detail_font,
		Vector2F::ZERO,
		this->render_resources(),
		look.art.ink,
		Colour(),
		Vector2F::ZERO,
		true);
	this->team_b_percentage_->set_position(Vector2F(
		this->calculate_team_b_fill_top_right_position().x -
			this->team_b_percentage_->bounds().width,
		this->calculate_team_b_fill_top_right_position().y));

	// Keep the score readable over every team palette, including yellow.
	// Reveal these plates with the numbers so the fill animation stays visible.
	RectangleF team_a_percentage_panel = this->team_a_percentage_->bounds();
	team_a_percentage_panel.inflate(look.art.focus_padding);
	this->team_a_percentage_panel_ = std::make_unique<MenuPaperArtwork>(
		team_a_percentage_panel, this->render_resources(), &style, &look.art, &this->focus_);
	this->team_a_percentage_panel_->set_hidden(true);
	RectangleF team_b_percentage_panel = this->team_b_percentage_->bounds();
	team_b_percentage_panel.inflate(look.art.focus_padding);
	this->team_b_percentage_panel_ = std::make_unique<MenuPaperArtwork>(
		team_b_percentage_panel, this->render_resources(), &style, &look.art, &this->focus_);
	this->team_b_percentage_panel_->set_hidden(true);

	this->winner_ = std::make_unique<UiTextDropShadow>(
		"winner",
		end_info.winning_team_string(this->text().results),
		style.item_font,
		Vector2F::ZERO,
		this->render_resources(),
		look.art.ink,
		Colour(),
		Vector2F::ZERO,
		true);
	center_text(*this->winner_, look.winner_y);

	this->proceed_ = std::make_unique<UiTextDropShadow>(
		"proceed",
		this->text().results.proceed,
		style.detail_font,
		Vector2F::ZERO,
		this->render_resources(),
		look.art.ink,
		Colour(),
		Vector2F::ZERO,
		true);
	center_text(*this->proceed_, look.proceed_y);

	// Fixed A/B marks label the bars throughout the fill animation.
	const RectangleF fill_box_rect = this->fill_box_->rectangle();
	const float mark_y = fill_box_rect.position().y +
		(fill_box_rect.height - look.team_mark_size.y) / 2.0f;

	const RectangleF team_a_mark_box(Vector2F(
			fill_box_rect.position().x - look.team_mark_gap -
				look.team_mark_size.x,
			mark_y), look.team_mark_size);
	this->team_a_mark_ = std::make_unique<UiTextDropShadow>(
		"team_a_mark", L"A", style.item_font, Vector2F::ZERO,
		this->render_resources(), look.art.ink,
		Colour(), Vector2F::ZERO);
	this->team_a_mark_->set_position(team_a_mark_box.center() -
		this->team_a_mark_->bounds().center());

	const RectangleF team_b_mark_box(Vector2F(
			fill_box_rect.position().x + fill_box_rect.width +
				look.team_mark_gap,
			mark_y), look.team_mark_size);
	this->team_b_mark_ = std::make_unique<UiTextDropShadow>(
		"team_b_mark", L"B", style.item_font, Vector2F::ZERO,
		this->render_resources(), look.art.ink,
		Colour(), Vector2F::ZERO);
	this->team_b_mark_->set_position(team_b_mark_box.center() -
		this->team_b_mark_->bounds().center());

	// Hide bars and marks for absent teams, regardless of animated width.
	this->team_a_fill_->set_hidden(!end_info.team_a_played);
	this->team_b_fill_->set_hidden(!end_info.team_b_played);
	this->team_a_mark_->set_hidden(!end_info.team_a_played);
	this->team_b_mark_->set_hidden(!end_info.team_b_played);

	this->texture_container_ = std::make_unique<UiContainer>(
		"texture_container");
	this->texture_container_->add_child(this->box_.get());
	this->texture_container_->add_child(this->fill_box_.get());
	this->texture_container_->add_child(this->fill_track_.get());
	this->texture_container_->add_child(this->team_a_fill_.get());
	this->texture_container_->add_child(this->team_b_fill_.get());
	this->texture_container_->add_child(this->team_a_percentage_panel_.get());
	this->texture_container_->add_child(this->team_b_percentage_panel_.get());

	this->text_container_ = std::make_unique<UiContainer>(
		"text_container");
	this->text_container_->add_child(this->heading_.get());
	this->text_container_->add_child(this->team_a_mark_.get());
	this->text_container_->add_child(this->team_b_mark_.get());
	this->text_container_->add_child(this->team_a_percentage_.get());
	this->text_container_->add_child(this->team_b_percentage_.get());
	this->text_container_->add_child(this->winner_.get());
	this->text_container_->add_child(this->proceed_.get());

	const auto add_detail = [this, &look](const std::wstring& text,
		const Vector2F& position, const std::string& font)
		{
			this->details_.push_back(std::make_unique<UiTextDropShadow>(
				"detail_" + std::to_string(this->details_.size()), text, font,
				position, this->render_resources(), look.art.ink,
				Colour(), Vector2F::ZERO, true));
			this->text_container_->add_child(this->details_.back().get());
			return this->details_.back().get();
		};
	if (this->session_.is_series())
	{
		center_text(*add_detail(this->session_.score_string(this->text().results), Vector2F::ZERO, style.detail_font),
			look.session_score_y);
		center_text(*add_detail(this->session_.result_string(this->text().results), Vector2F::ZERO, style.detail_font),
			look.session_result_y);
	}
	add_detail(this->text().results.contribution_heading,
		look.contribution_position, look.contribution_font);
	const std::vector<MenuPlayerSettings> players =
		this->level_settings()->player_settings();
	for (size_t i = 0; i < players.size(); ++i)
	{
		const MenuPlayerSettings& player = players[i];
		add_detail(this->level_settings()->player_label(static_cast<int>(i), this->text()) +
			(player.team == PlayerTeam::a ? this->text().results.contribution_team_a :
				this->text().results.contribution_team_b) +
			end_info.player_percentage_string(paint_owners::index(player.num, static_cast<int>(i))),
			look.contribution_position + Vector2F(0.0f,
				look.contribution_spacing * static_cast<float>(i + 1)), look.contribution_font);
		this->player_badges_.push_back(std::make_unique<UiPlayerBadge>(
			"player_badge_" + std::to_string(i + 1),
			UiPlayerBadge::before(this->details_.back()->bounds(), this->presentation().player_identity()),
			static_cast<int>(i), &this->presentation().player_identity(), this->render_resources(), true));
		this->texture_container_->add_child(this->player_badges_.back().get());
	}
	// The stage record sits on the contribution heading's row, right-aligned
	// to the score bar, and is revealed with the other details.
	const std::wstring record = record_text(this->record_, this->text().records);
	if (!record.empty())
	{
		UiTextDropShadow* line = add_detail(record, Vector2F::ZERO, look.contribution_font);
		line->set_position(Vector2F(this->fill_box_->rectangle().right() -
			line->bounds().width, look.record_y));
	}

	if (this->revealed_)
	{
		// The reveal already showed the numbers. Keep the bars hidden,
		// but let the first update play the fanfare after the person has left.
		this->delay_timer_ = look.team_fill_delay;
		this->fill_timer_ = look.team_fill_time;
		this->fill_box_->set_hidden(true);
		this->fill_track_->set_hidden(true);
		this->team_a_fill_->set_hidden(true);
		this->team_b_fill_->set_hidden(true);
		this->team_a_mark_->set_hidden(true);
		this->team_b_mark_->set_hidden(true);
		return;
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
