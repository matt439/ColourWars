#include "pch.h"
#include "PlayerBuilder.h"

using namespace DirectX;
//using namespace DirectX::SimpleMath;
using namespace MattMath;
using namespace player_consts;

PlayerBuilder::PlayerBuilder()
{
    this->_weapon_builder = std::make_unique<WeaponBuilder>();
    //this->_team_colours = std::make_unique<TeamColour>();
    //this->_viewport_generator = std::make_unique<ViewportGenerator>();
}

std::unique_ptr<std::vector<std::unique_ptr<Player>>> PlayerBuilder::build_players(
	const MenuLevelSettings& settings,
	const LevelLoadedInfo* load_info,
	team_colour team_colours,
	DirectX::SpriteBatch* sprite_batch,
	ResourceManager* resource_manager,
	ViewportManager* viewport_manager,
	const float* dt)
	//const ResolutionManager* resolution)
{
	std::unique_ptr<std::vector<std::unique_ptr<Player>>> players =
		std::make_unique<std::vector<std::unique_ptr<Player>>>();

	const std::vector<Vector2F> team_a_spawns = load_info->get_team_a_spawns();
	const std::vector<Vector2F> team_b_spawns = load_info->get_team_b_spawns();

	int i = 0;
	for (auto& setting : settings.get_player_settings())
	{
		Vector2F position = Vector2F::ZERO;
		if (setting.team == player_team::A)
		{
			position = team_a_spawns[i];
		}
		else
		{
			position = team_b_spawns[i];
		}
		RectangleF rect = RectangleF(position, DEFAULT_SIZE);

		std::unique_ptr<Player> player = std::make_unique<Player>(
			rect,
			DEFAULT_ANIMATION_INFO,
			sprite_batch,
			resource_manager,
			setting.num,
			setting.team,
			setting.weapon,
			this->_weapon_builder->build_weapon(
				setting.team,
				setting.num,
				team_colours.get_team_colour(setting.team),
				setting.weapon,
				rect.get_center(),
				sprite_batch,
				resource_manager,
				dt),
			wep_type::GRENADE_STD,
			team_colours.get_team_colour(setting.team),
			viewport_manager->
				get_player_viewport(setting.num),
			dt,
			position);

		players->push_back(std::move(player));

		i++;
	}
	return players;
}

//std::shared_ptr<std::vector<Player>> PlayerBuilder::build_players(
//	const MenuLevelSettings& settings,
//    team_colour team_colours,
//    const ResolutionManager* resolution,
//    const std::vector<Vector2F>& team_a_spawns,
//    const std::vector<Vector2F>& team_b_spawns)
//{
//	std::shared_ptr<std::vector<Player>> players;
//    players = std::make_shared<std::vector<Player>>();
//
//    int i = 0;
//	for (auto& setting : settings.get_player_settings())
//	{
//		player_info info = player_info();
//		info.team = setting.team;
//		info.player_num = setting.num;
//		info.primary = setting.weapon;
//		info.team_colour = this->_team_colours->
//            get_team_colour(team_colours, setting.team);
//        if (setting.team == player_team::A)
//        {
//			info.position = team_a_spawns[i];
//		}
//        else
//        {
//			info.position = team_b_spawns[i];
//		}
//        //info.position =
//        //    this->get_player_start_pos(settings.get_stage(), setting.num);
//        info.view_port = this->_viewport_manager->
//            get_player_viewport(setting.num);
//   //     info.projection = this->_projection.calculate_projection(
//			//settings.get_screen_layout(), resolution->get_resolution_ivec());
//        info.size = this->get_player_size();
//		//secondary wep
//
//		players->push_back(Player(info));
//
//        weapon_info w_info;
//        w_info.player_num = info.player_num;
//        w_info.team = info.team;
//        w_info.team_colour = info.team_colour;
//        w_info.type = info.primary;
//
//        (*players)[i].set_primary(this->_weapon_builder->build_weapon(w_info));
//
//        i++;
//	}
//	return players;
//}

//Vector2 PlayerBuilder::get_player_start_pos(
//    level_stage stage, int player_num)
//{
//    switch (stage)
//    {
//    case level_stage::TEST_1:
//        switch (player_num)
//        {
//        case 0:
//            return Vector2(8000.0f, 2200.0f);
//            //return glm::vec2(4700.0f, 1700.0f);
//        case 1:
//            return Vector2(7900.0f, 2200.0f);
//        default:
//            return Vector2(0.0f, 0.0f);
//        }
//    default:
//        return Vector2(0.0f, 0.0f);
//    }
//}
//
//Vector2 PlayerBuilder::get_player_respawn_pos(
//    level_stage stage, int player_num)
//{
//    switch (stage)
//    {
//    case level_stage::TEST_1:
//        switch (player_num)
//        {
//        case 0:
//            return Vector2(8000.0f, 2200.0f);
//        case 1:
//            return Vector2(7900.0f, 2200.0f);
//        default:
//            return Vector2(0.0f, 0.0f);
//        }
//    default:
//        return Vector2(0.0f, 0.0f);
//    }
//}

//Vector2F PlayerBuilder::get_player_size()
//{
//    return Vector2F(57.0f, 128.0f);
//}