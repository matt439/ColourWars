#pragma once

#include "game/objects/blocked_shot_contact.h"
#include "game/paint_owners.h"
#include "game/presentation_match.h"
#include "engine/render/texture_object.h"

#include <array>
#include <span>

// One match's bounded, detached cover contacts. Level advances these once per
// active tick, then every world view draws the same immutable snapshot.
class BlockedShotFeedback final : public labrador::TextureObject
{
public:
    struct Cue
    {
        BlockedShotContact contact;
        float age = 0.0f;
    };

    static constexpr size_t cue_capacity = 16;

    BlockedShotFeedback(const BlockedShotPresentation* presentation,
        labrador::RenderResources* render_resources);

    void update(float dt);
    void emit(const BlockedShotContact& contact);
    void clear();
    void draw(labrador::DrawList& draw_list) const;

    std::span<const Cue> cues() const;
    float repeat_remaining(int owner) const;
    mattmath::RectangleF bounds() const;
    mattmath::RectangleF cue_bounds(const Cue& cue) const;

private:
    const BlockedShotPresentation* presentation_;
    std::array<Cue, cue_capacity> cues_{};
    size_t count_ = 0;
    std::array<float, paint_owners::count + 1> repeat_remaining_{};

    static size_t owner_index(int owner);
    void draw_block(labrador::DrawList& draw_list,
        const mattmath::Vector2F& center, const mattmath::Vector2F& size,
        float angle, const labrador::Colour& colour, float alpha) const;
};
