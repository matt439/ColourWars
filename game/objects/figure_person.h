#pragma once

#include "game/config/presentation/presentation_match.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// The person drawing a match's figures (docs/drawing-mistakes-research.md,
// section 9, decisions 3, 5 and 8): which of figure.slips they made, chosen
// from the match's hand (MatchRandom::hand). One person drew both teams, so
// every figure in the match shares it. The house hand, 0, made no slip and
// draws the authored figure.
//
// It holds positions in figure.slips.list, never pointers into it, because
// `tune` rebuilds that list; every read takes the presentation again and
// skips a position the list no longer has.
class FigurePerson
{
public:
    FigurePerson() = default;
    FigurePerson(const FigureSlipsPresentation& slips, std::uint64_t hand);

    // Every person the slips allow, in the order build-painter-art.ps1 bakes
    // them: each slip alone, then each pair that is not exclusive.
    static std::vector<FigurePerson> everyone(const FigureSlipsPresentation& slips);

    bool slipped() const { return !this->slips_.empty(); }
    const std::vector<std::size_t>& slips() const { return this->slips_; }

    // "<id>" or "<id>+<id>" in list order, as the band frames are named.
    std::string name(const FigureSlipsPresentation& slips) const;
    // The band this person drew over frame `frame` of the body strip `strip`.
    std::string band_frame(const FigureSlipsPresentation& slips, const std::string& strip,
        int frame) const;

    // The strokes this person drew the figure with: the house strokes with
    // each of their slips' edits.
    std::vector<RespawnStroke> strokes(const RespawnRedrawPresentation& redraw,
        const FigureSlipsPresentation& slips) const;

    // The quiet blink's eyes, moved wherever this person's eye strokes moved
    // from the house strokes.
    std::array<labrador::RectangleF, 2> blink_eyes(const FigurePresentation& figure,
        const RespawnRedrawPresentation& redraw) const;

private:
    explicit FigurePerson(std::vector<std::size_t> slips);

    std::vector<std::size_t> slips_;
};

// Each eye, in texels of the canvas, from the strokes whose part is "eye";
// strokes that touch are one eye. In stroke order.
std::vector<labrador::RectangleF> eye_rectangles(const std::vector<RespawnStroke>& strokes,
    float pen_width);
