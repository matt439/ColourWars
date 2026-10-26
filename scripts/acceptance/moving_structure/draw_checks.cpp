#include "game/pch.h"
#include "scripts/acceptance/moving_structure/fixtures.h"

namespace moving_structure_acceptance
{
    namespace
    {
        void shifted(const mattmath::RectangleF& actual, mattmath::RectangleF anchor, float offset)
        {
            anchor.offset(offset, 0.0f);
            require(bits(actual.x) == bits(anchor.x) && bits(actual.y) == bits(anchor.y) &&
                bits(actual.width) == bits(anchor.width) && bits(actual.height) == bits(anchor.height),
                "Moving visual/cull bounds were shifted incorrectly or twice");
        }
    }
    void check_draw_cpu(World& world)
    {
        const HorizontalMotionSpec spec{ { 512.0f, 500.0f, 128.0f, 10.0f }, 96.0f, 2.0f, 0.25f };
        auto moving = world.mover(spec);
        auto stationary = world.surface(spec.anchor, { true, true, true, true });
        const float offset = moving->translation().x;
        shifted(moving->bounds(), stationary->bounds(), offset);
        require(moving->shape()->bounding_box() == moving->bounds(), "World collider and visual bounds disagree");
        for (float pixel : { 0.25f, 1.0f, 4.0f })
        { shifted(moving->cull_bounds(pixel), stationary->cull_bounds(pixel), offset); }
        const mattmath::RectangleF local(spec.anchor.left() + 20.0f, spec.anchor.top() - 2.0f, 12.0f, 6.0f);
        auto contact = local; contact.offset(offset, 0.0f);
        world.paint(*moving, contact, PlayerTeam::a, 5); world.paint(*stationary, local, PlayerTeam::a, 5);
        for (float pixel : { 0.25f, 1.0f, 4.0f })
        { shifted(moving->cull_bounds(pixel), stationary->cull_bounds(pixel), offset); }
        AirbrushShadow anchor(spec.anchor.left(), spec.anchor.right(), spec.anchor.bottom(), &world.render);
        TranslatedAirbrushShadow shadow(*moving, &world.render);
        shifted(shadow.bounds(), anchor.bounds(), offset);
        for (float pixel : { 0.25f, 1.0f, 4.0f })
        { shifted(shadow.cull_bounds(pixel), anchor.cull_bounds(pixel), offset); }
        const auto motion = motion_bits(moving->motion_state());
        const auto paint = sample_bits(snapshot(*moving));
        moving->set_miniature(true);
        std::ignore = moving->cull_bounds(8.0f); std::ignore = shadow.cull_bounds(8.0f);
        moving->set_miniature(false);
        require(motion_bits(moving->motion_state()) == motion && sample_bits(snapshot(*moving)) == paint,
            "View/cull selection changed the simulation or paint");

        // Exercise the actual normal retirement mechanism, not a fake lifetime
        // hook. Fixed geometry ignores both requested flags and bounds sweeps.
        labrador::Scene scene(nullptr, nullptr);
        scene.set_bounds({ -100.0f, -100.0f, 10.0f, 10.0f });
        auto* admitted = scene.add(std::move(moving));
        scene.add(std::make_unique<TranslatedAirbrushShadow>(*admitted, &world.render));
        for (int pass = 0; pass < 3; ++pass)
        {
            admitted->set_for_deletion(true); scene.end_tick();
            require(scene.collision_objects().size() == 1 && scene.collision_objects()[0] == admitted &&
                !admitted->for_deletion(), "Normal bounds retirement invalidated borrowed moving pose");
        }
        // Scene destruction next destroys mover before plain wrapper. Its
        // default shadow destructor must not observe the expired pose.
        // Actual DrawList/camera restoration/pixels are a separate root-owned
        // native gate; null resource handles above do not claim render acceptance.
    }
}
