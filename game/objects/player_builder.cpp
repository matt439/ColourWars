#include "game/pch.h"
#include "game/objects/player_builder.h"

using namespace DirectX;
using namespace mattmath;
using namespace artattack;

namespace
{
	// A level file is written by hand, so the one read of one that does not go
	// through JsonValue is checked here instead (T6). Left unchecked this is a
	// read past the end of the vector, used verbatim as both the spawn and the
	// respawn position, and it surfaces three states later as "Player out of
	// bounds" with nothing naming the level that is short of spawns.
	Vector2F team_spawn(const std::vector<Vector2F>& spawns, size_t index,
		const char* key, const std::string& stage)
	{
		if (index >= spawns.size())
		{
			throw std::runtime_error("level '" + stage +
				"': 'spawn_locations." + key + "' gives " +
				std::to_string(spawns.size()) +
				" spawn points, and this match needs at least " +
				std::to_string(index + 1) + " on that team");
		}
		return spawns[index];
	}
}

PlayerBuilder::PlayerBuilder(const GameTuning* tuning) :
	tuning_(tuning)
{
    this->weapon_builder_ = std::make_unique<WeaponBuilder>();
}

std::vector<std::unique_ptr<Player>> PlayerBuilder::build_players(
	const MenuLevelSettings& settings,
	const LevelLoadedInfo* load_info,
	TeamColour team_colours,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const ViewportManager* viewport_manager) const
{
	std::vector<std::unique_ptr<Player>> players;

	const std::vector<Vector2F> team_a_spawns = load_info->team_a_spawns();
	const std::vector<Vector2F> team_b_spawns = load_info->team_b_spawns();

	// The spawn arrays are per team, so each is indexed by its own team's
	// counter: the second player on team B takes team_b_spawns[1], not [3]
	// because two players on team A happened to be listed before them.
	size_t team_a_index = 0;
	size_t team_b_index = 0;
	for (auto& setting : settings.player_settings())
	{
		// The view index is this player's position in the list, not their pad
		// slot: setting.num is the slot, and the two diverge the moment the
		// pads driving a match are not 0..n-1 (a pad left unplugged at the
		// menu, or players who joined on the back two slots).
		const int view_index = static_cast<int>(players.size());

		Vector2F position = Vector2F::ZERO;
		if (setting.team == PlayerTeam::a)
		{
			position = team_spawn(team_a_spawns, team_a_index++, "team_a",
				settings.stage());
		}
		else
		{
			position = team_spawn(team_b_spawns, team_b_index++, "team_b",
				settings.stage());
		}
		auto rect = RectangleF(position, this->tuning_->player().size);

		auto player = std::make_unique<Player>(
			rect,
			this->tuning_,
			render_resources,
			audio_resources,
			setting.num,
			setting.team,
			setting.weapon,
			this->weapon_builder_->build_weapon(
				this->tuning_,
				setting.team,
				setting.num,
				team_colours.team_colour(setting.team),
				setting.weapon,
				rect.center(),
				render_resources,
				audio_resources),
			WeaponType::grenade_std,
			team_colours.team_colour(setting.team),
			viewport_manager->
				player_viewport(view_index),
			position);

		players.push_back(std::move(player));
	}
	return players;
}
