#pragma once

#include "game/debug/debug_menu_model.h"
#include "game/states/menu_page.h"
#include "game/states/option_row.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

class DebugDirectorImpl;

// The debug menu: the registry as a pad- and keyboard-navigable page, a
// sibling of the other menu pages. Pushed by the director over whatever is on
// top (a menu page or the match) with no result, like the console, so it pops
// without telling anyone; the match under it is frozen while it is open.
//
// Up/Down move, Left/Right step a variable, A/Enter selects, B/Escape goes
// back a level (and closes at the top), and F10, F11 or holding pad
// Back+Start closes it from anywhere. It draws DebugMenuModel's rows; the
// model is what the acceptance case tests.
class DebugMenuPage final : public MenuPage
{
public:
	DebugMenuPage(MenuContext* context, DebugDirectorImpl* director);
	~DebugMenuPage() override;

	void init() override;

	// F10, F11 or the chord while open. Called from this page's own update
	// (the director's hook), so the pop is deferred like any other.
	void request_close();

protected:
	void update_page(float dt) override;

private:
	DebugDirectorImpl* director_ = nullptr;
	DebugMenuModel model_;
	std::vector<std::unique_ptr<OptionRow>> rows_;
	std::vector<std::string> shown_labels_;
	int shown_level_ = -1;
	std::unique_ptr<labrador::UiTexture> panel_;
	std::unique_ptr<labrador::UiTextDropShadow> title_;
	std::unique_ptr<labrador::UiTextDropShadow> queued_;
	std::array<std::unique_ptr<labrador::UiTextDropShadow>, 3> footer_;
	bool closing_ = false;

	// Labels change with the level and with what is staged; values change
	// every frame. A label change rebuilds the rows (OptionRow keeps its
	// label), a value change only re-sets the value.
	void refresh();
	void rebuild(const std::vector<DebugMenuRow>& rows, std::size_t focus);
	std::size_t focused_row() const;
	void close();
};
