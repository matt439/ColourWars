#pragma once

// One real Player in a bare Scene, stepped in Level's order (Level::update in
// game/objects/level.cpp): the player's input, Scene movement, the paid-shot
// launch, contact resolution, then end_contacts, update_prev_rectangle and
// end_tick. The launch's projectiles are handed back, never added to the
// Scene, so a course has no hits it did not ask for. A probe's Course derives
// from this and keeps its own construction and observations.

#include "game/objects/collision_object_type.h"
#include "game/contracts/simulation.h"
#include "game/objects/player.h"
#include "game/objects/projectile.h"
#include "game/objects/structure.h"
#include "engine/scene/scene.h"

#include <memory>
#include <vector>

namespace acceptance
{
    class PlayerCourse
    {
    public:
        static inline const float step_seconds = SECONDS_PER_STEP;

        labrador::Scene scene{ nullptr, nullptr };
        Player* player = nullptr;
        // One step's length; Level's fixed step unless a course chooses its own.
        float seconds = step_seconds;

        // A plain white structure; the caller ends the tick when its course
        // is built.
        void add_structure(const labrador::RectangleF& rectangle, labrador::RenderResources& render,
            CollisionObjectType type = CollisionObjectType::structure)
        {
            this->scene.add(std::make_unique<Structure>("sprite_sheet_1", "square_white_4",
                rectangle, std::make_unique<labrador::RectangleF>(rectangle), &render, type));
        }

        // Input, Scene movement and the launch, before contacts resolve.
        std::vector<std::unique_ptr<Projectile>> begin_step(const PlayerInputData& actions)
        {
            this->player->set_player_input(actions);
            this->scene.update(this->seconds);
            return this->player->update_weapon_and_get_projectiles(this->seconds,
                this->scene.collision_objects());
        }

        // After contacts resolve. Level also moves the weapon to the resolved
        // body here; a course that draws or fires from it asks for that.
        void finish_step(bool weapon_position = false)
        {
            this->player->end_contacts(this->scene.contacts());
            if (weapon_position) { this->player->update_weapon_position(); }
            this->player->update_prev_rectangle();
            this->scene.end_tick();
        }

        std::vector<std::unique_ptr<Projectile>> step(const PlayerInputData& actions,
            bool weapon_position = false)
        {
            auto launched = this->begin_step(actions);
            this->scene.resolve();
            this->finish_step(weapon_position);
            return launched;
        }
    };
}
