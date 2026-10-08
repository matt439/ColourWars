#include "game/pch.h"
#include "game/objects/weapon_builder.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"

#include <filesystem>
#include <iostream>
#include <limits>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    class ProbeWeapon final : public Weapon
    {
    public:
        using Weapon::Weapon;

        void prime(float ammo)
        {
            this->set_ammo(ammo);
            this->set_ammo_timer(0.0f);
            this->set_shoot_timer(this->shoot_interval() + 1.0f);
        }

        void override_cost(float cost)
        {
            this->cost_variant_ = *this->weapon_tuning_;
            this->cost_variant_.details.ammo_usage = cost;
            this->weapon_tuning_ = &this->cost_variant_;
        }

        bool fire_sound = false;
        bool held_sound = false;

        bool firing_presentation() const
        {
            return this->shooting_this_update_;
        }

    protected:
        void handle_shoot_sound(bool fired, bool holding) override
        {
            this->fire_sound = fired;
            this->held_sound = holding;
        }

    private:
        WeaponTuning cost_variant_;
    };

    struct WeaponCase
    {
        WeaponType type;
        const char* name;
        size_t projectile_count;
    };

    const WeaponCase cases[] = {
        { WeaponType::sprayer, "sprayer", 1 },
        { WeaponType::sniper, "sniper", 1 },
        { WeaponType::roller, "roller", 1 },
        { WeaponType::mister, "mister", 5 },
        { WeaponType::bucket, "bucket", 1 }
    };

    auto update(Weapon& weapon, bool held, float dt)
    {
        PlayerInputData input;
        input.primary_shoot = held;
        return weapon.update_and_get_projectiles(input, Vector2F::ZERO,
            Vector2F::ZERO, true, dt);
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: ammo.exe <game/content directory>");
        const std::filesystem::path content(argv[1]);
        const auto tuning_path = (content / "tuning.json").string();
        const auto sheet_path =
            (content / "textures" / "sprite_sheet_1.json").string();

        RenderResources render;
        render.add_sprite_sheet("sprite_sheet_1",
            read_sprite_sheet(sheet_path.c_str(), TextureHandle{}));
        const auto weapon_sheet_path =
            (content / "textures" / "weapon_paint.json").string();
        render.add_sprite_sheet("weapon_paint",
            read_sprite_sheet(weapon_sheet_path.c_str(), TextureHandle{}));
        AudioResources audio;
        audio.add_sound_bank("sound_bank_1", SoundBank::silent());
        const GameTuning tuning(tuning_path.c_str());
        const PlaySettings preferences;
        const float dt = static_cast<float>(StepTimer::TicksToSeconds(
            StepTimer::SecondsToTicks(1.0 / 60.0)));
        WeaponBuilder builder;

        for (const auto& item : cases)
        {
            const float cost = tuning.weapon(item.type).details.ammo_usage;
            ProbeWeapon boundary(&tuning, PlayerTeam::a, 0, 0, Colour::white,
                item.type, Vector2F::ZERO, &render, &audio, &preferences);

            // Exact and adjacent representable values exercise the full-shot
            // boundary independently of regeneration and firing cadence.
            for (const float paid : { cost, std::nextafter(cost, 0.0f),
                std::nextafter(cost, std::numeric_limits<float>::infinity()) })
            {
                boundary.prime(paid);
                require(update(boundary, true, 0.0f).size() == item.projectile_count,
                    "full or rounding-adjacent tank did not fire");
                require(boundary.ammo() >= 0.0f && boundary.ammo() < cost,
                    "full shot did not consume its ammo");
                require(boundary.fire_sound && boundary.held_sound &&
                    boundary.firing_presentation(),
                    "final paid shot lost its sound or roller presentation");
                require(update(boundary, true, 0.0f).empty() &&
                    !boundary.fire_sound && !boundary.held_sound &&
                    !boundary.firing_presentation(),
                    "exhausted weapon kept firing or presenting held fire");
            }

            for (const float insufficient : { 0.0f, 0.001f, cost * 0.999f })
            {
                boundary.prime(insufficient);
                require(update(boundary, true, 0.0f).empty(),
                    "materially underfunded shot fired");
                require(boundary.ammo() == insufficient && !boundary.held_sound,
                    "refused shot consumed ammo or kept the firing loop active");
            }

            boundary.prime(cost);
            require(update(boundary, false, 0.0f).empty() &&
                boundary.ammo() == cost && !boundary.held_sound,
                "released trigger fired or consumed ammo");

            // Refused held requests must leave the regeneration delay advancing.
            boundary.prime(0.0f);
            int first_shot_tick = 0;
            for (int tick = 1; tick <= 240; ++tick)
            {
                if (!update(boundary, true, dt).empty())
                {
                    first_shot_tick = tick;
                    break;
                }
            }
            const float earliest = tuning.ammo_regen_delay() +
                cost / tuning.ammo_regen_rate();
            require(first_shot_tick > 0 &&
                static_cast<float>(first_shot_tick) * dt >= earliest - dt &&
                static_cast<float>(first_shot_tick) * dt <= earliest + 3.0f * dt,
                "held empty weapon failed to wait for and regenerate a full shot");

            // Exercise the actual stock subclasses through their factory. A long
            // hold reaches exhaustion even for the slow-firing sniper.
            auto stock = builder.build_weapon(&tuning, PlayerTeam::a, 0, 0,
                Colour::white, item.type, Vector2F::ZERO, &render, &audio,
                &preferences);
            bool saw_insufficient = false;
            bool recovered = false;
            int shots = 0;
            for (int tick = 0; tick < 2400; ++tick)
            {
                const float before = stock->ammo();
                const auto projectiles = update(*stock, true, dt);
                if (before < cost * 0.999f)
                {
                    saw_insufficient = true;
                }
                if (!projectiles.empty())
                {
                    require(projectiles.size() == item.projectile_count,
                        "stock weapon changed its volley size");
                    // This independent bound allows at most one part per
                    // million of rounding and excludes the former free shots.
                    require(static_cast<double>(cost) - before <=
                        static_cast<double>(cost) * 0.000001,
                        "stock weapon emitted an underfunded volley");
                    recovered = recovered || saw_insufficient;
                    ++shots;
                }
                require(stock->ammo() >= 0.0f &&
                    stock->ammo() <= tuning.starting_ammo(),
                    "ammo escaped its tank bounds");
            }
            require(saw_insufficient && recovered,
                "stock weapon did not exhaust and recover during the hold");
            stock->reset_ammo();
            require(stock->ammo() == tuning.starting_ammo(),
                "respawn did not restore the full tank");
            std::cout << "PASS " << item.name << ": boundaries, held recovery, "
                << shots << " paid volleys in 40 seconds\n";
        }

        ProbeWeapon tiny(&tuning, PlayerTeam::a, 0, 0, Colour::white,
            WeaponType::sprayer, Vector2F::ZERO, &render, &audio, &preferences);
        tiny.override_cost(0.000000001f);
        tiny.prime(0.000000000001f);
        require(update(tiny, true, 0.0f).empty(),
            "rounding allowance bought a shot with a tiny authored ammo cost");
        std::cout << "PASS tiny authored cost: no tank-sized rounding allowance\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
