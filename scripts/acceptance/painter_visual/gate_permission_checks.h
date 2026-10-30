// ROAD#26: production permission, settlement and drawing. Controlled contacts
// and placements establish inspection states; these are not route-play trials.
Frame gate_permission_frame(Application& app, const Canvas& canvas,
    const TeamGateStructurePaintable& gate, const Camera& camera, const Viewport& pane,
    bool miniature, bool cue, size_t& reads)
{
    Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
    DrawList list = renderer.view(0); list.set_viewport(pane); list.set_camera(camera);
    list.set_filter(TextureFilter::point); canvas.draw(list, camera.visible_rectangle(pane));
    if (miniature)
    {
        if (cue) { gate.draw_miniature(list); }
        else { gate.StructurePaintable::draw_miniature(list); }
    }
    else
    {
        if (cue) { gate.draw(list); }
        else { gate.StructurePaintable::draw(list); }
    }
    ++reads; return read(renderer);
}

std::vector<unsigned char> gate_permission_ink(const Frame& frame, const Colour& ink)
{
    std::vector<unsigned char> mask;
    const auto red = static_cast<unsigned char>(std::lround(ink.r * 255.0f));
    const auto green = static_cast<unsigned char>(std::lround(ink.g * 255.0f));
    const auto blue = static_cast<unsigned char>(std::lround(ink.b * 255.0f));
    for (size_t offset = 0; offset < frame.pixels.size(); offset += 4)
    {
        // The sign's common dark ink is independent of the selected team hue.
        mask.push_back(frame.pixels[offset] == red && frame.pixels[offset + 1] == green &&
            frame.pixels[offset + 2] == blue ? 1 : 0);
    }
    return mask;
}

Frame gate_permission_monochrome(const Frame& frame, const Colour& ink)
{
    Frame result = frame;
    const auto mask = gate_permission_ink(frame, ink);
    for (size_t pixel = 0; pixel < mask.size(); ++pixel)
    {
        for (size_t channel = 0; channel < 3; ++channel)
        { result.pixels[pixel * 4 + channel] = mask[pixel] != 0 ? 0 : 255; }
    }
    return result;
}

void gate_permission_paint(Application& app, TeamGateStructurePaintable& gate,
    const GameTuning& tuning, const TeamColour& colours, PlayerTeam team, bool right_half = false)
{
    RectangleF splash = gate.bounds(); splash.inflate(24.0f);
    if (right_half)
    {
        // Collision rectangles use closed edges. Start inside the first right
        // top tile, so touching the preceding tile's far edge cannot repaint
        // that extra four-unit tile and turn the intended exact tie into a lead.
        splash.x = gate.bounds().center().x + 0.25f;
        splash.width = gate.bounds().right() + 24.0f - splash.x;
    }
    splash.offset(tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f);
    const ProjectileSpray shot(splash, Vector2F::ZERO, team, team == PlayerTeam::a ? 3 : 1,
        colours.team_colour(team), app.render_resources(),
        resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()),
        &tuning.projectile(ProjectileType::spray));
    gate.on_contact(shot, Vector2F::ZERO, 0.0f); gate.update(10.0f);
}

void gate_permission_matrix(Application& app, LevelBuilder& builder,
    Registry<LevelLoadedInfo>& levels, const Canvas& canvas, const GameTuning& tuning,
    const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const StageSelectPresentation& stage_look = presentation.main_menu().stage_select;
    const RectangleF preview_box(stage_look.preview_position, stage_look.preview_size);
    const Camera menu_camera = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION),
        app.viewport_manager()->fullscreen_viewport());
    const Viewport miniature_pane(menu_camera.calculate_view_rectangle(preview_box));
    const Camera miniature_camera = Camera::frame(levels.get("turbulence_gate_trial")->zoom_out_finish_bounds_rectangle(), miniature_pane);
    std::ofstream records(output / "gate-permission.csv");
    records << "palette,players,state,path,permission,held,paint_leader,scale,crop_x,crop_y,crop_width,crop_height,changed_pixels\n";
    constexpr std::array<const char*, 7> names{ "unpainted", "open-a", "late-b", "held-a", "open-b", "held-a-tie", "closed-tie" };
    constexpr std::array<PlayerTeam, 7> permissions{ PlayerTeam::none, PlayerTeam::a, PlayerTeam::a,
        PlayerTeam::a, PlayerTeam::b, PlayerTeam::a, PlayerTeam::none };
    constexpr std::array<bool, 7> holds{ false, false, false, true, false, true, false };
    size_t builds = 0, updates = 0, reads = 0, full_frames = 0, crops = 0, rows = 0;
    std::optional<Frame> preview_sign;
    std::optional<RectangleI> preview_crop;
    std::vector<size_t> preview_witness;
    for (int palette_index = 0; palette_index < static_cast<int>(TeamColourPair::count); ++palette_index)
    {
        const auto palette = static_cast<TeamColourPair>(palette_index);
        const std::string palette_name = TeamColourTools::palette_key(palette);
        const TeamColour colours = TeamColourTools::team_colours(palette, presentation.team_palette());
        for (const int count : { 1, 2, 4 })
        {
            if (palette_index != 0 && count != 4) { continue; }
            MatchSetup settings; settings.set_stage("turbulence_gate_trial"); settings.set_player_count(count);
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player :
                (count == 2 ? ScreenLayout::two_player : ScreenLayout::four_player));
            settings.begin_input_claims(3); constexpr std::array<int, 4> slots{ 3, 1, 2, 0 };
            for (int seat = 0; seat < count; ++seat)
            {
                if (seat != 0) { require(settings.claim_input_slot(slots[static_cast<size_t>(seat)]), "Gate native claim failed"); }
                settings.set_player_team(seat, seat % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
                settings.set_player_weapon(seat, WeaponType::sprayer);
            }
            settings.set_duration(120.0f); app.viewport_manager()->set_layout(settings.screen_layout());
            std::srand(226); auto level = builder.build_level(settings, palette, OpeningMode::off); ++builds;
            const auto inputs = neutral_inputs();
            const auto step = [&]() { level->update(acceptance::slot_seats(*level, inputs), dt); require(++updates <= 5000, "Gate native update budget exceeded"); };
            for (int tick = 0; tick < 600 && level->state() != LevelState::active; ++tick) { step(); }
            require(level->state() == LevelState::active && level->team_gates().size() == 1,
                "Gate native fixture did not reach active play with its authored gate");
            auto& gate = const_cast<TeamGateStructurePaintable&>(*level->team_gates()[0]);
            const RectangleF body = gate.bounds();
            const auto outside = [&]()
            {
                for (int seat = 0; seat < count; ++seat)
                {
                    const Player& player = level->player(seat);
                    const float x = seat % 2 == 0 ? body.left() - player.rectangle().width - 24.0f : body.right() + 24.0f;
                    require(level->place_player(seat, { x, body.bottom() - player.rectangle().height }), "Gate station placement failed");
                }
            };
            const auto inside = [&]()
            { require(level->place_player(0, { body.left() + 14.0f, body.bottom() - level->player(0).rectangle().height }), "Gate occupant placement failed"); };
            outside(); for (int tick = 0; tick < 6; ++tick) { step(); }
            // Retain the actual initial station camera so translations cannot
            // confound the state-shape comparison. Contexts use each current view.
            const Camera native_camera = level->view_camera(0);
            std::array<std::array<std::vector<unsigned char>, 7>, 2> masks;
            for (size_t state = 0; state < names.size(); ++state)
            {
                if (state == 1) { gate_permission_paint(app, gate, tuning, colours, PlayerTeam::a); step(); }
                if (state == 2) { gate_permission_paint(app, gate, tuning, colours, PlayerTeam::b); }
                if (state == 3) { inside(); step(); }
                if (state == 4) { outside(); step(); }
                if (state == 5)
                {
                    gate_permission_paint(app, gate, tuning, colours, PlayerTeam::a); step(); inside();
                    gate_permission_paint(app, gate, tuning, colours, PlayerTeam::b, true); step();
                }
                if (state == 6) { outside(); step(); }
                require(gate.open_for() == permissions[state] && gate.held_open() == holds[state],
                    "Gate native state did not follow settled permission/occupied hold");
                if (state >= 5)
                {
                    const PaintTotal paint = gate.paint_total();
                    require(std::abs(paint.team_a - paint.team_b) < 0.01f && gate.paint_leader() == PlayerTeam::none,
                        "Gate native final closure needs exactly tied paint");
                }
                const auto before = observation(*level); const PaintTotal paint = gate.paint_total();
#if COLOURWARS_DEV_TOOLS
                const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 0);
#endif
                std::srand(22626); const int expected_random = std::rand(); std::srand(22626);
                if (palette_index == 0)
                {
                    const Frame full = render(*level, *app.renderer()); ++reads;
                    require(full.pixels == render(*level, *app.renderer()).pixels, "Gate context redraw changed pixels"); ++reads;
                    save(full, output / ("gate-permission-" + std::to_string(count) + "p-" + names[state] + "-context.ppm")); ++full_frames;
                }
                for (size_t path = 0; path < 2; ++path)
                {
                    const Viewport pane = path == 0 ? app.viewport_manager()->player_viewport(0) : miniature_pane;
                    const Camera camera = path == 0 ? native_camera : miniature_camera;
                    const Frame actual = gate_permission_frame(app, canvas, gate, camera, pane, path != 0, true, reads);
                    const Frame base = gate_permission_frame(app, canvas, gate, camera, pane, path != 0, false, reads);
                    require(actual.pixels == gate_permission_frame(app, canvas, gate, camera, pane, path != 0, true, reads).pixels,
                        "Gate isolated redraw changed pixels");
                    preferences.reduce_effects = true;
                    require(actual.pixels == gate_permission_frame(app, canvas, gate, camera, pane, path != 0, true, reads).pixels,
                        "Reduced effects hid or changed gate information");
                    preferences.reduce_effects = false;
                    RectangleF projected = camera.calculate_view_rectangle(gate.permission_bounds()); projected.offset(pane.position());
                    const RectangleI crop(static_cast<int>(std::floor(projected.x)), static_cast<int>(std::floor(projected.y)),
                        static_cast<int>(std::ceil(projected.right())) - static_cast<int>(std::floor(projected.x)),
                        static_cast<int>(std::ceil(projected.bottom())) - static_cast<int>(std::floor(projected.y)));
                    require(crop.width >= 7 && crop.height >= 14, "Gate cue became too small for its native sign grammar");
                    size_t changed = 0;
                    for (int y = 0; y < actual.height; ++y)
                    {
                        for (int x = 0; x < actual.width; ++x)
                        {
                            const size_t pixel = (static_cast<size_t>(y) * actual.width + x) * 4;
                            if (same_pixel(actual, base, pixel)) { continue; }
                            ++changed;
                            require(x >= crop.x && x < crop.right() && y >= crop.y && y < crop.bottom(),
                                "Gate permission drawing escaped its plaque or obscured paintable faces");
                        }
                    }
                    require(changed > 0, "Gate cue did not change any native pixel");
                    const Frame detail = crossfeed_crop(actual, crop);
                    if (palette_index == 0 && count == 1 && state == 6 && path == 1)
                    {
                        preview_sign = detail; preview_crop = crop;
                        const Frame base_detail = crossfeed_crop(base, crop);
                        for (size_t pixel = 0; pixel < detail.pixels.size(); pixel += 4)
                        {
                            if (!same_pixel(detail, base_detail, pixel)) { preview_witness.push_back(pixel); }
                        }
                    }
                    masks[path][state] = gate_permission_ink(detail, presentation.gate_permission().ink);
                    const std::string stem = "gate-permission-" + palette_name + '-' + std::to_string(count) + "p-" + names[state] +
                        (path == 0 ? "-native" : "-miniature");
                    save(detail, output / (stem + "-detail.ppm")); ++crops;
                    if (palette_index == 0)
                    { save(gate_permission_monochrome(detail, presentation.gate_permission().ink), output / (stem + "-ink-detail.ppm")); ++crops; }
                    records << palette_name << ',' << count << ',' << names[state] << ',' << (path == 0 ? "native" : "miniature") << ',' <<
                        static_cast<int>(gate.open_for()) << ',' << gate.held_open() << ',' << static_cast<int>(gate.paint_leader()) << ',' <<
                        camera.scale << ',' << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << ',' << changed << '\n'; ++rows;
                }
                require(before == observation(*level) && paint.total == gate.paint_total().total &&
                    paint.team_a == gate.paint_total().team_a && paint.team_b == gate.paint_total().team_b &&
                    paint.player_areas == gate.paint_total().player_areas && gate.open_for() == permissions[state] &&
                    gate.held_open() == holds[state] && std::rand() == expected_random,
                    "Gate drawing changed simulation, paint credit, settled permission, hold or gameplay RNG");
#if COLOURWARS_DEV_TOOLS
                require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
                    "Gate drawing changed its Debug replay fingerprint");
#endif
            }
            for (size_t path = 0; path < 2; ++path)
            {
                require(masks[path][1] != masks[path][4] && masks[path][0] != masks[path][1] &&
                    masks[path][1] != masks[path][3], "Gate closed/A/B/occupied states cannot be distinguished without hue");
                require(masks[path][1] == masks[path][2] && masks[path][3] == masks[path][5] &&
                    masks[path][0] == masks[path][6], "Gate ink followed live paint rather than settled permission/hold");
            }
        }
    }
    // The actual Stage Select widget owns an unpainted gate and chooses the
    // production miniature path. Its reference remains an authored initial pose.
    Save preview_save; GameData data; data.set_application(&app); data.set_save(&preview_save);
    data.set_level_infos(&levels); data.set_tuning(&tuning); data.set_presentation(&presentation);
    StagePreview preview("gate-permission-preview", preview_box, &data);
    preview.set_stage("turbulence_gate_trial");
    const auto preview_frame = [&]()
    {
        Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas.draw(list, { 0, 0, 1280, 720 }); preview.draw(list); ++reads; return read(renderer);
    };
    const Frame miniature = preview_frame(); require(miniature.pixels == preview_frame().pixels,
        "Actual gate StagePreview redraw changed pixels");
    save(miniature, output / "gate-permission-stage-preview-context.ppm"); ++full_frames;
    require(preview_sign.has_value() && preview_crop.has_value() && !preview_witness.empty(),
        "Actual StagePreview lacks a nonempty isolated closed gate cue reference");
    const Frame preview_detail = crossfeed_crop(miniature, *preview_crop);
    save(preview_detail, output / "gate-permission-stage-preview-detail.ppm"); ++crops;
    // Fractional crop rounding includes adjacent paint-band pixels. Compare
    // every pixel the isolated cue actually changed, retaining paper and glyph
    // coverage while excluding the intentionally different surrounding paint.
    for (const size_t pixel : preview_witness)
    {
        require(same_pixel(preview_detail, *preview_sign, pixel),
            "Actual StagePreview omitted or changed the independently isolated closed gate cue");
    }
    records.close();
    require(records.good() && builds == 8 && full_frames == 22 && crops == 155 && rows == 112,
        "Gate native evidence inventory changed");
    std::ofstream summary(output / "gate-permission-summary.json");
    summary << "{\"outcome\":\"passed\",\"level_builds\":" << builds << ",\"level_updates\":" << updates <<
        ",\"readbacks\":" << reads << ",\"full_frames\":" << full_frames << ",\"native_crops\":" << crops <<
        ",\"rows\":" << rows << ",\"palettes\":6,\"states\":7,\"miniature_scale\":" << miniature_camera.scale << '}';
    summary.close(); require(summary.good(), "Cannot flush gate native summary");
    std::cout << "PASS gate permission native matrix: settled closed/A/B/occupied shapes, all six palettes, native1/2/4P and miniature, "
        "reduced-effects parity, plaque containment, same-state base differential, frozen paint/permission/RNG and Debug fingerprint; "
        "22 context frames/155 native crops/112 rows. Controlled setup; attended trial play remains separate.\n";
}
