// Included inside painter_visual.cpp's private namespace after painter_fixtures.h.
// Crossfeed H6 inspection: public Level/Scene construction and real player cameras.
// Heavy paint is established by controlled contacts, not inferred weapon reach.

struct CrossfeedFixture
{
    std::unique_ptr<Level> level;
    Scene* scene = nullptr; // Borrowed from Level; useful for post-tick native observations.
    std::vector<CollisionObject*> structures;
    std::vector<StructurePaintable*> paintables;
};

RectangleF crossfeed_rectangle(const LevelLoadedInfo& info, const char* name)
{
    const JsonValue objects = info.collision_objects_json();
    for (size_t index = 0; index < objects.size(); ++index)
    {
        const JsonValue item = objects.at(index);
        if (!item.has("name") || item.string("name") != name) { continue; }
        const JsonValue box = item.object("rectangle");
        return { box.number("x"), box.number("y"), box.number("width"), box.number("height") };
    }
    throw std::runtime_error(std::string("Crossfeed native station needs authored rectangle '") + name + "'");
}

std::pair<float, float> crossfeed_ownership(const CrossfeedFixture& fixture)
{
    float total = 0.0f, owned = 0.0f;
    for (const StructurePaintable* surface : fixture.paintables)
    { const PaintTotal paint = surface->paint_total(); total += paint.total; owned += paint.team_a + paint.team_b; }
    require(total > 0.0f, "Crossfeed native fixture has no scored surfaces");
    return { total, owned / total };
}

std::vector<float> crossfeed_structure_state(const CrossfeedFixture& fixture)
{
    std::vector<float> state;
    for (const CollisionObject* object : fixture.structures)
    {
        const RectangleF box = object->shape()->bounding_box();
        state.insert(state.end(), { box.x, box.y, box.width, box.height });
        if (const auto* surface = dynamic_cast<const PaintableObject*>(object))
        {
            const PaintTotal paint = surface->paint_total();
            state.insert(state.end(), { paint.total, paint.team_a, paint.team_b });
            state.insert(state.end(), paint.player_areas.begin(), paint.player_areas.end());
        }
    }
    return state;
}

CrossfeedFixture crossfeed_fixture(Application& app, const LevelLoadedInfo& info,
    const GameTuning& tuning, const Presentation& presentation, const PlaySettings& preferences,
    int count, bool heavy, float duration, size_t& heavy_contacts,
    const char* stage = "crossfeed", std::optional<WeaponType> attacker_tool = std::nullopt,
    OpeningMode opening = OpeningMode::off)
{
    constexpr std::array<ScreenLayout, 4> layouts{ ScreenLayout::one_player, ScreenLayout::two_player,
        ScreenLayout::three_player, ScreenLayout::four_player };
    constexpr std::array<int, 4> slots{ 3, 1, 2, 0 };
    constexpr std::array<WeaponType, 4> tools{ WeaponType::sprayer, WeaponType::sniper,
        WeaponType::roller, WeaponType::bucket };
    MenuLevelSettings settings; settings.set_stage(stage); settings.set_player_count(count);
    settings.set_screen_layout(layouts[static_cast<size_t>(count - 1)]);
    settings.begin_input_claims(slots[0]);
    for (int view = 1; view < count; ++view)
    { require(settings.claim_input_slot(slots[static_cast<size_t>(view)]), "Crossfeed native sparse claim failed"); }
    for (int view = 0; view < count; ++view)
    {
        settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
        settings.set_player_weapon(view, count == 2 && view == 1 ? WeaponType::mister : tools[static_cast<size_t>(view)]);
        if (view == 1 && attacker_tool) { settings.set_player_weapon(view, *attacker_tool); }
    }
    app.viewport_manager()->set_layout(settings.screen_layout());
    const TeamColour colours = TeamColourTools::team_colours(TeamColourPair::blue_orange, presentation.team_palette());
    const LevelObjectBuilder objects(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint());
    auto scene = std::make_unique<Scene>(app.thread_pool(), app.partitioner());
    scene->set_bounds(info.out_of_bounds_rectangle());
    for (auto& object : objects.build_non_collision_objects(info.non_collision_objects_json()))
    { scene->add(std::move(object)); }
    for (auto& shadow : objects.build_shadows(info.collision_objects_json()))
    { scene->add(std::move(shadow)); }
    CrossfeedFixture fixture;
    fixture.scene = scene.get();
    for (auto& object : objects.build_collision_objects(info.collision_objects_json(), colours))
    {
        if (auto* surface = dynamic_cast<StructurePaintable*>(object.get()))
        {
            if (heavy)
            {
                RectangleF splash = object->shape()->bounding_box(); splash.inflate(32.0f);
                splash.offset(tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f);
                const PlayerTeam team = fixture.paintables.size() % 2 == 0 ? PlayerTeam::a : PlayerTeam::b;
                const ProjectileSpray shot(splash, Vector2F::ZERO, team, team == PlayerTeam::a ? 3 : 1,
                    colours.team_colour(team), app.render_resources(), resolve_projectile_strip(tuning.projectile(ProjectileType::spray), *app.render_resources()), &tuning.projectile(ProjectileType::spray));
                surface->on_contact(shot, Vector2F::ZERO, 0.0f); surface->update(10.0f); ++heavy_contacts;
            }
            fixture.paintables.push_back(surface);
        }
        fixture.structures.push_back(scene->add(std::move(object)));
    }
    const PlayerBuilder players(&tuning, &presentation); std::vector<Player*> roster;
    for (auto& player : players.build_players(settings, &info, colours, app.render_resources(),
        app.audio_resources(), &preferences, app.viewport_manager()))
    { roster.push_back(scene->add(std::move(player))); }
    const std::string display = info.display_name();
    fixture.level = std::make_unique<Level>(std::move(scene), std::move(roster),
        objects.build_viewport_dividers(app.viewport_manager()), colours, info.camera_bounds_rectangle(),
        info.zoom_out_start_bounds_rectangle(), info.zoom_out_finish_bounds_rectangle(), &tuning, &presentation,
        std::wstring(display.begin(), display.end()), info.sound_bank_name(), info.music_name(), info.music_volume(),
        info.end_sting_name(), info.end_sting_volume(), app.resolution_manager(), app.viewport_manager(),
        app.render_resources(), app.audio_resources(), &preferences, app.mouse(), info.stations(), opening, duration);
    require(fixture.level->player_count() == count, "Crossfeed native Level changed its requested player count");
    for (int view = 0; view < count; ++view)
    { require(fixture.level->player(view).player_num() == slots[static_cast<size_t>(view)], "Crossfeed native roster changed its view/input mapping"); }
    const auto ownership = crossfeed_ownership(fixture);
    require(heavy ? ownership.second > 0.99f : ownership.second == 0.0f,
        "Crossfeed controlled paint fixture did not establish measured clean/heavy ownership");
    return fixture;
}

Frame crossfeed_frozen(Application& app, const CrossfeedFixture& fixture, size_t& readbacks)
{
    const Level& level = *fixture.level; const auto before = observation(level);
    const auto structures = crossfeed_structure_state(fixture);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    std::srand(8282026); const int expected = std::rand(); std::srand(8282026);
    ++readbacks; const Frame first = render(level, *app.renderer());
    ++readbacks; const Frame repeat = render(level, *app.renderer());
    require(first.width == 1280 && first.height == 720 && first.pixels == repeat.pixels &&
        before == observation(level) && structures == crossfeed_structure_state(fixture) && std::rand() == expected,
        "Crossfeed frozen draw changed pixels, observed player/paint/geometry state or gameplay RNG");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(),
        "Crossfeed frozen draw changed its Debug replay fingerprint");
#endif
    return first;
}

Frame crossfeed_crop(const Frame& frame, const RectangleI& box)
{
    require(box.x >= 0 && box.y >= 0 && box.width > 0 && box.height > 0 &&
        box.right() <= frame.width && box.bottom() <= frame.height, "Crossfeed native crop escaped its frame");
    Frame crop{ box.width, box.height, {} };
    for (int y = box.y; y < box.bottom(); ++y)
    {
        const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) + static_cast<size_t>(box.x)) * 4;
        crop.pixels.insert(crop.pixels.end(), frame.pixels.begin() + start,
            frame.pixels.begin() + start + static_cast<size_t>(box.width) * 4);
    }
    return crop;
}

RectangleI crossfeed_projection(const RectangleF& world, const Camera& camera, const Viewport& viewport)
{
    const RectangleI pane = viewport.pixel_rect(); RectangleF box = camera.calculate_view_rectangle(world);
    box.offset(Vector2F(pane.position()));
    const int left = std::max(pane.x, static_cast<int>(std::floor(box.x)));
    const int top = std::max(pane.y, static_cast<int>(std::floor(box.y)));
    const int right = std::min(pane.right(), static_cast<int>(std::ceil(box.right())));
    const int bottom = std::min(pane.bottom(), static_cast<int>(std::ceil(box.bottom())));
    require(right > left && bottom > top, "Crossfeed miniature has no visible native crop");
    return { left, top, right - left, bottom - top };
}

void crossfeed_matrix(Application& app, Registry<LevelLoadedInfo>& levels, const Canvas& canvas,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    app.resolution_manager()->set_resolution_exactly({ 1280, 720 });
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const LevelLoadedInfo& info = *levels.get("crossfeed");
    const RectangleF floor_a = crossfeed_rectangle(info, "floor_paint_a");
    const RectangleF side_a = crossfeed_rectangle(info, "side_paint_a");
    const RectangleF side_b = crossfeed_rectangle(info, "side_paint_b");
    const RectangleF plinth = crossfeed_rectangle(info, "centre_plinth");
    const RectangleF step_a = crossfeed_rectangle(info, "upper_step_a");
    const RectangleF step_b = crossfeed_rectangle(info, "upper_step_b");
    const RectangleF home_a = crossfeed_rectangle(info, "spawn_a_floor");
    const RectangleF home_b = crossfeed_rectangle(info, "spawn_b_floor");
    const RectangleF roof_a = crossfeed_rectangle(info, "spawn_a_roof");
    const RectangleF roof_b = crossfeed_rectangle(info, "spawn_b_roof");
    const RectangleF overview = info.zoom_out_finish_bounds_rectangle();
    const Vector2F size = tuning.player().size;
    for (const auto& team : { info.team_a_spawns(), info.team_b_spawns() })
    {
        require(team.size() == 4, "Crossfeed native inspection needs four authored spawns per team");
        for (const Vector2F& spawn : team)
        { require(overview.contains(RectangleF(spawn, size)), "Crossfeed overview clips an authored full-body spawn"); }
    }
    const float mirror_axis = overview.left() + overview.right();
    const float lower_x = floor_a.right() - 100.0f;
    const std::array<Vector2F, 4> positions{
        Vector2F(lower_x - size.x / 2.0f, floor_a.top() - size.y),
        Vector2F(mirror_axis - lower_x - size.x / 2.0f, floor_a.top() - size.y),
        Vector2F(side_a.center().x - size.x / 2.0f, side_a.top() - size.y),
        Vector2F(side_b.center().x - size.x / 2.0f, side_b.top() - size.y) };
    std::array<RectangleF, 4> targets{ plinth, plinth, step_a, step_b };
    std::ofstream records(output / "crossfeed.csv");
    records << "capture,kind,phase,paint,players,view,input_slot,player_x,player_y,velocity_x,velocity_y,"
        "camera_x,camera_y,camera_scale,pane_x,pane_y,pane_width,pane_height,crop_x,crop_y,crop_width,crop_height,"
        "owned_fraction,target_x,target_y,target_width,target_height,target_visible,frozen_pixels,public_state,"
        "debug_fingerprint,rng,evidence_scope\n";
    size_t readbacks = 0, updates = 0, input_steps = 0, builds = 0, heavy_contacts = 0, frames = 0, crops = 0, rows = 0;
    float heavy_min = 1.0f;
    const auto record = [&](const std::string& stem, const char* kind, const char* phase, bool heavy, int count,
        int view, const Player* player, const Camera& camera, const Viewport& viewport, const RectangleI& crop,
        float fraction, const RectangleF& target)
    {
        const RectangleI pane = viewport.pixel_rect();
        const Vector2F position = player != nullptr ? player->rectangle().position() : Vector2F::ZERO;
        const Vector2F velocity = player != nullptr ? player->velocity() : Vector2F::ZERO;
        const bool target_visible = camera.visible_rectangle(viewport).contains(target);
        records << stem << ',' << kind << ',' << phase << ',' << (heavy ? "heavy" : "clean") << ',' << count << ',' <<
            view + 1 << ',' << (player != nullptr ? player->player_num() : -1) << ',' << position.x << ',' << position.y << ',' <<
            velocity.x << ',' << velocity.y << ',' << camera.translation.x << ',' << camera.translation.y << ',' << camera.scale << ',' <<
            pane.x << ',' << pane.y << ',' << pane.width << ',' << pane.height << ',' << crop.x << ',' << crop.y << ',' <<
            crop.width << ',' << crop.height << ',' << fraction << ',' << target.x << ',' << target.y << ',' <<
            target.width << ',' << target.height << ',' << target_visible << ",1," << (player != nullptr ? "1" : "static-widget-bounds") << ','
#if COLOURWARS_DEV_TOOLS
            << (player != nullptr ? "1" : "not-applicable-widget")
#else
            << "not-available-in-Release"
#endif
            << ",1,hidden-native-inspection\n"; ++rows;
    };
    const auto step = [&](CrossfeedFixture& fixture, const std::vector<PlayerInputData>& inputs, bool moving)
    {
        require(++updates <= 8000, "Crossfeed native matrix exceeded its bounded Level update budget");
        if (moving) { ++input_steps; }
        fixture.level->update(inputs, dt);
    };
    const auto capture = [&](CrossfeedFixture& fixture, const std::string& stem, const char* phase, bool heavy, bool ending)
    {
        const Frame frame = crossfeed_frozen(app, fixture, readbacks);
        save(frame, output / (stem + ".ppm")); ++frames;
        const auto ownership = crossfeed_ownership(fixture);
        require(heavy ? ownership.second > 0.99f : ownership.second < 0.01f,
            "Crossfeed captured paint state differs from its measured clean/heavy label");
        const int views = ending ? 1 : fixture.level->player_count();
        for (int view = 0; view < views; ++view)
        {
            const Player& player = fixture.level->player(view);
            require(player.state() == PlayerState::alive, "Crossfeed native pane contains an unexpectedly dead inspector");
            const Viewport viewport = ending ? app.viewport_manager()->fullscreen_viewport() : app.viewport_manager()->player_viewport(view);
            const Camera camera = ending ? Camera::frame(overview, viewport) : fixture.level->view_camera(view);
            require(ending || camera.visible_rectangle(viewport).intersects(player.rectangle()),
                "Crossfeed moving inspector left its actual player camera");
            const RectangleI crop = ending ? crossfeed_projection(overview, camera, viewport) : viewport.pixel_rect();
            save(crossfeed_crop(frame, crop), output / (stem + "-view" + std::to_string(view + 1) + "-native.ppm")); ++crops;
            record(stem, ending ? "actual-Level-overview" : "actual-Level-fixture", phase, heavy, fixture.level->player_count(),
                view, &player, camera, viewport, crop, ownership.second, ending ? overview : targets[static_cast<size_t>(view)]);
        }
        return frame;
    };
    const auto neutral = neutral_inputs();
    for (int count = 1; count <= 4; ++count)
    {
        std::array<CrossfeedFixture, 2> fixtures;
        for (int paint = 0; paint < 2; ++paint)
        {
            std::srand(8282); fixtures[static_cast<size_t>(paint)] = crossfeed_fixture(app, info, tuning, presentation,
                preferences, count, paint == 1, 30.0f, heavy_contacts); ++builds;
            auto& fixture = fixtures[static_cast<size_t>(paint)];
            if (paint == 1) { heavy_min = std::min(heavy_min, crossfeed_ownership(fixture).second); }
            for (int tick = 0; tick < 600 && fixture.level->state() != LevelState::active; ++tick) { step(fixture, neutral, false); }
            require(fixture.level->state() == LevelState::active, "Crossfeed native match failed to reach active play");
            for (int tick = 0; tick < 90; ++tick) { step(fixture, neutral, false); }
            for (int view = 0; view < count; ++view)
            { require(fixture.level->place_player(view, positions[static_cast<size_t>(view)]), "Crossfeed native station placement failed"); }
        }
        for (int phase = 0; phase < 2; ++phase)
        {
            const char* phase_name = phase == 0 ? "approach" : "advance";
            for (int tick = 0; tick < (phase == 0 ? 6 : 24); ++tick)
            {
                auto inputs = neutral;
                for (int view = 0; view < count; ++view)
                {
                    auto& input = inputs[static_cast<size_t>(fixtures[0].level->player(view).player_num())];
                    input.x_movement = view % 2 == 0 ? 1.0f : -1.0f;
                    input.jump_pressed = phase == 1 && tick == 0 && view >= 2;
                    input.jump_held = phase == 1 && view >= 2;
                }
                for (auto& fixture : fixtures) { step(fixture, inputs, true); }
            }
            std::array<Frame, 2> pair;
            for (int paint = 0; paint < 2; ++paint)
            {
                const std::string stem = "hidden-inspection-crossfeed-" + std::to_string(count) + "p-" +
                    (paint == 1 ? "heavy-" : "clean-") + phase_name + "-720p";
                pair[static_cast<size_t>(paint)] = capture(fixtures[static_cast<size_t>(paint)], stem, phase_name, paint == 1, false);
            }
            for (int view = 0; view < count; ++view)
            {
                const Player& clean = fixtures[0].level->player(view); const Player& heavy = fixtures[1].level->player(view);
                require(clean.rectangle() == heavy.rectangle() && clean.velocity() == heavy.velocity(),
                    "Controlled Crossfeed heavy paint changed the paired scripted movement");
                require(pane_delta(pair[0], pair[1], app.viewport_manager()->player_viewport(view).pixel_rect()) > 0,
                    "Crossfeed controlled heavy paint supplied no visible difference in an actual player pane");
            }
        }
        if (count == 4)
        {
            // These controlled floor/roof starts expose both home choices in
            // native panes; they do not stand in for a driven spawn departure.
            const std::array<Vector2F, 4> homes{
                Vector2F(home_a.right() - 100.0f - size.x / 2.0f, home_a.top() - size.y),
                Vector2F(home_b.left() + 100.0f - size.x / 2.0f, home_b.top() - size.y),
                Vector2F(roof_a.right() - 120.0f - size.x / 2.0f, roof_a.top() - size.y),
                Vector2F(roof_b.left() + 120.0f - size.x / 2.0f, roof_b.top() - size.y) };
            targets = { crossfeed_rectangle(info, "spawn_a_step"), crossfeed_rectangle(info, "spawn_b_step"),
                crossfeed_rectangle(info, "spawn_a_baffle"), crossfeed_rectangle(info, "spawn_b_baffle") };
            for (auto& fixture : fixtures)
            {
                for (int view = 0; view < count; ++view)
                { require(fixture.level->place_player(view, homes[static_cast<size_t>(view)]), "Crossfeed home/roof station placement failed"); }
                auto inputs = neutral;
                for (int view = 0; view < count; ++view)
                { inputs[static_cast<size_t>(fixture.level->player(view).player_num())].x_movement = view % 2 == 0 ? 1.0f : -1.0f; }
                for (int tick = 0; tick < 6; ++tick) { step(fixture, inputs, true); }
            }
            for (int paint = 0; paint < 2; ++paint)
            {
                capture(fixtures[static_cast<size_t>(paint)], std::string("hidden-inspection-crossfeed-4p-") +
                    (paint == 1 ? "heavy" : "clean") + "-home-exit-720p", "home-exit", paint == 1, false);
            }
        }
    }
    for (int paint = 0; paint < 2; ++paint)
    {
        auto fixture = crossfeed_fixture(app, info, tuning, presentation, preferences, 1, paint == 1, 0.75f, heavy_contacts); ++builds;
        if (paint == 1) { heavy_min = std::min(heavy_min, crossfeed_ownership(fixture).second); }
        for (int tick = 0; tick < 600 && fixture.level->state() != LevelState::overview; ++tick) { step(fixture, neutral, false); }
        require(fixture.level->state() == LevelState::overview, "Crossfeed short solo match missed its actual runtime overview");
        capture(fixture, std::string("hidden-inspection-crossfeed-overview-") + (paint == 1 ? "heavy" : "clean") + "-720p", "overview", paint == 1, true);
    }
    {
        Save saved; GameData data; data.set_application(&app); data.set_save(&saved); data.set_level_infos(&levels);
        data.set_tuning(&tuning); data.set_presentation(&presentation);
        const auto& look = presentation.main_menu().stage_select; const RectangleF box(look.preview_position, look.preview_size);
        StagePreview preview("crossfeed-actual-stage-preview", box, &data); preview.set_stage("crossfeed");
        const Viewport fullscreen = app.viewport_manager()->fullscreen_viewport();
        const Camera menu = Camera::frame(RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION), fullscreen);
        const auto draw = [&](bool shown)
        {
            Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
            DrawList list = renderer.view(0); list.set_viewport(fullscreen); list.set_camera(menu); list.set_filter(TextureFilter::point);
            canvas.draw(list, RectangleF(Vector2F::ZERO, menu_consts::DESIGN_RESOLUTION));
            if (shown) { preview.draw(list); } ++readbacks; return read(renderer);
        };
        std::srand(883); const int expected = std::rand(); std::srand(883); const Frame frame = draw(true);
        require(frame.pixels == draw(true).pixels && std::rand() == expected && preview.bounds() == box,
            "Crossfeed actual StagePreview changed frozen pixels, bounds or gameplay RNG");
        // Match StagePreview::draw: fit the world to its fractional menu pane,
        // then use Viewport's edge truncation for the physical clipping pixels.
        RectangleF preview_pane = menu.calculate_view_rectangle(box);
        preview_pane.offset(fullscreen.position());
        const Viewport preview_viewport(preview_pane);
        const Camera preview_camera = Camera::frame(overview, preview_viewport);
        const RectangleI crop = preview_viewport.pixel_rect();
        require(pane_delta(frame, draw(false), crop) > 100, "Crossfeed actual StagePreview is blank");
        const std::string stem = "hidden-inspection-crossfeed-stage-preview-clean-720p";
        save(frame, output / (stem + ".ppm")); ++frames;
        save(crossfeed_crop(frame, crop), output / (stem + "-view1-native.ppm")); ++crops;
        record(stem, "actual-StagePreview-isolated", "miniature", false, 0, 0, nullptr,
            preview_camera, preview_viewport, crop, 0.0f, overview);
    }
    require(frames == 21 && crops == 51 && rows == 51 && readbacks == 43 && builds == 10 && records.good(),
        "Crossfeed focused native evidence exceeded or missed its fixed frame/crop/row/readback/build budget");
    std::ofstream summary(output / "crossfeed-summary.json");
    summary << "{\"outcome\":\"passed\",\"scope\":\"crossfeed-only\",\"full_frames\":21,\"native_crops\":51,"
        "\"csv_rows\":51,\"readbacks\":43,\"level_updates\":" << updates << ",\"level_builds\":10,"
        "\"heavy_initial_fraction_min\":" << heavy_min << ",\"controlled_heavy_contacts\":" << heavy_contacts <<
        ",\"scripted_input_steps\":" << input_steps << "}\n";
    std::ofstream scope(output / "crossfeed-scope.txt");
    scope << "Hidden native 1280x720 Crossfeed H6 inspection: 1/2/3/4-player clean and controlled-heavy panes.\n"
        "Public Level/Scene/PlayerBuilder construction matches the authored objects, roster, HUD and player-camera path.\n"
        "Heavy states receive controlled ProjectileSpray contacts on real paintable objects; measured ownership exceeds 99%. This does not measure weapon reach or paint efficiency.\n"
        "Actors are placed at selected authored approach stations, then receive six walking ticks and 24 advancing ticks. Lower views walk toward the centre plinth; upper views hold jump toward their ruler landing.\n"
        "An additional four-player clean/heavy pair places actors at the authored home floor and roof stations and advances six walking ticks toward the next step/baffle. This is camera context, not proof of a driven spawn departure.\n"
        "CSV target_visible records complete target-rectangle visibility under the actual camera; zero is an observation for review, not hidden by a passing raster check.\n"
        "Clean/heavy pairs preserve player movement; repeated frozen drawing preserves pixels, public and per-surface state, RNG and Debug replay fingerprints.\n"
        "Actual solo runtime clean/heavy overviews and actual clean StagePreview supplement all moving player panes. Native crops are never enlarged.\n"
        "H1-H5/H7-H8 route, pressure, weapon, recovery and balance measurements belong to separate CPU trials and attended play. No human readability, warning-time, physical-pad, listening or competitive-balance acceptance is established.\n";
    require(summary.good() && scope.good(), "Crossfeed native summary/scope write failed");
    std::cout << "PASS Crossfeed-only hidden native inspection: clean/controlled-heavy 1/2/3/4P moving approaches, actual overview/StagePreview; "
        "21 full frames/51 native crops/43 readbacks, frozen state/RNG/fingerprint; human readability and balance remain open\n";
}
