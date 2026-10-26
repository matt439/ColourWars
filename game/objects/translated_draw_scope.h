#pragma once

#include "engine/render/renderer.h"
#include "engine/render/camera.h"
#include <stdexcept>

// Scene assigns a disjoint DrawList/View to each draw worker. The handle may
// alias, so this scope borrows that worker-owned view and saves Camera by VALUE.
// It never stores or changes shared drawable state. Do not use across frames.
class TranslatedDrawScope final
{
public:
    TranslatedDrawScope(labrador::DrawList& list, const mattmath::Vector2F& offset) :
        list_(list), saved_(checked_camera(list)), active_(offset != mattmath::Vector2F::ZERO)
    {
        if (this->active_)
        {
            labrador::Camera camera = this->saved_;
            camera.translation = camera.translation - offset;
            this->list_.set_camera(camera);
        }
    }
    ~TranslatedDrawScope() noexcept
    {
        // set_camera assigns only this valid View's value; no allocation,
        // draw, scale validation or shared object access occurs here.
        if (this->active_) { this->list_.set_camera(this->saved_); }
    }
    TranslatedDrawScope(const TranslatedDrawScope&) = delete;
    TranslatedDrawScope& operator=(const TranslatedDrawScope&) = delete;
private:
    static labrador::Camera checked_camera(const labrador::DrawList& list)
    {
        if (!list.valid()) { throw std::invalid_argument("Translated drawing requires a valid per-view DrawList."); }
        return list.camera();
    }
    labrador::DrawList& list_;
    labrador::Camera saved_;
    bool active_ = false;
};
