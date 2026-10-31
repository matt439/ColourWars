#pragma once

#include "engine/math/rectanglef.h"
#include "game/player_control.h"
#include "game/input_slots.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace labrador
{
	class DrawList;
	class RenderResources;
	class Text;
}
class PaperCard;
class Player;
class PlayerBadge;
class Presentation;
struct PlaySettings;
struct SeatSetup;

// Who each figure is, for one match (HYG#40): the roster badge over every
// figure and its emphasis after a spawn or a covering menu, each bot's
// difficulty label, and each participant's HUD identity text with the seat labels
// and pad mapping it is built from. It borrows the presentation and the
// preferences from the owning Level's services and reads them on every call,
// so a `tune` or a Reduce effects change shows at once. The players, the
// roster and whether the match is showing identities are Level's, passed in.
class PlayerIdentityHud
{
public:
	// Empty: no bot labels and no badge until add_bot_text and create_badge.
	PlayerIdentityHud(const Presentation* presentation, labrador::RenderResources* render_resources,
		const PlaySettings* preferences, std::size_t players);
	~PlayerIdentityHud();
	PlayerIdentityHud(const PlayerIdentityHud&) = delete;
	PlayerIdentityHud& operator=(const PlayerIdentityHud&) = delete;

	// Bot seat `participant`'s fixed roster label, made from the menu text.
	void add_bot_text(std::size_t participant, BotDifficulty difficulty);
	// The badge, with every participant's emphasis full.
	void create_badge();
	// Rebuilds each identity text from the seat, team, pad and seat label.
	// Main thread, never from a draw.
	void refresh_texts(const std::vector<Player*>& players, const std::vector<SeatSetup>& roster);
	// See Level::set_seat_labels and Level::set_pad_devices; both refresh
	// the texts, the second only on a change.
	void set_seat_labels(std::vector<std::wstring> labels,
		const std::vector<Player*>& players, const std::vector<SeatSetup>& roster);
	void set_pad_devices(const std::array<int, input_slots::pad_count>& devices,
		const std::vector<Player*>& players, const std::vector<SeatSetup>& roster);

	// Every participant's badge, or one's, back to full emphasis.
	void emphasise();
	void emphasise(std::size_t participant);
	// Runs the emphasis down by a live tick.
	void decay(float dt);

	// Empty for a human seat; throws out_of_range for no such participant.
	std::wstring_view bot_identity(std::size_t participant) const;
	const PlayerBadge& badge() const;
	const std::wstring& identity_text(std::size_t participant) const;

	// `player` is participant's; `showing` is whether the match is in its
	// countdown or live play, the only time a badge is drawn.
	bool visible(const Player& player, bool showing) const;
	// As seen by the pad in viewer_slot; see Level::player_identity_emphasis_for.
	float emphasis(std::size_t participant, const Player& player, bool showing, int viewer_slot) const;
	mattmath::RectangleF bounds(std::size_t participant, const Player& player, bool showing,
		int viewer_slot) const;

	// The badge at `bounds`.
	void draw_badge(labrador::DrawList& draw_list, const mattmath::RectangleF& bounds,
		int participant) const;
	// Bot `bot`'s badge at `badge` and its label above, both shifted to stay
	// inside the pane's `visible` world rectangle, the label on the card's paper.
	void draw_bot(labrador::DrawList& draw_list, mattmath::RectangleF badge, int bot,
		const mattmath::RectangleF& visible, const PaperCard& paper) const;

private:
	int pad_device(int slot) const;

	const Presentation* presentation_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
	std::unique_ptr<PlayerBadge> badge_;
	// By participant: seconds of emphasis left.
	std::vector<float> remaining_;
	// By participant; null for a human. Immutable labels and metrics shared
	// safely by every human pane.
	std::vector<std::unique_ptr<labrador::Text>> bot_texts_;
	// By participant; see set_seat_labels. Present in every build, only ever
	// set by the Debug director, so empty in Release.
	std::vector<std::wstring> seat_labels_;
	// See set_pad_devices; -1 until set, which is the slot's own pad.
	std::array<int, input_slots::pad_count> pad_devices_ = { -1, -1, -1, -1 };
	// By participant: each HUD identity box's whole text. Built from the seat, the
	// player's team, pad (fixed unless a seat is reclaimed) and seat_labels_,
	// so only construction, set_seat_labels and set_pad_devices rebuild it;
	// draws only read it.
	std::vector<std::wstring> identity_texts_;
};
