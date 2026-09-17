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
	void update(float dt) override = 0;
protected:
	// This menu family's own presentation, beside MenuPage::menu_style()'s
	// shared one. Named for what a reader is looking for rather than for the
	// type it returns: `this->look().box_colour` at the call site.
	const EndMenuPresentation& look() const;

	// Closes the end menu with what the player chose. EndMenuData held nothing
	// but the out-parameter this replaces, so it is gone entirely.
	void finish(EndMenuAction action) const;

	// No sounds of its own. The three MenuPage resolves - a cursor move, a
	// confirm, a cancel - are the three this family plays.
};

class EndMenuInitial final : public EndMenuPage
{
public:
	explicit EndMenuInitial(MenuContext* context, const Session& session = Session());
	void init() override;
	void update(float dt) override;
private:
	Session session_;
	std::unique_ptr<labrador::UiTexture> box_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> heading_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> change_teams_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> change_weapons_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> change_level_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> restart_ = nullptr;
	std::unique_ptr<labrador::UiTextDropShadow> exit_ = nullptr;
};
