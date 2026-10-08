#include "game/pch.h"
#include "game/objects/level.h"
#include "game/debug/debug_overlay_draw.h"
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
	this->presentation_ = presentation;
	this->render_resources_ = render_resources;
	this->overlay_sheet_ = render_resources->resolve_sprite_sheet(
		presentation->hud().sprite_sheet);
	this->overlay_fill_ = render_resources->sprite_sheet(this->overlay_sheet_)
		->resolve_sprite_frame(presentation->hud().fill_frame);
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
	const MenuStyle& menu = this->presentation_->menu();
	this->countdown_heading_ = std::make_unique<TextDropShadow>(
		L"GET READY", menu.detail_font, Vector2F::ZERO,
		render_resources, menu.highlight, menu.shadow_colour,
		menu.detail_shadow_offset);
	this->centre_countdown_text();

	// Keep these reminders synchronized with PlayerInput bindings, showing only
	// the devices in this match. They are shared by all panes, not repeated per player.
	bool anyone_on_keyboard = false;
	bool anyone_on_gamepad = false;
	for (const Player* player : this->players_)
	{
		if (input_slots::is_keyboard_and_mouse(player->player_num()))
		{
			anyone_on_keyboard = true;
		}
		else
		{
			anyone_on_gamepad = true;
		}
	}

	const auto add_reminder = [this, render_resources, &match, &menu](
		const std::wstring& heading, const std::wstring& text)
	{
		ControlsReminder reminder;
		reminder.heading = std::make_unique<TextDropShadow>(
			heading, menu.detail_font, Vector2F::ZERO, render_resources,
			menu.highlight, menu.shadow_colour, menu.detail_shadow_offset);
		reminder.text = std::make_unique<TextDropShadow>(
			text, match.controls_font, Vector2F::ZERO, render_resources,
			match.controls_colour, match.controls_shadow_colour,
			match.controls_shadow_offset);
		this->controls_reminders_.push_back(std::move(reminder));
	};

	if (anyone_on_gamepad)
	{
		add_reminder(L"GAMEPAD",
			L"Left stick  Move    |    Stick down  Drop through\n"
			L"A / LT  Jump    |    RT  Shoot\n"
			L"Right stick  Aim    |    Start  Pause");
	}
	if (anyone_on_keyboard)
	{
		add_reminder(L"KEYBOARD + MOUSE",
			L"A / D  Move    |    S  Drop through\n"
			L"Space  Jump    |    Left click  Shoot\n"
			L"Mouse  Aim    |    Esc  Pause");
	}
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
	const MatchPresentation& look = this->presentation_->match();
	const MenuStyle& menu = this->presentation_->menu();
	const float scale = this->opening_scale();
	const Vector2F card_size = look.countdown_size * scale;
	this->countdown_card_ = RectangleF(
		Vector2F((resolution.x - card_size.x) / 2.0f,
			resolution.y * look.countdown_center_y - card_size.y / 2.0f),
		card_size);
	const Vector2F padding = look.controls_padding * scale;
	const RectangleF heading_bounds = RectangleF::union_of(
		this->countdown_heading_->text_bounds_at(Vector2F::ZERO, 1.0f),
		this->countdown_heading_->text_bounds_at(menu.detail_shadow_offset, 1.0f));
	const float heading_scale = std::min(scale,
		(card_size.x - padding.x * 2.0f) / heading_bounds.width);
	this->countdown_heading_->set_scale(heading_scale);
	this->countdown_heading_->set_shadow_scale(heading_scale);
	this->countdown_heading_->set_shadow_offset(menu.detail_shadow_offset * heading_scale);
	this->countdown_heading_->set_position(Vector2F(
		(resolution.x - heading_bounds.width * heading_scale) / 2.0f,
		this->countdown_card_.y + padding.y) - heading_bounds.position() * heading_scale);
	const float number_top = this->countdown_card_.y + padding.y * 2.0f +
		heading_bounds.height * heading_scale;
	const RectangleF number_box(
		this->countdown_card_.x + padding.x, number_top,
		card_size.x - padding.x * 2.0f,
		this->countdown_card_.bottom() - padding.y - number_top);
	// Fit text and shadow together, including negative offsets. Start from authored
	// scale each time so a larger window or narrower beat restores the intended size.
	const RectangleF bounds = RectangleF::union_of(
		this->countdown_text_->text_bounds_at(Vector2F::ZERO,
			match.countdown_scale),
		this->countdown_text_->text_bounds_at(match.countdown_shadow_offset,
			match.countdown_scale));
	const float fit = std::min({ scale,
		number_box.width / bounds.width, number_box.height / bounds.height });

	this->countdown_text_->set_scale(match.countdown_scale * fit);
	this->countdown_text_->set_shadow_scale(match.countdown_scale * fit);
	this->countdown_text_->set_shadow_offset(match.countdown_shadow_offset * fit);
	this->countdown_text_->set_position(
		number_box.position() + (number_box.size() - bounds.size() * fit) / 2.0f -
		bounds.position() * fit);
}

void Level::centre_controls_text()
{
	if (this->controls_reminders_.empty())
	{
		return;
	}
	// Fit a pair of equally sized cards from authored measurements on every resize.
	// All positions stay in fullscreen pixel space, independent of split-screen panes.
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const MatchTuning& match = this->tuning_->match();
	const MatchPresentation& look = this->presentation_->match();
	const MenuStyle& menu = this->presentation_->menu();
	const float scale = this->opening_scale();
	const Vector2F margin = look.controls_margin * scale;
	const Vector2F padding = look.controls_padding * scale;
	const float gap = look.controls_gap * scale;
	const float count = static_cast<float>(this->controls_reminders_.size());
	float content_width = 0.0f;
	float heading_height = 0.0f;
	float text_height = 0.0f;
	for (const ControlsReminder& reminder : this->controls_reminders_)
	{
		const RectangleF heading = RectangleF::union_of(
			reminder.heading->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.heading->text_bounds_at(menu.detail_shadow_offset, 1.0f));
		const RectangleF text = RectangleF::union_of(
			reminder.text->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.text->text_bounds_at(match.controls_shadow_offset, 1.0f));
		content_width = std::max({ content_width, heading.width, text.width });
		heading_height = std::max(heading_height, heading.height);
		text_height = std::max(text_height, text.height);
	}
	const float column_width = (resolution.x - margin.x * 2.0f - gap * (count - 1.0f)) / count;
	const float text_scale = std::min(scale,
		(column_width - padding.x * 2.0f) / content_width);
	const Vector2F card_size(content_width * text_scale + padding.x * 2.0f,
		(heading_height + text_height) * text_scale + padding.y * 3.0f);
	const float total_width = card_size.x * count + gap * (count - 1.0f);
	float x = (resolution.x - total_width) / 2.0f;
	for (ControlsReminder& reminder : this->controls_reminders_)
	{
		reminder.card = RectangleF(Vector2F(x,
			std::min(resolution.y * match.controls_y,
				resolution.y - margin.y - card_size.y)), card_size);
		const RectangleF heading = RectangleF::union_of(
			reminder.heading->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.heading->text_bounds_at(menu.detail_shadow_offset, 1.0f));
		const RectangleF text = RectangleF::union_of(
			reminder.text->text_bounds_at(Vector2F::ZERO, 1.0f),
			reminder.text->text_bounds_at(match.controls_shadow_offset, 1.0f));
		reminder.heading->set_scale(text_scale);
		reminder.heading->set_shadow_scale(text_scale);
		reminder.heading->set_shadow_offset(menu.detail_shadow_offset * text_scale);
		reminder.heading->set_position(reminder.card.position() + padding -
			heading.position() * text_scale);
		reminder.text->set_scale(text_scale);
		reminder.text->set_shadow_scale(text_scale);
		reminder.text->set_shadow_offset(match.controls_shadow_offset * text_scale);
		reminder.text->set_position(reminder.card.position() +
			Vector2F(padding.x, padding.y * 2.0f + heading_height * text_scale) -
			text.position() * text_scale);
		x += card_size.x + gap;
	}
}

float Level::opening_scale() const
{
	const Vector2F resolution = this->resolution_manager_->resolution_vec();
	const Vector2F reference = this->presentation_->match().reference_size;
	return std::min(resolution.x / reference.x, resolution.y / reference.y);
}

void Level::suspend() const
{
	for (Player* player : this->players_)
	{
		player->reset_jump_input();
	}
	this->pause_audio();
}

void Level::pause_audio() const
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

void Level::update(const std::vector<PlayerInputData>& player_inputs, float dt,
	SeatInputFilter* seats)
{
	this->refresh_layout();
	this->frame_dt_ = dt;
	this->update_state(player_inputs, dt, seats);

	// Refresh on every path so views and overlays reflect the completed tick.
	this->refresh_views();
}

void Level::update_state(const std::vector<PlayerInputData>& player_inputs,
	float dt, SeatInputFilter* seats)
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
		this->update_level_logic(player_inputs, dt, seats);

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
	float dt, SeatInputFilter* seats)
{
	// Run the scene phases in order, with game-specific work between them.
	this->apply_player_inputs(player_inputs, seats);

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

	const Vector2F cursor(static_cast<float>(this->mouse_->state().position.x),
		static_cast<float>(this->mouse_->state().position.y));
	const Vector2F aim = PlayerCamera::mouse_direction(
		player->center(), cursor, viewport, player->camera());

	// A cursor exactly on the player requests no direction; do not normalize zero.
	if (aim == Vector2F::ZERO)
	{
		return;
	}

	input.shoot_direction_requested = true;
	input.shoot_direction = aim;
	input.shoot_angle = input.shoot_direction.angle();
}

void Level::apply_player_inputs(
	const std::vector<PlayerInputData>& player_inputs, SeatInputFilter* seats)
{
	// One input per view; an idle dummy (or an unknown slot) gets neutral input.
	std::vector<PlayerInputData> by_view(this->players_.size());
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		const int slot = player->player_num();

		if (slot >= 0 && static_cast<size_t>(slot) < player_inputs.size())
		{
			by_view[i] = player_inputs[slot];

			// Copy the slot input to fill in this player's camera-dependent mouse aim.
			if (input_slots::is_keyboard_and_mouse(slot))
			{
				this->resolve_mouse_aim(player, i, by_view[i]);
			}
		}
	}

	if (seats != nullptr)
	{
		seats->filter_seats(by_view, *this);
	}

	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		this->players_[i]->set_player_input(by_view[i]);
	}
}

void Level::update_players(float dt)
{
	this->refresh_player_cameras();
	for (Player* player : this->players_)
	{
		for (std::unique_ptr<Projectile>& projectile :
			player->update_weapon_and_get_projectiles(
				dt, this->projectile_targets_))
		{
			// The game keeps the projectile interface until Scene takes ownership
			// through its engine collision interface.
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
		const Camera camera = this->player_camera_.follow(
			player->center(),
			this->viewport_manager_->player_viewport(
				static_cast<int>(i)),
			this->resolution_manager_->resolution_vec().y,
			this->presentation_->camera(),
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
				Viewport(this->viewport_manager_->player_viewport(
					static_cast<int>(i)).pixel_rect()),
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

	// Developer overlays, in world space under the HUD. Never on in Release.
	if (this->overlays_.any())
	{
		debug_overlay::draw_pane(list, this->overlays_, *this->scene_,
			this->scene_->view(view_index), *player, this->camera_bounds_,
			*this->render_resources_->sprite_sheet(this->overlay_sheet_), this->overlay_fill_);
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

void Level::set_overlays(const LevelOverlays& overlays)
{
	this->overlays_ = overlays;
}

void Level::set_players_showing_debug(bool showing)
{
	for (Player* player : this->players_)
	{
		player->set_showing_debug(showing);
	}
}

void Level::set_player_cheats(bool damage_immune, bool infinite_ammo)
{
	for (Player* player : this->players_)
	{
		player->set_damage_immune(damage_immune);
		player->set_infinite_ammo(infinite_ammo);
	}
}

bool Level::place_player(int view, const Vector2F& top_left)
{
	if (view < 0 || view >= this->player_count())
	{
		return false;
	}
	Player* player = this->players_[static_cast<size_t>(view)];
	if (player->state() == PlayerState::dead)
	{
		player->force_respawn();
	}
	player->place_at(top_left);
	player->update_weapon_position();
	this->refresh_player_cameras();
	this->refresh_views();
	return true;
}

bool Level::respawn_player(int view)
{
	if (view < 0 || view >= this->player_count())
	{
		return false;
	}
	Player* player = this->players_[static_cast<size_t>(view)];
	player->force_respawn();
	player->update_weapon_position();
	this->refresh_player_cameras();
	this->refresh_views();
	return true;
}

int Level::player_count() const
{
	return static_cast<int>(this->players_.size());
}

const Player& Level::player(int view) const
{
	return *this->players_.at(static_cast<size_t>(view));
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
	const bool getting_ready = this->state_ == LevelState::start_countdown;
	if (getting_ready)
	{
		const float scale = this->opening_scale();
		draw_list.set_filter(TextureFilter::point);
		this->draw_opening_card(draw_list, this->countdown_card_, scale);
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{
			this->draw_opening_card(draw_list, reminder.card, scale);
		}
	}
	// Use linear filtering for countdown text, then restore point filtering.
	draw_list.set_filter(TextureFilter::linear);
	this->countdown_text_->draw(draw_list);

	// Clear the cards when movement begins. GO! keeps its existing one-second beat.
	if (getting_ready)
	{
		this->countdown_heading_->draw(draw_list);
		for (const ControlsReminder& reminder : this->controls_reminders_)
		{
			reminder.heading->draw(draw_list);
			reminder.text->draw(draw_list);
		}
	}

	draw_list.set_filter(TextureFilter::point);
}

void Level::draw_opening_card(DrawList& draw_list,
	const RectangleF& rectangle, float scale) const
{
	const MenuStyle& menu = this->presentation_->menu();
	const SpriteSheet* sheet = this->render_resources_->sprite_sheet(this->overlay_sheet_);
	sheet->draw(draw_list, this->overlay_fill_,
		RectangleF(rectangle.position() + menu.panel_shadow_offset * scale, rectangle.size()),
		menu.scrim_colour);
	sheet->draw(draw_list, this->overlay_fill_, rectangle, menu.panel_edge_colour);
	const float border = menu.rule_height * scale;
	sheet->draw(draw_list, this->overlay_fill_, RectangleF(
		rectangle.x + border, rectangle.y + border,
		rectangle.width - border * 2.0f, rectangle.height - border * 2.0f),
		menu.focus_panel_colour);
	sheet->draw(draw_list, this->overlay_fill_, RectangleF(
		rectangle.x + border, rectangle.y + border,
		rectangle.width - border * 2.0f, menu.accent_width * scale), menu.highlight);
}
