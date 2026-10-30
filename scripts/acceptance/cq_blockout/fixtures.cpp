#include "game/pch.h"
#include "scripts/acceptance/cq_blockout/fixtures.h"
#include "engine/math/triangle.h"

namespace cq_blockout_acceptance
{
    std::string fixed(float value, int precision)
    {
        std::ostringstream text;
        text << std::fixed << std::setprecision(precision) << value;
        return text.str();
    }

    bool approximately(float first, float second, float allowed)
    {
        return std::fabs(first - second) <= allowed;
    }

    bool supported(const Player& player)
    {
        return player.move_state() == PlayerMoveState::on_ground ||
            player.move_state() == PlayerMoveState::on_drop_down_ground ||
            player.move_state() == PlayerMoveState::on_ramp_left ||
            player.move_state() == PlayerMoveState::on_ramp_right;
    }

    PlayerInputData input(float movement, bool pressed, bool held, bool drop)
    {
        PlayerInputData actions;
        actions.connected = true;
        actions.x_movement = movement;
        actions.jump_pressed = pressed;
        actions.jump_held = held;
        actions.drop_through = drop;
        return actions;
    }

    PlayerInputData aimed(float angle, PlayerInputData actions)
    {
        actions.shoot_direction_requested = true;
        actions.shoot_angle = angle;
        actions.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        return actions;
    }

    RectangleF mirror(const RectangleF& rectangle, const RectangleF& bounds)
    {
        return { bounds.left() + bounds.right() - rectangle.right(), rectangle.y,
            rectangle.width, rectangle.height };
    }

    Map::Map(const std::filesystem::path& path) : info(path.string().c_str())
    {
        this->info.validate();
        const JsonValue entries = this->info.collision_objects_json();
        for (size_t index = 0; index < entries.size(); ++index)
        {
            const JsonValue object = entries.at(index);
            Surface surface;
            surface.name = object.has("name") ? object.string("name") :
                "collision_" + std::to_string(index);
            surface.type = object.string("type");
            surface.collision_type = object.string("collision_type");
            if (surface.ramp())
            {
                const JsonValue shape = object.object("triangle");
                surface.rectangle = TriangleRightAxisAligned(shape.number("x1"), shape.number("y1"),
                    shape.number("x2"), shape.number("y2"), shape.number("x3"), shape.number("y3")).bounding_box();
            }
            else if (object.has("rectangle"))
            {
                const JsonValue shape = object.object("rectangle");
                surface.rectangle = { shape.number("x"), shape.number("y"),
                    shape.number("width"), shape.number("height") };
            }
            else
            {
                const JsonValue shape = object.object("rectangle_tlbr");
                surface.rectangle = RectangleF::from_top_left_bottom_right(
                    { shape.number("left"), shape.number("top") },
                    { shape.number("right"), shape.number("bottom") });
            }
            if (surface.type == "StructurePaintable")
            {
                const JsonValue faces = object.object("paintable_faces");
                surface.faces = { faces.boolean("top"), faces.boolean("bottom"),
                    faces.boolean("left"), faces.boolean("right") };
            }
            for (const Surface& previous : this->surfaces)
            {
                require(previous.name != surface.name, "duplicate CQ object name '" + surface.name + "'");
            }
            this->surfaces.push_back(std::move(surface));
        }
    }

    const Surface& Map::named(const std::string& name) const
    {
        for (const Surface& surface : this->surfaces)
        {
            if (surface.name == name) { return surface; }
        }
        throw std::runtime_error("CQ route needs authored object '" + name + "'");
    }

    Services::Services(const std::filesystem::path& content) :
        tuning((content / "tuning.json").string().c_str()),
        presentation((content / "presentation.json").string().c_str()),
        map(content / "levels/close_quarters.json"),
        builder(&this->render, &this->audio, &this->preferences, &this->tuning.paint())
    {
        for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
        {
            this->render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(), TextureHandle{}));
        }
        this->audio.add_sound_bank("paint_exe", SoundBank::silent());
    }

    Run::Run(Services& with, const Vector2F& top_left, PlayerTeam team, WeaponType weapon) : services(with)
    {
        this->scene.set_bounds(with.map.info.out_of_bounds_rectangle());
        auto built = with.builder.build_collision_objects(with.map.info.collision_objects_json(), with.colours);
        require(built.size() == with.map.surfaces.size(), "CQ builder changed the authored object count");
        for (size_t index = 0; index < built.size(); ++index)
        {
            CollisionObject* object = this->scene.add(std::move(built[index]));
            const Surface& authored = with.map.surfaces[index];
            require(object->bounds() == authored.rectangle,
                "CQ built bounds differ from '" + authored.name + "'");
            this->structures.emplace_back(&authored, object);
            this->targets.push_back(object);
        }
        const RectangleF body(top_left, with.tuning.player().size);
        auto primary = WeaponBuilder::build_weapon(&with.tuning, team, 0, 0, Colour::white,
            weapon, body.center(), &with.render, &with.audio, &with.preferences,
            &with.presentation.tool_motion(), &with.presentation.weapon_feedback());
        this->player = this->scene.add(std::make_unique<Player>(body, &with.tuning,
            &with.presentation.movement_feedback(), &with.presentation.combat_feedback(),
            &with.presentation.respawn_redraw(), &with.render, &with.audio, &with.preferences,
            0, team, weapon, std::move(primary), WeaponType::none, Colour::white,
            Viewport{}, top_left));
        this->targets.push_back(this->player);
        this->scene.end_tick();
        this->minimum_ammo = this->player->weapon_ammo();
        this->require_clear("construction");
    }

    void Run::require_clear(const std::string& context) const
    {
        const RectangleF& body = this->player->rectangle();
        const RectangleF bounds = this->services.map.info.out_of_bounds_rectangle();
        require(body.left() >= bounds.left() - tolerance && body.right() <= bounds.right() + tolerance &&
            body.top() >= bounds.top() - tolerance && body.bottom() <= bounds.bottom() + tolerance,
            context + ": CQ player body left authored bounds");
        for (const auto& [surface, object] : this->structures)
        {
            if (surface->jump_through()) { continue; }
            const auto overlap = narrow_phase(*this->player->shape(), *object->shape());
            require(!overlap || overlap->penetration <= tolerance,
                context + ": player intersects '" + surface->name + "' at tick " +
                std::to_string(this->ticks) + " body=(" + fixed(body.x) + ',' + fixed(body.y) + ')');
        }
    }

    void Run::tick(const PlayerInputData& actions)
    {
        ++this->ticks;
        this->player->set_player_input(actions);
        this->scene.update(dt);
        const float before_sweep = this->team_area(this->player->team(), this->swept_surface);
        for (CollisionObject* object : this->scene.collision_objects())
        {
            if (is_projectile(to_collision_type(object->tag())))
            {
                dynamic_cast<Projectile&>(*object).resolve_movement_contacts(this->targets);
            }
        }
        this->swept_paint += this->team_area(this->player->team(), this->swept_surface) - before_sweep;
        for (auto& projectile : this->player->update_weapon_and_get_projectiles(dt, this->targets))
        {
            ++this->shots;
            this->scene.add(std::move(projectile));
        }
        this->scene.resolve();
        this->player->end_contacts(this->scene.contacts());
        this->player->update_weapon_position();
        this->player->update_prev_rectangle();
        this->scene.end_tick();
        require(this->death_expected || this->player->state() == PlayerState::alive,
            "CQ route player died without the damage fixture");
        this->require_clear("after resolve");
        this->minimum_ammo = std::min(this->minimum_ammo, this->player->weapon_ammo());
        for (const auto& effect : this->player->movement_feedback().effects())
        {
            if (effect.age == 0.0f && effect.kind == MovementFeedbackKind::takeoff) { ++this->takeoffs; }
        }
    }

    void Run::settle(int count)
    {
        for (int tick = 0; tick < count; ++tick) { this->tick(); }
        require(supported(*this->player), "CQ player did not find support after settling");
    }

    std::string Run::support() const
    {
        const RectangleF& body = this->player->rectangle();
        for (const auto& [surface, object] : this->structures)
        {
            (void)object;
            if (!surface->ramp() && approximately(body.bottom(), surface->rectangle.top()) &&
                body.right() > surface->rectangle.left() && body.left() < surface->rectangle.right())
            {
                return surface->name;
            }
        }
        return "air-or-ramp";
    }

    float Run::team_area(PlayerTeam team, const std::string& name) const
    {
        float area = 0.0f;
        for (const auto& [surface, object] : this->structures)
        {
            if (!name.empty() && surface->name != name) { continue; }
            if (const auto* scoring = dynamic_cast<const PaintableObject*>(object))
            {
                const PaintTotal total = scoring->paint_total();
                area += team == PlayerTeam::a ? total.team_a : total.team_b;
            }
        }
        return area;
    }

    float Run::total_area() const
    {
        float area = 0.0f;
        for (const auto& [surface, object] : this->structures)
        {
            (void)surface;
            if (const auto* scoring = dynamic_cast<const PaintableObject*>(object))
            {
                area += scoring->paint_total().total;
            }
        }
        return area;
    }

    void Run::kill()
    {
        this->death_expected = true;
        const PlayerTeam enemy = this->player->team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
        const ProjectileSpray shot(RectangleF(0, 0, 10, 10), Vector2F::ZERO, enemy, 1,
            Colour::white, &this->services.render,
            resolve_projectile_strip(this->services.tuning.projectile(ProjectileType::spray), this->services.render),
            &this->services.tuning.projectile(ProjectileType::spray));
        for (int hit = 0; hit < 1000 && this->player->health() > 0.0f; ++hit)
        {
            this->player->on_contact(shot, Vector2F::ZERO, 0.0f);
        }
        require(this->player->health() <= 0.0f, "CQ damage fixture did not eliminate the player");
    }

    Vector2F standing_on(const Services& s, const Surface& surface, float centre_x)
    {
        return { centre_x - s.tuning.player().size.x / 2.0f,
            surface.rectangle.top() - s.tuning.player().size.y };
    }

    namespace
    {
        float steering(const Run& run, float target)
        {
            const float error = target - run.player->center().x;
            const float velocity = run.player->velocity().x;
            const float stopping = velocity * velocity /
                (2.0f * run.services.tuning.player().x_deacceleration) + std::fabs(velocity) * dt + 4.0f;
            if (std::fabs(error) < 5.0f || (velocity != 0.0f &&
                (velocity > 0.0f) == (error > 0.0f) && std::fabs(error) <= stopping)) { return 0.0f; }
            return error > 0.0f ? 1.0f : -1.0f;
        }
    }

    void drive_to(Run& run, float centre_x, int limit)
    {
        const int start = run.ticks;
        for (int tick = 0; tick < limit; ++tick)
        {
            if (std::fabs(run.player->center().x - centre_x) <= 8.0f &&
                run.player->velocity().x == 0.0f && supported(*run.player)) { return; }
            run.tick(input(steering(run, centre_x)));
        }
        throw std::runtime_error("CQ walk from tick " + std::to_string(start) + " did not reach x=" +
            fixed(centre_x) + "; stopped at " + fixed(run.player->center().x) + " on " + run.support());
    }

    void jump_to(Run& run, const Surface& target, float centre_x, int delay)
    {
        require(supported(*run.player), "CQ jump trial did not start supported");
        // Negative and positive variants shift the press by four simulation
        // ticks around the same approach; they never alter player placement.
        for (int tick = 0; tick < delay + 2; ++tick) { run.tick(input(steering(run, centre_x))); }
        const int previous_takeoffs = run.takeoffs;
        run.tick(input(steering(run, centre_x), true, true));
        for (int tick = 0; tick < 240; ++tick)
        {
            if (run.takeoffs > previous_takeoffs && supported(*run.player) &&
                run.support() == target.name && std::fabs(run.player->center().x - centre_x) <= 8.0f &&
                run.player->velocity().x == 0.0f) { run.tick(); return; }
            run.tick(input(steering(run, centre_x), false, true));
        }
        throw std::runtime_error("CQ held jump (delay " + std::to_string(delay) + ") failed to land on '" +
            target.name + "'; at " + fixed(run.player->center().x) + " on " + run.support());
    }
}
