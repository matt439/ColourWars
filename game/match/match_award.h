#pragma once

// The results call-outs, in the order that picks a player's one call-out when
// they lead more than one. Paint comes first and splats last: score is painted
// surface, and eliminations only make space to paint.
enum class MatchAward
{
	paint_laid,
	recaptured,
	airtime,
	splats,
};

constexpr int match_award_count = static_cast<int>(MatchAward::splats) + 1;
