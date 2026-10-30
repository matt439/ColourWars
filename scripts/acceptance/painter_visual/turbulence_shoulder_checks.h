// Included after crossfeed and turbulence_mover helpers in the private namespace.
// Controlled native descent stations; the separate CPU case owns route coverage.
std::vector<std::uint64_t> turbulence_shoulder_frozen_state(const CrossfeedFixture& fixture)
{
    auto result = turbulence_mover_owner_state(fixture);
    const auto add = [&](double value) { result.push_back(std::bit_cast<std::uint64_t>(value)); };
    for (const auto& tile : fixture.level->paint_surfaces())
    {
        for (const auto& point : tile.corners) { add(point.x); add(point.y); }
        add(static_cast<int>(tile.face)); add(static_cast<int>(tile.team)); add(tile.paint_owner);
        add(tile.scored_area); add(static_cast<double>(tile.source_index));
    }
    return result;
}

void turbulence_shoulder_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& trial_json, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    require(!tuning.player().paint_underfoot.enabled, "Shoulder clean/heavy inspection needs shipped underfoot behavior off");
    const auto read_source = [&]()
    {
        std::ifstream input(trial_json, std::ios::binary); require(input.good(), "Cannot read explicit shoulder trial");
        const std::string bytes{ std::istreambuf_iterator<char>(input), {} };
        require(!input.bad(), "Cannot read complete shoulder trial"); return bytes;
    };
    const std::string source = read_source();
    Registry<LevelLoadedInfo> trial_levels("Explicit shoulder inspection Level");
    auto loaded = std::make_unique<LevelLoadedInfo>(path_to_utf8(trial_json).c_str()); loaded->validate();
    const LevelObjectBuilder builder(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint());
    loaded->validate_content(builder, *app.render_resources(), *app.audio_resources());
    require(loaded->collision_objects_json().size() == 26 && !loaded->include_in_random() &&
        loaded->display_name() == "Turbulence Yard - Shoulder Trial" &&
        loaded->camera_bounds_rectangle() == levels.get("turbulence_blockout")->camera_bounds_rectangle(),
        "Shoulder native source differs from the qualified explicit-only composite");
    trial_levels.add("turbulence_shoulder_trial", std::move(loaded));
    const LevelLoadedInfo& info = *trial_levels.get("turbulence_shoulder_trial");
    const Vector2F body = tuning.player().size;
    const std::array<RectangleF, 2> uppers{ crossfeed_rectangle(info, "upper_approach_a"), crossfeed_rectangle(info, "upper_approach_b") };
    const std::array<RectangleF, 2> crossovers{ crossfeed_rectangle(info, "crossover_a"), crossfeed_rectangle(info, "crossover_b") };
    const std::array<RectangleF, 2> shoes{ crossfeed_rectangle(info, "shoulder_shoe_a"), crossfeed_rectangle(info, "shoulder_shoe_b") };
    const std::array<RectangleF, 2> ramps{ RectangleF(1240, 800, 160, 80), RectangleF(2600, 800, 160, 80) };
    std::array<std::array<float, 6>, 2> triangles{};
    const auto entries = info.collision_objects_json();
    for (std::size_t side = 0; side < 2; ++side)
    {
        const auto ramp = entries.at(side == 0 ? 22 : 24);
        require(ramp.string("name") == (side == 0 ? "shoulder_ramp_a" : "shoulder_ramp_b") &&
            ramp.string("type") == "StructureRamp" && ramp.string("collision_type") ==
                (side == 0 ? "STRUCTURE_RAMP_RIGHT" : "STRUCTURE_RAMP_LEFT"), "Shoulder triangle identity/order differs");
        const auto triangle = ramp.object("triangle");
        triangles[side] = { triangle.number("x1"), triangle.number("y1"), triangle.number("x2"), triangle.number("y2"),
            triangle.number("x3"), triangle.number("y3") };
        require(triangles[side] == (side == 0 ? std::array<float, 6>{ 1240, 880, 1400, 880, 1400, 800 } :
            std::array<float, 6>{ 2760, 880, 2600, 880, 2600, 800 }) && shoes[side] ==
                RectangleF(side == 0 ? 1240.0f : 2600.0f, 880, 160, 24), "Shoulder triangle/shoe geometry differs");
    }
    const std::array<RectangleF, 2> ramp_patches{ RectangleF(1240, 800 - body.y, 160, 80 + body.y),
        RectangleF(2600, 800 - body.y, 160, 80 + body.y) };
    const std::array<RectangleF, 2> flat_patches{ RectangleF(1060 - body.x / 2, 880 - body.y, body.x, body.y + 8),
        RectangleF(2940 - body.x / 2, 880 - body.y, body.x, body.y + 8) };
    require(body.x <= 160 && body.y == 120, "Shoulder native stations differ from qualified source body size");
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    std::ofstream csv(output / "turbulence-shoulder.csv"); csv.precision(std::numeric_limits<double>::max_digits10);
    csv << "capture,kind,phase,paint,players,view,input_slot,actor_role,scene_views,scene_objects,moving_owners,moving_shadows,camera_source,"
        "player_x,player_y,player_width,player_height,centre_x,feet_y,velocity_x,velocity_y,move_state,supported,carrier_id,expected_feet,"
        "triangle_x1,triangle_y1,triangle_x2,triangle_y2,triangle_x3,triangle_y3,"
        "camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,"
        "ramp_x,ramp_y,ramp_width,ramp_height,ramp_visible,flat_x,flat_y,flat_width,flat_height,flat_visible,"
        "shoe_x,shoe_y,shoe_width,shoe_height,shoe_visible,frozen_pixels,public_state,full_paint_owner_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope\n";
    require(csv.good(), "Cannot open shoulder native receipt");
    std::size_t updates = 0, placements = 0, builds = 0, frames = 0, crops = 0, rows = 0, reads = 0, heavy_contacts = 0;
    float heavy_min = 1;
    const auto supported = [](const Player& player)
    {
        const auto state = player.move_state();
        return state == PlayerMoveState::on_ground || state == PlayerMoveState::on_drop_down_ground ||
            state == PlayerMoveState::on_ramp_left || state == PlayerMoveState::on_ramp_right;
    };
    const auto save_crop = [&](const Frame& frame, const RectangleI& box, const std::string& name)
    {
        const Frame crop = crossfeed_crop(frame, box);
        for (int line = 0; line < box.height; ++line)
        {
            const auto actual = frame.pixels.begin() + (static_cast<std::size_t>(box.y + line) * frame.width + box.x) * 4;
            require(std::equal(actual, actual + static_cast<std::size_t>(box.width) * 4,
                crop.pixels.begin() + static_cast<std::size_t>(line) * box.width * 4), "Shoulder crop RGBA differs from actual pane");
        }
        save(crop, output / (name + ".ppm")); ++crops;
    };
    const auto record = [&](const std::string& name, const char* phase, bool heavy, int count, int view,
        const CrossfeedFixture* fixture, const Camera& camera, const Viewport& viewport, const RectangleI& crop, float fraction)
    {
        const bool widget = fixture == nullptr; const auto side = static_cast<std::size_t>(view % 2);
        const Player* player = widget ? nullptr : &fixture->level->player(view);
        const RectangleF shape = widget ? RectangleF::ZERO : player->rectangle();
        const auto velocity = widget ? Vector2F::ZERO : player->velocity();
        const RectangleF overview = info.zoom_out_finish_bounds_rectangle();
        const auto& ramp_patch = widget ? overview : ramp_patches[side];
        const auto& flat_patch = widget ? overview : flat_patches[side];
        const auto& shoe = widget ? overview : shoes[side];
        const auto visible = camera.visible_rectangle(viewport); const auto pane = viewport.pixel_rect();
        require(visible.contains(ramp_patch) && visible.contains(flat_patch) && visible.contains(shoe),
            "Shoulder native pane clips a whole-body ramp/flat patch or shoe underside");
        const float support_x = widget ? 0.0f : (side == 0 ? shape.right() : shape.left());
        const bool middle = !widget && view < 2 && std::string_view(phase) == "mid-ramp-stop";
        const float expected_feet = widget ? 0.0f : (middle ? (side == 0 ? 880 - (support_x - 1240) * 0.5f :
            800 + (support_x - 2600) * 0.5f) : (view < 2 && std::string_view(phase) == "before-descent" ? 800.0f : 880.0f));
        csv << name << ',' << (widget ? "actual-StagePreview-isolated" : "actual-Level-fixture") << ',' << phase << ',' <<
            (heavy ? "heavy" : "clean") << ',' << count << ',' << view + 1 << ',' << (widget ? -1 : player->player_num()) << ',' <<
            (widget ? "static-widget" : (view < 2 ? "shoulder-walker" : "static-upper-wait")) << ',' << count << ',' <<
            (widget ? 0 : fixture->structures.size()) << ',' << (widget ? 0 : fixture->level->horizontal_structure_states().size()) << ',' <<
            (widget ? 0 : fixture->moving_shadows.size()) << ',' << (widget ? "StagePreview-fit" : "actual-Scene") << ',' <<
            shape.x << ',' << shape.y << ',' << shape.width << ',' << shape.height << ',' << (widget ? 0 : player->center().x) << ',' <<
            (widget ? 0 : shape.bottom()) << ',' << velocity.x << ',' << velocity.y << ',' << (widget ? -1 : static_cast<int>(player->move_state())) << ',' <<
            (!widget && supported(*player)) << ',' << (widget ? 0 : player->carrier_id()) << ',' << expected_feet;
        for (const float coordinate : triangles[side]) { csv << ',' << (widget ? 0.0f : coordinate); }
        csv << ',' << camera.translation.x << ',' << camera.translation.y << ',' << camera.scale << ',' << pane.x << ',' << pane.y << ',' <<
            pane.width << ',' << pane.height << ',' << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << ',' << fraction << ',' <<
            ramp_patch.x << ',' << ramp_patch.y << ',' << ramp_patch.width << ',' << ramp_patch.height << ",1," <<
            flat_patch.x << ',' << flat_patch.y << ',' << flat_patch.width << ',' << flat_patch.height << ",1," <<
            shoe.x << ',' << shoe.y << ',' << shoe.width << ',' << shoe.height << ",1,1," << (widget ? "bounds-only" : "checked") << ',' <<
            (widget ? "not-applicable-widget" : "checked") << ','
#if COLOURWARS_DEV_TOOLS
            << (widget ? "not-applicable-widget" : "checked")
#else
            << "not-built"
#endif
            << ",checked,checked,controlled-native-inspection-not-human-acceptance\n"; ++rows;
    };
    const auto neutral = neutral_inputs();
    for (const int count : { 1, 2, 3, 4 })
    {
        std::array<CrossfeedFixture, 2> pair;
        std::array<std::array<RectangleF, 2>, 2> shadow_anchors{};
        std::array<std::vector<std::uint64_t>, 2> local_paint;
        for (std::size_t paint = 0; paint < 2; ++paint)
        {
            std::srand(291026); pair[paint] = crossfeed_fixture(app, info, tuning, presentation, preferences,
                count, paint != 0, 30, heavy_contacts, "turbulence_shoulder_trial"); ++builds;
            const auto owners = pair[paint].level->horizontal_structure_states();
            require(pair[paint].structures.size() == 26 && owners.size() == 2 && pair[paint].moving_shadows.size() == 2,
                "Shoulder actual Level lost composite objects, movers or owner-bound shadows");
            local_paint[paint] = turbulence_mover_paint_state(owners);
            for (std::size_t side = 0; side < 2; ++side)
            { shadow_anchors[paint][side] = pair[paint].moving_shadows[side]->bounds(); shadow_anchors[paint][side].offset(-owners[side].motion.current_offset); }
            if (paint != 0) { heavy_min = std::min(heavy_min, crossfeed_ownership(pair[paint]).second); }
        }
        bool placed = false;
        const auto tick_pair = [&](const std::vector<PlayerInputData>& input)
        {
            for (const auto& action : input) { require(!action.jump_pressed && !action.jump_held && !action.drop_through,
                "Shoulder inspection supplied a jump or one-way drop"); }
            for (std::size_t paint = 0; paint < 2; ++paint)
            {
                require(++updates <= 6000, "Shoulder native inspection exceeded its finite update budget");
                pair[paint].level->update(acceptance::slot_seats(*pair[paint].level, input), dt); const auto owners = pair[paint].level->horizontal_structure_states();
                require(turbulence_mover_paint_state(owners) == local_paint[paint], "Shoulder walking changed mover-local paint credit");
                for (std::size_t side = 0; side < 2; ++side)
                {
                    auto expected = shadow_anchors[paint][side]; expected.offset(owners[side].motion.current_offset);
                    require(pair[paint].structures[20 + side]->shape()->bounding_box() == owners[side].current_bounds &&
                        pair[paint].moving_shadows[side]->bounds() == expected, "Shoulder stepping detached actual mover/shadow owners");
                }
                if (!placed) { continue; }
                for (int view = 0; view < count; ++view)
                {
                    const Player& player = pair[paint].level->player(view);
                    require(player.state() == PlayerState::alive && player.health() > 0.99f && player.carrier_id() == 0,
                        "Shoulder native walker lost health or acquired a moving carrier");
                    for (const auto& effect : player.movement_feedback().effects())
                    { require(effect.age != 0 || effect.kind != MovementFeedbackKind::takeoff, "Shoulder walk invented a takeoff"); }
                }
            }
            if (placed)
            {
                for (int view = 0; view < count; ++view) { const auto& a = pair[0].level->player(view); const auto& b = pair[1].level->player(view);
                    require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() && a.move_state() == b.move_state(),
                        "Shoulder controlled paint changed matched actual walking"); }
            }
        };
        for (int tick = 0; tick < 600 && pair[0].level->state() != LevelState::active; ++tick) { tick_pair(neutral); }
        require(pair[0].level->state() == LevelState::active && pair[1].level->state() == LevelState::active, "Shoulder Level missed active play");
        for (int tick = 0; tick < 90; ++tick) { tick_pair(neutral); }
        for (auto& fixture : pair)
        {
            for (int view = 0; view < count; ++view)
            {
                const bool side_b = view % 2 != 0; const float centre = view < 2 ? (side_b ? 2570.0f : 1430.0f) : (side_b ? 2840.0f : 1160.0f);
                require(fixture.level->place_player(view, { centre - body.x / 2, (view < 2 ? 800.0f : 880.0f) - body.y }),
                    "Shoulder initial controlled station placement failed"); ++placements;
            }
        }
        placed = true; for (int tick = 0; tick < 3; ++tick) { tick_pair(neutral); }
        const auto capture_phase = [&](const char* phase)
        {
            std::array<Frame, 2> frames_pair;
            for (std::size_t paint = 0; paint < 2; ++paint)
            {
                auto& fixture = pair[paint];
                require(fixture.level->state() == LevelState::active && fixture.scene->view_count() == count, "Shoulder capture lost actual active views");
                const auto before = turbulence_shoulder_frozen_state(fixture); frames_pair[paint] = crossfeed_frozen(app, fixture, reads);
                require(before == turbulence_shoulder_frozen_state(fixture), "Shoulder drawing changed full paint/owner/carrier/shadow/view state");
                const auto stem = "hidden-inspection-turbulence-shoulder-" + std::to_string(count) + "p-" + (paint != 0 ? "heavy-" : "clean-") + phase + "-720p";
                save(frames_pair[paint], output / (stem + ".ppm")); ++frames;
                const float fraction = crossfeed_ownership(fixture).second;
                require(paint != 0 ? fraction > 0.99f : fraction == 0, "Shoulder held paint differs from clean/heavy label");
                for (int view = 0; view < count; ++view)
                {
                    const auto side = static_cast<std::size_t>(view % 2); const Player& player = fixture.level->player(view);
                    const auto& actual = fixture.scene->view(view); const auto& shape = player.rectangle();
                    const bool middle = view < 2 && std::string_view(phase) == "mid-ramp-stop";
                    const auto support_x = side == 0 ? shape.right() : shape.left();
                    const float expected_feet = middle ? (side == 0 ? 880 - (support_x - 1240) * 0.5f : 800 + (support_x - 2600) * 0.5f) :
                        (view < 2 && std::string_view(phase) == "before-descent" ? 800.0f : 880.0f);
                    const float goal = view >= 2 ? (side == 0 ? 1160.0f : 2840.0f) : (middle ? (side == 0 ? 1320.0f : 2680.0f) :
                        (std::string_view(phase) == "before-descent" ? (side == 0 ? 1430.0f : 2570.0f) : (side == 0 ? 1060.0f : 2940.0f)));
                    require(supported(player) && player.carrier_id() == 0 && player.velocity().x == 0 && std::abs(player.center().x - goal) <= 0.04f &&
                        std::abs(shape.bottom() - expected_feet) <= 0.06f, "Shoulder capture lost actual whole-body supported stop");
                    if (middle) { require(player.move_state() == (side == 0 ? PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left) &&
                        shape.left() >= ramps[side].left() && shape.right() <= ramps[side].right(), "Shoulder midpoint is not a whole-body ramp stop"); }
                    else { const auto& surface = view < 2 && std::string_view(phase) == "before-descent" ? crossovers[side] : uppers[side];
                        require(shape.left() >= surface.left() && shape.right() <= surface.right(), "Shoulder flat stop clips its actual support"); }
                    require(actual.camera == fixture.level->view_camera(view) && actual.viewport == Viewport(app.viewport_manager()->player_viewport(view).pixel_rect()) &&
                        actual.camera.visible_rectangle(actual.viewport).contains(shape), "Shoulder whole body escaped actual camera/pane ownership");
                    const auto crop = actual.viewport.pixel_rect(); save_crop(frames_pair[paint], crop, stem + "-view" + std::to_string(view + 1) + "-native");
                    record(stem, phase, paint != 0, count, view, &fixture, actual.camera, actual.viewport, crop, fraction);
                }
            }
            for (int view = 0; view < count; ++view) { require(pane_delta(frames_pair[0], frames_pair[1], pair[0].scene->view(view).viewport.pixel_rect()) > 0,
                "Shoulder heavy paint supplied no actual pane difference"); }
        };
        const auto drive = [&](float centre_a)
        {
            for (int tick = 0; tick < 400; ++tick)
            {
                auto actions = neutral; bool ready = true;
                for (int view = 0; view < std::min(count, 2); ++view)
                {
                    const auto& player = pair[0].level->player(view); const float goal = view == 0 ? centre_a : 4000 - centre_a;
                    const float error = goal - player.center().x, speed = player.velocity().x;
                    ready = ready && supported(player) && std::abs(error) <= 0.03f && speed == 0;
                    const float braking = speed * speed / (2 * tuning.player().x_deacceleration) + std::abs(speed) * dt + 4;
                    float axis = 0;
                    if (speed == 0 && std::abs(error) > 0.03f && std::abs(error) < 6)
                    {
                        const float cap = std::min(0.4f, tuning.player().x_deacceleration * dt / tuning.player().x_initial_velocity * 0.9f);
                        axis = std::clamp(error / (tuning.player().x_initial_velocity * dt), -cap, cap);
                    }
                    else if (std::abs(error) >= 6 && !(speed != 0 && (speed > 0) == (error > 0) && std::abs(error) <= braking))
                    { axis = error > 0 ? 1.0f : -1.0f; }
                    actions[static_cast<std::size_t>(player.player_num())].x_movement = axis;
                }
                if (ready) { return; } tick_pair(actions);
            }
            throw std::runtime_error("Shoulder actual no-jump steering missed its finite supported stop");
        };
        capture_phase("before-descent"); drive(1320); capture_phase("mid-ramp-stop"); drive(1060); capture_phase("upper-flat-stop");
    }
    {
        Save saved; SettingsSession saved_session(saved); GameData data; data.set_application(&app); data.set_save(&saved); data.set_settings_session(&saved_session); const StageCatalog stage_catalog = acceptance::catalog_of(trial_levels, {}, { "turbulence_blockout", "turbulence_shoulder_trial" }); data.set_stage_catalog(&stage_catalog);
        data.set_tuning(&tuning); data.set_presentation(&presentation); const auto& look = presentation.main_menu().stage_select;
        const RectangleF box(look.preview_position, look.preview_size); StagePreview preview("explicit-shoulder-preview", box, &data); preview.set_stage("turbulence_shoulder_trial");
        const auto fullscreen = app.viewport_manager()->fullscreen_viewport(); const auto menu = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), fullscreen);
        const auto draw_preview = [&](bool shown)
        {
            auto& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); auto list = renderer.view(0);
            list.set_viewport(fullscreen); list.set_camera(menu); list.set_filter(TextureFilter::point); canvas.draw(list, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION));
            if (shown) { preview.draw(list); } ++reads; return read(renderer);
        };
        std::srand(2911); const int expected = std::rand(); std::srand(2911); const auto frame = draw_preview(true);
        require(frame.pixels == draw_preview(true).pixels && std::rand() == expected && preview.bounds() == box, "Shoulder StagePreview changed frozen pixels, bounds or RNG");
        auto pane = menu.calculate_view_rectangle(box); pane.offset(fullscreen.position()); const Viewport viewport(pane);
        const auto camera = Camera::frame(info.zoom_out_finish_bounds_rectangle(), viewport); const auto crop = viewport.pixel_rect();
        require(pane_delta(frame, draw_preview(false), crop) > 100, "Actual shoulder StagePreview is blank");
        const std::string stem = "hidden-inspection-turbulence-shoulder-stage-preview-clean-720p"; save(frame, output / (stem + ".ppm")); ++frames;
        save_crop(frame, crop, stem + "-view1-native"); record(stem, "miniature", false, 0, 0, nullptr, camera, viewport, crop, 0);
    }
    csv.close(); require(csv.good() && read_source() == source && frames == 25 && crops == 61 && rows == 61 && reads == 51 && builds == 8 && placements == 20,
        "Shoulder native source/inventory/initial-placement count differs");
    std::ofstream summary(output / "turbulence-shoulder-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"turbulence-shoulder-only\",\"full_frames\":25,\"native_crops\":61,\"csv_rows\":61,\"readbacks\":51,\"level_builds\":8,"
        "\"initial_placements\":20,\"level_updates\":" << updates << ",\"controlled_heavy_contacts\":" << heavy_contacts << ",\"heavy_initial_fraction_min\":" << heavy_min <<
        ",\"trial_bytes_unchanged\":true,\"human_acceptance\":false}\n";
    std::ofstream scope(output / "turbulence-shoulder-scope.txt");
    scope << "Explicit isolated26-object shoulder composite; this probe edits no runtime registration. Actual Level/Scene/object/player builders and real moving-shadow owners; separate CPU evidence owns public LevelBuilder and longer route qualification.\n"
        "Hidden720p all1/2/3/4-player clean/heavy phases: fixed crossover before descent, actual no-jump whole-body ramp midpoint (centre1320/2680 within0.04; nominalfeet827, actual body-edge triangle plane), and actual upper flat stop1060/2940. One controlled placement per actor only; extra3P/4P actors wait on their upper approaches. Broad release-braking plus small analog input pulses reach supported personal-vx0 stops; no intermediate teleport, jump, respawn or cheat.\n"
        "Every actual Scene pane retains whole player, standing-body ramp warning, whole-body upper-stop patch and complete shoe underside. The original mover pair's phase/credit/shadow invariants remain represented by two actual mover owners per Level; ramp walkers retain carrier0. Controlled projectile contacts provide measured>99percent paint, not weapon efficiency. Repeated Level draws preserve full pixels, public/structure state, every paint tile's world corners/face/team/raw owner/area and complete moving-owner/carrier/shadow/view state, RNG and Debug fingerprints. Native RGBA crops equal actual source panes.\n"
        "Actual isolated StagePreview checks only initial-pose pixels/widget bounds/RNG. No ending, full human trajectory, readability, reaction time, physical pad/motor, listening, performance, balance, arena acceptance or moving/rotated scored-slope capability is established.\n";
    summary.close(); scope.close(); require(summary.good() && scope.good(), "Cannot flush shoulder native summary/scope");
    std::cout << "PASS shoulder native controlled descent:25 full frames/61 native crops/51 reads, all split layouts, actual ramp/flat stops and frozen full paint/owner state; human acceptance open\n";
}
