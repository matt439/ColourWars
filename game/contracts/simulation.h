#pragma once

// The fixed simulation rate, written down once. main.cpp hands it to the
// engine as ApplicationOptions::target_fps, which Application turns into its
// fixed step, so anything that reasons about distance per step (the
// anti-tunnelling check in game/config/tunnelling_limits.h) reads the same number the
// match actually runs at.
constexpr int SIMULATION_HZ = 60;

// The one step length: the dt every Level update in the shipping game
// receives. The engine's StepTimer counts 10,000,000 ticks a second and
// truncates 1/SIMULATION_HZ to whole ticks (166666 at 60 Hz), so the step is
// 0.0166666 seconds, not the 0.016666668 that 1.0f / 60.0f spells. The tick
// rate is restated here because this header includes nothing; a probe that
// steps at 1.0f / 60.0f runs a little longer than the game's step.
constexpr long long SIMULATION_TICKS_PER_SECOND = 10'000'000;
constexpr float SECONDS_PER_STEP = static_cast<float>(
	static_cast<double>(SIMULATION_TICKS_PER_SECOND / SIMULATION_HZ) /
	static_cast<double>(SIMULATION_TICKS_PER_SECOND));
