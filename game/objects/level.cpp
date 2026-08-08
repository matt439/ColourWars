#include "game/pch.h"
#include "game/objects/level.h"

using namespace mattmath;
using namespace DirectX;
using namespace level_consts;
using namespace artattack;

Level::Level(std::unique_ptr<std::vector<std::unique_ptr<GameObject>>> non_collision_objects,
	std::unique_ptr<std::vector<std::unique_ptr<CollisionObject>>> collision_objects,
	std::unique_ptr<std::vector<std::unique_ptr<Player>>> player_objects,
	std::unique_ptr<std::vector<std::unique_ptr<GameObject>>> viewport_dividers,
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

	this->build_opening_frame();
}

Level::~Level()
{
	this->sound_bank_->stop_effect(this->music_, true);
	this->sound_bank_->stop_effect(this->zoom_out_sound_, true);
	this->stop_player_sounds();
}

void Level::build_opening_frame()
{
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

	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const Vector2F position = resolution / 2.0f -
		Vector2F(COUNTDOWN_TEXT_WIDTH / 2.0f, COUNTDOWN_TEXT_HEIGHT / 2.0f);

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

void Level::suspend() const
{
	this->sound_bank_->pause_effect(this->music_);
	this->sound_bank_->pause_effect(this->zoom_out_sound_);
	this->pause_player_sounds();
}

void Level::resume() const
{
	this->sound_bank_->resume_effect(this->music_);
	this->sound_bank_->resume_effect(this->zoom_out_sound_);
	this->resume_player_sounds();
}
void Level::update(const std::vector<PlayerInputData>& player_inputs,
	float dt)
{
	this->frame_dt_ = dt;
	if (this->state_ == LevelState::start_countdown ||
		this->start_timer_ > -1.0f)
	{
		// The first update, and now the only thing left in it: the cameras and
		// the countdown text are built with the level, because the frame drawn
		// before this one is a real frame and used to be skipped rather than
		// made correct.
		if (this->start_timer_ >= START_TIMER)
		{
			this->sound_bank_->play_effect(this->music_, true, this->music_volume_);
			this->sound_bank_->play_wave(this->countdown_sound_, COUNTDOWN_SOUND_VOLUME);
		}
		this->start_timer_ -= dt;
		if (this->start_timer_ > 0.0f)
		{
			this->countdown_text_->set_text(std::to_wstring(
				static_cast<int>(this->start_timer_) + 1));
		}
		else
		{
			this->countdown_text_->set_text(L"GO!");
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
	float dt)
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
		std::vector<std::unique_ptr<CollisionObject>> new_projs =
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

	// One sweep, one pass, every pair once.
	//
	// What this replaces was three nested loops - players against objects,
	// then objects against players, then objects against objects with both
	// orderings - so most pairs were measured twice and a pair's response
	// depended on which loop reached it first. Both participants' responses
	// fired off one participant's predicate, and the predicates disagreed:
	// Player::is_colliding matched structures only, so a player's response to
	// a projectile ran only because the projectile's predicate had matched.
	this->collidables_.clear();
	this->collidables_.reserve(
		this->collision_objects_->size() + this->player_objects_->size());
	for (const auto& object : *this->collision_objects_)
	{
		this->collidables_.push_back(object.get());
	}
	for (const auto& player : *this->player_objects_)
	{
		this->collidables_.push_back(player.get());
	}

	find_contacts(this->collidables_, this->contacts_);
	dispatch_contacts(this->contacts_);

	// "I touched no ground this tick" is the absence of a contact, so it is
	// only knowable once they have all been dispatched.
	for (const auto& player : *this->player_objects_)
	{
		player->end_contacts();
	}

	// update some player things after collisions have possible altered position
	for (const auto& object : *this->player_objects_)
	{
		object->update_weapon_position();
		object->update_prev_rectangle();
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
void Level::pause_player_sounds() const
{
	for (const auto& player : *this->player_objects_)
	{
		player->pause_sounds();
	}
}
void Level::resume_player_sounds() const
{
	for (const auto& player : *this->player_objects_)
	{
		player->resume_sounds();
	}
}
void Level::draw(Renderer& renderer) const
{
	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		this->draw_active_level(renderer);
	}
	else if (this->state_ == LevelState::zoom_out ||
		this->state_ == LevelState::overview ||
		this->state_ == LevelState::finished)
	{
		this->draw_zoom_out_level(renderer);
	}
}

void Level::draw_active_level(Renderer& renderer) const
{
	// One view per player, and the partition splits the *views* across the
	// pool - not the objects. Every worker enters draw() on every object; what
	// differs between them is which pane they are recording into. That is why
	// GameObject::draw is const.
	renderer.set_view_count(static_cast<int>(this->player_objects_->size()));

	const std::vector<std::pair<int, int>> partitioned_views =
		this->partitioner_->partition(this->player_objects_->size(),
			this->thread_pool_->max_num_threads());

	for (const std::pair<int, int>& range : partitioned_views)
	{
		this->thread_pool_->add_task([this, range, &renderer]()
			{
				this->draw_player_view_level(range.first, range.second,
					renderer);
			});
	}

	this->thread_pool_->wait_for_tasks_to_complete();
}

void Level::draw_player_view_level(int start, int end, Renderer& renderer) const
{
	for (int i = start; i < end; i++)
	{
		DrawList list = renderer.view(i);

		const int player_num = this->player_objects_->at(i)->player_num();
		const Viewport player_vp =
			this->viewport_manager_->player_viewport(player_num);
		list.set_viewport(player_vp);

		const Camera& camera = this->player_objects_->at(i)->camera();
		const RectangleF camera_view =
			this->viewport_manager_->camera_adjusted_player_viewport_rect(
				player_num, camera);

		// The world, through this player's camera.
		list.set_camera(camera);

		for (const auto& object : *this->non_collision_objects_)
		{
			if (object->bounds().intersects(camera_view))
			{
				object->draw(list);
			}
		}

		for (const auto& object : *this->collision_objects_)
		{
			if (object->bounds().intersects(camera_view))
			{
				object->draw(list);
			}
		}

		for (const auto& object : *this->player_objects_)
		{
			if (object->bounds().intersects(camera_view))
			{
				object->draw(list);
			}
		}

		// Everything from here down goes over the world, and this is the case
		// renderer.h describes for the camera being per draw-range rather than
		// per view: same viewport, same frame, three different mappings.
		//
		// The dividers are authored in screen coordinates, so they are drawn
		// through a camera that subtracts this pane's screen origin.
		list.set_camera(Camera(player_vp));

		for (const auto& divider : *this->viewport_dividers_)
		{
			divider->draw(list);
		}

		// The HUD is already in pane-local coordinates - it is laid out from
		// the viewport's own size - so it wants the identity. Both of these
		// used to be a bare SpriteBatch::Begin() with no camera anywhere near
		// them, which is what the identity means.
		list.set_camera(Camera::DEFAULT_CAMERA);

		this->interface_gameplay_->draw_gameplay_interface(list,
			player_vp.size(),
			this->player_objects_->at(i)->health(),
			this->player_objects_->at(i)->weapon_ammo(),
			this->timer_,
			this->player_objects_->at(i)->team_colour(),
			this->player_objects_->at(i)->respawn_timer(),
			this->player_objects_->at(i)->state() == PlayerState::dead);

		if (this->player_objects_->at(i)->showing_debug())
		{
			this->debug_text_->draw_debug_info(list,
				this->player_objects_->at(i).get(), this->count_projectiles(),
				this->frame_dt_);
		}

		if (this->state_ == LevelState::start_countdown ||
			this->start_timer_ > -1.0f)
		{
			// Back to the pane's camera: the countdown is positioned in screen
			// coordinates like the dividers, not in the HUD's local ones.
			list.set_camera(Camera(player_vp));
			this->draw_countdown_text(list);
		}
	}
}

void Level::draw_zoom_out_level(Renderer& renderer) const
{
	// One fullscreen view, and it is the whole of what "fullscreen" now costs.
	// Getting here used to mean calling ViewportManager::set_layout(one_player)
	// from inside a draw - permanently clobbering shared presentation state,
	// never restoring it, so restarting a 2-4 player match rendered every
	// camera into one pane with no split-screen dividers. Saying it on the list
	// says it for this frame and no further.
	renderer.set_view_count(1);
	DrawList list = renderer.view(0);

	list.set_viewport(this->viewport_manager_->fullscreen_viewport());
	list.set_camera(this->zoom_out_camera_);

	// No bounds() cull here, unlike the player views twenty lines above - and
	// this is the path that runs for the whole post-match menu flow. Left as
	// it was: culling is the scene's job and the scene does not exist yet.
	for (const auto& object : *this->non_collision_objects_)
	{
		object->draw(list);
	}
	for (const auto& object : *this->collision_objects_)
	{
		object->draw(list);
	}
	for (const auto& object : *this->player_objects_)
	{
		object->draw(list);
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
		// The ten-way comparison written out here is is_projectile(), which
		// already existed two files away.
		if (is_projectile(to_collision_type(object->tag())))
		{
			count++;
		}
	}
	return count;
}


bool Level::is_object_out_of_bounds(const CollisionObject* object) const
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
		if (to_collision_type(object->tag()) ==
			CollisionObjectType::structure_paintable)
		{
			// dynamic_cast on a POINTER returns nullptr on failure; only the
			// reference form throws bad_cast. The catch here could never fire,
			// and the null was dereferenced unchecked one line above it.
			auto paintable_object =
				dynamic_cast<PaintableObject*>(object.get());

			if (paintable_object == nullptr)
			{
				throw std::runtime_error(
					"Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
			}

			PaintTotal paint = paintable_object->paint_total();

			result.team_a_area += paint.team_a;
			result.team_b_area += paint.team_b;
		}
	}

	return result;
}

void Level::draw_countdown_text(DrawList& draw_list) const
{
	// Linear, because this is the one thing on screen that is not pixel art:
	// a 144pt face scaled by two. It was a bare SpriteBatch::Begin() before,
	// which is DirectXTK's LinearClamp by default - the only place in the
	// level that took it, and now the only place that asks for it. The list
	// goes back to point so the next range is not surprised by it.
	draw_list.set_filter(TextureFilter::linear);
	this->countdown_text_->draw(draw_list);
	draw_list.set_filter(TextureFilter::point);
}