#include "game/pch.h"
#include "scripts/acceptance/movement_course/fixtures.h"
#include "scripts/acceptance/support/probe.h"

// Route trials on the authored Movement Course geometry. Every trial builds a
// fresh Scene from movement_course.json through LevelObjectBuilder and places
// the real Player by construction or respawn; nothing is teleported. MEASURE
// lines record candidate tuning; assertions cover only intended outcomes.
// The trials are in movement_course/; main builds what they share and runs
// them in order.

using namespace labrador;
using namespace mattmath;
using namespace movement_course_acceptance;

int main(int argc, char** argv)
{
    return acceptance::run_probe("movement course", [&]
    {
        require(argc == 2, "Usage: movement_course.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        RenderResources render;
        for (const char* name : { "sprite_sheet_1", "painter", "weapon_paint", "paint_fx", "terrain" })
        {
            render.add_sprite_sheet(name, read_sprite_sheet(
                (content / "textures" / (std::string(name) + ".json")).string().c_str(),
                TextureHandle{}));
        }
        AudioResources audio;
        audio.add_sound_bank("paint_exe", SoundBank::silent());
        const PlaySettings preferences;
        const GameTuning tuning((content / "tuning.json").string().c_str());
        const Presentation presentation((content / "presentation.json").string().c_str());
        require(presentation.movement_feedback().enabled,
            "route trials count takeoffs and landings through movement feedback");
        const TeamColour colours;
        const LevelObjectBuilder builder(&render, &audio, &preferences, &tuning.paint());
        const CourseData course(content / "levels" / "movement_course.json");
        const Services services{ tuning, presentation.movement_feedback(),
            presentation.combat_feedback(), presentation.respawn_redraw(), render, audio,
            preferences, builder, colours, course };
        std::cout << "NOTE dt=" << dt << "; every trial builds a fresh authored Scene and steps it in "
            "Level::update_level_logic order (no const_cast placement)\n";

        const std::vector<Gap> routes{
            { "short", &course.named("spawn_a_runway"), &course.named("short_landing_long_takeoff"),
                &course.named("short_gap_recovery_low"), &course.named("short_gap_recovery_high") },
            { "long", &course.named("short_landing_long_takeoff"), &course.named("long_landing_running_takeoff"),
                &course.named("long_gap_recovery_low"), &course.named("long_gap_recovery_high") },
            { "running", &course.named("long_landing_running_takeoff"), &course.named("running_landing"),
                &course.named("running_gap_recovery_low"), &course.named("running_gap_recovery_high") } };
        for (const Gap& gap : routes)
        {
            require(gap.width() > 0.0f, "a Movement Course gap closed");
        }

        from_rest_travel(services);
        analog_movement(services);
        gaps(services, routes);
        recovery(services, routes);
        staircases(services);
        ceiling(services);
        drop_through(services);
        lip_timing(services);
        spawns(services);
        aim(services);
        painting(services);
        std::cout << "PASS Movement Course route trials on authored geometry\n";
        return 0;
    });
}
