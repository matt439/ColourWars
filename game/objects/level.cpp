#include "game/pch.h"
#include "game/objects/level.h"

using namespace mattmath;
using namespace DirectX;
using namespace level_consts;
using namespace artattack;

Level::Level(std::unique_ptr<Scene> scene,
	std::vector<Player*> players,
	std::vector<std::unique_ptr<GameObject>> viewport_dividers,
	const TeamColour& team_colours,
	const RectangleF& camera_bounds,
	const RectangleF& zoom_out_start_bounds,
	const RectangleF& zoom_out_finish_bounds,
	const std::string& sound_bank_name,
	const std::string& music_name,
	float music_volume,
	const ResolutionManager* resolution_manager,
	const ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources) :
	scene_(std::move(scene)),
	players_(std::move(players)),
	viewport_dividers_(std::move(viewport_dividers)),
	music_volume_(music_volume),
	resolution_manager_(resolution_manager),
	viewport_manager_(viewport_manager),
	team_colours_(team_colours),
	camera_bounds_(camera_bounds),
	zoom_out_start_bounds_(zoom_out_start_bounds),
	zoom_out_finish_bounds_(zoom_out_finish_bounds)
{
	this->debug_text_ = std::make_unique<DebugText>(
		render_resources, resolution_manager);
	this->interface_gameplay_ = std::make_unique<InterfaceGameplay>(
		render_resources);
	this->sound_bank_ = audio_resources->sound_bank(sound_bank_name);

	this->music_ = this->sound_bank_->resolve_effect(music_name);
	this->zoom_out_sound_ = this->sound_bank_->resolve_effect(ZOOM_OUT_SOUND);
	this->countdown_sound_ = this->sound_bank_->resolve_wave(COUNTDOWN_SOUND);

	this->build_opening_frame(render_resources);
}

Level::~Level()
{
	this->sound_bank_->stop_effect(this->music_, true);
	this->sound_bank_->stop_effect(this->zoom_out_sound_, true);
	this->stop_player_sounds();
}

void Level::build_opening_frame(RenderResources* render_resources)
{
	// The builder added every object to the scene and every add is pending
	// until a tick ends, so the scene has a tick to end before it has had one.
	// The frame drawn before the first update() is a real frame.
	this->scene_->end_tick();
	this->projectile_count_ = this->count_projectiles();

	for (Player* player : this->players_)
	{
		Camera camera = this->camera_tools_.calculate_camera(
			player->center(),
			this->viewport_manager_->player_viewport(
				player->player_num()).size(),
			player->camera(),
			this->camera_bounds_);
		player->set_camera(camera);
	}

	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const Vector2F position = resolution / 2.0f -
		Vector2F(COUNTDOWN_TEXT_WIDTH / 2.0f, COUNTDOWN_TEXT_HEIGHT / 2.0f);

	this->countdown_text_ = std::make_unique<TextDropShadow>(
		COUNTDOWN_TEXT,
		COUNTDOWN_FONT_NAME,
		position,
		render_resources,
		COUNTDOWN_COLOUR,
		COUNTDOWN_SHADOW_COLOUR,
		COUNTDOWN_SHADOW_OFFSET,
		COUNTDOWN_SCALE,
		COUNTDOWN_SCALE);

	this->refresh_views();
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

void Level::update(const std::vector<PlayerInputData>& player_inputs, float dt)
{
	this->frame_dt_ = dt;
	this->update_state(player_inputs, dt);

	// Last, and on every path: the views describe the state the tick left
	// behind, and draw() picks its overlay off the same state_ this saw.
	this->refresh_views();
}

void Level::update_state(const std::vector<PlayerInputData>& player_inputs,
	float dt)
{
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
	// The whole of a tick, in the order the scene names its phases. What this
	// replaces was a hundred lines of loops over three object lists - two
	// update sweeps, a collidables rebuild, the collision dispatch, an
	// out-of-bounds pass and a swap-and-pop compaction - none of which was
	// about paint.
	this->apply_player_inputs(player_inputs);

	this->scene_->update(dt);

	this->update_players(dt);

	this->scene_->resolve();

	// "I touched no ground this tick" is the absence of a contact, so it is
	// only knowable once they have all been dispatched.
	for (Player* player : this->players_)
	{
		player->end_contacts();
	}

	this->update_players_after_resolve();

	// Retires what left the arena or died, and admits the projectiles fired
	// above - which is why a projectile's first sweep is the next tick's and
	// not this one's. It has to be: the players and their projectiles are one
	// list now, so a push from inside the loop walking it would invalidate the
	// walk.
	this->scene_->end_tick();

	// Once per tick, for the debug overlay. It used to be once per view, from
	// inside the draw.
	this->projectile_count_ = this->count_projectiles();

	// A player leaving the level is a genuine simulation failure, not a
	// gameplay event, so it still reports loudly rather than being retired with
	// the projectiles that missed.
	for (const Player* player : this->players_)
	{
		if (!this->scene_->in_bounds(*player))
		{
			throw std::runtime_error("Player out of bounds");
		}
	}
}

void Level::apply_player_inputs(
	const std::vector<PlayerInputData>& player_inputs)
{
	for (Player* player : this->players_)
	{
		const int slot = player->player_num();

		if (slot >= 0 && static_cast<size_t>(slot) < player_inputs.size())
		{
			player->set_player_input(player_inputs[slot]);
		}
		else
		{
			player->set_player_input(PlayerInputData());
		}
	}
}

void Level::update_players(float dt)
{
	for (Player* player : this->players_)
	{
		Camera camera = this->camera_tools_.calculate_camera(
			player->center(),
			this->viewport_manager_->player_viewport(
				player->player_num()).size(),
			player->camera(),
			this->camera_bounds_);
		player->set_camera(camera);

		for (std::unique_ptr<CollisionObject>& projectile :
			player->update_weapon_and_get_projectiles(dt))
		{
			this->scene_->add(std::move(projectile));
		}
	}
}

void Level::update_players_after_resolve()
{
	for (Player* player : this->players_)
	{
		player->update_weapon_position();
		player->update_prev_rectangle();
	}
}

void Level::refresh_views()
{
	this->scene_->clear_views();

	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		// One pane per player, in player order - which is the order the
		// overlay reads players_ back in.
		for (const Player* player : this->players_)
		{
			this->scene_->add_view(
				this->viewport_manager_->player_viewport(player->player_num()),
				player->camera());
		}
	}
	else
	{
		// One fullscreen view, and it is the whole of what "fullscreen" now
		// costs. Getting here used to mean calling
		// ViewportManager::set_layout(one_player) from inside a draw -
		// permanently clobbering shared presentation state, never restoring it,
		// so restarting a 2-4 player match rendered every camera into one pane
		// with no split-screen dividers. Saying it on the view list says it for
		// this frame and no further.
		this->scene_->add_view(this->viewport_manager_->fullscreen_viewport(),
			this->zoom_out_camera_);
	}
}

void Level::stop_player_sounds() const
{
	for (const Player* player : this->players_)
	{
		player->stop_sounds();
	}
}

void Level::pause_player_sounds() const
{
	for (const Player* player : this->players_)
	{
		player->pause_sounds();
	}
}

void Level::resume_player_sounds() const
{
	for (const Player* player : this->players_)
	{
		player->resume_sounds();
	}
}

void Level::draw(Renderer& renderer) const
{
	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		this->scene_->draw(renderer,
			[this](int view_index, DrawList& list)
			{
				this->draw_player_overlay(view_index, list);
			});
	}
	else
	{
		// The overview has no HUD, no dividers and no countdown: one fullscreen
		// view of the world, and nothing over it.
		this->scene_->draw(renderer);
	}
}

void Level::draw_player_overlay(int view_index, DrawList& list) const
{
	const Player* player = this->players_[static_cast<size_t>(view_index)];

	// The pane the scene has just drawn the world into, rather than a second
	// question to the layout for the rectangle it was handed one of.
	const Viewport& player_vp = this->scene_->view(view_index).viewport;

	// Everything from here down goes over the world, and this is the case
	// renderer.h describes for the camera being per draw-range rather than per
	// view: same viewport, same frame, three different mappings.
	//
	// The dividers are authored in screen coordinates, so they are drawn
	// through a camera that subtracts this pane's screen origin.
	list.set_camera(Camera(player_vp));

	for (const std::unique_ptr<GameObject>& divider : this->viewport_dividers_)
	{
		divider->draw(list);
	}

	// The HUD is already in pane-local coordinates - it is laid out from the
	// viewport's own size - so it wants the identity. Both of these used to be
	// a bare SpriteBatch::Begin() with no camera anywhere near them, which is
	// what the identity means.
	list.set_camera(Camera::DEFAULT_CAMERA);

	this->interface_gameplay_->draw_gameplay_interface(list,
		player_vp.size(),
		player->health(),
		player->weapon_ammo(),
		this->timer_,
		player->team_colour(),
		player->respawn_timer(),
		player->state() == PlayerState::dead);

	if (player->showing_debug())
	{
		this->debug_text_->draw_debug_info(list, player,
			this->projectile_count_, this->frame_dt_);
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

LevelState Level::state() const
{
	return this->state_;
}

int Level::count_projectiles() const
{
	int count = 0;
	for (const CollisionObject* object : this->scene_->collision_objects())
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

LevelEndInfo Level::level_end_info() const
{
	auto result = LevelEndInfo();
	result.team_colours = this->team_colours_;

	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (to_collision_type(object->tag()) ==
			CollisionObjectType::structure_paintable)
		{
			// dynamic_cast on a POINTER returns nullptr on failure; only the
			// reference form throws bad_cast. The catch here could never fire,
			// and the null was dereferenced unchecked one line above it.
			auto paintable_object = dynamic_cast<PaintableObject*>(object);

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
