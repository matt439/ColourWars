#include "game/pch.h"
#include "game/objects/round_shot_shape.h"
#include <atomic>

namespace
{
	std::atomic<RoundShotShape> current_shape{ RoundShotShape::rectangle };
}

RoundShotShape round_shot_shape()
{
	return current_shape.load(std::memory_order_relaxed);
}

void set_round_shot_shape(RoundShotShape shape)
{
	current_shape.store(shape, std::memory_order_relaxed);
}
