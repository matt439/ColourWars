// A baffle at the trial's exposed dimensions. Paint the middle of the top
// first: it must keep its first owner and receive no sweep in either mode.
void check_yard_fill(Application& app, const Canvas& canvas, const GameTuning& tuning,
    const Presentation& presentation, const PlaySettings& preferences)
{
    const RectangleF body(0, 0, 120, 80);
    const auto& look = presentation.paint_fill();
    const PaperCard paper(&presentation, app.resolution_manager(), app.render_resources());
    for (const int count : { 2, 4 })
    {
        StructurePaintable baffle("sprite_sheet_1", "square_white_4", body, std::make_unique<RectangleF>(body),
            app.render_resources(), &preferences, &tuning.paint(), CollisionObjectType::structure_paintable,
            TeamColour{}, { true, true, true, false });
        PaintFill fill(baffle, { PaintSurfaceFace::left, PaintSurfaceFace::right }, baffle, PaintSurfaceFace::top,
            tuning.paint().fill.open_share, tuning.paint().fill.cooldown);
        const auto paint = [&](const PaintSurfaceSample& tile)
        {
            const auto& spray = tuning.projectile(ProjectileType::spray);
            const Vector2F centre = (tile.corners[0] + tile.corners[2]) / 2.0f;
            const Vector2F launch = centre - Vector2F(0.25f, 0.25f) + spray.details.col_rect_size / 2.0f;
            ProjectileSpray shot(RectangleF(launch, Vector2F(0.5f, 0.5f)), Vector2F::zero, PlayerTeam::a,
                0, Colour::white, app.render_resources(), resolve_projectile_strip(spray, *app.render_resources()), &spray);
            baffle.on_contact(shot, Vector2F::zero, 0.0f);
        };
        app.viewport_manager()->set_layout(count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player);
        const auto& viewport = app.viewport_manager()->player_viewport(0);
        const float scale = gameplay_scale(presentation.camera(), static_cast<float>(viewport.pixel_rect().width), 720.0f);
        std::array<Camera, 4> cameras;
        for (int pane = 0; pane < count; ++pane)
        {
            cameras[static_cast<std::size_t>(pane)] = centred(body.center(), app.viewport_manager()->player_viewport(pane), scale);
        }
        const auto render = [&](bool reduced)
        {
            return matrix(app, canvas, cameras, [&](int, DrawList& list)
            { draw_paint_fill(list, fill, paper, look, reduced); }, count);
        };
        const Frame ready = render(false);
        for (const auto& tile : fill.target_geometry())
        {
            if (tile.corners[0].x >= 40.0f && tile.corners[2].x <= 80.0f) { paint(tile); }
        }
        for (const auto& tile : fill.trigger_geometry())
        {
            if (tile.face == PaintSurfaceFace::left) { paint(tile); }
        }
        fill.settle(dt);
        require(fill.last_team() == PlayerTeam::a && !fill.changed_tiles().empty(), "Fill cue fixture failed to activate");
        fill.settle(look.sweep_seconds * 0.15f);
        const Frame early = render(false), steady = render(true);
        fill.settle(look.sweep_seconds * 0.70f);
        const Frame late = render(false);
        require(steady.pixels == render(true).pixels, "Reduced fill cue moved across live steps");
        fill.settle(look.sweep_seconds);
        const Frame cooling = render(false);
        require(cooling.pixels == render(true).pixels, "Expired fill stroke persisted in one effects mode");
        for (int pane = 0; pane < count; ++pane)
        {
            const Camera& camera = cameras[static_cast<std::size_t>(pane)];
            const Viewport& vp = app.viewport_manager()->player_viewport(pane);
            // Actual changes over the cooled frame must fall in the appropriate
            // end of the top strip. check_bounds also requires visible pixels.
            check_bounds(early, cooling, RectangleF(0, 0, 40, 12), camera, vp);
            check_bounds(late, cooling, RectangleF(80, 0, 40, 12), camera, vp);
            check_bounds(steady, cooling, RectangleF(0, 0, 120, 12), camera, vp);
            const auto differs = [&](const Frame& a, const Frame& b, const RectangleF& world)
            {
                const auto projected = camera.calculate_view_rectangle(world);
                const auto pane_box = vp.pixel_rect();
                const RectangleI region(pane_box.x + static_cast<int>(std::ceil(projected.x)),
                    pane_box.y + static_cast<int>(std::ceil(projected.y)),
                    static_cast<int>(std::floor(projected.width)) - 1, static_cast<int>(std::floor(projected.height)) - 1);
                return crop(a, region).pixels != crop(b, region).pixels;
            };
            require(differs(steady, cooling, { 0, 0, 36, 12 }) && differs(steady, cooling, { 84, 0, 36, 12 }) &&
                !differs(steady, cooling, { 42, 0, 36, 12 }), "Stationary fill stroke missed a changed run or crossed friendly paint");
            require(differs(ready, cooling, { 12, 24, 32, 32 }) && differs(ready, cooling, { 76, 24, 32, 32 }) &&
                differs(ready, cooling, { 44, 12, 32, 32 }), "A fill source or destination failed to change shape during cooldown");
        }
        fill.settle(4.0f);
        require(ready.pixels == render(false).pixels, "Rearmed fill failed to restore all three marks");
    }
    std::cout << "PASS Yard Fill at 720p in two and four panes: three readiness marks, single advancing sweep, "
        "stationary reduced cue, disjoint changed runs and rearm\n";
}
