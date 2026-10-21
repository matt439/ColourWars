// Included inside painter_visual.cpp's private namespace, after its shared
// renderer, builder and raster helpers. The fixtures exercise composition and
// story timing; they do not copy the monitor's decorative geometry.

enum class MonitorCapture { plain, picker, preview, saved, closed };

bool monitor_pixel_equal(const Frame& first, const Frame& second, const Vector2F& at)
{
    const int x = std::clamp(static_cast<int>(at.x), 0, first.width - 1);
    const int y = std::clamp(static_cast<int>(at.y), 0, first.height - 1);
    return same_pixel(first, second,
        (static_cast<size_t>(y) * static_cast<size_t>(first.width) + static_cast<size_t>(x)) * 4);
}

std::uint32_t monitor_rgb(const Frame& frame, int x, int y)
{
    require(x >= 0 && x < frame.width && y >= 0 && y < frame.height,
        "monitor colour sample escaped the render target");
    const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
        static_cast<size_t>(x)) * 4;
    return (static_cast<std::uint32_t>(frame.pixels[offset]) << 16) |
        (static_cast<std::uint32_t>(frame.pixels[offset + 1]) << 8) |
        static_cast<std::uint32_t>(frame.pixels[offset + 2]);
}

bool monitor_contains(const RectangleF& outer, const RectangleF& inner)
{
    return inner.width > 0.0f && inner.height > 0.0f && inner.x >= outer.x && inner.y >= outer.y &&
        inner.right() <= outer.right() && inner.bottom() <= outer.bottom();
}

void monitor_palette_pixels(const Frame& frame, const Monitor& monitor, const RectangleF& screen,
    const Frame* saved = nullptr)
{
    // Independent shipped-art fixture: Muted Day's table in
    // game/art/terrain/terrain.json, in role order. Reading the presentation's
    // frame list or drawing a second atlas row would bless a reordered list or
    // a wrong/tinted DDS. Duplicate white roles still need distinct cells.
    constexpr std::array<std::uint32_t, 26> expected{ {
        0x000000, 0xC9D3D9, 0xC3C8C0, 0xA7B09E, 0x8E958C, 0xF2F1EA,
        0x8C8A86, 0xB4B1AA, 0x6A6865, 0x9A9186, 0x756E65, 0xC9AE8C,
        0x3E3E40, 0xB5AFA3, 0x45423E, 0xB8958C, 0x8F716A, 0xB8BCBE,
        0x6E7275, 0x9CA2A2, 0x7E8382, 0xFFFFFF, 0xF4F2EC, 0x2A2A2A,
        0xFFFFFF, 0x945E52 } };
    const RectangleF picture = monitor.picture(screen);
    const RectangleF glass = monitor.glass(screen);
    const RectangleF slot = monitor.empty_slot(screen);
    require(monitor_contains(glass, slot) && slot.x >= picture.x && slot.right() < picture.right() &&
        slot.y >= picture.bottom() && std::abs(slot.width - slot.height) < 0.01f,
        "empty palette slot was not a square contained after the program's picture");
    const int y = static_cast<int>(slot.center().y);
    const int left = static_cast<int>(std::ceil(picture.x));
    const int right = static_cast<int>(std::floor(slot.x));
    const std::uint32_t gap_colour = monitor_rgb(frame, static_cast<int>(slot.center().x), y);
    struct Run { int left, right; std::uint32_t colour; };
    std::vector<Run> runs;
    for (int x = left; x < right; ++x)
    {
        const std::uint32_t colour = monitor_rgb(frame, x, y);
        if (colour == gap_colour) { continue; }
        if (!runs.empty() && runs.back().right == x && runs.back().colour == colour)
        {
            runs.back().right = x + 1;
        }
        else { runs.push_back({ x, x + 1, colour }); }
    }
    require(runs.size() == expected.size(), "monitor palette did not show all 26 distinct role cells");
    int previous_right = left;
    for (size_t index = 0; index < expected.size(); ++index)
    {
        const Run& run = runs[index];
        require(run.colour == expected[index], "monitor palette pixels differed from Muted Day role order");
        require(run.left > previous_right, "monitor palette lost an outer or inter-cell gap");
        const int x = (run.left + run.right) / 2;
        int top = y, bottom = y + 1;
        while (top > static_cast<int>(std::ceil(picture.bottom())) &&
            monitor_rgb(frame, x, top - 1) == run.colour) { --top; }
        while (bottom < static_cast<int>(std::floor(glass.bottom())) &&
            monitor_rgb(frame, x, bottom) == run.colour) { ++bottom; }
        require(std::abs((run.right - run.left) - (bottom - top)) <= 1 &&
            std::abs(static_cast<float>(run.right - run.left) - slot.width) <= 1.0f,
            "monitor palette cells were stretched or differed from the empty square slot");
        if (saved != nullptr)
        {
            for (int row = top; row < bottom; ++row)
            {
                for (int column = run.left; column < run.right; ++column)
                {
                    const size_t offset = (static_cast<size_t>(row) * static_cast<size_t>(frame.width) +
                        static_cast<size_t>(column)) * 4;
                    require(same_pixel(frame, *saved, offset), "saving recoloured an existing terrain palette cell");
                }
            }
        }
        previous_right = run.right;
    }
    require(previous_right < right, "final terrain colour ran into the empty palette slot");
    const int after_slot = static_cast<int>(std::ceil(slot.right()));
    const int program_right = static_cast<int>(std::floor(picture.right()));
    require(after_slot < program_right, "monitor palette lost the gap after its final empty slot");
    for (int x = after_slot; x < program_right; ++x)
    {
        require(monitor_rgb(frame, x, y) == gap_colour, "a palette colour appeared after the final empty slot");
    }
}

Frame monitor_fixture(Application& app, const Monitor& monitor, const RectangleF& screen,
    const Canvas& canvas, const Canvas& picture, MonitorCapture phase,
    bool front_details = true, const Silhouette* silhouette = nullptr)
{
    Renderer& renderer = *app.renderer();
    const RectangleF window(Vector2F::ZERO, app.resolution_manager()->resolution_vec());
    renderer.begin_frame();
    renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(app.viewport_manager()->fullscreen_viewport());
    list.set_camera(Camera::DEFAULT_CAMERA);
    list.set_filter(TextureFilter::point);
    canvas.draw(list, window);
    monitor.draw(list, screen);
    picture.draw(list, monitor.picture(screen));
    if (phase == MonitorCapture::picker) { monitor.draw_picker(list, screen, Colour::red, Colour::blue); }
    if (phase == MonitorCapture::preview)
    {
        monitor.draw_preview(list, monitor.picture(screen), Colour::red);
    }
    if (phase == MonitorCapture::closed) { monitor.draw_closed(list, screen); }
    if (front_details) { monitor.draw_front_details(list, screen); }
    if (phase != MonitorCapture::closed) { monitor.draw_close_button(list, screen); }
    if (phase == MonitorCapture::saved) { monitor.draw_saved(list, screen, Colour::red); }
    if (silhouette != nullptr) { silhouette->draw(list, window, monitor.persona(), {}); }
    return read(renderer);
}

void monitor_frozen(Application& app, Level& level, const std::filesystem::path& output,
    const std::string& stem, std::ofstream& records, bool retain_frame = true)
{
    const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    // Drawing may neither advance its seeded persona nor consume randomness
    // that a subsequent simulation update would otherwise receive.
    std::srand(19481);
    const int expected_random = std::rand();
    std::srand(19481);
    const Frame first = render(level, *app.renderer());
    for (int repeat = 0; repeat < 3; ++repeat)
    {
        require(first.pixels == render(level, *app.renderer()).pixels,
            "frozen story redraw changed monitor, decorations or scene pixels");
    }
    require(std::rand() == expected_random, "story drawing consumed random numbers");
    require(before == observation(level), "story drawing changed observed simulation state");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "story drawing changed the replay fingerprint");
#endif
    if (retain_frame) { save(first, output / (stem + ".ppm")); }
    records << stem << ',' << level.player_count() << ',' << static_cast<int>(level.state()) << ','
        << level.level_end_info().team_a_area << ',' << level.level_end_info().team_b_area << '\n';
}

std::unique_ptr<Level> monitor_story(Application& app, LevelBuilder& builder, int count)
{
    MenuLevelSettings settings;
    settings.set_player_count(count);
    settings.set_stage("movement_course");
    settings.set_duration(1.25f);
    settings.set_screen_layout(count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
    settings.begin_input_claims(3);
    constexpr int slots[] = { 3, 1, 2, 0 };
    for (int view = 1; view < count; ++view)
    {
        require(settings.claim_input_slot(slots[view]), "monitor story input claim failed");
    }
    for (int view = 0; view < count; ++view)
    {
        settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
        settings.set_player_weapon(view, WeaponType::roller);
    }
    app.viewport_manager()->set_layout(settings.screen_layout());
    std::srand(409 + count);
    return builder.build_level(settings, TeamColourPair::blue_orange, OpeningMode::full);
}

std::filesystem::path monitor_static_backdrop(const std::filesystem::path& content,
    const std::filesystem::path& output)
{
    const std::filesystem::path source = content / "levels" / "movement_course.json";
    std::ifstream input(source, std::ios::binary);
    require(input.good(), "cannot read the shipped monitor story stage");
    const std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    rapidjson::Document original; original.Parse(bytes.c_str());
    require(!original.HasParseError() && original.IsObject() && original.HasMember("non_collision_objects") &&
        original["non_collision_objects"].IsArray(), "monitor story stage has no decoration array");
    rapidjson::Document stationary; stationary.CopyFrom(original, stationary.GetAllocator());
    size_t removed = 0;
    for (auto& decoration : stationary["non_collision_objects"].GetArray())
    {
        if (!decoration.HasMember("drift")) { continue; }
        require(decoration.HasMember("type") && decoration["type"].IsString() &&
            std::string(decoration["type"].GetString()) == "Visual" && decoration.HasMember("frame_name") &&
            decoration["frame_name"].IsString() && (std::string(decoration["frame_name"].GetString()) == "cloud_big" ||
                std::string(decoration["frame_name"].GetString()) == "cloud_small"),
            "monitor isolation would remove drift from an unexpected decoration");
        decoration.RemoveMember("drift"); ++removed;
    }
    require(removed > 0, "monitor static-backdrop fixture removed no drifting clouds");
    // Reinsert only the removed values into another copy. Equality with the
    // original proves that geometry, collision, art, audio and unknown fields
    // have not been rewritten to make the click comparison pass.
    rapidjson::Document restored; restored.CopyFrom(stationary, restored.GetAllocator());
    const auto& original_decorations = original["non_collision_objects"];
    auto& restored_decorations = restored["non_collision_objects"];
    for (rapidjson::SizeType index = 0; index < original_decorations.Size(); ++index)
    {
        if (!original_decorations[index].HasMember("drift")) { continue; }
        rapidjson::Value drift(original_decorations[index]["drift"], restored.GetAllocator());
        restored_decorations[index].AddMember(rapidjson::StringRef("drift"), drift, restored.GetAllocator());
    }
    require(restored == original, "monitor isolation changed fields beyond decorative drift");
    rapidjson::StringBuffer serialized; rapidjson::Writer<rapidjson::StringBuffer> writer(serialized);
    stationary.Accept(writer);
    const std::filesystem::path fixture = output / "monitor-static-backdrop" / "movement_course.json";
    std::filesystem::create_directories(fixture.parent_path());
    std::ofstream file(fixture, std::ios::binary);
    file.write(serialized.GetString(), static_cast<std::streamsize>(serialized.GetSize()));
    require(file.good(), "monitor static-backdrop fixture write failed");
    std::ofstream record(output / "monitor-static-backdrop.csv");
    record << "source,removed_decorative_drift,other_fields_unchanged\nmovement_course," << removed << ",true\n";
    require(record.good(), "monitor static-backdrop isolation record write failed");
    return fixture;
}

void monitor_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    std::ofstream records(output / "monitor.csv");
    records << "capture,players,state,team_a_area,team_b_area\n";
    const OpeningPresentation& look = presentation.opening();
    foreground_matrix(app, canvas, look, output);
    require(look.bezel_shades.size() == 6, "monitor review requires the six authored body shades");
    const Canvas picture_canvas(app.render_resources(), Colour(0.2f, 0.55f, 0.4f));
    const Silhouette silhouette(&look, app.render_resources());
    const RectangleF reference(0, 0, 1280, 720);
    MonitorPersona persona;
    persona.mug = true;
    persona.coffee_ring = { 0.77f, 0.9f };
    persona.stickers = { { { 0.27f, 0.92f }, 0, 0 }, { { 0.65f, 0.91f }, 3, 0 } };
    persona.crack = { { 0.005f, 0.65f }, { 0.03f, 0.63f }, { 0.025f, 0.66f }, { 0.06f, 0.68f } };
    const Monitor geometry(&look, app.render_resources(), persona);
    const RectangleF picture = geometry.picture(reference);
    // A manually chosen sticky note reaches across the picture boundary. A
    // normal random roll is not guaranteed to expose the foreground-order bug.
    persona.sticky_notes = { { { picture.x / reference.width - 0.015f,
        picture.y / reference.height + 0.09f }, 0, 0.1f }, { { 0.72f, 0.005f }, 1, -0.1f } };

    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    std::vector<std::vector<unsigned char>> shades;
    for (size_t shade = 0; shade < look.bezel_shades.size(); ++shade)
    {
        persona.shade = shade;
        const Monitor monitor(&look, app.render_resources(), persona);
        const Frame frame = monitor_fixture(app, monitor, reference, canvas, picture_canvas,
            MonitorCapture::plain, true, &silhouette);
        for (const auto& previous : shades)
        {
            require(previous != frame.pixels, "two authored monitor body shades rendered identically");
        }
        shades.push_back(frame.pixels);
        save(frame, output / ("monitor-adult-mug-shade" + std::to_string(shade + 1) + "-1280.ppm"));
    }
    persona.child = true;
    persona.mug = false;
    const Monitor child(&look, app.render_resources(), persona);
    save(monitor_fixture(app, child, reference, canvas, picture_canvas,
        MonitorCapture::plain, true, &silhouette), output / "monitor-child-stickers-1280.ppm");
    persona.child = false;
    persona.mug = true;

    for (const Vector2I size : { Vector2I(640, 480), Vector2I(1024, 768), Vector2I(1280, 720),
        Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        const RectangleF window(0, 0, static_cast<float>(size.x), static_cast<float>(size.y));
        const Monitor monitor(&look, app.render_resources(), persona);
        const auto persona_before = monitor_persona_observation(monitor.persona());
        for (const bool pushed : { false, true })
        {
            if (pushed && size.x != 1280) { continue; }
            const RectangleF glass = monitor.glass(window);
            const RectangleF screen = pushed ? monitor.screen_for_glass(RectangleF(
                glass.x * 0.5f, glass.y * 0.5f,
                (glass.width + window.width) * 0.5f, (glass.height + window.height) * 0.5f)) : window;
            const std::string prefix = "monitor-" + std::string(pushed ? "pushed-" : "native-") +
                std::to_string(size.x);
            const Frame behind = monitor_fixture(app, monitor, screen, canvas, picture_canvas,
                MonitorCapture::plain, false);
            const Frame front = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::plain);
            const Frame picker = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::picker);
            const Frame preview = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::preview);
            const Frame saved = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::saved);
            const Frame closed = monitor_fixture(app, monitor, screen, canvas, picture_canvas, MonitorCapture::closed);
            const RectangleF area = monitor.picture(screen);
            const RectangleF screen_glass = monitor.glass(screen);
            const RectangleF program(area.x, area.y, area.width, screen_glass.bottom() - area.y);
            const RectangleF empty_slot = monitor.empty_slot(screen);
            monitor_palette_pixels(front, monitor, screen, &saved);
            monitor_palette_pixels(picker, monitor, screen);
            monitor_palette_pixels(preview, monitor, screen);
            for (const int which : { 0, 1 })
            {
                const RectangleF swatch = monitor.swatch(screen, which);
                require(monitor_contains(program, swatch) && monitor_contains(screen_glass, swatch) &&
                    swatch.bottom() < empty_slot.y,
                    "picker colour escaped the program/glass or covered the final empty slot");
                const Vector2F centre = swatch.center();
                require(monitor_rgb(picker, static_cast<int>(centre.x), static_cast<int>(centre.y)) ==
                    (which == 0 ? 0xFF0000u : 0x0000FFu), "picker lost a complete red or blue colour cell");
            }
            const RectangleI picker_delta = foreground_delta_bounds(picker, front);
            RectangleF raster_program = program;
            raster_program.inflate(1.0f); // Integer destinations may begin at the preceding pixel.
            require(monitor_contains(raster_program, RectangleF(static_cast<float>(picker_delta.x),
                static_cast<float>(picker_delta.y), static_cast<float>(picker_delta.width),
                static_cast<float>(picker_delta.height))) && picker_delta.bottom() <= empty_slot.y,
                "picker panel escaped the program or overlapped the empty palette slot");
            size_t overlapping = 0;
            size_t untinted = 0;
            for (int y = std::max(0, static_cast<int>(area.y)); y < std::min(size.y, static_cast<int>(area.bottom())); ++y)
            {
                for (int x = std::max(0, static_cast<int>(area.x)); x < std::min(size.x, static_cast<int>(area.right())); ++x)
                {
                    const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(size.x) + static_cast<size_t>(x)) * 4;
                    if (!same_pixel(behind, front, offset))
                    {
                        ++overlapping;
                        if (same_pixel(front, preview, offset)) { ++untinted; }
                    }
                }
            }
            require(overlapping > 100 && untinted > overlapping / 2,
                "picture or preview covered foreground monitor notes");
            require(monitor_pixel_equal(behind, front, area.center()),
                "monitor decoration shading changed the picture interior");
            const Vector2F slot = monitor.empty_slot(screen).center();
            require(monitor_pixel_equal(behind, front, slot) && monitor_pixel_equal(front, preview, slot),
                "monitor details or preview changed the empty palette slot");
            require(!monitor_pixel_equal(front, preview, area.center()), "preview did not tint the picture");
            require(!monitor_pixel_equal(front, saved, slot) && monitor_pixel_equal(front, saved, area.center()),
                "saving did not fill only the intended palette slot");
            require(!monitor_pixel_equal(front, closed, area.center()), "closing did not replace the picture with the desktop");
            std::srand(9983);
            const int expected_random = std::rand();
            std::srand(9983);
            for (int repeat = 0; repeat < 3; ++repeat)
            {
                require(front.pixels == monitor_fixture(app, monitor, screen, canvas, picture_canvas,
                    MonitorCapture::plain).pixels, "frozen monitor fixture changed pixels");
                require(picker.pixels == monitor_fixture(app, monitor, screen, canvas, picture_canvas,
                    MonitorCapture::picker).pixels, "frozen monitor picker changed pixels");
            }
            require(std::rand() == expected_random, "monitor drawing consumed random numbers");
            require(persona_before == monitor_persona_observation(monitor.persona()), "monitor drawing changed its persona");
            save(front, output / (prefix + ".ppm"));
            save(picker, output / (prefix + "-picker.ppm"));
            save(preview, output / (prefix + "-preview.ppm"));
            save(saved, output / (prefix + "-saved.ppm"));
            save(closed, output / (prefix + "-closed.ppm"));
            if (!pushed && (size.x == 640 || size.x == 1280))
            {
                const int left = static_cast<int>(std::floor(area.x));
                const int top = std::min(picker_delta.y, static_cast<int>(std::floor(empty_slot.y))) - 2;
                const RectangleI detail(left, top,
                    static_cast<int>(std::ceil(area.right())) - left,
                    static_cast<int>(std::ceil(empty_slot.bottom())) - top + 2);
                const std::string detail_stem = "monitor-palette-" + std::to_string(size.x);
                save(foreground_crop(front, detail, 0), output / (detail_stem + "-detail.ppm"));
                save(foreground_crop(picker, detail, 0), output / (detail_stem + "-picker-detail.ppm"));
                save(foreground_crop(saved, detail, 0), output / (detail_stem + "-saved-detail.ppm"));
            }
        }
    }

    const OpeningTuning& opening = tuning.match().opening;
    const RevealTuning& reveal = tuning.match().reveal;
    const auto fixture = monitor_static_backdrop(content, output);
    Registry<LevelLoadedInfo> static_levels("Monitor static-backdrop Level");
    auto static_info = std::make_unique<LevelLoadedInfo>(fixture.string().c_str());
    static_info->validate(); static_levels.add("movement_course", std::move(static_info));
    LevelBuilder static_builder(app.viewport_manager(), app.render_resources(), app.audio_resources(),
        &preferences, &static_levels, &tuning, &presentation, app.resolution_manager(),
        app.thread_pool(), app.partitioner(), app.mouse());
    std::ofstream click_records(output / "monitor-click.csv");
    click_records << "capture,players,expected_click_response,changed_pixels,state_unchanged,draw_rng_unchanged\n";
    // The shipped pass retains all real cloud motion and its original story
    // snapshots. Repeat the identical story/input sequence with only backdrop
    // drift removed so Reduce effects can isolate the click accents: moving a
    // cloud back to its anchor is a legitimate separate visual response.
    for (const bool static_backdrop : { false, true })
    {
        for (int count : { 2, 4 })
        {
            auto level = monitor_story(app, static_backdrop ? static_builder : builder, count);
            resize(app, *level, { 1280, 720 });
            auto inputs = neutral_inputs();
            const std::string prefix = "monitor-" + std::to_string(count) +
                (static_backdrop ? "p-static-backdrop-story-" : "p-story-");
            const auto snap = [&](const char* beat)
            { monitor_frozen(app, *level, output, prefix + beat, records, !static_backdrop); };
            const auto advance = [&](float seconds)
            {
                const int ticks = static_cast<int>(std::ceil(seconds / dt));
                for (int tick = 0; tick < ticks; ++tick) { level->update(inputs, dt); }
            };
            snap("opening");
            advance(opening.settle + opening.preview * 0.65f);
            snap("preview-a");
            advance(opening.preview);
            snap("preview-b");
            for (int tick = 0; tick < 1200 && level->state() == LevelState::opening; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::start_countdown, "monitor story missed its countdown");
            snap("countdown");
            advance(std::max(0.0f, tuning.match().countdown - opening.push * 0.5f));
            require(level->state() == LevelState::start_countdown, "monitor story missed its glass push");
            snap("push");
            for (int tick = 0; tick < 600 && level->state() != LevelState::active; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::active, "monitor story did not reach active play");
            if (count == 2)
            {
                require(level->place_player(0, { 1200, 1275 }), "could not place painting reveal fixture");
                inputs[3].primary_shoot = true;
                inputs[3].x_movement = 1;
            }
            for (int tick = 0; tick < 600 && level->state() == LevelState::active; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::zoom_out, "monitor story missed its reveal pullback");
            advance(tuning.match().zoom_out * 0.5f);
            snap("pullback");
            for (int tick = 0; tick < 600 && level->state() == LevelState::zoom_out; ++tick) { level->update(inputs, dt); }
            require(level->state() == LevelState::overview, "monitor story did not reach its monitor overview");
            const LevelEndInfo info = level->level_end_info();
            require(count == 2 ? info.team_a_area > 0 && info.winning_team() == PlayerTeam::a :
                info.team_a_area == 0 && info.team_b_area == 0, "monitor reveal fixture painted an unexpected result");
            // Mirror only the elapsed 60 Hz updates, so captures bracket the
            // actual story edge without exposing or writing private story timers.
            float reveal_time = 0.0f;
            const auto reveal_tick = [&]()
            {
                level->update(inputs, dt);
                reveal_time += dt;
            };
            const auto reveal_before = [&](float target)
            {
                for (int tick = 0; tick < 1200 && reveal_time + dt < target; ++tick) { reveal_tick(); }
                require(level->state() == LevelState::overview && reveal_time < target && reveal_time + dt >= target,
                    "monitor reveal capture missed its timing edge");
            };
            const RectangleF window(0, 0, 1280, 720);
            const Monitor reveal_geometry(&look, app.render_resources(), {});
            const Vector2F click_tip = count == 2 ? reveal_geometry.swatch(window, 0).center() :
                reveal_geometry.close_button(window).center();
            size_t click_sample = 0;
            const auto click_response = [&](bool present)
            {
                const auto before = observation(*level);
#if COLOURWARS_DEV_TOOLS
                const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 0);
#endif
                std::srand(1957); const int expected_random = std::rand(); std::srand(1957);
                const Frame normal = render(*level, *app.renderer());
                preferences.reduce_effects = true;
                const Frame reduced = render(*level, *app.renderer());
                preferences.reduce_effects = false;
                const float reach = look.cursor_size * window.height * 2.0f;
                RectangleF response(click_tip - Vector2F(reach), Vector2F(reach * 2.0f));
                if (count == 2)
                {
                    // A winning click also ticks the two coverage cards in the
                    // picture. The no-paint close retains only the cursor cue.
                    const RectangleF picture = reveal_geometry.picture(window);
                    const float left = std::min(response.x, picture.x);
                    const float top = std::min(response.y, picture.y);
                    response = RectangleF(left, top, std::max(response.right(), picture.right()) - left,
                        std::max(response.bottom(), picture.bottom()) - top);
                }
                const size_t changed = foreground_local_delta(normal, reduced, response);
                require(present ? changed > 0 : changed == 0,
                    "real reveal click ticks were missing, premature or did not expire");
                require(before == observation(*level), "reveal click comparison changed simulation state");
                require(std::rand() == expected_random, "reveal click comparison consumed match randomness");
#if COLOURWARS_DEV_TOOLS
                require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
                    "reveal click comparison changed the replay fingerprint");
#endif
                constexpr const char* samples[] = { "before-click", "at-click", "after-expiry" };
                require(click_sample < 3, "monitor isolated click comparison ran too many samples");
                const std::string stem = prefix + samples[click_sample++];
                click_records << stem << ',' << count << ',' << present << ',' << changed << ",true,true\n";
                const RectangleI cursor_crop(static_cast<int>(click_tip.x - reach) - 2,
                    static_cast<int>(click_tip.y - reach) - 2, static_cast<int>(reach * 2) + 4,
                    static_cast<int>(reach * 2) + 4);
                save(foreground_crop(normal, cursor_crop, 0), output / (stem + "-cursor-detail.ppm"));
                save(foreground_crop(reduced, cursor_crop, 0), output / (stem + "-cursor-reduced-detail.ppm"));
                if (count == 2)
                {
                    const RectangleF picture = reveal_geometry.picture(window);
                    const RectangleI cards(static_cast<int>(picture.x), static_cast<int>(picture.y),
                        static_cast<int>(picture.width), static_cast<int>(picture.height * 0.6f));
                    save(foreground_crop(normal, cards, 0), output / (stem + "-cards-detail.ppm"));
                    save(foreground_crop(reduced, cards, 0), output / (stem + "-cards-reduced-detail.ppm"));
                }
            };
            reveal_before(reveal.walk_in + reveal.hover * 0.65f);
            snap(count == 2 ? "reveal-preview" : "closing-hesitate");
            const float click_at = reveal.walk_in + reveal.hover + reveal.hover + reveal.decide;
            reveal_before(click_at);
            snap(count == 2 ? "reveal-before-click" : "closing-before-click");
            if (static_backdrop) { click_response(false); }
            reveal_tick();
            snap(count == 2 ? "reveal-click" : "closing-click");
            if (static_backdrop) { click_response(true); }
            reveal_before(click_at + look.cursor_art.click_duration + dt);
            reveal_tick();
            snap(count == 2 ? "reveal-saved" : "closing-desktop");
            if (static_backdrop) { click_response(false); }
            require(click_sample == (static_backdrop ? 3u : 0u), "monitor isolated click comparison missed a timing edge");
        }
    }
    require(click_records.good(), "monitor click isolation record write failed");
    // GameLevel::start_match supplies OpeningMode::off for solo practice;
    // use the same policy here rather than bypassing the shell's decision.
    auto solo = build(app, builder, 1, TeamColourPair::blue_orange);
    resize(app, *solo, { 1280, 720 });
    require(!solo->revealed() && !solo->intro_playing(), "solo practice unexpectedly acquired a monitor");
    monitor_frozen(app, *solo, output, "monitor-solo-practice-no-monitor", records);
    require(records.good(), "monitor capture record write failed");
    std::cout << "PASS monitor: six shades, adult/mug and child/stickers, picture-overlap notes before preview, "
        "26 ordered untinted Muted Day square cells and final empty slot, contained picker/panel, "
        "saved slot preserves every terrain cell; native 640x480/1024x768/720p/1080p/QHD and bounded push; "
        "adult/child/mug seated/sigh/stand/leave foreground at native sizes and offset window, full departure, "
        "fixed cursor hotspot, local detached click ticks, expiry and reduced-effects suppression; "
        "real shipped full-drift 2P/4P opening/preview/countdown/push/pullback/winner reveal/no-paint close; "
        "identical static-backdrop story isolates before/at/after click accents with only decorative drift removed, "
        "solo practice without monitor, frozen redraw/persona/state/fingerprint and unchanged draw RNG\n";
}
