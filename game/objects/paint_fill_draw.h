#pragma once

class PaintFill;
class PaperCard;
struct PaintFillPresentation;
namespace labrador { class DrawList; }

void draw_paint_fill(labrador::DrawList& list, const PaintFill& fill,
    const PaperCard& paper, const PaintFillPresentation& look, bool reduce_effects);
