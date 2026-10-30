#pragma once

#include "game/states/menu_page.h"
#include "game/states/menu_panel.h"
#include <cstddef>
#include <string>
#include <vector>

// A read-only snapshot of the latest optional user-stage failures. Covering
// Stage Select leaves its cursor and confirmation state alive underneath.
class UserStageRefusalsMenu final : public MenuPage
{
public:
	UserStageRefusalsMenu(MenuContext* context, std::vector<std::string> refusals);
	void init() override;
	bool covers_screen() const override { return true; }
	bool debug_dismiss(bool leaving_match) override;
protected:
	void update_page(float dt) override;
	bool shows_focus_backing() const override { return false; }
private:
	std::vector<std::string> refusals_;
	std::vector<std::wstring> pages_;
	std::size_t file_index_ = 0;
	std::size_t page_index_ = 0;
	labrador::FontHandle font_;
	mattmath::RectangleF body_bounds_;
	mattmath::RectangleF progress_bounds_;
	std::unique_ptr<labrador::UiTexture> background_;
	std::unique_ptr<MenuPaperArtwork> paper_;
	std::unique_ptr<labrador::UiText> heading_;
	std::unique_ptr<labrador::UiText> unicode_hint_;
	std::unique_ptr<labrador::UiText> body_;
	std::unique_ptr<labrador::UiText> progress_;
	void refresh_file();
	void refresh_text();
	void close();
};
