#pragma once

#include "engine/math/rectanglef.h"
#include "game/objects/level_station.h"
#include "game/objects/match_random.h"
#include "game/objects/opening_mode.h"
#include "game/objects/team_colour.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

// What a Level is built from (HYG#40), grouped by where it comes from.
// LevelBuilder fills the four records; a probe may fill them by hand. None
// names GameData: each is borrows and values a builder unpacks.

namespace labrador
{
	class Scene;
	class GameObject;
	class RenderResources;
	class AudioResources;
	class ResolutionManager;
	class ViewportManager;
}
class Player;
class GameTuning;
class Presentation;
struct PlaySettings;
struct InputBindings;
class MatchSetup;

// The match's world, handed over whole. `players` borrow into `scene`, in
// roster participant order.
struct LevelWorld
{
	std::unique_ptr<labrador::Scene> scene;
	std::vector<Player*> players;
	std::vector<std::unique_ptr<labrador::GameObject>> viewport_dividers;
};

// The services and records a Level borrows for its lifetime. Tuning,
// presentation, preferences and bindings are live: `tune`, `reload` and the
// options pages change them in place, and Level reads through the pointer.
struct LevelServices
{
	const GameTuning* tuning = nullptr;
	const Presentation* presentation = nullptr;
	labrador::RenderResources* render_resources = nullptr;
	const labrador::AudioResources* audio_resources = nullptr;
	const labrador::ResolutionManager* resolution_manager = nullptr;
	const labrador::ViewportManager* viewport_manager = nullptr;
	const PlaySettings* preferences = nullptr;
	// Null shows the built-in default bindings.
	const InputBindings* input_bindings = nullptr;
};

// The stage's sound: its bank, its music and its end sting.
struct StageAudio
{
	std::string sound_bank_name;
	std::string music_name;
	float music_volume = 0.0f;
	std::string end_sting_name;
	float end_sting_volume = 0.0f;
};

// What the stage's definition says about this match.
struct LevelStage
{
	std::wstring name;
	mattmath::RectangleF camera_bounds;
	mattmath::RectangleF zoom_out_start_bounds;
	mattmath::RectangleF zoom_out_finish_bounds;
	StageAudio audio;
	std::vector<LevelStation> stations;
	// Empty unless a solo practice match asked for its lessons.
	std::vector<LevelLesson> lessons;
};

// This round's decisions.
struct LevelMatch
{
	TeamColour team_colours;
	OpeningMode opening = OpeningMode::off;
	// Empty uses tuning.match.duration.
	std::optional<float> duration;
	// Null makes every seat a human with a view and no bots.
	const MatchSetup* roster = nullptr;
	MatchRandom random = MatchRandom();
};
