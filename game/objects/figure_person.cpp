#include "game/pch.h"
#include "game/objects/figure_person.h"
#include "game/ai/bot_random.h"

#include <algorithm>
#include <cmath>

using namespace labrador;

namespace
{
    // Its own constant, so the face never moves the bricks' draws from the
    // same hand ("figslips").
    constexpr std::uint64_t slip_salt = 0x666967736C697073ULL;
}

FigurePerson::FigurePerson(std::vector<std::size_t> slips) :
    slips_(std::move(slips))
{
}

FigurePerson::FigurePerson(const FigureSlipsPresentation& slips, std::uint64_t hand)
{
    if (hand == 0 || slips.list.empty())
    {
        return;
    }
    const std::vector<FigurePerson> people = everyone(slips);
    const std::size_t singles = slips.list.size();
    const std::size_t pairs = people.size() - singles;
    BotRandom random(hand ^ slip_salt);
    const bool two = pairs > 0 && random.unit() < slips.two_slip_chance;
    const std::uint64_t pick = random.next();
    *this = two
        ? people[singles + static_cast<std::size_t>(pick % pairs)]
        : people[static_cast<std::size_t>(pick % singles)];
}

std::vector<FigurePerson> FigurePerson::everyone(const FigureSlipsPresentation& slips)
{
    std::vector<FigurePerson> people;
    for (std::size_t first = 0; first < slips.list.size(); ++first)
    {
        people.push_back(FigurePerson({ first }));
    }
    for (std::size_t first = 0; first < slips.list.size(); ++first)
    {
        for (std::size_t second = first + 1; second < slips.list.size(); ++second)
        {
            if (!slips.is_exclusive(first, second))
            {
                people.push_back(FigurePerson({ first, second }));
            }
        }
    }
    return people;
}

std::string FigurePerson::name(const FigureSlipsPresentation& slips) const
{
    std::string name;
    for (const std::size_t index : this->slips_)
    {
        if (index >= slips.list.size())
        {
            continue;
        }
        name += (name.empty() ? "" : "+") + slips.list[index].id;
    }
    return name;
}

std::string FigurePerson::band_frame(const FigureSlipsPresentation& slips,
    const std::string& strip, int frame) const
{
    return slips.band_frame + "." + this->name(slips) + "." + strip + "." + std::to_string(frame);
}

std::vector<RespawnStroke> FigurePerson::strokes(const RespawnRedrawPresentation& redraw,
    const FigureSlipsPresentation& slips) const
{
    const std::vector<RespawnStroke>& house = redraw.strokes;
    // What happens to each house stroke: kept, redrawn, or left out, and
    // what the person drew straight after it.
    std::vector<const FigureSlipStroke*> changed(house.size(), nullptr);
    std::vector<std::vector<const FigureSlipStroke*>> after(house.size());
    for (const std::size_t index : this->slips_)
    {
        if (index >= slips.list.size())
        {
            continue;
        }
        for (const FigureSlipStroke& edit : slips.list[index].strokes)
        {
            if (edit.stroke >= house.size())
            {
                continue;
            }
            if (edit.edit == FigureSlipStroke::Edit::insert_after)
            {
                after[edit.stroke].push_back(&edit);
            }
            else
            {
                changed[edit.stroke] = &edit;
            }
        }
    }
    std::vector<RespawnStroke> drawn;
    drawn.reserve(house.size() + 2);
    for (std::size_t at = 0; at < house.size(); ++at)
    {
        if (changed[at] == nullptr)
        {
            drawn.push_back(house[at]);
        }
        else if (changed[at]->edit == FigureSlipStroke::Edit::replace)
        {
            drawn.push_back({ house[at].part, changed[at]->points });
        }
        for (const FigureSlipStroke* inserted : after[at])
        {
            drawn.push_back({ inserted->part, inserted->points });
        }
    }
    return drawn;
}

std::array<RectangleF, 2> FigurePerson::blink_eyes(const FigurePresentation& figure,
    const RespawnRedrawPresentation& redraw) const
{
    std::array<RectangleF, 2> eyes = figure.quiet.eyes;
    if (!this->slipped())
    {
        return eyes;
    }
    const std::vector<RectangleF> house = eye_rectangles(redraw.strokes, redraw.pen_width);
    const std::vector<RectangleF> own = eye_rectangles(this->strokes(redraw, figure.slips),
        redraw.pen_width);
    if (house.size() != eyes.size() || own.size() != eyes.size())
    {
        return eyes;
    }
    for (std::size_t index = 0; index < eyes.size(); ++index)
    {
        eyes[index].x += own[index].x - house[index].x;
        eyes[index].y += own[index].y - house[index].y;
    }
    return eyes;
}

std::vector<RectangleF> eye_rectangles(const std::vector<RespawnStroke>& strokes, float pen_width)
{
    std::vector<RectangleF> eyes;
    for (const RespawnStroke& stroke : strokes)
    {
        if (stroke.part != "eye")
        {
            continue;
        }
        for (std::size_t at = 1; at < stroke.points.size(); ++at)
        {
            const Vector2F& from = stroke.points[at - 1];
            const Vector2F& to = stroke.points[at];
            const RectangleF sweep(std::min(from.x, to.x), std::min(from.y, to.y),
                std::abs(to.x - from.x) + pen_width,
                std::abs(to.y - from.y) + pen_width);
            // Strokes that touch are one eye.
            bool merged = false;
            for (RectangleF& eye : eyes)
            {
                if (sweep.x <= eye.right() && eye.x <= sweep.right() &&
                    sweep.y <= eye.bottom() && eye.y <= sweep.bottom())
                {
                    eye = RectangleF::union_of(eye, sweep);
                    merged = true;
                }
            }
            if (!merged)
            {
                eyes.push_back(sweep);
            }
        }
    }
    return eyes;
}
