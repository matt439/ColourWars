#pragma once

#include "game/objects/i_collision_game_object.h"
#include "game/objects/team_colour.h"
#include "game/objects/level_stage.h"
#include "game/objects/player_input.h"
#include "game/objects/player.h"
#include "engine/render/resolution_manager.h"
#include "engine/render/viewport_manager.h"
#include "game/objects/debug_text.h"
#include "engine/render/camera_tools.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/level_end_info.h"
#include "engine/render/text_drop_shadow.h"
#include "engine/audio/sound_bank.h"
#include "game/objects/i_paintable_game_object.h"
#include "engine/render/render_resources.h"
#include "engine/audio/audio_resources.h"
#include "engine/core/thread_pool.h"
#include "engine/collision/partitioner.h"

enum class LevelState
{
	start_countdown,
	active,
	zoom_out,
	overview,
	finished,
};

namespace level_consts
{
	constexpr float TIMER = 240.0f;
	constexpr float START_TIMER = 3.0f;
	const static std::string COUNTDOWN_TEXT = "3";
	const static std::string COUNTDOWN_FONT_NAME = "gill_sans_mt_bold_144";
	const static mattmath::Colour COUNTDOWN_COLOUR = colour_consts::DARK_GRAY;
	const static mattmath::Colour COUNTDOWN_SHADOW_COLOUR = colour_consts::BLACK;
	constexpr float COUNTDOWN_SCALE = 2.0f;
	const static mattmath::Vector2F COUNTDOWN_SHADOW_OFFSET = { 5.0f, 5.0f };
	constexpr float COUNTDOWN_TEXT_WIDTH = 400.0f;
	constexpr float COUNTDOWN_TEXT_HEIGHT = 600.0f;

	constexpr float ZOOM_OUT_TIMER = 2.0f;
	constexpr float OVERVIEW_TIMER = 2.0f;

	const std::string COUNTDOWN_SOUND = "smash_countdown";
	constexpr float COUNTDOWN_SOUND_VOLUME = 1.0f;

	const std::string ZOOM_OUT_SOUND = "slide_whistle_up";
	constexpr float ZOOM_OUT_SOUND_VOLUME = 0.5f;
}

class Level
{
public:
	Level() = default;
	Level(std::unique_ptr<std::vector<std::unique_ptr<artattack::IGameObject>>> non_collision_objects,
		std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> collision_objects,
		std::unique_ptr<std::vector<std::unique_ptr<Player>>> player_objects,
		std::unique_ptr<std::vector<std::unique_ptr<artattack::IGameObject>>> viewport_dividers,
		LevelStage stage,
		const TeamColour& team_colours,
		const mattmath::RectangleF& out_of_bounds,
		const mattmath::RectangleF& camera_bounds,
		const mattmath::RectangleF& zoom_out_start_bounds,
		const mattmath::RectangleF& zoom_out_finish_bounds,
		const std::vector<mattmath::Vector2F>& team_a_spawns,
		const std::vector<mattmath::Vector2F>& team_b_spawns,
		const std::string& sound_bank_name,
		const std::string& music_name,
		float music_volume,
		const float* dt,
		ID3D11SamplerState* sampler_state,
		const std::string& level_name,
		const artattack::ResolutionManager* resolution_manager,
		artattack::ViewportManager* viewport_manager,
		artattack::RenderResources* render_resources,
		const artattack::AudioResources* audio_resources,
		artattack::ThreadPool* thread_pool,
		const artattack::Partitioner* partitioner);

	void update(const std::vector<PlayerInputData>& player_inputs);
	void draw(std::vector<ID3D11DeviceContext*>* deferred_contexts,
		std::vector<ID3D11CommandList*>* command_lists,
		std::vector<DirectX::SpriteBatch*>* sprite_batches) const;

	LevelState state() const;
	void set_state(LevelState state);

	LevelEndInfo level_end_info() const;

	void stop_music() const;

private:
	std::unique_ptr<std::vector<std::unique_ptr<artattack::IGameObject>>>
		non_collision_objects_ = nullptr;

	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>>
		collision_objects_ = nullptr;

	std::unique_ptr<std::vector<std::unique_ptr<Player>>>
		player_objects_ = nullptr;

	std::unique_ptr<std::vector<std::unique_ptr<artattack::IGameObject>>>
		viewport_dividers_ = nullptr;

	artattack::SoundBank* sound_bank_ = nullptr;

	// The level's own music comes from its definition file, so its name is not
	// known until here - but it is known once, at construction, which is where
	// it stops being a name. Same for the two fixed sounds the level plays.
	artattack::SoundBank::EffectHandle music_;
	artattack::SoundBank::EffectHandle zoom_out_sound_;
	artattack::SoundBank::WaveHandle countdown_sound_;
	float music_volume_ = 0.0f;

	std::unique_ptr<artattack::TextDropShadow> countdown_text_ = nullptr;

	std::unique_ptr<artattack::CameraTools> camera_tools_ = nullptr;
	std::unique_ptr<InterfaceGameplay> interface_gameplay_ = nullptr;

	std::string level_name_ = "";
	const artattack::ResolutionManager* resolution_manager_ = nullptr;
	artattack::ViewportManager* viewport_manager_ = nullptr;
	artattack::RenderResources* render_resources_ = nullptr;

	float timer_ = level_consts::TIMER;
	float start_timer_ = level_consts::START_TIMER;
	float zoom_out_timer_ = level_consts::ZOOM_OUT_TIMER;
	float overview_timer_ = level_consts::OVERVIEW_TIMER;

	mattmath::Camera zoom_out_camera_ = mattmath::Camera::DEFAULT_CAMERA;

	TeamColour team_colours_ = TeamColour();
	LevelStage stage_ = LevelStage::king_of_the_hill;

	mattmath::RectangleF out_of_bounds_ = mattmath::RectangleF::ZERO;
	mattmath::RectangleF camera_bounds_ = mattmath::RectangleF::ZERO;
	mattmath::RectangleF zoom_out_start_bounds_ = mattmath::RectangleF::ZERO;
	mattmath::RectangleF zoom_out_finish_bounds_ = mattmath::RectangleF::ZERO;

	std::vector<mattmath::Vector2F> team_a_spawns_ = std::vector<mattmath::Vector2F>();
	std::vector<mattmath::Vector2F> team_b_spawns_ = std::vector<mattmath::Vector2F>();

	LevelState state_ = LevelState::start_countdown;
	const float* dt_ = nullptr;
	ID3D11SamplerState* sampler_state_ = nullptr;

	std::vector<PlayerInputData> player_inputs_ = std::vector<PlayerInputData>();
	std::unique_ptr<DebugText> debug_text_ = nullptr;

	artattack::ThreadPool* thread_pool_ = nullptr;
	const artattack::Partitioner* partitioner_ = nullptr;

	int count_projectiles() const;
	float dt() const;
	bool is_object_out_of_bounds(const ICollisionGameObject* object) const;
	void draw_end_screen();

	void update_level_logic(const std::vector<PlayerInputData>& player_inputs) const;

	void draw_active_level(std::vector<ID3D11DeviceContext*>* deferred_contexts,
		std::vector<ID3D11CommandList*>* command_lists,
		std::vector<DirectX::SpriteBatch*>* sprite_batches) const;

	void draw_zoom_out_level(std::vector<ID3D11DeviceContext*>* deferred_contexts,
		std::vector<ID3D11CommandList*>* command_lists,
		std::vector<DirectX::SpriteBatch*>* sprite_batches) const;

	float zoom_out_camera_ratio() const;

	void stop_player_sounds() const;

	void draw_player_view_level(int start, int end,
		std::vector<ID3D11DeviceContext*>* deferred_contexts,
		std::vector<ID3D11CommandList*>* command_lists,
		std::vector<DirectX::SpriteBatch*>* sprite_batches) const;

	void draw_zoom_out_level_component(std::vector<ID3D11DeviceContext*>* deferred_contexts,
		std::vector<ID3D11CommandList*>* command_lists,
		std::vector<DirectX::SpriteBatch*>* sprite_batches) const;

	void draw_countdown_text(DirectX::SpriteBatch* sprite_batch,
		const mattmath::Camera& viewport_camera) const;
};
