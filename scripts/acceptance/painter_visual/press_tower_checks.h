// Included after crossfeed_checks.h inside painter_visual.cpp's private namespace.
// Controlled real-Level inspection, not route, pressure or readability acceptance.
void press_tower_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const LevelLoadedInfo* loaded = levels.get("press_tower");
    require(loaded != nullptr, "Press Tower native inspection needs the authored stage");
    const LevelLoadedInfo& info = *loaded;
    const auto rectangle = [&](const char* name) { return crossfeed_rectangle(info, name); };
    const Vector2F body = tuning.player().size;
    const auto standing = [&](const RectangleF& support)
    { return Vector2F(support.center().x - body.x / 2.0f, support.top() - body.y); };
    const RectangleF head = rectangle("press_head"), crown = rectangle("press_crown");
    require(crown.left() > head.left() && crown.right() < head.right() && crown.bottom() == head.top(),
        "Press Tower summit slots need the authored crown/head relationship");
    const std::array<std::array<Vector2F, 4>, 2> stations{
        std::array<Vector2F, 4>{ standing(rectangle("stair_1_a")), standing(rectangle("stair_1_b")),
            standing(rectangle("stair_3_a")), standing(rectangle("stair_3_b")) },
        std::array<Vector2F, 4>{ standing(rectangle("shoulder_a")), standing(rectangle("shoulder_b")),
            Vector2F((head.left() + crown.left()) / 2.0f - body.x / 2.0f, head.top() - body.y),
            Vector2F((crown.right() + head.right()) / 2.0f - body.x / 2.0f, head.top() - body.y) } };
    constexpr std::array<const char*, 2> phases{ "stairs", "hoist-exit" };
    constexpr std::array<std::array<const char*, 4>, 2> names{
        std::array<const char*, 4>{ "stair_1_a", "stair_1_b", "stair_3_a", "stair_3_b" },
        std::array<const char*, 4>{ "shoulder_a", "shoulder_b", "press_head_a", "press_head_b" } };
    constexpr std::array<std::array<const char*, 4>, 2> targets{
        std::array<const char*, 4>{ "stair_2_a", "stair_2_b", "shoulder_a", "shoulder_b" },
        std::array<const char*, 4>{ "press_head", "press_head", "shoulder_a", "shoulder_b" } };
#if COLOURWARS_DEV_TOOLS
    constexpr const char* fingerprint_scope = "checked";
#else
    constexpr const char* fingerprint_scope = "not-built";
#endif
    std::ofstream records(output / "press-tower.csv");
    records.precision(std::numeric_limits<float>::max_digits10);
    records << "capture,phase,paint,view,input_slot,station,player_x,player_y,velocity_x,velocity_y,"
        "camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,"
        "owned_fraction,target,target_x,target_y,target_width,target_height,target_visible,frozen_pixels,public_state,"
        "debug_fingerprint,rng,evidence_scope\n";
    size_t builds = 0, updates = 0, contacts = 0, frames = 0, crops = 0, rows = 0, readbacks = 0;
    float heavy_min = 1.0f;
    for (const bool heavy : { false, true })
    {
        std::srand(2532);
        auto fixture = crossfeed_fixture(app, info, tuning, presentation, preferences, 4, heavy, 30.0f, contacts, "press_tower");
        ++builds;
        const auto initial = crossfeed_ownership(fixture);
        if (heavy) { heavy_min = std::min(heavy_min, initial.second); }
        const auto inputs = neutral_inputs();
        for (int tick = 0; tick < 600 && fixture.level->state() != LevelState::active; ++tick)
        { fixture.level->update(inputs, dt); ++updates; }
        require(fixture.level->state() == LevelState::active, "Press Tower native fixture did not enter active play");
        // Retire the countdown's GO artwork before inspecting the standing bands.
        for (int tick = 0; tick < 90; ++tick) { fixture.level->update(inputs, dt); ++updates; }
        for (size_t phase = 0; phase < phases.size(); ++phase)
        {
            for (int view = 0; view < 4; ++view)
            {
                require(fixture.level->place_player(view, stations[phase][static_cast<size_t>(view)]),
                    "Press Tower controlled station placement failed");
            }
            for (int tick = 0; tick < 6; ++tick) { fixture.level->update(inputs, dt); ++updates; }
            require(fixture.level->state() == LevelState::active, "Press Tower inspection left live gameplay");
            const auto ownership = crossfeed_ownership(fixture);
            require(heavy ? ownership.second > 0.99f : ownership.second == 0.0f,
                "Press Tower controlled clean/heavy ownership changed");
            const Frame frame = crossfeed_frozen(app, fixture, readbacks);
            const std::string stem = "hidden-inspection-press-tower-4p-" + std::string(heavy ? "heavy-" : "clean-") + phases[phase] + "-720p";
            save(frame, output / (stem + ".ppm")); ++frames;
            for (int view = 0; view < 4; ++view)
            {
                const size_t index = static_cast<size_t>(view);
                const Player& player = fixture.level->player(view);
                const Camera camera = fixture.level->view_camera(view);
                const Viewport viewport = app.viewport_manager()->player_viewport(view);
                const RectangleI pane = viewport.pixel_rect();
                const RectangleF target = rectangle(targets[phase][index]);
                require(camera.visible_rectangle(viewport).contains(target),
                    "Press Tower native landing/exit target is outside the real player camera");
                save(crossfeed_crop(frame, pane), output / (stem + "-view" + std::to_string(view + 1) + "-native.ppm"));
                ++crops; ++rows;
                const Vector2F position = player.rectangle().position(), velocity = player.velocity();
                records << stem << ',' << phases[phase] << ',' << (heavy ? "heavy" : "clean") << ',' << view + 1 << ',' <<
                    player.player_num() << ',' << names[phase][index] << ',' << position.x << ',' << position.y << ',' <<
                    velocity.x << ',' << velocity.y << ',' << camera.translation.x << ',' << camera.translation.y << ',' <<
                    camera.scale << ',' << pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << ',' <<
                    pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << ',' << ownership.second << ',' <<
                    targets[phase][index] << ',' << target.x << ',' << target.y << ',' << target.width << ',' << target.height <<
                    ",1,1,1," << fingerprint_scope << ",1,controlled-native-inspection-not-human-acceptance\n";
            }
        }
    }
    require(builds == 2 && frames == 4 && crops == 16 && rows == 16 && readbacks == 8 &&
        updates > 0 && updates <= 1404 && records.good(), "Press Tower native evidence exceeded its finite inspection budget");
    std::ofstream summary(output / "press-tower-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"press-tower-only\",\"players\":4,\"phases\":2,\"paint_states\":2,"
        "\"full_frames\":4,\"native_crops\":16,\"csv_rows\":16,\"readbacks\":8,\"level_builds\":2,\"level_updates\":" <<
        updates << ",\"controlled_heavy_contacts\":" << contacts << ",\"heavy_initial_fraction_min\":" << heavy_min <<
        ",\"human_acceptance\":false}\n";
    require(summary.good(), "Press Tower native summary write failed");
    std::cout << "PASS Press Tower: actual four-pane stair landings and hoist exits, clean/controlled-heavy, "
        "4 native frames/16 pane crops, frozen public state/RNG and Debug fingerprint; no human balance/readability acceptance\n";
}
