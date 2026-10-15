#pragma once

#include "game/presentation.h"
#include "engine/core/game_object.h"
#include "engine/collision/collision_object.h"
#include "engine/render/texture_object.h"

#include <array>
#include <cstdint>
#include <span>

// Scene-owned decoration, explicitly advanced after contacts and muzzle repair.
// It never joins collision dispatch and cannot spend paint or capture a surface.
class IdleDrip final : public labrador::GameObject, public labrador::TextureObject
{
public:
    struct Drop
    {
        mattmath::Vector2F position;
        float age = 0.0f;
        bool operator==(const Drop&) const = default;
    };

    IdleDrip(const IdleDripPresentation* presentation,
        labrador::RenderResources* resources, const labrador::Colour& colour,
        uint32_t identity);

    // Scene's ordinary update precedes final player contacts. Level calls
    // advance exactly once afterwards, including for eliminated players.
    void update(float) override {}
    void advance(bool settled, const mattmath::Vector2F& nozzle, float rotation,
        float dt, std::span<labrador::CollisionObject* const> terrain);
    void clear();
    void draw(labrador::DrawList& list) const override;
    mattmath::RectangleF bounds() const override;
    bool attached() const;
    float bead_age() const;
    float interval_remaining() const;
    mattmath::RectangleF bead_rectangle() const;
    std::span<const Drop> drops() const;

private:
    const IdleDripPresentation* presentation_;
    labrador::Colour paint_;
    uint32_t sequence_;
    float interval_ = 0.0f;
    float bead_age_ = -1.0f;
    bool was_settled_ = false;
    mattmath::Vector2F attachment_;
    std::array<Drop, 4> drops_{};
    size_t count_ = 0;

    float next_interval();
    mattmath::RectangleF drop_rectangle(const Drop& drop) const;
    bool touches(const mattmath::RectangleF& rectangle,
        std::span<labrador::CollisionObject* const> terrain) const;
    void draw_bead(labrador::DrawList& list,
        const mattmath::RectangleF& rectangle) const;
};
