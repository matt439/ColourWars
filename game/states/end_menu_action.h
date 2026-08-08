#pragma once

// What the end-of-match menu was closed with. See pause_menu_action.h for why
// there is no `none`.
enum class EndMenuAction
{
	change_teams,
	change_weapons,
	change_level,
	restart,
	exit,
};
