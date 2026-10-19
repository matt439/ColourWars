// Static scenery only: the primary matrix counts clean/heavy pairs, not matches.
// Expected landmark draws use literal authored rectangles and raw atlas sources,
// bypassing Visual, its cull bounds and the miniature scenery traversal.
struct LandmarkStage
{
    const char* name;
    std::array<RectangleF, 2> props;
};

const std::array<LandmarkStage, 4> landmark_stages{{
    { "close_quarters", { RectangleF(1792, 4632, 256, 320), RectangleF(3952, 4792, 256, 160) } },
    { "king_of_the_hill", { RectangleF(2376, 4356, 256, 320), RectangleF(3368, 4516, 256, 160) } },
    { "turbulence", { RectangleF(1488, 5016, 256, 320), RectangleF(4256, 5176, 256, 160) } },
    { "movement_course", { RectangleF(1088, 680, 256, 320), RectangleF(5040, 840, 256, 160) } }
}};
constexpr std::array<const char*, 2> landmark_names{ "brush_pot", "tape_roll" };

struct LandmarkWorld
{
    std::unique_ptr<Scene> scene;
    std::vector<GameObject*> scenery;
    std::array<GameObject*, 2> props{};
    float total_area = 0.0f, owned_area = 0.0f;
};

std::vector<float> landmark_world_state(const LandmarkWorld& world)
{
    std::vector<float> state;
    for (const GameObject* object : world.scenery)
    {
        const RectangleF box = object->bounds(); state.insert(state.end(), { box.x, box.y, box.width, box.height });
    }
    for (const CollisionObject* object : world.scene->collision_objects())
    {
        const RectangleF box = object->shape()->bounding_box(); state.insert(state.end(), { box.x, box.y, box.width, box.height });
        if (const auto* painted = dynamic_cast<const PaintableObject*>(object))
        {
            const PaintTotal total = painted->paint_total(); state.insert(state.end(), { total.total, total.team_a, total.team_b });
            state.insert(state.end(), total.player_areas.begin(), total.player_areas.end());
        }
    }
    return state;
}

LandmarkWorld landmark_world(Application& app, const LevelLoadedInfo& info, const LandmarkStage& stage,
    const GameTuning& tuning, PlaySettings& preferences, const TeamColour& colours, bool heavy)
{
    const LevelObjectBuilder builder(app.render_resources(), app.audio_resources(), &preferences, &tuning.paint());
    LandmarkWorld world; world.scene = std::make_unique<Scene>(app.thread_pool(), app.partitioner());
    auto objects = builder.build_non_collision_objects(info.non_collision_objects_json());
    for (size_t index = 0; index < objects.size(); ++index)
    {
        const JsonValue entry = info.non_collision_objects_json().at(index);
        for (size_t prop = 0; prop < landmark_names.size(); ++prop)
        {
            if (entry.string("name") != landmark_names[prop]) { continue; }
            const JsonValue rect = entry.object("rectangle");
            const RectangleF box(rect.number("x"), rect.number("y"), rect.number("width"), rect.number("height"));
            require(world.props[prop] == nullptr && entry.string("type") == "Visual" && entry.string("sheet_name") == "terrain" &&
                entry.string("frame_name") == landmark_names[prop] && entry.string("colour") == "WHITE" && !entry.has("drift") &&
                !entry.has("material") && box == stage.props[prop] && objects[index]->bounds() == box &&
                dynamic_cast<Visual*>(objects[index].get()) != nullptr && dynamic_cast<PaintableObject*>(objects[index].get()) == nullptr,
                "Authored landmark is no longer one regular WHITE nonpaintable native-size Visual at its independent rectangle");
            world.props[prop] = objects[index].get();
        }
        world.scenery.push_back(world.scene->add(std::move(objects[index])));
    }
    for (auto& shadow : builder.build_shadows(info.collision_objects_json())) { world.scenery.push_back(world.scene->add(std::move(shadow))); }
    size_t surfaces = 0; float total = 0.0f, owned = 0.0f;
    for (auto& object : builder.build_collision_objects(info.collision_objects_json(), colours))
    {
        if (auto* painted = dynamic_cast<StructurePaintable*>(object.get()))
        {
            if (heavy)
            {
                RectangleF splash = object->shape()->bounding_box(); splash.inflate(32.0f);
                splash.offset(tuning.projectile(ProjectileType::spray).details.col_rect_size / 2.0f);
                const PlayerTeam team = surfaces % 2 == 0 ? PlayerTeam::a : PlayerTeam::b;
                const ProjectileSpray shot(splash, Vector2F::ZERO, team, surfaces % 2 == 0 ? 3 : 1,
                    colours.team_colour(team), app.render_resources(), &tuning.projectile(ProjectileType::spray));
                painted->on_contact(shot, Vector2F::ZERO, 0.0f); painted->update(10.0f);
            }
            const PaintTotal paint = painted->paint_total(); total += paint.total; owned += paint.team_a + paint.team_b; ++surfaces;
        }
        world.scene->add(std::move(object));
    }
    world.scene->end_tick();
    world.total_area = total; world.owned_area = owned;
    if (heavy && !(total > 0.0f && owned / total > 0.99f))
    { throw std::runtime_error(std::string(stage.name) + " heavy landmark fixture owned " + std::to_string(owned) + " of " + std::to_string(total) + " paintable units; requires >99%"); }
    require(world.props[0] != nullptr && world.props[1] != nullptr && surfaces > 0 && total > 0.0f &&
        (heavy ? owned / total > 0.99f : owned == 0.0f), "Landmark clean/heavy fixture did not establish its measured real paint coverage");
    return world;
}

enum class LandmarkDraw { miniature, atlas, omitted, native };

Frame landmark_frame(Application& app, const LandmarkWorld& world, const LandmarkStage& stage,
    const std::array<RectangleI, 2>& sources, LandmarkDraw mode, size_t& rendered)
{
    ++rendered; Renderer& renderer = *app.renderer(); renderer.begin_frame();
    if (mode == LandmarkDraw::native) { world.scene->draw(renderer); return read(renderer); }
    renderer.set_view_count(world.scene->view_count());
    for (int view = 0; view < world.scene->view_count(); ++view)
    {
        const Scene::View& pane = world.scene->view(view); DrawList list = renderer.view(view); list.set_filter(TextureFilter::point);
        if (mode == LandmarkDraw::miniature)
        { draw_miniature_view(list, pane.camera, pane.viewport, world.scenery, world.scene->collision_objects()); continue; }
        list.set_viewport(pane.viewport); list.set_camera(pane.camera);
        for (const GameObject* object : world.scenery)
        {
            int prop = -1;
            for (size_t index = 0; index < world.props.size(); ++index) { if (world.props[index] == object) { prop = static_cast<int>(index); } }
            if (prop < 0) { object->draw(list); }
            else if (mode != LandmarkDraw::omitted)
            { app.render_resources()->sprite_sheet("terrain")->draw(list, sources[static_cast<size_t>(prop)], stage.props[static_cast<size_t>(prop)], Colour::white); }
        }
        // Deliberately explicit: every actual terrain/paint object occludes the
        // expected atlas scenery, including the Hill ruler crossing the pot.
        for (const CollisionObject* object : world.scene->collision_objects())
        {
            if (const auto* structure = dynamic_cast<const Structure*>(object)) { structure->draw_miniature(list, pane.camera); }
            else { object->draw(list); }
        }
    }
    return read(renderer);
}

void landmark_views(Application& app, LandmarkWorld& world, const RectangleF& bounds, int count)
{
    world.scene->clear_views();
    for (int view = 0; view < count; ++view)
    { const Viewport pane = app.viewport_manager()->player_viewport(view); world.scene->add_view(pane, Camera::frame(bounds, pane)); }
}

Frame landmark_frozen_scene(Application& app, const LandmarkWorld& world, const LandmarkStage& stage,
    const std::array<RectangleI, 2>& sources, size_t& rendered)
{
    const auto before = landmark_world_state(world); std::srand(9721); const int expected = std::rand(); std::srand(9721);
    const Frame frame = landmark_frame(app, world, stage, sources, LandmarkDraw::miniature, rendered);
    require(frame.pixels == landmark_frame(app, world, stage, sources, LandmarkDraw::miniature, rendered).pixels &&
        before == landmark_world_state(world) && std::rand() == expected, "Frozen landmark Scene draw changed pixels, geometry, paint ownership or RNG");
    return frame;
}

void landmark_edges(Application& app, const LandmarkStage& stage, const std::array<RectangleI, 2>& sources,
    PlaySettings& preferences, size_t& rendered, size_t& edge_cases)
{
    for (size_t prop = 0; prop < sources.size(); ++prop)
    {
        LandmarkWorld isolated; isolated.scene = std::make_unique<Scene>(app.thread_pool(), app.partitioner());
        isolated.props[prop] = isolated.scene->add(std::make_unique<Visual>("terrain", landmark_names[prop], stage.props[prop], app.render_resources()));
        isolated.scenery.push_back(isolated.props[prop]); isolated.scene->end_tick();
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080) })
        {
            app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            const Viewport pane = app.viewport_manager()->fullscreen_viewport(); const float scale = static_cast<float>(size.y) / 1080.0f;
            for (int edge = 0; edge < 4; ++edge)
            {
                Vector2F position = stage.props[prop].center() - pane.size() / (2.0f * scale);
                if (edge == 0) { position.x = stage.props[prop].center().x + 0.25f; }
                if (edge == 1) { position.x = stage.props[prop].center().x - pane.width / scale - 0.25f; }
                if (edge == 2) { position.y = stage.props[prop].center().y + 0.25f; }
                if (edge == 3) { position.y = stage.props[prop].center().y - pane.height / scale - 0.25f; }
                isolated.scene->clear_views(); isolated.scene->add_view(pane, Camera(position, scale));
                const Frame native = landmark_frame(app, isolated, stage, sources, LandmarkDraw::native, rendered);
                const Frame expected = landmark_frame(app, isolated, stage, sources, LandmarkDraw::atlas, rendered);
                const Frame blank = landmark_frame(app, isolated, stage, sources, LandmarkDraw::omitted, rendered);
                require(native.pixels == expected.pixels && miniature_changed_pixels(native, blank) > 0 &&
                    landmark_frame(app, isolated, stage, sources, LandmarkDraw::miniature, rendered).pixels == native.pixels,
                    "Static landmark disappeared at a viewport edge or differed from unculled raw-atlas/native Scene pixels");
                preferences.reduce_effects = true; isolated.scene->update(1.0f);
                require(landmark_frame(app, isolated, stage, sources, LandmarkDraw::native, rendered).pixels == native.pixels,
                    "Static unpaintable landmark changed silhouette after update/Reduce effects"); preferences.reduce_effects = false;
                ++edge_cases;
            }
        }
    }
}

std::filesystem::path landmark_omission_copy(const std::filesystem::path& content, const std::filesystem::path& output,
    const LandmarkStage& stage)
{
    const auto source = content / "levels" / (std::string(stage.name) + ".json"); std::ifstream input(source, std::ios::binary);
    require(input.good(), "Cannot read the shipped landmark stage");
    const std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    rapidjson::Document original; original.Parse(bytes.c_str()); require(!original.HasParseError(), "Cannot parse landmark omission source");
    rapidjson::Document omitted; omitted.CopyFrom(original, omitted.GetAllocator());
    auto& decorations = omitted["non_collision_objects"]; size_t removed = 0;
    for (rapidjson::SizeType index = decorations.Size(); index > 0; --index)
    {
        const auto& item = decorations[index - 1]; const std::string name = item["name"].GetString();
        if (name == landmark_names[0] || name == landmark_names[1]) { decorations.Erase(decorations.Begin() + index - 1); ++removed; }
    }
    require(removed == 2, "Landmark omission copy did not remove exactly its two props");
    rapidjson::Document restored; restored.CopyFrom(omitted, restored.GetAllocator());
    rapidjson::Value restored_objects(rapidjson::kArrayType); rapidjson::SizeType retained = 0;
    for (const auto& item : original["non_collision_objects"].GetArray())
    {
        const std::string name = item["name"].GetString(); const bool prop = name == landmark_names[0] || name == landmark_names[1];
        rapidjson::Value copy(prop ? item : omitted["non_collision_objects"][retained++], restored.GetAllocator());
        restored_objects.PushBack(copy, restored.GetAllocator());
    }
    restored["non_collision_objects"] = std::move(restored_objects);
    require(restored == original, "Landmark omission changed fields beyond the two scenery entries");
    rapidjson::StringBuffer serialized; rapidjson::Writer<rapidjson::StringBuffer> writer(serialized); omitted.Accept(writer);
    const auto path = output / "landmark-omission" / (std::string(stage.name) + ".json"); std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary); file.write(serialized.GetString(), static_cast<std::streamsize>(serialized.GetSize()));
    require(file.good(), "Cannot retain the private landmark omission proof"); return path;
}

Frame landmark_frozen_level(Application& app, Level& level, size_t& rendered)
{
    const auto before = observation(level);
#if COLOURWARS_DEV_TOOLS
    const MatchFingerprint fingerprint = MatchFingerprint::of(level, 0);
#endif
    std::srand(8029); const int expected = std::rand(); std::srand(8029);
    ++rendered; const Frame frame = render(level, *app.renderer()); ++rendered;
    require(frame.pixels == render(level, *app.renderer()).pixels && before == observation(level) && std::rand() == expected,
        "Frozen actual landmark Level changed pixels, public state or RNG");
#if COLOURWARS_DEV_TOOLS
    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(level, 0)).empty(), "Landmark drawing changed the Debug replay fingerprint");
#endif
    return frame;
}

void landmark_integrations(Application& app, Registry<LevelLoadedInfo>& levels, LevelBuilder& builder,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& content, const std::filesystem::path& output, std::ofstream& extra,
    size_t& rendered, size_t& previews, size_t& stories, int& ticks)
{
    const NestedSaveProfile profile(output / "landmark-story-profile"); Registry<LevelLoadedInfo> omitted("Landmark omission Level");
    for (const auto& stage : landmark_stages)
    { const auto path = landmark_omission_copy(content, output, stage); auto info = std::make_unique<LevelLoadedInfo>(path.string().c_str()); info->validate(); omitted.add(stage.name, std::move(info)); }
    LevelBuilder without(app.viewport_manager(), app.render_resources(), app.audio_resources(), &preferences, &omitted, &tuning,
        &presentation, app.resolution_manager(), app.thread_pool(), app.partitioner(), app.mouse());
    Save saved; PlaySettings preview_preferences = saved.play_settings(); preview_preferences.reduce_effects = false;
    preview_preferences.music_volume = preview_preferences.sfx_volume = 0;
    require(saved.set_play_settings_and_save(preview_preferences), "Cannot isolate actual StagePreview preferences");
    GameData data; data.set_application(&app); data.set_save(&saved); data.set_level_infos(&levels); data.set_tuning(&tuning); data.set_presentation(&presentation);
    for (const auto& stage : landmark_stages)
    {
        const auto* info = levels.get(stage.name); const TeamColour colours{};
        auto world = landmark_world(app, *info, stage, tuning, preferences, colours, false);
        const SpriteSheet* sheet = app.render_resources()->sprite_sheet("terrain");
        const std::array<RectangleI, 2> sources{ sheet->sprite_frame(sheet->resolve_sprite_frame(landmark_names[0])).source_rectangle(),
            sheet->sprite_frame(sheet->resolve_sprite_frame(landmark_names[1])).source_rectangle() };
        StagePreview preview("landmark-actual-preview", { 960, 270, 960, 540 }, &data); preview.set_stage(stage.name);
        Frame first_preview; int preview_sample = 0;
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(1280, 720) })
        {
            app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            const Viewport pane(static_cast<float>(size.x / 2), static_cast<float>(size.y / 4), static_cast<float>(size.x / 2), static_cast<float>(size.y / 2));
            world.scene->clear_views(); world.scene->add_view(pane, Camera::frame(info->zoom_out_finish_bounds_rectangle(), pane));
            const Frame expected = landmark_frame(app, world, stage, sources, LandmarkDraw::atlas, rendered);
            const auto draw_preview = [&]
            {
                ++rendered; Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1); DrawList list = renderer.view(0);
                list.set_viewport(app.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point); preview.draw(list); return read(renderer);
            };
            std::srand(5021); const int expected_random = std::rand(); std::srand(5021); const Frame actual = draw_preview();
            require(actual.pixels == expected.pixels && actual.pixels == draw_preview().pixels && std::rand() == expected_random,
                "Actual StagePreview lost a landmark, its captured miniature placement, frozen pixels or RNG");
            if (first_preview.pixels.empty()) { first_preview = actual; }
            else if (size.x == 1280) { require(actual.pixels == first_preview.pixels, "Landmark StagePreview resize return changed captured pixels"); }
            if (preview_sample++ < 2)
            { ++previews; extra << "stage-preview," << stage.name << ",1," << size.x << ',' << size.y << ",atlas-parity\n"; }
        }
        for (const int count : { 1, 4 })
        {
            MenuLevelSettings settings; settings.set_stage(stage.name); settings.set_player_count(count); settings.set_duration(0.75f);
            settings.set_screen_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player); settings.begin_input_claims(3);
            for (int view = 1; view < count; ++view) { require(settings.claim_input_slot(view - 1), "Landmark story sparse claim failed"); }
            for (int view = 0; view < count; ++view) { settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b); settings.set_player_weapon(view, WeaponType::roller); }
            app.viewport_manager()->set_layout(settings.screen_layout());
            std::srand(6203); auto actual = builder.build_level(settings, TeamColourPair::blue_orange, count == 1 ? OpeningMode::off : OpeningMode::full);
            std::srand(6203); auto missing = without.build_level(settings, TeamColourPair::blue_orange, count == 1 ? OpeningMode::off : OpeningMode::full);
            const auto inputs = neutral_inputs();
            const auto step = [&]
            {
                require(++ticks <= 16000, "Landmark story exceeded its hard update bound");
                std::srand(6203 + ticks); actual->update(inputs, dt); std::srand(6203 + ticks); missing->update(inputs, dt);
            };
            const auto sample = [&](const char* phase)
            {
                Frame first; int story_sample = 0;
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(1280, 720) })
                {
                    resize(app, *actual, size); missing->refresh_layout(); const Frame frame = landmark_frozen_level(app, *actual, rendered);
                    const Frame absent = landmark_frozen_level(app, *missing, rendered);
                    require(observation(*actual) == observation(*missing) && miniature_changed_pixels(frame, absent) > 0,
                        "Actual monitor/overview omitted both landmarks or scenery changed gameplay state");
                    if (first.pixels.empty()) { first = frame; }
                    else if (size.x == 1280) { require(frame.pixels == first.pixels, "Actual landmark Level changed after resize return"); }
                    if (story_sample++ < 2) { ++stories; extra << "level-" << phase << ',' << stage.name << ',' << count << ',' << size.x << ',' << size.y << ',' << miniature_changed_pixels(frame, absent) << '\n'; }
                }
            };
            if (count == 4)
            {
                const int preview_ticks = static_cast<int>(std::ceil((tuning.match().opening.settle + tuning.match().opening.preview * 0.5f) / dt));
                for (int tick = 0; tick < preview_ticks; ++tick) { step(); }
                require(actual->state() == LevelState::opening, "Landmark story missed actual monitor preview"); sample("monitor-preview");
            }
            else { require(!actual->intro_playing(), "Solo landmark practice unexpectedly acquired a monitor"); }
            for (int limit = 0; limit < 2400 && actual->state() != LevelState::overview; ++limit) { step(); }
            require(actual->state() == LevelState::overview && missing->state() == LevelState::overview, "Landmark story did not reach actual overview"); sample("overview");
        }
    }
}

void landmark_matrix(Application& app, Registry<LevelLoadedInfo>& levels, LevelBuilder& builder,
    const GameTuning& tuning, const Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& content, const std::filesystem::path& output)
{
    const RestoreFlag restore{ preferences.reduce_effects, preferences.reduce_effects }; preferences.reduce_effects = false;
    const SpriteSheet* sheet = app.render_resources()->sprite_sheet("terrain"); std::array<RectangleI, 2> sources;
    for (size_t prop = 0; prop < sources.size(); ++prop)
    {
        const SpriteFrame& frame = sheet->sprite_frame(sheet->resolve_sprite_frame(landmark_names[prop])); sources[prop] = frame.source_rectangle();
        require(frame.origin() == Vector2F::ZERO && sources[prop].width == 256 && sources[prop].height == (prop == 0 ? 320 : 160), "Landmark atlas changed native frame size/origin");
    }
    std::ofstream records(output / "landmarks.csv"), extra(output / "landmark-integrations.csv");
    records << "stage,palette,panes,width,height,framing,heavy_owned_fraction,clean_visible_pot,clean_visible_tape,heavy_visible_pot,heavy_visible_tape,paint_changed_pixels,atlas_and_cull_match,frozen_state_rng\n";
    extra << "kind,stage,panes,width,height,observed\n";
    size_t pairs = 0, primary_states = 0, rendered = 0, edges = 0, previews = 0, stories = 0, crops = 0; int ticks = 0;
    for (const auto& stage : landmark_stages)
    {
        const LevelLoadedInfo* info = levels.get(stage.name);
        for (const TeamColourPair palette : { TeamColourPair::blue_orange, TeamColourPair::blue_yellow, TeamColourPair::pink_green })
        {
            const TeamColour colours = presentation.team_palette().fixed_pair(palette).colours;
            auto clean = landmark_world(app, *info, stage, tuning, preferences, colours, false);
            auto heavy = landmark_world(app, *info, stage, tuning, preferences, colours, true);
            for (const auto& prop : stage.props) { require(info->zoom_out_finish_bounds_rectangle().contains(prop), "Shipped overview clips an authored landmark rectangle"); }
            for (const int count : { 1, 4 })
            {
                app.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
                std::array<Frame, 2> first;
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(1280, 720) })
                {
                    app.resolution_manager()->set_resolution_exactly(size); std::ignore = app.renderer()->window_size_changed(size.x, size.y);
                    landmark_views(app, clean, info->zoom_out_finish_bounds_rectangle(), count); landmark_views(app, heavy, info->zoom_out_finish_bounds_rectangle(), count);
                    std::array<Frame, 2> frames; std::array<size_t, 4> visible{};
                    for (size_t coverage = 0; coverage < 2; ++coverage)
                    {
                        auto& world = coverage == 0 ? clean : heavy; frames[coverage] = landmark_frozen_scene(app, world, stage, sources, rendered);
                        require(frames[coverage].pixels == landmark_frame(app, world, stage, sources, LandmarkDraw::atlas, rendered).pixels,
                            "Landmark miniature scenery/terrain order or culling differs from the independent raw atlas oracle");
                        const Frame absent = landmark_frame(app, world, stage, sources, LandmarkDraw::omitted, rendered);
                        for (size_t prop = 0; prop < sources.size(); ++prop)
                        {
                            RectangleF box = stage.props[prop]; world.scene->view(0).camera.calculate_view_rectangle(box); box.offset(world.scene->view(0).viewport.position());
                            const RectangleI crop = countdown_crop_bounds(box); visible[coverage * 2 + prop] = miniature_changed_pixels(foreground_crop(frames[coverage], crop, 0), foreground_crop(absent, crop, 0));
                            require(visible[coverage * 2 + prop] > 0, "An authored landmark has no surviving pixels in a clean/heavy overview pane");
                        }
                    }
                    if (first[0].pixels.empty()) { first = frames; }
                    else if (size.x == 1280)
                    { require(first[0].pixels == frames[0].pixels && first[1].pixels == frames[1].pixels, "Clean/heavy landmark Scene changed after resize return"); continue; }
                    const size_t painted = miniature_changed_pixels(frames[0], frames[1]); require(painted > 0, "Heavy ownership did not visibly paint actual terrain");
                    ++pairs; primary_states += 2;
                    records << stage.name << ',' << static_cast<int>(palette) << ',' << count << ',' << size.x << ',' << size.y << ",shipped-overview," << heavy.owned_area / heavy.total_area << ',' << visible[0] << ',' << visible[1] << ',' << visible[2] << ',' << visible[3] << ',' << painted << ",true,true\n";
                }
            }
            if (palette == TeamColourPair::blue_orange)
            {
                app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({ 1280, 720 }); std::ignore = app.renderer()->window_size_changed(1280, 720);
                const Viewport pane = app.viewport_manager()->fullscreen_viewport(); const float scale = 720.0f / presentation.camera().reference_height;
                const size_t prop = std::string(stage.name) == "movement_course" ? 1 : 0;
                const Camera camera(stage.props[prop].center() - pane.size() / (2.0f * scale), scale);
                clean.scene->clear_views(); clean.scene->add_view(pane, camera); heavy.scene->clear_views(); heavy.scene->add_view(pane, camera);
                RectangleF area = stage.props[prop]; area.inflate(40.0f); camera.calculate_view_rectangle(area); const RectangleI crop = countdown_crop_bounds(area);
                for (size_t coverage = 0; coverage < 2; ++coverage)
                {
                    const auto& world = coverage == 0 ? clean : heavy; const Frame frame = landmark_frame(app, world, stage, sources, LandmarkDraw::native, rendered);
                    require(frame.pixels == landmark_frame(app, world, stage, sources, LandmarkDraw::miniature, rendered).pixels &&
                        frame.pixels == landmark_frame(app, world, stage, sources, LandmarkDraw::atlas, rendered).pixels, "Native landmark Scene pixels/order differ from direct atlas/miniature traversal");
                    save(foreground_crop(frame, crop, 0), output / ("landmark-" + std::string(stage.name) + (coverage == 0 ? "-clean-detail.ppm" : "-heavy-detail.ppm"))); ++crops;
                }
            }
        }
        landmark_edges(app, stage, sources, preferences, rendered, edges);
        extra << "native-edge-cases," << stage.name << ",1,720/1080,four-edges-two-props,16\n";
    }
    landmark_integrations(app, levels, builder, tuning, presentation, preferences, content, output, extra, rendered, previews, stories, ticks);
    require(pairs == 48 && primary_states == 96 && edges == 64 && previews == 8 && stories == 24 && crops == 8 && rendered == 1100,
        "Landmark matrix checked counts differ from their explicit primary/supplementary axes");
    std::ofstream counts(output / "landmark-counts.csv");
    counts << "primary_pairs,primary_states,native_edge_cases,stage_preview_size_cases,level_story_size_cases,total_readbacks,paired_story_ticks,crops\n" <<
        pairs << ',' << primary_states << ',' << edges << ',' << previews << ',' << stories << ',' << rendered << ',' << ticks << ',' << crops << '\n';
    require(records.good() && extra.good() && counts.good(), "Landmark evidence CSV write failed");
    std::cout << "PASS landmarks: " << pairs << " clean/heavy paired static Scene comparisons / " << primary_states <<
        " primary states, all4 arenas x3 palettes x1P/4P pane framing x720/1080; measured>99% real paint ownership, positive per-prop pixels, raw-atlas/order/cull parity, frozen geometry/ownership/RNG and resize. " <<
        edges << " native clipped-edge cases, " << previews << " actual StagePreview size cases, " << stories << " actual Level monitor/overview size cases, " <<
        rendered << " total renderer readbacks including references/repeats; " << ticks << " paired story ticks, " << crops <<
        " compact crops. Solo monitor bypass preserved; static Visual Reduce effects invariance and Debug frozen fingerprints checked. No route, human readability, physical-device or performance acceptance.\n";
}
