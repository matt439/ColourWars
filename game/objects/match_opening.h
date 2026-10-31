#pragma once

#include "game/carried/sound_bank_object.h"
#include "game/objects/level_state.h"
#include "game/objects/level_end_info.h"
#include "game/objects/monitor.h"
#include "game/objects/silhouette.h"
#include "game/objects/screen_cursor.h"
#include "game/objects/figure_acting.h"
#include "game/objects/reveal_art.h"
#include "game/objects/player_team.h"
#include "game/ai/bot_random.h"
#include "engine/math/rectanglef.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace labrador
{
	class DrawList;
	class Renderer;
	class Scene;
	class ResolutionManager;
	class ViewportManager;
}
class Player;
class Presentation;
class MatchAudio;
struct LevelServices;
struct MatchTuning;
struct PlaySettings;
struct TeamColour;
struct MenuResultsText;

// What the opening reads of the match at one moment. The owning Level keeps
// the state, the intro countdown flag and every timer, and builds one of
// these for each call, so nothing here is a second copy of them.
struct OpeningClock
{
	LevelState state = LevelState::start_countdown;
	// The countdown plays in the picture and ends by pushing through the glass.
	bool intro_countdown = false;
	float start_timer = 0.0f;
	// How far the zoom out is, 0 to 1 (Level::zoom_out_camera_ratio).
	float zoom_ratio = 0.0f;
};

// The match's opening and its reveal (HYG#40, docs/backstory.md): the
// person's monitor, the person, the cursor, the colours tried and undone, the
// push through the glass, the pull back out at the end, the click on the
// winner and the figures' acting throughout. It exists only when the match
// has an intro. The owning Level keeps the match state and its timers, passes
// them in as an OpeningClock and applies each transition this returns, at the
// point it always wrote it. It borrows the match tuning and the presentation
// and reads them on every call, so a `tune` of `match.opening`,
// `match.reveal` or `opening` shows at once, and borrows MatchAudio for the
// room tone and the story voices; the hover, click and close cues are its own
// waves. Not copyable or movable: it is held by pointer and borrows into
// its owner.
class MatchOpening : public colourwars::SoundBankObject
{
public:
	// What an opening update asks of the Level.
	enum class Step
	{
		stay,
		// The split-screen countdown from its start: the intro was skipped.
		countdown,
		// The countdown in the picture, ending in the push through the glass.
		intro_countdown,
	};

	// Rolls the person from `persona`, then makes the monitor, the person,
	// the cursor, the acting, the coverage text and art and resolves the
	// hover, click and close waves, in that order; a quick rematch starts
	// with the colours being tried. Greys every figure. Throws what
	// SoundBankObject throws for a wave the bank lacks.
	MatchOpening(const LevelServices& services, const std::string& sound_bank_name,
		const TeamColour* team_colours, MatchAudio* audio, BotRandom persona, bool quick,
		const std::vector<Player*>& players);
	~MatchOpening();
	MatchOpening(const MatchOpening&) = delete;
	MatchOpening& operator=(const MatchOpening&) = delete;

	// What of the world the picture shows, and where its two halves meet.
	// Once, before the first frame.
	void set_picture(const mattmath::RectangleF& bounds, float middle);

	// The opening is on screen, or its countdown in the picture is.
	static bool intro_showing(const OpeningClock& clock);
	// The glass return, the overview and the results behind them.
	static bool reveal_showing(const OpeningClock& clock);

	// One stepping tick of the opening. A requested skip skips (see skip).
	Step update_opening(float dt, bool skip_requested, const std::vector<Player*>& players);
	// Stops the story voices and the room tone, plays the skip cue and puts
	// every figure in colour. The Level then goes straight to the
	// split-screen countdown, from its start if it had not begun.
	void skip(const std::vector<Player*>& players);
	// Time up: the person comes back. The coverage is counted once, by the
	// caller, and is what the click and the results agree on.
	void start_reveal(const LevelEndInfo& end_info);
	// One stepping tick of the reveal; true once the person has left.
	bool update_reveal(float dt, const std::vector<Player*>& players);
	// Chooses each figure's act for this moment and poses it. Every
	// stepping tick, after the state has advanced.
	void update_acting(float dt, const OpeningClock& clock, const std::vector<Player*>& players);

	// The intro's views of `scene`: the whole picture, then one per human
	// flying out to its pane as the view pushes through the glass.
	void add_intro_views(labrador::Scene& scene, const std::vector<int>& view_participants,
		const std::vector<Player*>& players, const OpeningClock& clock) const;
	// The monitor behind the scene, the figures' acting in each view, then
	// the reveal or the opening in front. `draw_countdown` draws the
	// countdown's number and cards where the countdown in the picture shows.
	void draw(labrador::Renderer& renderer, const labrador::Scene& scene,
		const OpeningClock& clock, const std::vector<Player*>& players,
		const std::function<void(labrador::DrawList&)>& draw_countdown) const;

private:
	struct OpeningBeat
	{
		bool previewing_a = false;
		bool previewing_b = false;
		bool picker = false;
		float cursor_at_a = 0.0f;
		float cursor_at_b = 0.0f;
		float cursor_back = 0.0f;
		SilhouettePose pose;
	};
	struct RevealBeat
	{
		float cursor_at_a = 0.0f;
		float cursor_at_b = 0.0f;
		bool showing_a = false;
		bool showing_b = false;
		bool picker = false;
		float waver = 0.0f;
		bool clicked = false;
		SilhouettePose pose;
	};

	OpeningBeat opening_beat() const;
	float opening_length() const;
	RevealBeat reveal_beat() const;
	float reveal_length() const;
	float push_ratio(const OpeningClock& clock) const;
	// How far the view is through the glass: 0 at the monitor, 1 in the panes.
	float glass_push(const OpeningClock& clock) const;
	mattmath::RectangleF intro_screen(const OpeningClock& clock) const;
	void colour_players(const std::vector<Player*>& players, bool team_a, bool team_b);
	void draw_reveal(labrador::DrawList& front, const mattmath::RectangleF& screen,
		const OpeningClock& clock) const;
	void draw_acting(labrador::DrawList& list, const std::vector<Player*>& players) const;

	// Borrowed; all outlive the level.
	const MatchTuning* match_ = nullptr;
	const Presentation* presentation_ = nullptr;
	const labrador::ResolutionManager* resolution_manager_ = nullptr;
	const labrador::ViewportManager* viewport_manager_ = nullptr;
	const PlaySettings* preferences_ = nullptr;
	// The owning Level's; fixed for the match.
	const TeamColour* team_colours_ = nullptr;
	// The room tone and the story voices.
	MatchAudio* audio_ = nullptr;

	std::unique_ptr<Monitor> monitor_;
	std::unique_ptr<Silhouette> silhouette_;
	std::unique_ptr<ScreenCursor> cursor_;
	std::unique_ptr<FigureActing> acting_;
	// Capture the wrappers alongside RevealArt's percent font, so replacement
	// presentation text never draws against the preceding font snapshot. A copy
	// held by pointer, so the menu text records stay out of this header.
	std::unique_ptr<const MenuResultsText> reveal_text_;
	std::unique_ptr<RevealArt> reveal_art_;
	labrador::SoundBank::WaveHandle hover_sound_;
	labrador::SoundBank::WaveHandle click_sound_;
	labrador::SoundBank::WaveHandle close_sound_;

	float opening_time_ = 0.0f;
	// By roster participant: what each figure is doing, and for how long.
	std::vector<FigureAct> acts_;
	float act_time_ = 0.0f;
	mattmath::RectangleF picture_bounds_;
	float picture_middle_ = 0.0f;

	float reveal_time_ = 0.0f;
	LevelEndInfo end_info_;
	PlayerTeam winner_ = PlayerTeam::none;
};
