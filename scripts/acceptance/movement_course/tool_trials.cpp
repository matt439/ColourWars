#include "game/pch.h"
#include "game/play_settings.h"
#include "game/tuning.h"
#include "game/objects/paintable_object.h"
#include "game/objects/structure.h"
#include "game/objects/paint_tile.h"
#include "game/objects/team_colour.h"
#include "engine/audio/audio_resources.h"
#include "engine/audio/sound_bank.h"
#include "engine/render/colour.h"

// Attribute paint to one authored face through the tiles the surface scores.
// Load its dependencies first so this does not change any other declaration.
#define private public
#include "game/objects/structure_paintable.h"
#undef private

#include "scripts/acceptance/movement_course/fixtures.h"

// Aiming each tool on the course, and the paint it lays on authored faces.
namespace movement_course_acceptance
{
    void aim(const Services& s)
    {
        const Surface& lane = s.course.named("recovery_floor");
        // Keyboard/mouse: Level requests the cursor direction on every tick
        // while the window is focused, so aim is re-supplied, not retained.
        {
            Run run(s, standing_on(s, lane, 1000.0f));
            run.settle();
            for (int tick = 0; tick < 160; ++tick)
            {
                const float angle = tick < 120 ? -0.6f : 2.6f;
                PlayerInputData actions = aimed(angle, input(false, false, tick < 60 ? 1.0f : -1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                require(run.player->primary_weapon().rotation() == angle,
                    "a requested aim changed while moving, reversing or firing");
            }
            require(run.shots > 0 && !run.player->facing_right(),
                "the aim fixture did not fire or reverse facing");
            std::cout << "MEASURE aim keyboard/mouse model: requested every tick, held through 160 ticks of "
                "moving, reversing and firing; shots=" << run.shots << '\n';
        }
        // Gamepad: once an aimed tool receives an explicit direction, the
        // neutral/deadzoned stick must retain it independently of movement.
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket })
        {
            Run run(s, standing_on(s, lane, lane.rectangle.center().x), Vector2F::ZERO, type);
            run.settle();
            run.tick(input(false, false, 1.0f));
            require(run.player->primary_weapon().rotation() == 0.0f,
                "an unaimed weapon did not follow the initial right facing");
            run.tick(input(false, false, -1.0f));
            require(run.player->primary_weapon().rotation() == PI,
                "an unaimed weapon did not follow the initial left facing");
            const Vector2F up_left(-0.7f, -0.7f);
            const float wanted = up_left.angle();
            const auto holds = [&run](float angle)
                { return std::fabs(run.player->primary_weapon().rotation() - angle) < 0.0001f; };
            for (int tick = 0; tick < 30; ++tick)
            {
                PlayerInputData actions = pad_aim(up_left, s.preferences, input(false, false, 1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                require(holds(wanted), "a held pad aim was not applied while moving and firing");
            }
            const int shots_before_release = run.shots;
            for (int tick = 0; tick < 90; ++tick)
            {
                const Vector2F stick = tick < 30 ? Vector2F(-0.1f, -0.1f) : Vector2F::ZERO;
                PlayerInputData actions = pad_aim(stick, s.preferences,
                    input(false, false, tick < 30 ? 1.0f : -1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                require(holds(wanted), "released or deadzoned pad aim changed during movement, reversal or firing");
            }
            require(run.shots > shots_before_release && !run.player->facing_right() &&
                run.player->velocity().x < 0.0f,
                "the released-aim fixture did not fire and reverse movement");

            const Vector2F replacement(0.8f, -0.3f);
            const float replaced = replacement.angle();
            run.tick(pad_aim(replacement, s.preferences));
            require(holds(replaced), "a fresh pad aim did not replace the retained direction");
            brake(run);
            require(holds(replaced), "released pad aim changed while coming to rest");

            // These are the player hooks used when Level suspends/resumes;
            // no simulation time passes while the menu owns the match.
            MatchVoices voices;
            run.player->add_voices(voices);
            voices.pause();
            run.player->reset_jump_input();
            voices.resume();
            run.tick();
            require(holds(replaced), "pause and resume discarded retained pad aim");

            run.kill();
            run.tick();
            require(run.player->state() == PlayerState::dead, "the aim fixture did not enter death");
            for (int tick = 0; tick < 400 && run.player->state() == PlayerState::dead; ++tick)
            {
                run.tick();
                require(holds(replaced), "death and respawn discarded retained pad aim");
            }
            require(run.player->state() == PlayerState::alive, "the aim fixture did not respawn");
            run.tick();
            require(holds(replaced), "released pad aim changed in the new life");
            std::cout << "MEASURE aim pad model tool=" << static_cast<int>(type)
                << ": retained=" << fixed(wanted, 3) << " during deadzone/release/movement/reversal/fire"
                << " replaced=" << fixed(replaced, 3) << " after rest/pause/death+respawn"
                << " shots=" << run.shots << '\n';
        }
        {
            Run run(s, standing_on(s, lane, lane.rectangle.center().x), Vector2F::ZERO, WeaponType::roller);
            run.settle();
            for (int tick = 0; tick < 90; ++tick)
            {
                const bool right = tick < 30;
                PlayerInputData actions = pad_aim(tick < 60 ? Vector2F(-0.7f, -0.7f) : Vector2F::ZERO,
                    s.preferences, input(false, false, right ? 1.0f : -1.0f));
                actions.primary_shoot = true;
                run.tick(actions);
                const float wanted = right ? PI / 4.0f : 3.0f * PI / 4.0f;
                require(run.player->primary_weapon().rotation() == wanted,
                    "the roller stopped using its fixed facing-down aim");
            }
            require(run.shots > 0, "the fixed-aim roller fixture never fired");
        }
        std::cout << "PASS aim: every aimed tool retains/replaces pad aim through deadzone, release, movement, reversal, "
            "firing, pause and respawn; initial facing fallback and the roller's fixed aim remain\n";
    }

    namespace
    {
    float face_paint(const StructurePaintable& surface, bool underside, float thickness)
    {
        const RectangleF r = surface.bounds();
        float area = 0.0f;
        for (const PaintTile& tile : surface.paint_tiles_)
        {
            const RectangleF b = tile.bounds();
            const bool on_face = underside
                ? std::fabs(b.height - thickness) < 0.01f && std::fabs(b.bottom() - r.bottom()) < 0.01f
                : std::fabs(b.width - thickness) < 0.01f && std::fabs(b.left() - r.left()) < 0.01f;
            if (on_face && tile.team() == PlayerTeam::a) { area += tile.area(); }
        }
        return area;
    }
    }

    void painting(const Services& s)
    {
        const Surface& floor = s.course.named("running_landing");
        const Surface& block = s.course.named("low_ceiling");
        const Surface& step = s.course.named("short_step");
        const float thickness = s.tuning.paint().thickness;
        for (const WeaponType type : { WeaponType::sprayer, WeaponType::roller, WeaponType::sniper,
            WeaponType::mister, WeaponType::bucket })
        {
            const char* name = type == WeaponType::sprayer ? "sprayer" : type == WeaponType::roller ? "roller" :
                type == WeaponType::sniper ? "sniper" : type == WeaponType::mister ? "mister" : "bucket";
            // Fire for 90 ticks while sweeping aim from `first` to `last`, then
            // let flying paint settle. Walking into a face lets the roller paint
            // by contact. A muzzle inside cover paints through the launch
            // obstruction instead of flying, so `shots` counts only flyers and
            // `swept` only the face area painted while flying projectiles sweep
            // through Projectile::resolve_movement_contacts.
            struct Painted
            {
                float area = 0.0f;
                float swept = 0.0f;
                int shots = 0;
            };
            const auto station = [&](const Vector2F& top_left, float first, float last, float walk,
                const Surface& target, bool underside)
            {
                Run run(s, top_left, Vector2F::ZERO, type);
                run.settle(5);
                run.watched_paint = [&run, &target, underside, thickness]
                    { return face_paint(run.paintable(target.name), underside, thickness); };
                for (int tick = 0; tick < 240; ++tick)
                {
                    const float angle = first + (last - first) * static_cast<float>(std::min(tick, 89)) / 89.0f;
                    PlayerInputData actions = aimed(angle, input(false, false, tick < 90 ? walk : 0.0f));
                    actions.primary_shoot = tick < 90;
                    run.tick(actions);
                }
                return Painted{ run.watched_paint(), run.swept_paint, run.shots };
            };
            // Underside: from directly beneath (muzzle in contact range), then
            // from outside its left end so the paint has to fly. The ceiling
            // clears a standing head by only 40 units, so a steep straight shot
            // passes over its end. The sniper, which launches once here, stands
            // back past its long muzzle and holds a shallow aim; the bucket's
            // tall, dropping blob needs a steep lob from close outside.
            float outside = 100.0f;
            float first = -1.25f;
            float last = -0.55f;
            if (type == WeaponType::sniper) { outside = 300.0f; first = last = -0.15f; }
            if (type == WeaponType::bucket) { outside = 150.0f; first = last = -0.8f; }
            const auto under = station(standing_on(s, floor, block.rectangle.center().x),
                -PI / 2.0f, -PI / 2.0f, 0.0f, block, true);
            const auto under_far = station(standing_on(s, floor, block.rectangle.left() - outside),
                first, last, 0.0f, block, true);
            // Wall: walk into the short step's left face aiming slightly down,
            // then sweep across it from 200 units away.
            const auto wall = station(front_at(s, floor, step.rectangle.left() - 40.0f, 1.0f),
                0.35f, 0.35f, 1.0f, step, false);
            const auto wall_far = station(front_at(s, floor, step.rectangle.left() - 200.0f, 1.0f),
                -0.3f, 0.3f, 0.0f, step, false);
            const auto report = [](const Painted& painted)
            {
                std::ostringstream text;
                text << fixed(painted.area) << " (swept " << fixed(painted.swept) << ", flying shots "
                    << painted.shots << ")";
                return text.str();
            };
            std::cout << "MEASURE paint " << name << ": low_ceiling underside team_a beneath=" << report(under)
                << " from-outside=" << report(under_far) << " | short_step left face team_a contact="
                << report(wall) << " from-200=" << report(wall_far) << "\n";
            if (type == WeaponType::roller)
            {
                // The roller's aim is fixed forward-down: it paints by contact.
                require(wall.area > 0.0f, "the roller did not paint the step face by contact");
            }
            else
            {
                require(under.area > 0.0f && wall.area > 0.0f,
                    "an aimed tool did not paint the authored ceiling underside and step face");
                // The far stations must paint by flight, not launch obstruction.
                require(under_far.shots > 0 && under_far.swept > 0.0f &&
                    wall_far.shots > 0 && wall_far.swept > 0.0f,
                    "an aimed tool's flying paint did not reach the ceiling underside and step face");
            }
        }
        std::cout << "PASS painting: aimed tools paint the low_ceiling underside and a staircase face, "
            "from close range and by swept flight from outside cover; the roller the face by contact\n";
    }
}
