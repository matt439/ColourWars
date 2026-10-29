#include "game/pch.h"
#include "scripts/acceptance/turbulence_blockout/fixtures.h"
#include "engine/math/triangle.h"
#include "game/objects/collision_object_type.h"

namespace turbulence_blockout_acceptance
{
    bool supported(const Player& player)
    {
        const auto state = player.move_state();
        return state == PlayerMoveState::on_ground || state == PlayerMoveState::on_drop_down_ground ||
            state == PlayerMoveState::on_ramp_left || state == PlayerMoveState::on_ramp_right;
    }

    PlayerInputData input(float movement, bool pressed, bool held, bool drop)
    {
        PlayerInputData result;
        result.connected = true; result.x_movement = movement;
        result.jump_pressed = pressed; result.jump_held = held; result.drop_through = drop;
        return result;
    }

    Services::Services(const std::filesystem::path& content) :
        tuning((content / "tuning.json").string().c_str()),
        presentation((content / "presentation.json").string().c_str()),
        info((content / "levels/turbulence_blockout.json").string().c_str()),
        builder(&this->render, &this->audio, &this->preferences, &this->tuning.paint())
    {
        for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
        {
            this->render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(), TextureHandle{}));
        }
        this->audio.add_sound_bank("paint_exe", SoundBank::silent());
        this->info.validate(); this->info.validate_content(this->builder, this->render, this->audio);
        const JsonValue entries = this->info.collision_objects_json();
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            const JsonValue object = entries.at(index);
            require(!object.has("motion"), "Static Turbulence skeleton unexpectedly acquired motion");
            Surface surface{ object.string("name"), object.string("type"), object.string("collision_type"), {} };
            if (surface.ramp())
            {
                const JsonValue shape = object.object("triangle");
                surface.rectangle = TriangleRightAxisAligned(shape.number("x1"), shape.number("y1"),
                    shape.number("x2"), shape.number("y2"), shape.number("x3"), shape.number("y3")).bounding_box();
            }
            else
            {
                const JsonValue shape = object.object("rectangle");
                surface.rectangle = { shape.number("x"), shape.number("y"), shape.number("width"), shape.number("height") };
            }
            for (const Surface& previous : this->surfaces)
            { require(previous.name != surface.name, "Duplicate Turbulence surface name " + surface.name); }
            this->surfaces.push_back(std::move(surface));
        }
    }

    const Surface& Services::named(const std::string& name) const
    {
        for (const Surface& surface : this->surfaces) { if (surface.name == name) { return surface; } }
        throw std::runtime_error("Missing Turbulence surface " + name);
    }

    Run::Run(Services& with, const Vector2F& position, PlayerTeam team, float initial_velocity, WeaponType tool) : services(with)
    {
        this->scene.set_bounds(with.info.out_of_bounds_rectangle());
        auto built = with.builder.build_collision_objects(with.info.collision_objects_json(), with.colours);
        require(built.size() == with.surfaces.size(), "Builder changed the Turbulence object count");
        for (std::size_t index = 0; index < built.size(); ++index)
        {
            CollisionObject* object = this->scene.add(std::move(built[index]));
            this->structures.emplace_back(&with.surfaces[index], object);
            this->targets.push_back(object);
        }
        const RectangleF body(position, with.tuning.player().size);
        auto weapon = WeaponBuilder::build_weapon(&with.tuning, team, 0, 0, Colour::white,
            tool, body.center(), &with.render, &with.audio, &with.preferences,
            &with.presentation.tool_motion(), &with.presentation.weapon_feedback());
        this->player = this->scene.add(std::make_unique<Player>(body, &with.tuning,
            &with.presentation.movement_feedback(), &with.presentation.combat_feedback(),
            &with.presentation.respawn_redraw(), &with.render, &with.audio, &with.preferences,
            0, team, tool, std::move(weapon), WeaponType::none, Colour::white,
            Viewport{}, position, Vector2F(initial_velocity, 0.0f)));
        this->scene.end_tick();
        this->targets.push_back(this->player);
        this->minimum_ammo = this->player->weapon_ammo();
        this->require_clear();
    }

    void Run::require_clear() const
    {
        const RectangleF& body = this->player->rectangle();
        const RectangleF bounds = this->services.info.out_of_bounds_rectangle();
        require(body.left() >= bounds.left() - tolerance && body.right() <= bounds.right() + tolerance &&
            body.top() >= bounds.top() - tolerance && body.bottom() <= bounds.bottom() + tolerance,
            "Turbulence route left whole-body bounds at tick " + std::to_string(this->ticks));
        for (const auto& [surface, object] : this->structures)
        {
            if (surface->one_way()) { continue; }
            const auto hit = narrow_phase(*this->player->shape(), *object->shape());
            require(!hit || hit->penetration <= tolerance,
                "Turbulence body overlaps " + surface->name + " at tick " + std::to_string(this->ticks));
        }
    }

    Player& Run::add_target(const Vector2F& position, PlayerTeam team)
    {
        require(this->target_player == nullptr && this->ticks == 0, "A single real target is constructed before the first tick");
        const RectangleF body(position, this->services.tuning.player().size);
        require(this->services.info.out_of_bounds_rectangle().contains(body), "Real target body leaves the authored arena");
        for (const auto& [surface, object] : this->structures)
        {
            if (surface->one_way()) { continue; }
            const auto hit = narrow_phase(body, *object->shape());
            require(!hit || hit->penetration <= tolerance, "Real target starts inside " + surface->name);
        }
        auto weapon = WeaponBuilder::build_weapon(&this->services.tuning, team, 1, 1, Colour::white,
            WeaponType::sprayer, body.center(), &this->services.render, &this->services.audio, &this->services.preferences,
            &this->services.presentation.tool_motion(), &this->services.presentation.weapon_feedback());
        this->target_player = this->scene.add(std::make_unique<Player>(body, &this->services.tuning,
            &this->services.presentation.movement_feedback(), &this->services.presentation.combat_feedback(),
            &this->services.presentation.respawn_redraw(), &this->services.render, &this->services.audio, &this->services.preferences,
            1, team, WeaponType::sprayer, std::move(weapon), WeaponType::none, Colour::white, Viewport{}, position));
        this->targets.push_back(this->target_player);
        this->scene.end_tick();
        return *this->target_player;
    }

    void Run::tick(const PlayerInputData& actions)
    {
        ++this->ticks;
        this->player->set_player_input(actions);
        if (this->target_player != nullptr) { this->target_player->set_player_input(this->target_input); }
        this->scene.update(dt);
        for (CollisionObject* object : this->scene.collision_objects())
        {
            if (is_projectile(to_collision_type(object->tag())))
            { dynamic_cast<Projectile&>(*object).resolve_movement_contacts(this->targets); }
        }
        const float before = this->player->weapon_ammo();
        auto emitted = this->player->update_weapon_and_get_projectiles(dt, this->targets);
        if (this->player->weapon_ammo() < before) { ++this->paid_events; this->last_paid_tick = this->ticks; }
        if (this->on_weapon_launch) { this->on_weapon_launch(emitted); }
        for (auto& projectile : emitted)
        { ++this->emitted_objects; this->scene.add(std::move(projectile)); }
        if (this->target_player != nullptr && this->target_weapon_updates)
        {
            const float target_before = this->target_player->weapon_ammo();
            auto target_emitted = this->target_player->update_weapon_and_get_projectiles(dt, this->targets);
            if (this->target_player->weapon_ammo() < target_before)
            { ++this->target_paid_events; this->target_last_paid_tick = this->ticks; }
            if (this->on_target_weapon_launch) { this->on_target_weapon_launch(target_emitted); }
            for (auto& projectile : target_emitted)
            { ++this->target_emitted_objects; this->scene.add(std::move(projectile)); }
        }
        this->scene.resolve();
        this->player->end_contacts(this->scene.contacts());
        this->player->update_weapon_position();
        this->player->update_prev_rectangle();
        if (this->target_player != nullptr)
        {
            this->target_player->end_contacts(this->scene.contacts());
            this->target_player->update_weapon_position();
            this->target_player->update_prev_rectangle();
        }
        if (this->after_scene_contacts) { this->after_scene_contacts(); }
        this->scene.end_tick();
        require(this->death_expected || this->player->state() == PlayerState::alive, "Unexpected route death");
        this->require_clear();
        this->minimum_ammo = std::min(this->minimum_ammo, this->player->weapon_ammo());
        for (const auto& effect : this->player->movement_feedback().effects())
        { if (effect.age == 0.0f && effect.kind == MovementFeedbackKind::takeoff) { ++this->takeoffs; } }
    }

    void Run::settle(int count)
    {
        for (int tick = 0; tick < count; ++tick) { this->tick(); }
        require(supported(*this->player), "Turbulence trial failed to settle");
    }

    std::string Run::support() const
    {
        const RectangleF& body = this->player->rectangle();
        for (const auto& [surface, object] : this->structures)
        {
            (void)object;
            if (!surface->ramp() && std::fabs(body.bottom() - surface->rectangle.top()) <= tolerance &&
                body.right() > surface->rectangle.left() && body.left() < surface->rectangle.right())
            { return surface->name; }
        }
        return "air-or-ramp";
    }

    float reflected(bool right, float centre_x) { return right ? centre_x : 4000.0f - centre_x; }

    Vector2F standing(const Services& services, const Surface& surface, float centre_x)
    { return { centre_x - services.tuning.player().size.x / 2.0f, surface.rectangle.top() - services.tuning.player().size.y }; }

    float steering(const Run& run, float centre_x)
    {
        const float error = centre_x - run.player->center().x;
        const float velocity = run.player->velocity().x;
        const float stopping = velocity * velocity / (2.0f * run.services.tuning.player().x_deacceleration) +
            std::fabs(velocity) * dt + 4.0f;
        if (std::fabs(error) < 5.0f || (velocity != 0.0f &&
            (velocity > 0.0f) == (error > 0.0f) && std::fabs(error) <= stopping)) { return 0.0f; }
        return error > 0.0f ? 1.0f : -1.0f;
    }

    void support_is(const Run& run, const std::string& support)
    { require(supported(*run.player) && run.support() == support, "Expected " + support + ", found " + run.support()); }

    void drive(Run& run, float centre_x, int limit)
    {
        for (int tick = 0; tick < limit; ++tick)
        {
            if (std::fabs(run.player->center().x - centre_x) <= 8.0f &&
                run.player->velocity().x == 0.0f && supported(*run.player)) { return; }
            run.tick(input(steering(run, centre_x)));
        }
        throw std::runtime_error("Turbulence walk failed x=" + std::to_string(centre_x) + " current=" +
            std::to_string(run.player->center().x) + " support=" + run.support());
    }

    void jump(Run& run, const std::string& target, float centre_x, int delay, int hold_ticks)
    {
        require(supported(*run.player), "Jump must start supported");
        for (int tick = 0; tick < delay + 2; ++tick) { run.tick(input(steering(run, centre_x))); }
        const int takeoffs = run.takeoffs;
        run.tick(input(steering(run, centre_x), true, true));
        for (int tick = 0; tick < 240; ++tick)
        {
            if (run.takeoffs > takeoffs && supported(*run.player) && run.support() == target &&
                std::fabs(run.player->center().x - centre_x) <= 8.0f && run.player->velocity().x == 0.0f)
            {
                require(run.takeoffs == takeoffs + 1, "Single edge required multiple jumps");
                const RectangleF& landing = run.services.named(target).rectangle;
                require(run.player->rectangle().left() >= landing.left() - tolerance &&
                    run.player->rectangle().right() <= landing.right() + tolerance,
                    "Landing did not fit the full player body on " + target);
                run.tick();
                return;
            }
            run.tick(input(steering(run, centre_x), false, tick + 1 < hold_ticks));
        }
        throw std::runtime_error("Turbulence jump failed target=" + target + " x=" + std::to_string(centre_x) +
            " delay=" + std::to_string(delay) + " hold=" + std::to_string(hold_ticks) +
            " current=" + std::to_string(run.player->center().x) + " y=" +
            std::to_string(run.player->rectangle().y) + " support=" + run.support());
    }
}
