#include "game/pch.h"
#include "game/objects/level.h"
#include "game/input_slots.h"

using namespace mattmath;
using namespace DirectX;
using namespace labrador;

Level::Level(std::unique_ptr<Scene> scene,
	std::vector<Player*> players,
	std::vector<std::unique_ptr<GameObject>> viewport_dividers,
	const TeamColour& team_colours,
	const RectangleF& camera_bounds,
	const RectangleF& zoom_out_start_bounds,
	const RectangleF& zoom_out_finish_bounds,
	const GameTuning* tuning,
	const Presentation* presentation,
	const std::string& sound_bank_name,
	const std::string& music_name,
	float music_volume,
	const ResolutionManager* resolution_manager,
	const ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	// Disambiguate from DirectX::Mouse.
	const labrador::Mouse* mouse) :
	scene_(std::move(scene)),
	players_(std::move(players)),
	viewport_dividers_(std::move(viewport_dividers)),
	music_volume_(music_volume),
	resolution_manager_(resolution_manager),
	viewport_manager_(viewport_manager),
	mouse_(mouse),
	team_colours_(team_colours),
	tuning_(tuning),
	camera_bounds_(camera_bounds),
	zoom_out_start_bounds_(zoom_out_start_bounds),
	zoom_out_finish_bounds_(zoom_out_finish_bounds)
{
	this->preferences_ = preferences;
	const MatchTuning& match = tuning->match();

	this->timer_ = match.duration;
	this->start_timer_ = match.countdown;
	this->zoom_out_timer_ = match.zoom_out;
	this->overview_timer_ = match.overview;

	this->debug_text_ = std::make_unique<DebugText>(
		render_resources, resolution_manager,
		&presentation->debug_overlay());
	this->interface_gameplay_ = std::make_unique<InterfaceGameplay>(
		render_resources, &presentation->hud());
	this->sound_bank_ = audio_resources->sound_bank(sound_bank_name);

	this->music_ = this->sound_bank_->resolve_effect(music_name);
	this->zoom_out_sound_ =
		this->sound_bank_->resolve_effect(match.zoom_out_sound.name);
	this->countdown_sound_ =
		this->sound_bank_->resolve_wave(match.countdown_sound.name);

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
	// Admit the builder's pending objects before the first frame is drawn.
	this->scene_->end_tick();
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (!is_projectile(to_collision_type(object->tag())))
		{
			this->projectile_targets_.push_back(object);
		}
	}
	this->projectile_count_ = this->count_projectiles();

	// Use the configured countdown for the opening beat.
	const MatchTuning& match = this->tuning_->match();
	this->countdown_text_ = std::make_unique<TextDropShadow>(
		std::to_wstring(static_cast<int>(match.countdown)),
		match.countdown_font,
		Vector2F::ZERO,
		render_resources,
		match.countdown_colour,
		match.countdown_shadow_colour,
		match.countdown_shadow_offset,
		match.countdown_scale,
		match.countdown_scale);
	this->centre_countdown_text();

	// Keep the legend synchronized with PlayerInput bindings. Include the keyboard
	// line only when a keyboard player is present; the legend is shared by all panes.
	bool anyone_on_keyboard = false;
	for (const Player* player : this->players_)
	{
		if (input_slots::is_keyboard_and_mouse(player->player_num()))
		{
			anyone_on_keyboard = true;
		}
	}

	std::wstring controls =
		L"Left stick  Move      Left stick down  Drop through      A or LT  Jump\n"
		L"RT  Shoot      Right stick  Aim      Start  Pause";

	if (anyone_on_keyboard)
	{
		controls += L"\nA/D  Move      S  Drop through      Space  Jump      "
			L"Mouse  Aim      Left click  Shoot      Esc  Pause";
	}

	this->controls_text_ = std::make_unique<TextDropShadow>(
		controls,
		match.controls_font,
		Vector2F::ZERO,
		render_resources,
		match.controls_colour,
		match.controls_shadow_colour,
		match.controls_shadow_offset);
	this->centre_controls_text();

	this->refresh_layout();
}

void Level::set_countdown_text(const std::wstring& text)
{
	this->countdown_text_->set_text(text);

	// Re-centre after each beat because the text width changes.
	this->centre_countdown_text();
}

void Level::centre_countdown_text()
{
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const MatchTuning& match = this->tuning_->match();
	// Fit text and shadow together, including negative offsets. Start from authored
	// scale each time so a larger window or narrower beat restores the intended size.
	const RectangleF bounds = RectangleF::union_of(
		this->countdown_text_->text_bounds_at(Vector2F::ZERO,
			match.countdown_scale),
		this->countdown_text_->text_bounds_at(match.countdown_shadow_offset,
			match.countdown_scale));
	const float fit = std::min({ 1.0f,
		resolution.x / bounds.width, resolution.y / bounds.height });

	this->countdown_text_->set_scale(match.countdown_scale * fit);
	this->countdown_text_->set_shadow_scale(match.countdown_scale * fit);
	this->countdown_text_->set_shadow_offset(match.countdown_shadow_offset * fit);
	this->countdown_text_->set_position(
		(resolution - bounds.size() * fit) / 2.0f - bounds.position() * fit);
}

void Level::centre_controls_text()
{
	// Fit text and shadow from authored scale on every resize. Keep the tuned
	// vertical position when the block fits.
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const MatchTuning& match = this->tuning_->match();
	const RectangleF bounds = RectangleF::union_of(
		this->controls_text_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->controls_text_->text_bounds_at(match.controls_shadow_offset, 1.0f));
	const float scale = std::min({ 1.0f,
		resolution.x / bounds.width, resolution.y / bounds.height });
	const Vector2F size = bounds.size() * scale;
	const Vector2F position((resolution.x - size.x) / 2.0f,
		std::min(resolution.y * match.controls_y, resolution.y - size.y));

	this->controls_text_->set_scale(scale);
	this->controls_text_->set_shadow_scale(scale);
	this->controls_text_->set_shadow_offset(match.controls_shadow_offset * scale);
	this->controls_text_->set_position(position - bounds.position() * scale);
}

void Level::suspend() const
{
	this->sound_bank_->pause_effect(this->music_);
	this->sound_bank_->pause_effect(this->zoom_out_sound_);
	this->pause_player_sounds();
}

void Level::resume() const
{
	this->sound_bank_->set_effect_volume(this->music_,
		this->music_volume_ * this->preferences_->music_gain());
	this->sound_bank_->set_effect_volume(this->zoom_out_sound_,
		this->tuning_->match().zoom_out_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->music_);
	this->sound_bank_->resume_effect(this->zoom_out_sound_);
	this->resume_player_sounds();
}

void Level::update(const std::vector<PlayerInputData>& player_inputs, float dt)
{
	this->refresh_layout();
	this->frame_dt_ = dt;
	this->update_state(player_inputs, dt);

	// Refresh on every path so views and overlays reflect the completed tick.
	this->refresh_views();
}

void Level::update_state(const std::vector<PlayerInputData>& player_inputs,
	float dt)
{
	if (this->state_ == LevelState::start_countdown ||
		this->start_timer_ > -1.0f)
	{
		if (this->start_timer_ >= this->tuning_->match().countdown)
		{
			this->sound_bank_->play_effect(this->music_, true, this->music_volume_ * this->preferences_->music_gain());
			this->sound_bank_->play_wave(this->countdown_sound_,
				this->tuning_->match().countdown_sound.volume * this->preferences_->sfx_gain());
		}
		this->start_timer_ -= dt;
		if (this->start_timer_ > 0.0f)
		{
			this->set_countdown_text(std::to_wstring(
				static_cast<int>(this->start_timer_) + 1));
		}
		else
		{
			this->set_countdown_text(L"GO!");
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

			// Seed the camera now: the zoom_out arm runs next tick, but refresh_views
			// already renders the newly entered state this frame.
			this->zoom_out_camera_ = this->zoom_out_start_camera_;

			this->sound_bank_->stop_effect(this->music_, true);
			this->stop_player_sounds();
			this->sound_bank_->play_effect(this->zoom_out_sound_, false,
				this->tuning_->match().zoom_out_sound.volume * this->preferences_->sfx_gain());
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

		// refresh_layout refits the endpoints when the window changes size.
		this->zoom_out_camera_ = Camera::calculate_intermediate_camera(
			this->zoom_out_start_camera_, this->zoom_out_finish_camera_,
			this->zoom_out_camera_ratio());
	}
	else if (this->state_ == LevelState::overview)
	{
		this->overview_timer_ -= dt;
		if (this->overview_timer_ <= 0.0f)
		{
			this->state_ = LevelState::finished;
		}
	}
	// GameLevel opens results when it sees finished, so this state needs no update arm.
	// Music stops on entry to zoom_out; destruction also stops level sounds.

}

float Level::zoom_out_camera_ratio() const
{
	// Defensively treat a non-positive duration as a completed zoom, avoiding 0/0.
	// The tuning loader requires a positive duration.
	const float duration = this->tuning_->match().zoom_out;
	if (duration <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - (this->zoom_out_timer_ / duration);
}

void Level::update_level_logic(const std::vector<PlayerInputData>& player_inputs,
	float dt)
{
	// Run the scene phases in order, with game-specific work between them.
	this->apply_player_inputs(player_inputs);

	this->scene_->update(dt);
	this->resolve_projectile_movement();

	this->update_players(dt);

	this->scene_->resolve();

	// Loss of ground support is known only after all contacts have been dispatched.
	for (Player* player : this->players_)
	{
		player->end_contacts();
	}

	this->update_players_after_resolve();

	// Retire deleted/out-of-bounds objects and admit this tick's projectiles.
	// Deferred admission keeps the scene stable during iteration.
	this->scene_->end_tick();

	// Count once per tick for all debug overlays.
	this->projectile_count_ = this->count_projectiles();

	// Players must remain in the arena; unlike projectiles, they cannot be retired.
	for (const Player* player : this->players_)
	{
		if (!this->scene_->in_bounds(*player))
		{
			throw std::runtime_error("Player out of bounds");
		}
	}
}

void Level::resolve_mouse_aim(Player* player, size_t view_index,
	PlayerInputData& input) const
{
	if (!this->mouse_->focused())
	{
		// Without a valid cursor, leave aim unrequested rather than reuse a stale direction.
		return;
	}

	const labrador::Viewport viewport =
		this->viewport_manager_->player_viewport(static_cast<int>(view_index));

	// Convert the player centre to window client pixels: camera transform, then pane offset.
	const Vector2F centre_in_view =
		player->camera().calculate_view_position(player->center());
	const Vector2F centre_on_screen = centre_in_view + viewport.position();

	const Vector2F cursor(static_cast<float>(this->mouse_->state().position.x),
		static_cast<float>(this->mouse_->state().position.y));

	const Vector2F aim = cursor - centre_on_screen;

	// A cursor exactly on the player requests no direction; do not normalize zero.
	if (aim == Vector2F::ZERO)
	{
		return;
	}

	input.shoot_direction_requested = true;
	input.shoot_direction = aim.normalized();
	input.shoot_angle = input.shoot_direction.angle();
}

void Level::apply_player_inputs(
	const std::vector<PlayerInputData>& player_inputs)
{
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		const int slot = player->player_num();

		if (slot >= 0 && static_cast<size_t>(slot) < player_inputs.size())
		{
			PlayerInputData input = player_inputs[slot];

			// Copy the slot input to fill in this player's camera-dependent mouse aim.
			if (input_slots::is_keyboard_and_mouse(slot))
			{
				this->resolve_mouse_aim(player, i, input);
			}

			player->set_player_input(input);
		}
		else
		{
			player->set_player_input(PlayerInputData());
		}
	}
}

void Level::update_players(float dt)
{
	this->refresh_player_cameras();
	for (Player* player : this->players_)
	{
		for (std::unique_ptr<CollisionObject>& projectile :
			player->update_weapon_and_get_projectiles(
				dt, this->projectile_targets_))
		{
			this->scene_->add(std::move(projectile));
		}
	}
}

void Level::resolve_projectile_movement()
{
	// Resolve the first hit along each projectile's travel before endpoint contacts
	// can consume it in scene order or miss a thin paintable face.
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (is_projectile(to_collision_type(object->tag())))
		{
			dynamic_cast<Projectile&>(*object).resolve_movement_contacts(
				this->projectile_targets_);
		}
	}
}

void Level::refresh_player_cameras()
{
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		Camera camera = this->camera_tools_.calculate_camera(
			player->center(),
			this->viewport_manager_->player_viewport(
				static_cast<int>(i)).size(),
			player->camera(),
			this->camera_bounds_);
		player->set_camera(camera);
	}
}

void Level::refresh_layout()
{
	const Vector2F size = this->resolution_manager_->resolution_vec();
	if (size == this->layout_size_)
	{
		return;
	}

	const Viewport screen(RectangleF(Vector2F::ZERO, size));
	this->zoom_out_start_camera_ = Camera::frame(this->zoom_out_start_bounds_, screen);
	this->zoom_out_finish_camera_ = Camera::frame(this->zoom_out_finish_bounds_, screen);
	this->zoom_out_camera_ = Camera::calculate_intermediate_camera(
		this->zoom_out_start_camera_, this->zoom_out_finish_camera_,
		this->zoom_out_camera_ratio());
	this->centre_countdown_text();
	this->centre_controls_text();
	this->refresh_player_cameras();
	this->refresh_views();
	this->layout_size_ = size;
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
		// Pane order follows players_, not input slots, which may be sparse.
		for (size_t i = 0; i < this->players_.size(); ++i)
		{
			const Player* player = this->players_[i];
			this->scene_->add_view(
				this->viewport_manager_->player_viewport(
					static_cast<int>(i)),
				player->camera());
		}
	}
	else
	{
		// Use one fullscreen view without changing the shared split-screen layout.
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

		// Scene has joined its render workers. The last view executes after
		// every player pane; menus above this state append to the same list.
		if (this->state_ == LevelState::start_countdown ||
			this->start_timer_ > -1.0f)
		{
			DrawList list = renderer.view(renderer.view_count() - 1);
			list.set_viewport(this->viewport_manager_->fullscreen_viewport());
			list.set_camera(Camera::DEFAULT_CAMERA);
			this->draw_countdown_text(list);
		}
	}
	else
	{
		// The overview draws only the world, with no gameplay overlays.
		this->scene_->draw(renderer);
	}
}

void Level::draw_player_overlay(int view_index, DrawList& list) const
{
	const Player* player = this->players_[static_cast<size_t>(view_index)];

	const Viewport& player_vp = this->scene_->view(view_index).viewport;

	// Dividers use window coordinates; subtract the pane's screen origin
	// to draw them into this viewport.
	list.set_camera(Camera(player_vp));

	for (const std::unique_ptr<GameObject>& divider : this->viewport_dividers_)
	{
		divider->draw(list);
	}

	// The HUD uses pane-local coordinates, so draw it with the identity camera.
	list.set_camera(Camera::DEFAULT_CAMERA);

	this->interface_gameplay_->draw_gameplay_interface(list,
		player_vp.size(),
		view_index,
		player->player_num(),
		player->team(),
		player->health() / this->tuning_->player().starting_health,
		player->weapon_ammo() / this->tuning_->starting_ammo(),
		this->timer_,
		player->team_colour(),
		player->respawn_timer(),
		player->state() == PlayerState::dead);

	if (player->showing_debug())
	{
		this->debug_text_->draw_debug_info(list, player,
			this->projectile_count_, this->frame_dt_);
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

	// Derive team participation from the players actually in the match.
	result.team_a_played = false;
	result.team_b_played = false;
	for (const Player* player : this->players_)
	{
		switch (player->team())
		{
		case PlayerTeam::a:
			result.team_a_played = true;
			break;
		case PlayerTeam::b:
			result.team_b_played = true;
			break;
		default:
			break;
		}
	}

	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (to_collision_type(object->tag()) ==
			CollisionObjectType::structure_paintable)
		{
			// A failed pointer cast returns nullptr.
			auto paintable_object = dynamic_cast<PaintableObject*>(object);

			if (paintable_object == nullptr)
			{
				throw std::runtime_error(
					"Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
			}

			PaintTotal paint = paintable_object->paint_total();

			result.team_a_area += paint.team_a;
			result.team_b_area += paint.team_b;
			result.total_area += paint.total;
			for (size_t slot = 0; slot < result.player_areas.size(); ++slot)
			{
				result.player_areas[slot] += paint.player_areas[slot];
			}
		}
	}

	return result;
}

void Level::draw_countdown_text(DrawList& draw_list) const
{
	// Use linear filtering for countdown text, then restore point filtering.
	draw_list.set_filter(TextureFilter::linear);
	this->countdown_text_->draw(draw_list);

	// Show the shared controls reminder only during the countdown.
	this->controls_text_->draw(draw_list);

	draw_list.set_filter(TextureFilter::point);
}
