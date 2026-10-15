#pragma once

#include "game/states/end_menu_action.h"
#include "game/session.h"
#include "engine/ui/widget.h"
#include "game/states/menu_page.h"
#include "engine/core/state_context.h"
#include "engine/render/colour.h"

class EndMenuPage : public MenuPage
{
public:
	explicit EndMenuPage(MenuContext* context);
	~EndMenuPage() override = default;
	void init() override = 0;
	void update_page(float dt) override = 0;
	// Continue (the match's build adopts the queued plan and, like the
	// page's own primary action, moves the series on) or exit to the main menu.
	bool debug_dismiss(bool leaving_match) override;
protected:
	const EndMenuPresentation& look() const;

	// Closes this menu and returns the selected action to the match.
	void finish(EndMenuAction action) const;

};

class EndMenuInitial final : public EndMenuPage
{
public:
	explicit EndMenuInitial(MenuContext* context, const Session& session = Session());
	void init() override;
	void update_page(float dt) override;
protected:
	bool shows_brand() const override { return false; }
private:
	Session session_;
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiText> heading_ = nullptr;
	std::unique_ptr<labrador::UiText> change_teams_ = nullptr;
	std::unique_ptr<labrador::UiText> change_weapons_ = nullptr;
	std::unique_ptr<labrador::UiText> change_level_ = nullptr;
	std::unique_ptr<labrador::UiText> restart_ = nullptr;
	std::unique_ptr<labrador::UiText> exit_ = nullptr;
};
