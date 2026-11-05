#pragma once

#include <stdexcept>

// Level::update throws this when a player has left the scene's bounds. Players
// are never retired, so the match cannot continue. Its own type lets
// FatalReport classify it without reading its message.
class PlayerOutOfBounds : public std::runtime_error
{
public:
	PlayerOutOfBounds() : std::runtime_error("Player out of bounds") {}
};
