#ifndef LEVELLOADEDINFO_H
#define LEVELLOADEDINFO_H

#include <cstdio>
#include "rapidjson/document.h"
#include <string>
#include <vector>
#include <map>
#include "MattMath.h"
//#include "border_thickness.h"
//#include "StructureWall.h"
//#include "Structure.h"
//#include "ICollisionGameObject.h"
//#include "IGameObject.h"

class LevelLoadedInfo
{
private:
	rapidjson::Document _json_doc;
	
	//std::string _level_name = "";
	//MattMath::Vector2F _level_size = MattMath::Vector2F::ZERO;
	//std::vector<MattMath::Vector2F> _team_a_spawns;
	//std::vector<MattMath::Vector2F> _team_b_spawns;
	//std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> _collision_objects = nullptr;
	//std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> _collision_objects = nullptr;
	//std::vector<std::string> _tile_types;
	//border_thickness _camera_bounds_border = border_thickness();

	//std::vector<std::string> decode_tile_types_json(
	//	const rapidjson::Value& json) const;

	std::vector<MattMath::Vector2F>
		decode_team_spawns_json(const rapidjson::Value& json) const;

	rapidjson::Document load_from_json(const char* json_path) const;

	//border_thickness decode_camera_bounds_json(const rapidjson::Value& json) const;
public:
	LevelLoadedInfo(const char* json_path) :
		_json_doc(load_from_json(json_path)) {}

	std::string get_level_name() const;
	std::vector<MattMath::Vector2F> get_team_a_spawns() const;
	std::vector<MattMath::Vector2F> get_team_b_spawns() const;
	//const std::vector<std::string>& get_tile_types() const;
	
	//border_thickness get_camera_bounds_border() const;

	MattMath::RectangleF get_out_of_bounds_rectangle() const;
	MattMath::RectangleF get_camera_bounds_rectangle() const;

	MattMath::RectangleF get_zoom_out_start_bounds_rectangle() const;
	MattMath::RectangleF get_zoom_out_finish_bounds_rectangle() const;

	//std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> get_collision_objects() const;
	//std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> get_non_collision_objects() const;

	const rapidjson::Value& get_collision_objects_json() const;
	const rapidjson::Value& get_non_collision_objects_json() const;

	std::string get_sound_bank_name() const;
	std::string get_music_name() const;
	float get_music_volume() const;

	
};
#endif // !LEVEL_LOADED_INFO_H
