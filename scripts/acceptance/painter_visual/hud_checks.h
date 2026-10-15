// Included after reveal_checks.h: use the real HUD and hardware raster. The
// tests observe exact meter fractions and text contrast, not a second card art.

ScreenLayout hud_layout(int count)
{
    return count == 1 ? ScreenLayout::one_player :
        (count == 2 ? ScreenLayout::two_player :
            (count == 3 ? ScreenLayout::three_player : ScreenLayout::four_player));
}

Frame hud_fixture(Application& app, const Canvas& canvas, const InterfaceGameplay& hud,
    int count, const TeamColour& colours, float health, float paint,
    bool reduced = false, int only_view = -1)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    const int panes = count == 3 ? 4 : count;
    renderer.set_view_count(panes);
    constexpr int slots[] = { input_slots::keyboard_and_mouse, 3, 1, 0 };
    for (int view = 0; view < panes; ++view)
    {
        DrawList list = renderer.view(view);
        const Viewport viewport = app.viewport_manager()->player_viewport(view);
        list.set_viewport(viewport);
        list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(Vector2F::ZERO, viewport.size()));
        if (view >= count || only_view == -2 || (only_view >= 0 && only_view != view)) { continue; }
        const PlayerTeam team = view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b;
        hud.draw_gameplay_interface(list, viewport.size(), view, slots[view], team,
            health, paint, 123.25f, colours.team_colour(team), reduced, true);
    }
    return read(renderer);
}

size_t hud_pixel_offset(const Frame& frame, int x, int y)
{
    return (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
        static_cast<size_t>(x)) * 4;
}

float hud_luminance(const Frame& frame, size_t offset)
{
    const auto linear = [](unsigned char value)
    {
        const float channel = static_cast<float>(value) / 255.0f;
        return channel <= 0.04045f ? channel / 12.92f :
            std::pow((channel + 0.055f) / 1.055f, 2.4f);
    };
    return 0.2126f * linear(frame.pixels[offset]) +
        0.7152f * linear(frame.pixels[offset + 1]) +
        0.0722f * linear(frame.pixels[offset + 2]);
}

void hud_text_contrast(const Frame& frame, const RectangleF& area)
{
    const RectangleI box = countdown_crop_bounds(area);
    require(box.x >= 0 && box.y >= 0 && box.right() <= frame.width && box.bottom() <= frame.height,
        "HUD text bounds escaped the display");
    float darkest = 1.0f, lightest = 0.0f;
    size_t dark_pixels = 0, pale_pixels = 0;
    for (int y = box.y; y < box.bottom(); ++y)
    {
        for (int x = box.x; x < box.right(); ++x)
        {
            const float value = hud_luminance(frame, hud_pixel_offset(frame, x, y));
            darkest = std::min(darkest, value);
            lightest = std::max(lightest, value);
            if (value < 0.1f) { ++dark_pixels; }
            if (value > 0.7f) { ++pale_pixels; }
        }
    }
    require(dark_pixels > 2 && pale_pixels > 2 && (lightest + 0.05f) / (darkest + 0.05f) >= 7.0f,
        "Native HUD text lost dark ink, pale paper or raster contrast");
}

void hud_meter_raster(const Frame& empty, const Frame& half, const Frame& full,
    const std::vector<RectangleF>& tracks)
{
    std::vector<size_t> changed(tracks.size());
    for (int y = 0; y < full.height; ++y)
    {
        for (int x = 0; x < full.width; ++x)
        {
            const size_t offset = hud_pixel_offset(full, x, y);
            if (same_pixel(empty, full, offset))
            {
                require(same_pixel(empty, half, offset), "Half HUD meter altered stationary paper, ink or labels");
                continue;
            }
            bool local = false;
            for (size_t index = 0; index < tracks.size(); ++index)
            {
                const RectangleF& track = tracks[index];
                const RectangleI pixels = countdown_crop_bounds(track);
                if (x < pixels.x || x >= pixels.right() || y < pixels.y || y >= pixels.bottom()) { continue; }
                local = true;
                ++changed[index];
                // Leave one raster pixel around the fractional midpoint; on
                // both sides the independently supplied half must equal the
                // actual full or empty rendering, with no other changes.
                const float center = static_cast<float>(x) + 0.5f;
                if (center < track.center().x - 1.0f)
                {
                    require(same_pixel(half, full, offset), "Half HUD meter failed to fill its owned left fraction");
                }
                else if (center > track.center().x + 1.0f)
                {
                    require(same_pixel(half, empty, offset), "Half HUD meter filled beyond its owned fraction");
                }
                break;
            }
            require(local, "HUD ammunition or health changed pixels outside its exact meter track");
        }
    }
    for (size_t index = 0; index < tracks.size(); ++index)
    {
        const RectangleF& track = tracks[index];
        require(static_cast<float>(changed[index]) > track.width * track.height * 0.65f,
            "Full and empty HUD meters were missing or visually indistinct");
        const int row = static_cast<int>(track.center().y);
        for (int x = static_cast<int>(std::ceil(track.x + 1.0f));
            x < static_cast<int>(std::floor(track.right() - 1.0f)); ++x)
        {
            const size_t offset = hud_pixel_offset(full, x, row);
            require(!same_pixel(empty, full, offset), "HUD meter left an unfilled column at full supply");
        }
        // The keyline belongs outside the unchanged track. Observe at least
        // one dark border pixel there without assuming stepped corner pixels.
        const RectangleI around = countdown_crop_bounds(RectangleF(track.position() - Vector2F(2.0f),
            track.size() + Vector2F(4.0f)));
        size_t border = 0;
        for (int y = std::max(0, around.y); y < std::min(full.height, around.bottom()); ++y)
        {
            for (int x = std::max(0, around.x); x < std::min(full.width, around.right()); ++x)
            {
                const Vector2F center(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
                if (center.x >= track.left() && center.x < track.right() &&
                    center.y >= track.top() && center.y < track.bottom()) { continue; }
                if (hud_luminance(full, hud_pixel_offset(full, x, y)) < 0.05f) { ++border; }
            }
        }
        require(border > 2, "HUD meter lost its exterior dark keyline");
    }
}

void hud_static_matrix(Application& app, const Canvas& canvas, const Presentation& presentation,
    const std::filesystem::path& output)
{
    const HudPresentation& look = presentation.hud();
    const InterfaceGameplay hud(app.render_resources(), &look, &presentation.player_identity());
    struct Case { int count; Vector2I size; };
    const std::array cases{ Case{ 1, { 1280, 720 } }, Case{ 2, { 1280, 720 } },
        Case{ 3, { 1280, 720 } }, Case{ 4, { 1280, 720 } }, Case{ 4, { 1920, 1080 } },
        Case{ 4, { 2560, 1440 } }, Case{ 2, { 1024, 768 } }, Case{ 4, { 720, 1280 } },
        Case{ 2, { 2560, 1080 } } };
    std::ofstream records(output / "hud.csv");
    records << "capture,players,width,height,palette,view,hud_pixels\n";
    for (const Case& item : cases)
    {
        app.viewport_manager()->set_layout(hud_layout(item.count));
        app.resolution_manager()->set_resolution_exactly(item.size);
        std::ignore = app.renderer()->window_size_changed(item.size.x, item.size.y);
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            if (palette != TeamColourPair::blue_orange &&
                (item.size.x != 1280 || (item.count != 1 && item.count != 4))) { continue; }
            const TeamColour colours = TeamColourTools::team_colours(palette, presentation.team_palette());
            const auto draw = [&](float amount, bool reduced = false, int view = -1)
            {
                return hud_fixture(app, canvas, hud, item.count, colours, amount, amount, reduced, view);
            };
            const Frame blank = draw(0.0f, false, -2);
            const Frame empty = draw(0.0f);
            const Frame half = draw(0.5f);
            const Frame full = draw(1.0f);
            require(empty.pixels == draw(-0.5f).pixels && full.pixels == draw(1.5f).pixels,
                "HUD meters did not clamp negative or oversized quantities");
            require(half.pixels == draw(0.5f, true).pixels,
                "Reduce effects altered static HUD information or ordinary clock artwork");
            std::srand(49171);
            const int expected_random = std::rand();
            std::srand(49171);
            require(half.pixels == draw(0.5f).pixels && half.pixels == draw(0.5f).pixels,
                "Repeated frozen HUD drawing changed pixels");
            require(std::rand() == expected_random, "HUD drawing consumed random numbers");
            const std::string stem = "hud-" + std::to_string(item.count) + "p-" +
                std::to_string(item.size.x) + "x" + std::to_string(item.size.y) + "-" +
                TeamColourTools::palette_key(palette);
            std::vector<RectangleF> tracks;
            for (int view = 0; view < item.count; ++view)
            {
                const Viewport viewport = app.viewport_manager()->player_viewport(view);
                const RectangleI pane = viewport.pixel_rect();
                const Vector2F size = viewport.size();
                const Vector2F origin(static_cast<float>(pane.x), static_cast<float>(pane.y));
                const float scale = std::min(size.x / look.player_identity_reference_width,
                    size.y / look.reference_height);
                const auto measure = [&](const std::string& font, const std::wstring& text, float factor)
                {
                    return app.render_resources()->measure_text(
                        app.render_resources()->resolve_sprite_font(font), text) * factor;
                };
                const Vector2F label_size = measure(look.meter_label_font, L"HEALTH", scale);
                for (const float top : { look.health_top_margin, look.ammo_top_margin })
                {
                    const Vector2F position = origin + Vector2F(size.x * (1.0f - look.right_margin), size.y * top);
                    tracks.emplace_back(position, Vector2F(size.x * look.box_size.x, size.y * look.box_size.y));
                    hud_text_contrast(half, RectangleF(position -
                        Vector2F(0, label_size.y + look.meter_label_gap * scale), label_size));
                }
                const float timer_scale = scale * look.player_identity_reference_width / look.timer_reference_width;
                const Vector2F timer_size = measure(look.timer_font, L"2:04", timer_scale);
                hud_text_contrast(half, RectangleF(origin + Vector2F((size.x - timer_size.x) / 2.0f -
                    size.x * look.timer_offset.x, size.y * look.timer_offset.y), timer_size));
                const Vector2F identity_pos = origin + Vector2F(size.x * look.player_identity_offset.x,
                    size.y * look.player_identity_offset.y);
                const float badge = presentation.player_identity().enabled ?
                    (presentation.player_identity().ui_size.x + presentation.player_identity().ui_gap) * scale : 0.0f;
                hud_text_contrast(half, RectangleF(identity_pos + Vector2F(badge, 0),
                    measure(look.player_identity_font, PlayerBadge::label(view) + L"  /  TEAM A", scale)));
                const Frame local = draw(0.5f, false, view);
                const RectangleI occupied = foreground_delta_bounds(local, blank);
                require(occupied.width > 0 && occupied.height > 0 && occupied.x > pane.x && occupied.y > pane.y &&
                    occupied.right() < pane.right() && occupied.bottom() < pane.bottom(),
                    "HUD artwork reached a pane edge or escaped its owning pane");
                const size_t pixels = pane_delta(local, blank, pane);
                require(pixels > 100, "HUD artwork was absent from an active pane");
                records << stem << ',' << item.count << ',' << item.size.x << ',' << item.size.y << ',' <<
                    TeamColourTools::palette_key(palette) << ',' << view << ',' << pixels << '\n';
                if (view == 0)
                {
                    save(foreground_crop(empty, occupied), output / (stem + "-empty-detail.ppm"));
                    save(foreground_crop(half, occupied), output / (stem + "-half-detail.ppm"));
                    save(foreground_crop(full, occupied), output / (stem + "-full-detail.ppm"));
                }
            }
            hud_meter_raster(empty, half, full, tracks);
            if (item.count == 4 && item.size.x == 1280 && palette == TeamColourPair::blue_orange)
            {
                const Frame health_only = hud_fixture(app, canvas, hud, item.count, colours, 1.0f, 0.0f);
                const Frame paint_only = hud_fixture(app, canvas, hud, item.count, colours, 0.0f, 1.0f);
                for (int y = 0; y < full.height; ++y)
                {
                    for (int x = 0; x < full.width; ++x)
                    {
                        const size_t offset = hud_pixel_offset(full, x, y);
                        bool in_health = false, in_paint = false;
                        for (size_t track = 0; track < tracks.size(); ++track)
                        {
                            const RectangleI area = countdown_crop_bounds(tracks[track]);
                            if (x < area.x || x >= area.right() || y < area.y || y >= area.bottom()) { continue; }
                            (track % 2 == 0 ? in_health : in_paint) = true;
                        }
                        require(same_pixel(health_only, in_health ? full : empty, offset) &&
                            same_pixel(paint_only, in_paint ? full : empty, offset),
                            "HUD health and paint quantities crossed tracks or altered other artwork");
                    }
                }
                save(health_only, output / (stem + "-health-only-context.ppm"));
                save(paint_only, output / (stem + "-paint-only-context.ppm"));
            }
            save(half, output / (stem + "-context.ppm"));
            std::cout << "PASS " << stem << " exact meter fractions, native ink/paper contrast and pane bounds\n";
        }
    }
    require(records.good(), "HUD raster record write failed");
}

void hud_matrix(Application& app, LevelBuilder& builder, const Canvas& canvas,
    Presentation& presentation, const std::filesystem::path& output, bool check_closing = true)
{
    hud_static_matrix(app, canvas, presentation, output);
    // Re-run the actual final-ten boundary matrix with the new ordinary card.
    // Reveal cards and story timing are deliberately outside this focused run.
    if (check_closing) { closing_clock_matrix(app, canvas, presentation, output); }
    auto level = build(app, builder, 4, TeamColourPair::blue_yellow);
    resize(app, *level, { 1280, 720 });
    settle(*level, neutral_inputs());
    const auto before = observation(*level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 0);
#endif
    const Frame frozen = render(*level, *app.renderer());
    std::srand(49171);
    const int expected_random = std::rand();
    std::srand(49171);
    require(frozen.pixels == render(*level, *app.renderer()).pixels &&
        frozen.pixels == render(*level, *app.renderer()).pixels, "Frozen real-match HUD changed pixels");
    require(std::rand() == expected_random && before == observation(*level),
        "HUD rendering changed real match state or consumed draw RNG");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
        "HUD rendering changed the replay fingerprint");
#endif
    resize(app, *level, { 1920, 1080 });
    std::ignore = render(*level, *app.renderer());
    resize(app, *level, { 1280, 720 });
    require(frozen.pixels == render(*level, *app.renderer()).pixels && before == observation(*level),
        "Real-match HUD failed an exact frozen resize round-trip");
    save(frozen, output / "hud-real-4p-1280x720-context.ppm");
    std::cout << "PASS gameplay HUD: native full/half/empty and clamped meters, exact track locality and keylines, "
        "dark text/pale paper contrast, 1P/2P/3P/4P720 palettes, representative 1080p/QHD and aspect ratios, "
        "own-pane bounds, reduced/static ordinary clocks, frozen pixels/state/draw RNG and resize round-trip; "
        "existing final-ten clock gates and accents, Debug replay fingerprint\n";
}
