#pragma once

#include "game/objects/level_end_info.h"
#include "game/session.h"
#include "game/match_records.h"
#include "engine/render/colour.h"
#include "game/states/menu_page.h"
#include "game/states/ui_player_badge.h"
#include "game/carried/ui_text_drop_shadow.h"


class ResultsMenuPage : public MenuPage
{
public:
	// Copies the final scores so the results stay fixed.
	ResultsMenuPage(MenuContext* context, const LevelEndInfo& end_info);
	~ResultsMenuPage() override = default;
	void init() override = 0;
	void update_page(float dt) override = 0;
protected:
	const ResultsMenuPresentation& look() const;

	LevelEndInfo level_end_info() const;

	// The fill sound loops until the score reveal completes.
	labrador::SoundBank::WaveHandle winner_sound_;
	labrador::SoundBank::EffectHandle fill_sound_;
private:
	LevelEndInfo level_end_info_ = LevelEndInfo();
};

class ResultsMenuInitial final : public ResultsMenuPage
{
public:
	// `revealed`: the match already showed the winner and the coverage on
	// the person's monitor, so this page keeps only the percentages.
	// `record`: what this result did to the stage's record; the default
	// (nothing recorded) shows no record line.
	ResultsMenuInitial(MenuContext* context, const LevelEndInfo& end_info,
		const Session& session = Session(), bool revealed = false,
		const RecordOutcome& record = RecordOutcome());
	// The record line's text, or empty when the page shows none.
	static std::wstring record_text(const RecordOutcome& record, const MenuRecordText& text);
	void init() override;
	void update_page(float dt) override;
	// Proceeds as a confirm would; the match then opens the end menu.
	bool debug_dismiss(bool leaving_match) override;
private:
	Session session_;
	bool revealed_ = false;
	RecordOutcome record_;
	std::vector<std::unique_ptr<colourwars::UiTextDropShadow>> details_;
	std::vector<std::unique_ptr<UiPlayerBadge>> player_badges_;
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<colourwars::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<labrador::UiTexture> fill_box_ = nullptr;
	std::unique_ptr<labrador::UiTexture> fill_track_ = nullptr;
	std::unique_ptr<labrador::UiTexture> team_a_fill_ = nullptr;
	std::unique_ptr<labrador::UiTexture> team_b_fill_ = nullptr;

	// A/B marks identify the bars without relying on colour.
	std::unique_ptr<colourwars::UiTextDropShadow> team_a_mark_ = nullptr;
	std::unique_ptr<colourwars::UiTextDropShadow> team_b_mark_ = nullptr;
	std::unique_ptr<labrador::UiTexture> team_a_percentage_panel_ = nullptr;
	std::unique_ptr<labrador::UiTexture> team_b_percentage_panel_ = nullptr;
	std::unique_ptr<colourwars::UiTextDropShadow> team_a_percentage_ = nullptr;
	std::unique_ptr<colourwars::UiTextDropShadow> team_b_percentage_ = nullptr;
	std::unique_ptr<colourwars::UiTextDropShadow> winner_ = nullptr;
	std::unique_ptr<colourwars::UiTextDropShadow> proceed_ = nullptr;
	bool winner_sound_played_ = false;
	float delay_timer_ = 0.0f;
	float fill_timer_ = 0.0f;
	float show_results_timer_ = 0.0f;
	float fill_time_ratio() const;
	mattmath::Vector2F calculate_team_b_fill_top_right_position() const;
	static int check_for_continue_input(const std::vector<ProcessedMenuInput>& menu_input);
	void update_fill_box() const;
	void update_team_a_fill() const;
	void update_team_b_fill() const;
};
