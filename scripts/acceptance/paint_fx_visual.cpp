#include "game/pch.h"
#include "engine/app/application.h"
#include "engine/collision/narrow_phase.h"
#include "game/input_slots.h"
#include "game/objects/level_builder.h"
#include "game/objects/projectile_builder.h"
#include "game/objects/structure_paintable.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));
    constexpr std::array<ProjectileType, 4> flying = {
        ProjectileType::spray, ProjectileType::jet, ProjectileType::mist, ProjectileType::ball };
    constexpr std::array<WeaponType, 4> flying_tools = {
        WeaponType::sprayer, WeaponType::sniper, WeaponType::mister, WeaponType::bucket };
    constexpr std::array<ProjectileType, 5> kinds = {
        ProjectileType::spray, ProjectileType::jet, ProjectileType::rolling,
        ProjectileType::mist, ProjectileType::ball };
    constexpr std::array<const char*, 5> names = { "spray", "jet", "rolling", "mist", "ball" };
    constexpr std::array<const char*, 4> faces = { "top", "bottom", "left", "right" };
    const RectangleF block(0, 0, 90, 50);

    struct Frame
    {
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
    };

    Frame read(Renderer& renderer)
    {
        renderer.submit();
        const Vector2F size = renderer.back_buffer_size();
        Frame result{ static_cast<int>(size.x), static_cast<int>(size.y), {} };
        renderer.read_back_buffer(result.pixels);
        require(result.pixels.size() == static_cast<size_t>(result.width) *
            static_cast<size_t>(result.height) * 4, "readback size mismatch");
        return result;
    }

    void save(const Frame& frame, const std::filesystem::path& path)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
        std::vector<unsigned char> rgb(frame.pixels.size() / 4 * 3);
        size_t target = 0;
        for (size_t index = 0; index < frame.pixels.size(); index += 4)
        {
            rgb[target++] = frame.pixels[index];
            rgb[target++] = frame.pixels[index + 1];
            rgb[target++] = frame.pixels[index + 2];
        }
        file.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
        require(file.good(), "capture write failed");
    }

    Frame crop(const Frame& frame, const RectangleI& area)
    {
        require(area.x >= 0 && area.y >= 0 && area.right() <= frame.width &&
            area.bottom() <= frame.height, "crop lies outside the back buffer");
        Frame result{ area.width, area.height, {} };
        result.pixels.reserve(static_cast<size_t>(area.width) * static_cast<size_t>(area.height) * 4);
        for (int y = area.y; y < area.bottom(); ++y)
        {
            const size_t start = (static_cast<size_t>(y) * static_cast<size_t>(frame.width) +
                static_cast<size_t>(area.x)) * 4;
            result.pixels.insert(result.pixels.end(), frame.pixels.begin() + start,
                frame.pixels.begin() + start + static_cast<size_t>(area.width) * 4);
        }
        return result;
    }

    struct Canvas : TextureObject
    {
        explicit Canvas(RenderResources* resources) :
            TextureObject("sprite_sheet_1", "pixel", resources, Colour(0.72f, 0.75f, 0.78f)) {}
        using TextureObject::draw;
    };

    Camera centred(const Vector2F& centre, const Viewport& viewport, float scale)
    {
        return Camera(centre - Vector2F(viewport.width, viewport.height) / (2 * scale), scale);
    }

    // Every matrix really submits four views to the renderer. These controlled
    // canvases retain the actual Level's camera scale, not a magnified art zoom.
    template<class Draw>
    Frame matrix(Application& app, const Canvas& canvas, const std::array<Camera, 4>& cameras,
        Draw draw)
    {
        Renderer& renderer = *app.renderer();
        renderer.begin_frame();
        renderer.set_view_count(4);
        for (int view = 0; view < 4; ++view)
        {
            const Viewport& viewport = app.viewport_manager()->player_viewport(view);
            DrawList list = renderer.view(view);
            list.set_viewport(viewport);
            list.set_camera(cameras[static_cast<size_t>(view)]);
            list.set_filter(TextureFilter::point);
            canvas.draw(list, cameras[static_cast<size_t>(view)].visible_rectangle(viewport));
            draw(view, list);
        }
        return read(renderer);
    }

    size_t check_bounds(const Frame& image, const Frame& blank, const RectangleF& world,
        const Camera& camera, const Viewport& viewport)
    {
        const RectangleI pane = viewport.pixel_rect();
        RectangleF bounds = camera.calculate_view_rectangle(world);
        bounds.x += static_cast<float>(pane.x) - 1;
        bounds.y += static_cast<float>(pane.y) - 1;
        bounds.width += 2;
        bounds.height += 2;
        size_t changed = 0;
        for (int y = pane.y; y < pane.bottom(); ++y)
        {
            for (int x = pane.x; x < pane.right(); ++x)
            {
                const size_t offset = (static_cast<size_t>(y) * static_cast<size_t>(image.width) +
                    static_cast<size_t>(x)) * 4;
                if (image.pixels[offset] == blank.pixels[offset] &&
                    image.pixels[offset + 1] == blank.pixels[offset + 1] &&
                    image.pixels[offset + 2] == blank.pixels[offset + 2]) { continue; }
                require(static_cast<float>(x) >= bounds.x && static_cast<float>(x) <= bounds.right() &&
                    static_cast<float>(y) >= bounds.y && static_cast<float>(y) <= bounds.bottom(),
                    "paint effect escaped its projected scene culling bounds");
                ++changed;
            }
        }
        require(changed > 0, "paint effect has no visible pixels at gameplay scale");
        return changed;
    }

    void crops(Application& app, const Frame& image, const std::filesystem::path& output,
        const std::string& prefix, const std::array<std::string, 4>& suffixes)
    {
        // Fixed playing-size crops preserve comparisons, with ample room for
        // the 90x50 surface and its impact at the largest gameplay scale.
        for (int view = 0; view < 4; ++view)
        {
            const RectangleI pane = app.viewport_manager()->player_viewport(view).pixel_rect();
            const RectangleI area(pane.x + pane.width / 2 - 108,
                pane.y + pane.height / 2 - 80, 216, 160);
            save(crop(image, area), output / (prefix + '-' + suffixes[static_cast<size_t>(view)] + "-crop.ppm"));
        }
    }

    using Shots = std::vector<std::unique_ptr<Projectile>>;
    using ShotMatrix = std::array<Shots, 4>;
    using Surfaces = std::array<std::unique_ptr<StructurePaintable>, 4>;

    RectangleF shot_bounds(const Shots& shots)
    {
        require(!shots.empty(), "builder returned no projectile");
        RectangleF result = shots.front()->bounds();
        for (const auto& shot : shots) { result = RectangleF::union_of(result, shot->bounds()); }
        return result;
    }

    RectangleF shot_cull_bounds(const Shots& shots, float scale)
    {
        require(!shots.empty(), "builder returned no projectile");
        RectangleF result = shots.front()->cull_bounds(1 / scale);
        for (const auto& shot : shots)
            result = RectangleF::union_of(result, shot->cull_bounds(1 / scale));
        return result;
    }

    std::vector<float> observation(const ShotMatrix& shots)
    {
        std::vector<float> result;
        for (const auto& group : shots)
        {
            for (const auto& shot : group)
            {
                const RectangleF rectangle = shot->shape()->bounding_box();
                result.insert(result.end(), { rectangle.x, rectangle.y, rectangle.width, rectangle.height,
                    static_cast<float>(shot->player_num()), shot->for_deletion() ? 1.0f : 0.0f });
            }
        }
        return result;
    }

    void shots(Application& app, const Canvas& canvas, const GameTuning& tuning,
        const TeamColour& colours, float scale, const std::filesystem::path& output,
        const std::string& prefix, std::ofstream& records)
    {
        const ProjectileBuilder builder(&tuning);
        for (const PlayerTeam team : { PlayerTeam::a, PlayerTeam::b })
        {
            for (int direction = 0; direction < 8; ++direction)
            {
                ShotMatrix groups;
                const float angle = static_cast<float>(direction) * PI / 4;
                for (size_t index = 0; index < flying.size(); ++index)
                {
                    // Match Weapon::shoot: projectile sprites receive no aim
                    // rotation; movement velocity supplies the firing direction.
                    groups[index] = builder.build_projectiles(Vector2F::ZERO,
                        Vector2F::unit_vec_from_angle(angle) *
                            tuning.weapon(flying_tools[index]).details.starting_vel_length, team, 0,
                        colours.team_colour(team), flying[index], app.render_resources());
                }
                for (int phase = 0; phase < 2; ++phase)
                {
                    if (phase == 1)
                    {
                        for (auto& group : groups)
                            for (auto& shot : group) { shot->update(0.075f); }
                    }
                    std::array<Camera, 4> cameras;
                    std::array<std::string, 4> suffixes;
                    for (int view = 0; view < 4; ++view)
                    {
                        const size_t index = static_cast<size_t>(view);
                        cameras[index] = centred(shot_bounds(groups[index]).center(),
                            app.viewport_manager()->player_viewport(view), scale);
                        suffixes[index] = names[static_cast<size_t>(flying[index])];
                    }
                    const auto draw = [&](int view, DrawList& list)
                    {
                        for (const auto& shot : groups[static_cast<size_t>(view)]) { shot->draw(list); }
                    };
                    const auto before = observation(groups);
                    const Frame blank = matrix(app, canvas, cameras, [](int, DrawList&) {});
                    const Frame image = matrix(app, canvas, cameras, draw);
                    for (int repeat = 0; repeat < 2; ++repeat)
                        require(image.pixels == matrix(app, canvas, cameras, draw).pixels,
                            "const projectile redraw changed pixels");
                    require(before == observation(groups), "projectile draw changed simulation state");
                    const std::string stem = prefix + "-shots-" + (team == PlayerTeam::a ? "a" : "b") +
                        "-dir" + std::to_string(direction) + "-frame" + std::to_string(phase);
                    for (int view = 0; view < 4; ++view)
                    {
                        const size_t index = static_cast<size_t>(view);
                        const size_t pixels = check_bounds(image, blank, shot_cull_bounds(groups[index], scale),
                            cameras[index], app.viewport_manager()->player_viewport(view));
                        records << stem << ',' << suffixes[index] << ",flying," << scale << ',' << pixels << '\n';
                    }
                    // Raster bounds are checked at all eight aim directions;
                    // retain representative horizontal/diagonal review rasters.
                    if (direction < 2) { crops(app, image, output, stem, suffixes); }
                    if (team == PlayerTeam::a && direction == 0 && phase == 0)
                        save(image, output / (prefix + "-four-view-shots.ppm"));
                }
            }
        }
    }

    Surfaces surfaces(Application& app, const GameTuning& tuning, const PlaySettings& preferences,
        const TeamColour& colours)
    {
        Surfaces result;
        constexpr std::array<PaintableFaces, 4> enabled = {{
            { false, true, false, false }, { false, false, false, true },
            { true, false, false, false }, { false, false, true, false } }};
        for (size_t view = 0; view < result.size(); ++view)
        {
            result[view] = std::make_unique<StructurePaintable>("sprite_sheet_1", "square_white_4",
                block, std::make_unique<RectangleF>(block), app.render_resources(), app.audio_resources(),
                &preferences, &tuning.paint(), CollisionObjectType::structure_paintable,
                colours, enabled[view], std::span<const RectangleF>{}, Colour(0.30f, 0.33f, 0.36f));
        }
        return result;
    }

    void contact(Application& app, const GameTuning& tuning, const TeamColour& colours,
        Surfaces& objects, ProjectileType type, PlayerTeam team, int owner, float fraction = 0.5f)
    {
        const ProjectileBuilder builder(&tuning);
        const std::array<Vector2F, 4> centres = {
            Vector2F(block.width * fraction, 0), Vector2F(block.width * fraction, 50),
            Vector2F(0, block.height * fraction), Vector2F(90, block.height * fraction) };
        for (size_t view = 0; view < objects.size(); ++view)
        {
            const auto projectiles = builder.build_projectiles(centres[view], Vector2F::ZERO,
                team, owner, colours.team_colour(team), type, app.render_resources());
            for (const auto& projectile : projectiles)
            {
                const auto hit = narrow_phase(*objects[view]->shape(), *projectile->shape());
                require(hit.has_value(), "authored projectile does not contact the selected surface face");
                objects[view]->on_contact(*projectile, hit->normal, hit->penetration);
            }
        }
    }

    void area_is(float actual, float expected)
    {
        require(std::fabs(actual - expected) < 0.001f, "surface ownership changed unexpectedly");
    }

    std::vector<float> observation(const Surfaces& objects)
    {
        std::vector<float> result;
        for (const auto& object : objects)
        {
            const PaintTotal total = object->paint_total();
            const RectangleF geometry = object->bounds();
            const RectangleF collision = object->shape()->bounding_box();
            result.insert(result.end(), { geometry.x, geometry.y, geometry.width, geometry.height,
                collision.x, collision.y, collision.width, collision.height });
            result.insert(result.end(), { total.total, total.team_a, total.team_b });
            result.insert(result.end(), total.player_areas.begin(), total.player_areas.end());
        }
        return result;
    }

    void impacts(Application& app, const Canvas& canvas, const GameTuning& tuning,
        const PlaySettings& preferences, const TeamColour& colours, float scale,
        const std::filesystem::path& output, const std::string& prefix, std::ofstream& records)
    {
        std::array<Camera, 4> cameras;
        std::array<std::string, 4> suffixes;
        for (int view = 0; view < 4; ++view)
        {
            cameras[static_cast<size_t>(view)] = centred(block.center(),
                app.viewport_manager()->player_viewport(view), scale);
            suffixes[static_cast<size_t>(view)] = faces[static_cast<size_t>(view)];
        }
        const Frame blank = matrix(app, canvas, cameras, [](int, DrawList&) {});
        for (const ProjectileType type : kinds)
        {
            auto objects = surfaces(app, tuning, preferences, colours);
            const auto draw = [&](int view, DrawList& list) { objects[static_cast<size_t>(view)]->draw(list); };
            const std::string stem = prefix + "-impact-" + names[static_cast<size_t>(type)];
            const auto capture = [&](const char* phase)
            {
                const auto before = observation(objects);
                const Frame image = matrix(app, canvas, cameras, draw);
                for (int repeat = 0; repeat < 2; ++repeat)
                    require(image.pixels == matrix(app, canvas, cameras, draw).pixels,
                        "const painted-surface redraw changed pixels");
                require(before == observation(objects), "surface draw changed paint ownership");
                for (int view = 0; view < 4; ++view)
                {
                    const size_t index = static_cast<size_t>(view);
                    const size_t pixels = check_bounds(image, blank, objects[index]->cull_bounds(1 / scale),
                        cameras[index], app.viewport_manager()->player_viewport(view));
                    records << stem << '-' << phase << ',' << faces[index] << ",surface," << scale << ',' << pixels << '\n';
                }
                crops(app, image, output, stem + '-' + phase, suffixes);
                return image;
            };
            contact(app, tuning, colours, objects, type, PlayerTeam::a, 0);
            const Frame fresh = capture("fresh-a");
            if (type == ProjectileType::rolling)
                save(fresh, output / (prefix + "-four-view-roller-contact.ppm"));
            const auto ownership = observation(objects);
            for (auto& object : objects) { object->update(0.075f); }
            capture("age");
            contact(app, tuning, colours, objects, type, PlayerTeam::a, 1);
            require(fresh.pixels == capture("friendly-a").pixels,
                "friendly repaint did not restart the same tint and impact immediately");
            require(ownership == observation(objects), "friendly repaint stole paint credit");
            contact(app, tuning, colours, objects, type, PlayerTeam::b, 1);
            const Frame enemy = capture("enemy-b");
            require(enemy.pixels != fresh.pixels, "enemy repaint retained the old team tint");
            auto control = surfaces(app, tuning, preferences, colours);
            contact(app, tuning, colours, control, type, PlayerTeam::b, 1);
            const Frame expected = matrix(app, canvas, cameras,
                [&](int view, DrawList& list) { control[static_cast<size_t>(view)]->draw(list); });
            require(enemy.pixels == expected.pixels,
                "enemy repaint tint lags behind a fresh contact of the same team");
            for (const auto& object : objects)
            {
                const PaintTotal total = object->paint_total();
                area_is(total.team_a, 0);
                require(total.team_b > 0, "surface contact did not capture paint");
                area_is(total.player_areas[1], total.team_b);
            }
            for (auto& object : objects) { object->update(2.0f); }
            require(enemy.pixels != capture("settled-b").pixels,
                "wet impact did not settle after its presentation lifetime");
        }
        // Keep a continuous friendly/enemy border in the review, rather than
        // judging isolated splats only. These are ordinary overlapping builder
        // spray contacts on the same four owned StructurePaintable objects.
        auto adjacent = surfaces(app, tuning, preferences, colours);
        for (int step = 0; step <= 10; ++step)
            contact(app, tuning, colours, adjacent, ProjectileType::spray,
                PlayerTeam::a, 0, static_cast<float>(step) / 10);
        for (auto& object : adjacent) { object->update(2.0f); }
        for (int step = 6; step <= 10; ++step)
            contact(app, tuning, colours, adjacent, ProjectileType::spray,
                PlayerTeam::b, 1, static_cast<float>(step) / 10);
        contact(app, tuning, colours, adjacent, ProjectileType::spray, PlayerTeam::a, 2, 0.35f);
        const auto draw_adjacent = [&](int view, DrawList& list)
            { adjacent[static_cast<size_t>(view)]->draw(list); };
        const Frame mixed = matrix(app, canvas, cameras, draw_adjacent);
        require(mixed.pixels == matrix(app, canvas, cameras, draw_adjacent).pixels,
            "adjacent friendly/enemy paint changed during const redraw");
        for (int view = 0; view < 4; ++view)
        {
            const size_t index = static_cast<size_t>(view);
            const PaintTotal total = adjacent[index]->paint_total();
            require(total.team_a > 0 && total.team_b > 0, "adjacent-paint fixture lost a team");
            check_bounds(mixed, blank, adjacent[index]->cull_bounds(1 / scale), cameras[index],
                app.viewport_manager()->player_viewport(view));
        }
        crops(app, mixed, output, prefix + "-impact-adjacent-friendly-enemy", suffixes);
    }

    std::unique_ptr<Level> level(Application& app, LevelBuilder& builder, TeamColourPair palette)
    {
        MenuLevelSettings settings;
        settings.set_player_count(4);
        settings.set_stage("movement_course");
        settings.set_screen_layout(ScreenLayout::four_player);
        settings.begin_input_claims(0);
        for (int view = 1; view < 4; ++view) { require(settings.claim_input_slot(view), "input claim failed"); }
        constexpr std::array<WeaponType, 4> tools = {
            WeaponType::sprayer, WeaponType::sniper, WeaponType::mister, WeaponType::bucket };
        for (int view = 0; view < 4; ++view)
        {
            settings.set_player_team(view, view % 2 == 0 ? PlayerTeam::a : PlayerTeam::b);
            settings.set_player_weapon(view, tools[static_cast<size_t>(view)]);
        }
        app.viewport_manager()->set_layout(ScreenLayout::four_player);
        auto result = builder.build_level(settings, palette, OpeningMode::off);
        require(result->player_count() == 4, "builder changed the four-player roster");
        return result;
    }
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 3, "usage: paint_fx_visual <staged-content> <output>");
        const std::filesystem::path content = std::filesystem::absolute(argv[1]);
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(output);
        std::ofstream records(output / "paint-fx.csv");
        records << "capture,kind_or_face,role,camera_scale,pixels\n";
        Registry<LevelLoadedInfo> levels("Level");
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        PlaySettings preferences;
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsPaintFxProbe";
        options.window_title = L"ColourWars paint effects validation";
        options.resolution = ScreenResolution::s_1280_720;
        options.fullscreen = false;
        options.max_threads = 4;
        options.visible = false;
        Application app(options);
        app.initialize();
        const auto& device = app.renderer()->device_info();
        std::cout << "DEVICE backend=" << device.backend << " api=" << device.api
            << " name=" << device.device_name << " vendor=" << device.vendor_id
            << " device=" << device.device_id << '\n';
        require(device.backend == "d3d11" && device.kind == RenderDeviceKind::hardware,
            "visual evidence requires a D3D11 hardware device");
        app.resource_loader()->register_kind("level", {
            [&](const std::string& directory, const std::string& name, bool) {
                auto info = std::make_unique<LevelLoadedInfo>((directory + name + ".json").c_str());
                info->validate();
                levels.add(name, std::move(info));
            }, nullptr });
        app.load_manifest((content / "manifest.json").string().c_str());
        tuning.validate_resources(*app.render_resources(), *app.audio_resources());
        presentation.validate_resources(*app.render_resources(), *app.audio_resources());
        LevelBuilder builder(app.viewport_manager(), app.render_resources(), app.audio_resources(),
            &preferences, &levels, &tuning, &presentation, app.resolution_manager(),
            app.thread_pool(), app.partitioner(), app.mouse());
        const Canvas canvas(app.render_resources());
        for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
        {
            app.resolution_manager()->set_resolution_exactly(size);
            std::ignore = app.renderer()->window_size_changed(size.x, size.y);
            for (const TeamColourPair palette : { TeamColourPair::blue_orange,
                TeamColourPair::pink_green, TeamColourPair::blue_yellow })
            {
                auto match = level(app, builder, palette);
                auto inputs = std::vector<PlayerInputData>(input_slots::count);
                for (auto& input : inputs)
                {
                    input.connected = true;
                    input.shoot_angle = -PI / 4;
                    input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
                    input.shoot_direction_requested = true;
                }
                for (int tick = 0; tick < 600 && match->state() != LevelState::active; ++tick)
                    match->update(inputs, dt);
                require(match->state() == LevelState::active, "real four-player match did not become active");
                for (int tick = 0; tick < 60; ++tick) { match->update(inputs, dt); }
                for (auto& input : inputs) { input.primary_shoot = true; }
                for (int tick = 0; tick < 5; ++tick) { match->update(inputs, dt); }
                const float scale = match->player(0).camera().scale;
                require(std::fabs(scale - static_cast<float>(size.y) / 1080) < 0.0001f,
                    "matrix no longer matches the authored gameplay camera scale");
                const std::string prefix = std::to_string(size.x) + 'x' + std::to_string(size.y) + '-' +
                    TeamColourTools::palette_key(palette);
                app.renderer()->begin_frame();
                match->draw(*app.renderer());
                save(read(*app.renderer()), output / (prefix + "-real-level-four-player.ppm"));
                const TeamColour colours = TeamColourTools::team_colours(palette, presentation.team_palette());
                shots(app, canvas, tuning, colours, scale, output, prefix, records);
                impacts(app, canvas, tuning, preferences, colours, scale, output, prefix, records);
                std::cout << "PASS " << prefix << " real 4P scene, four flying types, five impact types, "
                    "four faces, immediate friendly/enemy repaint, frozen redraw and raster bounds\n";
            }
        }
        require(records.good(), "capture record write failed");
        std::cout << "PASS playing-size matrices use real builder dimensions and all palettes; "
            "roller is a contact effect, not a flying shot; no physical-device or listening acceptance\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
