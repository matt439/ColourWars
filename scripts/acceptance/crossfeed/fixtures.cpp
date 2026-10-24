#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include "engine/math/triangle.h"

namespace crossfeed_acceptance
{
    void require(bool condition, const std::string& message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

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
                require(previous.name != surface.name, "duplicate Crossfeed object name '" + surface.name + "'");
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
        throw std::runtime_error("Crossfeed route needs authored object '" + name + "'");
    }

    Services::Services(const std::filesystem::path& content) :
        tuning((content / "tuning.json").string().c_str()),
        presentation((content / "presentation.json").string().c_str()),
        map(content / "levels/crossfeed.json"),
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
        require(built.size() == with.map.surfaces.size(), "Crossfeed builder changed the authored object count");
        for (size_t index = 0; index < built.size(); ++index)
        {
            CollisionObject* object = this->scene.add(std::move(built[index]));
            const Surface& authored = with.map.surfaces[index];
            require(object->bounds() == authored.rectangle,
                "Crossfeed built bounds differ from '" + authored.name + "'");
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
        this->require_clear(*this->player, context);
    }

    void Run::require_clear(const Player& subject, const std::string& context) const
    {
        const RectangleF& body = subject.rectangle();
        const RectangleF bounds = this->services.map.info.out_of_bounds_rectangle();
        require(body.left() >= bounds.left() - tolerance && body.right() <= bounds.right() + tolerance &&
            body.top() >= bounds.top() - tolerance && body.bottom() <= bounds.bottom() + tolerance,
            context + ": Crossfeed player body left authored bounds");
        for (const auto& [surface, object] : this->structures)
        {
            if (surface->jump_through()) { continue; }
            const auto overlap = narrow_phase(*subject.shape(), *object->shape());
            require(!overlap || overlap->penetration <= tolerance,
                context + ": player intersects '" + surface->name + "' at tick " +
                std::to_string(this->ticks) + " body=(" + fixed(body.x) + ',' + fixed(body.y) + ')');
        }
    }

    void Run::tick(const PlayerInputData& actions)
    {
        ++this->ticks;
        this->player->set_player_input(actions);
        require(this->passive_players.size() == this->passive_input_streams.size() &&
            this->passive_players.size() == this->passive_weapon_aims.size() &&
            this->passive_players.size() == this->passive_weapon_stats.size(),
            "Crossfeed opponent input streams lost their player association");
        for (size_t index = 0; index < this->passive_players.size(); ++index)
        {
            const auto& stream = this->passive_input_streams[index];
            this->passive_players[index]->set_player_input(stream ? stream(this->ticks) : input());
        }
        this->scene.update(dt);
        if (this->after_scene_update) { this->after_scene_update(); }
        const float before_sweep = this->team_area(this->player->team(), this->swept_surface);
        for (CollisionObject* object : this->scene.collision_objects())
        {
            if (is_projectile(to_collision_type(object->tag())))
            {
                auto& projectile = dynamic_cast<Projectile&>(*object);
                const bool retired_before = projectile.for_deletion();
                projectile.resolve_movement_contacts(this->targets);
                this->swept_retired_objects += !retired_before && projectile.for_deletion() ? 1 : 0;
                if (!this->interception_surface.empty())
                {
                    const RectangleF& cover = this->services.map.named(this->interception_surface).rectangle;
                    if (const auto impact = projectile.take_blocked_contact())
                    {
                        if (impact->position.y >= cover.top() - tolerance &&
                            impact->position.y <= cover.bottom() + tolerance &&
                            (approximately(impact->position.x, cover.left()) ||
                                approximately(impact->position.x, cover.right())))
                        {
                            ++this->cover_contacts;
                            if (this->first_cover_tick < 0)
                            {
                                this->first_cover_tick = this->ticks;
                                this->first_cover_point = impact->position;
                            }
                        }
                    }
                }
            }
        }
        this->swept_paint += this->team_area(this->player->team(), this->swept_surface) - before_sweep;
        if (this->weapon_aim_after_movement)
        {
            this->player->set_player_input(aimed(this->weapon_aim_after_movement(), actions));
        }
        const float ammo_before = this->player->weapon_ammo();
        const float paint_before_launch = this->team_area(this->player->team(), this->swept_surface);
        auto emitted = this->player->update_weapon_and_get_projectiles(dt, this->targets);
        const bool paid = this->player->weapon_ammo() < ammo_before;
        if (paid && this->first_paid_tick < 0)
        {
            this->first_paid_tick = this->ticks;
            this->first_paid_velocity = this->player->velocity();
            this->first_paid_nozzle = this->player->primary_weapon().nozzle_pos();
        }
        this->paid_events += paid ? 1 : 0;
        this->paid_events_without_flight += paid && emitted.empty() ? 1 : 0;
        if (paid)
        {
            this->paid_ticks.push_back(this->ticks);
            // The builder emits five mist pellets and one object for every
            // other primary. Player removes launch contacts before returning
            // survivors; these retirements include enemies as well as cover.
            const int created = this->player->primary_weapon().type() == WeaponType::mister ? 5 : 1;
            require(emitted.size() <= static_cast<size_t>(created),
                "Crossfeed paid-event object count exceeds the projectile builder contract");
            this->launch_retired_objects += created - static_cast<int>(emitted.size());
        }
        this->launch_paint += this->team_area(this->player->team(), this->swept_surface) - paint_before_launch;
        if (this->on_weapon_launch) { this->on_weapon_launch(emitted); }
        for (auto& projectile : emitted)
        {
            ++this->emitted_objects;
            this->scene.add(std::move(projectile));
        }
        // Level launches in player order after all moving projectile contacts,
        // before Scene::resolve. Additional players run the same weapon phase,
        // including release, cooldown and recovery when their fire is absent.
        for (size_t index = 0; index < this->passive_players.size(); ++index)
        {
            Player* opponent = this->passive_players[index];
            if (this->passive_weapon_aims[index])
            {
                opponent->set_player_input(aimed(this->passive_weapon_aims[index](), opponent->input()));
            }
            const float opponent_ammo = opponent->weapon_ammo();
            auto opponent_emitted = opponent->update_weapon_and_get_projectiles(dt, this->targets);
            OpponentWeaponStats& stats = this->passive_weapon_stats[index];
            if (opponent->weapon_ammo() < opponent_ammo)
            {
                ++stats.paid_events;
                stats.paid_ticks.push_back(this->ticks);
                if (stats.first_paid_tick < 0) { stats.first_paid_tick = this->ticks; }
                const int created = opponent->primary_weapon().type() == WeaponType::mister ? 5 : 1;
                require(opponent_emitted.size() <= static_cast<size_t>(created),
                    "Crossfeed opponent paid-event object count exceeds the projectile builder contract");
                stats.launch_retired_objects += created - static_cast<int>(opponent_emitted.size());
            }
            stats.minimum_ammo = std::min(stats.minimum_ammo, opponent->weapon_ammo());
            for (auto& projectile : opponent_emitted)
            {
                ++stats.emitted_objects;
                this->scene.add(std::move(projectile));
            }
        }
        this->scene.resolve();
        this->player->end_contacts(this->scene.contacts());
        this->player->update_weapon_position();
        this->player->update_prev_rectangle();
        for (Player* passive : this->passive_players)
        {
            passive->end_contacts(this->scene.contacts());
            passive->update_weapon_position();
            passive->update_prev_rectangle();
        }
        if (this->after_scene_contacts) { this->after_scene_contacts(); }
        this->scene.end_tick();
        require(this->death_expected || this->player->state() == PlayerState::alive,
            "Crossfeed route player died without the damage fixture");
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
        require(supported(*this->player), "Crossfeed player did not find support after settling");
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
        require(this->player->health() <= 0.0f, "Crossfeed damage fixture did not eliminate the player");
    }

    Player* Run::add_opponent(const Vector2F& top_left, WeaponType weapon_type)
    {
        const PlayerTeam team = this->player->team() == PlayerTeam::a ? PlayerTeam::b : PlayerTeam::a;
        const RectangleF body(top_left, this->services.tuning.player().size);
        auto weapon = WeaponBuilder::build_weapon(&this->services.tuning, team, 1, 1, Colour::white,
            weapon_type, body.center(), &this->services.render, &this->services.audio,
            &this->services.preferences, &this->services.presentation.tool_motion(),
            &this->services.presentation.weapon_feedback());
        Player* opponent = this->scene.add(std::make_unique<Player>(body, &this->services.tuning,
            &this->services.presentation.movement_feedback(), &this->services.presentation.combat_feedback(),
            &this->services.presentation.respawn_redraw(), &this->services.render, &this->services.audio,
            &this->services.preferences, 1, team, weapon_type, std::move(weapon), WeaponType::none,
            Colour::white, Viewport{}, top_left));
        this->targets.push_back(opponent);
        this->passive_players.push_back(opponent);
        this->passive_input_streams.emplace_back();
        this->passive_weapon_aims.emplace_back();
        OpponentWeaponStats stats;
        stats.minimum_ammo = opponent->weapon_ammo();
        this->passive_weapon_stats.push_back(std::move(stats));
        this->scene.end_tick();
        return opponent;
    }

    void Run::set_opponent_input(Player* opponent, PlayerInputStream stream)
    {
        for (size_t index = 0; index < this->passive_players.size(); ++index)
        {
            if (this->passive_players[index] == opponent)
            {
                this->passive_input_streams[index] = std::move(stream);
                return;
            }
        }
        throw std::runtime_error("Crossfeed input stream needs an opponent owned by this Run");
    }

    void Run::set_opponent_weapon_aim(Player* opponent, std::function<float()> aim)
    {
        for (size_t index = 0; index < this->passive_players.size(); ++index)
        {
            if (this->passive_players[index] == opponent)
            {
                this->passive_weapon_aims[index] = std::move(aim);
                return;
            }
        }
        throw std::runtime_error("Crossfeed aim stream needs an opponent owned by this Run");
    }

    const Run::OpponentWeaponStats& Run::opponent_weapon_stats(const Player* opponent) const
    {
        for (size_t index = 0; index < this->passive_players.size(); ++index)
        {
            if (this->passive_players[index] == opponent) { return this->passive_weapon_stats[index]; }
        }
        throw std::runtime_error("Crossfeed weapon stats need an opponent owned by this Run");
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
        const RectangleF& body = run.player->rectangle();
        const Vector2F velocity = run.player->velocity();
        std::string nearby;
        for (const auto& [surface, object] : run.structures)
        {
            (void)object;
            const RectangleF& box = surface->rectangle;
            if (box.right() < body.left() - 1.0f || box.left() > body.right() + 1.0f ||
                box.bottom() < body.top() - 1.0f || box.top() > body.bottom() + 1.0f) { continue; }
            nearby += surface->name + "=(" + fixed(box.x) + ',' + fixed(box.y) + ',' +
                fixed(box.width) + ',' + fixed(box.height) + ";" + surface->collision_type + ") ";
        }
        throw std::runtime_error("Crossfeed walk from tick " + std::to_string(start) + " did not reach x=" +
            fixed(centre_x) + "; centre=" + fixed(run.player->center().x) + " support=" + run.support() +
            " body_x=" + fixed(body.x) + " body_y=" + fixed(body.y) + " body_bottom=" + fixed(body.bottom()) +
            " vx=" + fixed(velocity.x) + " vy=" + fixed(velocity.y) +
            " move_state=" + std::to_string(static_cast<int>(run.player->move_state())) + " nearby=" + nearby);
    }

    void jump_to(Run& run, const Surface& target, float centre_x, int delay, int hold_ticks)
    {
        require(supported(*run.player), "Crossfeed jump trial did not start supported");
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
            run.tick(input(steering(run, centre_x), false, tick + 1 < hold_ticks));
        }
        throw std::runtime_error("Crossfeed jump (hold " + std::to_string(hold_ticks) + ", delay " + std::to_string(delay) + ") failed to land on '" +
            target.name + "'; at " + fixed(run.player->center().x) + " on " + run.support());
    }
}
