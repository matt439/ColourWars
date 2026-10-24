#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "engine/app/application.h"
#include "game/debug/debug_perf.h"
#include "game/objects/debug_text.h"
#include <cmath>
#include <limits>

// The perf ring: frame-rate snapshot, overlay statistics and bounded capture.
// The perf.capture command grammar is checked through the director in
// session_checks.cpp (check_polish).
namespace debug_console_acceptance
{
	namespace
	{
		labrador::FrameTimings completed_frame(std::uint64_t id, int begin = 1, int submit = 2, int end = 3)
		{
			labrador::FrameTimings frame;
			frame.frame = id;
			// A zero engine interval is valid on reset frames. Its phases
			// still exist and must not replace the local Level/QPC stopwatches.
			frame.interval = std::chrono::nanoseconds::zero();
			frame.begin_frame_time = std::chrono::milliseconds(begin);
			frame.submit_time = std::chrono::milliseconds(submit);
			frame.end_frame_time = std::chrono::milliseconds(end);
			frame.update_time = std::chrono::milliseconds(999);
			frame.draw_time = std::chrono::milliseconds(888);
			return frame;
		}

		void prime_pending(DebugPerf& perf, std::size_t target)
		{
			require(perf.start_capture(target), "Start an Application-frame pairing capture");
			perf.begin_draw(100.0, completed_frame(10));
			perf.end_draw(102.0);
			perf.add_update(0.75);
			perf.begin_draw(116.0, completed_frame(11));
			perf.end_draw(118.0);
			require(perf.size() == 1 && perf.at(0).application_frame == 0 &&
				perf.capture_active() && !perf.capture_ready() && perf.captured_frames().size() == 1,
				"A committed sample remains pending until its exact Application frame completes");
		}

		void check_perf_engine_frames()
		{
			DebugPerf perf;
			require(perf.start_capture(1), "Start from Application frame zero");
			perf.begin_draw(0.0, completed_frame(0, 0, 0, 0)); perf.end_draw(1.0);
			perf.observe_frame_timings(completed_frame(0, 0, 0, 0));
			perf.begin_draw(16.0, completed_frame(1, 0, 0, 0)); perf.end_draw(17.0);
			perf.observe_frame_timings(completed_frame(1, 0, 0, 0));
			require(perf.capture_active() && perf.at(0).application_frame == 0 && perf.engine_samples() == 0,
				"Frame zero and a still-in-flight predecessor do not invent engine samples");
			perf.observe_frame_timings(completed_frame(2, 0, 0, 0));
			require(perf.capture_complete() && perf.captured_frames()[0].application_frame == 2 && perf.engine_samples() == 1,
				"The first real completed frame can carry legitimate zero phases after a priming draw");
			perf.reset();
			prime_pending(perf, 1);
			perf.observe_frame_timings(completed_frame(11));
			perf.observe_frame_timings(completed_frame(11));
			require(perf.capture_active() && perf.engine_samples() == 0 &&
				perf.captured_frames().size() == 1 && perf.at(0).application_frame == 0,
				"Repeated catch-up observations of the predecessor neither consume nor append a row");
			perf.observe_frame_timings(completed_frame(12, 4, 50, 60));
			require(perf.capture_ready() && perf.capture_complete() && !perf.capture_active() &&
				perf.captured_frames().size() == 1 && perf.captured_frames()[0].application_frame == 12 &&
				perf.at(0).begin_frame_ms == 4.0 && perf.at(0).submit_ms == 50.0 && perf.at(0).end_frame_ms == 60.0 &&
				perf.at(0).frame_ms == 16.0 && perf.at(0).update_ms == 0.75 && perf.at(0).draw_ms == 2.0,
				"The last requested sample completes from an update observation without an extra draw, using its own ID and phases");
			perf.observe_frame_timings(completed_frame(12, 90, 91, 92));
			perf.break_interval();
			require(perf.capture_complete() && perf.captured_frames()[0].submit_ms == 50.0 &&
				perf.captured_frames()[0].application_frame == 12,
				"A duplicate snapshot is idempotent and a completed final sample survives an immediate menu/focus/capture break");
			perf.discard_capture();
			perf.begin_draw(200.0, completed_frame(20)); perf.end_draw(201.0);
			perf.begin_draw(216.0, completed_frame(21)); perf.end_draw(218.0);
			perf.observe_frame_timings(completed_frame(22, 0, 0, 0));
			require(perf.engine_samples() == 2 && perf.at(0).application_frame == 22 &&
				perf.begin_frame().min == 0.0 && perf.begin_frame().max == 4.0 && perf.begin_frame().avg == 2.0 &&
				perf.submit().min == 0.0 && perf.submit().max == 50.0 && perf.submit().avg == 25.0 &&
				perf.end_frame().min == 0.0 && perf.end_frame().max == 60.0 && perf.end_frame().avg == 30.0,
				"Real zero phases and zero engine intervals remain valid; unavailable samples do not bias CPU phase statistics");

			// The same value-fed break operation used by the director's
			// match_suspended callback must discriminate publication from flight.
			perf.reset(); prime_pending(perf, 1);
			perf.break_interval(completed_frame(12, 7, 8, 9));
			require(perf.capture_complete() && perf.capture_ready() && !perf.capture_active() &&
				perf.captured_frames().size() == 1 && perf.captured_frames()[0].application_frame == 12 &&
				perf.captured_frames()[0].begin_frame_ms == 7.0 && perf.captured_frames()[0].submit_ms == 8.0 &&
				perf.captured_frames()[0].end_frame_ms == 9.0 && perf.current_frame_ms() == 0.0,
				"Suspension before another update preserves target1 when its exact final Application frame was already published");
			perf.reset(); prime_pending(perf, 1);
			perf.break_interval(completed_frame(11, 7, 8, 9));
			perf.observe_frame_timings(completed_frame(12, 7, 8, 9));
			require(perf.capture_ready() && !perf.capture_complete() && perf.capture_reason() == "interrupted" &&
				perf.captured_frames().size() == 1 && perf.captured_frames()[0].application_frame == 0 &&
				perf.engine_samples() == 0 && perf.current_frame_ms() == 0.0,
				"Suspension while target1 is still in flight interrupts and clears its join; later publication cannot revive it");

			// A pending final sample must not survive cancellation and attach
			// later to a different match/capture with the same numeric ID.
			perf.reset(); prime_pending(perf, 1);
			perf.observe_frame_timings(completed_frame(11));
			perf.break_interval();
			perf.observe_frame_timings(completed_frame(12, 7, 8, 9));
			require(perf.capture_ready() && !perf.capture_complete() && perf.capture_reason() == "interrupted" &&
				perf.captured_frames().size() == 1 && perf.captured_frames()[0].application_frame == 0 && perf.engine_samples() == 0,
				"Breaking before publication interrupts and clears the pending join instead of filling it from a later observation");

			for (int invalid = 0; invalid < 6; ++invalid)
			{
				perf.reset(); prime_pending(perf, 1);
				auto snapshot = completed_frame(invalid == 0 ? 13 : invalid == 1 ? 10 : invalid == 2 ? 0 : 12);
				if (invalid == 3) { snapshot.begin_frame_time = std::chrono::nanoseconds(-1); }
				if (invalid == 4) { snapshot.submit_time = std::chrono::nanoseconds(-1); }
				if (invalid == 5) { snapshot.end_frame_time = std::chrono::nanoseconds(-1); }
				perf.observe_frame_timings(snapshot);
				require(perf.capture_ready() && !perf.capture_complete() && perf.at(0).application_frame == 0 &&
					perf.engine_samples() == 0 && perf.captured_frames().size() == 1,
					"Missed/regressed/frame-zero or negative exact snapshots abort rather than borrow another frame or fill false phase zeros");
			}
			for (const std::uint64_t id : { std::uint64_t{11}, std::uint64_t{13}, std::uint64_t{10} })
			{
				perf.reset(); prime_pending(perf, 1);
				perf.begin_draw(132.0, completed_frame(id));
				perf.end_draw(134.0);
				require(perf.capture_ready() && !perf.capture_complete() && perf.size() == 1 &&
					perf.captured_frames()[0].application_frame == 0,
					"A duplicate, missed or regressed Application frame at draw begin breaks the interval without appending an extra target row");
			}
			for (const double bad_time : { -1.0, 115.0, 116.0,
				(std::numeric_limits<double>::quiet_NaN)(), (std::numeric_limits<double>::infinity)(),
				-(std::numeric_limits<double>::infinity)() })
			{
				perf.reset(); prime_pending(perf, 1);
				perf.begin_draw(bad_time, completed_frame(12));
				perf.end_draw(134.0);
				require(perf.capture_ready() && !perf.capture_complete() && perf.at(0).application_frame == 0,
					"Invalid runtime QPC beginnings cancel a pending pair instead of exporting a completed capture");
			}
			perf.reset(); prime_pending(perf, 2);
			perf.begin_draw(132.0, completed_frame(12, 4, 5, 6));
			perf.end_draw((std::numeric_limits<double>::quiet_NaN)());
			require(perf.capture_ready() && !perf.capture_complete() && perf.captured_frames().size() == 1 &&
				perf.captured_frames()[0].application_frame == 12 && perf.captured_frames()[0].submit_ms == 5.0,
				"An invalid later draw end interrupts but preserves the trustworthy previously completed sample");
			perf.reset(); prime_pending(perf, 1); perf.reset();
			perf.observe_frame_timings(completed_frame(12));
			require(perf.size() == 0 && !perf.capture_ready() && perf.captured_frames().empty(),
				"Reset clears pending IDs so a late completed snapshot cannot revive a discarded recording");
			perf.reset();
			require(perf.start_capture(1), "Start counter-overflow guard");
			perf.begin_draw(1.0, completed_frame((std::numeric_limits<std::uint64_t>::max)()));
			perf.end_draw(2.0);
			require(perf.capture_ready() && !perf.capture_complete() && perf.size() == 0,
				"The maximum Application frame cannot wrap its predicted ID to unavailable zero");

			// Runtime pairing must update the correct ring slot and chronological
			// capture copy after the overlay wraps, including the final target.
			perf.reset();
			const int total = static_cast<int>(DebugPerf::capacity) + 7;
			require(perf.start_capture(static_cast<std::size_t>(total)), "Start a paired capture longer than the ring");
			perf.begin_draw(0.0, completed_frame(20)); perf.end_draw(1.0);
			for (int i = 1; i <= total; ++i)
			{
				perf.begin_draw(16.0 * i, completed_frame(20 + static_cast<std::uint64_t>(i)));
				perf.end_draw(16.0 * i + 1.0);
				require(perf.capture_active() && perf.captured_frames().size() == static_cast<std::size_t>(i),
					"The target stays pending without overrunning until its exact engine snapshot arrives");
				perf.observe_frame_timings(completed_frame(21 + static_cast<std::uint64_t>(i), i, i * 2, 0));
			}
			require(perf.capture_complete() && perf.captured_frames().size() == static_cast<std::size_t>(total) &&
				perf.captured_frames().front().application_frame == 22 &&
				perf.captured_frames().back().application_frame == 21 + static_cast<std::uint64_t>(total) &&
				perf.engine_samples() == DebugPerf::capacity && perf.begin_frame().min == 8.0 &&
				perf.begin_frame().max == total && perf.begin_frame().avg == 127.5,
				"Exact phase data survives ring wrap in both history and the independent full capture");
		}
	}

	void check_perf_frame_rate()
	{
		DebugPerf perf;
		require(perf.current_frame_ms() == 0.0, "No render interval exists before the first draw");
		perf.begin_draw(1000.0);
		require(perf.current_frame_ms() == 0.0, "The first draw cannot invent a render interval");
		perf.end_draw(1002.0);
		require(perf.current_frame_ms() == 0.0, "A completed priming draw has no current snapshot");
		double begin = 1000.0;
		for (const auto& [fps, updates] : { std::pair{ 30, 0 }, std::pair{ 60, 1 }, std::pair{ 120, 4 } })
		{
			for (int update = 0; update < updates; ++update) { perf.add_update(0.25); }
			const double interval = 1000.0 / fps;
			begin += interval;
			perf.begin_draw(begin);
			const double snapshot = perf.current_frame_ms();
			require(std::abs(snapshot - interval) < 1.0e-9 && perf.current_frame_ms() == snapshot &&
				DebugText::frame_rate_text(snapshot) == L"render fps: " + std::to_wstring(fps) + L".0",
				"Render FPS follows measured draw cadence independently of zero, one or four fixed simulation updates");
			perf.end_draw(begin + 0.5);
			require(perf.current_frame_ms() == 0.0 && perf.at(0).updates == updates,
				"Ending a draw clears its current snapshot without changing the simulation update count");
		}
		const std::size_t samples = perf.size();
		perf.break_interval();
		require(samples == 3 && perf.size() == samples && perf.current_frame_ms() == 0.0,
			"A timing break clears the current FPS without reading stale samples from the retained ring");
		perf.begin_draw(9000.0);
		require(perf.current_frame_ms() == 0.0,
			"The first draw after a timing gap shows unavailable instead of a pre-gap average");
		perf.end_draw(9002.0);
		perf.begin_draw(9016.0);
		require(perf.current_frame_ms() == 16.0, "A second post-gap draw restores its fresh interval");
		perf.begin_draw(8999.0);
		require(perf.current_frame_ms() == 0.0, "A backwards timestamp invalidates the current FPS snapshot");
		perf.begin_draw(10000.0);
		perf.end_draw(10001.0);
		perf.begin_draw(10016.0);
		perf.end_draw((std::numeric_limits<double>::quiet_NaN)());
		require(perf.current_frame_ms() == 0.0, "An invalid draw end clears its FPS snapshot");
		perf.reset();
		require(perf.current_frame_ms() == 0.0, "Reset leaves no current FPS snapshot");

		for (const double invalid : { 0.0, -1.0, (std::numeric_limits<double>::quiet_NaN)(),
			(std::numeric_limits<double>::infinity)(), -(std::numeric_limits<double>::infinity)(),
			(std::numeric_limits<double>::denorm_min)() })
		{
			require(DebugText::frame_rate_text(invalid) == L"render fps: n/a",
				"Missing, invalid or reciprocally overflowing timing never formats a false FPS");
		}
		require(DebugText::frame_rate_text(1000.0 / 30.0) == L"render fps: 30.0" &&
			DebugText::frame_rate_text(1000.0 / 60.0) == L"render fps: 60.0" &&
			DebugText::frame_rate_text(1000.0 / 120.0) == L"render fps: 120.0" &&
			DebugText::frame_rate_text((std::numeric_limits<double>::max)()) == L"render fps: 0.0",
			"The readout formats measured FPS, including an extreme finite interval, to one decimal place");
	}

	void check_perf()
	{
		check_perf_engine_frames();
		check_perf_frame_rate();
		DebugPerf perf;
		require(perf.size() == 0 && perf.frame().max == 0.0, "Empty");
		for (int i = 0; i < 300; ++i)
		{
			const double begin = 16.0 * i;
			perf.add_update(0.5);
			perf.add_update(i % 2 == 0 ? 0.25 : 0.75);
			perf.begin_draw(begin);
			perf.end_draw(begin + 2.0 + (i % 3));
		}
		require(perf.size() == DebugPerf::capacity, "The ring keeps 240 frames");
		require(perf.at(0).draw_ms == 2.0 + (299 % 3) && perf.at(1).draw_ms == 2.0 + (298 % 3),
			"at(0) is the newest");
		require(perf.frame().min == 16.0 && perf.frame().max == 16.0 && perf.frame().avg == 16.0,
			"frame_ms is the interval between draws");
		require(perf.draw().min == 2.0 && perf.draw().max == 4.0 && perf.draw().avg == 3.0,
			"draw min / avg / max are exact");
		require(perf.update().min == 0.75 && perf.update().max == 1.25 && perf.update().avg == 1.0 &&
			perf.at(0).updates == 2, "update sums each frame's updates");
		perf.set_counts(182, 12);
		const std::vector<std::string> lines = perf.lines();
		require(lines.size() == 8 && lines[0] == "CPU wall ms min    avg    max  (240 frames)" &&
			lines[1] == "frame   16.00  16.00  16.00" &&
			lines[2] == "update   0.75   1.00   1.25  x2.00 a frame" &&
			lines[3] == "draw     2.00   3.00   4.00" && lines[4] == "begin  n/a (0 engine frames)" &&
			lines[5] == "submit  n/a" && lines[6] == "end  n/a" && lines[7] == "objects 182  projectiles 12",
			"perf lines, got '" + lines[2] + "'");
		perf.end_draw(9999.0);
		require(perf.size() == DebugPerf::capacity && perf.at(0).draw_ms == 2.0 + (299 % 3),
			"An end without a begin (a screenshot's redraw) adds nothing");
		perf.reset();
		require(perf.size() == 0, "reset");

		// The first draw after a reset has no interval: it commits nothing, so
		// min is never a fake 0 straight after perf.reset.
		perf.add_update(9.0);
		perf.begin_draw(1000.0);
		perf.end_draw(1001.0);
		require(perf.size() == 0, "The first draw after reset only starts an interval");
		perf.add_update(0.5);
		perf.begin_draw(1016.0);
		perf.end_draw(1018.0);
		require(perf.size() == 1 && perf.frame().min == 16.0 && perf.frame().max == 16.0 &&
			perf.update().max == 0.5 && perf.at(0).updates == 1,
			"The second draw is the first frame, without the first's updates");

		// A gap (a build, the menus, a page over the match, a capture): the
		// next draw starts again instead of recording the gap as one frame.
		perf.add_update(7.0);
		perf.break_interval();
		perf.begin_draw(9000.0);
		perf.end_draw(9001.0);
		perf.begin_draw(9016.0);
		perf.end_draw(9018.0);
		require(perf.size() == 2 && perf.frame().max == 16.0 && perf.update().max == 0.5,
			"break_interval: the gap and the updates before it are not a frame");

		// A capture keeps its whole chronological recording independently of
		// the overlay ring. Starting one must not include the previous gap.
		require(!perf.start_capture(0) && !perf.start_capture(36001) &&
			!perf.capture_active() && !perf.capture_ready() && perf.size() == 2,
			"Capture bounds reject zero and more than 36000 without disturbing the ring");
		perf.add_update(99.0);
		require(perf.start_capture(300) && perf.capture_active() && !perf.capture_ready() &&
			!perf.capture_complete() && perf.capture_target() == 300 &&
			perf.captured_frames().empty() && perf.capture_reason().empty(),
			"A bounded capture starts empty and active");
		require(!perf.start_capture(1) && perf.capture_target() == 300,
			"An active recording cannot be replaced");
		perf.end_draw(99999.0);
		require(perf.capture_active() && perf.captured_frames().empty(),
			"An unmatched draw end does not consume or interrupt the capture");
		perf.add_update(88.0);
		perf.begin_draw(10000.0);
		perf.end_draw(10002.0);
		require(perf.size() == 2 && perf.captured_frames().empty() && perf.capture_active(),
			"The first capture draw only primes its interval and discards earlier updates");
		for (int i = 1; i <= 300; ++i)
		{
			const int updates = i % 3;
			if (updates > 0) { perf.add_update(0.25); }
			if (updates > 1) { perf.add_update(0.5); }
			perf.set_counts(100 + i, i % 7);
			const double begin = 10000.0 + 17.0 * i;
			perf.begin_draw(begin);
			perf.end_draw(begin + 1.0 + (i % 3));
			require(perf.captured_frames().size() == static_cast<std::size_t>(i) &&
				perf.capture_active() == (i < 300),
				"Capture counts actual rendered frames, including zero-update and catch-up frames");
		}
		require(!perf.capture_active() && perf.capture_ready() && perf.capture_complete() &&
			perf.capture_reason() == "complete" && perf.size() == DebugPerf::capacity,
			"The exact target completes while the overlay ring remains bounded");
		for (int i = 1; i <= 300; ++i)
		{
			const DebugPerf::Frame& frame = perf.captured_frames()[static_cast<std::size_t>(i - 1)];
			const int updates = i % 3;
			const double update_ms = updates == 0 ? 0.0 : updates == 1 ? 0.25 : 0.75;
			require(frame.frame_ms == 17.0 && frame.draw_ms == 1.0 + (i % 3) &&
				frame.updates == updates && frame.update_ms == update_ms &&
				frame.objects == 100 + i && frame.projectiles == i % 7,
				"Capture retains each interval, CPU sum and population snapshot in chronological order");
		}
		require(perf.at(0).objects == 400 && perf.at(DebugPerf::capacity - 1).objects == 161 &&
			!perf.start_capture(1), "The ring retains its newest rows and a completed capture awaits disposal");
		perf.set_counts(999, 999);
		perf.begin_draw(15117.0);
		perf.end_draw(15119.0);
		perf.break_interval();
		require(perf.captured_frames().size() == 300 && perf.captured_frames().back().objects == 400 &&
			perf.capture_complete() && perf.capture_reason() == "complete" && perf.at(0).objects == 999,
			"Later frames and gaps cannot overrun or change a completed recording");
		perf.discard_capture();
		require(!perf.capture_active() && !perf.capture_ready() && !perf.capture_complete() &&
			perf.capture_target() == 0 && perf.capture_reason().empty() &&
			perf.captured_frames().empty() && perf.size() == DebugPerf::capacity,
			"Discard clears only capture state and preserves the overlay history");

		perf.reset();
		require(perf.start_capture(4), "Start an interruptible capture");
		perf.begin_draw(100.0);
		perf.end_draw(102.0);
		perf.add_update(0.5);
		perf.begin_draw(116.0);
		perf.end_draw(118.0);
		perf.add_update(7.0);
		perf.break_interval();
		require(!perf.capture_active() && perf.capture_ready() && !perf.capture_complete() &&
			perf.capture_target() == 4 && perf.capture_reason() == "interrupted" &&
			perf.captured_frames().size() == 1 && perf.size() == 1 && !perf.start_capture(1),
			"A gap retains an explicitly incomplete recording and refuses replacement");
		perf.begin_draw(9000.0);
		perf.end_draw(9001.0);
		perf.add_update(0.25);
		perf.begin_draw(9016.0);
		perf.end_draw(9018.0);
		require(perf.size() == 2 && perf.frame().max == 16.0 && perf.at(0).update_ms == 0.25 &&
			perf.captured_frames().size() == 1 && perf.capture_reason() == "interrupted",
			"The ring re-primes after a gap but an interrupted capture never silently resumes");
		perf.discard_capture();
		require(perf.start_capture(1), "A discarded recording permits a new one-frame capture");
		perf.begin_draw(10000.0);
		perf.end_draw(10001.0);
		require(perf.capture_active() && perf.captured_frames().empty(),
			"A fresh capture primes again even when the ring already has a previous frame");
		const double nan = (std::numeric_limits<double>::quiet_NaN)();
		const double infinity = (std::numeric_limits<double>::infinity)();
		for (const double invalid : { -1.0, nan, infinity, -infinity }) { perf.add_update(invalid); }
		perf.add_update(0.0);
		perf.begin_draw(10016.0);
		perf.end_draw(10016.0);
		require(perf.capture_complete() && perf.captured_frames().size() == 1 &&
			perf.captured_frames()[0].updates == 1 && perf.captured_frames()[0].update_ms == 0.0 &&
			perf.captured_frames()[0].draw_ms == 0.0,
			"Invalid update durations are ignored; genuine zero-duration updates and draws remain valid");

		// Invalid timestamps must produce an interrupted partial capture,
		// rather than a negative/non-finite interval or a poisoned ring.
		for (const bool invalid_end : { false, true })
		{
			for (const double invalid : { -1.0, nan, infinity, -infinity, 115.0, 116.0 })
			{
				perf.reset();
				require(perf.start_capture(3), "Start a capture for invalid-clock coverage");
				perf.begin_draw(100.0);
				perf.end_draw(102.0);
				perf.add_update(0.5);
				perf.begin_draw(116.0);
				perf.end_draw(118.0);
				perf.add_update(9.0);
				if (invalid_end)
				{
					perf.begin_draw(132.0);
					perf.end_draw(invalid);
				}
				else
				{
					perf.begin_draw(invalid);
					perf.end_draw(134.0);
				}
				require(perf.capture_ready() && !perf.capture_complete() &&
					perf.capture_reason() == "interrupted" && perf.captured_frames().size() == 1 &&
					perf.size() == 1 && perf.at(0).frame_ms == 16.0 && perf.at(0).update_ms == 0.5,
					"Negative, non-finite and backwards timestamps retain only valid rows");
				perf.begin_draw(9000.0);
				perf.end_draw(9002.0);
				perf.begin_draw(9016.0);
				perf.end_draw(9018.0);
				require(perf.size() == 2 && perf.at(0).frame_ms == 16.0 &&
					perf.at(0).updates == 0 && perf.captured_frames().size() == 1,
					"Invalid-clock recovery re-primes and drops pending updates without reviving the capture");
			}
		}
		perf.reset();
		require(!perf.capture_active() && !perf.capture_ready() && perf.capture_target() == 0 &&
			perf.captured_frames().empty() && perf.capture_reason().empty() && perf.size() == 0,
			"Reset clears interrupted captures and all overlay samples");
		require(perf.start_capture(36000), "The documented upper capture bound is accepted");
		perf.reset();
		require(!perf.capture_active() && !perf.capture_ready() && perf.capture_target() == 0 &&
			perf.captured_frames().empty() && perf.size() == 0,
			"Reset also cancels an active capture");
	}
}
#endif
