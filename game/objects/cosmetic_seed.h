#pragma once

#include <cstdint>

// The start of one roster member's private cosmetic stream: QuietCharacter's
// breathing and blinks, IdleDrip's drip intervals. A hash of the identity
// alone, so neither render count nor a gameplay draw moves it.
inline std::uint32_t cosmetic_seed(std::uint32_t identity)
{
    return 0x9e3779b9u ^ ((identity + 1u) * 0x85ebca6bu);
}
