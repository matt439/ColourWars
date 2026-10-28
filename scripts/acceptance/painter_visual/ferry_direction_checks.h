// ROAD#27: controlled paint contacts establish inspection states. Production
// scene ownership, Level motion commitment and drawing remain in use.
struct FerryDirectionFixture
{
    std::unique_ptr<Level> level;
    TranslatingStructurePaintable* ferry = nullptr;
};

FerryDirectionFixture ferry_direction_level(Application& app, const LevelLoadedInfo& info,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences, int count,
    bool left_heading)
{
    MenuLevelSettings settings; settings.set_stage("turbulence_ferry_trial"); settings.set_player_count(count);
    settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
        (count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player));
    settings.begin_input_claims(3); constexpr std::array<int, 4> slots{ 3, 1, 2, 0 };
    for (int seat = 0; seat < count; ++seat)
    {
        if (seat != 0) { require(settings.claim_input_slot(slots[static_cast<size_t>(seat)]), "Ferry input claim failed"); }
        settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
        settings.set_player_weapon(seat, WeaponType::sprayer);
    }
    settings.set_duration(120.0f); app.viewport_manager()->set_layout(settings.screen_layout());
    const TeamColour colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
    LevelObjectBuilder objects(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint(),
        &presentation.gate_permission(), &presentation.ferry_direction());
    auto scene = std::make_unique<Scene>(app.thread_pool(), app.partitioner()); scene->set_bounds(info.out_of_bounds_rectangle());
    for (auto& object : objects.build_non_collision_objects(info.non_collision_objects_json())) { scene->add(std::move(object)); }
    for (auto& shadow : objects.build_shadows(info.collision_objects_json())) { scene->add(std::move(shadow)); }
    auto collision = objects.build_collision_objects(info.collision_objects_json(), colours);
    for (auto& shadow : objects.build_moving_shadows(info.collision_objects_json(), collision)) { scene->add(std::move(shadow)); }
    TranslatingStructurePaintable* ferry = nullptr; size_t movers = 0;
    for (auto& object : collision)
    {
        if (auto* moving = dynamic_cast<TranslatingStructurePaintable*>(object.get()))
        { if (movers == (left_heading ? 1u : 0u)) { ferry = moving; } ++movers; }
        scene->add(std::move(object));
    }
    require(ferry != nullptr && movers == 2, "Ferry fixture lost an authored mover");
    std::vector<Player*> players;
    PlayerBuilder player_builder(&tuning, &presentation);
    for (auto& player : player_builder.build_players(settings, &info, colours, app.render_resources(),
        app.audio_resources(), &preferences, app.viewport_manager())) { players.push_back(scene->add(std::move(player))); }
    auto level = std::make_unique<Level>(std::move(scene), std::move(players),
        objects.build_viewport_dividers(app.viewport_manager()), colours, info.camera_bounds_rectangle(),
        info.zoom_out_start_bounds_rectangle(), info.zoom_out_finish_bounds_rectangle(), &tuning, &presentation,
        L"Ferry direction inspection", info.sound_bank_name(), info.music_name(), info.music_volume(),
        info.end_sting_name(), info.end_sting_volume(), app.resolution_manager(), app.viewport_manager(),
        app.render_resources(), app.audio_resources(), &preferences, app.mouse(), info.stations(), OpeningMode::off,
        settings.duration(), nullptr, &settings);
    return { std::move(level), ferry };
}

Frame ferry_direction_frame(Application& app, const Canvas& canvas, const TranslatingStructurePaintable& ferry,
    const Camera& camera, const Viewport& pane, bool miniature, bool cue, size_t& reads)
{
    Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
    DrawList list = renderer.view(0); list.set_viewport(pane); list.set_camera(camera); list.set_filter(TextureFilter::point);
    canvas.draw(list, camera.visible_rectangle(pane));
    if (cue)
    { if (miniature) { ferry.draw_miniature(list); } else { ferry.draw(list); } }
    else
    {
        const TranslatedDrawScope translated(list, ferry.translation());
        if (miniature) { ferry.StructurePaintable::draw_miniature(list); } else { ferry.StructurePaintable::draw(list); }
    }
    ++reads; return read(renderer);
}

void ferry_direction_paint(Application& app, TranslatingStructurePaintable& ferry, const GameTuning& tuning,
    PlayerTeam team, bool half = false)
{
    RectangleF splash = ferry.bounds(); splash.inflate(24.0f);
    if (half) { splash.x = ferry.bounds().center().x + 0.25f; splash.width = ferry.bounds().right() + 24.0f - splash.x; }
    const ProjectileTuning& paint = tuning.projectile(ProjectileType::spray);
    splash.offset(paint.details.col_rect_size / 2.0f);
    const ProjectileSpray shot(splash, Vector2F::ZERO, team, team == PlayerTeam::a ? 0 : 1, Colour::white,
        app.render_resources(), resolve_projectile_strip(paint, *app.render_resources()), &paint);
    ferry.on_contact(shot, Vector2F::ZERO, 0.0f); ferry.update(10.0f);
}

void ferry_direction_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
    // Only the headings are changed in this private stage. Restoring them must
    // recover the original JSON, including unknown future authored fields.
    std::ifstream source(content / "levels" / "turbulence_ferry_trial.json", std::ios::binary);
    require(source.good(), "Cannot read ferry trial source");
    const std::string bytes((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    rapidjson::Document original; original.Parse(bytes.c_str()); require(!original.HasParseError(), "Cannot parse ferry source");
    rapidjson::Document mirrored; mirrored.CopyFrom(original, mirrored.GetAllocator()); size_t changed_headings = 0;
    for (auto& object : mirrored["collision_objects"].GetArray())
    {
        if (object.HasMember("motion") && object["motion"].HasMember("drive") &&
            std::string(object["motion"]["drive"].GetString()) == "paint")
        { object["motion"]["team_a_heads"].SetString("left"); ++changed_headings; }
    }
    require(changed_headings == 2, "Ferry mirror must change exactly two headings");
    rapidjson::Document restored; restored.CopyFrom(mirrored, restored.GetAllocator());
    for (auto& object : restored["collision_objects"].GetArray())
    { if (object.HasMember("motion")) { object["motion"]["team_a_heads"].SetString("right"); } }
    require(restored == original, "Ferry mirror changed more than authored headings");
    rapidjson::StringBuffer buffer; rapidjson::Writer<rapidjson::StringBuffer> writer(buffer); mirrored.Accept(writer);
    const auto left_path = output / "ferry-direction-left-stage.json";
    { std::ofstream file(left_path, std::ios::binary); file << buffer.GetString(); require(file.good(), "Cannot write isolated ferry stage"); }
    LevelLoadedInfo left_info(path_to_utf8(left_path).c_str()); left_info.validate();
    const LevelLoadedInfo& right_info = *levels.get("turbulence_ferry_trial");
    const auto& stage_look = presentation.main_menu().stage_select;
    const RectangleF preview_box(stage_look.preview_position, stage_look.preview_size);
    const Camera menu_camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), app.viewport_manager()->fullscreen_viewport());
    const Viewport miniature_pane(menu_camera.calculate_view_rectangle(preview_box));
    const Camera miniature_camera = Camera::frame(right_info.zoom_out_finish_bounds_rectangle(), miniature_pane);
    constexpr std::array<const char*, 7> names{ "unpainted", "a-moving", "late-b", "b-moving", "b-stop", "tie", "a-stop" };
    size_t builds = 0, updates = 0, reads = 0, contexts = 0, crops = 0, rows = 0;
    std::ofstream records(output / "ferry-direction.csv");
    records << "heading,players,state,path,demand_team,demand_heading,committed_heading,movement_heading,motion,scale,crop_x,crop_y,crop_width,crop_height,changed_pixels\n";
    for (const bool left : { false, true })
    {
        for (const int count : { 1, 2, 4 })
        {
            std::srand(227); auto fixture = ferry_direction_level(app, left ? left_info : right_info,
                tuning, presentation, preferences, count, left); ++builds;
            Level& level = *fixture.level; auto& ferry = *fixture.ferry; const auto inputs = neutral_inputs();
            const auto step = [&]() { level.update(inputs, dt); require(++updates <= 5000, "Ferry native update budget exceeded"); };
            for (int tick = 0; tick < 600 && level.state() != LevelState::active; ++tick) { step(); }
            require(level.state() == LevelState::active, "Ferry native countdown did not finish");
            require(level.place_player(0, { ferry.bounds().center().x, ferry.bounds().top() - level.player(0).rectangle().height }),
                "Ferry native station placement failed");
            for (int tick = 0; tick < 30; ++tick) { step(); }
            const float initial_offset = ferry.translation().x;
            const Camera native_camera = level.view_camera(0);
            std::array<std::array<std::vector<unsigned char>, 7>, 2> masks;
            for (size_t state = 0; state < names.size(); ++state)
            {
                if (state == 1) { ferry_direction_paint(app, ferry, tuning, PlayerTeam::a); for (int tick = 0; tick < 50; ++tick) { step(); } }
                if (state == 2) { ferry_direction_paint(app, ferry, tuning, PlayerTeam::b); }
                if (state == 3) { step(); }
                if (state == 4) { for (int tick = 0; tick < 130; ++tick) { step(); } }
                if (state == 5) { ferry_direction_paint(app, ferry, tuning, PlayerTeam::a, true); step(); }
                if (state == 6) { ferry_direction_paint(app, ferry, tuning, PlayerTeam::a); for (int tick = 0; tick < 130; ++tick) { step(); } }
                const auto cue = ferry.cue_state(); const int a_heading = left ? -1 : 1;
                require(cue.paint_driven && cue.demand_heading == (state == 0 || state == 5 ? 0 :
                    state == 1 || state == 6 ? a_heading : -a_heading), "Ferry native demand did not match contacts");
                if (state == 2) { require(cue.committed_heading == a_heading && cue.movement_heading == a_heading,
                    "Late repaint rewrote already committed movement"); }
                if (state == 1 || state == 3) { require(cue.motion == FerryMotionCue::moving, "Ferry midspan cue did not report actual movement"); }
                if (state == 0 || state == 5) { require(cue.motion == FerryMotionCue::held, "Unpainted/tied ferry did not report a committed hold"); }
                if (state == 4 || state == 6)
                { require(cue.motion == ((state == 4 ? -a_heading : a_heading) < 0 ? FerryMotionCue::left_stop : FerryMotionCue::right_stop),
                    "Ferry endpoint cue did not report the reached stop"); }
                const auto before = observation(level); const auto motion = ferry.motion_state(); const auto paint = ferry.paint_total();
#if COLOURWARS_DEV_TOOLS
                const auto fingerprint = MatchFingerprint::of(level, 0);
#endif
                std::srand(22727); const int expected_random = std::rand(); std::srand(22727);
                const Frame context = render(level, *app.renderer()); ++reads;
                require(context.pixels == render(level, *app.renderer()).pixels, "Ferry context redraw changed pixels"); ++reads;
                const std::string stem = "ferry-direction-" + std::string(left ? "left" : "right") + '-' + std::to_string(count) + "p-" + names[state];
                save(context, output / (stem + "-context.ppm")); ++contexts;
                for (size_t path = 0; path < 2; ++path)
                {
                    const Viewport pane = path == 0 ? app.viewport_manager()->player_viewport(0) : miniature_pane;
                    Camera camera = path == 0 ? native_camera : miniature_camera;
                    // Preserve actual pane scale, while holding the sign at a
                    // common native station for independent shape comparison.
                    if (path == 0) { camera.translation = ferry.cue_bounds().center() - pane.size() / (2.0f * camera.scale); }
                    else { camera.translation.x += ferry.translation().x - initial_offset; }
                    const Frame actual = ferry_direction_frame(app, canvas, ferry, camera, pane, path != 0, true, reads);
                    const Frame base = ferry_direction_frame(app, canvas, ferry, camera, pane, path != 0, false, reads);
                    require(actual.pixels == ferry_direction_frame(app, canvas, ferry, camera, pane, path != 0, true, reads).pixels,
                        "Ferry isolated redraw changed pixels"); preferences.reduce_effects = true;
                    require(actual.pixels == ferry_direction_frame(app, canvas, ferry, camera, pane, path != 0, true, reads).pixels,
                        "Reduced effects hid ferry information"); preferences.reduce_effects = false;
                    RectangleF projected = camera.calculate_view_rectangle(ferry.cue_bounds()); projected.offset(pane.position());
                    const RectangleI crop(static_cast<int>(std::floor(projected.x)), static_cast<int>(std::floor(projected.y)),
                        static_cast<int>(std::ceil(projected.right())) - static_cast<int>(std::floor(projected.x)),
                        static_cast<int>(std::ceil(projected.bottom())) - static_cast<int>(std::floor(projected.y)));
                    require(crop.width >= 17 && crop.height >= 9, "Ferry sign is too small for its native glyph grammar");
                    size_t changed = 0;
                    for (int y = 0; y < actual.height; ++y) { for (int x = 0; x < actual.width; ++x)
                    {
                        const size_t pixel = (static_cast<size_t>(y) * actual.width + x) * 4;
                        if (same_pixel(actual, base, pixel)) { continue; } ++changed;
                        require(x >= crop.x && x < crop.right() && y >= crop.y && y < crop.bottom(), "Ferry cue escaped its declared rectangle");
                    } }
                    require(changed > 0, "Ferry cue changed no native pixel"); const Frame detail = crossfeed_crop(actual, crop);
                    masks[path][state] = gate_permission_ink(detail, presentation.ferry_direction().ink);
                    const std::string path_stem = stem + (path == 0 ? "-native" : "-miniature");
                    save(detail, output / (path_stem + "-detail.ppm")); ++crops;
                    save(gate_permission_monochrome(detail, presentation.ferry_direction().ink), output / (path_stem + "-ink-detail.ppm")); ++crops;
                    records << (left ? "left" : "right") << ',' << count << ',' << names[state] << ',' << (path == 0 ? "native" : "miniature") << ',' <<
                        static_cast<int>(cue.demand_team) << ',' << cue.demand_heading << ',' << cue.committed_heading << ',' << cue.movement_heading << ',' <<
                        static_cast<int>(cue.motion) << ',' << camera.scale << ',' << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << ',' << changed << '\n'; ++rows;
                }
                const auto after = ferry.cue_state();
                require(before == observation(level) && motion.phase == ferry.motion_state().phase && motion.current_offset == ferry.motion_state().current_offset &&
                    paint.total == ferry.paint_total().total && paint.team_a == ferry.paint_total().team_a && paint.team_b == ferry.paint_total().team_b &&
                    paint.player_areas == ferry.paint_total().player_areas && cue.demand_heading == after.demand_heading &&
                    cue.committed_heading == after.committed_heading && cue.movement_heading == after.movement_heading && cue.motion == after.motion &&
                    std::rand() == expected_random, "Ferry drawing changed simulation, paint credit, committed state or RNG");
#if COLOURWARS_DEV_TOOLS
                require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(), "Ferry drawing changed Debug replay fingerprint");
#endif
            }
            for (size_t path = 0; path < 2; ++path)
            {
                require(masks[path][0] == masks[path][5] && masks[path][1] != masks[path][2] && masks[path][2] != masks[path][3] &&
                    masks[path][3] != masks[path][4] && masks[path][4] != masks[path][6] && masks[path][0] != masks[path][4],
                    "Ferry demand, reversal, tie or endpoint shapes cannot be distinguished without hue");
            }
        }
    }
    // StagePreview builds the shipped, unpainted initial ferry independently.
    Save preview_save; GameData data; data.set_application(&app); data.set_save(&preview_save); data.set_level_infos(&levels);
    data.set_tuning(&tuning); data.set_presentation(&presentation); StagePreview preview("ferry-preview", preview_box, &data);
    preview.set_stage("turbulence_ferry_trial");
    const LevelObjectBuilder preview_builder(app.render_resources(), app.audio_resources(), &preferences,
        &tuning.paint(), &presentation.gate_permission(), &presentation.ferry_direction());
    auto preview_objects = preview_builder.build_collision_objects(right_info.collision_objects_json(), TeamColour{});
    const TranslatingStructurePaintable* preview_ferry = nullptr;
    for (const auto& object : preview_objects)
    {
        preview_ferry = dynamic_cast<const TranslatingStructurePaintable*>(object.get());
        if (preview_ferry != nullptr) { break; }
    }
    require(preview_ferry && preview_ferry->cue_state().motion == FerryMotionCue::unstepped,
        "Initial ferry miniature must not claim a committed step");
    const Frame isolated = ferry_direction_frame(app, canvas, *preview_ferry, miniature_camera, miniature_pane, true, true, reads);
    const Frame base = ferry_direction_frame(app, canvas, *preview_ferry, miniature_camera, miniature_pane, true, false, reads);
    RectangleF projected = miniature_camera.calculate_view_rectangle(preview_ferry->cue_bounds()); projected.offset(miniature_pane.position());
    const RectangleI preview_crop(static_cast<int>(std::floor(projected.x)), static_cast<int>(std::floor(projected.y)),
        static_cast<int>(std::ceil(projected.right())) - static_cast<int>(std::floor(projected.x)),
        static_cast<int>(std::ceil(projected.bottom())) - static_cast<int>(std::floor(projected.y)));
    const Frame preview_sign = crossfeed_crop(isolated, preview_crop), preview_base = crossfeed_crop(base, preview_crop);
    std::vector<size_t> preview_witness;
    for (size_t pixel = 0; pixel < preview_sign.pixels.size(); pixel += 4)
    { if (!same_pixel(preview_sign, preview_base, pixel)) { preview_witness.push_back(pixel); } }
    const auto preview_frame = [&]()
    {
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
        list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, { 0, 0, 1280, 720 }); preview.draw(list); ++reads; return read(renderer);
    };
    const Frame miniature = preview_frame(); require(miniature.pixels == preview_frame().pixels, "Ferry StagePreview redraw changed pixels");
    save(miniature, output / "ferry-direction-stage-preview-context.ppm"); ++contexts;
    require(!preview_witness.empty(), "Ferry miniature witness is absent");
    const Frame detail = crossfeed_crop(miniature, preview_crop); save(detail, output / "ferry-direction-stage-preview-detail.ppm"); ++crops;
    for (const size_t pixel : preview_witness)
    { require(same_pixel(detail, preview_sign, pixel), "Actual StagePreview omitted or changed the initial ferry sign"); }
    records.close(); require(records.good() && builds == 6 && rows == 84 && contexts == 43 && crops == 169 && reads == 424,
        "Ferry native evidence inventory changed");
    std::ofstream summary(output / "ferry-direction-summary.json");
    summary << "{\"outcome\":\"passed\",\"level_builds\":" << builds << ",\"level_updates\":" << updates << ",\"readbacks\":" << reads <<
        ",\"full_frames\":" << contexts << ",\"native_crops\":" << crops << ",\"rows\":" << rows << ",\"states\":7,\"headings\":2,\"miniature_scale\":" << miniature_camera.scale << '}';
    summary.close(); require(summary.good(), "Cannot flush ferry native summary");
    std::cout << "PASS ferry direction native matrix: both headings, native1/2/4P/miniature, current demand versus committed movement/end stops, "
        "shape distinctions, reduced-effects parity, bounded pure drawing and actual StagePreview. Controlled inspection; attended trial play remains separate.\n";
}
