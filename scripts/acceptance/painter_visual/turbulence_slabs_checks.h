// Included after crossfeed and turbulence_mover helpers in the private namespace.
// Controlled native descent stations; the separate CPU case owns route coverage.
std::pair<float, float> turbulence_slab_ownership(const CrossfeedFixture& fixture)
{
    float total = 0, owned = 0;
    for (const auto* object : fixture.structures)
    {
        if (const auto* surface = dynamic_cast<const PaintableObject*>(object))
        { const auto paint = surface->paint_total(); total += paint.total; owned += paint.team_a + paint.team_b; }
    }
    require(total > 0, "Slab native fixture has no scored surfaces");
    return { total, owned / total };
}

std::vector<std::uint64_t> turbulence_slab_surface_state(const CrossfeedFixture& fixture)
{
    std::vector<std::uint64_t> result;
    const auto add = [&](double value) { result.push_back(std::bit_cast<std::uint64_t>(value)); };
    for (const auto* object : fixture.structures)
    {
        if (const auto* slab = dynamic_cast<const FixedSlabPaintable*>(object))
        {
            std::vector<PaintSurfaceSample> samples; slab->append_paint_surfaces(samples);
            for (const auto& sample : samples)
            {
                for (const auto& corner : sample.corners) { add(corner.x); add(corner.y); }
                add(static_cast<int>(sample.face)); add(static_cast<int>(sample.team)); add(sample.paint_owner); add(sample.scored_area);
            }
        }
    }
    return result;
}

std::vector<std::uint64_t> turbulence_slab_frozen_state(const CrossfeedFixture& fixture)
{
    auto result = turbulence_mover_owner_state(fixture);
    const auto add = [&](double value) { result.push_back(std::bit_cast<std::uint64_t>(value)); };
    const auto box = [&](const RectangleF& value) { add(value.x); add(value.y); add(value.width); add(value.height); };
    for (const auto* object : fixture.structures)
    {
        box(object->bounds()); box(object->cull_bounds(2.0f));
        if (const auto* slab = dynamic_cast<const FixedSlabPaintable*>(object))
        {
            for (const auto& point : slab->geometry().shape().points()) { add(point.x); add(point.y); }
            for (const auto& quad : slab->local_material().quads())
            { box(quad.rectangle); add(static_cast<int>(quad.role)); }
        }
    }
    for (const auto* shadow : fixture.static_shadows) { box(shadow->bounds()); box(shadow->cull_bounds(2.0f)); }
    for (const auto& tile : fixture.level->paint_surfaces())
    {
        for (const auto& point : tile.corners) { add(point.x); add(point.y); }
        add(static_cast<int>(tile.face)); add(static_cast<int>(tile.team)); add(tile.paint_owner);
        add(tile.scored_area); add(static_cast<double>(tile.source_index));
    }
    return result;
}

// The ending probe lends the actual finished Level to the real menu page.
// It does not replace the result with a synthetic LevelEndInfo or claim that
// it drove GameLevel's intervening results-menu navigation.
class SlabEndingBackdrop final : public State
{
public:
    explicit SlabEndingBackdrop(const Level& level) : level_(level) {}
    void init() override {}
    void update(float) override {}
    void draw(Renderer& renderer) const override { this->level_.draw(renderer); }
private:
    const Level& level_;
};

std::vector<float> turbulence_slab_end_state(const LevelEndInfo& result)
{
    std::vector<float> state{ result.total_area, result.team_a_area, result.team_b_area,
        static_cast<float>(result.team_a_played), static_cast<float>(result.team_b_played) };
    state.insert(state.end(), result.player_areas.begin(), result.player_areas.end());
    return state;
}

void turbulence_slab_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& trial_json, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    require(!tuning.player().paint_underfoot.enabled, "Slab clean/heavy inspection needs shipped underfoot behavior off");
    const auto read_source = [&]()
    {
        std::ifstream input(trial_json, std::ios::binary); require(input.good(), "Cannot read explicit slab trial");
        const std::string bytes{ std::istreambuf_iterator<char>(input), {} };
        require(!input.bad(), "Cannot read complete slab trial"); return bytes;
    };
    const std::string source = read_source();
    Registry<LevelLoadedInfo> trial_levels("Explicit slab inspection Level");
    auto loaded = std::make_unique<LevelLoadedInfo>(path_to_utf8(trial_json).c_str()); loaded->validate();
    const LevelObjectBuilder builder(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint());
    loaded->validate_content(builder, *app.render_resources(), *app.audio_resources());
    require(loaded->collision_objects_json().size() == 24 && !loaded->include_in_random() &&
        (loaded->display_name() == "Turbulence Yard - Mover Trial" || loaded->display_name() == "Turbulence Yard - Slab Trial") &&
        loaded->camera_bounds_rectangle() == levels.get("turbulence_blockout")->camera_bounds_rectangle(),
        "Slab native source differs from the explicit-only authored comparison");
    trial_levels.add("turbulence_slab_trial", std::move(loaded));
    const LevelLoadedInfo& info = *trial_levels.get("turbulence_slab_trial");
    const Vector2F body = tuning.player().size;
    const std::array<RectangleF, 2> uppers{ crossfeed_rectangle(info, "upper_approach_a"), crossfeed_rectangle(info, "upper_approach_b") };
    const std::array<RectangleF, 2> crossovers{ crossfeed_rectangle(info, "crossover_a"), crossfeed_rectangle(info, "crossover_b") };
    const std::array<RectangleF, 2> slopes{ RectangleF(1240, 800, 160, 80), RectangleF(2600, 800, 160, 80) };
    const std::array<FixedSlabGeometry, 2> geometry{
        FixedSlabGeometry({ { 1240, 880 }, FixedSlabRise::right }),
        FixedSlabGeometry({ { 2600, 800 }, FixedSlabRise::left }) };
    const auto entries = info.collision_objects_json();
    for (std::size_t side = 0; side < 2; ++side)
    {
        const auto object = entries.at(22 + side);
        require(object.string("name") == (side == 0 ? "fixed_slab_a" : "fixed_slab_b") &&
            object.string("type") == "FixedSlabPaintable" && object.string("collision_type") == "STRUCTURE_FIXED_SLAB" &&
            object.string("material") == "eraser", "Fixed slab identity/order differs");
        const auto spec = object.object("slab");
        require(spec.number("x") == (side == 0 ? 1240 : 2600) && spec.number("y") == (side == 0 ? 880 : 800) &&
            spec.string("rise") == (side == 0 ? "right" : "left"), "Fixed slab endpoint/rise differs");
        const auto faces = object.object("paintable_faces");
        require(faces.boolean("top") && faces.boolean("bottom") && faces.boolean("left") && faces.boolean("right"),
            "Native fixed slabs lost an exposed scoring face");
    }
    const std::array<RectangleF, 2> slope_patches{ RectangleF(1240, 800 - body.y, 160, 80 + body.y),
        RectangleF(2600, 800 - body.y, 160, 80 + body.y) };
    const std::array<RectangleF, 2> flat_patches{ RectangleF(1060 - body.x / 2, 880 - body.y, body.x, body.y + 8),
        RectangleF(2940 - body.x / 2, 880 - body.y, body.x, body.y + 8) };
    require(body.x <= 160 && body.y == 120, "Slab native stations differ from qualified source body size");
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    std::ofstream csv(output / "turbulence-slab.csv"); csv.precision(std::numeric_limits<double>::max_digits10);
    csv << "capture,kind,phase,paint,players,view,input_slot,actor_role,scene_views,scene_objects,moving_owners,moving_shadows,fixed_slabs,fixed_shadows,camera_source,"
        "player_x,player_y,player_width,player_height,centre_x,feet_y,velocity_x,velocity_y,move_state,supported,carrier_id,expected_feet,"
        "top_start_x,top_start_y,top_end_x,top_end_y,normal_thickness,slab_p0_x,slab_p0_y,slab_p1_x,slab_p1_y,slab_p2_x,slab_p2_y,slab_p3_x,slab_p3_y,"
        "camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,"
        "slope_x,slope_y,slope_width,slope_height,slope_visible,flat_x,flat_y,flat_width,flat_height,flat_visible,"
        "slab_x,slab_y,slab_width,slab_height,slab_visible,frozen_pixels,public_state,full_paint_owner_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope\n";
    require(csv.good(), "Cannot open slab native receipt");
    std::size_t updates = 0, placements = 0, builds = 0, frames = 0, crops = 0, rows = 0, reads = 0, heavy_contacts = 0;
    std::size_t wet_frames = 0, wet_contacts = 0, interior_candidates = 0, interior_checks = 0;
    std::size_t ending_frames = 0, ending_rows = 0, ending_levels = 0, ending_updates = 0;
    std::ofstream interior_csv(output / "slab-interior-pixels.csv");
    interior_csv.precision(std::numeric_limits<double>::max_digits10);
    interior_csv << "capture,x,y,world_x,world_y,local_x,local_y,interior,red,green,blue\n";
    require(interior_csv.good(), "Cannot open slab interior pixel receipt");
    std::ofstream ending_csv(output / "turbulence-slab-ending.csv");
    ending_csv.precision(std::numeric_limits<double>::max_digits10);
    ending_csv << "capture,kind,paint,players,level_state,scene_views,scene_objects,moving_owners,moving_shadows,fixed_slabs,fixed_shadows,"
        "initial_placements,level_updates,duration_seconds,menu_overlay,menu_result_source,team_a_played,team_b_played,total_area,team_a_area,team_b_area,"
        "coverage_ratio,winning_team,session_round,session_a_wins,session_b_wins,session_complete,camera_source,camera_x,camera_y,camera_scale,"
        "pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,owned_fraction,frozen_pixels,public_state,"
        "full_paint_owner_shadow_state,debug_fingerprint,rng,crop_rgba,evidence_scope\n";
    require(ending_csv.good(), "Cannot open slab ending receipt");
    float heavy_min = 1;
    const auto heavy_paint = [&](CrossfeedFixture& fixture)
    {
        const auto colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
        std::size_t source_index = 0;
        for (auto* object : fixture.structures)
        {
            if (dynamic_cast<const PaintableObject*>(object) == nullptr) { continue; }
            RectangleF splash = object->shape()->bounding_box(); splash.inflate(32);
            splash.offset(tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f);
            const auto team = source_index++ % 2 == 0 ? PlayerTeam::a : PlayerTeam::b;
            const auto& shot_tuning = tuning.projectile(ProjectileType::spray);
            const ProjectileSpray shot(splash, Vector2F::ZERO, team, team == PlayerTeam::a ? 3 : 1,
                colours.team_colour(team), app.render_resources(), resolve_projectile_strip(shot_tuning, *app.render_resources()), &shot_tuning);
            object->on_contact(shot, Vector2F::ZERO, 0); object->update(10); ++heavy_contacts;
        }
    };
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
                crop.pixels.begin() + static_cast<std::size_t>(line) * box.width * 4), "Slab crop RGBA differs from actual pane");
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
        const auto& slope_patch = widget ? overview : slope_patches[side];
        const auto& flat_patch = widget ? overview : flat_patches[side];
        const FixedSlabPaintable* slab = widget ? nullptr : dynamic_cast<const FixedSlabPaintable*>(fixture->structures[22 + side]);
        require(widget || slab != nullptr, "Actual slab source ordinal lost its concrete shape");
        const auto slab_bounds = widget ? overview : slab->geometry().bounds();
        const auto visible = camera.visible_rectangle(viewport); const auto pane = viewport.pixel_rect();
        require(visible.contains(slope_patch) && visible.contains(flat_patch) && visible.contains(slab_bounds),
            "Slab native pane clips a whole-body slope/flat patch or the solid underside");
        const float support_x = widget ? 0.0f : (side == 0 ? shape.right() : shape.left());
        const bool middle = !widget && view < 2 && (std::string_view(phase) == "mid-slab-stop" || std::string_view(phase) == "mid-slab-wet");
        const float expected_feet = widget ? 0.0f : (middle ? (side == 0 ? 880 - (support_x - 1240) * 0.5f :
            800 + (support_x - 2600) * 0.5f) : (view < 2 && std::string_view(phase) == "before-descent" ? 800.0f : 880.0f));
        csv << name << ',' << (widget ? "actual-StagePreview-isolated" : "actual-Level-fixture") << ',' << phase << ',' <<
            (heavy ? "heavy" : "clean") << ',' << count << ',' << view + 1 << ',' << (widget ? -1 : player->player_num()) << ',' <<
            (widget ? "static-widget" : (view < 2 ? "slab-walker" : "static-upper-wait")) << ',' << count << ',' <<
            (widget ? 0 : fixture->structures.size()) << ',' << (widget ? 0 : fixture->level->horizontal_structure_states().size()) << ',' <<
            (widget ? 0 : fixture->moving_shadows.size()) << ',' << (widget ? 0 : 2) << ',' << (widget ? 0 : 2) << ',' <<
            (widget ? "StagePreview-fit" : "actual-Scene") << ',' <<
            shape.x << ',' << shape.y << ',' << shape.width << ',' << shape.height << ',' << (widget ? 0 : player->center().x) << ',' <<
            (widget ? 0 : shape.bottom()) << ',' << velocity.x << ',' << velocity.y << ',' << (widget ? -1 : static_cast<int>(player->move_state())) << ',' <<
            (!widget && supported(*player)) << ',' << (widget ? 0 : player->carrier_id()) << ',' << expected_feet;
        if (widget) { for (int field = 0; field < 13; ++field) { csv << ",0"; } }
        else
        {
            const auto& actual = slab->geometry(); const auto start = actual.top_start(), end = actual.top_end();
            csv << ',' << start.x << ',' << start.y << ',' << end.x << ',' << end.y << ',' << FixedSlabGeometry::normal_thickness;
            for (const auto& point : actual.shape().points()) { csv << ',' << point.x << ',' << point.y; }
        }
        csv << ',' << camera.translation.x << ',' << camera.translation.y << ',' << camera.scale << ',' << pane.x << ',' << pane.y << ',' <<
            pane.width << ',' << pane.height << ',' << crop.x << ',' << crop.y << ',' << crop.width << ',' << crop.height << ',' << fraction << ',' <<
            slope_patch.x << ',' << slope_patch.y << ',' << slope_patch.width << ',' << slope_patch.height << ",1," <<
            flat_patch.x << ',' << flat_patch.y << ',' << flat_patch.width << ',' << flat_patch.height << ",1," <<
            slab_bounds.x << ',' << slab_bounds.y << ',' << slab_bounds.width << ',' << slab_bounds.height << ",1,1," << (widget ? "bounds-only" : "checked") << ',' <<
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
        std::array<std::vector<std::uint64_t>, 2> slab_paint;
        for (std::size_t paint = 0; paint < 2; ++paint)
        {
            std::srand(291026); pair[paint] = crossfeed_fixture(app, info, tuning, presentation, preferences,
                count, false, 30, heavy_contacts, "turbulence_slab_trial"); ++builds;
            if (paint != 0)
            {
                heavy_paint(pair[paint]);
            }
            const auto owners = pair[paint].level->horizontal_structure_states();
            const auto fixed_shadows = std::count_if(pair[paint].static_shadows.begin(), pair[paint].static_shadows.end(),
                [](const GameObject* shadow) { return dynamic_cast<const FixedSlabShadow*>(shadow) != nullptr; });
            require(pair[paint].structures.size() == 24 && owners.size() == 2 && pair[paint].moving_shadows.size() == 2 && fixed_shadows == 2,
                "Slab actual Level lost fixed/moving objects or composed shadows");
            for (std::size_t side = 0; side < 2; ++side)
            {
                const auto* slab = dynamic_cast<const FixedSlabPaintable*>(pair[paint].structures[22 + side]);
                require(slab != nullptr && slab->paint_total().total > 0 &&
                    slab->geometry().shape().points() == geometry[side].shape().points(),
                    "Actual fixed slab collider differs from source-bound corner geometry");
            }
            local_paint[paint] = turbulence_mover_paint_state(owners);
            slab_paint[paint] = turbulence_slab_surface_state(pair[paint]);
            for (std::size_t side = 0; side < 2; ++side)
            { shadow_anchors[paint][side] = pair[paint].moving_shadows[side]->bounds(); shadow_anchors[paint][side].offset(-owners[side].motion.current_offset); }
            const auto held = turbulence_slab_ownership(pair[paint]);
            require(paint != 0 ? held.second > 0.99f : held.second == 0, "Slab fixture clean/heavy all-role ownership differs");
            if (paint != 0) { heavy_min = std::min(heavy_min, held.second); }
        }
        bool placed = false;
        const auto tick_pair = [&](const std::vector<PlayerInputData>& input)
        {
            for (const auto& action : input) { require(!action.jump_pressed && !action.jump_held && !action.drop_through,
                "Slab inspection supplied a jump or one-way drop"); }
            for (std::size_t paint = 0; paint < 2; ++paint)
            {
                require(++updates <= 6000, "Slab native inspection exceeded its finite update budget");
                pair[paint].level->update(input, dt); const auto owners = pair[paint].level->horizontal_structure_states();
                require(turbulence_mover_paint_state(owners) == local_paint[paint], "Slab walking changed mover-local paint credit");
                require(turbulence_slab_surface_state(pair[paint]) == slab_paint[paint], "Slab walking changed fixed local paint/credit/corners");
                for (std::size_t side = 0; side < 2; ++side)
                {
                    auto expected = shadow_anchors[paint][side]; expected.offset(owners[side].motion.current_offset);
                    require(pair[paint].structures[20 + side]->shape()->bounding_box() == owners[side].current_bounds &&
                        pair[paint].moving_shadows[side]->bounds() == expected, "Slab stepping detached actual mover/shadow owners");
                }
                if (!placed) { continue; }
                for (int view = 0; view < count; ++view)
                {
                    const Player& player = pair[paint].level->player(view);
                    require(player.state() == PlayerState::alive && player.health() > 0.99f && player.carrier_id() == 0,
                        "Slab native walker lost health or acquired a moving carrier");
                    for (const auto& effect : player.movement_feedback().effects())
                    { require(effect.age != 0 || effect.kind != MovementFeedbackKind::takeoff, "Slab walk invented a takeoff"); }
                }
            }
            if (placed)
            {
                for (int view = 0; view < count; ++view) { const auto& a = pair[0].level->player(view); const auto& b = pair[1].level->player(view);
                    require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() && a.move_state() == b.move_state(),
                        "Slab controlled paint changed matched actual walking"); }
            }
        };
        for (int tick = 0; tick < 600 && pair[0].level->state() != LevelState::active; ++tick) { tick_pair(neutral); }
        require(pair[0].level->state() == LevelState::active && pair[1].level->state() == LevelState::active, "Slab Level missed active play");
        for (int tick = 0; tick < 90; ++tick) { tick_pair(neutral); }
        for (auto& fixture : pair)
        {
            for (int view = 0; view < count; ++view)
            {
                const bool side_b = view % 2 != 0; const float centre = view < 2 ? (side_b ? 2570.0f : 1430.0f) : (side_b ? 2840.0f : 1160.0f);
                require(fixture.level->place_player(view, { centre - body.x / 2, (view < 2 ? 800.0f : 880.0f) - body.y }),
                    "Slab initial controlled station placement failed"); ++placements;
            }
        }
        placed = true; for (int tick = 0; tick < 3; ++tick) { tick_pair(neutral); }
        const auto capture_phase = [&](const char* phase, bool wet = false)
        {
            std::array<Frame, 2> frames_pair;
            for (std::size_t paint = wet ? 1 : 0; paint < 2; ++paint)
            {
                auto& fixture = pair[paint];
                require(fixture.level->state() == LevelState::active && fixture.scene->view_count() == count, "Slab capture lost actual active views");
                const auto before = turbulence_slab_frozen_state(fixture); frames_pair[paint] = crossfeed_frozen(app, fixture, reads);
                require(before == turbulence_slab_frozen_state(fixture), "Slab drawing changed full paint/owner/carrier/shadow/view state");
                const auto stem = "hidden-inspection-turbulence-slab-" + std::to_string(count) + "p-" + (paint != 0 ? "heavy-" : "clean-") + phase + "-720p";
                save(frames_pair[paint], output / (stem + ".ppm")); ++frames;
                if (wet) { ++wet_frames; }
                const float fraction = turbulence_slab_ownership(fixture).second;
                require(paint != 0 ? fraction > 0.99f : fraction == 0, "Slab held paint differs from clean/heavy label");
                for (int view = 0; view < count; ++view)
                {
                    const auto side = static_cast<std::size_t>(view % 2); const Player& player = fixture.level->player(view);
                    const auto& actual = fixture.scene->view(view); const auto& shape = player.rectangle();
                    const bool middle = view < 2 && (std::string_view(phase) == "mid-slab-stop" || std::string_view(phase) == "mid-slab-wet");
                    const auto support_x = side == 0 ? shape.right() : shape.left();
                    const float expected_feet = middle ? (side == 0 ? 880 - (support_x - 1240) * 0.5f : 800 + (support_x - 2600) * 0.5f) :
                        (view < 2 && std::string_view(phase) == "before-descent" ? 800.0f : 880.0f);
                    const float goal = view >= 2 ? (side == 0 ? 1160.0f : 2840.0f) : (middle ? (side == 0 ? 1320.0f : 2680.0f) :
                        (std::string_view(phase) == "before-descent" ? (side == 0 ? 1430.0f : 2570.0f) : (side == 0 ? 1060.0f : 2940.0f)));
                    require(supported(player) && player.carrier_id() == 0 && player.velocity().x == 0 && std::abs(player.center().x - goal) <= 0.04f &&
                        std::abs(shape.bottom() - expected_feet) <= 0.06f, "Slab capture lost actual whole-body supported stop");
                    if (middle) { require(player.move_state() == (side == 0 ? PlayerMoveState::on_ramp_right : PlayerMoveState::on_ramp_left) &&
                        shape.left() >= slopes[side].left() && shape.right() <= slopes[side].right(), "Slab midpoint is not a whole-body slab stop"); }
                    else { const auto& surface = view < 2 && std::string_view(phase) == "before-descent" ? crossovers[side] : uppers[side];
                        require(shape.left() >= surface.left() && shape.right() <= surface.right(), "Slab flat stop clips its actual support"); }
                    require(actual.camera == fixture.level->view_camera(view) && actual.viewport == Viewport(app.viewport_manager()->player_viewport(view).pixel_rect()) &&
                        actual.camera.visible_rectangle(actual.viewport).contains(shape), "Slab whole body escaped actual camera/pane ownership");
                    const auto crop = actual.viewport.pixel_rect(); save_crop(frames_pair[paint], crop, stem + "-view" + std::to_string(view + 1) + "-native");
                    record(stem, phase, paint != 0, count, view, &fixture, actual.camera, actual.viewport, crop, fraction);
                }
                if (count == 1 && paint == 1 && !wet && std::string_view(phase) == "mid-slab-stop")
                {
                    // These are the retained v1 white-pinhole locations. A
                    // source-derived geometric predicate excludes the first,
                    // which lies just outside the actual top edge. Six true
                    // interior top/underside points must have owned blue or
                    // the continuous black inward edge, never white primer.
                    constexpr std::array<Vector2I, 7> points{ Vector2I(704, 334), Vector2I(712, 334), Vector2I(713, 336),
                        Vector2I(722, 336), Vector2I(700, 340), Vector2I(710, 342), Vector2I(705, 344) };
                    const auto& actual = fixture.scene->view(0);
                    const double length = std::sqrt(160.0 * 160.0 + 80.0 * 80.0);
                    for (std::size_t index = 0; index < points.size(); ++index)
                    {
                        const auto pixel = points[index];
                        const Vector2F world = actual.camera.world_position(pixel, actual.viewport) +
                            Vector2F(0.5f / actual.camera.scale, 0.5f / actual.camera.scale);
                        const double dx = static_cast<double>(world.x) - 1240.0, dy = static_cast<double>(world.y) - 880.0;
                        const double along = (dx * 160.0 - dy * 80.0) / length;
                        const double across = (dx * 80.0 + dy * 160.0) / length;
                        const bool interior = along > 24.0 && along < length - 24.0 && across > 0.0 && across < 24.0;
                        require(interior == (index != 0), "Known slab pixel candidates changed their source-derived interior coverage");
                        const std::size_t offset = (static_cast<std::size_t>(pixel.y) * frames_pair[paint].width + pixel.x) * 4;
                        const int red = frames_pair[paint].pixels[offset], green = frames_pair[paint].pixels[offset + 1], blue = frames_pair[paint].pixels[offset + 2];
                        interior_csv << stem << ',' << pixel.x << ',' << pixel.y << ',' << world.x << ',' << world.y << ',' <<
                            along << ',' << across << ',' << interior << ',' << red << ',' << green << ',' << blue << '\n'; ++interior_candidates;
                        if (interior)
                        {
                            require((red == 6 && green == 52 && blue == 204) || (red == 0 && green == 0 && blue == 0),
                                "Owned slab interior still exposes unpainted primer at a retained pinhole location");
                            ++interior_checks;
                        }
                    }
                }
            }
            for (int view = 0; !wet && view < count; ++view) { require(pane_delta(frames_pair[0], frames_pair[1], pair[0].scene->view(view).viewport.pixel_rect()) > 0,
                "Slab heavy paint supplied no actual pane difference"); }
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
            throw std::runtime_error("Slab actual no-jump steering missed its finite supported stop");
        };
        capture_phase("before-descent"); drive(1320); capture_phase("mid-slab-stop");
        {
            auto& fixture = pair[1]; const auto before_paint = turbulence_slab_surface_state(fixture);
            const auto before_players = observation(*fixture.level);
            const auto colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
            const auto& shot_tuning = tuning.projectile(ProjectileType::spray);
            for (std::size_t side = 0; side < 2; ++side)
            {
                auto* slab = dynamic_cast<FixedSlabPaintable*>(fixture.structures[22 + side]);
                require(slab != nullptr, "Wet slab contact lost its actual owner");
                RectangleF splash = slab->bounds(); splash.inflate(32);
                splash.offset(shot_tuning.details.col_rect_size / 2.0f);
                const auto team = side == 0 ? PlayerTeam::a : PlayerTeam::b;
                const ProjectileSpray shot(splash, Vector2F::ZERO, team, side == 0 ? 3 : 1, colours.team_colour(team),
                    app.render_resources(), resolve_projectile_strip(shot_tuning, *app.render_resources()), &shot_tuning);
                slab->on_contact(shot, Vector2F::ZERO, 0); ++heavy_contacts; ++wet_contacts;
            }
            require(before_paint == turbulence_slab_surface_state(fixture) && before_players == observation(*fixture.level),
                "Friendly wet repaint changed actual slab ownership/score or personal poses");
        }
        capture_phase("mid-slab-wet", true);
        const auto after_wet = updates; drive(1060);
        const auto clip = app.render_resources()->resolve_sheet_strip(tuning.paint().splash_sheet,
            tuning.paint().splash_animations[static_cast<std::size_t>(ProjectileType::spray)]);
        const auto& strip = app.render_resources()->sprite_sheet(clip.sheet())->animation_strip(clip.strip());
        require(static_cast<float>((updates - after_wet) / 2) * dt >= std::max(tuning.paint().wet_duration,
            strip.frame_time() * static_cast<float>(strip.frame_count())), "Upper stop was captured before the actual wet walk settled");
        capture_phase("upper-flat-stop");
    }
    {
        Save saved; GameData data; data.set_application(&app); data.set_save(&saved); data.set_level_infos(&trial_levels);
        data.set_tuning(&tuning); data.set_presentation(&presentation); const auto& look = presentation.main_menu().stage_select;
        const RectangleF box(look.preview_position, look.preview_size); StagePreview preview("explicit-slab-preview", box, &data); preview.set_stage("turbulence_slab_trial");
        const auto fullscreen = app.viewport_manager()->fullscreen_viewport(); const auto menu = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), fullscreen);
        const auto draw_preview = [&](bool shown)
        {
            auto& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); auto list = renderer.view(0);
            list.set_viewport(fullscreen); list.set_camera(menu); list.set_filter(TextureFilter::point); canvas.draw(list, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION));
            if (shown) { preview.draw(list); } ++reads; return read(renderer);
        };
        std::srand(2911); const int expected = std::rand(); std::srand(2911); const auto frame = draw_preview(true);
        require(frame.pixels == draw_preview(true).pixels && std::rand() == expected && preview.bounds() == box, "Slab StagePreview changed frozen pixels, bounds or RNG");
        auto pane = menu.calculate_view_rectangle(box); pane.offset(fullscreen.position()); const Viewport viewport(pane);
        const auto camera = Camera::frame(info.zoom_out_finish_bounds_rectangle(), viewport); const auto crop = viewport.pixel_rect();
        require(pane_delta(frame, draw_preview(false), crop) > 100, "Actual slab StagePreview is blank");
        const std::string stem = "hidden-inspection-turbulence-slab-stage-preview-clean-720p"; save(frame, output / (stem + ".ppm")); ++frames;
        save_crop(frame, crop, stem + "-view1-native"); record(stem, "miniature", false, 0, 0, nullptr, camera, viewport, crop, 0);
    }
    for (const int count : { 1, 4 })
    {
        const bool heavy = count == 4;
        std::srand(291030);
        auto fixture = crossfeed_fixture(app, info, tuning, presentation, preferences,
            count, false, 0.75f, heavy_contacts, "turbulence_slab_trial"); ++builds; ++ending_levels;
        const auto slab_count = std::count_if(fixture.structures.begin(), fixture.structures.end(),
            [](const CollisionObject* object) { return dynamic_cast<const FixedSlabPaintable*>(object) != nullptr; });
        const auto shadow_count = std::count_if(fixture.static_shadows.begin(), fixture.static_shadows.end(),
            [](const GameObject* object) { return dynamic_cast<const FixedSlabShadow*>(object) != nullptr; });
        require(fixture.structures.size() == 24 && slab_count == 2 && shadow_count == 2 &&
            fixture.level->horizontal_structure_states().size() == 2 && fixture.moving_shadows.size() == 2,
            "Slab ending fixture lost its actual fixed/moving shapes or composed shadows");
        if (heavy) { heavy_paint(fixture); heavy_min = std::min(heavy_min, turbulence_slab_ownership(fixture).second); }
        std::size_t level_updates = 0;
        const auto tick = [&]()
        {
            require(++level_updates <= 1200 && ++updates <= 8000, "Slab short-match ending exceeded its finite update budget");
            ++ending_updates; fixture.level->update(neutral, dt);
        };
        while (fixture.level->state() != LevelState::overview) { tick(); }
        const auto fullscreen = app.viewport_manager()->fullscreen_viewport();
        require(fixture.scene->view_count() == 1 && fixture.scene->view(0).viewport == fullscreen,
            "Slab actual ending did not collapse to its fullscreen Scene overview");
        const auto expected_camera = Camera::frame(info.zoom_out_finish_bounds_rectangle(), fullscreen);
        const auto& actual_view = fixture.scene->view(0);
        require(actual_view.camera == expected_camera && actual_view.camera.visible_rectangle(actual_view.viewport).contains(info.zoom_out_finish_bounds_rectangle()),
            "Slab actual ending camera does not fit its complete authored overview");
        const auto record_ending = [&](const std::string& stem, bool menu, const Session* session)
        {
            const auto result = fixture.level->level_end_info(); const auto& actual = fixture.scene->view(0);
            const auto pane = actual.viewport.pixel_rect(); const auto held = turbulence_slab_ownership(fixture);
            require(result.total_area == held.first && (heavy ? held.second > 0.99f : held.second == 0),
                "Actual slab ending lost its held score or controlled paint label");
            require(fixture.level->state() == (menu ? LevelState::finished : LevelState::overview) &&
                result.team_a_played && result.team_b_played == (count == 4), "Slab ending state/result participation differs from actual roster");
            ending_csv << stem << ',' << (menu ? "actual-EndMenuInitial-derived-result" : "actual-Level-overview") << ',' <<
                (heavy ? "heavy" : "clean") << ',' << count << ',' << static_cast<int>(fixture.level->state()) << ',' <<
                fixture.scene->view_count() << ',' << fixture.structures.size() << ',' << fixture.level->horizontal_structure_states().size() << ',' <<
                fixture.moving_shadows.size() << ',' << slab_count << ',' << shadow_count << ",0," << level_updates << ",0.75," << menu << ',' <<
                (menu ? "actual-LevelEndInfo-Session" : "not-applicable") << ',' << result.team_a_played << ',' << result.team_b_played << ',' <<
                result.total_area << ',' << result.team_a_area << ',' << result.team_b_area << ',' << result.coverage_ratio() << ',' <<
                static_cast<int>(result.winning_team()) << ',' << (session ? session->round() : 0) << ',' <<
                (session ? session->team_a_wins() : 0) << ',' << (session ? session->team_b_wins() : 0) << ',' << (session && session->complete()) << ',' <<
                (menu ? "actual-Scene-finished-plus-menu" : "actual-Scene-overview") << ',' <<
                actual.camera.translation.x << ',' << actual.camera.translation.y << ',' << actual.camera.scale << ',' <<
                pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << ',' << pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << ',' <<
                held.second << ",1,checked,checked,"
#if COLOURWARS_DEV_TOOLS
                << "checked"
#else
                << "not-built"
#endif
                << ",checked,checked,controlled-native-inspection-not-human-acceptance\n"; ++ending_rows;
        };
        const auto stem = "hidden-inspection-turbulence-slab-ending-" + std::to_string(count) + "p-" + (heavy ? "heavy-" : "clean-");
        {
            const auto before = turbulence_slab_frozen_state(fixture); const auto result = turbulence_slab_end_state(fixture.level->level_end_info());
            const auto frame = crossfeed_frozen(app, fixture, reads);
            require(before == turbulence_slab_frozen_state(fixture) && result == turbulence_slab_end_state(fixture.level->level_end_info()),
                "Slab overview drawing changed actual result/paint/collider/shadow state");
            const auto name = stem + "overview-720p"; save(frame, output / (name + ".ppm")); ++frames; ++ending_frames;
            save_crop(frame, fullscreen.pixel_rect(), name + "-view1-native"); record_ending(name, false, nullptr);
        }
        while (fixture.level->state() != LevelState::finished) { tick(); }
        const LevelEndInfo actual_result = fixture.level->level_end_info();
        Session session; session.finish_round(actual_result);
        Save saved; const_cast<PlaySettings&>(saved.play_settings()) = preferences;
        auto& menu_preferences = const_cast<PlaySettings&>(saved.play_settings());
        menu_preferences.music_volume = menu_preferences.sfx_volume = 0;
        GameData data; data.set_application(&app); data.set_save(&saved); data.set_level_infos(&trial_levels);
        data.set_tuning(&tuning); data.set_presentation(&presentation);
        MenuInput input(app.gamepads(), app.keyboard(), &tuning);
        MatchSetup settings; settings.set_stage("turbulence_slab_trial"); settings.set_player_count(count);
        constexpr std::array<int, 4> slots{ 3, 1, 2, 0 }; settings.begin_input_claims(slots[0]);
        for (int view = 1; view < count; ++view) { require(settings.claim_input_slot(slots[static_cast<std::size_t>(view)]), "Ending sparse input claim failed"); }
        for (int view = 0; view < count; ++view) { settings.set_player_team(view, fixture.level->player(view).team()); }
        MenuContext context(&data, &input, &settings);
        StateContext& stack = app; stack.clear();
        int actions = 0;
        struct ClearEndingStack { StateContext& stack; ~ClearEndingStack() { this->stack.clear(); } } clear{ stack };
        stack.notify_activation(true); app.keyboard()->set_focused(true); app.mouse()->set_focused(true);
        for (const Key key : { Key::up, Key::down, Key::left, Key::right, Key::enter, Key::space, Key::escape }) { app.keyboard()->on_key_up(key); }
        app.keyboard()->poll(); app.keyboard()->poll(); app.mouse()->poll(); app.mouse()->poll(); input.reset_actions();
        stack.push(std::make_unique<SlabEndingBackdrop>(*fixture.level));
        stack.push<EndMenuAction>(std::make_unique<EndMenuInitial>(&context, session), [&](const EndMenuAction&) { ++actions; });
        stack.update(0.51f);
        require(stack.depth() == 2 && actions == 0 && !context.music_exit_pending(), "Actual slab EndMenuInitial failed to remain a neutral result-derived page");
        const auto public_before = observation(*fixture.level); const auto full_before = turbulence_slab_frozen_state(fixture);
        const auto result_before = turbulence_slab_end_state(actual_result);
        const std::array<int, 4> session_before{ session.round(), session.team_a_wins(), session.team_b_wins(), static_cast<int>(session.complete()) };
#if COLOURWARS_DEV_TOOLS
        const auto fingerprint = MatchFingerprint::of(*fixture.level, 0);
#endif
        const auto draw_menu = [&]()
        { app.renderer()->begin_frame(); stack.draw(*app.renderer()); ++reads; return read(*app.renderer()); };
        std::srand(2931); const int expected_rng = std::rand(); std::srand(2931);
        const auto frame = draw_menu(); const auto repeated = draw_menu();
        require(frame.width == 1280 && frame.height == 720 && frame.pixels == repeated.pixels && std::rand() == expected_rng &&
            public_before == observation(*fixture.level) && full_before == turbulence_slab_frozen_state(fixture) &&
            result_before == turbulence_slab_end_state(fixture.level->level_end_info()) && session_before ==
                std::array<int, 4>{ session.round(), session.team_a_wins(), session.team_b_wins(), static_cast<int>(session.complete()) } &&
            stack.depth() == 2 && actions == 0 && !context.music_exit_pending(),
            "Actual slab EndMenu drawing changed pixels, Level/result/Session state, stack action or RNG");
#if COLOURWARS_DEV_TOOLS
        require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*fixture.level, 0)).empty(), "Slab EndMenu drawing changed Debug replay state");
#endif
        const auto name = stem + "end-menu-720p"; save(frame, output / (name + ".ppm")); ++frames; ++ending_frames;
        save_crop(frame, fullscreen.pixel_rect(), name + "-view1-native"); record_ending(name, true, &session);
    }
    csv.close(); interior_csv.close(); ending_csv.close();
    require(csv.good() && interior_csv.good() && ending_csv.good() && read_source() == source && frames == 33 && crops == 75 && rows == 71 && ending_rows == 4 && reads == 67 && builds == 10 && placements == 20 &&
        ending_frames == 4 && ending_levels == 2 &&
        wet_frames == 4 && wet_contacts == 8 && interior_candidates == 7 && interior_checks == 6,
        "Slab native source/inventory/initial-placement count differs");
    std::ofstream summary(output / "turbulence-slab-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"turbulence-slab-only\",\"full_frames\":33,\"native_crops\":75,\"csv_rows\":75,\"readbacks\":67,\"level_builds\":10,"
        "\"initial_placements\":20,\"level_updates\":" << updates << ",\"controlled_heavy_contacts\":" << heavy_contacts << ",\"heavy_initial_fraction_min\":" << heavy_min <<
        ",\"wet_frames\":4,\"wet_slab_contacts\":8,\"interior_pixel_candidates\":7,\"interior_pixel_checks\":6,\"friendly_repaint_owner_state\":\"checked\","
        "\"ending_full_frames\":4,\"ending_csv_rows\":4,\"gameplay_csv_rows\":71,\"ending_levels\":2,\"ending_level_updates\":" << ending_updates << ',' <<
        "\"trial_bytes_unchanged\":true,\"human_acceptance\":false}\n";
    std::ofstream scope(output / "turbulence-slab-scope.txt");
    scope << "Explicit24-object fixed scored-slab source loaded into an isolated inspection registry; this probe edits no runtime registration. The source may be the qualified private comparison or the registered Slab Trial. Actual Level/Scene/object/player builders and real fixed/moving shadows; separate CPU evidence owns public LevelBuilder and longer route qualification.\n"
        "Hidden720p all1/2/3/4-player clean/heavy phases: fixed crossover before descent, actual no-jump whole-body slab midpoint (centre1320/2680 within0.04; nominalfeet827, actual body-edge top plane), and actual upper flat stop1060/2940. One controlled placement per actor only; extra3P/4P actors wait on their upper approaches. Broad release-braking plus small analog input pulses reach supported personal-vx0 stops; no intermediate teleport, jump, respawn or cheat.\n"
        "Every actual Scene pane retains whole player, standing-body slope warning, whole-body upper-stop patch and complete solid slab underside. The original mover pair retains its phase/credit/shadow invariants; slab walkers retain carrier0 and fixed slab world quads/local areas/credit stay unchanged during their walks. Controlled projectile contacts provide measured>99percent paint across all scoring roles, not weapon efficiency. Repeated Level draws preserve full pixels, public/structure/collider/local-material/static-shadow bounds, every paint tile's world corners/face/team/raw owner/area and complete moving-owner/carrier/shadow/view state, RNG and Debug fingerprints. Native RGBA crops equal actual source panes.\n"
        "Four additional heavy-only midpoint frames capture immediate actual friendly repaint on the two slabs, before aging. Actual paint ownership/local areas and personal poses remain identical; wet pixels/state/RNG are frozen across repeated draws. The ordinary walk then settles before the upper capture. Six source-derived geometric interior points at known old primer pinholes must be owned blue/black; a seventh candidate is outside the top edge and is explicitly not an interior assertion. Other wet ages, colors and tools are not inferred. Actual isolated StagePreview checks only initial-pose pixels/widget bounds/RNG. Its miniature and diagonal tile raster are retained for visual inspection; no math-only universal seam-absence claim follows.\n"
        "Two further actual0.75-second matches (1Pclean/4Pheavy) run from their original spawns without placement to runtime overview and finished states. Each supplies one actual fullscreen overview and one actual EndMenuInitial over the finished Level. The page receives Session.finish_round(actualLevel.level_end_info()), never synthetic areas or a fake result; its neutral page update does not update the finished Level. This qualifies the displayed ending composition, not GameLevel results-navigation actions. All scoring/public/collider/paint/shadow state, Session ledger, pixels, RNG and Debug fingerprints remain frozen across repeated draws; retained native full-pane crops match all source RGBA/RGB rows. No human trajectory/readability/reaction, physical pad/motor, listening, performance, balance or arena acceptance is established.\n";
    summary.close(); scope.close(); require(summary.good() && scope.good(), "Cannot flush slab native summary/scope");
    std::cout << "PASS slab native controlled descent/endings:33 full frames/75 native crops/67 reads, original all-layout wet/pinhole controls plus actual short-match overview/result-derived EndMenu; human acceptance open\n";
}
