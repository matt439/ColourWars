#include "game/pch.h"
#include "game/objects/level.h"
#include "game/input_slots.h"
#include "game/objects/structure.h"
#include "game/objects/translating_structure_paintable.h"
#include "game/objects/team_gate_structure_paintable.h"
#include "game/simulation.h"
#include "game/ai/match_bot_driver.h"
#include "engine/math/intersects.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <stdexcept>

using namespace mattmath;
using namespace labrador;


namespace
{
    bool passable_gate(const CollisionObject* object, PlayerTeam team)
    {
        if (team == PlayerTeam::none) { return false; }
        const Structure* structure = Collider::of(*object).structure();
        return structure != nullptr && structure->passable_for(team);
    }
}

Level::Level(std::unique_ptr<Scene> scene,
	std::vector<Player*> players,
	std::vector<std::unique_ptr<GameObject>> viewport_dividers,
	const TeamColour& team_colours,
	const RectangleF& camera_bounds,
	const RectangleF& zoom_out_start_bounds,
	const RectangleF& zoom_out_finish_bounds,
	const GameTuning* tuning,
	const Presentation* presentation,
	const std::wstring& stage_name,
	const std::string& sound_bank_name,
	const std::string& music_name,
	float music_volume,
	const std::string& end_sting_name,
	float end_sting_volume,
	const ResolutionManager* resolution_manager,
	const ViewportManager* viewport_manager,
	RenderResources* render_resources,
	const AudioResources* audio_resources,
	const PlaySettings* preferences,
	const labrador::Mouse* mouse,
	std::vector<LevelStation> stations,
	OpeningMode opening, std::optional<float> duration, const InputBindings* input_bindings,
	const MatchSetup* roster, std::uint64_t bot_seed, std::vector<LevelLesson> lessons) :
	scene_(std::move(scene)),
	players_(std::move(players)),
	stations_(std::move(stations)),
	viewport_dividers_(std::move(viewport_dividers)),
	music_volume_(music_volume),
	end_sting_volume_(end_sting_volume),
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
	this->input_bindings_ = input_bindings;
	this->presentation_ = presentation;
	this->render_resources_ = render_resources;
	this->roster_.resize(this->players_.size());
	if (roster != nullptr)
	{
		if (roster->player_settings().size() != this->players_.size())
		{ throw std::invalid_argument("Level roster must match its participants"); }
		this->roster_ = roster->player_settings();
	}
	this->participant_views_.assign(this->players_.size(), -1);
	this->bot_identity_texts_.resize(this->players_.size());
	for (std::size_t participant = 0; participant < this->players_.size(); ++participant)
	{
		if (this->roster_[participant].controller == PlayerController::human)
		{
			this->participant_views_[participant] = static_cast<int>(this->view_participants_.size());
			this->view_participants_.push_back(static_cast<int>(participant));
		}
		else
		{
			this->bot_identity_texts_[participant] = std::make_unique<Text>(
				presentation->menu_text().bot_roster_label(static_cast<int>(participant),
					this->roster_[participant].difficulty), presentation->hud().player_identity_font,
				Vector2F::ZERO, render_resources, presentation->player_identity().ink_colour);
		}
	}
	if (this->view_participants_.empty())
	{ throw std::invalid_argument("Level requires at least one human view"); }
	if (roster != nullptr && roster->bot_count() != 0)
	{ this->bot_driver_ = std::make_unique<MatchBotDriver>(tuning, *roster, bot_seed); }
	const MenuStyle& menu = presentation->menu();
	const CountdownArtPresentation& paper = presentation->match().countdown_art;
	this->stage_entry_ = std::make_unique<colourwars::TextDropShadow>(stage_name,
		menu.heading_font, Vector2F::ZERO, render_resources,
		paper.ink, paper.paper, menu.heading_shadow_offset);
	this->player_badge_ = std::make_unique<PlayerBadge>(
		&presentation->player_identity(), render_resources);
	this->player_identity_remaining_.assign(this->players_.size(),
		presentation->player_identity().emphasis_duration);
	this->teammate_marker_ = std::make_unique<TeammateMarker>(
		&presentation->hud().teammate_marker, render_resources);
	this->teammate_marker_age_.assign(this->view_participants_.size() * this->players_.size(), 0.0f);
	this->paint_audio_ = std::make_unique<PaintAudio>(&tuning->paint(), audio_resources);
	this->blocked_shot_feedback_ = std::make_unique<BlockedShotFeedback>(
		&presentation->blocked_shot(), render_resources);
	for (Player* player : this->players_)
	{
		player->set_paint_audio(this->paint_audio_.get());
		player->set_blocked_shot_feedback(this->blocked_shot_feedback_.get());
		IdleDrip* drip = nullptr;
		const WeaponType type = player->primary_weapon().type();
		if (type == WeaponType::sprayer || type == WeaponType::sniper || type == WeaponType::bucket)
		{
			drip = this->scene_->add(std::make_unique<IdleDrip>(
				&presentation->idle_drip(type), render_resources, player->team_colour(),
				static_cast<uint32_t>(this->idle_drips_.size()), preferences));
		}
		this->idle_drips_.push_back(drip);
	}
	this->overlay_sheet_ = render_resources->resolve_sprite_sheet(
		presentation->hud().sprite_sheet);
	this->overlay_fill_ = render_resources->sprite_sheet(this->overlay_sheet_)
		->resolve_sprite_frame(presentation->hud().fill_frame);
	const MatchTuning& match = tuning->match();

	this->timer_ = duration.value_or(match.duration);
	this->start_timer_ = match.countdown;
	this->zoom_out_timer_ = match.zoom_out;
	this->overview_timer_ = match.overview;

	this->debug_text_ = std::make_unique<DebugText>(
		render_resources, resolution_manager,
		&presentation->debug_overlay());
	this->interface_gameplay_ = std::make_unique<InterfaceGameplay>(
		render_resources, &presentation->hud(), &presentation->player_identity());
	this->refresh_identity_texts();
	this->sound_bank_ = audio_resources->sound_bank(sound_bank_name);

	// A silent stage leaves music_ unresolved; every music call checks it.
	if (!music_name.empty())
	{
		this->music_ = this->sound_bank_->resolve_effect(music_name);
	}
	if (!end_sting_name.empty())
	{
		this->end_sting_ = this->sound_bank_->resolve_effect(end_sting_name);
	}
	this->zoom_out_sound_ =
		this->sound_bank_->resolve_effect(match.zoom_out_sound.name);
	this->countdown_sound_ =
		this->sound_bank_->resolve_effect(match.countdown_sound.name);
	this->go_sound_ = this->sound_bank_->resolve_effect(match.go_sound.name);
	this->time_up_sound_ = this->sound_bank_->resolve_effect(match.time_up_sound.name);
	this->last_ten_sound_ = this->sound_bank_->resolve_effect(match.last_ten_sound.name);
	this->room_tone_sound_ = this->sound_bank_->resolve_effect(match.room_tone_sound.name);
	this->sigh_sound_ = this->sound_bank_->resolve_effect(match.sigh_sound.name);
	this->chair_sound_ = this->sound_bank_->resolve_effect(match.chair_sound.name);
	this->glass_in_sound_ = this->sound_bank_->resolve_effect(match.glass_in_sound.name);
	this->celebrate_sound_ = this->sound_bank_->resolve_effect(match.celebrate_sound.name);
	this->slump_sound_ = this->sound_bank_->resolve_effect(match.slump_sound.name);
	this->colour_in_sound_ = this->sound_bank_->resolve_effect(match.colour_in_sound.name);
	this->skip_sound_ = this->sound_bank_->resolve_effect(match.skip_sound.name);
	this->undo_sound_ = this->sound_bank_->resolve_effect(match.undo_sound.name);

	if (opening != OpeningMode::off)
	{
		const OpeningPresentation& look = presentation->opening();
		this->monitor_ = std::make_unique<Monitor>(&look, render_resources,
			MonitorPersona::roll(look));
		this->silhouette_ = std::make_unique<Silhouette>(&look, render_resources);
		this->cursor_ = std::make_unique<ScreenCursor>(&look, render_resources);
		this->acting_ = std::make_unique<FigureActing>(&look, &presentation->respawn_redraw(),
			render_resources);
		this->reveal_text_ = presentation->menu_text().results;
		this->reveal_art_ = std::make_unique<RevealArt>(&look, render_resources);
		this->hover_sound_ = this->sound_bank_->resolve_wave(match.hover_sound.name);
		this->click_sound_ = this->sound_bank_->resolve_wave(match.click_sound.name);
		this->close_sound_ = this->sound_bank_->resolve_wave(match.close_sound.name);
		this->state_ = LevelState::opening;
		// A quick rematch starts with the colours being tried.
		this->opening_time_ = opening == OpeningMode::quick ? match.opening.settle : 0.0f;
		this->colour_players(false, false);
	}

	this->lessons_ = std::move(lessons);
	this->build_lesson_cards(render_resources);
	this->build_opening_frame(render_resources);
}

void Level::update(const std::vector<PlayerInputData>& player_inputs, float dt,
    SeatInputFilter* seats)
{
    // All dynamic validation/allocation precedes layout, clocks and input/RNG.
    std::optional<HorizontalFrame> horizontal;
    if (this->has_horizontal_motion())
    {
        HorizontalMotion::validate_dt(dt);
        if (dt > 0.0f) { horizontal.emplace(this->prepare_horizontal_frame(dt)); }
        this->publish_horizontal_zero();
    }
	this->refresh_layout();
	this->simulation_dt_ = dt;
	if (dt > 0.0f)
	{
		this->update_state(player_inputs, dt, seats, horizontal ? &*horizontal : nullptr);
		this->update_acting(dt);
	}

	// Refresh on every path so views and overlays reflect the completed tick.
	this->refresh_views();
}

std::vector<SlotRumbleEvents> Level::take_rumble_events()
{
	std::vector<SlotRumbleEvents> result;
	// Complete allocation before consuming any player's pending flags.
	result.reserve(this->players_.size());
	for (Player* player : this->players_)
	{
		result.push_back({ player->player_num(), player->take_rumble_events() });
	}
	return result;
}

void Level::clear_rumble_events() noexcept
{
	for (Player* player : this->players_) { player->clear_rumble_events(); }
}

void Level::update_state(const std::vector<PlayerInputData>& player_inputs,
	float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal)
{
	if (this->state_ == LevelState::opening)
	{
		this->update_opening(player_inputs, dt);
		return;
	}
	if (this->intro_countdown_ && this->skip_requested(player_inputs))
	{
		this->skip_intro();
	}
	if (this->state_ == LevelState::start_countdown ||
		(this->state_ == LevelState::active && this->start_timer_ > -1.0f))
	{
		const bool starting = !this->countdown_started_;
		if (starting)
		{
			this->countdown_started_ = true;
			if (this->music_.valid())
			{
				this->music_fade_.fade_to(1.0f, this->presentation_->music_mix().fade_in);
				this->sound_bank_->play_effect(this->music_, true, 0.0f);
			}
		}
		const float before = this->start_timer_;
		this->start_timer_ -= dt;
		// Only the opening countdown pushes through the monitor. A long tick
		// crossing its start still owns one transition cue, never a backlog.
		if (this->intro_countdown_ && !this->glass_in_started_ &&
			this->start_timer_ <= this->tuning_->match().opening.push)
		{
			this->glass_in_started_ = true;
			this->set_room_tone(false);
			this->sound_bank_->play_effect(this->glass_in_sound_, false,
				this->tuning_->match().glass_in_sound.volume);
		}
		// One cue for the displayed number, including the initial beat. A long
		// tick never queues the missed beats over GO or live play.
		if (this->start_timer_ > 0.0f &&
			(starting ||
				std::ceil(before) != std::ceil(this->start_timer_)))
		{
			this->sound_bank_->play_effect(this->countdown_sound_, false,
				this->tuning_->match().countdown_sound.volume);
		}
		if (this->start_timer_ > 0.0f)
		{
			this->set_countdown_text(std::to_wstring(
				static_cast<int>(std::ceil(this->start_timer_))));
		}
		else
		{
			this->set_countdown_text(L"GO!");
		}

		if (this->start_timer_ <= 0.0f)
		{
			if (this->state_ == LevelState::start_countdown)
			{
				this->sound_bank_->stop_effect(this->countdown_sound_, true);
				this->sound_bank_->play_effect(this->go_sound_, false,
					this->tuning_->match().go_sound.volume);
			}
			this->state_ = LevelState::active;
			if (before > 0.0f || starting) { this->emphasise_players(); }
			this->intro_countdown_ = false;
		}
		else
		{
			return;
		}
	}
	if (this->state_ == LevelState::active)
	{
		this->update_level_logic(player_inputs, dt, seats, horizontal);
		this->update_active_lesson();
		// Practice has no clock to run out; the player leaves from pause.
		if (this->practice()) { return; }

		this->timer_ -= dt;
		// Sound only the current displayed second, never missed seconds after a
		// long update. Short matches also get their first positive clock cue.
		const float second = std::ceil(this->timer_);
		if (this->timer_ > 0.0f && second <= 10.0f && second != this->last_ten_second_)
		{
			this->last_ten_second_ = second;
			this->sound_bank_->play_effect(this->last_ten_sound_, false,
				this->tuning_->match().last_ten_sound.volume);
		}
		if (this->timer_ <= 0.0f)
		{
			this->state_ = LevelState::zoom_out;
			this->blocked_shot_feedback_->clear();

			// Seed the camera now: the zoom_out arm runs next tick, but refresh_views
			// already renders the newly entered state this frame.
			this->zoom_out_camera_ = this->zoom_out_start_camera_;

			if (this->music_.valid())
			{
				this->music_ending_ = true;
				this->music_fade_.fade_to(0.0f, this->presentation_->music_mix().fade_out);
			}
			if (this->end_sting_.valid())
			{
				this->sound_bank_->play_effect(this->end_sting_, false,
					this->end_sting_volume_);
			}
			this->stop_player_sounds();
			for (Player* player : this->players_)
			{
				player->reset_tool_motion();
			}
			for (IdleDrip* drip : this->idle_drips_)
			{
				if (drip != nullptr) { drip->clear(); }
			}
			this->sound_bank_->stop_effect(this->countdown_sound_, true);
			this->sound_bank_->stop_effect(this->go_sound_, true);
			this->sound_bank_->stop_effect(this->last_ten_sound_, true);
			this->sound_bank_->play_effect(this->time_up_sound_, false,
				this->tuning_->match().time_up_sound.volume);
			this->sound_bank_->play_effect(this->zoom_out_sound_, false,
				this->tuning_->match().zoom_out_sound.volume);
			this->start_reveal();
		}
	}
	else if (this->state_ == LevelState::zoom_out)
	{
		this->zoom_out_timer_ -= dt;

		if (this->zoom_out_timer_ <= 0.0f)
		{
			this->state_ = LevelState::overview;
			this->zoom_out_timer_ = 0.0f;
			// Even a shortened glass transition leaves the returning person silent.
			if (this->music_.valid()) { this->sound_bank_->stop_effect(this->music_, true); }
			if (this->end_sting_.valid()) { this->sound_bank_->stop_effect(this->end_sting_, true); }
			// Let the glass cue's tail finish naturally; destruction still owns it.
		}

		// refresh_layout refits the endpoints when the window changes size.
		this->zoom_out_camera_ = Camera::calculate_intermediate_camera(
			this->zoom_out_start_camera_, this->zoom_out_finish_camera_,
			this->zoom_out_camera_ratio());
	}
	else if (this->state_ == LevelState::overview && this->monitor_ != nullptr)
	{
		this->update_reveal(dt);
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
	// Arena music fades through glass-out; overview and destruction stop it.

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
	float dt, SeatInputFilter* seats, const HorizontalFrame* horizontal)
{
	std::array<bool, 4> was_dead{};
	for (size_t participant = 0; participant < this->players_.size(); ++participant)
	{
		was_dead[participant] = this->players_[participant]->state() == PlayerState::dead;
	}
	this->update_player_identity(dt);
	// Run the scene phases in order, with game-specific work between them.
	this->paint_audio_->update(dt);
	this->blocked_shot_feedback_->update(dt);
	this->apply_player_inputs(player_inputs, seats);
    if (horizontal != nullptr) { this->commit_horizontal_frame(*horizontal); }
	// Before any of this step's movement or paint, so a gate follows the
	// paint held before the step and its overlap test sees where players are.
	this->settle_team_gates();

	this->scene_->update(dt);
	this->resolve_projectile_movement();

	this->update_players(dt);

	this->scene_->resolve();
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (Projectile* projectile = Collider::of(*object).projectile())
		{
			projectile->flush_paint_audio(*this->paint_audio_);
			if (const auto contact = projectile->take_blocked_contact())
			{
				this->blocked_shot_feedback_->emit(*contact);
			}
		}
	}

	// Loss of ground support is known only after all contacts have been dispatched.
	for (Player* player : this->players_)
	{
        if (this->has_horizontal_motion())
        { player->end_contacts(this->scene_->contacts(), this->horizontal_supports_); }
        else { player->end_contacts(this->scene_->contacts()); }
	}

	this->update_players_after_resolve();
	for (size_t participant = 0; participant < this->players_.size(); ++participant)
	{
		if (was_dead[participant] && this->players_[participant]->state() == PlayerState::alive)
		{
			this->player_identity_remaining_[participant] =
				this->presentation_->player_identity().emphasis_duration;
		}
	}
	this->update_idle_drips(dt);

	// Retire deleted/out-of-bounds objects and admit this tick's projectiles.
	// Deferred admission keeps the scene stable during iteration.
	this->scene_->end_tick();

	// Count once per tick for all debug overlays.
	this->projectile_count_ = this->count_projectiles();
	this->update_teammate_markers(dt);

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
	// One input per participant; an idle human dummy gets neutral input.
	std::vector<PlayerInputData> by_participant(this->players_.size());
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		const int slot = player->player_num();

		if (slot >= 0 && static_cast<size_t>(slot) < player_inputs.size())
		{
			by_participant[i] = player_inputs[slot];

			// Copy the slot input to fill in this player's camera-dependent mouse aim.
			if (input_slots::is_keyboard_and_mouse(slot))
			{
				this->resolve_mouse_aim(player,
					static_cast<size_t>(this->view_for_participant(static_cast<int>(i))), by_participant[i]);
			}
		}
	}

	if (this->bot_driver_ != nullptr && (seats == nullptr || !seats->replaces_bot_inputs()))
	{
		this->bot_driver_->filter_seats(by_participant, *this);
	}
	if (seats != nullptr)
	{
		seats->filter_seats(by_participant, *this);
	}

	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		this->players_[i]->set_player_input(by_participant[i]);
	}
}

const IdleDrip* Level::idle_drip(int participant) const
{
	return this->idle_drips_.at(static_cast<size_t>(participant));
}

void Level::update_idle_drips(float dt)
{
	for (size_t index = 0; index < this->players_.size(); ++index)
	{
		if (IdleDrip* drip = this->idle_drips_[index])
		{
			const Player& player = *this->players_[index];
			const Weapon& weapon = player.primary_weapon();
			const PlayerMoveState move = player.move_state();
			const bool supported = move == PlayerMoveState::on_ground ||
				move == PlayerMoveState::on_drop_down_ground ||
				move == PlayerMoveState::on_ramp_left || move == PlayerMoveState::on_ramp_right;
			const bool settled = player.state() == PlayerState::alive && player.health() > 0.0f &&
				supported && player.velocity().x == 0.0f && player.velocity().y == 0.0f &&
				player.input().x_movement == 0.0f && !player.input().primary_shoot &&
				!player.input().jump_held && !player.input().jump_pressed &&
				player.animation_state() == PlayerAnimationState::idle;
			drip->advance(settled, weapon.nozzle_pos(), weapon.rotation(), dt,
				this->projectile_targets_);
		}
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
		if (Projectile* projectile = Collider::of(*object).projectile())
		{
			projectile->resolve_movement_contacts(this->projectile_targets_);
		}
	}
}

void Level::refresh_player_cameras()
{
	for (size_t i = 0; i < this->players_.size(); ++i)
	{
		Player* player = this->players_[i];
		const int view = this->participant_views_[i];
		if (view < 0) { continue; }
		// Fatal contacts can leave state alive until the next Player update.
		// Dead players retain velocity while camera_focus holds then visits spawn.
		const float velocity = player->state() == PlayerState::alive && player->health() > 0.0f
			? player->velocity().x : 0.0f;
		const Camera camera = this->player_camera_.follow(
			player->camera_focus(),
			this->viewport_manager_->player_viewport(
				view),
			this->resolution_manager_->resolution_vec().y,
			this->presentation_->camera(),
			player->camera(),
			this->camera_bounds_, velocity);
		player->set_camera(camera);
	}
}

void Level::set_debug_frame_time(double frame_ms)
{
	this->debug_frame_ms_ = frame_ms;
}

void Level::refresh_layout()
{
	this->refresh_station_labels();
	this->refresh_keyboard_controls();
	const Vector2F size = this->resolution_manager_->resolution_vec();
	if (size == this->layout_size_)
	{
		// Preferences can change while pause covers the same-sized match.
		this->refresh_views();
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
	this->centre_lesson_cards();
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

Camera Level::view_camera(int participant) const
{
	const Player& player = *this->players_.at(static_cast<size_t>(participant));
	const int pane = this->view_for_participant(participant);
	if (this->state_ != LevelState::active || !this->preferences_->view_shake_enabled(player.player_num()) ||
		pane < 0 || player.view_shake().offset() == Vector2F::ZERO) { return player.camera(); }
	return PlayerCamera::displaced(player.camera(),
		this->viewport_manager_->player_viewport(pane), player.view_shake().offset(),
		this->camera_bounds_);
}

void Level::refresh_views()
{
	// Match draw()'s paths, including the intro countdown and glass return.
	// A small active pane still uses ordinary terrain. All mode writes happen
	// before drawing; Scene's workers only read each shared structure.
	const bool miniature = this->intro_showing() || this->reveal_showing() ||
		(this->state_ != LevelState::start_countdown && this->state_ != LevelState::active);
	// These existing stable targets include every structure and player, but no
	// retiring projectile. This is mode configuration, not a draw-order mirror.
	for (CollisionObject* object : this->projectile_targets_)
	{
		if (Structure* structure = Collider::of(*object).structure())
		{
			structure->set_miniature(miniature);
		}
	}
	this->scene_->clear_views();

	if (this->intro_showing() || this->reveal_showing())
	{
		this->add_intro_views();
		return;
	}

	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		// Human panes retain their participant identity across bot-seat gaps.
		for (size_t i = 0; i < this->view_participants_.size(); ++i)
		{
			this->scene_->add_view(
				Viewport(this->viewport_manager_->player_viewport(
					static_cast<int>(i)).pixel_rect()),
				this->view_camera(this->view_participants_[i]));
		}
	}
	else
	{
		// Use one fullscreen view without changing the shared split-screen layout.
		this->scene_->add_view(this->viewport_manager_->fullscreen_viewport(),
			this->zoom_out_camera_);
	}
}

int Level::player_count() const
{
	return static_cast<int>(this->players_.size());
}

const Player& Level::player(int participant) const
{
	return *this->players_.at(static_cast<size_t>(participant));
}

int Level::view_count() const
{
	return static_cast<int>(this->view_participants_.size());
}

int Level::participant_for_view(int view) const
{
	return this->view_participants_.at(static_cast<std::size_t>(view));
}

int Level::view_for_participant(int participant) const
{
	return this->participant_views_.at(static_cast<std::size_t>(participant));
}

bool Level::is_bot(int participant) const
{
	return this->roster_.at(static_cast<std::size_t>(participant)).controller == PlayerController::bot;
}

BotDifficulty Level::bot_difficulty(int participant) const
{
	return this->roster_.at(static_cast<std::size_t>(participant)).difficulty;
}

std::wstring_view Level::bot_identity(int participant) const
{
	const std::unique_ptr<Text>& text = this->bot_identity_texts_.at(static_cast<std::size_t>(participant));
	return text == nullptr ? std::wstring_view{} : text->text();
}

std::vector<RectangleF> Level::bot_obstacles(PlayerTeam team) const
{
	std::vector<RectangleF> bounds;
	for (const CollisionObject* object : this->projectile_targets_)
	{
		if (is_structure(to_collision_type(object->tag())) && object->shape() != nullptr &&
			!passable_gate(object, team))
		{ bounds.push_back(object->shape()->bounding_box()); }
	}
	return bounds;
}

std::vector<MatchBotSupport> Level::bot_supports(PlayerTeam team) const
{
	std::vector<MatchBotSupport> supports;
	// The target list is fixed for the match, so a position in it is a stable
	// identity even for a support that moves.
	std::uint32_t id = 0;
	for (const CollisionObject* object : this->projectile_targets_)
	{
		++id;
		const Structure* structure = Collider::of(*object).structure();
		if (structure != nullptr && structure->flat() && object->shape() != nullptr &&
			!passable_gate(object, team))
		{
			supports.push_back({ object->shape()->bounding_box(),
				structure->jump_through(), structure->paintable() != nullptr, id,
				structure->mover() != nullptr });
		}
	}
	return supports;
}

std::vector<MatchBotRamp> Level::bot_ramps() const
{
	std::vector<MatchBotRamp> ramps;
	for (const CollisionObject* object : this->projectile_targets_)
	{
		const CollisionObjectType type = to_collision_type(object->tag());
		if (is_structure_ramp(type) && object->shape() != nullptr)
		{
			ramps.push_back({ object->shape()->bounding_box(),
				type == CollisionObjectType::structure_ramp_right });
		}
	}
	return ramps;
}

bool Level::bot_line_of_sight(Vector2F from, Vector2F to) const
{
	return this->bot_traversal_line_of_sight(from, to, PlayerTeam::none);
}

bool Level::bot_traversal_line_of_sight(Vector2F from, Vector2F to, PlayerTeam team) const
{
	if (!std::isfinite(from.x) || !std::isfinite(from.y) ||
		!std::isfinite(to.x) || !std::isfinite(to.y)) { return false; }
	const Segment sight(from, to);
	for (const CollisionObject* object : this->projectile_targets_)
	{
		if (is_structure(to_collision_type(object->tag())) && object->shape() != nullptr &&
			!passable_gate(object, team) &&
			rectangle_segment_intersect(object->shape()->bounding_box(), sight)) { return false; }
	}
	return true;
}

LevelState Level::state() const
{
	return this->state_;
}

bool Level::match_started() const
{
	return this->state_ == LevelState::active || this->state_ == LevelState::zoom_out ||
		this->state_ == LevelState::overview || this->state_ == LevelState::finished;
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
		// Scoring is a role: a paintable support can retain jump-through
		// collision without losing its held area or player contribution.
		PaintableObject* paintable_object = Collider::of(*object).paintable();
		if (paintable_object == nullptr)
		{
			if (to_collision_type(object->tag()) == CollisionObjectType::structure_paintable)
			{
				throw std::runtime_error(
					"Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
			}
			continue;
		}

		PaintTotal paint = paintable_object->paint_total();

		result.team_a_area += paint.team_a;
		result.team_b_area += paint.team_b;
		result.total_area += paint.total;
		for (size_t slot = 0; slot < result.player_areas.size(); ++slot)
		{
			result.player_areas[slot] += paint.player_areas[slot];
			result.laid_areas[slot] += paint.credit.laid[slot];
			result.recaptured_areas[slot] += paint.credit.recaptured[slot];
		}
	}

	// The award tallies the players keep. Lethal credit is already by owner.
	for (size_t participant = 0; participant < this->players_.size(); ++participant)
	{
		const Player* player = this->players_[participant];
		const int owner = paint_owners::index(player->player_num(), static_cast<int>(participant));
		if (owner >= 0)
		{
			result.longest_airtimes[static_cast<size_t>(owner)] = player->longest_airtime();
		}
		const PlayerLethalProjectiles& lethal = player->lethal_projectiles();
		for (size_t slot = 0; slot < result.splats.size(); ++slot)
		{
			result.splats[slot] += lethal.enemy_by_owner[slot];
		}
	}

	return result;
}

std::vector<PaintSurfaceSample> Level::paint_surfaces() const
{
    std::vector<PaintSurfaceSample> samples;
    std::size_t source_index = 0;
    for (const CollisionObject* object : this->scene_->collision_objects())
    {
        const PaintableObject* paintable = Collider::of(*object).paintable();
        if (paintable == nullptr)
        {
            if (to_collision_type(object->tag()) == CollisionObjectType::structure_paintable)
            {
                throw std::runtime_error("Object tagged STRUCTURE_PAINTABLE is not an PaintableObject");
            }
            continue;
        }
        paintable->append_paint_surfaces(samples, source_index++);
    }
    return samples;
}

std::span<const TeamGateStructurePaintable* const> Level::team_gates() const
{
    return this->team_gates_view_;
}

void Level::settle_team_gates()
{
    for (TeamGateStructurePaintable* gate : this->team_gates_) { gate->settle(this->players_); }
}
