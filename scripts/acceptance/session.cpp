#include "game/pch.h"
#include "game/match/session.h"
#include "game/config/presentation/presentation.h"
#include "game/objects/level_end_info.h"
#include "engine/core/file_path.h"
#include "scripts/acceptance/support/probe.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

// The shipping Session: which continuation and result the end page offers for
// each kind of finish, the series score's authored tokens, and the ledger
// next_session hands the next match. Every check here is shipping code, so the
// case runs in both presets; it was Debug-only inside debug_console (HYG#60).

using namespace labrador;

namespace
{
    using acceptance::require;

    void check_end_text(const Presentation& presentation)
    {
        // The end page offers the next playable action for each kind of finish.
        const MenuResultsText original_results = presentation.menu_text().results;
        LevelEndInfo win;
        win.team_a_area = 1.0f;
        MenuEndText custom = presentation.menu_text().end;
        custom.rematch = L"Single {again}";
        custom.play_series_again = L"Series {again}";
        custom.replay_draw = L"Draw {again}";
        custom.next_round = L"Round {next}";
        const auto check_continuation = [&](const Session& session, const wchar_t* expected)
        {
            const bool series = session.is_series(), complete = session.complete();
            const int round = session.round(), a_wins = session.team_a_wins(), b_wins = session.team_b_wins();
            const std::wstring score = session.score_string(original_results), result = session.result_string(original_results);
            std::wstring selected = session.continue_string(custom);
            require(selected == expected, "Session selects the supplied continuation without formatting literal braces");
            selected.clear();
            require(session.continue_string(custom) == expected && session.is_series() == series &&
                session.complete() == complete && session.round() == round && session.team_a_wins() == a_wins &&
                session.team_b_wins() == b_wins && session.score_string(original_results) == score && session.result_string(original_results) == result,
                "Continuation selection returns an owned copy and leaves the Session ledger unchanged");
        };
        for (const bool solo : { false, true })
        {
            Session single;
            LevelEndInfo finish = win;
            finish.team_b_played = !solo;
            single.finish_round(finish);
            require(single.complete() && single.continue_string(presentation.menu_text().end) == L"Rematch",
                "A finished single round or solo practice offers Rematch");
            check_continuation(single, L"Single {again}");
        }
        for (const int length : { 3, 5 })
        {
            Session series(length);
            series.finish_round(LevelEndInfo{});
            require(!series.complete() && series.continue_string(presentation.menu_text().end) == L"Replay Draw",
                "An unresolved series draw offers Replay Draw");
            check_continuation(series, L"Draw {again}");
            series.next_round();
            for (int wins = 1; wins <= length / 2 + 1; ++wins)
            {
                series.finish_round(win);
                if (series.complete())
                {
                    require(series.continue_string(presentation.menu_text().end) == L"Play Series Again",
                        "A completed best-of series offers Play Series Again");
                    check_continuation(series, L"Series {again}");
                }
                else
                {
                    require(series.continue_string(presentation.menu_text().end) == L"Next Round",
                        "A decisive unfinished series offers Next Round");
                    check_continuation(series, L"Round {next}");
                    series.next_round();
                }
            }
        }
        require(custom.rematch == L"Single {again}" && custom.play_series_again == L"Series {again}" &&
            custom.replay_draw == L"Draw {again}" && custom.next_round == L"Round {next}",
            "Session selection leaves supplied continuation text unchanged");
        MenuResultsText custom_results = original_results;
        custom_results.series_team_a_wins = L"Alpha {series}";
        custom_results.series_team_b_wins = L"Beta {series}";
        custom_results.draw_no_win = L"Draw {award}";
        custom_results.series_continues = L"Continue {series}";
        const auto check_result = [&](const Session& session, const wchar_t* expected_default, const wchar_t* expected_custom)
        {
            const bool series = session.is_series(), complete = session.complete();
            const int round = session.round(), a_wins = session.team_a_wins(), b_wins = session.team_b_wins();
            const std::wstring score = session.score_string(original_results), continuation = session.continue_string(custom);
            std::wstring selected = session.result_string(custom_results);
            require(selected == expected_custom && session.result_string(original_results) == expected_default,
                "Session selects the exact default or supplied result while retaining literal braces");
            selected.clear();
            require(session.result_string(custom_results) == expected_custom && session.is_series() == series &&
                session.complete() == complete && session.round() == round && session.team_a_wins() == a_wins &&
                session.team_b_wins() == b_wins && session.score_string(original_results) == score &&
                session.continue_string(custom) == continuation && session.result_string(original_results) == expected_default,
                "Result selection returns an owned copy without changing the Session ledger, score or continuation");
        };
        LevelEndInfo win_b = win;
        win_b.team_a_area = 0.0f; win_b.team_b_area = 1.0f;
        LevelEndInfo draw = win;
        draw.team_a_area = draw.team_b_area = 0.5f;
        for (const int length : { 1, 3, 5 })
        {
            Session fresh(length);
            check_result(fresh, L"Draw - no win awarded", L"Draw {award}");
        }
        for (const bool team_b : { false, true })
        {
            const LevelEndInfo decisive = team_b ? win_b : win;
            Session single; single.finish_round(decisive);
            check_result(single, team_b ? L"Team B wins the series!" : L"Team A wins the series!",
                team_b ? L"Beta {series}" : L"Alpha {series}");
            // Completion takes precedence over the last round: solo finishes
            // award no win and retain the historical tied-ledger B result.
            LevelEndInfo solo = decisive;
            solo.team_a_played = !team_b; solo.team_b_played = team_b;
            Session practice; practice.finish_round(solo);
            check_result(practice, L"Team B wins the series!", L"Beta {series}");
        }
        Session single_draw; single_draw.finish_round(draw);
        check_result(single_draw, L"Team B wins the series!", L"Beta {series}");
        for (const int length : { 3, 5 })
        {
            Session ongoing(length); ongoing.finish_round(win);
            check_result(ongoing, L"Series continues", L"Continue {series}");
            ongoing.next_round(); ongoing.finish_round(draw);
            check_result(ongoing, L"Draw - no win awarded", L"Draw {award}");
            ongoing.next_round(); ongoing.finish_round(win_b);
            check_result(ongoing, L"Series continues", L"Continue {series}");
            for (const bool team_b : { false, true })
            {
                Session completed(length);
                for (int wins = 0; wins < length / 2 + 1; ++wins)
                {
                    completed.finish_round(team_b ? win_b : win);
                    if (!completed.complete()) { completed.next_round(); }
                }
                check_result(completed, team_b ? L"Team B wins the series!" : L"Team A wins the series!",
                    team_b ? L"Beta {series}" : L"Alpha {series}");
            }
        }
        require(custom_results.series_team_a_wins == L"Alpha {series}" &&
            custom_results.series_team_b_wins == L"Beta {series}" && custom_results.draw_no_win == L"Draw {award}" &&
            custom_results.series_continues == L"Continue {series}",
            "Session result selection leaves supplied text unchanged, including completed single-round ties");
        struct ScoreCase
        {
            int a_wins, b_wins, target;
            const wchar_t* original;
            const wchar_t* reordered;
        };
        const std::array<ScoreCase, 26> score_cases{ {
            { 0, 0, 1, L"Wins: A 0 - B 0  |  First to 1", L"Goal 1 | B 0, A 0" },
            { 0, 1, 1, L"Wins: A 0 - B 1  |  First to 1", L"Goal 1 | B 1, A 0" },
            { 1, 0, 1, L"Wins: A 1 - B 0  |  First to 1", L"Goal 1 | B 0, A 1" },
            { 0, 0, 2, L"Wins: A 0 - B 0  |  First to 2", L"Goal 2 | B 0, A 0" },
            { 0, 1, 2, L"Wins: A 0 - B 1  |  First to 2", L"Goal 2 | B 1, A 0" },
            { 0, 2, 2, L"Wins: A 0 - B 2  |  First to 2", L"Goal 2 | B 2, A 0" },
            { 1, 0, 2, L"Wins: A 1 - B 0  |  First to 2", L"Goal 2 | B 0, A 1" },
            { 1, 1, 2, L"Wins: A 1 - B 1  |  First to 2", L"Goal 2 | B 1, A 1" },
            { 1, 2, 2, L"Wins: A 1 - B 2  |  First to 2", L"Goal 2 | B 2, A 1" },
            { 2, 0, 2, L"Wins: A 2 - B 0  |  First to 2", L"Goal 2 | B 0, A 2" },
            { 2, 1, 2, L"Wins: A 2 - B 1  |  First to 2", L"Goal 2 | B 1, A 2" },
            { 0, 0, 3, L"Wins: A 0 - B 0  |  First to 3", L"Goal 3 | B 0, A 0" },
            { 0, 1, 3, L"Wins: A 0 - B 1  |  First to 3", L"Goal 3 | B 1, A 0" },
            { 0, 2, 3, L"Wins: A 0 - B 2  |  First to 3", L"Goal 3 | B 2, A 0" },
            { 0, 3, 3, L"Wins: A 0 - B 3  |  First to 3", L"Goal 3 | B 3, A 0" },
            { 1, 0, 3, L"Wins: A 1 - B 0  |  First to 3", L"Goal 3 | B 0, A 1" },
            { 1, 1, 3, L"Wins: A 1 - B 1  |  First to 3", L"Goal 3 | B 1, A 1" },
            { 1, 2, 3, L"Wins: A 1 - B 2  |  First to 3", L"Goal 3 | B 2, A 1" },
            { 1, 3, 3, L"Wins: A 1 - B 3  |  First to 3", L"Goal 3 | B 3, A 1" },
            { 2, 0, 3, L"Wins: A 2 - B 0  |  First to 3", L"Goal 3 | B 0, A 2" },
            { 2, 1, 3, L"Wins: A 2 - B 1  |  First to 3", L"Goal 3 | B 1, A 2" },
            { 2, 2, 3, L"Wins: A 2 - B 2  |  First to 3", L"Goal 3 | B 2, A 2" },
            { 2, 3, 3, L"Wins: A 2 - B 3  |  First to 3", L"Goal 3 | B 3, A 2" },
            { 3, 0, 3, L"Wins: A 3 - B 0  |  First to 3", L"Goal 3 | B 0, A 3" },
            { 3, 1, 3, L"Wins: A 3 - B 1  |  First to 3", L"Goal 3 | B 1, A 3" },
            { 3, 2, 3, L"Wins: A 3 - B 2  |  First to 3", L"Goal 3 | B 2, A 3" } } };
        MenuResultsText custom_score = original_results;
        custom_score.series_score = L"Goal {target} | B {team_b_wins}, A {team_a_wins}";
        for (const ScoreCase& fixture : score_cases)
        {
            Session scored(fixture.target * 2 - 1);
            int awarded = 0;
            const auto add_wins = [&](const LevelEndInfo& result, int count)
            {
                for (int index = 0; index < count; ++index)
                {
                    require(!scored.complete(), "Score fixture must not award a round after completion");
                    if (awarded != 0) { scored.next_round(); }
                    scored.finish_round(result); ++awarded;
                }
            };
            // A completed team's final win must follow every opposing win.
            if (fixture.a_wins == fixture.target)
            {
                add_wins(win_b, fixture.b_wins); add_wins(win, fixture.a_wins);
            }
            else
            {
                add_wins(win, fixture.a_wins); add_wins(win_b, fixture.b_wins);
            }
            require(scored.team_a_wins() == fixture.a_wins && scored.team_b_wins() == fixture.b_wins &&
                scored.is_series() == (fixture.target > 1) &&
                scored.complete() == (fixture.a_wins == fixture.target || fixture.b_wins == fixture.target),
                "Score fixture did not reach its literal ledger through legal Session rounds");
            const int round = scored.round(); const bool complete = scored.complete(), series = scored.is_series();
            const std::wstring result = scored.result_string(original_results), continuation = scored.continue_string(custom);
            std::wstring score = scored.score_string(custom_score);
            require(scored.score_string(original_results) == fixture.original && score == fixture.reordered,
                "Session score changed exact legacy composition or reordered authored tokens for a reachable ledger");
            score.clear();
            require(scored.score_string(custom_score) == fixture.reordered && scored.round() == round &&
                scored.complete() == complete && scored.is_series() == series && scored.team_a_wins() == fixture.a_wins &&
                scored.team_b_wins() == fixture.b_wins && scored.result_string(original_results) == result &&
                scored.continue_string(custom) == continuation,
                "Session score returns an owned copy and leaves the ledger, result and continuation unchanged");
        }
        require(custom_score.series_score == L"Goal {target} | B {team_b_wins}, A {team_a_wins}" &&
            original_results.series_score == L"Wins: A {team_a_wins} - B {team_b_wins}  |  First to {target}",
            "Session scoring changed its supplied or captured source template");
    }

    // C6 (DES#3): next_session is the ledger GameLevel::on_end_menu_closed
    // builds the next match against. Next Round and Replay Draw keep the
    // ledger; once the series is complete, Play Series Again (and Rematch)
    // start afresh at the chosen length. Pause > Restart does not call it, and
    // painter_visual's bot_runtime_checks covers that side on a real GameLevel.
    void check_session_continuation(const Presentation& presentation)
    {
        const MenuResultsText& results = presentation.menu_text().results;
        LevelEndInfo win_a;
        win_a.team_a_area = 1.0f;
        LevelEndInfo win_b;
        win_b.team_b_area = 1.0f;
        LevelEndInfo draw;
        draw.team_a_area = draw.team_b_area = 0.5f;
        const auto ledger = [](const Session& session, int round, int a_wins, int b_wins)
            {
                return session.round() == round && session.team_a_wins() == a_wins &&
                    session.team_b_wins() == b_wins;
            };

        for (const int length : { 3, 5 })
        {
            const int target = length / 2 + 1;
            Session series(length);
            series.finish_round(win_a);
            require(ledger(series, 1, 1, 0) && !series.complete(), "Fixture: Team A takes round 1");

            // Next Round keeps the ledger and opens round 2.
            Session playing = next_session(series, length);
            require(ledger(playing, 2, 1, 0) && playing.is_series() && !playing.complete() &&
                playing.round_winner() == PlayerTeam::none,
                "Next Round keeps the series ledger and moves to round 2");
            require(ledger(series, 1, 1, 0), "Next Round leaves the ledger it was given alone");
            playing.finish_round(win_b);
            require(ledger(playing, 2, 1, 1), "Round 2 counts on the kept ledger");

            // A drawn round is replayed on the next round, ledger unchanged.
            Session drawn = next_session(playing, length);
            drawn.finish_round(draw);
            const Session replay_draw = next_session(drawn, length);
            require(ledger(replay_draw, 4, 1, 1) && !replay_draw.complete(),
                "Replay Draw keeps the ledger");

            // An unfinished round has no continuation.
            bool refused = false;
            try { (void)next_session(replay_draw, length); }
            catch (const std::logic_error&) { refused = true; }
            require(refused, "An unfinished round offers no next session");

            // Play to completion, then Play Series Again.
            Session finishing = replay_draw;
            while (!finishing.complete())
            {
                finishing.finish_round(win_a);
                if (!finishing.complete()) { finishing = next_session(finishing, length); }
            }
            require(finishing.complete() && finishing.team_a_wins() == target && finishing.team_b_wins() == 1 &&
                finishing.round() > 1, "Fixture: Team A wins the series");
            const Session again = next_session(finishing, length);
            require(ledger(again, 1, 0, 0) && again.is_series() && !again.complete() &&
                again.round_winner() == PlayerTeam::none,
                "Play Series Again starts a fresh series at round 1 with no wins");
            require(again.score_string(results) == Session(length).score_string(results),
                "Play Series Again keeps the series length");
            require(finishing.complete() && finishing.team_a_wins() == target,
                "Play Series Again leaves the finished ledger it was given alone");
            Session fresh = again;
            fresh.finish_round(win_b);
            require(ledger(fresh, 1, 0, 1) && !fresh.complete(),
                "The fresh series counts its first round");
        }

        // A single match: Rematch is a fresh one-round ledger.
        Session single;
        single.finish_round(win_b);
        require(single.complete() && ledger(single, 1, 0, 1), "Fixture: Team B wins the single match");
        Session rematch = next_session(single, 1);
        require(ledger(rematch, 1, 0, 0) && !rematch.complete() && !rematch.is_series(),
            "Rematch starts a fresh single match");
        rematch.finish_round(win_a);
        require(rematch.complete() && ledger(rematch, 1, 1, 0), "The rematch is scored afresh");
    }
}

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("session", [&]
    {
        require(argc == 2, "usage: session <content>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const Presentation presentation(path_to_utf8(content / "presentation.json").c_str());
        check_end_text(presentation);
        check_session_continuation(presentation);
        std::cout << "PASS session: continuation, result and score text for single, solo, drawn and "
            "best-of finishes, with supplied text left unchanged; Next Round, Replay Draw, Play Series Again "
            "and Rematch ledgers\n";
        return 0;
    });
}
