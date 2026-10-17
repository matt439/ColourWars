#pragma once

// The fixed simulation rate, written down once. main.cpp hands it to the
// engine as ApplicationOptions::target_fps, which Application turns into its
// fixed step, so anything that reasons about distance per step (the
// anti-tunnelling check in game/tunnelling_limits.h) reads the same number the
// match actually runs at.
constexpr int SIMULATION_HZ = 60;
constexpr float SECONDS_PER_STEP = 1.0f / static_cast<float>(SIMULATION_HZ);
