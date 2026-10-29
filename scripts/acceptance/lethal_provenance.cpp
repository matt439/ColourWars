#include "game/pch.h"
#include "game/objects/player.h"
#include "game/objects/projectile_spray.h"
#include "game/objects/projectile_jet.h"
#include "game/objects/projectile_rolling.h"
#include "game/objects/weapon_builder.h"
#include "game/presentation.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/file_path.h"
#include "engine/core/step_timer.h"
#include "engine/scene/scene.h"
#include "scripts/acceptance/support/probe.h"
#include <array>
#include <bit>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace labrador;
using namespace mattmath;

namespace
{
    using acceptance::require;

    std::uint64_t credited(const Player& player)
    {
        std::uint64_t result = 0;
        for (std::uint64_t count : player.lethal_projectiles().enemy_by_owner) { result += count; }
        return result;
    }

    class World final
    {
    public:
        explicit World(const std::filesystem::path& content) :
            tuning(path_to_utf8(content / "tuning.json").c_str()),
            presentation(path_to_utf8(content / "presentation.json").c_str())
        {
            for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
            {
                render.add_sprite_sheet(name, read_sprite_sheet(path_to_utf8(content / "textures" /
                    path_from_utf8(std::string(name) + ".json")).c_str(), TextureHandle{}));
            }
            audio.add_sound_bank("paint_exe", SoundBank::silent());
        }
        std::unique_ptr<Player> player(int view = 0, PlayerTeam team = PlayerTeam::a)
        {
            const RectangleF rectangle(0.0f, 0.0f, tuning.player().size.x, tuning.player().size.y);
            auto weapon = WeaponBuilder::build_weapon(&tuning, team, paint_owners::index(-1, view),
                view, Colour::white, WeaponType::sprayer, rectangle.center(), &render, &audio, &preferences);
            return std::make_unique<Player>(rectangle, &tuning, &presentation.movement_feedback(),
                &presentation.combat_feedback(), &presentation.respawn_redraw(), &render, &audio,
                &preferences, -1, team, WeaponType::sprayer, std::move(weapon), WeaponType::none,
                Colour::white, Viewport{}, rectangle.top_left());
        }
        std::unique_ptr<ProjectileSpray> shot(PlayerTeam team, int owner,
            const RectangleF& rectangle = RectangleF(0, 0, 200, 200))
        {
            return std::make_unique<ProjectileSpray>(rectangle, Vector2F::ZERO, team, owner, Colour::white,
                &render, resolve_projectile_strip(tuning.projectile(ProjectileType::spray), render),
                &tuning.projectile(ProjectileType::spray));
        }
        void hit(Player& victim, const CollisionObject& shot) const
        {
            const float before = victim.health();
            victim.on_contact(shot, Vector2F::ZERO, 0.0f);
            const float expected = before - tuning.projectile(ProjectileType::spray).details.player_damage;
            require(std::bit_cast<std::uint32_t>(victim.health()) == std::bit_cast<std::uint32_t>(expected),
                "Passive provenance changed the existing contact damage arithmetic");
        }
        void eliminate(Player& victim, const CollisionObject& shot) const
        {
            for (int count = 0; count < 10000 && victim.health() > 0.0f; ++count) { hit(victim, shot); }
            require(victim.health() <= 0.0f, "Real projectile fixture did not reach a lethal health crossing");
        }
        GameTuning tuning;
        Presentation presentation;
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
    };

    class ProjectileTagProxy final : public CollisionObject
    {
    public:
        void update(float) override {}
        void draw(DrawList&) const override {}
        RectangleF bounds() const override { return rectangle_; }
        const Shape* shape() const override { return &rectangle_; }
        CollisionLayer layer() const override { return collision_layers::PROJECTILE_TEAM_B; }
        CollisionMask mask() const override { return collision_layers::PLAYER_TEAM_A; }
        CollisionTag tag() const override { return to_collision_tag(CollisionObjectType::projectile_spray_team_b); }
        void on_contact(const CollisionObject&, const Vector2F&, float) override {}
        bool for_deletion() const override { return false; }
    private:
        RectangleF rectangle_{ 0, 0, 200, 200 };
    };

    void fatal_crossing_and_respawn(World& world)
    {
        auto victim = world.player();
        auto first = world.shot(PlayerTeam::b, paint_owners::index(-1, 1));
        auto competing = world.shot(PlayerTeam::b, paint_owners::index(-1, 3));
        const int owner = first->player_num(), other_owner = competing->player_num();
        const float damage = world.tuning.projectile(ProjectileType::spray).details.player_damage;
        require(damage > 0.0f && damage < victim->health(), "Fixture needs real nonfatal spray damage");
        world.hit(*victim, *first);
        require(victim->health() > 0.0f && credited(*victim) == 0 && victim->lethal_projectiles().uncredited == 0,
            "A nonfatal hit received elimination credit");
        world.eliminate(*victim, *first);
        require(victim->state() == PlayerState::alive &&
            victim->lethal_projectiles().enemy_by_owner.at(static_cast<std::size_t>(owner)) == 1,
            "Terminal health crossing was not credited before the later dead-state transition");
        world.hit(*victim, *competing); world.hit(*victim, *first);
        require(credited(*victim) == 1 &&
            victim->lethal_projectiles().enemy_by_owner.at(static_cast<std::size_t>(other_owner)) == 0,
            "Repeated/competing same-tick contacts duplicated or stole the fatal credit");
        const PlayerLethalProjectiles snapshot = victim->lethal_projectiles();
        const float health = victim->health(), ammo = victim->weapon_ammo();
        std::srand(731u); const int expected_random = std::rand(); std::srand(731u);
        for (int read = 0; read < 8; ++read) { require(victim->lethal_projectiles() == snapshot, "Snapshot changed across reads"); }
        require(std::rand() == expected_random && victim->health() == health && victim->weapon_ammo() == ammo,
            "Read-only provenance snapshot changed RNG or gameplay state");
        const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::TicksPerSecond / 60));
        victim->update(dt);
        require(victim->state() == PlayerState::dead && victim->lethal_projectiles() == snapshot,
            "Entering the dead state changed already-recorded credit");
        victim->update(world.tuning.player().respawn_delay + dt);
        require(victim->state() == PlayerState::alive && victim->lethal_projectiles() == snapshot,
            "Timed respawn cleared match-cumulative provenance");
        world.eliminate(*victim, *competing);
        require(credited(*victim) == 2 &&
            victim->lethal_projectiles().enemy_by_owner.at(static_cast<std::size_t>(other_owner)) == 1,
            "A later life did not receive its own fatal-owner credit");
        victim->force_respawn();
        require(credited(*victim) == 2, "Explicit normal respawn path cleared match totals");
    }

    void immunity_debug_and_unknown(World& world)
    {
        auto shot = world.shot(PlayerTeam::b, paint_owners::index(-1, 1));
        auto immune = world.player(); const float initial = immune->health();
        immune->set_damage_immune(true);
        for (int count = 0; count < 8; ++count) { immune->on_contact(*shot, Vector2F::ZERO, 0.0f); }
        require(immune->health() == initial && credited(*immune) == 0 && immune->lethal_projectiles().uncredited == 0,
            "Immunity created damage or provenance");
        auto killed = world.player(); killed->kill(); world.hit(*killed, *shot);
        require(credited(*killed) == 0 && killed->lethal_projectiles().uncredited == 0,
            "A debug kill or later contact fabricated a projectile elimination");
        for (int owner : { -1, paint_owners::count, paint_owners::count + 17 })
        {
            auto victim = world.player(); auto invalid = world.shot(PlayerTeam::b, owner);
            world.eliminate(*victim, *invalid);
            require(credited(*victim) == 0 && victim->lethal_projectiles().uncredited == 1,
                "Invalid projectile owner was credited or lost instead of reported uncredited");
        }
        // Contacts dispatch on the other party's class (Collider), never its
        // tag: an engine object that only carries a projectile tag is refused.
        ProjectileTagProxy proxy;
        auto tagged = world.player(); const float untouched = tagged->health();
        bool refused = false;
        try { tagged->on_contact(proxy, Vector2F::ZERO, 0.0f); }
        catch (const std::logic_error&) { refused = true; }
        require(refused && tagged->health() == untouched && credited(*tagged) == 0 &&
            tagged->lethal_projectiles().uncredited == 0,
            "A tag-only collision proxy was answered by its tag instead of refused");
        // Deliberately bypass masks here to preserve the pre-existing public
        // callback behavior while proving that friendly metadata never earns credit.
        auto forced = world.player(); auto friendly = world.shot(PlayerTeam::a, paint_owners::index(-1, 2));
        world.eliminate(*forced, *friendly);
        require(credited(*forced) == 0 && forced->lethal_projectiles().uncredited == 1,
            "Forced friendly callback acquired enemy credit");
    }

    void actual_masks(World& world)
    {
        Scene scene(nullptr, nullptr);
        scene.set_bounds(RectangleF(-10000, -10000, 20000, 20000));
        Player* victim = scene.add(world.player());
        scene.add(world.shot(PlayerTeam::a, paint_owners::index(-1, 0))); // Self.
        scene.add(world.shot(PlayerTeam::a, paint_owners::index(-1, 2))); // Friendly other seat.
        scene.end_tick();
        const float health = victim->health();
        scene.resolve();
        require(victim->health() == health && scene.contacts().empty() && credited(*victim) == 0 &&
            victim->lethal_projectiles().uncredited == 0,
            "Actual collision masks admitted an overlapping self/friendly projectile");
        scene.add(world.shot(PlayerTeam::b, paint_owners::index(-1, 1)));
        scene.end_tick(); scene.resolve();
        require(victim->health() < health && !scene.contacts().empty(),
            "Mask fixture did not exercise a real enemy contact");
    }

    void launch_contact(World& world)
    {
        auto victim = world.player();
        auto earlier = world.shot(PlayerTeam::b, paint_owners::index(-1, 1));
        const float damage = world.tuning.projectile(ProjectileType::spray).details.player_damage;
        for (int hit = 0; hit < 10000 && victim->health() > damage; ++hit) { world.hit(*victim, *earlier); }
        require(victim->health() > 0.0f && victim->health() <= damage && credited(*victim) == 0,
            "Cannot prepare a real terminal launch-contact fixture");
        const Vector2F target = victim->center();
        auto fatal = world.shot(PlayerTeam::b, paint_owners::index(-1, 3),
            RectangleF(target.x, target.y, 10, 10));
        std::array<CollisionObject*, 1> target_objects{ victim.get() };
        fatal->resolve_launch_obstruction(Vector2F(target.x - 500.0f, target.y), target_objects);
        require(victim->health() <= 0.0f && victim->state() == PlayerState::alive && fatal->for_deletion() &&
            victim->lethal_projectiles().enemy_by_owner.at(static_cast<std::size_t>(fatal->player_num())) == 1,
            "Real launch sweep lost the fatal projectile's provenance");
    }
    void movement_contact(World& world)
    {
        auto victim = world.player();
        auto earlier = world.shot(PlayerTeam::b, paint_owners::index(-1, 1));
        const ProjectileTuning& jet = world.tuning.projectile(ProjectileType::jet);
        const float damage = jet.details.player_damage;
        require(damage > 0.0f && damage < victim->health(), "Fixture needs real nonfatal jet damage");
        for (int hit = 0; hit < 10000 && victim->health() > damage; ++hit) { world.hit(*victim, *earlier); }
        require(victim->health() > 0.0f && victim->health() <= damage && credited(*victim) == 0,
            "Cannot prepare a real terminal movement-contact fixture");
        const Vector2F target = victim->center();
        const int owner = paint_owners::index(-1, 3);
        ProjectileJet fatal(RectangleF(Vector2F(target.x - 200.0f, target.y), jet.details.col_rect_size),
            Vector2F(4000.0f, 0.0f), PlayerTeam::b, owner, Colour::white, &world.render,
            resolve_projectile_strip(jet, world.render), &jet);
        const RectangleF obstacle = victim->shape()->bounding_box();
        const RectangleF start = fatal.shape()->bounding_box();
        require(!start.intersects(obstacle) && start.right() < obstacle.left(),
            "Movement sweep must begin clear of the victim on its left");
        fatal.update(0.1f);
        const RectangleF finish = fatal.shape()->bounding_box();
        require(!finish.intersects(obstacle) && finish.left() > obstacle.right() && !fatal.for_deletion(),
            "Movement sweep needs a live projectile whose endpoint has passed clear of the victim");
        const float before = victim->health();
        std::array<CollisionObject*, 1> target_objects{ victim.get() };
        fatal.resolve_movement_contacts(target_objects);
        const float expected = before - damage;
        require(std::bit_cast<std::uint32_t>(victim->health()) == std::bit_cast<std::uint32_t>(expected) &&
            victim->health() <= 0.0f && victim->state() == PlayerState::alive && fatal.for_deletion() &&
            credited(*victim) == 1 && victim->lethal_projectiles().uncredited == 0 &&
            victim->lethal_projectiles().enemy_by_owner.at(static_cast<std::size_t>(owner)) == 1,
            "Real movement sweep lost or duplicated the terminal projectile owner's credit");
        const PlayerLethalProjectiles snapshot = victim->lethal_projectiles();
        fatal.resolve_movement_contacts(target_objects);
        require(victim->lethal_projectiles() == snapshot && victim->health() == expected,
            "A retired movement projectile repeated its fatal contact");
    }

    void shared_roller_contact(World& world)
    {
        Scene scene(nullptr, nullptr);
        scene.set_bounds(RectangleF(-10000, -10000, 20000, 20000));
        std::array<std::unique_ptr<Player>, 2> pending{ world.player(0), world.player(2) };
        auto earlier = world.shot(PlayerTeam::b, paint_owners::index(-1, 1));
        const ProjectileTuning& rolling = world.tuning.projectile(ProjectileType::rolling);
        const float damage = rolling.details.player_damage;
        std::array<Player*, 2> victims{};
        std::array<float, 2> health{};
        for (std::size_t index = 0; index < pending.size(); ++index)
        {
            Player& victim = *pending[index];
            require(damage > 0.0f && damage < victim.health(), "Fixture needs real nonfatal rolling damage");
            for (int hit = 0; hit < 10000 && victim.health() > damage; ++hit) { world.hit(victim, *earlier); }
            require(victim.health() > 0.0f && victim.health() <= damage && credited(victim) == 0 &&
                victim.lethal_projectiles().uncredited == 0, "Cannot prepare two terminal roller victims");
            health[index] = victim.health();
            victims[index] = scene.add(std::move(pending[index]));
        }
        const int owner = paint_owners::index(-1, 3);
        ProjectileRolling* fatal = scene.add(std::make_unique<ProjectileRolling>(
            RectangleF(victims[0]->center(), rolling.details.col_rect_size), Vector2F::ZERO,
            PlayerTeam::b, owner, Colour::white, &world.render,
            resolve_projectile_strip(rolling, world.render), &rolling));
        const float dt = static_cast<float>(StepTimer::TicksToSeconds(StepTimer::TicksPerSecond / 60));
        fatal->update(dt); // Real first-step lifetime grace keeps the shared volume available.
        require(!fatal->for_deletion() &&
            fatal->shape()->bounding_box().intersects(victims[0]->shape()->bounding_box()) &&
            fatal->shape()->bounding_box().intersects(victims[1]->shape()->bounding_box()),
            "Shared roller fixture must overlap both victims after its real first update");
        scene.end_tick(); scene.resolve();
        require(scene.contacts().size() == 2 && !fatal->for_deletion(),
            "One shared roller volume did not retain both real Scene contacts");
        std::array<PlayerLethalProjectiles, 2> snapshots{};
        for (std::size_t index = 0; index < victims.size(); ++index)
        {
            const Player& victim = *victims[index];
            const float expected = health[index] - damage;
            require(std::bit_cast<std::uint32_t>(victim.health()) == std::bit_cast<std::uint32_t>(expected) &&
                victim.health() <= 0.0f && victim.state() == PlayerState::alive && credited(victim) == 1 &&
                victim.lethal_projectiles().uncredited == 0 &&
                victim.lethal_projectiles().enemy_by_owner.at(static_cast<std::size_t>(owner)) == 1,
                "Shared roller failed to credit both terminal enemy contacts exactly once");
            snapshots[index] = victim.lethal_projectiles();
        }
        scene.resolve();
        require(!fatal->for_deletion() && victims[0]->lethal_projectiles() == snapshots[0] &&
            victims[1]->lethal_projectiles() == snapshots[1],
            "Repeated shared-volume contacts duplicated fatal credit");
    }
}

int wmain(int argc, wchar_t* argv[])
{
    return acceptance::run_probe("lethal provenance", [&]
    {
        require(argc == 2, "usage: lethal_provenance <content>");
        World world(std::filesystem::absolute(argv[1]));
        fatal_crossing_and_respawn(world);
        immunity_debug_and_unknown(world);
        actual_masks(world);
        launch_contact(world);
        movement_contact(world);
        shared_roller_contact(world);
        std::cout << "PASS lethal provenance: real nonfatal/fatal contacts before dead state, competing/repeated hits, "
            "timed/forced respawn persistence, immunity/debug kill, invalid owners/tag proxy, actual self/friendly masks, "
            "real launch/movement sweeps, shared roller double elimination and read-only RNG/state invariance. No environmental or sampled-death kill inference.\n";
        return 0;
    });
}
