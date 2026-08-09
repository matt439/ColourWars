#pragma once

#include "game/objects/level_end_info.h"
#include "engine/render/colour.h"
#include "game/states/menu_page.h"
#include "engine/audio/sound_bank_object.h"


class ResultsMenuPage : public MenuPage, public artattack::SoundBankObject
{
public:
	// The scores are fixed the moment the match ends, so they arrive as a value
	// rather than through a data class that also had to carry the result out.
	ResultsMenuPage(MenuContext* context, const LevelEndInfo& end_info);
	~ResultsMenuPage() override = default;
	void init() override = 0;
	void update(float dt) override = 0;
	void draw(artattack::Renderer& renderer) const override = 0;
protected:
	// This menu family's own presentation, beside MenuPage::menu_style()'s
	// shared one. Named for what a reader is looking for rather than for the
	// type it returns: `this->look().box_colour` at the call site.
	const ResultsMenuPresentation& look() const;

	LevelEndInfo level_end_info() const;

	// Every sound this page family can make, resolved once when the page is
	// built. The drum roll is an effect rather than a wave because it is
	// looped and then stopped; the rest are fire-and-forget.
	artattack::SoundBank::WaveHandle confirm_sound_;
	artattack::SoundBank::WaveHandle winner_sound_;
	artattack::SoundBank::EffectHandle fill_sound_;
private:
	LevelEndInfo level_end_info_ = LevelEndInfo();
};

class ResultsMenuInitial final : public ResultsMenuPage
{
public:
	ResultsMenuInitial(MenuContext* context, const LevelEndInfo& end_info);
	void init() override;
	void update(float dt) override;
	void draw(artattack::Renderer& renderer) const override;
private:
	std::unique_ptr<artattack::UiContainer> texture_container_ = nullptr;
	std::unique_ptr<artattack::UiContainer> text_container_ = nullptr;
	std::unique_ptr<artattack::UiTexture> box_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<artattack::UiTexture> fill_box_ = nullptr;
	std::unique_ptr<artattack::UiTexture> team_a_fill_ = nullptr;
	std::unique_ptr<artattack::UiTexture> team_b_fill_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> team_a_percentage_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> team_b_percentage_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> winner_ = nullptr;
	std::unique_ptr<artattack::UiTextDropShadow> proceed_ = nullptr;
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
