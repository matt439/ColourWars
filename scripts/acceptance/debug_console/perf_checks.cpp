#include "game/pch.h"
#include "scripts/acceptance/debug_console/fixtures.h"

#if COLOURWARS_DEV_TOOLS
#include "game/debug/debug_perf.h"
#include "game/objects/debug_text.h"
#include <cmath>
#include <limits>

// The perf ring: frame-rate snapshot, overlay statistics and bounded capture.
// The perf.capture command grammar is checked through the director in
// session_checks.cpp (check_polish).
namespace debug_console_acceptance
{
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
		require(lines.size() == 5 && lines[0] == "CPU ms    min    avg    max  (240 frames)" &&
			lines[1] == "frame   16.00  16.00  16.00" &&
			lines[2] == "update   0.75   1.00   1.25  x2.00 a frame" &&
			lines[3] == "draw     2.00   3.00   4.00" && lines[4] == "objects 182  projectiles 12",
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
