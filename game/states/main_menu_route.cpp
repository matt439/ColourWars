#include "game/pch.h"
#include "game/states/main_menu.h"

// The setup flow, as one graph. Each arrow is a go_to from the page on its
// left; the pages themselves decide when, this file decides what is built.
//
//   Title         -> Home
//   Home          -> Title (Back), Mode Select, Options, Credits
//                    (Exit pushes its confirmation above Home instead)
//   Options       -> Home on Options        (as a paused child it pops)
//   Credits       -> Home on Credits
//   Mode Select   -> Home (Back), Player Count
//   Player Count  -> Mode Select (Back), Join
//   Join          -> Player Count (Back), Team Select, or Weapon Select alone
//   Team Select   -> Player Count (Back), Weapon Select
//   Weapon Select -> Team Select (Back), or Mode Select (Back) alone,
//                    Stage Select
//   Stage Select  -> Weapon Select (Back); confirming pops the frame with
//                    MatchStart::menu and GameMenu starts the match
//   any roster page waiting on a missing controller -> Player Count (Back)
//
// After a match, GameMenu reopens the frame on Team, Weapon or Stage Select
// or Home (GameMenu::on_match_ended). Between rounds of a series the End
// menu re-picks on Weapon and Stage Select in its own frame, where Back and
// confirming return to that frame instead (RosterSetupPage::between_rounds).

MainMenuRoute::MainMenuRoute(MainMenuScreen screen) :
	screen(screen)
{
}

MainMenuRoute MainMenuRoute::home(MainMenuHome::InitialFocus focus)
{
	MainMenuRoute route(MainMenuScreen::home);
	route.home_focus = focus;
	return route;
}

MainMenuRoute MainMenuRoute::re_pick(MainMenuScreen screen, std::optional<Session> between_rounds,
	PlayerTeam picker)
{
	MainMenuRoute route(screen);
	route.between_rounds = std::move(between_rounds);
	route.picker = picker;
	return route;
}

std::unique_ptr<MainMenuPage> make_main_menu_page(MenuContext* context, const MainMenuRoute& route)
{
	switch (route.screen)
	{
	case MainMenuScreen::home:
		return std::make_unique<MainMenuHome>(context, route.home_focus);
	case MainMenuScreen::options:
		return std::make_unique<MainMenuOptions>(context);
	case MainMenuScreen::credits:
		return std::make_unique<MainMenuCredits>(context);
	case MainMenuScreen::mode_select:
		return std::make_unique<MainMenuModeSelect>(context);
	case MainMenuScreen::player_count:
		return std::make_unique<MainMenuPlayerCount>(context);
	case MainMenuScreen::join:
		return std::make_unique<MainMenuJoin>(context);
	case MainMenuScreen::team_select:
		return std::make_unique<MainMenuTeamSelect>(context);
	case MainMenuScreen::weapon_select:
		return std::make_unique<MainMenuWeaponSelect>(context, route.between_rounds);
	case MainMenuScreen::stage_select:
		return std::make_unique<MainMenuStageSelect>(context, route.between_rounds, route.picker);
	case MainMenuScreen::title:
	default:
		return std::make_unique<MainMenuTitle>(context);
	}
}

void MainMenuPage::go_to(const MainMenuRoute& route)
{
	this->context()->transition_to(make_main_menu_page(this->menu_context(), route));
}
