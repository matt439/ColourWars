#include "game/pch.h"
#include "game/objects/player_builder.h"

using namespace DirectX;
using namespace mattmath;
using namespace player_consts;

PlayerBuilder::PlayerBuilder()
{
    this->weapon_builder_ = std::make_unique<WeaponBuilder>();
}

std::unique_ptr<std::vector<std::unique_ptr<Player>>> PlayerBuilder::build_players(
	const MenuLevelSettings& settings,
	const LevelLoadedInfo* load_info,
	TeamColour team_colours,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const ViewportManager* viewport_manager,
	const float* dt) const
{
	auto players = std::make_unique<std::vector<std::unique_ptr<Player>>>();

	const std::vector<Vector2F> team_a_spawns = load_info->team_a_spawns();
	const std::vector<Vector2F> team_b_spawns = load_info->team_b_spawns();

	int i = 0;
	for (auto& setting : settings.player_settings())
	{
		Vector2F position = Vector2F::ZERO;
		if (setting.team == PlayerTeam::a)
		{
			position = team_a_spawns[i];
		}
		else
		{
			position = team_b_spawns[i];
		}
		auto rect = RectangleF(position, DEFAULT_SIZE);

		auto player = std::make_unique<Player>(
			rect,
			DEFAULT_ANIMATION_INFO,
			render_resources,
			audio_resources,
			setting.num,
			setting.team,
			setting.weapon,
			this->weapon_builder_->build_weapon(
				setting.team,
				setting.num,
				team_colours.team_colour(setting.team),
				setting.weapon,
				rect.center(),
				render_resources,
				audio_resources,
				dt),
			WeaponType::grenade_std,
			team_colours.team_colour(setting.team),
			viewport_manager->
				player_viewport(setting.num),
			dt,
			position);

		players->push_back(std::move(player));

		i++;
	}
	return players;
}
