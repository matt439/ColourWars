#include "game/pch.h"
#include "game/objects/level.h"
#include "game/objects/crude_drawing.h"
#include "engine/render/text_encoding.h"

using namespace mattmath;
using namespace labrador;

// Level's developer tools (level.h, "Developer tools"): overlays, station
// labels and the per-player cheats. Present in every build; only Debug
// builds call them.

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
