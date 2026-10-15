#include "game/pch.h"
#include "game/objects/idle_drip.h"
#include "game/objects/collision_object_type.h"
#include "game/play_settings.h"
#include "engine/collision/narrow_phase.h"

#include <cmath>

using namespace labrador;
using namespace mattmath;

IdleDrip::IdleDrip(const IdleDripPresentation* presentation,
    RenderResources* resources, const Colour& colour, uint32_t identity,
    const PlaySettings* preferences) :
    TextureObject(presentation->sprite_sheet, presentation->fill_frame, resources),
    presentation_(presentation), preferences_(preferences), paint_(colour),
    sequence_(0x9e3779b9u ^ ((identity + 1u) * 0x85ebca6bu))
{
    if (this->sequence_ == 0) { this->sequence_ = 1; }
    this->interval_ = this->next_interval();
}

float IdleDrip::next_interval()
{
    // Private xorshift stream: neither render count nor gameplay RNG affects it.
    this->sequence_ ^= this->sequence_ << 13;
    this->sequence_ ^= this->sequence_ >> 17;
    this->sequence_ ^= this->sequence_ << 5;
    const float fraction = static_cast<float>(this->sequence_ >> 8) / 16777216.0f;
    return this->presentation_->interval_min + fraction *
        (this->presentation_->interval_max - this->presentation_->interval_min);
}

RectangleF IdleDrip::drop_rectangle(const Drop& drop) const
{
    const float size = this->presentation_->size;
    return RectangleF(drop.position - Vector2F(size * 0.5f, size * 0.5f),
        Vector2F(size, size));
}

bool IdleDrip::touches(const RectangleF& rectangle,
    std::span<CollisionObject* const> terrain) const
{
    for (const CollisionObject* object : terrain)
    {
        if (!is_structure(to_collision_type(object->tag())) || object->for_deletion()) { continue; }
        const Shape* shape = object->shape();
        if (shape != nullptr && rectangle.intersects(shape->bounding_box()) &&
            narrow_phase(rectangle, *shape).has_value()) { return true; }
    }
    return false;
}

void IdleDrip::advance(bool settled, const Vector2F& nozzle, float rotation,
    float dt, std::span<CollisionObject* const> terrain)
{
    if (dt <= 0.0f || !std::isfinite(dt)) { return; }
    if (this->preferences_ != nullptr && this->preferences_->reduce_effects)
    {
        // Retire an interrupted bead's expired interval as well as its shape.
        // Restoring detail waits for a fresh bead instead of replaying it.
        if (this->was_settled_ || this->bead_age_ >= 0.0f || this->count_ > 0)
        {
            this->interval_ = this->next_interval();
        }
        this->clear();
        return;
    }
    const IdleDripPresentation& look = *this->presentation_;
    if (!look.enabled)
    {
        this->count_ = 0;
        this->bead_age_ = -1.0f;
        this->was_settled_ = false;
        this->interval_ = this->next_interval();
        return;
    }

    for (size_t index = 0; index < this->count_;)
    {
        Drop& drop = this->drops_[index];
        const RectangleF before = this->drop_rectangle(drop);
        const float elapsed = std::min(dt, std::max(0.0f, look.lifetime - drop.age));
        drop.position.y += look.gravity * (drop.age * elapsed + 0.5f * elapsed * elapsed);
        drop.age += elapsed;
        // The drop moves only vertically and keeps its size, so this rectangle
        // is its exact swept footprint, including on thin platforms and ramps.
        const RectangleF swept = RectangleF::union_of(before, this->drop_rectangle(drop));
        if (drop.age >= look.lifetime || this->touches(swept, terrain))
        {
            this->drops_[index] = this->drops_[--this->count_];
        }
        else { ++index; }
    }

    if (!settled)
    {
        if (this->was_settled_) { this->interval_ = this->next_interval(); }
        this->bead_age_ = -1.0f;
        this->was_settled_ = false;
        return;
    }
    this->was_settled_ = true;

    // Match the tool's vertical flip when facing left. Gravity stretches the
    // bead downwards even while its attachment follows continuous aiming.
    const float side = rotation > PI * 0.5f || rotation < -PI * 0.5f ? -1.0f : 1.0f;
    this->attachment_ = nozzle + Vector2F::rotate_vector(
        Vector2F(-look.nozzle_inset, side * look.nozzle_underside), rotation);
    if (!this->attached())
    {
        this->interval_ -= dt;
        if (this->interval_ <= 0.0f) { this->bead_age_ = 0.0f; }
    }
    else { this->bead_age_ += dt; }

    if (this->attached() && this->touches(this->bead_rectangle(), terrain))
    {
        this->bead_age_ = -1.0f;
        this->interval_ = this->next_interval();
        return;
    }
    if (this->bead_age_ >= look.grow_time + look.stretch_time)
    {
        if (this->count_ < this->drops_.size())
        {
            this->drops_[this->count_++] = Drop{ this->bead_rectangle().center(), 0.0f };
        }
        this->bead_age_ = -1.0f;
        this->interval_ = this->next_interval();
    }
}

bool IdleDrip::attached() const { return this->bead_age_ >= 0.0f; }
void IdleDrip::clear()
{
    this->count_ = 0;
    this->bead_age_ = -1.0f;
    this->was_settled_ = false;
}

float IdleDrip::bead_age() const { return this->bead_age_; }
float IdleDrip::interval_remaining() const { return this->interval_; }
std::span<const IdleDrip::Drop> IdleDrip::drops() const
{
    return { this->drops_.data(), this->count_ };
}

RectangleF IdleDrip::bead_rectangle() const
{
    const IdleDripPresentation& look = *this->presentation_;
    const float growth = std::clamp(this->bead_age_ / look.grow_time, 0.0f, 1.0f);
    const float stretch = std::clamp((this->bead_age_ - look.grow_time) / look.stretch_time, 0.0f, 1.0f);
    const float width = look.size * (0.35f + 0.65f * growth);
    return RectangleF(this->attachment_.x - width * 0.5f, this->attachment_.y,
        width, width + stretch * look.stretch);
}

void IdleDrip::draw_bead(DrawList& list, const RectangleF& rectangle) const
{
    RectangleF outer = rectangle;
    outer.inflate(this->presentation_->keyline_width);
    // Coarse keylined liquid from the existing neutral fill texel.
    // No new atlas or collision silhouette is required.
    this->draw_with(list, outer, this->frame(), this->presentation_->keyline,
        Vector2F::ZERO, SpriteFlip::none, 0.0f);
    this->draw_with(list, rectangle, this->frame(), this->paint_,
        Vector2F::ZERO, SpriteFlip::none, 0.0f);
}

void IdleDrip::draw(DrawList& list) const
{
    if (!this->presentation_->enabled ||
        (this->preferences_ != nullptr && this->preferences_->reduce_effects)) { return; }
    if (this->attached()) { this->draw_bead(list, this->bead_rectangle()); }
    for (const Drop& drop : this->drops()) { this->draw_bead(list, this->drop_rectangle(drop)); }
}

RectangleF IdleDrip::bounds() const
{
    RectangleF bounds = this->attached() ? this->bead_rectangle() : RectangleF::ZERO;
    bool any = this->attached();
    for (const Drop& drop : this->drops())
    {
        bounds = any ? RectangleF::union_of(bounds, this->drop_rectangle(drop)) : this->drop_rectangle(drop);
        any = true;
    }
    if (any) { bounds.inflate(this->presentation_->keyline_width); }
    return bounds;
}
