#include "game/pch.h"
#include "game/objects/level.h"
#include "game/debug/debug_overlay_draw.h"
#include "game/input_slots.h"

using namespace mattmath;
using namespace labrador;

// Level's drawing: the match and overview passes, the per-pane overlay,
// player identity and the aim-marker query. Cameras and views are in level.cpp.

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
		this->scene_->draw(renderer);
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

	this->interface_gameplay_->draw_gameplay_interface_prepared(list,
		player_vp.size(),
		view_index,
		this->identity_texts_[static_cast<std::size_t>(view_index)],
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

void Level::refresh_identity_texts()
{
	this->identity_texts_.resize(this->players_.size());
	for (std::size_t view = 0; view < this->players_.size(); ++view)
	{
		const Player& player = *this->players_[view];
		const bool labelled = view < this->seat_labels_.size() &&
			!this->seat_labels_[view].empty();
		this->identity_texts_[view] = InterfaceGameplay::identity_text(
			static_cast<int>(view), player.team(), labelled ?
			this->seat_labels_[view] : input_slots::device_name(player.player_num()));
	}
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
