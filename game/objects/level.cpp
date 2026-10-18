#include "game/pch.h"
#include "game/objects/level.h"
#include "game/debug/debug_overlay_draw.h"
#include "game/objects/crude_drawing.h"
#include "game/objects/miniature_view.h"
#include "game/input_slots.h"
#include "engine/render/text_encoding.h"

using namespace mattmath;
using namespace DirectX;
using namespace labrador;

Level::Level(std::unique_ptr<Scene> scene,
	std::vector<GameObject*> scenery,
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
	// Disambiguate from DirectX::Mouse.
	const labrador::Mouse* mouse,
	std::vector<LevelStation> stations,
	OpeningMode opening, std::optional<float> duration) :
	scene_(std::move(scene)),
	scenery_(std::move(scenery)),
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
	this->presentation_ = presentation;
	this->render_resources_ = render_resources;
	const MenuStyle& menu = presentation->menu();
	const CountdownArtPresentation& paper = presentation->match().countdown_art;
	this->stage_entry_ = std::make_unique<TextDropShadow>(stage_name,
		menu.heading_font, Vector2F::ZERO, render_resources,
		paper.ink, paper.paper, menu.heading_shadow_offset);
	this->player_badge_ = std::make_unique<PlayerBadge>(
		&presentation->player_identity(), render_resources);
	this->player_identity_remaining_.assign(this->players_.size(),
		presentation->player_identity().emphasis_duration);
	this->paint_audio_ = std::make_unique<PaintAudio>(&tuning->paint(), audio_resources, preferences);
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
			this->scenery_.push_back(drip);
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
		this->reveal_art_ = std::make_unique<RevealArt>(&look, render_resources);
		this->hover_sound_ = this->sound_bank_->resolve_wave(match.hover_sound.name);
		this->click_sound_ = this->sound_bank_->resolve_wave(match.click_sound.name);
		this->close_sound_ = this->sound_bank_->resolve_wave(match.close_sound.name);
		this->state_ = LevelState::opening;
		// A quick rematch starts with the colours being tried.
		this->opening_time_ = opening == OpeningMode::quick ? match.opening.settle : 0.0f;
		this->colour_players(false, false);
	}

	this->build_opening_frame(render_resources);
}

Level::~Level()
{
	if (this->end_sting_.valid()) { this->sound_bank_->stop_effect(this->end_sting_, true); }
	if (this->music_.valid())
	{
		this->sound_bank_->stop_effect(this->music_, true);
	}
	this->sound_bank_->stop_effect(this->zoom_out_sound_, true);
	this->sound_bank_->stop_effect(this->countdown_sound_, true);
	this->sound_bank_->stop_effect(this->go_sound_, true);
	this->sound_bank_->stop_effect(this->time_up_sound_, true);
	this->sound_bank_->stop_effect(this->last_ten_sound_, true);
	this->sound_bank_->stop_effect(this->room_tone_sound_, true);
	this->sound_bank_->stop_effect(this->sigh_sound_, true);
	this->sound_bank_->stop_effect(this->chair_sound_, true);
	this->sound_bank_->stop_effect(this->glass_in_sound_, true);
	this->sound_bank_->stop_effect(this->celebrate_sound_, true);
	this->sound_bank_->stop_effect(this->slump_sound_, true);
	this->sound_bank_->stop_effect(this->colour_in_sound_, true);
	this->sound_bank_->stop_effect(this->skip_sound_, true);
	this->sound_bank_->stop_effect(this->undo_sound_, true);
	this->stop_player_sounds();
}

void Level::suspend() const
{
	for (Player* player : this->players_)
	{
		player->reset_jump_input();
		player->clear_view_shake();
	}
	this->pause_audio();
}

void Level::pause_audio() const
{
	if (this->end_sting_.valid()) { this->sound_bank_->pause_effect(this->end_sting_); }
	if (this->music_.valid())
	{
		this->sound_bank_->pause_effect(this->music_);
	}
	this->sound_bank_->pause_effect(this->zoom_out_sound_);
	this->sound_bank_->pause_effect(this->countdown_sound_);
	this->sound_bank_->pause_effect(this->go_sound_);
	this->sound_bank_->pause_effect(this->time_up_sound_);
	this->sound_bank_->pause_effect(this->last_ten_sound_);
	this->sound_bank_->pause_effect(this->room_tone_sound_);
	this->sound_bank_->pause_effect(this->sigh_sound_);
	this->sound_bank_->pause_effect(this->chair_sound_);
	this->sound_bank_->pause_effect(this->glass_in_sound_);
	this->sound_bank_->pause_effect(this->celebrate_sound_);
	this->sound_bank_->pause_effect(this->slump_sound_);
	this->sound_bank_->pause_effect(this->colour_in_sound_);
	this->sound_bank_->pause_effect(this->skip_sound_);
	this->sound_bank_->pause_effect(this->undo_sound_);
	this->pause_player_sounds();
}

void Level::resume()
{
	this->set_music_ducked(false);
	if (this->end_sting_.valid())
	{
		this->sound_bank_->set_effect_volume(this->end_sting_,
			this->end_sting_volume_ * this->preferences_->music_gain());
		this->sound_bank_->resume_effect(this->end_sting_);
	}
	this->resume_music();
	this->sound_bank_->set_effect_volume(this->zoom_out_sound_,
		this->tuning_->match().zoom_out_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->zoom_out_sound_);
	this->sound_bank_->set_effect_volume(this->countdown_sound_,
		this->tuning_->match().countdown_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->countdown_sound_);
	this->sound_bank_->set_effect_volume(this->go_sound_,
		this->tuning_->match().go_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->go_sound_);
	this->sound_bank_->set_effect_volume(this->time_up_sound_,
		this->tuning_->match().time_up_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->time_up_sound_);
	this->sound_bank_->set_effect_volume(this->last_ten_sound_,
		this->tuning_->match().last_ten_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->last_ten_sound_);
	this->sound_bank_->set_effect_volume(this->room_tone_sound_,
		this->tuning_->match().room_tone_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->room_tone_sound_);
	this->sound_bank_->set_effect_volume(this->sigh_sound_,
		this->tuning_->match().sigh_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->sigh_sound_);
	this->sound_bank_->set_effect_volume(this->chair_sound_,
		this->tuning_->match().chair_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->chair_sound_);
	this->sound_bank_->set_effect_volume(this->glass_in_sound_,
		this->tuning_->match().glass_in_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->glass_in_sound_);
	this->sound_bank_->set_effect_volume(this->celebrate_sound_,
		this->tuning_->match().celebrate_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->celebrate_sound_);
	this->sound_bank_->set_effect_volume(this->slump_sound_,
		this->tuning_->match().slump_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->slump_sound_);
	this->sound_bank_->set_effect_volume(this->colour_in_sound_,
		this->tuning_->match().colour_in_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->colour_in_sound_);
	this->sound_bank_->set_effect_volume(this->skip_sound_,
		this->tuning_->match().skip_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->skip_sound_);
	this->sound_bank_->set_effect_volume(this->undo_sound_,
		this->tuning_->match().undo_sound.volume * this->preferences_->sfx_gain());
	this->sound_bank_->resume_effect(this->undo_sound_);
	this->resume_player_sounds();
}

void Level::prepare_resume(const std::vector<PlayerInputData>& inputs)
{
	for (Player* player : this->players_)
	{
		player->primary_weapon().stop_sounds();
		const int slot = player->player_num();
		if (slot >= 0 && static_cast<size_t>(slot) < inputs.size())
		{
			player->observe_jump_release(inputs[slot].jump_held || inputs[slot].jump_pressed);
		}
	}
}

void Level::update_music(float dt)
{
	if (!(dt > 0.0f) || !std::isfinite(dt) || !this->music_.valid() ||
		this->sound_bank_->effect_state(this->music_) != SoundState::playing) { return; }
	this->music_fade_.advance(dt);
	this->music_duck_.advance(dt);
	this->sound_bank_->set_effect_volume(this->music_, this->music_volume_ *
		this->preferences_->music_gain() * this->music_fade_.gain() * this->music_duck_.gain());
	if (this->music_ending_ && this->music_fade_.gain() == 0.0f)
	{
		this->sound_bank_->stop_effect(this->music_, true);
	}
}

void Level::set_music_ducked(bool ducked)
{
	const MusicMixPresentation& mix = this->presentation_->music_mix();
	this->music_duck_.fade_to(ducked ? mix.pause_gain : 1.0f,
		ducked ? mix.duck_time : mix.restore_time);
}

void Level::resume_music() const
{
	if (!this->music_.valid() || this->sound_bank_->effect_state(this->music_) != SoundState::paused) { return; }
	this->sound_bank_->set_effect_volume(this->music_, this->music_volume_ *
		this->preferences_->music_gain() * this->music_fade_.gain() * this->music_duck_.gain());
	this->sound_bank_->resume_effect(this->music_);
}

void Level::update(const std::vector<PlayerInputData>& player_inputs, float dt,
	SeatInputFilter* seats)
{
	this->refresh_layout();
	this->simulation_dt_ = dt;
	if (dt > 0.0f)
	{
		this->update_state(player_inputs, dt, seats);
		this->update_acting(dt);
	}

	// Refresh on every path so views and overlays reflect the completed tick.
	this->refresh_views();
}

void Level::update_state(const std::vector<PlayerInputData>& player_inputs,
	float dt, SeatInputFilter* seats)
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
				this->tuning_->match().glass_in_sound.volume * this->preferences_->sfx_gain());
		}
		// One cue for the displayed number, including the initial beat. A long
		// tick never queues the missed beats over GO or live play.
		if (this->start_timer_ > 0.0f &&
			(starting ||
				std::ceil(before) != std::ceil(this->start_timer_)))
		{
			this->sound_bank_->play_effect(this->countdown_sound_, false,
				this->tuning_->match().countdown_sound.volume * this->preferences_->sfx_gain());
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
					this->tuning_->match().go_sound.volume * this->preferences_->sfx_gain());
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
		this->update_level_logic(player_inputs, dt, seats);

		this->timer_ -= dt;
		// Sound only the current displayed second, never missed seconds after a
		// long update. Short matches also get their first positive clock cue.
		const float second = std::ceil(this->timer_);
		if (this->timer_ > 0.0f && second <= 10.0f && second != this->last_ten_second_)
		{
			this->last_ten_second_ = second;
			this->sound_bank_->play_effect(this->last_ten_sound_, false,
				this->tuning_->match().last_ten_sound.volume * this->preferences_->sfx_gain());
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
					this->end_sting_volume_ * this->preferences_->music_gain());
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
				this->tuning_->match().time_up_sound.volume * this->preferences_->sfx_gain());
			this->sound_bank_->play_effect(this->zoom_out_sound_, false,
				this->tuning_->match().zoom_out_sound.volume * this->preferences_->sfx_gain());
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
	float dt, SeatInputFilter* seats)
{
	std::array<bool, 4> was_dead{};
	for (size_t view = 0; view < this->players_.size(); ++view)
	{
		was_dead[view] = this->players_[view]->state() == PlayerState::dead;
	}
	this->update_player_identity(dt);
	// Run the scene phases in order, with game-specific work between them.
	this->paint_audio_->update(dt);
	this->blocked_shot_feedback_->update(dt);
	this->apply_player_inputs(player_inputs, seats);

	this->scene_->update(dt);
	this->resolve_projectile_movement();

	this->update_players(dt);

	this->scene_->resolve();
	for (CollisionObject* object : this->scene_->collision_objects())
	{
		if (is_projectile(to_collision_type(object->tag())))
		{
			Projectile& projectile = dynamic_cast<Projectile&>(*object);
			projectile.flush_paint_audio(*this->paint_audio_);
			if (const auto contact = projectile.take_blocked_contact())
			{
				this->blocked_shot_feedback_->emit(*contact);
			}
		}
	}

	// Loss of ground support is known only after all contacts have been dispatched.
	for (Player* player : this->players_)
	{
		player->end_contacts(this->scene_->contacts());
	}

	this->update_players_after_resolve();
	for (size_t view = 0; view < this->players_.size(); ++view)
	{
		if (was_dead[view] && this->players_[view]->state() == PlayerState::alive)
		{
			this->player_identity_remaining_[view] =
				this->presentation_->player_identity().emphasis_duration;
		}
	}
	this->update_idle_drips(dt);

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

const IdleDrip* Level::idle_drip(int view) const
{
	return this->idle_drips_.at(static_cast<size_t>(view));
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
			player->camera_focus(),
			this->viewport_manager_->player_viewport(
				static_cast<int>(i)),
			this->resolution_manager_->resolution_vec().y,
			this->presentation_->camera(),
			player->camera(),
			this->camera_bounds_);
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

Camera Level::view_camera(int view) const
{
	const Player& player = *this->players_.at(static_cast<size_t>(view));
	if (this->state_ != LevelState::active || !this->preferences_->view_shake_enabled() ||
		player.view_shake().offset() == Vector2F::ZERO) { return player.camera(); }
	return PlayerCamera::displaced(player.camera(),
		this->viewport_manager_->player_viewport(view), player.view_shake().offset(),
		this->camera_bounds_);
}

void Level::refresh_views()
{
	this->scene_->clear_views();

	if (this->intro_showing() || this->reveal_showing())
	{
		this->add_intro_views();
		return;
	}

	if (this->state_ == LevelState::start_countdown ||
		this->state_ == LevelState::active)
	{
		// Pane order follows players_, not input slots, which may be sparse.
		for (size_t i = 0; i < this->players_.size(); ++i)
		{
			this->scene_->add_view(
				Viewport(this->viewport_manager_->player_viewport(
					static_cast<int>(i)).pixel_rect()),
				this->view_camera(static_cast<int>(i)));
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
	if (this->intro_showing() || this->reveal_showing())
	{
		this->draw_intro(renderer);
		return;
	}
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
		this->draw_miniature_world(renderer);
	}
}

void Level::draw_miniature_world(Renderer& renderer, const Scene::ViewOverlay& overlay) const
{
	const int count = this->scene_->view_count();
	if (count == 0) { return; }
	const int offset = std::max(0, renderer.view_count() - 1);
	renderer.set_view_count(offset + count);
	// At most four cosmetic views, drawn sequentially. Scene remains the only
	// owner of the normal gameplay fan-out. The pin's collision span mirrors
	// its current render order; a native parity check guards that dependency.
	for (int index = 0; index < count; ++index)
	{
		const Scene::View& view = this->scene_->view(index);
		DrawList list = renderer.view(offset + index);
		draw_miniature_view(list, view.camera, view.viewport, this->scenery_,
			this->scene_->collision_objects());
		if (overlay) { overlay(index, list); }
	}
}

void Level::draw_player_overlay(int view_index, DrawList& list) const
{
	const Player* player = this->players_[static_cast<size_t>(view_index)];

	const Viewport& player_vp = this->scene_->view(view_index).viewport;
	// Detached world contacts remain visible even if their shot or owner has
	// retired or left the pane. Render workers only read the fixed cue storage.
	list.set_camera(this->scene_->view(view_index).camera);
	this->blocked_shot_feedback_->draw(list);
	if (this->aim_marker_visible(view_index))
	{
		list.set_camera(this->scene_->view(view_index).camera);
		player->primary_weapon().draw_aim_marker(list);
	}
	if (this->player_identity_visible(view_index))
	{
		list.set_filter(TextureFilter::point);
		this->player_badge_->draw(list, this->player_identity_bounds(view_index), view_index);
	}

	// Dividers use window coordinates; subtract the pane's screen origin
	// to draw them into this viewport.
	list.set_camera(Camera(player_vp));

	for (const std::unique_ptr<GameObject>& divider : this->viewport_dividers_)
	{
		divider->draw(list);
	}

	// Developer overlays, in world space under the HUD. Never on in Release.
	if (this->overlays_.any_pane())
	{
		debug_overlay::draw_pane(list, this->overlays_, *this->scene_,
			this->scene_->view(view_index), *player, this->camera_bounds_,
			*this->render_resources_->sprite_sheet(this->overlay_sheet_), this->overlay_fill_);
	}

	// The HUD uses pane-local coordinates, so draw it with the identity camera.
	list.set_camera(Camera::DEFAULT_CAMERA);

	const std::size_t view = static_cast<std::size_t>(view_index);
	const std::wstring device = view < this->seat_labels_.size() &&
		!this->seat_labels_[view].empty() ?
		this->seat_labels_[view] : input_slots::device_name(player->player_num());
	this->interface_gameplay_->draw_gameplay_interface(list,
		player_vp.size(),
		view_index,
		device,
		player->team(),
		player->health() / this->tuning_->player().starting_health,
		player->weapon_ammo() / this->tuning_->starting_ammo(),
		this->timer_,
		player->team_colour(),
		this->preferences_->reduce_effects,
		this->state_ == LevelState::active);

	if (player->showing_debug())
	{
		this->debug_text_->draw_debug_info(list, player,
			this->projectile_count_, this->simulation_dt_, this->debug_frame_ms_);
	}
}

bool Level::player_identity_visible(int view) const
{
	return view >= 0 && view < this->player_count() &&
		this->presentation_->player_identity().enabled &&
		(this->state_ == LevelState::start_countdown || this->state_ == LevelState::active) &&
		this->player(view).state() == PlayerState::alive && this->player(view).health() > 0.0f;
}

float Level::player_identity_emphasis(int view) const
{
	if (!this->player_identity_visible(view) || this->preferences_->reduce_effects)
	{
		return 0.0f;
	}
	return std::clamp(this->player_identity_remaining_[static_cast<size_t>(view)] /
		this->presentation_->player_identity().emphasis_duration, 0.0f, 1.0f);
}

RectangleF Level::player_identity_bounds(int view) const
{
	const PlayerIdentityPresentation& look = this->presentation_->player_identity();
	const RectangleF body = this->player(view).rectangle();
	const float scale = 1.0f + (look.emphasis_scale - 1.0f) *
		this->player_identity_emphasis(view);
	const Vector2F size = look.world_size * scale;
	const Vector2F bottom = Vector2F(body.center().x, body.top()) + look.world_offset;
	return RectangleF(bottom - Vector2F(size.x * 0.5f, size.y), size);
}

void Level::emphasise_players()
{
	if (this->state_ != LevelState::active) { return; }
	std::fill(this->player_identity_remaining_.begin(), this->player_identity_remaining_.end(),
		this->presentation_->player_identity().emphasis_duration);
}

void Level::update_player_identity(float dt)
{
	for (float& remaining : this->player_identity_remaining_)
	{
		remaining = std::max(0.0f, remaining - std::max(0.0f, dt));
	}
}

bool Level::aim_marker_visible(int view) const
{
	if (view < 0 || static_cast<size_t>(view) >= this->players_.size()) { return false; }
	const Player& player = *this->players_.at(static_cast<size_t>(view));
	return this->state_ == LevelState::active && this->preferences_->aim_marker &&
		this->presentation_->weapon_feedback().enabled && player.aim_marker_allowed() &&
		player.primary_weapon().type() != WeaponType::roller;
}

const BlockedShotFeedback& Level::blocked_shot_feedback() const
{
	return *this->blocked_shot_feedback_;
}

void Level::set_overlays(const LevelOverlays& overlays)
{
	this->overlays_ = overlays;
	this->refresh_station_labels();
}

void Level::refresh_station_labels()
{
	const StationLabelPresentation& look = this->presentation_->debug_overlay().station_label;
	if (this->overlays_.stations && this->station_labels_.empty() && !this->stations_.empty())
	{
		for (const LevelStation& station : this->stations_)
		{
			this->station_labels_.push_back(this->scene_->add(std::make_unique<Label>(
				widen(station.name), look.font, station.position + look.offset,
				this->render_resources_, look.colour, look.scale)));
			this->scenery_.push_back(this->station_labels_.back());
		}
		// Admit them now, so a paused match shows them too. This runs between
		// world updates, when every tick's own additions have been admitted
		// and its retirements made, so the labels are all this tick ends.
		this->scene_->end_tick();
	}
	// The scene cannot retire a Label and Label is final, so off is a tint
	// that contributes nothing (docs/review/engine-reports.md, report 18).
	// The blend is premultiplied (ONE, INV_SRC_ALPHA) and the texel is
	// multiplied by the tint as given, so that tint is zero in all four
	// channels: make_transparent() zeroes only alpha, and its rgb would be
	// added over the world. On is premultiplied for the same reason.
	const Colour colour = this->overlays_.stations ?
		CrudeDrawing::premultiplied(look.colour) : Colour(0.0f, 0.0f, 0.0f, 0.0f);
	for (std::size_t index = 0; index < this->station_labels_.size(); ++index)
	{
		Label& label = *this->station_labels_[index];
		label.set_position(this->stations_[index].position + look.offset);
		label.set_scale(look.scale);
		label.set_colour(colour);
	}
}

int Level::station_label_count() const
{
	return static_cast<int>(this->station_labels_.size());
}

const Label& Level::station_label(int index) const
{
	return *this->station_labels_.at(static_cast<std::size_t>(index));
}

void Level::set_seat_labels(std::vector<std::wstring> labels)
{
	this->seat_labels_ = std::move(labels);
}

int Level::projectile_count() const
{
	return this->projectile_count_;
}

int Level::collision_object_count() const
{
	return static_cast<int>(this->scene_->collision_objects().size());
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
		this->player_identity_remaining_[static_cast<size_t>(view)] =
			this->presentation_->player_identity().emphasis_duration;
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
	const bool was_dead = player->state() == PlayerState::dead;
	player->force_respawn();
	if (was_dead)
	{
		this->player_identity_remaining_[static_cast<size_t>(view)] =
			this->presentation_->player_identity().emphasis_duration;
	}
	player->update_weapon_position();
	this->refresh_player_cameras();
	this->refresh_views();
	return true;
}

bool Level::kill_player(int view)
{
	if (view < 0 || view >= this->player_count())
	{
		return false;
	}
	this->players_[static_cast<size_t>(view)]->kill();
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
