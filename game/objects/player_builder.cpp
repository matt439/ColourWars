#include "game/pch.h"
#include "game/objects/player_builder.h"

using namespace DirectX;
using namespace mattmath;
using namespace labrador;

namespace
{
	// Check the team-local spawn index before using it for both spawn and respawn.
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

PlayerBuilder::PlayerBuilder(const GameTuning* tuning, const Presentation* presentation) :
	tuning_(tuning), presentation_(presentation)
{
    this->weapon_builder_ = std::make_unique<WeaponBuilder>();
}

std::vector<std::unique_ptr<Player>> PlayerBuilder::build_players(
	const MenuLevelSettings& settings,
	const LevelLoadedInfo* load_info,
	TeamColour team_colours,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	const ViewportManager* viewport_manager) const
{
	std::vector<std::unique_ptr<Player>> players;

	const std::vector<Vector2F> team_a_spawns = load_info->team_a_spawns();
	const std::vector<Vector2F> team_b_spawns = load_info->team_b_spawns();

	// Index each team's spawn array using its own player counter.
	size_t team_a_index = 0;
	size_t team_b_index = 0;
	for (auto& setting : settings.player_settings())
	{
		// View index is match position, not input slot; connected slots may be sparse.
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
			&this->presentation_->movement_feedback(),
			&this->presentation_->respawn_redraw(),
			render_resources,
			audio_resources,
			preferences,
			setting.num,
			setting.team,
			setting.weapon,
			this->weapon_builder_->build_weapon(
				this->tuning_,
				setting.team,
				setting.num,
				view_index,
				team_colours.team_colour(setting.team),
				setting.weapon,
				rect.center(),
				render_resources,
				audio_resources, preferences),
			WeaponType::grenade_std,
			team_colours.team_colour(setting.team),
			viewport_manager->
				player_viewport(view_index),
			position);

		if (settings.stage() == "movement_course" &&
			settings.workshop_visual_mode() != PlayerVisualMode::original)
		{
			player->enable_workshop_visual(&this->presentation_->workshop_player(),
				render_resources, settings.workshop_visual_mode(), view_index);
		}

		players.push_back(std::move(player));
	}
	return players;
}
