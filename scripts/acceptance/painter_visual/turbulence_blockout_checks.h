// Included after crossfeed_checks.h inside painter_visual.cpp's private namespace.
// Hidden native composition of the static candidate. Controlled initial actor
// placement and heavy paint are inspection fixtures, not human play evidence.
void turbulence_blockout_matrix(Application& app, Registry<LevelLoadedInfo>& levels,
    const Canvas& canvas, const GameTuning& tuning, const Presentation& presentation,
    PlaySettings& preferences, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    require(!tuning.player().paint_underfoot.enabled,
        "Turbulence native clean/heavy movement comparison requires the shipped underfoot prototype to stay off");
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const LevelLoadedInfo& info = *levels.get("turbulence_blockout");
    require(!info.include_in_random(), "Native candidate inspection must preserve explicit-only selection");
    const auto rectangle = [&](const char* name) { return crossfeed_rectangle(info, name); };
    const Vector2F body = tuning.player().size;
    const RectangleF overview = info.zoom_out_finish_bounds_rectangle();
    const auto standing = [&](const RectangleF& support, float centre)
    { return Vector2F(centre - body.x / 2.0f, support.top() - body.y); };
    const auto patch = [&](const RectangleF& support, float left, float width, float support_cap = 80.0f)
    {
        require(left >= support.left() && left + width <= support.right() && width >= body.x,
            "Turbulence warning patch must fit its authored support and a whole player");
        // Include the standing body and the visible support cap, without
        // requiring a buried foundation to fit in the same gameplay pane.
        return RectangleF(left, support.top() - body.y, width,
            body.y + std::min(support.height, support_cap));
    };
    const RectangleF ground = rectangle("ground");
    const RectangleF upper_a = rectangle("upper_approach_a"), upper_b = rectangle("upper_approach_b");
    const RectangleF cross_a = rectangle("crossover_a"), cross_b = rectangle("crossover_b");
    const RectangleF bed_a = rectangle("lower_bed_a"), bed_b = rectangle("lower_bed_b");
    const RectangleF home_a = rectangle("home_floor_a"), home_b = rectangle("home_floor_b");
    const RectangleF roof_a = rectangle("home_roof_a"), roof_b = rectangle("home_roof_b");
    const RectangleF step_a = rectangle("side_step_a"), step_b = rectangle("side_step_b");
    std::array<Vector2F, 4> positions;
    std::array<RectangleF, 4> landings, escapes;
    std::array<const char*, 4> landing_names{}, escape_names{};
    std::ofstream records(output / "turbulence-blockout.csv");
    records.precision(std::numeric_limits<float>::max_digits10);
    records << "capture,kind,phase,paint,players,view,input_slot,scene_views,camera_source,player_x,player_y,velocity_x,velocity_y,"
        "camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,"
        "owned_fraction,landing,landing_x,landing_y,landing_width,landing_height,landing_visible,"
        "escape,escape_x,escape_y,escape_width,escape_height,escape_visible,frozen_pixels,public_state,"
        "debug_fingerprint,rng,evidence_scope\n";
    require(records.good(), "Cannot open Turbulence native pane receipt");
    std::size_t readbacks = 0, updates = 0, input_steps = 0, builds = 0, heavy_contacts = 0;
    std::size_t frames = 0, crops = 0, rows = 0;
    float heavy_min = 1.0f;
    const auto record = [&](const std::string& stem, const char* kind, const char* phase, bool heavy,
        int count, int view, int scene_views, const Player* player, const Camera& camera, const Viewport& viewport,
        const RectangleI& crop, float fraction, const RectangleF& landing, const RectangleF& escape,
        const char* landing_name, const char* escape_name)
    {
        const RectangleI pane = viewport.pixel_rect();
        const Vector2F position = player != nullptr ? player->rectangle().position() : Vector2F::ZERO;
        const Vector2F velocity = player != nullptr ? player->velocity() : Vector2F::ZERO;
        const RectangleF visible = camera.visible_rectangle(viewport);
        const bool landing_visible = visible.contains(landing), escape_visible = visible.contains(escape);
        require(landing_visible && escape_visible,
            "Turbulence native pane clips its next full-body landing or first escape warning patch");
        records << stem << ',' << kind << ',' << phase << ',' << (heavy ? "heavy" : "clean") << ',' << count << ',' <<
            view + 1 << ',' << (player != nullptr ? player->player_num() : -1) << ',' << scene_views << ',' <<
            (player != nullptr ? "actual-Scene" : "StagePreview-fit") << ',' << position.x << ',' << position.y << ',' <<
            velocity.x << ',' << velocity.y << ',' << camera.translation.x << ',' << camera.translation.y << ',' << camera.scale << ',' <<
            pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << ',' << crop.x << ',' << crop.y << ',' <<
            crop.width << ',' << crop.height << ',' << fraction << ',' << landing_name << ',' << landing.x << ',' << landing.y << ',' <<
            landing.width << ',' << landing.height << ',' << landing_visible << ',' << escape_name << ',' << escape.x << ',' <<
            escape.y << ',' << escape.width << ',' << escape.height << ',' << escape_visible << ",1," <<
            (player != nullptr ? "checked" : "static-widget-bounds") << ','
#if COLOURWARS_DEV_TOOLS
            << (player != nullptr ? "checked" : "not-applicable-widget")
#else
            << "not-built"
#endif
            << ",checked,controlled-native-inspection-not-human-acceptance\n";
        ++rows;
    };
    const auto step = [&](CrossfeedFixture& fixture, const std::vector<PlayerInputData>& inputs, bool moving)
    {
        require(++updates <= 8000, "Turbulence native matrix exceeded its finite Level-update budget");
        if (moving) { ++input_steps; }
        fixture.level->update(acceptance::slot_seats(*fixture.level, inputs), dt);
    };
    const auto capture = [&](CrossfeedFixture& fixture, const std::string& stem, const char* phase, bool heavy, bool ending)
    {
        require(fixture.level->state() == (ending ? LevelState::overview : LevelState::active),
            "Turbulence capture differs from its claimed runtime phase");
        const int views = ending ? 1 : fixture.level->player_count();
        require(fixture.scene->view_count() == views, "Turbulence actual Scene view count differs from its runtime phase");
        std::vector<Scene::View> before_views;
        for (int view = 0; view < views; ++view) { before_views.push_back(fixture.scene->view(view)); }
        const Frame frame = crossfeed_frozen(app, fixture, readbacks);
        require(fixture.scene->view_count() == views, "Turbulence frozen draw changed actual Scene view count");
        save(frame, output / (stem + ".ppm")); ++frames;
        const auto ownership = crossfeed_ownership(fixture);
        require(heavy ? ownership.second > 0.99f : ownership.second == 0.0f,
            "Turbulence native clean/heavy label differs from actual held paint");
        for (int view = 0; view < views; ++view)
        {
            const auto index = static_cast<std::size_t>(view);
            const Player& player = fixture.level->player(view);
            require(player.state() == PlayerState::alive, "Turbulence inspector died unexpectedly");
            const auto& actual_view = fixture.scene->view(view);
            require(actual_view.viewport == before_views[index].viewport && actual_view.camera == before_views[index].camera,
                "Turbulence frozen draw changed actual Scene camera or viewport");
            const Viewport viewport = actual_view.viewport;
            const Camera camera = actual_view.camera;
            const Viewport expected_viewport = ending ? app.viewport_manager()->fullscreen_viewport() :
                Viewport(app.viewport_manager()->player_viewport(view).pixel_rect());
            require(viewport == expected_viewport && camera == (ending ? Camera::frame(overview, viewport) : fixture.level->view_camera(view)),
                "Turbulence actual Scene camera or viewport differs from runtime framing");
            require(ending || camera.visible_rectangle(viewport).contains(player.rectangle()),
                "Turbulence whole player body left its actual native pane");
            const RectangleI crop = ending ? crossfeed_projection(overview, camera, viewport) : viewport.pixel_rect();
            save(crossfeed_crop(frame, crop), output / (stem + "-view" + std::to_string(view + 1) + "-native.ppm")); ++crops;
            record(stem, ending ? "actual-Level-overview" : "actual-Level-fixture", phase, heavy, fixture.level->player_count(),
                view, fixture.scene->view_count(), &player, camera, viewport, crop, ownership.second,
                ending ? overview : landings[index], ending ? overview : escapes[index],
                ending ? "overview" : landing_names[index], ending ? "overview" : escape_names[index]);
        }
        return frame;
    };
    const auto neutral = neutral_inputs();
    for (int count = 1; count <= 4; ++count)
    {
        std::array<CrossfeedFixture, 2> fixtures;
        for (int paint = 0; paint < 2; ++paint)
        {
            std::srand(261026);
            fixtures[static_cast<std::size_t>(paint)] = crossfeed_fixture(app, info, tuning, presentation,
                preferences, count, paint == 1, 30.0f, heavy_contacts, "turbulence_blockout"); ++builds;
            auto& fixture = fixtures[static_cast<std::size_t>(paint)];
            if (paint == 1) { heavy_min = std::min(heavy_min, crossfeed_ownership(fixture).second); }
            for (int tick = 0; tick < 600 && fixture.level->state() != LevelState::active; ++tick) { step(fixture, neutral, false); }
            require(fixture.level->state() == LevelState::active, "Turbulence native fixture never reached active play");
            for (int tick = 0; tick < 90; ++tick) { step(fixture, neutral, false); }
        }
        for (int phase = 0; phase < 2; ++phase)
        {
            const bool upper = phase == 1;
            const char* phase_name = upper ? "upper-landing-escape" : "lower-landing-escape";
            if (upper)
            {
                positions = { standing(upper_a, 1160), standing(upper_b, 2840), standing(cross_a, 1690), standing(cross_b, 2310) };
                landings = { patch(cross_a, 1400, 260), patch(cross_b, 2340, 260),
                    patch(upper_a, 1080, 160), patch(upper_b, 2760, 160) };
                // Immediate supported reversal space, not the eventual lower
                // drop. Keep the whole standing body and an eight-unit cap
                // visible while the jumper's actual camera follows upward.
                escapes = { patch(upper_a, 920, 160, 8), patch(upper_b, 2920, 160, 8),
                    patch(cross_a, 1680, 200, 8), patch(cross_b, 2120, 200, 8) };
                landing_names = { "crossover_a_near", "crossover_b_near", "upper_approach_a_near", "upper_approach_b_near" };
                escape_names = { "upper_approach_a_return_cap8", "upper_approach_b_return_cap8",
                    "crossover_a_reversal_cap8", "crossover_b_reversal_cap8" };
            }
            else
            {
                positions = { standing(ground, 1000), standing(ground, 3000), standing(bed_a, 1450), standing(bed_b, 2550) };
                landings = { patch(bed_a, 1260, 320), patch(bed_b, 2420, 320), patch(ground, 1740, 260), patch(ground, 2000, 260) };
                escapes = { patch(ground, 840, 160), patch(ground, 3000, 160), patch(bed_a, 1260, 260), patch(bed_b, 2480, 260) };
                landing_names = { "lower_bed_a", "lower_bed_b", "centre_ground_a", "centre_ground_b" };
                escape_names = { "yard_a", "yard_b", "lower_bed_a", "lower_bed_b" };
            }
            for (auto& fixture : fixtures)
            {
                for (int view = 0; view < count; ++view)
                { require(fixture.level->place_player(view, positions[static_cast<std::size_t>(view)]), "Turbulence controlled station placement failed"); }
                for (int tick = 0; tick < 6; ++tick) { step(fixture, neutral, false); }
            }
            for (int tick = 0; tick < 18; ++tick)
            {
                auto inputs = neutral;
                for (int view = 0; view < count; ++view)
                {
                    auto& action = inputs[static_cast<std::size_t>(fixtures[0].level->player(view).player_num())];
                    const float inward = view % 2 == 0 ? 1.0f : -1.0f;
                    action.x_movement = upper && view >= 2 ? -inward : inward;
                    action.jump_pressed = upper && view < 2 && tick == 0;
                    action.jump_held = upper && view < 2;
                }
                for (auto& fixture : fixtures) { step(fixture, inputs, true); }
            }
            std::array<Frame, 2> pair;
            for (int paint = 0; paint < 2; ++paint)
            {
                const std::string stem = "hidden-inspection-turbulence-blockout-" + std::to_string(count) + "p-" +
                    (paint == 1 ? "heavy-" : "clean-") + phase_name + "-720p";
                pair[static_cast<std::size_t>(paint)] = capture(fixtures[static_cast<std::size_t>(paint)], stem, phase_name, paint == 1, false);
            }
            for (int view = 0; view < count; ++view)
            {
                const Player& clean = fixtures[0].level->player(view), &heavy = fixtures[1].level->player(view);
                require(clean.rectangle() == heavy.rectangle() && clean.velocity() == heavy.velocity(),
                    "Controlled Turbulence paint changed paired actual movement");
                require(pane_delta(pair[0], pair[1], app.viewport_manager()->player_viewport(view).pixel_rect()) > 0,
                    "Controlled heavy paint supplied no native pane difference");
            }
        }
        if (count == 4)
        {
            positions = { standing(home_a, 540), standing(home_b, 3460), standing(roof_a, 400), standing(roof_b, 3600) };
            landings = { patch(step_a, 600, 280), patch(step_b, 3120, 280), patch(step_a, 600, 280), patch(step_b, 3120, 280) };
            escapes = { patch(ground, 300, 260, 8), patch(ground, 3440, 260, 8), patch(home_a, 220, 260), patch(home_b, 3520, 260) };
            landing_names = { "side_step_a", "side_step_b", "side_step_a", "side_step_b" };
            escape_names = { "home_ground_a_cap8", "home_ground_b_cap8", "home_floor_a", "home_floor_b" };
            for (auto& fixture : fixtures)
            {
                for (int view = 0; view < count; ++view)
                { require(fixture.level->place_player(view, positions[static_cast<std::size_t>(view)]), "Turbulence home placement failed"); }
                for (int tick = 0; tick < 6; ++tick)
                {
                    auto inputs = neutral;
                    for (int view = 0; view < count; ++view)
                    { inputs[static_cast<std::size_t>(fixture.level->player(view).player_num())].x_movement = view % 2 == 0 ? 1.0f : -1.0f; }
                    step(fixture, inputs, true);
                }
            }
            std::array<Frame, 2> home_pair;
            for (int paint = 0; paint < 2; ++paint)
            {
                home_pair[static_cast<std::size_t>(paint)] = capture(fixtures[static_cast<std::size_t>(paint)], "hidden-inspection-turbulence-blockout-4p-" +
                    std::string(paint == 1 ? "heavy" : "clean") + "-home-exit-720p", "home-exit", paint == 1, false);
            }
            for (int view = 0; view < count; ++view)
            {
                const Player& clean = fixtures[0].level->player(view), &heavy = fixtures[1].level->player(view);
                require(clean.rectangle() == heavy.rectangle() && clean.velocity() == heavy.velocity(),
                    "Controlled Turbulence home paint changed paired actual movement");
                require(pane_delta(home_pair[0], home_pair[1], fixtures[0].scene->view(view).viewport.pixel_rect()) > 0,
                    "Controlled Turbulence home paint supplied no native pane difference");
            }
        }
    }
    for (int paint = 0; paint < 2; ++paint)
    {
        auto fixture = crossfeed_fixture(app, info, tuning, presentation, preferences, 1, paint == 1,
            0.75f, heavy_contacts, "turbulence_blockout"); ++builds;
        if (paint == 1) { heavy_min = std::min(heavy_min, crossfeed_ownership(fixture).second); }
        for (int tick = 0; tick < 600 && fixture.level->state() != LevelState::overview; ++tick) { step(fixture, neutral, false); }
        require(fixture.level->state() == LevelState::overview, "Turbulence short match missed the actual runtime overview");
        capture(fixture, "hidden-inspection-turbulence-blockout-overview-" + std::string(paint == 1 ? "heavy" : "clean") +
            "-720p", "overview", paint == 1, true);
    }
    {
        Save saved; GameData data; data.set_application(&app); data.set_save(&saved); data.set_level_infos(&levels);
        data.set_tuning(&tuning); data.set_presentation(&presentation);
        const auto& look = presentation.main_menu().stage_select;
        const RectangleF box(look.preview_position, look.preview_size);
        StagePreview preview("turbulence-actual-stage-preview", box, &data); preview.set_stage("turbulence_blockout");
        const Viewport fullscreen = app.viewport_manager()->fullscreen_viewport();
        const Camera menu = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), fullscreen);
        const auto draw = [&](bool shown)
        {
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); list.set_viewport(fullscreen); list.set_camera(menu); list.set_filter(TextureFilter::point);
            canvas.draw(list, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION));
            if (shown) { preview.draw(list); } ++readbacks; return read(renderer);
        };
        std::srand(2611); const int expected = std::rand(); std::srand(2611); const Frame frame = draw(true);
        require(frame.pixels == draw(true).pixels && std::rand() == expected && preview.bounds() == box,
            "Turbulence actual StagePreview changed frozen pixels, bounds or gameplay RNG");
        RectangleF pane = menu.calculate_view_rectangle(box); pane.offset(fullscreen.position());
        const Viewport viewport(pane); const Camera camera = Camera::frame(overview, viewport);
        const RectangleI crop = viewport.pixel_rect();
        require(pane_delta(frame, draw(false), crop) > 100, "Turbulence actual StagePreview is blank");
        const std::string stem = "hidden-inspection-turbulence-blockout-stage-preview-clean-720p";
        save(frame, output / (stem + ".ppm")); ++frames;
        save(crossfeed_crop(frame, crop), output / (stem + "-view1-native.ppm")); ++crops;
        record(stem, "actual-StagePreview-isolated", "miniature", false, 0, 0, 0, nullptr, camera, viewport, crop,
            0.0f, overview, overview, "overview", "overview");
    }
    records.close();
    require(frames == 21 && crops == 51 && rows == 51 && readbacks == 43 && builds == 10 && records.good(),
        "Turbulence focused native frame/crop/row/readback/build inventory differs");
    std::ofstream summary(output / "turbulence-blockout-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"turbulence-blockout-only\",\"full_frames\":21,\"native_crops\":51,"
        "\"csv_rows\":51,\"readbacks\":43,\"level_builds\":10,\"level_updates\":" << updates <<
        ",\"scripted_input_steps\":" << input_steps << ",\"controlled_heavy_contacts\":" << heavy_contacts <<
        ",\"heavy_initial_fraction_min\":" << heavy_min << ",\"human_acceptance\":false}\n";
    std::ofstream scope(output / "turbulence-blockout-scope.txt");
    scope << "Hidden native 1280x720 Turbulence static blockout composition. All 1/2/3/4-player panes have clean and controlled-heavy lower/upper moving approach pairs.\n"
        "Initial positions are deliberately placed; six neutral settle ticks precede 18 actual movement ticks. Upper first/second views hold jump; third/fourth views walk toward a possible retreat. CPU route qualification is separate.\n"
        "Each pane records a whole-body landing/support patch and immediate supported reversal space, both inside its actual Scene camera. Upper reversal and home-ground patches show the whole standing body plus an eight-world-unit support cap; upper reversal patches stay on the source shelf and do not claim visibility of an eventual lower-bed drop or an entire foundation.\n"
        "Four-player home-floor/roof departures add six walking ticks, without claiming a completed spawn departure. Actual short-match overviews and an actual clean StagePreview supplement player panes.\n"
        "Controlled ProjectileSpray contacts establish measured >99 percent held paint; these fixtures do not measure weapon painting efficiency. P3 underfoot modification stays disabled and clean/heavy actor motion must match.\n"
        "Repeated Level drawing preserves full pixels, observed public/paint/geometry state, gameplay RNG and Debug fingerprints. The isolated StagePreview checks repeated pixels, widget bounds and gameplay RNG only. Native crops are unscaled. The paper-only backdrop is provisional art.\n"
        "No visible performance capture, human readability/warning-time, balance, physical-pad, listening or arena acceptance is established.\n";
    summary.close(); scope.close();
    require(summary.good() && scope.good(), "Cannot flush Turbulence native summary/scope");
    std::cout << "PASS Turbulence native controlled composition: 21 full frames/51 native crops/43 reads, "
        "1/2/3/4 clean-heavy landing/escape panes, home exits, runtime overviews and actual StagePreview; human acceptance open\n";
}
