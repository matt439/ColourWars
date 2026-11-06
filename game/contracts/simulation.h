#pragma once

// The fixed simulation rate, written down once. main.cpp hands it to the
// engine as ApplicationOptions::target_fps, which Application turns into its
// fixed step, so anything that reasons about distance per step (the
// anti-tunnelling check in game/config/tunnelling_limits.h) reads the same number the
// match actually runs at.
namespace simulation
{
constexpr int simulation_hz = 60;

// The one step length: the dt every Level update in the shipping game
// receives. The engine's StepTimer counts 10,000,000 ticks a second and
// truncates 1/simulation_hz to whole ticks (166666 at 60 Hz), so the step is
// 0.0166666 seconds, not the 0.016666668 that 1.0f / 60.0f spells. The tick
// rate is restated here because this header includes nothing; a probe that
// steps at 1.0f / 60.0f runs a little longer than the game's step.
constexpr long long simulation_ticks_per_second = 10'000'000;
constexpr float seconds_per_step = static_cast<float>(
	static_cast<double>(simulation_ticks_per_second / simulation_hz) /
	static_cast<double>(simulation_ticks_per_second));
}
