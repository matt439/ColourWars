// Included after crossfeed_checks.h. Observations use completed real Level ticks.
// A conservative geometric candidate is deliberately not a readable raster claim.
struct RouteGeometry
{
    RectangleF projected;
    float clip_width = 0.0f, clip_height = 0.0f, edge_x = 0.0f;
    bool intersects = false, contained = false, edge_inside = false, hud_overlap = false;
};

RectangleF route_intersection(const RectangleF& a, const RectangleF& b)
{
    const float x = std::max(a.left(), b.left()), y = std::max(a.top(), b.top());
    return { x, y, std::max(0.0f, std::min(a.right(), b.right()) - x),
        std::max(0.0f, std::min(a.bottom(), b.bottom()) - y) };
}

RouteGeometry route_geometry(const RectangleF& world, const Camera& camera, const Viewport& viewport,
    const std::array<RectangleF, 3>& hud, bool near_right = false)
{
    RouteGeometry result;
    result.projected = camera.calculate_view_rectangle(world);
    const RectangleF pane(0.0f, 0.0f, viewport.width, viewport.height);
    const RectangleF clipped = route_intersection(result.projected, pane);
    result.clip_width = clipped.width; result.clip_height = clipped.height;
    result.intersects = clipped.width > 0.0f && clipped.height > 0.0f;
    result.contained = pane.contains(result.projected);
    result.edge_x = near_right ? result.projected.right() : result.projected.left();
    result.edge_inside = result.edge_x >= 0.0f && result.edge_x < pane.width &&
        result.projected.top() >= 0.0f && result.projected.bottom() <= pane.height;
    for (const RectangleF& panel : hud)
    {
        const RectangleF overlap = route_intersection(clipped, panel);
        result.hud_overlap = result.hud_overlap || (overlap.width > 0.0f && overlap.height > 0.0f);
    }
    return result;
}

// Paper face+shadow envelopes. Identity and meters use actual font measurements.
// The ordinary clock is conservatively the union of ALL labels in 0:00..4:00;
// it is not an opaque-pixel mask, and world badges/tool effects are not included.
std::array<RectangleF, 3> route_hud_bounds(Application& app, const Presentation& presentation,
    const Player& player, int view, const Viewport& viewport)
{
    const HudPresentation& look = presentation.hud();
    const auto& identity = presentation.player_identity();
    const Vector2F size = viewport.size();
    const float scale = std::min(size.x / look.player_identity_reference_width, size.y / look.reference_height);
    const Vector2F padding = look.player_identity_padding * scale;
    const auto measure = [&](const std::string& font, const std::wstring& text)
    { return app.render_resources()->measure_text(app.render_resources()->resolve_sprite_font(font), text) * scale; };
    const std::wstring label = PlayerBadge::label(view) +
        (player.team() == PlayerTeam::a ? L"  /  TEAM A\n" : L"  /  TEAM B\n") +
        input_slots::device_name(player.player_num());
    const Vector2F label_size = measure(look.player_identity_font, label);
    const float badge = identity.enabled ? (identity.ui_size.x + identity.ui_gap) * scale : 0.0f;
    const Vector2F identity_pos(size.x * look.player_identity_offset.x, size.y * look.player_identity_offset.y);
    RectangleF identity_panel(identity_pos - padding, Vector2F(label_size.x + badge,
        std::max(label_size.y, identity.enabled ? identity.ui_size.y * scale : 0.0f)) + padding * 2.0f);
    const float x = size.x * (1.0f - look.right_margin);
    const float top = size.y * std::min(look.ammo_top_margin, look.health_top_margin) -
        measure(look.meter_label_font, L"HEALTH").y - look.meter_label_gap * scale;
    const float bottom = size.y * (std::max(look.ammo_top_margin, look.health_top_margin) + look.box_size.y);
    RectangleF meters(Vector2F(x, top) - padding,
        Vector2F(size.x * look.box_size.x, bottom - top) + padding * 2.0f);
    float clock_width = 0.0f, clock_height = 0.0f;
    for (int seconds = 0; seconds <= 240; ++seconds)
    {
        const std::wstring text = std::to_wstring(seconds / 60) +
            (seconds % 60 < 10 ? L":0" : L":") + std::to_wstring(seconds % 60);
        const Vector2F measured = measure(look.timer_font, text) *
            (look.player_identity_reference_width / look.timer_reference_width);
        clock_width = std::max(clock_width, measured.x); clock_height = std::max(clock_height, measured.y);
    }
    RectangleF clock(Vector2F((size.x - clock_width) / 2.0f - size.x * look.timer_offset.x,
        size.y * look.timer_offset.y) - padding, Vector2F(clock_width, clock_height) + padding * 2.0f);
    const float shadow = std::max(0.0f, look.paper.shadow_offset * scale);
    for (RectangleF* panel : { &identity_panel, &meters, &clock })
    { panel->width += shadow; panel->height += shadow; }
    return { identity_panel, meters, clock };
}

bool route_supported(const Player& player)
{
    return player.move_state() == PlayerMoveState::on_ground ||
        player.move_state() == PlayerMoveState::on_drop_down_ground ||
        player.move_state() == PlayerMoveState::on_ramp_left || player.move_state() == PlayerMoveState::on_ramp_right;
}

bool route_on_target(const Player& player, const RectangleF& target)
{
    const RectangleF body = player.rectangle();
    return route_supported(player) && std::abs(body.bottom() - target.top()) < 0.05f &&
        body.right() > target.left() && body.left() < target.right();
}

struct RouteTrial
{
    std::string id, stage, name, paint, tool = "none", support = "not-observed";
    int count = 0, view = 0, direction = 1, ticks = 0, commit = -1, lip_crossing = -1, landing = -1;
    int first_edge = -1, first_strip = -1, full_before_commit = -1, full_at_commit = -1, losses = 0;
    int paid = -1, actor = -1, collision = -1, candidate = -1, damage = -1;
    bool previous_full = false, previous_whole = false, whole_before_commit = false;
    RectangleF target, strip;
    std::array<RectangleF, 3> hud_boxes{};
    Vector2F aim = Vector2F::ZERO;
    float initial_owned = 0.0f, initial_health = 0.0f;
};

struct RouteWarningRecords
{
    Application& app;
    const Presentation& presentation;
    std::filesystem::path output;
    std::ofstream ticks, trials, captures, panels;
    size_t updates = 0, builds = 0, readbacks = 0, frames = 0, crops = 0, rows = 0, trial_count = 0, contacts = 0;
    size_t geometry_controls = 0, paired_ticks = 0;

    RouteWarningRecords(Application& shell, const Presentation& look, const std::filesystem::path& path) :
        app(shell), presentation(look), output(path), ticks(path / "route-warning-ticks.csv"),
        trials(path / "route-warning-trials.csv"), captures(path / "route-warning-captures.csv"),
        panels(path / "route-warning-hud.csv")
    {
        ticks.precision(9); trials.precision(9); panels.precision(9);
        ticks << "trial,kind,tick,players,view,input_slot,object_id,player_x,player_y,velocity_x,velocity_y,health,ammo,"
            "camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,world_x,world_y,world_width,world_height,"
            "projected_x,projected_y,projected_width,projected_height,clip_width,clip_height,intersects,contained,edge_x,"
            "edge_inside,hud_overlap,commit,lip_crossing,contact,paid,evidence_scope\n";
        trials << "trial,stage,case,paint,players,view,input_slot,direction,tool,ticks,commit_tick,lip_crossing_tick,landing_tick,"
            "first_edge_tick,first_strip_tick,strip_full_before_commit,strip_full_at_commit,strip_losses,paid_tick,"
            "first_actor_tick,first_collision_tick,first_draw_candidate_tick,first_damage_tick,health_loss,live_projectiles_end,"
            "landing_support,whole_target_before_commit,target_x,target_y,target_width,target_height,strip_x,strip_y,"
            "strip_width,strip_height,initial_owned_fraction,aim_x,aim_y,projectile_attribution,evidence_scope\n";
        captures << "capture,trial,event,tick,players,view,input_slot,pane_x,pane_y,pane_width,pane_height,full_width,full_height,"
            "frozen_pixels,public_state,projectile_state,debug_fingerprint,rng,evidence_scope\n";
        panels << "trial,panel,x,y,width,height,envelope_scope\n";
    }

    void step(CrossfeedFixture& fixture, const std::vector<PlayerInputData>& inputs)
    {
        require(++updates <= 15000, "Route/warning leaf exceeded its Level update ceiling");
        fixture.level->update(inputs, dt);
    }

    void settle(CrossfeedFixture& fixture)
    {
        ++builds;
        const auto neutral = neutral_inputs();
        for (int tick = 0; tick < 600 && fixture.level->state() != LevelState::active; ++tick) { step(fixture, neutral); }
        require(fixture.level->state() == LevelState::active, "Route/warning fixture never became active");
        for (int tick = 0; tick < 90; ++tick) { step(fixture, neutral); } // GO overlay expires after one active second.
    }

    std::array<RectangleF, 3> hud(CrossfeedFixture& fixture, const RouteTrial& trial)
    { return route_hud_bounds(app, presentation, fixture.level->player(trial.view), trial.view, fixture.scene->view(trial.view).viewport); }

    void begin(CrossfeedFixture& fixture, RouteTrial& trial)
    {
        ++trial_count;
        trial.initial_owned = crossfeed_ownership(fixture).second;
        trial.initial_health = fixture.level->player(trial.view).health();
        const auto boxes = hud(fixture, trial); trial.hud_boxes = boxes;
        for (size_t index = 0; index < boxes.size(); ++index)
        {
            const RectangleF& box = boxes[index];
            panels << trial.id << ',' << index << ',' << box.x << ',' << box.y << ',' << box.width << ',' << box.height <<
                ",conservative-paper-face-shadow-clock-union\n";
        }
    }

    RouteGeometry observe(CrossfeedFixture& fixture, const RouteTrial& trial, const char* kind, int tick,
        const RectangleF& world, int object_id = 0)
    {
        require(++rows <= 25000, "Route/warning leaf exceeded its observation row ceiling");
        const auto& view = fixture.scene->view(trial.view);
        require(view.camera == fixture.level->view_camera(trial.view), "Post-tick Scene camera disagrees with Level draw camera");
        const Player& player = fixture.level->player(trial.view);
        const RouteGeometry geometry = route_geometry(world, view.camera, view.viewport, trial.hud_boxes,
            std::string_view(kind) == "landing-strip" && trial.direction < 0);
        const RectangleF body = player.rectangle(); const RectangleI pane = view.viewport.pixel_rect();
        const RectangleF& projected = geometry.projected;
        ticks << trial.id << ',' << kind << ',' << tick << ',' << trial.count << ',' << trial.view + 1 << ',' << player.player_num() << ',' << object_id << ',' <<
            body.x << ',' << body.y << ',' << player.velocity().x << ',' << player.velocity().y << ',' << player.health() << ',' << player.weapon_ammo() << ',' <<
            view.camera.translation.x << ',' << view.camera.translation.y << ',' << view.camera.scale << ',' <<
            pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << ',' <<
            world.x << ',' << world.y << ',' << world.width << ',' << world.height << ',' <<
            projected.x << ',' << projected.y << ',' << projected.width << ',' << projected.height << ',' <<
            geometry.clip_width << ',' << geometry.clip_height << ',' << geometry.intersects << ',' << geometry.contained << ',' <<
            geometry.edge_x << ',' << geometry.edge_inside << ',' << geometry.hud_overlap << ',' << (trial.commit == tick) << ',' << (trial.lip_crossing == tick) << ',' <<
            (trial.landing == tick || trial.damage == tick) << ',' << (trial.paid == tick) << ",post-tick-geometric-observation\n";
        return geometry;
    }

    void landing(CrossfeedFixture& fixture, RouteTrial& trial, int tick)
    {
        const RouteGeometry box = observe(fixture, trial, "landing-strip", tick, trial.strip);
        const auto& view = fixture.scene->view(trial.view);
        const bool whole = view.camera.visible_rectangle(view.viewport).contains(trial.target);
        if (box.edge_inside && trial.first_edge < 0) { trial.first_edge = tick; }
        if (box.contained && trial.first_strip < 0) { trial.first_strip = tick; }
        if (trial.commit == tick)
        {
            trial.full_before_commit = trial.previous_full ? 1 : 0;
            trial.whole_before_commit = trial.previous_whole;
            trial.full_at_commit = box.contained ? 1 : 0;
        }
        if ((trial.landing < 0 || tick <= trial.landing) && trial.previous_full && !box.contained) { ++trial.losses; }
        trial.previous_full = box.contained; trial.previous_whole = whole; trial.ticks = tick;
    }

    std::vector<float> projectile_state(const CrossfeedFixture& fixture) const
    {
        std::vector<float> state;
        for (const CollisionObject* object : fixture.scene->collision_objects())
        {
            if (const auto* shot = dynamic_cast<const Projectile*>(object))
            {
                const RectangleF box = shot->shape()->bounding_box(), art = shot->cull_bounds(1.5f);
                state.insert(state.end(), { box.x, box.y, box.width, box.height, art.x, art.y, art.width, art.height,
                    static_cast<float>(static_cast<int>(shot->projectile_type())), shot->delete_timer(), shot->for_deletion() ? 1.0f : 0.0f });
            }
        }
        return state;
    }

    void capture(CrossfeedFixture& fixture, const RouteTrial& trial, const char* event, int tick,
        bool all_views = false)
    {
        require(++frames <= 40, "Route/warning leaf exceeded its full-frame capture ceiling");
        const auto before = projectile_state(fixture);
        const Frame frame = crossfeed_frozen(app, fixture, readbacks);
        require(before == projectile_state(fixture), "Frozen route/warning draw changed surviving projectile state");
        const std::string stem = "route-warning-" + trial.id + '-' + event + '-' + std::to_string(tick);
        save(frame, output / (stem + "-full.ppm"));
        for (int view_index = all_views ? 0 : trial.view; view_index < (all_views ? trial.count : trial.view + 1); ++view_index)
        {
            require(++crops <= 100, "Route/warning leaf exceeded its native pane crop ceiling");
            const RectangleI pane = fixture.scene->view(view_index).viewport.pixel_rect();
            save(crossfeed_crop(frame, pane), output / (stem + "-player" + std::to_string(view_index + 1) + ".ppm"));
            captures << stem << ',' << trial.id << ',' << event << ',' << tick << ',' << trial.count << ',' << view_index + 1 << ',' <<
                fixture.level->player(view_index).player_num() << ',' << pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height <<
                ",1280,720,1,1,1," <<
#if COLOURWARS_DEV_TOOLS
                "1" <<
#else
                "not-available-in-Release" <<
#endif
                ",1,hidden-native-inspection\n";
        }
    }

    void finish(CrossfeedFixture& fixture, const RouteTrial& trial)
    {
        trials << trial.id << ',' << trial.stage << ',' << trial.name << ',' << trial.paint << ',' << trial.count << ',' << trial.view + 1 << ',' <<
            fixture.level->player(trial.view).player_num() << ',' << trial.direction << ',' << trial.tool << ',' << trial.ticks << ',' <<
            trial.commit << ',' << trial.lip_crossing << ',' << trial.landing << ',' << trial.first_edge << ',' << trial.first_strip << ',' << trial.full_before_commit << ',' <<
            trial.full_at_commit << ',' << trial.losses << ',' << trial.paid << ',' << trial.actor << ',' << trial.collision << ',' << trial.candidate << ',' <<
            trial.damage << ',' << trial.initial_health - fixture.level->player(trial.view).health() << ',' << fixture.level->projectile_count() << ',' <<
            trial.support << ',' << trial.whole_before_commit << ',' << trial.target.x << ',' << trial.target.y << ',' << trial.target.width << ',' << trial.target.height << ',' <<
            trial.strip.x << ',' << trial.strip.y << ',' << trial.strip.width << ',' << trial.strip.height << ',' << trial.initial_owned << ',' <<
            trial.aim.x << ',' << trial.aim.y << ",single-paid-burst-or-no-projectile,simulated-60Hz-observation\n";
    }
};

void route_geometry_controls(RouteWarningRecords& records)
{
    const Camera camera(100.0f, 200.0f, 2.0f / 3.0f);
    const Viewport pane(RectangleI(640, 360, 640, 360));
    const std::array<RectangleF, 3> hud{ RectangleF(10, 10, 50, 50), RectangleF(), RectangleF() };
    const auto fully = route_geometry(RectangleF(130, 230, 52, 8), camera, pane, hud);
    require(fully.contained && fully.edge_inside && fully.hud_overlap && std::abs(fully.clip_width - 52.0f * 2.0f / 3.0f) < 0.001f,
        "Route geometry control lost non-unit scale, local pane origin or HUD intersection");
    ++records.geometry_controls;
    const auto clipped = route_geometry(RectangleF(1059.5f, 230, 52, 8), camera, pane, hud);
    require(clipped.intersects && !clipped.contained && clipped.clip_width < 1.0f && !clipped.hud_overlap,
        "Route geometry sliver control confused intersection, full fit or raster proof");
    ++records.geometry_controls;
    const auto outside = route_geometry(RectangleF(1060, 230, 52, 8), camera, pane, hud);
    require(!outside.intersects && !outside.contained && !outside.edge_inside,
        "Route geometry control accepted an edge exactly outside the pane");
    ++records.geometry_controls;
    const auto bottom = route_geometry(RectangleF(130, 739, 52, 8), camera, pane, hud);
    require(bottom.intersects && !bottom.contained && !bottom.edge_inside,
        "Route geometry control missed vertical clipping of a landing strip");
    ++records.geometry_controls;
    const auto wide = route_geometry(RectangleF(100, 230, 980, 40), camera, pane, hud);
    require(wide.intersects && !wide.contained,
        "Route geometry control treated an oversized whole support as fully visible");
    ++records.geometry_controls;
}
