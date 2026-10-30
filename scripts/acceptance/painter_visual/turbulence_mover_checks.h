// Included after crossfeed_checks.h in painter_visual.cpp's private namespace.
// The trial is loaded from an explicit private path, never a shipped manifest.
// These controlled native stations supplement the separately driven CPU routes.
std::vector<std::uint64_t> turbulence_mover_paint_state(const std::vector<HorizontalStructureState>& states)
{
    std::vector<std::uint64_t> result;
    const auto add = [&](double value) { result.push_back(std::bit_cast<std::uint64_t>(value)); };
    for (const auto& owner : states)
    {
        add(owner.id); add(static_cast<double>(owner.local_paint.size()));
        for (const auto& tile : owner.local_paint)
        {
            for (const auto& corner : tile.corners) { add(corner.x); add(corner.y); }
            add(static_cast<int>(tile.face)); add(static_cast<int>(tile.team)); add(tile.paint_owner);
            add(tile.scored_area); add(static_cast<double>(tile.source_index));
        }
    }
    return result;
}

std::vector<std::uint64_t> turbulence_mover_owner_state(const CrossfeedFixture& fixture)
{
    const auto owners = fixture.level->horizontal_structure_states();
    auto result = turbulence_mover_paint_state(owners);
    const auto add = [&](double value) { result.push_back(std::bit_cast<std::uint64_t>(value)); };
    const auto box = [&](const RectangleF& value) { add(value.x); add(value.y); add(value.width); add(value.height); };
    for (const auto& owner : owners)
    {
        box(owner.motion.spec.anchor); add(owner.motion.spec.travel_x); add(owner.motion.spec.period_seconds);
        add(owner.motion.spec.initial_phase); add(owner.motion.phase); add(owner.motion.direction);
        add(owner.motion.previous_offset.x); add(owner.motion.previous_offset.y);
        add(owner.motion.current_offset.x); add(owner.motion.current_offset.y);
        box(owner.previous_bounds); box(owner.current_bounds);
    }
    for (const GameObject* shadow : fixture.moving_shadows) { box(shadow->bounds()); box(shadow->cull_bounds(2.0f)); }
    add(fixture.scene->view_count());
    for (int view = 0; view < fixture.scene->view_count(); ++view)
    {
        const auto& actual = fixture.scene->view(view);
        add(actual.camera.translation.x); add(actual.camera.translation.y); add(actual.camera.scale);
        add(actual.viewport.x); add(actual.viewport.y); add(actual.viewport.width); add(actual.viewport.height);
        add(actual.viewport.minDepth); add(actual.viewport.maxDepth); add(fixture.level->player(view).carrier_id());
    }
    return result;
}

void turbulence_mover_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& trial_json, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    require(!tuning.player().paint_underfoot.enabled, "Mover native paint/motion pairs need shipped underfoot behavior disabled");
    const auto source_bytes = [&]()
    {
        std::ifstream input(trial_json, std::ios::binary);
        require(input.good(), "Cannot read explicitly supplied Turbulence mover trial");
        const std::string bytes{ std::istreambuf_iterator<char>(input), {} };
        require(!input.bad(), "Cannot read complete Turbulence mover trial"); return bytes;
    };
    const std::string source = source_bytes();
    Registry<LevelLoadedInfo> trial_levels("Private Turbulence mover trial Level");
    auto loaded = std::make_unique<LevelLoadedInfo>(path_to_utf8(trial_json).c_str()); loaded->validate();
    const LevelObjectBuilder builder(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint());
    loaded->validate_content(builder, *app.render_resources(), *app.audio_resources());
    require(!loaded->include_in_random() && loaded->collision_objects_json().size() == 22 &&
        loaded->display_name() == "Turbulence Yard - Mover Trial" &&
        loaded->camera_bounds_rectangle() == levels.get("turbulence_blockout")->camera_bounds_rectangle(),
        "Private mover candidate differs from its qualified explicit-only trial");
    trial_levels.add("turbulence_mover_trial", std::move(loaded));
    const LevelLoadedInfo& info = *trial_levels.get("turbulence_mover_trial");
    const Vector2F body = tuning.player().size;
    const RectangleF overview = info.zoom_out_finish_bounds_rectangle();
    const std::array<RectangleF, 2> uppers{ crossfeed_rectangle(info, "upper_approach_a"), crossfeed_rectangle(info, "upper_approach_b") };
    const std::array<RectangleF, 2> landings{ RectangleF(1400, 680, 240, 200), RectangleF(2360, 680, 240, 200) };
    const std::array<RectangleF, 2> escapes{ RectangleF(1000, 760, 160, 128), RectangleF(2840, 760, 160, 128) };
    for (std::size_t side = 0; side < 2; ++side)
    {
        const RectangleF crossover = crossfeed_rectangle(info, side == 0 ? "crossover_a" : "crossover_b");
        require(landings[side].width >= body.x && landings[side].y == crossover.top() - body.y &&
            landings[side].height == body.y + 80.0f && landings[side].left() >= crossover.left() &&
            landings[side].right() <= crossover.right() && escapes[side].width >= body.x &&
            escapes[side].y == uppers[side].top() - body.y && escapes[side].height == body.y + 8.0f &&
            escapes[side].left() >= uppers[side].left() && escapes[side].right() <= uppers[side].right(),
            "Mover native whole-body landing/escape patches differ from authored static supports");
    }
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    std::ofstream records(output / "turbulence-mover.csv"); records.precision(std::numeric_limits<double>::max_digits10);
    records << "capture,kind,phase,condition,paint,players,view,input_slot,actor_role,scene_views,camera_source,"
        "player_x,player_y,velocity_x,velocity_y,carrier_id,mover_id,mover_phase,mover_direction,previous_offset,current_offset,"
        "mover_x,mover_y,mover_width,mover_height,shadow_x,shadow_y,shadow_width,shadow_height,local_owned_area,local_credited_area,"
        "camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,"
        "landing_required,landing_x,landing_y,landing_width,landing_height,landing_visible,"
        "escape_x,escape_y,escape_width,escape_height,escape_visible,frozen_pixels,public_state,motion_paint_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope\n";
    require(records.good(), "Cannot open mover native receipt");
    std::size_t updates = 0, builds = 0, readbacks = 0, heavy_contacts = 0, frames = 0, crops = 0, rows = 0;
    float heavy_min = 1.0f;
    const auto write_row = [&](const std::string& stem, const char* kind, const char* phase, const char* condition,
        bool heavy, int count, int view, const Player* player, const HorizontalStructureState* owner,
        const RectangleF& shadow, const Camera& camera, const Viewport& viewport, const RectangleI& crop,
        float fraction, bool landing_required, const RectangleF& landing, const RectangleF& escape)
    {
        const RectangleI pane = viewport.pixel_rect(); const auto visible = camera.visible_rectangle(viewport);
        const bool landing_visible = visible.contains(landing), escape_visible = visible.contains(escape);
        require(escape_visible && (!landing_required || landing_visible),
            "Mover native pane clips a required whole-body warning patch");
        const auto position = player != nullptr ? player->rectangle().position() : Vector2F::ZERO;
        const auto velocity = player != nullptr ? player->velocity() : Vector2F::ZERO;
        const RectangleF deck = owner != nullptr ? owner->current_bounds : RectangleF::ZERO;
        double owned = 0.0, credited = 0.0;
        if (owner != nullptr)
        {
            for (const auto& tile : owner->local_paint)
            {
                if (tile.team != PlayerTeam::none) { owned += tile.scored_area; if (tile.paint_owner >= 0) { credited += tile.scored_area; } }
            }
        }
        records << stem << ',' << kind << ',' << phase << ',' << condition << ',' << (heavy ? "heavy" : "clean") << ',' <<
            count << ',' << view + 1 << ',' << (player != nullptr ? player->player_num() : -1) << ',' <<
            (player == nullptr ? "static-widget" : (view < 2 ? "mover-rider" : "static-upper-wait")) << ',' << count << ',' <<
            (player != nullptr ? "actual-Scene" : "StagePreview-fit") << ',' << position.x << ',' << position.y << ',' <<
            velocity.x << ',' << velocity.y << ',' << (player != nullptr ? player->carrier_id() : 0) << ',' <<
            (owner != nullptr ? owner->id : 0) << ',' << (owner != nullptr ? owner->motion.phase : 0.0) << ',' <<
            (owner != nullptr ? owner->motion.direction : 0) << ',' << (owner != nullptr ? owner->motion.previous_offset.x : 0.0f) << ',' <<
            (owner != nullptr ? owner->motion.current_offset.x : 0.0f) << ',' << deck.x << ',' << deck.y << ',' << deck.width << ',' << deck.height << ',' <<
            shadow.x << ',' << shadow.y << ',' << shadow.width << ',' << shadow.height << ',' << owned << ',' << credited << ',' <<
            camera.translation.x << ',' << camera.translation.y << ',' << camera.scale << ',' << pane.x << ',' << pane.y << ',' <<
            pane.width << ',' << pane.height << ',' << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << ',' << fraction << ',' <<
            landing_required << ',' << landing.x << ',' << landing.y << ',' << landing.width << ',' << landing.height << ',' << landing_visible << ',' <<
            escape.x << ',' << escape.y << ',' << escape.width << ',' << escape.height << ',' << escape_visible << ",1," <<
            (player != nullptr ? "checked" : "bounds-only") << ',' << (owner != nullptr ? "checked" : "not-applicable-widget") << ','
#if COLOURWARS_DEV_TOOLS
            << (player != nullptr ? "checked" : "not-applicable-widget")
#else
            << "not-built"
#endif
            << ",checked,checked,controlled-native-inspection-not-human-acceptance\n";
        ++rows;
    };
    const auto save_crop = [&](const Frame& frame, const RectangleI& box, const std::string& stem)
    {
        const Frame crop = crossfeed_crop(frame, box);
        for (int line = 0; line < box.height; ++line)
        {
            const auto first = frame.pixels.begin() + (static_cast<std::size_t>(box.y + line) * frame.width + box.x) * 4;
            const auto copied = crop.pixels.begin() + static_cast<std::size_t>(line) * box.width * 4;
            require(std::equal(first, first + static_cast<std::size_t>(box.width) * 4, copied),
                "Mover native crop differs from its source RGBA pane");
        }
        save(crop, output / (stem + ".ppm")); ++crops;
    };
    const auto neutral = neutral_inputs();
    for (const int count : { 1, 2, 3, 4 })
    {
        std::array<CrossfeedFixture, 2> fixtures;
        std::array<std::array<RectangleF, 2>, 2> shadow_anchors{}, shadow_cull_anchors{};
        std::array<std::vector<std::uint64_t>, 2> initial_paint;
        for (std::size_t paint = 0; paint < 2; ++paint)
        {
            std::srand(271026); fixtures[paint] = crossfeed_fixture(app, info, tuning, presentation, preferences,
                count, paint != 0, 30.0f, heavy_contacts, "turbulence_mover_trial"); ++builds;
            const auto owners = fixtures[paint].level->horizontal_structure_states();
            require(owners.size() == 2 && fixtures[paint].structures.size() == 22 && fixtures[paint].moving_shadows.size() == 2,
                "Mover native fixture lost its two actual owners or owner-bound moving shadows");
            initial_paint[paint] = turbulence_mover_paint_state(owners);
            for (std::size_t side = 0; side < 2; ++side)
            {
                require(owners[side].id == side + 1 && owners[side].motion.spec.anchor ==
                    RectangleF(side == 0 ? 600.0f : 2760.0f, 720, 320, 20) && owners[side].motion.spec.travel_x == 320 &&
                    owners[side].motion.spec.period_seconds == 4 && owners[side].motion.spec.initial_phase == (side == 0 ? 0.0f : 0.5f),
                    "Mover native fixture differs from the editor-authored pair");
                shadow_anchors[paint][side] = fixtures[paint].moving_shadows[side]->bounds();
                shadow_anchors[paint][side].offset(-owners[side].motion.current_offset);
                shadow_cull_anchors[paint][side] = fixtures[paint].moving_shadows[side]->cull_bounds(2.0f);
                shadow_cull_anchors[paint][side].offset(-owners[side].motion.current_offset);
            }
            if (paint != 0) { heavy_min = std::min(heavy_min, crossfeed_ownership(fixtures[paint]).second); }
        }
        bool boarded = false;
        std::array<std::array<float, 2>, 2> local_x{};
        const auto check_fixture = [&](std::size_t paint)
        {
            const auto& fixture = fixtures[paint]; const auto owners = fixture.level->horizontal_structure_states();
            require(owners.size() == 2 && turbulence_mover_paint_state(owners) == initial_paint[paint],
                "Mover native stepping changed retained local paint/owner credit");
            for (std::size_t side = 0; side < 2; ++side)
            {
                auto expected = shadow_anchors[paint][side]; expected.offset(owners[side].motion.current_offset);
                auto expected_cull = shadow_cull_anchors[paint][side]; expected_cull.offset(owners[side].motion.current_offset);
                require(fixture.structures[20 + side]->shape()->bounding_box() == owners[side].current_bounds &&
                    fixture.moving_shadows[side]->bounds() == expected && fixture.moving_shadows[side]->cull_bounds(2.0f) == expected_cull,
                    "Actual mover collider/shadow/cull bounds lost their real owner's offset");
            }
            if (!boarded) { return; }
            for (int view = 0; view < count; ++view)
            {
                const Player& player = fixture.level->player(view); const auto side = static_cast<std::size_t>(view % 2);
                require(player.state() == PlayerState::alive && player.health() > 0.99f, "Mover native inspector lost health");
                if (view < 2)
                {
                    const auto& deck = owners[side].current_bounds;
                    require(player.carrier_id() == owners[side].id && player.velocity().x == 0 &&
                        std::abs(player.rectangle().bottom() - deck.top()) <= 0.06f &&
                        player.rectangle().left() >= deck.left() && player.rectangle().right() <= deck.right() &&
                        std::abs(player.rectangle().x - deck.x - local_x[paint][side]) <= 0.06f,
                        "Controlled native rider lost actual carrier, local standing position or personal rest");
                }
                else { require(player.carrier_id() == 0, "Static upper waiting inspector acquired a moving carrier"); }
            }
        };
        const auto tick_pair = [&]()
        {
            for (std::size_t paint = 0; paint < 2; ++paint)
            {
                require(++updates <= 6000, "Mover native matrix exceeded its finite Level-update budget");
                fixtures[paint].level->update(acceptance::slot_seats(*fixtures[paint].level, neutral), dt); check_fixture(paint);
            }
        };
        for (int tick = 0; tick < 600 && fixtures[0].level->state() != LevelState::active; ++tick) { tick_pair(); }
        require(fixtures[0].level->state() == LevelState::active && fixtures[1].level->state() == LevelState::active,
            "Mover native fixtures never reached active play");
        for (int tick = 0; tick < 90; ++tick) { tick_pair(); }
        const auto advance = [&](const auto& ready)
        {
            for (int tick = 0; tick < 250; ++tick)
            {
                if (ready(fixtures[0].level->horizontal_structure_states())) { return; }
                tick_pair();
            }
            throw std::runtime_error("Mover native fixture missed its bounded actual motion phase");
        };
        advance([](const auto& owners) { return owners[0].motion.phase >= 0.975 && owners[0].motion.phase < 0.985; });
        for (std::size_t paint = 0; paint < 2; ++paint)
        {
            const auto owners = fixtures[paint].level->horizontal_structure_states();
            for (int view = 0; view < count; ++view)
            {
                const auto side = static_cast<std::size_t>(view % 2);
                const RectangleF& support = view < 2 ? owners[side].current_bounds : uppers[side];
                const float centre = view < 2 ? support.x + (side == 0 ? 220.0f : 100.0f) : (side == 0 ? 1160.0f : 2840.0f);
                require(fixtures[paint].level->place_player(view, { centre - body.x / 2.0f, support.top() - body.y }),
                    "Mover controlled native station placement failed");
            }
        }
        for (int tick = 0; tick < 6; ++tick) { tick_pair(); }
        for (std::size_t paint = 0; paint < 2; ++paint)
        {
            const auto owners = fixtures[paint].level->horizontal_structure_states();
            for (int view = 0; view < std::min(count, 2); ++view)
            { local_x[paint][static_cast<std::size_t>(view)] = fixtures[paint].level->player(view).rectangle().x - owners[static_cast<std::size_t>(view)].current_bounds.x; }
        }
        boarded = true; check_fixture(0); check_fixture(1);
        const auto capture_phase = [&](const char* phase, const char* condition, bool landing_required)
        {
            std::array<Frame, 2> pair;
            for (std::size_t paint = 0; paint < 2; ++paint)
            {
                auto& fixture = fixtures[paint]; check_fixture(paint);
                require(fixture.level->state() == LevelState::active && fixture.scene->view_count() == count,
                    "Mover native capture lost actual active player views");
                const auto owners = fixture.level->horizontal_structure_states();
                if (landing_required)
                {
                    require(owners[0].current_bounds.right() >= 1230 && owners[1].current_bounds.left() <= 2770,
                        "Mover crossover visibility asserted outside the CPU-qualified inward departure window");
                    for (int view = 0; view < std::min(count, 2); ++view)
                    {
                        const auto side = static_cast<std::size_t>(view);
                        const auto& rider = fixture.level->player(view);
                        require(std::abs(rider.center().x - owners[side].current_bounds.x - (side == 0 ? 220.0f : 100.0f)) < 8.0f &&
                            std::abs(rider.velocity().x) < 1.0f,
                            "Mover native inward inspector differs from the CPU-qualified local departure station");
                    }
                }
                const auto before = turbulence_mover_owner_state(fixture);
                pair[paint] = crossfeed_frozen(app, fixture, readbacks);
                require(before == turbulence_mover_owner_state(fixture), "Frozen native draw changed motion/credit/carrier/shadows/views");
                const std::string stem = "hidden-inspection-turbulence-mover-" + std::to_string(count) + "p-" +
                    (paint != 0 ? "heavy-" : "clean-") + phase + "-720p";
                save(pair[paint], output / (stem + ".ppm")); ++frames;
                const float fraction = crossfeed_ownership(fixture).second;
                require(paint != 0 ? fraction > 0.99f : fraction == 0.0f, "Mover native captured paint label differs from held paint");
                for (int view = 0; view < count; ++view)
                {
                    const Player& player = fixture.level->player(view); const auto& actual = fixture.scene->view(view);
                    require(actual.camera == fixture.level->view_camera(view) && actual.viewport == Viewport(app.viewport_manager()->player_viewport(view).pixel_rect()) &&
                        actual.camera.visible_rectangle(actual.viewport).contains(player.rectangle()), "Mover native whole body/camera/pane ownership differs");
                    const auto side = static_cast<std::size_t>(view % 2); const RectangleI crop = actual.viewport.pixel_rect();
                    save_crop(pair[paint], crop, stem + "-view" + std::to_string(view + 1) + "-native");
                    write_row(stem, "actual-Level-fixture", phase, condition, paint != 0, count, view, &player, &owners[side],
                        fixture.moving_shadows[side]->bounds(), actual.camera, actual.viewport, crop, fraction,
                        landing_required, landings[side], escapes[side]);
                }
            }
            for (int view = 0; view < count; ++view)
            {
                const Player& clean = fixtures[0].level->player(view), &heavy = fixtures[1].level->player(view);
                require(clean.rectangle() == heavy.rectangle() && clean.velocity() == heavy.velocity() && clean.carrier_id() == heavy.carrier_id() &&
                    pane_delta(pair[0], pair[1], fixtures[0].scene->view(view).viewport.pixel_rect()) > 0,
                    "Mover native clean/heavy pairs lost matching movement/carriers or visible paint difference");
            }
        };
        require(fixtures[0].level->horizontal_structure_states()[0].motion.current_offset.x < 16,
            "Mover outer capture missed the near-outer waiting state");
        capture_phase("outer-wait", "wait-not-crossover-commit", false);
        advance([](const auto& owners) { return owners[0].motion.phase >= 0.49 && owners[0].motion.phase < 0.5; });
        capture_phase("inner-commit", "qualified-inward-window", true);
        advance([](const auto& owners) { return owners[0].motion.direction == -1; });
        capture_phase("inner-reversal", "carried-direction-reversal", false);
        advance([](const auto& owners) { return owners[0].motion.phase >= 0.745 && owners[0].motion.phase < 0.755; });
        capture_phase("outward-ride", "continuous-carrier-ride", false);
    }
    {
        Save saved; GameData data; data.set_application(&app); data.set_save(&saved); const StageCatalog stage_catalog = acceptance::catalog_of(trial_levels, {}, { "turbulence_blockout", "turbulence_mover_trial" }); data.set_stage_catalog(&stage_catalog);
        data.set_tuning(&tuning); data.set_presentation(&presentation);
        const auto& look = presentation.main_menu().stage_select; const RectangleF box(look.preview_position, look.preview_size);
        StagePreview preview("private-turbulence-mover-preview", box, &data); preview.set_stage("turbulence_mover_trial");
        const Viewport fullscreen = app.viewport_manager()->fullscreen_viewport();
        const Camera menu = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), fullscreen);
        const auto draw_preview = [&](bool shown)
        {
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); list.set_viewport(fullscreen); list.set_camera(menu); list.set_filter(TextureFilter::point);
            canvas.draw(list, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION)); if (shown) { preview.draw(list); }
            ++readbacks; return read(renderer);
        };
        std::srand(2711); const int expected = std::rand(); std::srand(2711); const Frame frame = draw_preview(true);
        require(frame.pixels == draw_preview(true).pixels && std::rand() == expected && preview.bounds() == box,
            "Private mover StagePreview changed repeated pixels, widget bounds or gameplay RNG");
        RectangleF pane = menu.calculate_view_rectangle(box); pane.offset(fullscreen.position());
        const Viewport viewport(pane); const Camera camera = Camera::frame(overview, viewport); const RectangleI crop = viewport.pixel_rect();
        require(pane_delta(frame, draw_preview(false), crop) > 100, "Private mover actual StagePreview is blank");
        const std::string stem = "hidden-inspection-turbulence-mover-stage-preview-clean-720p";
        save(frame, output / (stem + ".ppm")); ++frames; save_crop(frame, crop, stem + "-view1-native");
        write_row(stem, "actual-StagePreview-isolated", "miniature", "static-authored-initial-pose", false, 0, 0, nullptr,
            nullptr, RectangleF::ZERO, camera, viewport, crop, 0.0f, true, overview, overview);
    }
    records.close();
    require(source_bytes() == source && records.good() && builds == 8 && frames == 33 && crops == 81 && rows == 81 && readbacks == 67,
        "Mover native evidence changed trial bytes or exceeded its bounded inventory");
    std::ofstream summary(output / "turbulence-mover-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"turbulence-mover-only\",\"full_frames\":33,\"native_crops\":81,\"csv_rows\":81,"
        "\"readbacks\":67,\"level_builds\":8,\"level_updates\":" << updates << ",\"controlled_heavy_contacts\":" << heavy_contacts <<
        ",\"heavy_initial_fraction_min\":" << heavy_min << ",\"trial_bytes_unchanged\":true,\"human_acceptance\":false}\n";
    std::ofstream scope(output / "turbulence-mover-scope.txt");
    scope << "Explicitly supplied editor-authored trial; this probe changes no shipped registration or Random inclusion. Actual Levels/Scenes use production object/player builders, with actual owner-bound moving shadows; public LevelBuilder route/load qualification is separate CPU evidence.\n"
        "Hidden1280x720 1P/2P/3P/4P clean/heavy outer wait, inward commit, real carried inner reversal and continuing outward ride. Front riders are deliberately placed once, then neutral actual Level updates carry them continuously; extra3P/4P actors wait on static upper approaches. No driven-route completion is claimed.\n"
        "Whole player bodies and first safe upper-approach standing-body-plus8unit support caps stay in each actual Scene camera. Fixed crossover standing-body-plus80unit landing strips are required only at the separately CPU-qualified inward window (A right>=1230/B left<=2770); outer endpoint is a wait state, without a480unit-gap target-visibility requirement. Other landing-visible values are observations.\n"
        "Controlled ProjectileSpray contacts establish >99percent held paint without claiming weapon efficiency. Local face/team/raw-owner/area, actual carrier/local standing position, collider and real shadow/cull translation are checked while stepping. Repeated Level draws freeze full pixels, public observations, exact motion/local-credit/carrier/shadow/view scalars, RNG and Debug fingerprints. Every native crop has exact source RGBA parity.\n"
        "Isolated actual local-registry StagePreview checks repeated pixels, widget bounds and RNG at authored initial poses only; it owns no match or clock. Native crops are unscaled. No human/readability/reaction-time, physical-pad/motor, listening, performance, balance, slope or arena acceptance.\n";
    summary.close(); scope.close(); require(summary.good() && scope.good(), "Cannot flush mover native summary/scope");
    std::cout << "PASS Turbulence mover native inspection:33 full frames/81 native crops/67 reads across1P/2P/3P/4P, actual carry/reversal/shadows, "
        "qualified inward landing visibility, source crop parity and frozen state; human acceptance open\n";
}
