// Included inside painter_visual.cpp's private namespace directly above
// monitor_checks.h. Isolated person/cursor foreground rasters, plus the
// delta/crop/persona raster helpers that later check headers share.

// Isolate the two foreground drawings against the same opaque canvas. These
// checks observe their raster and public inputs, not a second copy of the art.
Frame foreground_fixture(Application& app, const Canvas& canvas, const RectangleF& screen,
    const Silhouette* person, const MonitorPersona& persona, const SilhouettePose& pose,
    const ScreenCursor* cursor = nullptr, const Vector2F& tip = {}, float click_age = -1.0f,
    bool reduced_effects = false)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(app.viewport_manager()->fullscreen_viewport());
    list.set_camera(Camera::DEFAULT_CAMERA);
    list.set_filter(TextureFilter::point);
    canvas.draw(list, RectangleF(Vector2F::ZERO, app.resolution_manager()->resolution_vec()));
    if (person != nullptr) { person->draw(list, screen, persona, pose); }
    if (cursor != nullptr) { cursor->draw(list, tip, screen.height, click_age, reduced_effects); }
    return read(renderer);
}

RectangleI foreground_delta_bounds(const Frame& frame, const Frame& blank)
{
    int left = frame.width, top = frame.height, right = 0, bottom = 0;
    for (int y = 0; y < frame.height; ++y)
    {
        for (int x = 0; x < frame.width; ++x)
        {
            const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(x)) * 4;
            if (same_pixel(frame, blank, offset)) { continue; }
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x + 1);
            bottom = std::max(bottom, y + 1);
        }
    }
    return right > left && bottom > top ? RectangleI(left, top, right - left, bottom - top) : RectangleI();
}

Frame foreground_crop(const Frame& frame, const RectangleI& bounds, int margin = 12)
{
    const int left = std::max(0, bounds.x - margin);
    const int top = std::max(0, bounds.y - margin);
    const int right = std::min(frame.width, bounds.right() + margin);
    const int bottom = std::min(frame.height, bounds.bottom() + margin);
    require(right > left && bottom > top, "foreground crop had no visible pixels");
    Frame crop{ right - left, bottom - top, {} };
    for (int y = top; y < bottom; ++y)
    {
        const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
            static_cast<size_t>(left)) * 4;
        crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
            frame.pixels.begin() + start + static_cast<size_t>(crop.width) * 4);
    }
    return crop;
}

size_t foreground_local_delta(const Frame& first, const Frame& second, const RectangleF& area)
{
    size_t changed = 0;
    for (int y = 0; y < first.height; ++y)
    {
        for (int x = 0; x < first.width; ++x)
        {
            const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(first.width) +
                static_cast<size_t>(x)) * 4;
            if (same_pixel(first, second, offset)) { continue; }
            require(static_cast<float>(x) >= area.x && static_cast<float>(y) >= area.y &&
                static_cast<float>(x) < area.right() && static_cast<float>(y) < area.bottom(),
                "foreground response changed pixels outside its local bounds");
            ++changed;
        }
    }
    return changed;
}

std::vector<float> monitor_persona_observation(const MonitorPersona& persona)
{
    std::vector<float> result{ static_cast<float>(persona.shade), persona.child ? 1.0f : 0.0f,
        persona.mug ? 1.0f : 0.0f, persona.coffee_ring.x, persona.coffee_ring.y };
    for (const auto& note : persona.sticky_notes)
    {
        result.insert(result.end(), { note.at.x, note.at.y, static_cast<float>(note.colour), note.tilt });
    }
    for (const auto& sticker : persona.stickers)
    {
        result.insert(result.end(), { sticker.at.x, sticker.at.y, static_cast<float>(sticker.colour), sticker.tilt });
    }
    for (const auto& point : persona.crack) { result.insert(result.end(), { point.x, point.y }); }
    return result;
}

void foreground_matrix(Application& app, const Canvas& canvas, const OpeningPresentation& look,
    const std::filesystem::path& output)
{
    const Silhouette person(&look, app.render_resources());
    const ScreenCursor cursor(&look, app.render_resources());
    OpeningPresentation plain_look = look;
    plain_look.silhouette_art.ink.a = 0.0f;
    plain_look.silhouette_art.rim.a = 0.0f;
    plain_look.silhouette_art.shade.a = 0.0f;
    plain_look.silhouette_art.pen_width = 0.0f;
    plain_look.silhouette_art.shade_width = 0.0f;
    const Silhouette plain_person(&plain_look, app.render_resources());
    std::ofstream records(output / "monitor-foreground.csv");
    records << "capture,width,height,persona,pose,changed_x,changed_y,changed_width,changed_height\n";
    const std::array<SilhouettePose, 4> poses{ {
        {}, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.6f } } };
    constexpr const char* pose_names[] = { "seated", "sigh", "stand", "leave" };
    constexpr const char* persona_names[] = { "adult", "child", "mug" };

    for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
    {
        app.resolution_manager()->set_resolution_exactly(size);
        std::ignore = app.renderer()->window_size_changed(size.x, size.y);
        const RectangleF screen(0, 0, static_cast<float>(size.x), static_cast<float>(size.y));
        const Frame blank = foreground_fixture(app, canvas, screen, nullptr, {}, {});
        std::vector<std::vector<unsigned char>> seated_personas;
        for (int variant = 0; variant < 3; ++variant)
        {
            MonitorPersona persona;
            persona.child = variant == 1;
            persona.mug = variant == 2;
            const auto persona_before = monitor_persona_observation(persona);
            std::vector<unsigned char> previous_pose;
            for (size_t beat = 0; beat < poses.size(); ++beat)
            {
                const SilhouettePose& pose = poses[beat];
                const SilhouettePose pose_before = pose;
                std::srand(8471);
                const int expected_random = std::rand();
                std::srand(8471);
                const Frame frame = foreground_fixture(app, canvas, screen, &person, persona, pose);
                const RectangleI bounds = foreground_delta_bounds(frame, blank);
                require(bounds.width > 0 && bounds.height > 0, "foreground person disappeared during a visible pose");
                require(frame.pixels != previous_pose, "seated/sigh/stand/leave foreground poses rendered identically");
                for (int repeat = 0; repeat < 2; ++repeat)
                {
                    require(frame.pixels == foreground_fixture(app, canvas, screen, &person, persona, pose).pixels,
                        "frozen foreground person changed pixels");
                }
                require(std::rand() == expected_random, "foreground person consumed random numbers");
                require(persona_before == monitor_persona_observation(persona), "foreground drawing changed its persona");
                require(pose.sigh == pose_before.sigh && pose.stand == pose_before.stand && pose.leave == pose_before.leave,
                    "foreground drawing changed its pose");
                if (beat == 0)
                {
                    for (const auto& previous : seated_personas)
                    {
                        require(frame.pixels != previous, "adult, child and mug foreground personas rendered identically");
                    }
                    seated_personas.push_back(frame.pixels);
                    require(frame.pixels != foreground_fixture(app, canvas, screen, &plain_person, persona, pose).pixels,
                        "foreground rim, outline and shading had no raster effect");
                }
                const std::string stem = "monitor-person-" + std::string(persona_names[variant]) + '-' +
                    pose_names[beat] + '-' + std::to_string(size.x);
                records << stem << ',' << size.x << ',' << size.y << ',' << persona_names[variant] << ',' <<
                    pose_names[beat] << ',' << bounds.x << ',' << bounds.y << ',' << bounds.width << ',' << bounds.height << '\n';
                // Retain every 720p pose, then seated/standing native detail at
                // higher sizes. The remaining rasters are checked in memory.
                if (size.x == 1280 || beat == 0 || beat == 2)
                {
                    save(foreground_crop(frame, bounds), output / (stem + ".ppm"));
                }
                previous_pose = frame.pixels;
            }
            require(blank.pixels == foreground_fixture(app, canvas, screen, &person, persona,
                { 0.0f, 1.0f, 1.0f }).pixels, "fully departed foreground person left visible pixels");
        }

        // At each native scale the selected point remains the first opaque
        // cell, while detached click ticks leave every resting arrow pixel as is.
        const Vector2F tip(static_cast<float>(size.x / 2), static_cast<float>(size.y / 3));
        const auto draw_cursor = [&](float age, bool reduced = false)
        {
            return foreground_fixture(app, canvas, screen, nullptr, {}, {}, &cursor, tip, age, reduced);
        };
        const Frame resting = draw_cursor(-1.0f);
        require(resting.pixels == draw_cursor(-1.0f).pixels, "frozen resting cursor redraw changed pixels");
        const RectangleI rest_bounds = foreground_delta_bounds(resting, blank);
        require(rest_bounds.x == static_cast<int>(tip.x) && rest_bounds.y == static_cast<int>(tip.y),
            "cursor hotspot moved away from its requested tip");
        require(!same_pixel(resting, blank,
            (static_cast<size_t>(tip.y) * static_cast<size_t>(size.x) + static_cast<size_t>(tip.x)) * 4),
            "cursor tip has no opaque native pixel");
        require(resting.pixels == draw_cursor(-0.01f).pixels &&
            resting.pixels == draw_cursor(look.cursor_art.click_duration).pixels &&
            resting.pixels == draw_cursor(look.cursor_art.click_duration + 1.0f).pixels,
            "negative or expired cursor click age did not restore the resting raster");
        Frame clicked{};
        for (const float age : { 0.0f, look.cursor_art.click_duration * 0.5f })
        {
            std::srand(6299);
            const int expected_random = std::rand();
            std::srand(6299);
            clicked = draw_cursor(age);
            require(clicked.pixels == draw_cursor(age).pixels, "frozen cursor click redraw changed pixels");
            require(std::rand() == expected_random, "cursor drawing consumed random numbers");
            require(resting.pixels == draw_cursor(age, true).pixels,
                "reduced effects did not suppress only the cursor click ticks");
            const float reach = look.cursor_size * screen.height * 2.0f;
            require(foreground_local_delta(clicked, resting,
                RectangleF(tip - Vector2F(reach), Vector2F(reach * 2.0f))) > 0,
                "active cursor click had no local raster response");
            require(pane_delta(clicked, resting, rest_bounds) == 0,
                "click response moved or covered the resting cursor arrow");
        }
        const RectangleI click_bounds = foreground_delta_bounds(clicked, blank);
        const std::string stem = "monitor-cursor-" + std::to_string(size.x);
        // Identical crop bounds make the fixed hotspot easy to review at 1:1.
        save(foreground_crop(resting, click_bounds), output / (stem + "-rest.ppm"));
        save(foreground_crop(clicked, click_bounds), output / (stem + "-click.ppm"));
    }

    // A window rectangle need not begin at the render target's origin. Keep
    // the entire small window and its continued back inside the target.
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const RectangleF origin(0, 0, 1180, 520);
    const RectangleF offset(64, 32, 1180, 520);
    MonitorPersona mug;
    mug.mug = true;
    const Frame blank = foreground_fixture(app, canvas, origin, nullptr, {}, {});
    const Frame first = foreground_fixture(app, canvas, origin, &person, mug, {});
    const Frame shifted = foreground_fixture(app, canvas, offset, &person, mug, {});
    const RectangleI first_bounds = foreground_delta_bounds(first, blank);
    const RectangleI shifted_bounds = foreground_delta_bounds(shifted, blank);
    require(shifted_bounds.x == first_bounds.x + 64 && shifted_bounds.y == first_bounds.y + 32 &&
        shifted_bounds.width == first_bounds.width && shifted_bounds.height == first_bounds.height &&
        foreground_crop(first, first_bounds, 0).pixels == foreground_crop(shifted, shifted_bounds, 0).pixels,
        "foreground person did not follow the offset window rectangle");
    save(foreground_crop(shifted, shifted_bounds), output / "monitor-person-offset-window.ppm");
    require(blank.pixels == foreground_fixture(app, canvas, offset, &person, mug,
        { 0.0f, 1.0f, 1.0f }).pixels, "fully departed person remained in the offset window");
    require(records.good(), "foreground capture record write failed");
}
