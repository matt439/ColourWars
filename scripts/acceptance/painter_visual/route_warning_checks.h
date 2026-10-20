#include "scripts/acceptance/painter_visual/route_warning_fixtures.h"

void route_pair_equal(RouteWarningRecords& records, const std::array<CrossfeedFixture, 2>& pair)
{
    ++records.paired_ticks;
    for (int view = 0; view < pair[0].level->player_count(); ++view)
    {
        const Player& clean = pair[0].level->player(view), &painted = pair[1].level->player(view);
        require(clean.rectangle() == painted.rectangle() && clean.velocity() == painted.velocity() &&
            clean.move_state() == painted.move_state() && clean.health() == painted.health() &&
            pair[0].scene->view(view).camera == pair[1].scene->view(view).camera,
            "Controlled heavy paint changed a paired route's motion, support, health or actual camera");
    }
}

void route_crossfeed_approaches(RouteWarningRecords& records, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, const PlaySettings& preferences)
{
    const LevelLoadedInfo& info = *levels.get("crossfeed");
    const RectangleF floor = crossfeed_rectangle(info, "floor_paint_a");
    const RectangleF plinth = crossfeed_rectangle(info, "centre_plinth");
    const RectangleF bench_a = crossfeed_rectangle(info, "side_paint_a"), bench_b = crossfeed_rectangle(info, "side_paint_b");
    const RectangleF ruler_a = crossfeed_rectangle(info, "upper_step_a"), ruler_b = crossfeed_rectangle(info, "upper_step_b");
    const Vector2F size = tuning.player().size;
    for (int count = 1; count <= 4; ++count)
    {
        std::array<CrossfeedFixture, 2> pair;
        std::array<std::vector<RouteTrial>, 2> trials;
        for (int paint = 0; paint < 2; ++paint)
        {
            pair[paint] = crossfeed_fixture(records.app, info, tuning, presentation, preferences, count,
                paint != 0, 240.0f, records.contacts);
            records.settle(pair[paint]);
            for (int view = 0; view < count; ++view)
            {
                const int direction = view % 2 == 0 ? 1 : -1;
                const bool upper = view >= 2;
                const RectangleF bench = direction > 0 ? bench_a : bench_b;
                const RectangleF target = upper ? (direction > 0 ? ruler_a : ruler_b) : plinth;
                const float centre = upper ? (direction > 0 ? bench.right() - size.x / 2.0f : bench.left() + size.x / 2.0f) :
                    (direction > 0 ? 1180.0f : 2420.0f);
                require(pair[paint].level->place_player(view,
                    { centre - size.x / 2.0f, (upper ? bench.top() : floor.top()) - size.y }),
                    "Route Crossfeed controlled initial station failed");
                RouteTrial trial;
                trial.id = "crossfeed-" + std::to_string(count) + "p-" + (paint == 0 ? "clean" : "heavy") + "-v" + std::to_string(view + 1);
                trial.stage = "crossfeed"; trial.name = upper ? "optional-ruler-tap" : "compulsory-ramp-walk";
                trial.paint = paint == 0 ? "clean" : "heavy"; trial.count = count; trial.view = view; trial.direction = direction;
                trial.target = target;
                trial.strip = { direction > 0 ? target.left() : target.right() - size.x, target.top(), size.x, 8.0f };
                trials[paint].push_back(trial);
            }
            records.step(pair[paint], neutral_inputs());
            for (RouteTrial& trial : trials[paint]) { records.begin(pair[paint], trial); records.landing(pair[paint], trial, 0); }
        }
        route_pair_equal(records, pair);
        for (int tick = 1; tick <= 120; ++tick)
        {
            auto inputs = neutral_inputs();
            for (const RouteTrial& trial : trials[0])
            {
                const Player& player = pair[0].level->player(trial.view);
                auto& input = inputs[static_cast<size_t>(player.player_num())];
                const RectangleF body = player.rectangle();
                const bool upper = trial.view >= 2;
                const bool steer = trial.landing < 0 && (upper ?
                    (trial.direction > 0 ? body.left() <= trial.target.left() + 20.0f : body.right() >= trial.target.right() - 20.0f) :
                    (trial.direction > 0 ? body.center().x < trial.target.left() + 60.0f : body.center().x > trial.target.right() - 60.0f));
                input.x_movement = steer ? static_cast<float>(trial.direction) : 0.0f;
                input.jump_pressed = upper && tick == 1; input.jump_held = input.jump_pressed;
            }
            for (int paint = 0; paint < 2; ++paint)
            {
                records.step(pair[paint], inputs);
                for (RouteTrial& trial : trials[paint])
                {
                    const Player& player = pair[paint].level->player(trial.view);
                    const float front = trial.direction > 0 ? player.rectangle().right() : player.rectangle().left();
                    const bool upper = trial.view >= 2;
                    const float lip = upper ? (trial.direction > 0 ? bench_a.right() : bench_b.left()) :
                        (trial.direction > 0 ? 1300.0f : 2300.0f);
                    if (trial.lip_crossing < 0 && static_cast<float>(trial.direction) * (front - lip) > 0.0f) { trial.lip_crossing = tick; }
                    if (trial.commit < 0 && (upper ? player.velocity().y < 0.0f : trial.lip_crossing == tick)) { trial.commit = tick; }
                    if (trial.landing < 0 && trial.commit >= 0 && route_on_target(player, trial.target))
                    { trial.landing = tick; trial.support = "target"; }
                    records.landing(pair[paint], trial, tick);
                }
                const RouteTrial& focus = trials[paint][0];
                if (focus.commit == tick) { records.capture(pair[paint], focus, "walk-entry", tick, true); }
            }
            route_pair_equal(records, pair);
        }
        for (int paint = 0; paint < 2; ++paint)
        { for (const RouteTrial& trial : trials[paint]) { records.finish(pair[paint], trial); } }
    }
}

void route_course_landings(RouteWarningRecords& records, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, const PlaySettings& preferences)
{
    const LevelLoadedInfo& info = *levels.get("movement_course");
    const std::array<RectangleF, 4> supports{ crossfeed_rectangle(info, "spawn_a_runway"),
        crossfeed_rectangle(info, "short_landing_long_takeoff"), crossfeed_rectangle(info, "long_landing_running_takeoff"),
        crossfeed_rectangle(info, "running_landing") };
    constexpr std::array<const char*, 3> names{ "tap120", "held280", "running440-early120" };
    const Vector2F size = tuning.player().size;
    for (int count = 1; count <= 4; ++count)
    {
        std::array<CrossfeedFixture, 2> pair;
        for (int paint = 0; paint < 2; ++paint)
        {
            pair[paint] = crossfeed_fixture(records.app, info, tuning, presentation, preferences, count,
                paint != 0, 240.0f, records.contacts, "movement_course");
            records.settle(pair[paint]);
        }
        for (size_t gap = 0; gap < names.size(); ++gap)
        {
            const RectangleF from = supports[gap], target = supports[gap + 1];
            const float lip = from.right();
            const bool running = gap == 2;
            std::array<RouteTrial, 2> trials;
            for (int paint = 0; paint < 2; ++paint)
            {
                // Start the running route at rest 580 before the lip, then use
                // ordinary movement until the front reaches lip-120. No velocity injection.
                require(pair[paint].level->place_player(0,
                    { lip - (running ? 580.0f : 0.0f) - size.x, from.top() - size.y }),
                    "Course controlled initial gap station failed");
                records.step(pair[paint], neutral_inputs());
                RouteTrial& trial = trials[paint];
                trial.id = "course-" + std::to_string(count) + "p-" + (paint == 0 ? "clean-" : "heavy-") + names[gap];
                trial.stage = "movement_course"; trial.name = names[gap]; trial.paint = paint == 0 ? "clean" : "heavy";
                trial.count = count; trial.target = target; trial.strip = { target.left(), target.top(), size.x, 8.0f };
                records.begin(pair[paint], trial); records.landing(pair[paint], trial, 0);
            }
            route_pair_equal(records, pair);
            for (int tick = 1; tick <= 150; ++tick)
            {
                auto inputs = neutral_inputs();
                const RouteTrial& clean = trials[0];
                const Player& player = pair[0].level->player(0);
                const bool press = clean.commit < 0 && (!running || player.rectangle().right() >= lip - 120.0f);
                auto& input = inputs[static_cast<size_t>(player.player_num())];
                input.jump_pressed = press;
                input.jump_held = gap == 0 ? press : (clean.commit < 0 || clean.landing < 0);
                input.x_movement = clean.landing < 0 && player.rectangle().left() <= target.left() + 20.0f ? 1.0f : 0.0f;
                for (int paint = 0; paint < 2; ++paint)
                {
                    const bool supported_before = route_supported(pair[paint].level->player(0));
                    records.step(pair[paint], inputs);
                    RouteTrial& trial = trials[paint]; const Player& after = pair[paint].level->player(0);
                    if (trial.commit < 0 && press && supported_before && after.velocity().y < 0.0f)
                    {
                        trial.commit = tick;
                        if (running) { require(std::abs(after.velocity().x - tuning.player().max_velocity.x) < 0.01f,
                            "Course early running takeoff was not reached by actual full-speed inputs"); }
                    }
                    if (trial.lip_crossing < 0 && after.rectangle().right() > lip) { trial.lip_crossing = tick; }
                    if (trial.landing < 0 && trial.commit >= 0 && tick > trial.commit && route_supported(after))
                    { trial.landing = tick; trial.support = route_on_target(after, target) ? "target" : "other"; }
                    records.landing(pair[paint], trial, tick);
                    if (trial.commit == tick && (running || count == 4)) { records.capture(pair[paint], trial, "takeoff", tick); }
                    if (trial.landing == tick && count == 4) { records.capture(pair[paint], trial, "first-contact", tick); }
                }
                route_pair_equal(records, pair);
                if (trials[0].landing >= 0 && trials[1].landing >= 0) { break; }
            }
            for (int paint = 0; paint < 2; ++paint)
            {
                require(trials[paint].commit >= 0 && trials[paint].landing > trials[paint].commit,
                    "Course route observation did not include actual takeoff and a first contact");
                records.finish(pair[paint], trials[paint]);
            }
        }
    }
}

struct RouteIncomingCase
{
    int count;
    WeaponType tool;
    const char* name;
    bool retreat = false, mirrored = false, heavy = false;
};

void route_incoming_shots(RouteWarningRecords& records, Registry<LevelLoadedInfo>& levels,
    const GameTuning& tuning, const Presentation& presentation, const PlaySettings& preferences)
{
    const LevelLoadedInfo& info = *levels.get("crossfeed");
    const Vector2F size = tuning.player().size;
    std::vector<RouteIncomingCase> cases;
    std::vector<std::pair<std::string, std::vector<float>>> clean_traces;
    const std::array tools{ std::pair{ WeaponType::sprayer, "sprayer" }, std::pair{ WeaponType::sniper, "sniper" },
        std::pair{ WeaponType::mister, "mister" }, std::pair{ WeaponType::bucket, "bucket" } };
    for (const auto& tool : tools)
    { for (bool retreat : { false, true }) { cases.push_back({ 2, tool.first, tool.second, retreat }); } }
    for (int count : { 3, 4 })
    { for (const auto& tool : { tools[1], tools[3] })
      { for (bool retreat : { false, true }) { cases.push_back({ count, tool.first, tool.second, retreat }); } } }
    for (const auto& tool : { tools[1], tools[3] })
    { for (bool retreat : { false, true }) { cases.push_back({ 4, tool.first, tool.second, retreat, true }); } }
    for (const auto& tool : { tools[1], tools[3] }) { cases.push_back({ 4, tool.first, tool.second, true, false, true }); }
    require(cases.size() == 22, "Route incoming representative matrix changed size");
    for (const RouteIncomingCase& entry : cases)
    {
        auto fixture = crossfeed_fixture(records.app, info, tuning, presentation, preferences, entry.count,
            entry.heavy, 240.0f, records.contacts, "crossfeed", entry.tool);
        records.settle(fixture);
        // Open lower corridor, below the home-baffle underside. These stations
        // and fixed aims are controlled representative shots, not a smart bot.
        const float separation = entry.tool == WeaponType::sniper ? 750.0f :
            (entry.tool == WeaponType::sprayer ? 450.0f : 300.0f);
        const float attacker_x = entry.mirrored ? 2550.0f : 1050.0f;
        const int shot_direction = entry.mirrored ? 1 : -1;
        const float target_x = attacker_x + static_cast<float>(shot_direction) * separation;
        for (int view = 0; view < 2; ++view)
        {
            require(fixture.level->place_player(view, { (view == 0 ? target_x : attacker_x) - size.x / 2.0f, 1280.0f - size.y }),
                "Incoming controlled initial attacker/target station failed");
        }
        for (int view = 2; view < entry.count; ++view)
        {
            const float safe_centre = entry.mirrored ? 300.0f - 120.0f * static_cast<float>(view - 2) :
                3300.0f + 120.0f * static_cast<float>(view - 2);
            require(fixture.level->place_player(view, { safe_centre - size.x / 2.0f, 1120.0f - size.y }),
                "Incoming idle roster actor could not be placed behind the attacker at its safe home station");
        }
        RouteTrial trial;
        trial.id = "incoming-" + std::to_string(entry.count) + "p-" + entry.name +
            (entry.retreat ? "-retreat" : "-stationary") + (entry.mirrored ? "-mirror" : "-left") + (entry.heavy ? "-heavy" : "-clean");
        trial.stage = "crossfeed"; trial.name = entry.retreat ? "fixed-single-burst-retreat" : "fixed-single-burst-stationary";
        trial.paint = entry.heavy ? "heavy" : "clean"; trial.tool = entry.name; trial.count = entry.count; trial.direction = shot_direction;
        const float loft = entry.tool == WeaponType::sprayer ? 0.4f : (entry.tool == WeaponType::bucket ? 0.2f : 0.0f);
        trial.aim = Vector2F(static_cast<float>(shot_direction) * std::cos(loft), -std::sin(loft));
        auto setup = neutral_inputs(); auto& setup_shooter = setup[static_cast<size_t>(fixture.level->player(1).player_num())];
        setup_shooter.shoot_direction = trial.aim; setup_shooter.shoot_angle = trial.aim.angle();
        records.step(fixture, setup);
        records.begin(fixture, trial);
        std::vector<const Projectile*> identities;
        std::vector<float> motion_trace;
        bool captured_end = false, captured_candidate = false;
        const bool capture_end = entry.count == 4 || (entry.tool == WeaponType::sniper && !entry.retreat);
        const auto observe = [&](int tick)
        {
            const Player& target = fixture.level->player(0), &attacker = fixture.level->player(1);
            if (trial.damage < 0 && target.health() < trial.initial_health) { trial.damage = tick; }
            std::vector<const Projectile*> live;
            for (const CollisionObject* object : fixture.scene->collision_objects())
            {
                if (const auto* shot = dynamic_cast<const Projectile*>(object); shot && shot->player_num() == attacker.player_num())
                { live.push_back(shot); }
            }
            records.observe(fixture, trial, "target", tick, target.rectangle());
            const auto actor = records.observe(fixture, trial, "attacker", tick, attacker.rectangle());
            if (actor.intersects && trial.actor < 0) { trial.actor = tick; }
            for (const Projectile* shot : live)
            {
                const auto found = std::find(identities.begin(), identities.end(), shot);
                const int id = found == identities.end() ? static_cast<int>(identities.size()) + 1 :
                    static_cast<int>(std::distance(identities.begin(), found)) + 1;
                if (found == identities.end()) { identities.push_back(shot); }
                // No pointer survives as a dereference across updates. Only objects
                // reacquired from the post-end_tick live list are observed/drawn.
                const auto collision = records.observe(fixture, trial, "collision", tick, shot->shape()->bounding_box(), id);
                const auto& view = fixture.scene->view(0);
                const auto candidate = records.observe(fixture, trial, "draw-candidate", tick,
                    shot->cull_bounds(1.0f / view.camera.scale), id);
                if (collision.intersects && trial.collision < 0) { trial.collision = tick; }
                if (candidate.intersects && trial.candidate < 0) { trial.candidate = tick; }
            }
            trial.ticks = tick;
            for (int view_index = 0; view_index < trial.count; ++view_index)
            {
                const Player& actor_player = fixture.level->player(view_index);
                const RectangleF body = actor_player.rectangle(); const Camera camera = fixture.scene->view(view_index).camera;
                motion_trace.insert(motion_trace.end(), { body.x, body.y, body.width, body.height,
                    actor_player.velocity().x, actor_player.velocity().y, actor_player.health(), actor_player.weapon_ammo(),
                    camera.translation.x, camera.translation.y, camera.scale });
            }
            const auto shot_state = records.projectile_state(fixture);
            motion_trace.push_back(static_cast<float>(shot_state.size()));
            motion_trace.insert(motion_trace.end(), shot_state.begin(), shot_state.end());
            if (entry.heavy && trial.candidate == tick && !captured_candidate)
            { records.capture(fixture, trial, "first-geometric-candidate", tick); captured_candidate = true; }
            if (capture_end && trial.damage == tick && !captured_end)
            { records.capture(fixture, trial, "first-health-loss", tick); captured_end = true; }
        };
        observe(0);
        for (int tick = 1; tick <= 96; ++tick)
        {
            auto inputs = neutral_inputs();
            auto& target_input = inputs[static_cast<size_t>(fixture.level->player(0).player_num())];
            target_input.x_movement = entry.retreat ? static_cast<float>(shot_direction) : 0.0f;
            auto& shooter = inputs[static_cast<size_t>(fixture.level->player(1).player_num())];
            shooter.shoot_direction = trial.aim; shooter.shoot_angle = trial.aim.angle(); shooter.primary_shoot = trial.paid < 0;
            const float ammo = fixture.level->player(1).weapon_ammo();
            records.step(fixture, inputs);
            if (fixture.level->player(1).weapon_ammo() < ammo - 0.000001f)
            { require(trial.paid < 0, "Incoming observation paid a second burst"); trial.paid = tick; }
            observe(tick);
            if (trial.damage >= 0 && tick >= trial.damage + 6) { break; }
        }
        require(trial.paid >= 0, "Incoming observation never paid its single actual weapon burst");
        if (capture_end && !captured_end) { records.capture(fixture, trial, "censored-end", trial.ticks); }
        const std::string pair_key = trial.id.substr(0, trial.id.size() - 6); // remove -clean/-heavy
        if (entry.heavy)
        {
            const auto clean = std::find_if(clean_traces.begin(), clean_traces.end(),
                [&](const auto& value) { return value.first == pair_key; });
            require(clean != clean_traces.end() && clean->second == motion_trace,
                "Controlled heavy warning subset changed paired player/camera/projectile traces");
            records.paired_ticks += static_cast<size_t>(trial.ticks + 1);
        }
        else { clean_traces.emplace_back(pair_key, std::move(motion_trace)); }
        records.finish(fixture, trial);
    }
}

void route_warning_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const GameTuning& tuning,
    const Presentation& presentation, PlaySettings& preferences, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    RouteWarningRecords records(app, presentation, output);
    route_geometry_controls(records);
    route_crossfeed_approaches(records, levels, tuning, presentation, preferences);
    route_course_landings(records, levels, tuning, presentation, preferences);
    route_incoming_shots(records, levels, tuning, presentation, preferences);
    require(records.builds == 38 && records.trial_count == 66 && records.frames <= 40 && records.crops <= 100 &&
        records.readbacks == records.frames * 2 && records.geometry_controls == 5 && records.paired_ticks > 0,
        "Route/warning matrix changed its bounded evidence scope");
    std::ofstream summary(output / "route-warning-summary.json"); summary.precision(9);
    summary << "{\"outcome\":\"passed\",\"scope\":\"route-warning-only\",\"full_frames\":" << records.frames <<
        ",\"native_crops\":" << records.crops << ",\"readbacks\":" << records.readbacks << ",\"level_updates\":" << records.updates <<
        ",\"level_builds\":" << records.builds << ",\"tick_rows\":" << records.rows << ",\"trials\":" << records.trial_count <<
        ",\"capture_rows\":" << records.crops << ",\"negative_controls\":" << records.geometry_controls << ",\"paired_ticks\":" << records.paired_ticks <<
        ",\"controlled_heavy_contacts\":" << records.contacts << ",\"route_trials\":44,\"warning_trials\":22,\"simulated_dt\":" << dt << "}\n";
    std::ofstream scope(output / "route-warning-scope.txt");
    scope << "Bounded hidden native 720p observations on actual P3 Level/Player/Scene update order. Sparse view slots 3/1/2/0; 3P order TL/BR/BL.\n"
        "Crossfeed lower ramp walks and optional upper tap routes: controlled initial stations, then 120 ordinary input ticks in paired clean/heavy 1/2/3/4P.\n"
        "Selected Crossfeed rasters capture the lower focus's walk-entry event. Upper tap takeoff is recorded numerically every tick; its commitment raster is not necessarily captured.\n"
        "Course tap 120, held 280 and early 120 running 440: controlled initial rest, actual full-speed run-up, accepted takeoff, later lip crossing and first support contact.\n"
        "A required landing strip is one body width by 8 world units beyond its near lip; whole support fit is context. Continuous numeric ticks record first exposure, losses, precommit/postcommit fit. A failure to fit is retained as an observation.\n"
        "Incoming stationary/retreat shots: one real paid burst, fixed labeled attacker stations/aims, no postlaunch projectile edits. Four aimed tools in 2P; Sniper/Bucket additionally in 3/4P and mirrored 4P; heavy 4P subset. Roller has no projectile draw and is deliberately N/A.\n"
        "Unused incoming roster views 3/4 stand at opposite-home centres x=3300/3420 (mirrored x=300/180), feet y=1120, outside the launch corridor. They cannot intercept a retreating target's selected burst.\n"
        "Samples are AFTER completed Level updates including launch/movement contacts, retirement and pending admission. A contact before any surviving post-tick projectile is a zero draw opportunity case. Missing contact is right-censored at 96 ticks, never infinite/safe.\n"
        "Collision and conservative cull rectangles are geometric candidates, NOT actual sprite pixels, occlusion proof or readable warning. Call-order world coverage, badges, aim/tool effects and opaque HUD pixels are not reconstructed.\n"
        "HUD flags intersect conservative paper-face+shadow envelopes; clock unions all 0:00..4:00 labels. Native full/crop pixels are selected context, not every-tick raster or per-projectile isolation.\n"
        "Single-burst geometry-to-health-loss intervals use simulated 60 Hz ticks; no input/display latency or human reaction-time acceptance. Earlier attacker presence is separate awareness context.\n"
        "Mister candidates aggregate the five pellets of its one paid burst. Earliest candidate is not identified as the pellet that later damages the target.\n"
        "Controlled heavy contacts exceed 99 percent initial ownership; paired routes and warning subset preserve player/camera/public projectile traces. Frozen redraws preserve pixels/public state/static paint/projectile bounds/type/configured-lifetime/deletion/RNG and the public-match Debug fingerprint. Protected projectile velocity/elapsed-lifetime and private animation clocks are not independently inspected.\n"
        "No attended play, physical-pad, listening, human readability, balance, continuous raster visibility or performance acceptance.\n";
    require(records.ticks.good() && records.trials.good() && records.captures.good() && records.panels.good() && summary.good() && scope.good(),
        "Route/warning evidence write failed");
    std::cout << "PASS RouteWarningOnly observations: 66 trials/38 Levels; " << records.frames << " full frames/" << records.crops <<
        " native crops; geometric visibility and censored warning evidence retain objective camera limitations\n";
}
