#pragma once

#include "game/objects/countdown_art.h"
#include "game/carried/text_drop_shadow.h"
#include "game/input_bindings.h"
#include "game/input_slots.h"
#include "engine/math/rectanglef.h"
#include "engine/math/vector2f.h"
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace labrador
{
	class DrawList;
	class RenderResources;
	class ResolutionManager;
}
class Presentation;
class Player;
class PaperCard;
struct MatchTuning;

// The window-space cards around the start signal (HYG#40): the countdown card
// with its GET READY heading and its number, the stage title, one controls
// reminder per input device in the match, and the RESUMING and REPLAY cards.
// The owning Level still decides when each shows and computes the number's
// beat; this part keeps their text, lays them out and draws them. It borrows
// the match tuning, the presentation, the window and the effective input
// bindings, and copies only what it always copied when built: the reminders'
// look and the first label. Not copyable or movable: the reminder pointers
// point into it.
class CountdownCards
{
public:
	// Creates only the stage title's text, where the Level constructor made it.
	CountdownCards(const std::wstring& stage_name, const MatchTuning* match,
		const Presentation* presentation, const labrador::ResolutionManager* resolution_manager,
		labrador::RenderResources* render_resources, const InputBindings* input_bindings);
	CountdownCards(const CountdownCards&) = delete;
	CountdownCards& operator=(const CountdownCards&) = delete;

	// The art, the three headings and a reminder for each device the players
	// use, laid out for the current window. Once, before the first frame.
	void build(const std::vector<Player*>& players, const PaperCard& paper);

	// Sets the displayed beat; drawing derives its age from the match clock.
	void set_label(const std::wstring& text);
	// Rewords a reminder whose binding changed since it was last worded, and
	// re-lays the reminders out if one did. Every layout, before centre.
	void refresh_controls();
	// The countdown, the replay and the stage title cards, then the reminders.
	void centre(const PaperCard& paper);

	// The countdown's number, and while `getting_ready` its cards, headings
	// and reminders, with the filter changes the match has always drawn them with.
	void draw(labrador::DrawList& draw_list, const PaperCard& paper, bool getting_ready,
		float beat_age, bool reduce_effects) const;
	// The shared resume signal, `remaining` seconds from the end.
	void draw_resume(labrador::DrawList& list, const PaperCard& paper, float remaining,
		bool reduce_effects) const;
	void draw_replay_banner(labrador::DrawList& list, const PaperCard& paper) const;

	// The countdown card, each reminder's and the stage title's when it has
	// one, as of the last layout.
	std::vector<mattmath::RectangleF> cards() const;
	std::wstring_view stage_entry_name() const;
	const mattmath::RectangleF& stage_entry_card() const { return this->stage_entry_card_; }
	const mattmath::RectangleF& stage_entry_text_bounds() const { return this->stage_entry_text_bounds_; }
	// Empty, and zero bounds, when no player uses that device.
	std::wstring_view keyboard_controls_text() const;
	mattmath::RectangleF keyboard_controls_text_bounds() const;
	mattmath::RectangleF keyboard_controls_card_bounds() const;
	std::wstring_view gamepad_controls_text() const;
	mattmath::RectangleF gamepad_controls_text_bounds() const;
	mattmath::RectangleF gamepad_controls_card_bounds() const;

private:
	void centre_countdown_text(const PaperCard& paper);
	void centre_stage_entry(const PaperCard& paper);
	void centre_controls_text();

	const MatchTuning* match_ = nullptr;
	const Presentation* presentation_ = nullptr;
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	labrador::RenderResources* render_resources_ = nullptr;

	std::unique_ptr<CountdownArt> countdown_art_;
	std::wstring countdown_label_;
	mattmath::RectangleF countdown_number_box_;
	std::unique_ptr<colourwars::TextDropShadow> countdown_heading_ = nullptr;
	std::unique_ptr<colourwars::TextDropShadow> resume_heading_;
	std::unique_ptr<colourwars::TextDropShadow> replay_heading_;
	mattmath::RectangleF replay_card_;
	mattmath::RectangleF countdown_card_;
	std::unique_ptr<colourwars::TextDropShadow> stage_entry_;
	mattmath::RectangleF stage_entry_card_;
	mattmath::RectangleF stage_entry_text_bounds_;

	// One shared reminder per input device type present in the match.
	struct ControlsReminder
	{
		std::unique_ptr<colourwars::TextDropShadow> heading;
		std::unique_ptr<colourwars::TextDropShadow> text;
		mattmath::RectangleF card;
		mattmath::RectangleF text_bounds;
	};
	std::vector<ControlsReminder> controls_reminders_;
	// Save owns the effective binding; a paused draw refreshes only its cached
	// words and fitting, retaining the text widget's original font and copy.
	const InputBindings* input_bindings_ = nullptr;
	colourwars::TextDropShadow* keyboard_controls_ = nullptr;
	std::wstring keyboard_controls_words_;
	colourwars::TextDropShadow* gamepad_controls_ = nullptr;
	std::vector<int> gamepad_slots_;
	std::array<PadProfile, input_slots::pad_count> gamepad_pads_{};
	struct ControlsReminderLook
	{
		mattmath::Vector2F reference_size, margin, padding, heading_shadow, text_shadow;
		float gap = 0.0f, y = 0.0f;
	} controls_look_;
};
