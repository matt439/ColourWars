#include "game/pch.h"
#include "game/objects/level.h"
#include "game/presentation.h"
#include "game/debug/debug_overlay_draw.h"
#include "game/input_slots.h"
#include "game/objects/crude_drawing.h"
#include "game/objects/teammate_markers.h"
#include "game/objects/player_identity_hud.h"
#include "game/objects/match_opening.h"

using namespace mattmath;
using namespace labrador;

// Level's drawing: the match and overview passes, the per-pane overlay,
// player identity and the aim-marker query. Cameras and views are in level.cpp.

void Level::draw(Renderer& renderer) const
{
	if (this->intro_showing() || this->reveal_showing())
	{
		// The countdown's number and cards show in the picture as they do in the panes.
		this->opening_->draw(renderer, *this->scene_, this->opening_clock(), this->players_,
			[this](DrawList& list) { this->draw_countdown_text(list); });
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
		if (this->lesson_card_bounds().width > 0.0f)
		{
			DrawList list = renderer.view(renderer.view_count() - 1);
			list.set_viewport(this->viewport_manager_->fullscreen_viewport());
			list.set_camera(Camera::DEFAULT_CAMERA);
			this->lessons_.draw(list, this->paper_);
		}
		if (this->cursor_marker_visible())
		{
			DrawList list = renderer.view(renderer.view_count() - 1);
			list.set_viewport(this->viewport_manager_->fullscreen_viewport());
			list.set_camera(Camera::DEFAULT_CAMERA);
			this->cursor_marker_.draw(list, this->paper_);
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
	const int participant = this->participant_for_view(view_index);
	const Player* player = this->players_[static_cast<size_t>(participant)];

	const Viewport& player_vp = this->scene_->view(view_index).viewport;
	// Detached world contacts remain visible even if their shot or owner has
	// retired or left the pane. Render workers only read the fixed cue storage.
	list.set_camera(this->scene_->view(view_index).camera);
	this->blocked_shot_feedback_->draw(list);
	if (this->aim_marker_visible(participant))
	{
		list.set_camera(this->scene_->view(view_index).camera);
		player->primary_weapon().draw_aim_marker(list);
	}
	if (this->player_identity_visible(participant))
	{
		list.set_filter(TextureFilter::point);
		this->identity_->draw_badge(list, this->player_identity_bounds(participant), participant);
	}
	// Bots share human views. Their stable roster number and explicit difficulty
	// travel with their world figure, never masquerading as an unclaimed human.
	// A bot's badge and label stay inside this pane, even when its body is
	// at or past the edge, so its identity is never cut off.
	const RectangleF visible = this->scene_->view(view_index).camera.visible_rectangle(
		Viewport(player_vp.pixel_rect()));
	for (int bot = 0; bot < this->player_count(); ++bot)
	{
		if (!this->is_bot(bot) || !this->player_identity_visible(bot)) { continue; }
		list.set_filter(TextureFilter::point);
		this->identity_->draw_bot(list, this->player_identity_bounds_for(bot, player->player_num()),
			bot, visible, this->paper_);
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
			*this->paper_.sheet(), this->paper_.fill());
	}

	// The HUD uses pane-local coordinates, so draw it with the identity camera.
	list.set_camera(Camera::DEFAULT_CAMERA);

	// Teammates out of this pane's view, under the HUD so a pane's own health,
	// paint and clock stay on top where a chip reaches a corner.
	for (const TeammateMarkerPlacement& marker : this->teammate_markers(view_index))
	{
		this->teammate_markers_->draw(list, marker,
			this->players_[static_cast<std::size_t>(marker.participant)]->team_colour(),
			this->identity_->badge());
	}

	this->interface_gameplay_->draw_gameplay_interface_prepared(list,
		player_vp.size(),
		participant,
		this->identity_->identity_text(static_cast<std::size_t>(participant)),
		player->health() / this->tuning_->player().starting_health,
		player->weapon_ammo() / this->tuning_->starting_ammo(),
		this->timer_,
		player->team_colour(),
		this->preferences_->reduce_effects_for(player->player_num()),
		this->state_ == LevelState::active,
		// The weapon stops updating while its player is dead, which would
		// freeze the blink mid-phase until the respawn refill.
		player->state() == PlayerState::alive && player->primary_weapon().low_paint(),
		player->primary_weapon().low_paint_elapsed(), !this->practice());

	if (player->showing_debug())
	{
		this->debug_text_->draw_debug_info(list, player,
			this->projectile_count_, this->simulation_dt_, this->debug_frame_ms_);
	}
}

bool Level::cursor_marker_visible() const
{
	bool keyboard_and_mouse_seat = false;
	for (int participant = 0; participant < this->player_count(); ++participant)
	{
		if (!this->is_bot(participant) &&
			input_slots::is_keyboard_and_mouse(this->player(participant).player_num()))
		{
			keyboard_and_mouse_seat = true;
			break;
		}
	}
	return this->cursor_marker_.visible(keyboard_and_mouse_seat);
}

std::array<CursorMarkerFrame, 3> Level::cursor_marker_frames(
	const CursorMarkerPresentation& look, const Vector2F& at, float window_height)
{
	// Whole pixels keep the marquee's edges crisp while the pointer moves.
	const float scale = window_height / 1080.0f;
	const float ink = std::max(1.0f, std::round(look.ink_width * scale));
	const float paper = std::max(1.0f, std::round(look.paper_width * scale));
	const float half = std::round(look.size * scale * 0.5f);
	const auto frame = [&](float inset, float width, const Colour& colour)
	{
		return CursorMarkerFrame{ RectangleF(at.x - half + inset, at.y - half + inset,
			(half - inset) * 2.0f, (half - inset) * 2.0f), width, colour };
	};
	// Ink outside and inside the paper rim reads on pale and dark ground alike.
	return { frame(0.0f, ink, look.ink), frame(ink, paper, look.paper), frame(ink + paper, ink, look.ink) };
}

void Level::set_pointer(std::optional<Vector2F> window_point)
{
	this->cursor_marker_.set_pointer(window_point);
}

bool Level::identity_showing() const
{
	return this->state_ == LevelState::start_countdown || this->state_ == LevelState::active;
}

bool Level::player_identity_visible(int participant) const
{
	return participant >= 0 && participant < this->player_count() &&
		this->identity_->visible(this->player(participant), this->identity_showing());
}

float Level::player_identity_emphasis(int participant) const
{
	return this->player_identity_emphasis_for(participant, this->player(participant).player_num());
}

float Level::player_identity_emphasis_for(int participant, int viewer_slot) const
{
	if (participant < 0 || participant >= this->player_count()) { return 0.0f; }
	return this->identity_->emphasis(static_cast<std::size_t>(participant), this->player(participant),
		this->identity_showing(), viewer_slot);
}

RectangleF Level::player_identity_bounds(int participant) const
{
	return this->player_identity_bounds_for(participant, this->player(participant).player_num());
}

RectangleF Level::player_identity_bounds_for(int participant, int viewer_slot) const
{
	return this->identity_->bounds(static_cast<std::size_t>(participant), this->player(participant),
		this->identity_showing(), viewer_slot);
}

void Level::emphasise_players()
{
	if (this->state_ != LevelState::active) { return; }
	this->identity_->emphasise();
}

void Level::set_pad_devices(const std::array<int, input_slots::pad_count>& devices)
{
	this->identity_->set_pad_devices(devices, this->players_, this->roster_);
}

std::vector<TeammateMarkerPlacement> Level::teammate_markers(int view) const
{
	return this->teammate_markers_->markers(view, this->players_, this->view_participants_,
		this->state_ == LevelState::active);
}

bool Level::aim_marker_visible(int participant) const
{
	if (participant < 0 || static_cast<size_t>(participant) >= this->players_.size()) { return false; }
	const Player& player = *this->players_.at(static_cast<size_t>(participant));
	return this->state_ == LevelState::active && this->preferences_->aim_marker_enabled(player.player_num()) &&
		this->presentation_->weapon_feedback().enabled && player.aim_marker_allowed() &&
		player.primary_weapon().has_aim_marker();
}

const BlockedShotFeedback& Level::blocked_shot_feedback() const
{
	return *this->blocked_shot_feedback_;
}
