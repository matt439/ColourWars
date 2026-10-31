#include "game/pch.h"
#include "game/objects/player_identity_hud.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/paper_card.h"
#include "game/objects/player.h"
#include "game/objects/player_badge.h"
#include "game/play_settings.h"
#include "game/presentation.h"
#include "game/seat_setup.h"
#include "engine/render/text.h"
#include <algorithm>

using namespace mattmath;
using namespace labrador;

PlayerIdentityHud::PlayerIdentityHud(const Presentation* presentation, RenderResources* render_resources,
	const PlaySettings* preferences, std::size_t players) :
	presentation_(presentation),
	render_resources_(render_resources),
	preferences_(preferences)
{
	this->bot_texts_.resize(players);
}

PlayerIdentityHud::~PlayerIdentityHud() = default;

void PlayerIdentityHud::add_bot_text(std::size_t participant, BotDifficulty difficulty)
{
	this->bot_texts_[participant] = std::make_unique<Text>(
		this->presentation_->menu_text().bot_roster_label(static_cast<int>(participant),
			difficulty), this->presentation_->hud().player_identity_font,
		Vector2F::ZERO, this->render_resources_, this->presentation_->player_identity().ink_colour);
}

void PlayerIdentityHud::create_badge()
{
	this->badge_ = std::make_unique<PlayerBadge>(
		&this->presentation_->player_identity(), this->render_resources_);
	this->remaining_.assign(this->bot_texts_.size(),
		this->presentation_->player_identity().emphasis_duration);
}

void PlayerIdentityHud::refresh_texts(const std::vector<Player*>& players, const std::vector<SeatSetup>& roster)
{
	this->identity_texts_.resize(players.size());
	for (std::size_t participant = 0; participant < players.size(); ++participant)
	{
		const Player& player = *players[participant];
		const bool labelled = participant < this->seat_labels_.size() &&
			!this->seat_labels_[participant].empty();
		this->identity_texts_[participant] = InterfaceGameplay::identity_text(
			static_cast<int>(participant), player.team(), labelled ?
			this->seat_labels_[participant] :
			(roster.at(participant).controller == PlayerController::bot ?
				this->presentation_->menu_text().bot_label(roster[participant].difficulty) :
				input_slots::device_name(this->pad_device(player.player_num()))));
	}
}

void PlayerIdentityHud::set_seat_labels(std::vector<std::wstring> labels,
	const std::vector<Player*>& players, const std::vector<SeatSetup>& roster)
{
	this->seat_labels_ = std::move(labels);
	this->refresh_texts(players, roster);
}

void PlayerIdentityHud::set_pad_devices(const std::array<int, input_slots::pad_count>& devices,
	const std::vector<Player*>& players, const std::vector<SeatSetup>& roster)
{
	if (devices == this->pad_devices_) { return; }
	this->pad_devices_ = devices;
	this->refresh_texts(players, roster);
}

int PlayerIdentityHud::pad_device(int slot) const
{
	return input_slots::is_pad(slot) && this->pad_devices_[static_cast<size_t>(slot)] >= 0 ?
		this->pad_devices_[static_cast<size_t>(slot)] : slot;
}

void PlayerIdentityHud::emphasise()
{
	std::fill(this->remaining_.begin(), this->remaining_.end(),
		this->presentation_->player_identity().emphasis_duration);
}

void PlayerIdentityHud::emphasise(std::size_t participant)
{
	this->remaining_[participant] = this->presentation_->player_identity().emphasis_duration;
}

void PlayerIdentityHud::decay(float dt)
{
	for (float& remaining : this->remaining_)
	{
		remaining = std::max(0.0f, remaining - std::max(0.0f, dt));
	}
}

std::wstring_view PlayerIdentityHud::bot_identity(std::size_t participant) const
{
	const std::unique_ptr<Text>& text = this->bot_texts_.at(participant);
	return text == nullptr ? std::wstring_view{} : text->text();
}

const PlayerBadge& PlayerIdentityHud::badge() const
{
	return *this->badge_;
}

const std::wstring& PlayerIdentityHud::identity_text(std::size_t participant) const
{
	return this->identity_texts_[participant];
}

bool PlayerIdentityHud::visible(const Player& player, bool showing) const
{
	return this->presentation_->player_identity().enabled && showing &&
		player.state() == PlayerState::alive && player.health() > 0.0f;
}

float PlayerIdentityHud::emphasis(std::size_t participant, const Player& player, bool showing,
	int viewer_slot) const
{
	if (!this->visible(player, showing) || this->preferences_->reduce_effects_for(viewer_slot))
	{
		return 0.0f;
	}
	return std::clamp(this->remaining_[participant] /
		this->presentation_->player_identity().emphasis_duration, 0.0f, 1.0f);
}

RectangleF PlayerIdentityHud::bounds(std::size_t participant, const Player& player, bool showing,
	int viewer_slot) const
{
	const PlayerIdentityPresentation& look = this->presentation_->player_identity();
	const RectangleF body = player.rectangle();
	const float emphasis = this->emphasis(participant, player, showing, viewer_slot);
	const float scale = 1.0f + (look.emphasis_scale - 1.0f) * emphasis;
	const Vector2F size = look.world_size * scale;
	const Vector2F bottom = Vector2F(body.center().x, body.top()) + look.world_offset;
	return RectangleF(bottom - Vector2F(size.x * 0.5f, size.y), size);
}

void PlayerIdentityHud::draw_badge(DrawList& list, const RectangleF& bounds, int participant) const
{
	this->badge_->draw(list, bounds, participant);
}

void PlayerIdentityHud::draw_bot(DrawList& list, RectangleF badge, int bot,
	const RectangleF& visible, const PaperCard& paper) const
{
	const Text& text = *this->bot_texts_[static_cast<std::size_t>(bot)];
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
	this->badge_->draw(list, badge, bot);
	paper.sheet()->draw(list,
		paper.fill(), RectangleF(position, size),
		this->presentation_->player_identity().background_colour);
	text.draw_with(list, this->presentation_->player_identity().ink_colour,
		position - measured.position() * scale, scale);
}
