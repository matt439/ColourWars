#include "game/pch.h"
#include "scripts/acceptance/crossfeed/fixtures.h"
#include <limits>

namespace crossfeed_acceptance
{
    namespace
    {
        const char* tool_name(WeaponType type)
        {
            switch (type)
            {
            case WeaponType::sprayer: return "sprayer";
            case WeaponType::sniper: return "sniper";
            case WeaponType::roller: return "roller";
            case WeaponType::mister: return "mister";
            case WeaponType::bucket: return "bucket";
            default: throw std::runtime_error("Crossfeed supply needs a concrete primary");
            }
        }

        void supply(Services& s, WeaponType type)
        {
            const Vector2F origin(1800.0f, 1060.0f);
            auto weapon = WeaponBuilder::build_weapon(&s.tuning, PlayerTeam::a, 0, 0,
                Colour::white, type, origin, &s.render, &s.audio, &s.preferences,
                &s.presentation.tool_motion(), &s.presentation.weapon_feedback());
            const float starting = s.tuning.starting_ammo();
            const float cost = s.tuning.weapon(type).details.ammo_usage;
            const float interval = s.tuning.weapon(type).details.shoot_interval;
            require(starting > 0.0f && cost > 0.0f && cost <= starting &&
                approximately(weapon->ammo(), starting),
                "Crossfeed supply did not construct a full affordable starting tank");

            // Prime cooldown with ordinary shot-free updates. This excludes a
            // construction delay from the measured held-trigger supply window.
            int priming_ticks = 0;
            // Public readiness combines affordability with cooldown. Rebuild
            // only the float cooldown clock from its known zero construction
            // state and observed paid resets, so an unaffordable opportunity
            // can be distinguished from an empty state during cooldown.
            float observed_cooldown = 0.0f;
            for (; !weapon->ready_to_shoot() && priming_ticks < 1800; ++priming_ticks)
            {
                const auto objects = weapon->update_and_get_projectiles(aimed(0.0f),
                    origin, Vector2F::ZERO, true, dt);
                require(objects.empty(), "Crossfeed supply priming emitted without fire input");
                observed_cooldown += dt;
            }
            require(weapon->ready_to_shoot(), "Crossfeed supply cooldown never primed within 30 seconds");
            const float initial = weapon->ammo();
            int paid = 0, first_paid = -1, last_paid = -1, emitted_objects = 0;
            int minimum_cadence = std::numeric_limits<int>::max(), maximum_cadence = 0;
            int raw_below_cost = -1, empty_tick = -1, first_post_paid_empty = -1, held_ticks = 0;
            int first_missed_opportunity = -1, paid_at_first_empty = 0, empty_state_ticks = 0;
            float missed_before_ammo = -1.0f, missed_after_ammo = -1.0f, missed_cooldown = -1.0f;
            int paid_before_first_miss = -1, last_paid_before_first_miss = -1;
            int pre_miss_minimum_cadence = -1, pre_miss_maximum_cadence = -1;
            float recovered_at_first_miss = 0.0f;
            constexpr int continued_held_ticks = 300;
            int tail_ticks = 0, tail_paid = 0, first_resumed_tick = -1, last_tail_paid_tick = -1;
            int first_recovery_gap = -1, tail_minimum_cadence = std::numeric_limits<int>::max();
            int tail_maximum_cadence = 0, tail_unaffordable_eligible_ticks = 0;
            float first_resumed_before_ammo = -1.0f, first_resumed_after_ammo = -1.0f;
            float tail_minimum_ammo = -1.0f, tail_maximum_ammo = -1.0f;
            std::ostringstream tail_paid_ticks;
            float recovered_while_held = 0.0f;
            float minimum_ammo = initial;
            for (int tick = 1; tick <= 1800 + continued_held_ticks; ++tick)
            {
                const bool in_tail = first_missed_opportunity >= 0;
                PlayerInputData actions = aimed(0.0f);
                actions.primary_shoot = true;
                const float before = weapon->ammo();
                const WeaponReadiness readiness_before = weapon->readiness();
                const bool cooldown_eligible = observed_cooldown > interval;
                if (readiness_before != WeaponReadiness::empty)
                {
                    require(weapon->ready_to_shoot() == cooldown_eligible,
                        "Crossfeed observed cooldown clock disagrees with public affordable readiness");
                }
                // The returned unique_ptrs are owned only for this iteration,
                // counted and then discarded. No Scene, collision, damage,
                // paint, flight distance or range is inferred from this assay.
                const auto objects = weapon->update_and_get_projectiles(actions,
                    origin, Vector2F::ZERO, true, dt);
                const float after = weapon->ammo();
                const bool paid_this_tick = after < before;
                require(paid_this_tick == !objects.empty(),
                    "Crossfeed direct supply payment and unobstructed emitted objects disagree");
                emitted_objects += static_cast<int>(objects.size());
                if (paid_this_tick)
                {
                    if (first_paid < 0) { first_paid = tick; }
                    if (last_paid >= 0)
                    {
                        minimum_cadence = std::min(minimum_cadence, tick - last_paid);
                        maximum_cadence = std::max(maximum_cadence, tick - last_paid);
                    }
                    last_paid = tick;
                    ++paid;
                    if (in_tail)
                    {
                        if (first_resumed_tick < 0)
                        {
                            first_resumed_tick = tick;
                            first_recovery_gap = tick - last_paid_before_first_miss;
                            first_resumed_before_ammo = before;
                            first_resumed_after_ammo = after;
                        }
                        if (last_tail_paid_tick >= 0)
                        {
                            tail_minimum_cadence = std::min(tail_minimum_cadence, tick - last_tail_paid_tick);
                            tail_maximum_cadence = std::max(tail_maximum_cadence, tick - last_tail_paid_tick);
                        }
                        if (tail_paid > 0) { tail_paid_ticks << ','; }
                        tail_paid_ticks << tick - first_missed_opportunity;
                        last_tail_paid_tick = tick;
                        ++tail_paid;
                    }
                }
                else { recovered_while_held += after - before; }
                minimum_ammo = std::min(minimum_ammo, after);
                held_ticks = tick;
                if (after < cost && raw_below_cost < 0) { raw_below_cost = tick; }
                if (after < cost && weapon->readiness() == WeaponReadiness::empty)
                {
                    ++empty_state_ticks;
                    if (empty_tick < 0) { empty_tick = tick; paid_at_first_empty = paid; }
                    if (paid_this_tick && first_post_paid_empty < 0) { first_post_paid_empty = tick; }
                }
                // Check firing eligibility BEFORE this update's regeneration,
                // matching Weapon's payment-before-regeneration order. A
                // freshly paid shot may leave readiness empty during cooldown
                // yet recover enough paint before the next eligible shot.
                const bool missed = cooldown_eligible && readiness_before == WeaponReadiness::empty &&
                    !paid_this_tick;
                if (missed && first_missed_opportunity < 0)
                {
                    first_missed_opportunity = tick;
                    missed_before_ammo = before;
                    missed_after_ammo = after;
                    missed_cooldown = observed_cooldown;
                    paid_before_first_miss = paid;
                    last_paid_before_first_miss = last_paid;
                    pre_miss_minimum_cadence = paid > 1 ? minimum_cadence : 0;
                    pre_miss_maximum_cadence = maximum_cadence;
                    recovered_at_first_miss = recovered_while_held;
                    tail_minimum_ammo = tail_maximum_ammo = after;
                }
                if (in_tail)
                {
                    ++tail_ticks;
                    tail_unaffordable_eligible_ticks += missed ? 1 : 0;
                    tail_minimum_ammo = std::min(tail_minimum_ammo, after);
                    tail_maximum_ammo = std::max(tail_maximum_ammo, after);
                }
                if (paid_this_tick) { observed_cooldown = 0.0f; }
                observed_cooldown += dt;
                // Holding the trigger can resume without a manual release.
                // Keep five more seconds after the first missed opportunity;
                // one quantized missed tick alone is not an enforced pause or
                // the tool's eventual sustained firing cadence.
                if ((first_missed_opportunity >= 0 && tail_ticks >= continued_held_ticks) ||
                    (first_missed_opportunity < 0 && tick >= 1800))
                {
                    break;
                }
            }
            require(paid > 0 && emitted_objects > 0,
                "Crossfeed supply held fire never paid for an unobstructed launch");
            const float release_ammo = weapon->ammo();
            std::array<float, 3> release_samples{ -1.0f, -1.0f, -1.0f };
            int full_refill_tick = release_ammo >= starting ? 0 : -1;
            int release_ticks = 0;
            for (int tick = 1; tick <= 3600; ++tick)
            {
                const float before = weapon->ammo();
                const auto objects = weapon->update_and_get_projectiles(aimed(0.0f),
                    origin, Vector2F::ZERO, true, dt);
                require(objects.empty() && weapon->ammo() >= before,
                    "Crossfeed supply released input paid or reduced ammunition");
                if (tick == 30) { release_samples[0] = weapon->ammo(); }
                if (tick == 60) { release_samples[1] = weapon->ammo(); }
                if (tick == 120) { release_samples[2] = weapon->ammo(); }
                if (full_refill_tick < 0 && weapon->ammo() >= starting) { full_refill_tick = tick; }
                release_ticks = tick;
                if (tick >= 120 && full_refill_tick >= 0) { break; }
            }
            const double authored_paid_cost = static_cast<double>(paid) * static_cast<double>(cost);
            const double actual_net_drain = static_cast<double>(initial) - static_cast<double>(release_ammo);
            std::cout << "MEASURE Crossfeed supply tool=" << tool_name(type) <<
                " starting_ammo=" << fixed(starting, 6) << " authored_cost=" << fixed(cost, 6) <<
                " priming_ticks=" << priming_ticks << " held_ticks=" << held_ticks <<
                " held_seconds=" << fixed(static_cast<float>(held_ticks) * dt, 4) <<
                " first_paid_tick=" << first_paid << " last_paid_tick=" << last_paid <<
                " paid_events=" << paid << " emitted_objects=" << emitted_objects <<
                " cadence_min_ticks=" << (paid > 1 ? minimum_cadence : 0) <<
                " cadence_max_ticks=" << maximum_cadence <<
                " raw_below_authored_cost_tick=" << raw_below_cost <<
                " first_empty_readiness_tick=" << empty_tick <<
                " first_post_paid_empty_tick=" << first_post_paid_empty <<
                " empty_state_ticks_while_held=" << empty_state_ticks <<
                " paid_events_after_first_empty_state=" << (empty_tick < 0 ? 0 : paid - paid_at_first_empty) <<
                " first_missed_cooldown_eligible_tick=" << first_missed_opportunity <<
                " paid_events_before_first_missed_opportunity=" << paid_before_first_miss <<
                " last_paid_before_first_missed_opportunity_tick=" << last_paid_before_first_miss <<
                " pre_miss_cadence_min_ticks=" << pre_miss_minimum_cadence <<
                " pre_miss_cadence_max_ticks=" << pre_miss_maximum_cadence <<
                " missed_before_ammo=" << fixed(missed_before_ammo, 6) <<
                " missed_after_regen_ammo=" << fixed(missed_after_ammo, 6) <<
                " missed_observed_cooldown=" << fixed(missed_cooldown, 6) <<
                " continued_held_tail_ticks=" << tail_ticks <<
                " continued_held_tail_seconds=" << fixed(static_cast<float>(tail_ticks) * dt, 4) <<
                " tail_paid_events=" << tail_paid <<
                " tail_paid_ticks_after_first_miss=" << (tail_paid > 0 ? tail_paid_ticks.str() : "none") <<
                " first_resumed_paid_tick=" << first_resumed_tick <<
                " first_resume_delay_after_miss_ticks=" << (first_resumed_tick < 0 ? -1 :
                    first_resumed_tick - first_missed_opportunity) <<
                " first_gap_from_pre_miss_paid_to_resumed_paid_ticks=" << first_recovery_gap <<
                " first_resumed_before_ammo=" << fixed(first_resumed_before_ammo, 6) <<
                " first_resumed_after_ammo=" << fixed(first_resumed_after_ammo, 6) <<
                " tail_cadence_min_ticks=" << (tail_paid > 1 ? tail_minimum_cadence : 0) <<
                " tail_cadence_max_ticks=" << tail_maximum_cadence <<
                " tail_unaffordable_eligible_ticks=" << tail_unaffordable_eligible_ticks <<
                " tail_minimum_ammo=" << fixed(tail_minimum_ammo, 6) <<
                " tail_maximum_ammo=" << fixed(tail_maximum_ammo, 6) <<
                " tail_regained_ammo=" << fixed(tail_ticks > 0 ? recovered_while_held - recovered_at_first_miss : 0.0f, 6) <<
                " tail_end_ammo=" << fixed(tail_ticks > 0 ? release_ammo : -1.0f, 6) <<
                " observation_end=" << (first_missed_opportunity >= 0 ? "five-more-seconds-of-held-fire-after-first-miss" :
                    "30-second-bound-without-missed-eligible-shot") <<
                " minimum_ammo=" << fixed(minimum_ammo, 6) <<
                " regained_while_held=" << fixed(recovered_while_held, 6) <<
                " authored_paid_cost=" << authored_paid_cost << " actual_net_drain=" << actual_net_drain <<
                " release_ammo=" << fixed(release_ammo, 6) <<
                " release_ammo_fraction=" << fixed(release_ammo / starting, 6) <<
                " release_plus_30_ticks_ammo=" << fixed(release_samples[0], 6) <<
                " release_plus_60_ticks_ammo=" << fixed(release_samples[1], 6) <<
                " release_plus_120_ticks_ammo=" << fixed(release_samples[2], 6) <<
                " full_refill_tick=" << full_refill_tick <<
                " full_refill_seconds=" << (full_refill_tick >= 0 ? fixed(static_cast<float>(full_refill_tick) * dt, 4) : "unobserved") <<
                " release_observed_ticks=" << release_ticks <<
                " scope=actual-weapon-updates-discarded-owned-projectiles-no-scene-paint-or-damage-claim\n";
        }
    }

    void supply_checks(Services& s)
    {
        std::cout << "NOTE Crossfeed supply hypotheses: sprayer sustains pressure while changing cover; "
            "sniper trades slower paid shots for straight lane reach; roller paints by close contact and travel; "
            "mister distributes growing pellets into nearby approaches; bucket trades cadence for a broad ballistic contact. "
            "These roles need actual map, target and face tests. This assay measures trigger supply only, including "
            "recovery between held shots. It retains the first empty readiness state and first cooldown-eligible "
            "held firing opportunity that misses for lack of paint, then continues holding fire for five more seconds. "
            "The first miss is a burst boundary observation, not a forced manual pause: paint can recover on that "
            "same update and permit a shot on the next tick. Subsequent paid ticks, the first resumed gap and the "
            "later held cadence are measured separately. Without a miss, observation ends at thirty seconds. Empty readiness "
            "just after a paid shot can recover before the next firing opportunity. The observed float cooldown "
            "clock starts at construction, follows actual paid resets and is checked against public affordable "
            "readiness. No private state is read; no four-sniper/250-roller budget is assumed\n";
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper, WeaponType::roller,
            WeaponType::mister, WeaponType::bucket })
        { supply(s, type); }
        std::cout << "PASS Crossfeed actual five-tool held supply and released recovery observations\n";
    }
}
