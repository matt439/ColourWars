#include "pch.h"
#include "level.h"

using namespace MattMath;
using namespace DirectX;
using namespace level_consts;

Level::Level(std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> non_collision_objects,
	std::unique_ptr<std::vector<std::unique_ptr<ICollisionGameObject>>> collision_objects,
	std::unique_ptr<std::vector<std::unique_ptr<Player>>> player_objects,
	std::unique_ptr<std::vector<std::unique_ptr<IGameObject>>> viewport_dividers,
	level_stage stage,
	const team_colour& team_colours,
	const RectangleF& out_of_bounds,
	const RectangleF& camera_bounds,
	const RectangleF& zoom_out_start_bounds,
	const RectangleF& zoom_out_finish_bounds,
	const std::vector<Vector2F>& team_a_spawns,
	const std::vector<Vector2F>& team_b_spawns,
	const std::string& sound_bank_name,
	const std::string& music_name,
	float music_volume,
	const float* dt,
	SpriteBatch* sprite_batch,
	ID3D11SamplerState* sampler_state,
	const std::string& level_name,
	const ResolutionManager* resolution_manager,
	ViewportManager* viewport_manager,
	ResourceManager* resource_manager) :
	_non_collision_objects(std::move(non_collision_objects)),
	_collision_objects(std::move(collision_objects)),
	_player_objects(std::move(player_objects)),
	_viewport_dividers(std::move(viewport_dividers)),
	_stage(stage),
	_team_colours(team_colours),
	_out_of_bounds(out_of_bounds),
	_camera_bounds(camera_bounds),
	_zoom_out_start_bounds(zoom_out_start_bounds),
	_zoom_out_finish_bounds(zoom_out_finish_bounds),
	_team_a_spawns(team_a_spawns),
	_team_b_spawns(team_b_spawns),
	_music_name(music_name),
	_music_volume(music_volume),
	_dt(dt),
	_sprite_batch(sprite_batch),
	_sampler_state(sampler_state),
	_level_name(level_name),
	_resolution_manager(resolution_manager),
	_viewport_manager(viewport_manager),
	_resource_manager(resource_manager)
{
	this->_debug_text = std::make_unique<DebugText>(sprite_batch,
		resource_manager, dt, resolution_manager);
	this->_camera_tools = std::make_unique<CameraTools>();
	this->_interface_gameplay = std::make_unique<InterfaceGameplay>(
		sprite_batch, resource_manager, dt);
	this->_sound_bank = resource_manager->get_sound_bank(sound_bank_name);
}
void Level::stop_music() const
{
	this->_sound_bank->stop_effect(this->_music_name, true);
}
void Level::update(const std::vector<player_input>& player_inputs)
{
	if (this->_state == level_state::START_COUNTDOWN ||
		this->_start_timer > -1.0f)
	{
		// first update
		if (this->_start_timer >= START_TIMER)
		{
			this->_sound_bank->play_effect(this->_music_name, true, this->_music_volume);
			this->_sound_bank->play_wave(COUNTDOWN_SOUND, COUNTDOWN_SOUND_VOLUME);
			
			// update player cameras
			int player_index = 0;
			for (auto& object : *this->_player_objects)
			{
				Camera camera = this->_camera_tools->calculate_camera(
					object->get_center(),
					this->_viewport_manager->get_player_viewport(player_index).get_size(),
					object->get_camera(),
					this->_camera_bounds);
				object->set_camera(camera);
			}

			// create countdown text
			Vector2F resolution = this->_resolution_manager->get_resolution_vec();
			Vector2F position = resolution / 2.0f;
			position = position - Vector2F(COUNTDOWN_TEXT_WIDTH / 2.0f,
								COUNTDOWN_TEXT_HEIGHT / 2.0f);

			this->_countdown_text = std::make_unique<TextDropShadow>(
				COUNTDOWN_TEXT,
				COUNTDOWN_FONT_NAME,
				position,
				this->_sprite_batch,
				this->_resource_manager,
				COUNTDOWN_COLOUR,
				COUNTDOWN_SHADOW_COLOUR,
				COUNTDOWN_SHADOW_OFFSET,
				COUNTDOWN_SCALE,
				COUNTDOWN_SCALE);
		}
		this->_start_timer -= this->get_dt();
		if (this->_start_timer > 0.0f)
		{
			this->_countdown_text->set_text(std::to_string(
				static_cast<int>(this->_start_timer) + 1));
		}
		else
		{
			this->_countdown_text->set_text("GO!");
		}

		if (this->_start_timer <= 0.0f)
		{
			this->_state = level_state::ACTIVE;
		}
		else
		{
			return;
		}
	}
	if (this->_state == level_state::ACTIVE)
	{
		this->update_level_logic(player_inputs);
		
		this->_timer -= this->get_dt();
		if (this->_timer <= 0.0f)
		{
			this->_state = level_state::ZOOM_OUT;
			this->_sound_bank->stop_effect(this->_music_name, true);
			this->stop_player_sounds();
			this->_sound_bank->play_effect(ZOOM_OUT_SOUND, false, ZOOM_OUT_SOUND_VOLUME);
		}
	}
	else if (this->_state == level_state::ZOOM_OUT)
	{		
		this->_zoom_out_timer -= this->get_dt();

		if (this->_zoom_out_timer <= 0.0f)
		{
			this->_state = level_state::OVERVIEW;
			this->_zoom_out_timer = 0.0f;
			this->_sound_bank->stop_effect(ZOOM_OUT_SOUND, true);
		}

		Camera start_camera = Camera::calculate_camera_from_view_rectangle(
			this->_zoom_out_start_bounds,
			RectangleF(Vector2F::ZERO, this->_resolution_manager->get_resolution_vec()));

		Camera finish_camera = Camera::calculate_camera_from_view_rectangle(
			this->_zoom_out_finish_bounds,
			RectangleF(Vector2F::ZERO, this->_resolution_manager->get_resolution_vec()));

		this->_zoom_out_camera = Camera::calculate_intermediate_camera(
			start_camera, finish_camera, this->zoom_out_camera_ratio());
	}
	else if (this->_state == level_state::OVERVIEW)
	{
		this->_overview_timer -= this->get_dt();
		if (this->_overview_timer <= 0.0f)
		{
			this->_state = level_state::FINISHED;
		}
	}
	else if (this->_state == level_state::FINISHED)
	{
		this->_sound_bank->stop_effect(this->_music_name, true);
		return;
	}
	
}
float Level::zoom_out_camera_ratio()
{
	return 1.0f - (this->_zoom_out_timer / ZOOM_OUT_TIMER);
}
void Level::update_level_logic(const std::vector<player_input>& player_inputs)
{
	// update collision objects
	for (auto& object : *this->_collision_objects)
	{
		object->update();
	}
	
	// update player objects
	int player_index = 0;
	for (auto& object : *this->_player_objects)
	{		
		object->set_player_input(player_inputs[player_index]);
		object->update();

		// update player camera
		Camera camera = this->_camera_tools->calculate_camera(
			object->get_center(),
			this->_viewport_manager->get_player_viewport(player_index).get_size(),
			object->get_camera(),
			this->_camera_bounds);
		object->set_camera(camera);

		// update player weapon
		std::vector<std::unique_ptr<ICollisionGameObject>> new_projs =
			object->update_weapon_and_get_projectiles();
		// add new projectiles to collision objects
		for (auto& proj : new_projs)
		{
			this->_collision_objects->push_back(std::move(proj));
		}

		player_index++;
	}

	// update non-collision objects
	for (auto& object : *this->_non_collision_objects)
	{
		object->update();
	}

	// check player collisions
	for (auto& player : *this->_player_objects)
	{
		// check player collisions with collision objects
		for (auto& other_object : *this->_collision_objects)
		{
			if (other_object->get_for_deletion())
			{
				continue;
			}
			if (player->is_colliding(other_object.get()))
			{
				player->on_collision(other_object.get());
				other_object->on_collision(player.get());
			}
		}
	}

	// update some player things after collisions have possible altered position
	for (auto& object : *this->_player_objects)
	{
		object->update_weapon_position();
		object->update_prev_rectangle();
	}

	// check collision objects collisions
	for (auto& object : *this->_collision_objects)
	{
		// check collision object collisions with players
			for (auto& player : *this->_player_objects)
			{
				if (object->get_for_deletion() || player->get_for_deletion())
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
		for (auto& object_2 : *this->_collision_objects)
		{
			if (object->get_for_deletion() || object_2->get_for_deletion())
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

	// check for deletable objects
	for (size_t i = 0; i < this->_collision_objects->size(); i++)
	{
		if (this->_collision_objects->at(i)->get_for_deletion())
		{
			this->_collision_objects->at(i) = std::move(
				this->_collision_objects->back());
			this->_collision_objects->pop_back();
			i--;
		}
	}

	// check for out of bounds objects
	for (auto& player : *this->_player_objects)
	{
		if (this->is_object_out_of_bounds(player.get()))
		{
			throw std::exception("Player out of bounds");
		}
	}
	for (auto& object : *this->_collision_objects)
	{
		if (this->is_object_out_of_bounds(object.get()))
		{
			throw std::exception("Collision object out of bounds");
		}
	}
}
void Level::stop_player_sounds() const
{
	for (auto& player : *this->_player_objects)
	{
		player->stop_sounds();
	}

}
void Level::draw()
{		
	if (this->_state == level_state::START_COUNTDOWN)
	{
		this->draw_active_level();
	}
	else if (this->_state == level_state::ACTIVE)
	{
		this->draw_active_level();
	}
	else if (this->_state == level_state::ZOOM_OUT)
	{
		this->draw_zoom_out_level();
	}
	else if (this->_state == level_state::OVERVIEW)
	{
		this->draw_zoom_out_level();
	}
	else if (this->_state == level_state::FINISHED)
	{
		this->draw_zoom_out_level();
	}
}
void Level::draw_active_level()
{
	for (auto& player : *this->_player_objects)
	{
		const int player_num = player->get_player_num();

		// apply player viewport
		this->_viewport_manager->apply_player_viewport(
			player_num);

		const Camera& camera = player->get_camera();
		const RectangleF camera_view =
			this->_viewport_manager->get_camera_adjusted_player_viewport_rect(
				player_num, camera);

		this->_sprite_batch->Begin(SpriteSortMode_Deferred, nullptr, this->_sampler_state);

		// draw non-collision objects
		for (auto& object : *this->_non_collision_objects)
		{
			if (object->is_visible_in_viewport(camera_view))
			{
				object->draw(camera);
			}
		}
		// draw collision objects
		for (auto& object : *this->_collision_objects)
		{
			if (object->is_visible_in_viewport(camera_view))
			{
				object->draw(camera);
			}
		}
		// draw player objects
		for (auto& object : *this->_player_objects)
		{
			if (object->is_visible_in_viewport(camera_view))
			{
				object->draw(camera);
			}
		}

		// draw viewport dividers
		const Viewport player_vp =
			this->_viewport_manager->get_player_viewport(player_num);
		const Camera viewport_camera = Camera(player_vp);
		for (auto& divider : *this->_viewport_dividers)
		{
			divider->draw(viewport_camera);
		}

		this->_sprite_batch->End();

		player_state state = player->get_state();

		// draw interface
		Vector2F viewport_size = player_vp.get_size();

		this->_interface_gameplay->draw_gameplay_interface(
			viewport_size,
			player->get_health(),
			player->get_weapon_ammo(),
			this->_timer,
			player->get_team_colour(),
			this->_sampler_state,
			player->get_respawn_timer(),
			state == player_state::DEAD);

		// draw debug info
		if (player->get_showing_debug())
		{
			int num_projectiles = this->count_projectiles();
			this->_debug_text->draw_debug_info(player.get(), num_projectiles);
		}

		this->_sprite_batch->Begin();

		// draw countdown text
		if (this->_state == level_state::START_COUNTDOWN ||
			this->_start_timer > -1.0f)
		{
			this->_countdown_text->draw(viewport_camera);
		}

		this->_sprite_batch->End();
	}
}
void Level::draw_zoom_out_level()
{
	this->_viewport_manager->set_layout(screen_layout::ONE_PLAYER);
	this->_viewport_manager->apply_player_viewport(0);

	const Camera& camera = this->_zoom_out_camera;

	this->_sprite_batch->Begin(SpriteSortMode_Deferred, nullptr, this->_sampler_state);

	// draw non-collision objects
	for (auto& object : *this->_non_collision_objects)
	{
		object->draw(camera);
	}
	// draw collision objects
	for (auto& object : *this->_collision_objects)
	{
		object->draw(camera);
	}
	// draw player objects
	for (auto& object : *this->_player_objects)
	{
		object->draw(camera);
	}

	this->_sprite_batch->End();
}
level_state Level::get_state() const
{
	return this->_state;
}

int Level::count_projectiles() const
{
	int count = 0;
	for (auto& object : *this->_collision_objects)
	{
		CollisionObjectType type = object->get_collision_object_type();

		bool is_projectile =
			type == CollisionObjectType::PROJECTILE_SPRAY_TEAM_A ||
			type == CollisionObjectType::PROJECTILE_SPRAY_TEAM_B ||
			type == CollisionObjectType::PROJECTILE_JET_TEAM_A ||
			type == CollisionObjectType::PROJECTILE_JET_TEAM_B ||
			type == CollisionObjectType::PROJECTILE_ROLLING_TEAM_A ||
			type == CollisionObjectType::PROJECTILE_ROLLING_TEAM_B ||
			type == CollisionObjectType::PROJECTILE_BALL_TEAM_A ||
			type == CollisionObjectType::PROJECTILE_BALL_TEAM_B ||
			type == CollisionObjectType::PROJECTILE_MIST_TEAM_A ||
			type == CollisionObjectType::PROJECTILE_MIST_TEAM_B;
		if (is_projectile)
		{
			count++;
		}
	}
	return count;
}

float Level::get_dt() const
{
	return *this->_dt;
}

bool Level::is_object_out_of_bounds(const ICollisionGameObject* object) const
{
	bool object_in_bounds =
		this->_out_of_bounds.intersects(object->get_shape()->get_bounding_box());
	return !object_in_bounds;
}

void Level::draw_end_screen() const
{
	// TODO
}

void Level::set_state(level_state state)
{
	this->_state = state;
}

level_end_info Level::get_level_end_info() const
{
	level_end_info result = level_end_info();
	result.team_colours = this->_team_colours;

	for (auto& object : *this->_collision_objects)
	{
		CollisionObjectType type = object->get_collision_object_type();

		if (type == CollisionObjectType::STRUCTURE_PAINTABLE)
		{
			try
			{
				IPaintableGameObject* paintable_object =
					dynamic_cast<IPaintableGameObject*>(object.get());

				paint_total paint = paintable_object->get_paint_total();

				if (paint.team_a != 0.0f || paint.team_b != 0.0f)
				{
					int x = 0;
				}

				result.team_a_area += paint.team_a;
				result.team_b_area += paint.team_b;
			}
			catch (const std::bad_cast& e)
			{
				throw std::exception("Could not cast to IPaintableGameObject");
			}
		}
	}

	return result;
}