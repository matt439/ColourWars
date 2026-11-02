#include "game/pch.h"
#include "game/states/match_suspension.h"
#include "scripts/acceptance/support/probe.h"
#include <cstdlib>
#include <iostream>

// HYG#41: GameLevel's suspension reasons and the answers its callbacks act on.
namespace
{
	using acceptance::require;

	constexpr SuspensionGate focused{ true, false, false };
	constexpr SuspensionGate unfocused{ false, false, false };
	constexpr SuspensionGate clock_paused{ true, true, false };
	constexpr SuspensionGate background{ false, false, true };
	constexpr float count = 2.0f;

	void check_independent_reasons()
	{
		MatchSuspension s;
		require(s.audio_may_play(focused) && s.live() && !s.music_ducked() &&
			!s.rumble_paused(focused) && !s.suspends_level(focused),
			"A fresh match is not playing");
		require(!s.audio_may_play(unfocused) && !s.audio_may_play(clock_paused) &&
			s.rumble_paused(clock_paused) && s.suspends_level(unfocused),
			"Focus or the debug clock did not hold the match");

		s.open_pause_menu();
		s.cover();
		require(!s.audio_may_play(focused) && !s.live() && s.music_ducked() &&
			s.ducked_music_plays(focused) && !s.ducked_music_plays(unfocused) &&
			!s.ducked_music_plays(clock_paused) && s.rumble_paused(focused),
			"The pause menu over the match did not duck its music and hold play");

		// A state above the pause menu: still covered, music still ducked.
		s.uncover();
		require(!s.music_ducked() && !s.audio_may_play(focused),
			"An uncovered open pause menu let audio play or kept music ducked");
		s.close_pause_menu();
		require(s.audio_may_play(focused) && s.live(), "Closing the pause menu left a reason behind");

		// Coverage without the pause menu (Results, a developer overlay).
		s.cover();
		require(!s.music_ducked() && !s.audio_may_play(focused) && s.suspends_level(focused),
			"Plain coverage ducked music or let audio play");
		s.uncover();
	}

	void check_focus_hold()
	{
		MatchSuspension s;
		require(!s.lose_focus(unfocused, true), "Focus loss without background mode kept running");
		s.regain_focus();
		require(s.take_focus_interruption() && !s.take_focus_interruption(),
			"A live round's focus loss was not owed exactly one count-in");

		require(!s.lose_focus(unfocused, false), "Background mode appeared from nowhere");
		require(!s.take_focus_interruption(), "A finished round was owed a count-in");

		// A covered match keeps the hold for the uncover.
		require(!s.lose_focus(unfocused, true), "Focus loss without background mode kept running");
		s.cover();
		s.regain_focus();
		require(s.covered(), "Regaining focus uncovered the match");
		s.uncover();
		require(s.take_focus_interruption(), "Uncovering lost the focus hold");

		// The pause menu stops background running.
		s.open_pause_menu();
		require(!s.lose_focus(background, true) && !s.may_run_unfocused(background),
			"Background mode ran a paused match");
		s.close_pause_menu();
		std::ignore = s.take_focus_interruption();
	}

	void check_count_in()
	{
		MatchSuspension s;
		std::ignore = s.lose_focus(unfocused, true);
		require(!s.begin_count_in(false, count) && !s.counting_in() && !s.take_focus_interruption(),
			"A count-in began for a round that is not active, or kept the hold");
		require(s.begin_count_in(true, count) && s.counting_in() && !s.live() && s.music_ducked() &&
			!s.audio_may_play(focused) && s.rumble_paused(focused),
			"The count-in did not hold play with ducked music");
		s.cover();
		require(!s.music_ducked(), "Covered count-in music stayed ducked");
		s.uncover();
		require(s.countdown().advance(count) && !s.counting_in() && s.audio_may_play(focused),
			"The completed count-in did not release play");
	}

	void check_background_mode()
	{
		MatchSuspension s;
		require(s.lose_focus(background, true) && s.may_run_unfocused(background) &&
			!s.suspends_level(background) && !s.take_focus_interruption(),
			"Background mode did not run the unfocused match without a hold");

		std::ignore = s.lose_focus(background, true);
		require(s.background_changed(unfocused, true) == MatchSuspension::BackgroundChange::stopped &&
			s.take_focus_interruption(),
			"Ending background mode did not suspend with a count-in owed");
		require(s.background_changed(unfocused, true) == MatchSuspension::BackgroundChange::none,
			"A background stop was reported twice");

		std::ignore = s.lose_focus(unfocused, true);
		require(s.background_changed(background, true) == MatchSuspension::BackgroundChange::continuing &&
			!s.take_focus_interruption(),
			"Enabling background mode kept an ordinary focus hold");

		std::ignore = s.lose_focus(background, true);
		s.cover();
		require(s.background_changed(unfocused, true) == MatchSuspension::BackgroundChange::none,
			"Coverage did not end background running");

		MatchSuspension reset;
		std::ignore = reset.lose_focus(unfocused, true);
		require(reset.begin_count_in(true, count), "The count-in did not begin");
		std::ignore = reset.lose_focus(unfocused, true);
		reset.reset();
		require(!reset.counting_in() && !reset.take_focus_interruption(),
			"A new level inherited a count-in or a focus hold");
	}
}

int main()
{
	return acceptance::run_probe("match suspension", [&]
	{
		check_independent_reasons();
		check_focus_hold();
		check_count_in();
		check_background_mode();
		std::cout << "PASS match suspension reasons, focus hold, count-in and background mode\n";
		return EXIT_SUCCESS;
	});
}
