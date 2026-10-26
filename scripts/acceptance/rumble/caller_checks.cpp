#include "game/pch.h"
#include "game/rumble_events.h"
#include "game/objects/projectile_spray.h"
#include "scripts/acceptance/moving_structure/fixtures.h"
#include <bit>
#include <cstdlib>
#include <iostream>
#if COLOURWARS_DEV_TOOLS
#include "game/debug/match_fingerprint.h"
#endif

namespace rumble_acceptance
{
namespace
{
    using namespace mattmath;
    using namespace labrador;
    using moving_structure_acceptance::World;
    using moving_structure_acceptance::Fixture;
    using moving_structure_acceptance::input;
    using moving_structure_acceptance::dt;
    using moving_structure_acceptance::require;

    void shorten_countdown(Fixture& fixture)
    {
        fixture.tuning = fixture.world.variant([](rapidjson::Document& document)
        {
            moving_structure_acceptance::set_countdown(document, dt / 4.0f);
        });
    }

    void all_empty(const std::vector<SlotRumbleEvents>& records)
    {
        for (const SlotRumbleEvents& record : records)
        {
            require(record.events.empty(), "Unexpected pending rumble event");
        }
    }

    ProjectileSpray shot(World& world, PlayerTeam team = PlayerTeam::b,
        int owner = input_slots::keyboard_and_mouse)
    {
        const ProjectileTuning& tuning = world.tuning.projectile(ProjectileType::spray);
        return ProjectileSpray(RectangleF(0.0f, 0.0f, 10.0f, 10.0f), Vector2F::ZERO,
            team, owner, world.palette.team_colour(team),
            &world.render, resolve_projectile_strip(tuning, world.render), &tuning);
    }

    std::vector<std::uint32_t> gameplay_bits(const Level& level)
    {
        std::vector<std::uint32_t> result;
        result.push_back(static_cast<std::uint32_t>(level.state()));
        for (int view = 0; view < level.player_count(); ++view)
        {
            const Player& player = level.player(view);
            result.push_back(static_cast<std::uint32_t>(player.state()));
            result.push_back(static_cast<std::uint32_t>(player.move_state()));
            for (const float value : { player.rectangle().x, player.rectangle().y,
                player.velocity().x, player.velocity().y, player.health(), player.weapon_ammo(),
                player.respawn_timer() })
            {
                result.push_back(std::bit_cast<std::uint32_t>(value));
            }
        }
        return result;
    }

    std::vector<SlotRumbleEvents> drain_without_gameplay_change(Level& level)
    {
        const auto before = gameplay_bits(level);
#if COLOURWARS_DEV_TOOLS
        const MatchFingerprint print = MatchFingerprint::of(level, 17);
#endif
        std::srand(271u);
        const int expected_random = std::rand();
        std::srand(271u);
        auto events = level.take_rumble_events();
        require(std::rand() == expected_random, "Draining rumble events consumed the gameplay RNG");
        require(gameplay_bits(level) == before, "Draining rumble events changed public gameplay state");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(print, MatchFingerprint::of(level, 17)).empty(),
            "Draining feedback changed the existing replay fingerprint");
#endif
        return events;
    }

    void damage_and_sparse_drain(World& world)
    {
        Fixture fixture(world, 3);
        shorten_countdown(fixture);
        fixture.plain({ 0.0f, 800.0f, 6000.0f, 80.0f });
        Player& first = fixture.add_player(1000.0f, 800.0f, PlayerTeam::a, WeaponType::sprayer, 3);
        fixture.add_player(1600.0f, 800.0f, PlayerTeam::a, WeaponType::sprayer, 1);
        fixture.add_player(2200.0f, 800.0f, PlayerTeam::b, WeaponType::sprayer, input_slots::keyboard_and_mouse);
        fixture.build(); fixture.start(); fixture.settle();
        auto records = fixture.level->take_rumble_events();
        require(records.size() == 3 && records[0].input_slot == 3 && records[1].input_slot == 1 &&
            records[2].input_slot == input_slots::keyboard_and_mouse,
            "Rumble drain replaced sparse/reordered physical slots with view indices");
        all_empty(records);
        const ProjectileSpray projectile = shot(world);
        first.set_damage_immune(true);
        const float initial_health = first.health();
        first.on_contact(projectile, Vector2F::ZERO, 0.0f);
        require(first.health() == initial_health, "Immune damage control changed health");
        all_empty(fixture.level->take_rumble_events());
        const ProjectileSpray keyboard_threat = shot(world, PlayerTeam::a, 3);
        fixture.players[2]->on_contact(keyboard_threat, Vector2F::ZERO, 0.0f);
        records = fixture.level->take_rumble_events();
        require(records[2].input_slot == input_slots::keyboard_and_mouse && records[2].events.hit &&
            records[0].events.empty() && records[1].events.empty(),
            "Level discarded or reassigned a keyboard event before the output policy could gate it");
        first.set_damage_immune(false);
        first.on_contact(projectile, Vector2F::ZERO, 0.0f);
        require(first.health() > 0.0f && first.health() < initial_health,
            "Nonfatal rumble fixture did not use production projectile damage");
        records = drain_without_gameplay_change(*fixture.level);
        require(records[0].events == RumbleEvents{ true, false, false, false } &&
            records[1].events.empty() && records[2].events.empty(),
            "Actual nonfatal hit emitted the wrong rumble event or recipient");
        all_empty(fixture.level->take_rumble_events());

        // Lethal damage remains a contact decision; elimination remains the
        // existing once-only dead-state transition on the next positive tick.
        for (int hit = 0; hit < 10000 && first.health() > 0.0f; ++hit)
        {
            first.on_contact(projectile, Vector2F::ZERO, 0.0f);
        }
        require(first.health() <= 0.0f && first.state() == PlayerState::alive,
            "Fatal contact fixture skipped the production death transition");
        fixture.level->clear_rumble_events();
        fixture.tick();
        records = drain_without_gameplay_change(*fixture.level);
        require(records[0].events == RumbleEvents{ false, true, false, false },
            "Fatal damage did not emit exactly the elimination cue");
        fixture.tick();
        all_empty(fixture.level->take_rumble_events());
        first.force_respawn();
        first.on_contact(projectile, Vector2F::ZERO, 0.0f);
        first.place_at(first.rectangle().top_left());
        all_empty(fixture.level->take_rumble_events());
        first.on_contact(projectile, Vector2F::ZERO, 0.0f);
        first.force_respawn();
        all_empty(fixture.level->take_rumble_events());
        fixture.tick(input(), 0.0f);
        all_empty(fixture.level->take_rumble_events());
    }

    void landing_decisions(World& world)
    {
        Fixture fixture(world);
        shorten_countdown(fixture);
        fixture.plain({ 0.0f, 800.0f, 6000.0f, 80.0f });
        Player& player = fixture.add_player(1000.0f, 800.0f, PlayerTeam::a, WeaponType::sprayer, 2);
        fixture.build(); fixture.start(); fixture.settle();
        all_empty(fixture.level->take_rumble_events());
        fixture.tick(input(0.0f, true));
        bool airborne = player.velocity().y < 0.0f;
        bool landed = false;
        for (int tick = 0; tick < 240 && !landed; ++tick)
        {
            PlayerInputData actions = input();
            actions.jump_held = tick < 60;
            fixture.tick(actions);
            airborne = airborne || player.velocity().y < 0.0f;
            const auto records = fixture.level->take_rumble_events();
            require(records.size() == 1 && !records[0].events.hit && !records[0].events.elimination &&
                !records[0].events.dry_fire, "Jump/landing emitted an unrelated rumble event");
            if (records[0].events.hard_landing)
            {
                require(airborne, "Spawn settling was mistaken for a hard landing");
                landed = true;
            }
        }
        require(landed, "Real held jump did not reach the existing hard-landing decision");
        fixture.settle(12);
        all_empty(fixture.level->take_rumble_events());
        player.place_at(player.rectangle().top_left());
        fixture.settle();
        all_empty(fixture.level->take_rumble_events());
    }

    class EmptyWeapon final : public Weapon
    {
    public:
        EmptyWeapon(World& world, WeaponType tool) :
            Weapon(&world.tuning, PlayerTeam::a, 3, 0, world.palette.team_colour(PlayerTeam::a),
                tool, Vector2F::ZERO, &world.render, &world.audio, &world.preferences) {}
        void empty()
        {
            this->set_ammo(0.0f);
            this->set_ammo_timer(0.0f);
            this->set_shoot_timer(this->shoot_interval() + 1.0f);
        }
        void recovering()
        {
            this->reset_ammo();
            this->set_shoot_timer(0.0f);
        }
    };

    void dry_trigger_decisions(World& world)
    {
        for (const WeaponType tool : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        {
            EmptyWeapon weapon(world, tool);
            weapon.empty();
            const auto update = [&](bool holding, float elapsed)
            {
                return weapon.update_and_get_projectiles(input(0.0f, false, holding),
                    Vector2F::ZERO, Vector2F::ZERO, true, elapsed);
            };
            require(update(true, 0.0f).empty() && weapon.take_rumble_events().empty(),
                "Frozen empty weapon emitted rumble");
            require(update(true, dt).empty() &&
                weapon.take_rumble_events() == RumbleEvents{ false, false, false, true },
                "Actual empty-trigger decision did not emit dry fire");
            require(weapon.take_rumble_events().empty(), "Weapon events were not consumed exactly once");
            require(update(true, dt).empty() && weapon.take_rumble_events().empty(),
                "Held dry trigger bypassed the existing warning repeat limiter");
            update(false, dt);
            require(weapon.take_rumble_events().empty(), "Released empty weapon emitted dry fire");
            update(true, dt);
            require(weapon.take_rumble_events().dry_fire, "Release/repress did not arm a new dry-trigger cue");
            weapon.recovering();
            require(update(true, dt).empty() && weapon.take_rumble_events().empty(),
                "A full tool waiting on cooldown was mistaken for a dry trigger");
            weapon.empty();
            update(true, dt);
            weapon.reset_ammo();
            require(weapon.take_rumble_events().empty(), "Respawn refill retained a stale dry-trigger cue");
        }

        // A real Player owns the tool and merges its dry-trigger decision with
        // contact feedback. Cue generation is independent of the saved output
        // switch; the later GameLevel consumer owns that gating decision.
        Fixture fixture(world);
        fixture.tuning = world.variant([&](rapidjson::Document& document)
        {
            moving_structure_acceptance::set_countdown(document, dt / 4.0f);
            document["weapons"]["starting_ammo"].SetFloat(
                world.tuning.weapon(WeaponType::sprayer).details.ammo_usage * 0.5f);
        });
        fixture.plain({ 0.0f, 800.0f, 6000.0f, 80.0f });
        Player& player = fixture.add_player(1000.0f, 800.0f, PlayerTeam::a, WeaponType::sprayer, 3);
        fixture.build(); fixture.start();
        fixture.level->clear_rumble_events();
        world.preferences.pad_rumble[3] = false;
        fixture.tick(input(0.0f, false, true));
        const ProjectileSpray projectile = shot(world);
        player.on_contact(projectile, Vector2F::ZERO, 0.0f);
        const auto records = fixture.level->take_rumble_events();
        require(records.size() == 1 && records[0].input_slot == 3 &&
            records[0].events == RumbleEvents{ true, false, false, true },
            "Player/Level did not merge actual tool and contact events independently of output settings");
        world.preferences.pad_rumble[3] = true;
        all_empty(fixture.level->take_rumble_events());
    }
}

void check_callers(const std::filesystem::path& content)
{
    World world(content);
    damage_and_sparse_drain(world);
    landing_decisions(world);
    dry_trigger_decisions(world);
    std::cout << "PASS rumble data/callers: actual damage/death/landing/dry-trigger decisions, consumed sparse-slot events, "
        "spawn/frozen/cooldown/immune controls, lifecycle discard and exact gameplay/RNG/fingerprint drain controls; "
        "GameLevel output, settings UI and native motors remain separate\n";
}
}
