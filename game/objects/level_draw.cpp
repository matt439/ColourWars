#include "game/pch.h"
#include "game/objects/level.h"
#include "game/presentation.h"
#include "game/debug/debug_overlay_draw.h"
#include "game/input_slots.h"
#include "game/objects/crude_drawing.h"

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
		if (this->lesson_card_bounds().width > 0.0f)
		{
			DrawList list = renderer.view(renderer.view_count() - 1);
			list.set_viewport(this->viewport_manager_->fullscreen_viewport());
			list.set_camera(Camera::DEFAULT_CAMERA);
			this->draw_lesson_card(list);
		}
		if (this->cursor_marker_visible())
		{
			DrawList list = renderer.view(renderer.view_count() - 1);
			list.set_viewport(this->viewport_manager_->fullscreen_viewport());
			list.set_camera(Camera::DEFAULT_CAMERA);
			this->draw_cursor_marker(list);
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
		this->player_badge_->draw(list, this->player_identity_bounds(participant), participant);
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
		RectangleF badge = this->player_identity_bounds_for(bot, player->player_num());
		const Text& text = *this->bot_identity_texts_[static_cast<std::size_t>(bot)];
		const RectangleF measured = text.text_bounds_at(Vector2F::ZERO, 1.0f);
		const PlayerIdentityPresentation& look = this->presentation_->player_identity();
		const float emphasis = badge.width / look.world_size.x;
		const float scale = emphasis * std::min(
			look.bot_label_size.x / std::max(1.0f, measured.width),
			look.bot_label_size.y / std::max(1.0f, measured.height));
		const Vector2F size = measured.size() * scale;
		Vector2F position(badge.center().x - size.x * 0.5f,
			badge.top() - size.y - look.bot_label_gap * emphasis);
		const auto inside = [](float low, float high, float min, float max)
		{
			if (high - low > max - min) { return min - low; }
			return low < min ? min - low : (high > max ? max - high : 0.0f);
		};
		const Vector2F shift(
			inside(std::min(badge.left(), position.x), std::max(badge.right(), position.x + size.x),
				visible.left(), visible.right()),
			inside(position.y, badge.bottom(), visible.top(), visible.bottom()));
		badge = RectangleF(badge.position() + shift, badge.size());
		position += shift;
		this->player_badge_->draw(list, badge, bot);
		this->render_resources_->sprite_sheet(this->overlay_sheet_)->draw(list,
			this->overlay_fill_, RectangleF(position, size),
			this->presentation_->player_identity().background_colour);
		text.draw_with(list, this->presentation_->player_identity().ink_colour,
			position - measured.position() * scale, scale);
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

	// Teammates out of this pane's view, under the HUD so a pane's own health,
	// paint and clock stay on top where a chip reaches a corner.
	for (const TeammateMarkerPlacement& marker : this->teammate_markers(view_index))
	{
		this->teammate_marker_->draw(list, marker,
			this->players_[static_cast<std::size_t>(marker.participant)]->team_colour(),
			*this->player_badge_);
	}

	this->interface_gameplay_->draw_gameplay_interface_prepared(list,
		player_vp.size(),
		participant,
		this->identity_texts_[static_cast<std::size_t>(participant)],
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
	if (!this->presentation_->hud().cursor_marker.enabled || !this->pointer_)
	{
		return false;
	}
	const Vector2F size = this->resolution_manager_->resolution_vec();
	const Vector2F at = *this->pointer_;
	if (at.x < 0.0f || at.y < 0.0f || at.x >= size.x || at.y >= size.y) { return false; }
	for (int participant = 0; participant < this->player_count(); ++participant)
	{
		if (!this->is_bot(participant) &&
			input_slots::is_keyboard_and_mouse(this->player(participant).player_num()))
		{
			return true;
		}
	}
	return false;
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
	this->pointer_ = window_point;
}

void Level::draw_cursor_marker(DrawList& list) const
{
	const Vector2F at = *this->pointer_;
	const SpriteSheet* sheet = this->render_resources_->sprite_sheet(this->overlay_sheet_);
	list.set_filter(TextureFilter::point);
	for (const CursorMarkerFrame& frame : cursor_marker_frames(this->presentation_->hud().cursor_marker,
		at, this->resolution_manager_->resolution_vec().y))
	{
		const RectangleF& box = frame.box;
		const float width = frame.width;
		const Colour tint = CrudeDrawing::premultiplied(frame.colour);
		sheet->draw(list, this->overlay_fill_, RectangleF(box.x, box.y, box.width, width), tint);
		sheet->draw(list, this->overlay_fill_, RectangleF(box.x, box.bottom() - width, box.width, width), tint);
		sheet->draw(list, this->overlay_fill_, RectangleF(box.x, box.y + width, width, box.height - width * 2.0f), tint);
		sheet->draw(list, this->overlay_fill_, RectangleF(box.right() - width, box.y + width, width, box.height - width * 2.0f), tint);
	}
}

bool Level::player_identity_visible(int participant) const
{
	return participant >= 0 && participant < this->player_count() &&
		this->presentation_->player_identity().enabled &&
		(this->state_ == LevelState::start_countdown || this->state_ == LevelState::active) &&
		this->player(participant).state() == PlayerState::alive && this->player(participant).health() > 0.0f;
}

float Level::player_identity_emphasis(int participant) const
{
	return this->player_identity_emphasis_for(participant, this->player(participant).player_num());
}

float Level::player_identity_emphasis_for(int participant, int viewer_slot) const
{
	if (!this->player_identity_visible(participant) || this->preferences_->reduce_effects_for(viewer_slot))
	{
		return 0.0f;
	}
	return std::clamp(this->player_identity_remaining_[static_cast<size_t>(participant)] /
		this->presentation_->player_identity().emphasis_duration, 0.0f, 1.0f);
}

RectangleF Level::player_identity_bounds(int participant) const
{
	return this->player_identity_bounds_for(participant, this->player(participant).player_num());
}

RectangleF Level::player_identity_bounds_for(int participant, int viewer_slot) const
{
	const PlayerIdentityPresentation& look = this->presentation_->player_identity();
	const RectangleF body = this->player(participant).rectangle();
	const float emphasis = this->player_identity_emphasis_for(participant, viewer_slot);
	const float scale = 1.0f + (look.emphasis_scale - 1.0f) * emphasis;
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
	for (std::size_t participant = 0; participant < this->players_.size(); ++participant)
	{
		const Player& player = *this->players_[participant];
		const bool labelled = participant < this->seat_labels_.size() &&
			!this->seat_labels_[participant].empty();
		this->identity_texts_[participant] = InterfaceGameplay::identity_text(
			static_cast<int>(participant), player.team(), labelled ?
			this->seat_labels_[participant] : (this->is_bot(static_cast<int>(participant)) ?
				this->presentation_->menu_text().bot_label(this->roster_[participant].difficulty) :
				input_slots::device_name(this->pad_device(player.player_num()))));
	}
}

int Level::pad_device(int slot) const
{
	return input_slots::is_pad(slot) && this->pad_devices_[static_cast<size_t>(slot)] >= 0 ?
		this->pad_devices_[static_cast<size_t>(slot)] : slot;
}

void Level::set_pad_devices(const std::array<int, input_slots::pad_count>& devices)
{
	if (devices == this->pad_devices_) { return; }
	this->pad_devices_ = devices;
	this->refresh_identity_texts();
}

void Level::update_player_identity(float dt)
{
	for (float& remaining : this->player_identity_remaining_)
	{
		remaining = std::max(0.0f, remaining - std::max(0.0f, dt));
	}
}

float Level::hud_scale(int view) const
{
	const HudPresentation& look = this->presentation_->hud();
	const Vector2F pane = this->viewport_manager_->player_viewport(view).size();
	return std::min(pane.x / look.player_identity_reference_width, pane.y / look.reference_height);
}

std::vector<TeammateMarkerPlacement> Level::teammate_markers(int view) const
{
	std::vector<TeammateMarkerPlacement> markers;
	const TeammateMarkerPresentation& look = this->presentation_->hud().teammate_marker;
	if (view < 0 || view >= this->view_count() || !look.enabled ||
		this->state_ != LevelState::active) { return markers; }
	const int viewer = this->participant_for_view(view);
	const Player& own = this->player(viewer);
	const Vector2F pane = Viewport(this->viewport_manager_->player_viewport(view).pixel_rect()).size();
	const float scale = this->hud_scale(view);
	const bool steady = this->preferences_->reduce_effects_for(own.player_num());
	for (int participant = 0; participant < this->player_count(); ++participant)
	{
		const Player& mate = this->player(participant);
		if (participant == viewer || mate.team() != own.team() ||
			mate.state() != PlayerState::alive || mate.health() <= 0.0f) { continue; }
		std::optional<TeammateMarkerPlacement> placed = TeammateMarker::place(
			look, pane, own.camera(), mate.rectangle(), scale);
		if (!placed) { continue; }
		placed->participant = participant;
		const float age = this->teammate_marker_age_[
			static_cast<std::size_t>(view * this->player_count() + participant)];
		placed->opacity = steady || look.fade_in <= 0.0f ? 1.0f :
			std::clamp(age / look.fade_in, 0.0f, 1.0f);
		markers.push_back(*placed);
	}
	return markers;
}

void Level::update_teammate_markers(float dt)
{
	const TeammateMarkerPresentation& look = this->presentation_->hud().teammate_marker;
	const int count = this->player_count();
	for (int view = 0; view < this->view_count(); ++view)
	{
		const Player& own = this->player(this->participant_for_view(view));
		const Vector2F pane = Viewport(this->viewport_manager_->player_viewport(view).pixel_rect()).size();
		const float scale = this->hud_scale(view);
		for (int participant = 0; participant < count; ++participant)
		{
			float& age = this->teammate_marker_age_[static_cast<std::size_t>(view * count + participant)];
			const Player& mate = this->player(participant);
			const bool away = mate.state() == PlayerState::alive && mate.health() > 0.0f &&
				TeammateMarker::place(look, pane, own.camera(), mate.rectangle(), scale).has_value();
			// Held at the fade's end so a long absence never grows without bound.
			age = away ? std::min(age + std::max(0.0f, dt), std::max(look.fade_in, 0.0f)) : 0.0f;
		}
	}
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
