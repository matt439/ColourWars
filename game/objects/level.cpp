#include "game/pch.h"
#include "game/objects/level.h"

using namespace mattmath;
using namespace DirectX;
using namespace level_consts;
using namespace artattack;

Level::Level(std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> non_collision_objects,
	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> collision_objects,
	std::unique_ptr<std::vector<std::unique_ptr<Player>>> player_objects,
	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> viewport_dividers,
	const TeamColour& team_colours,
	const RectangleF& out_of_bounds,
	const RectangleF& camera_bounds,
	const RectangleF& zoom_out_start_bounds,
	const RectangleF& zoom_out_finish_bounds,
	const std::vector<Vector2F>& team_a_spawns,
	const std::vector<Vector2F>& team_b_spawns,
	const std::string& sound_bank_name,
	const std::string& music_name,
	float music_volume,
	ID3D11SamplerState* sampler_state,
	const ResolutionManager* resolution_manager,
	ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	ThreadPool* thread_pool,
	const Partitioner* partitioner) :
	non_collision_objects_(std::move(non_collision_objects)),
	collision_objects_(std::move(collision_objects)),
	player_objects_(std::move(player_objects)),
	viewport_dividers_(std::move(viewport_dividers)),
	music_volume_(music_volume),
	resolution_manager_(resolution_manager),
	viewport_manager_(viewport_manager),
	render_resources_(render_resources),
	team_colours_(team_colours),
	out_of_bounds_(out_of_bounds),
	camera_bounds_(camera_bounds),
	zoom_out_start_bounds_(zoom_out_start_bounds),
	zoom_out_finish_bounds_(zoom_out_finish_bounds),
	team_a_spawns_(team_a_spawns),
	team_b_spawns_(team_b_spawns),
	sampler_state_(sampler_state),
	thread_pool_(thread_pool),
	partitioner_(partitioner)
{
	this->debug_text_ = std::make_unique<DebugText>(
		render_resources, resolution_manager);
	this->camera_tools_ = std::make_unique<CameraTools>();
	this->interface_gameplay_ = std::make_unique<InterfaceGameplay>(
		render_resources);
	this->sound_bank_ = audio_resources->sound_bank(sound_bank_name);

	this->music_ = this->sound_bank_->resolve_effect(music_name);
	this->zoom_out_sound_ = this->sound_bank_->resolve_effect(ZOOM_OUT_SOUND);
	this->countdown_sound_ = this->sound_bank_->resolve_wave(COUNTDOWN_SOUND);
}
void Level::stop_music() const
{
	this->sound_bank_->stop_effect(this->music_, true);
}
void Level::update(const std::vector<PlayerInputData>& player_inputs,
	float dt)
{
	this->frame_dt_ = dt;
	if (this->state_ == LevelState::start_countdown ||
		this->start_timer_ > -1.0f)
	{
		// first update
		if (this->start_timer_ >= START_TIMER)
		{
			this->sound_bank_->play_effect(this->music_, true, this->music_volume_);
			this->sound_bank_->play_wave(this->countdown_sound_, COUNTDOWN_SOUND_VOLUME);
			
			// update player cameras
			int player_index = 0;
			for (auto& object : *this->player_objects_)
			{
				Camera camera = this->camera_tools_->calculate_camera(
					object->center(),
					this->viewport_manager_->player_viewport(player_index).size(),
					object->camera(),
					this->camera_bounds_);
				object->set_camera(camera);
				player_index++;
			}

			// create countdown text
			Vector2F resolution = this->resolution_manager_->resolution_vec();
			Vector2F position = resolution / 2.0f;
			position = position - Vector2F(COUNTDOWN_TEXT_WIDTH / 2.0f,
								COUNTDOWN_TEXT_HEIGHT / 2.0f);

			this->countdown_text_ = std::make_unique<TextDropShadow>(
				COUNTDOWN_TEXT,
				COUNTDOWN_FONT_NAME,
				position,
				this->render_resources_,
				COUNTDOWN_COLOUR,
				COUNTDOWN_SHADOW_COLOUR,
				COUNTDOWN_SHADOW_OFFSET,
				COUNTDOWN_SCALE,
				COUNTDOWN_SCALE);
		}
		this->start_timer_ -= dt;
		if (this->start_timer_ > 0.0f)
		{
			this->countdown_text_->set_text(std::to_string(
				static_cast<int>(this->start_timer_) + 1));
		}
		else
		{
			this->countdown_text_->set_text("GO!");
		}

		if (this->start_timer_ <= 0.0f)
		{
			this->state_ = LevelState::active;
		}
		else
		{
			return;
		}
	}
	if (this->state_ == LevelState::active)
	{
		this->update_level_logic(player_inputs, dt);
		
		this->timer_ -= dt;
		if (this->timer_ <= 0.0f)
		{
			this->state_ = LevelState::zoom_out;
			this->sound_bank_->stop_effect(this->music_, true);
			this->stop_player_sounds();
			this->sound_bank_->play_effect(this->zoom_out_sound_, false, ZOOM_OUT_SOUND_VOLUME);
		}
	}
	else if (this->state_ == LevelState::zoom_out)
	{		
		this->zoom_out_timer_ -= dt;

		if (this->zoom_out_timer_ <= 0.0f)
		{
			this->state_ = LevelState::overview;
			this->zoom_out_timer_ = 0.0f;
			this->sound_bank_->stop_effect(this->zoom_out_sound_, true);
		}

		Camera start_camera = Camera::calculate_camera_from_view_rectangle(
			this->zoom_out_start_bounds_,
			RectangleF(Vector2F::ZERO, this->resolution_manager_->resolution_vec()));

		Camera finish_camera = Camera::calculate_camera_from_view_rectangle(
			this->zoom_out_finish_bounds_,
			RectangleF(Vector2F::ZERO, this->resolution_manager_->resolution_vec()));

		this->zoom_out_camera_ = Camera::calculate_intermediate_camera(
			start_camera, finish_camera, this->zoom_out_camera_ratio());
	}
	else if (this->state_ == LevelState::overview)
	{
		this->overview_timer_ -= dt;
		if (this->overview_timer_ <= 0.0f)
		{
			this->state_ = LevelState::finished;
		}
	}
	else if (this->state_ == LevelState::finished)
	{
		this->sound_bank_->stop_effect(this->music_, true);
		return;
	}
	
}
float Level::zoom_out_camera_ratio() const
{
	return 1.0f - (this->zoom_out_timer_ / ZOOM_OUT_TIMER);
}
void Level::update_level_logic(const std::vector<PlayerInputData>& player_inputs,
	float dt) const
{
	// update collision objects
	for (const auto& object : *this->collision_objects_)
	{
		object->update(dt);
	}
	
	// update player objects
	for (const auto& object : *this->player_objects_)
	{
		// Index by the player's own pad slot, not by position in this loop.
		// player_inputs is one entry per pad slot; a running counter only
		// happened to agree while every pad stayed connected.
		const int player_index = object->player_num();

		if (player_index >= 0 &&
			static_cast<size_t>(player_index) < player_inputs.size())
		{
			object->set_player_input(player_inputs[player_index]);
		}
		else
		{
			object->set_player_input(PlayerInputData());
		}
		object->update(dt);

		// update player camera
		Camera camera = this->camera_tools_->calculate_camera(
			object->center(),
			this->viewport_manager_->player_viewport(player_index).size(),
			object->camera(),
			this->camera_bounds_);
		object->set_camera(camera);

		// update player weapon
		std::vector<std::unique_ptr<ICollisionGameObject>> new_projs =
			object->update_weapon_and_get_projectiles(dt);
		// add new projectiles to collision objects
		for (auto& proj : new_projs)
		{
			this->collision_objects_->push_back(std::move(proj));
		}
	}

	// update non-collision objects
	for (const auto& object : *this->non_collision_objects_)
	{
		object->update(dt);
	}

	// check player collisions
	for (auto& player : *this->player_objects_)
	{
		bool player_colliding_with_structure = false;
		
		// check player collisions with collision objects
		for (auto& other_object : *this->collision_objects_)
		{
			if (other_object->for_deletion())
			{
				continue;
			}
			if (player->is_colliding(other_object.get()))
			{
				player->on_collision(other_object.get());
				other_object->on_collision(player.get());

				if (is_structure(other_object->collision_object_type()))
				{
					player_colliding_with_structure = true;
				}
			}
		}

		if (!player_colliding_with_structure)
		{
			player->on_no_collision();
		}
	}

	// update some player things after collisions have possible altered position
	for (const auto& object : *this->player_objects_)
	{
		object->update_weapon_position();
		object->update_prev_rectangle();
	}

	// check collision objects collisions
	for (auto& object : *this->collision_objects_)
	{
		// check collision object collisions with players
			for (auto& player : *this->player_objects_)
			{
				if (object->for_deletion() || player->for_deletion())
				{
					continue;
				}
				if (object->is_colliding(player.get()))
				{
					object->on_collision(player.get());
					player->on_collision(object.get());
				}
			}
		
		
		// check collision object collisions with other collision objects
		for (auto& object_2 : *this->collision_objects_)
		{
			if (object->for_deletion() || object_2->for_deletion())
			{
				continue;
			}
			if (object == object_2)
			{
				continue;
			}
			if (object->is_colliding(object_2.get()))
			{
				object->on_collision(object_2.get());
				object_2->on_collision(object.get());
			}
		}
	}

	// A projectile that leaves the level is ordinary - it missed. Retire it
	// with everything else being deleted this frame rather than throwing, which
	// terminated the process because nothing on the tick path catches.
	for (auto& object : *this->collision_objects_)
	{
		if (!object->for_deletion() &&
			this->is_object_out_of_bounds(object.get()))
		{
			object->set_for_deletion(true);
		}
	}

	// check for deletable objects
	for (size_t i = 0; i < this->collision_objects_->size(); i++)
	{
		if (this->collision_objects_->at(i)->for_deletion())
		{
			this->collision_objects_->at(i) = std::move(
				this->collision_objects_->back());
			this->collision_objects_->pop_back();
			i--;
		}
	}

	// A player leaving the level is a genuine simulation failure, not a
	// gameplay event, so it still reports loudly.
	for (auto& player : *this->player_objects_)
	{
		if (this->is_object_out_of_bounds(player.get()))
		{
			throw std::runtime_error("Player out of bounds");
		}
	}
}
void Level::stop_player_sounds() const
{
	for (const auto& player : *this->player_objects_)
	{
		player->stop_sounds();
	}

}
//void Level::draw()
//{		
//	if (this->state_ == LevelState::start_countdown)
//	{
//		this->draw_active_level();
//	}
//	else if (this->state_ == LevelState::active)
//	{
//		this->draw_active_level();
//	}
//	else if (this->state_ == LevelState::zoom_out)
//	{
//		this->draw_zoom_out_level();
//	}
//	else if (this->state_ == LevelState::overview)
//	{
//		this->draw_zoom_out_level();
//	}
//	else if (this->state_ == LevelState::finished)
//	{
//		this->draw_zoom_out_level();
//	}
//}
void Level::draw(std::vector<ID3D11DeviceContext*>* deferred_contexts,
	std::vector<ID3D11CommandList*>* command_lists,
	std::vector<SpriteBatch*>* sprite_batches) const
{
	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		this->draw_active_level(deferred_contexts,
			command_lists,
			sprite_batches);
	}
	else if (this->state_ == LevelState::zoom_out ||
		this->state_ == LevelState::overview ||
		this->state_ == LevelState::finished)
	{
		this->draw_zoom_out_level(deferred_contexts,
			command_lists,
			sprite_batches);
	}
}

void Level::draw_active_level(std::vector<ID3D11DeviceContext*>* deferred_contexts,
		std::vector<ID3D11CommandList*>* command_lists,
		std::vector<SpriteBatch*>* sprite_batches) const
{
	int num_threads = this->thread_pool_->max_num_threads();

	// partition player objects
	auto partitioned_players =
		this->partitioner_->partition(this->player_objects_->size(), num_threads);

	// draw player objects
	for (int i = 0; i < partitioned_players.size(); i++)
	{
		this->thread_pool_->add_task([this, i, partitioned_players,
			deferred_contexts, command_lists, sprite_batches]()
			{
				this->draw_player_view_level(partitioned_players[i].first,
				partitioned_players[i].second,
				deferred_contexts,
				command_lists,
				sprite_batches);
			});
	}

	this->thread_pool_->wait_for_tasks_to_complete();
}

//void Level::draw_active_level(std::vector<ID3D11DeviceContext*>* deferred_contexts,
//	std::vector<ID3D11CommandList*>* command_lists,
//	std::vector<SpriteBatch*>* sprite_batches) const
//{
//	for (auto& player : *this->player_objects_)
//	{
//		const int player_num = player->player_num();
//
//		// apply player viewport
//		this->viewport_manager_->apply_player_viewport(
//			player_num);
//
//		const Camera& camera = player->camera();
//		const RectangleF camera_view =
//			this->viewport_manager_->camera_adjusted_player_viewport_rect(
//				player_num, camera);
//
//		this->sprite_batch_->Begin(SpriteSortMode_Deferred, nullptr, this->sampler_state_);
//
//		// draw non-collision objects
//		for (const auto& object : *this->non_collision_objects_)
//		{
//			if (object->is_visible_in_viewport(camera_view))
//			{
//				object->draw(camera);
//			}
//		}
//		// draw collision objects
//		for (const auto& object : *this->collision_objects_)
//		{
//			if (object->is_visible_in_viewport(camera_view))
//			{
//				object->draw(camera);
//			}
//		}
//		// draw player objects
//		for (const auto& object : *this->player_objects_)
//		{
//			if (object->is_visible_in_viewport(camera_view))
//			{
//				object->draw(camera);
//			}
//		}
//
//		// draw viewport dividers
//		const Viewport player_vp =
//			this->viewport_manager_->player_viewport(player_num);
//		const auto viewport_camera = Camera(player_vp);
//		for (const auto& divider : *this->viewport_dividers_)
//		{
//			divider->draw(viewport_camera);
//		}
//
//		this->sprite_batch_->End();
//
//		const PlayerState state = player->state();
//
//		// draw interface
//		Vector2F viewport_size = player_vp.size();
//
//		this->interface_gameplay_->draw_gameplay_interface(
//			viewport_size,
//			player->health(),
//			player->weapon_ammo(),
//			this->timer_,
//			player->team_colour(),
//			this->sampler_state_,
//			player->respawn_timer(),
//			state == PlayerState::dead);
//
//		// draw debug info
//		if (player->showing_debug())
//		{
//			int num_projectiles = this->count_projectiles();
//			this->debug_text_->draw_debug_info(player.get(), num_projectiles);
//		}
//
//		this->sprite_batch_->Begin();
//
//		// draw countdown text
//		if (this->state_ == LevelState::start_countdown ||
//			this->start_timer_ > -1.0f)
//		{
//			this->countdown_text_->draw(viewport_camera);
//		}
//
//		this->sprite_batch_->End();
//	}
//}
void Level::draw_player_view_level(int start, int end,
	std::vector<ID3D11DeviceContext*>* deferred_contexts,
	std::vector<ID3D11CommandList*>* command_lists,
	std::vector<SpriteBatch*>* sprite_batches) const
{
	for (int i = start; i < end; i++)
	{
		if (deferred_contexts->at(i)->GetType() != D3D11_DEVICE_CONTEXT_DEFERRED)
		{
			throw std::exception("Deferred context not created");
		}

		const int player_num = this->player_objects_->at(i)->player_num();

		this->viewport_manager_->apply_player_viewport(
			player_num, deferred_contexts->at(i), sprite_batches->at(i));

		const Camera& camera = this->player_objects_->at(i)->camera();
		const RectangleF camera_view =
			this->viewport_manager_->camera_adjusted_player_viewport_rect(
				player_num, camera);

		sprite_batches->at(i)->Begin(SpriteSortMode_Deferred, nullptr, this->sampler_state_);

		// draw non-collision objects
		for (auto& object : *this->non_collision_objects_)
		{
			if (object->is_visible_in_viewport(camera_view))
			{
				object->draw(sprite_batches->at(i), camera);
			}
		}

		// draw collision objects
		for (auto& object : *this->collision_objects_)
		{
			if (object->is_visible_in_viewport(camera_view))
			{
				object->draw(sprite_batches->at(i), camera);
			}
		}

		// draw player objects
		for (auto& object : *this->player_objects_)
		{
			if (object->is_visible_in_viewport(camera_view))
			{
				object->draw(sprite_batches->at(i), camera);
			}
		}

		// draw viewport dividers
		const Viewport player_vp =
			this->viewport_manager_->player_viewport(player_num);

		const Camera viewport_camera = Camera(player_vp);

		for (auto& divider : *this->viewport_dividers_)
		{
			divider->draw(sprite_batches->at(i), viewport_camera);
		}

		sprite_batches->at(i)->End();

		const PlayerState state =  this->player_objects_->at(i)->state();

		// draw interface
		Vector2F viewport_size = player_vp.size();

		this->interface_gameplay_->draw_gameplay_interface(
			sprite_batches->at(i),
			viewport_size,
			this->player_objects_->at(i)->health(),
			this->player_objects_->at(i)->weapon_ammo(),
			this->timer_,
			this->player_objects_->at(i)->team_colour(),
			this->sampler_state_,
			this->player_objects_->at(i)->respawn_timer(),
			state == PlayerState::dead);

		// draw debug info
		if (this->player_objects_->at(i)->showing_debug())
		{
			int num_projectiles = this->count_projectiles();

			this->debug_text_->draw_debug_info(sprite_batches->at(i),
				this->player_objects_->at(i).get(), num_projectiles,
				this->frame_dt_);
		}

		// draw countdown text
		if (this->state_ == LevelState::start_countdown ||
			this->start_timer_ > -1.0f)
		{
			this->draw_countdown_text(sprite_batches->at(i), viewport_camera);
		}

		HRESULT hr = deferred_contexts->at(i)->FinishCommandList(TRUE, &command_lists->at(i));
		if (FAILED(hr))
		{
			throw std::exception("Failed to finish command list");
		}
	}
}

void Level::draw_zoom_out_level(std::vector<ID3D11DeviceContext*>* deferred_contexts,
	std::vector<ID3D11CommandList*>* command_lists,
	std::vector<SpriteBatch*>* sprite_batches) const
{
	// Submitting exactly one task and then immediately blocking on it gains no
	// parallelism and pays a full thread-pool round trip. Record it here, on
	// the calling thread.
	this->draw_zoom_out_level_component(deferred_contexts, command_lists,
		sprite_batches);
}

void Level::draw_zoom_out_level_component(
	std::vector<ID3D11DeviceContext*>* deferred_contexts,
	std::vector<ID3D11CommandList*>* command_lists,
	std::vector<DirectX::SpriteBatch*>* sprite_batches) const
{
	if (deferred_contexts->at(0)->GetType() != D3D11_DEVICE_CONTEXT_DEFERRED)
	{
		throw std::runtime_error("Deferred context not created");
	}

	const Camera& camera = this->zoom_out_camera_;

	ID3D11DeviceContext* context = deferred_contexts->at(0);
	SpriteBatch* sprite_batch = sprite_batches->at(0);

	// Compute the fullscreen viewport and apply it to this context only.
	// This used to call ViewportManager::set_layout(ONE_PLAYER), permanently
	// clobbering shared presentation state from inside a draw call: the layout
	// was never restored, so restarting a 2-4 player match rendered every
	// camera into one fullscreen viewport with no split-screen dividers.
	const D3D11_VIEWPORT viewport =
		this->viewport_manager_->fullscreen_d3d11_viewport();
	context->RSSetViewports(1, &viewport);
	sprite_batch->SetViewport(viewport);

	sprite_batch->Begin(SpriteSortMode_Deferred, nullptr, this->sampler_state_);

	// draw non-collision objects
	for (const auto& object : *this->non_collision_objects_)
	{
		object->draw(sprite_batch, camera);
	}
	// draw collision objects
	for (const auto& object : *this->collision_objects_)
	{
		object->draw(sprite_batch, camera);
	}
	// draw player objects
	for (const auto& object : *this->player_objects_)
	{
		object->draw(sprite_batch, camera);
	}

	sprite_batch->End();

	HRESULT hr = deferred_contexts->at(0)->FinishCommandList(TRUE, &command_lists->at(0));
	if (FAILED(hr))
	{
		throw std::exception("Failed to finish command list");
	}
}

LevelState Level::state() const
{
	return this->state_;
}

int Level::count_projectiles() const
{
	int count = 0;
	for (const auto& object : *this->collision_objects_)
	{
		CollisionObjectType type = object->collision_object_type();

		bool is_projectile =
			type == CollisionObjectType::projectile_spray_team_a ||
			type == CollisionObjectType::projectile_spray_team_b ||
			type == CollisionObjectType::projectile_jet_team_a ||
			type == CollisionObjectType::projectile_jet_team_b ||
			type == CollisionObjectType::projectile_rolling_team_a ||
			type == CollisionObjectType::projectile_rolling_team_b ||
			type == CollisionObjectType::projectile_ball_team_a ||
			type == CollisionObjectType::projectile_ball_team_b ||
			type == CollisionObjectType::projectile_mist_team_a ||
			type == CollisionObjectType::projectile_mist_team_b;
		if (is_projectile)
		{
			count++;
		}
	}
	return count;
}


bool Level::is_object_out_of_bounds(const ICollisionGameObject* object) const
{
	bool object_in_bounds =
		this->out_of_bounds_.intersects(object->shape()->bounding_box());
	return !object_in_bounds;
}

void Level::draw_end_screen()
{
	// TODO
}

void Level::set_state(LevelState state)
{
	this->state_ = state;
}

LevelEndInfo Level::level_end_info() const
{
	auto result = LevelEndInfo();
	result.team_colours = this->team_colours_;

	for (auto& object : *this->collision_objects_)
	{
		CollisionObjectType type = object->collision_object_type();

		if (type == CollisionObjectType::structure_paintable)
		{
			// dynamic_cast on a POINTER returns nullptr on failure; only the
			// reference form throws bad_cast. The catch here could never fire,
			// and the null was dereferenced unchecked one line above it.
			auto paintable_object =
				dynamic_cast<IPaintableGameObject*>(object.get());

			if (paintable_object == nullptr)
			{
				throw std::runtime_error(
					"Object tagged STRUCTURE_PAINTABLE is not an IPaintableGameObject");
			}

			PaintTotal paint = paintable_object->paint_total();

			result.team_a_area += paint.team_a;
			result.team_b_area += paint.team_b;
		}
	}

	return result;
}

void Level::draw_countdown_text(SpriteBatch* sprite_batch,
	const mattmath::Camera& viewport_camera) const
{
	sprite_batch->Begin();
	
	this->countdown_text_->draw(sprite_batch, viewport_camera);

	sprite_batch->End();
}