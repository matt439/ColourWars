#include "game/pch.h"
#include "engine/app/application.h"
#include "engine/app/content_root.h"
#include "engine/assets/asset_manifest_loader.h"
#include "engine/assets/json.h"
#include "game/objects/interface_gameplay.h"
#include "game/objects/player_camera.h"
#include "game/objects/team_colour.h"
#include "game/play_settings.h"
#include "game/tuning.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

#include "game/objects/weapon.h"
#include "game/objects/weapon_builder.h"

using namespace labrador;
using namespace mattmath;

namespace
{
    constexpr float texel_world_size = 4.0f;
    constexpr float pi = 3.14159265358979323846f;

    // A derived observer may form pointers to the inherited protected members;
    // those member pointers retain Weapon as their declaring type and operate
    // on the actual builder-created object, including the final roller. This
    // avoids changing access specifiers (part of MSVC's symbol decoration) or
    // adding inspection APIs to the game for an art-only probe.
    class WeaponReadback : public Weapon
    {
    public:
        static float angle(const Weapon& weapon)
        {
            return (weapon.*&WeaponReadback::rotation)();
        }
        static bool flipped(const Weapon& weapon)
        {
            return (weapon.*&WeaponReadback::invert_y)();
        }
        static Vector2F position(const Weapon& weapon)
        {
            return (weapon.*&WeaponReadback::draw_pos)();
        }
        static Vector2F nozzle(const Weapon& weapon)
        {
            return (weapon.*&WeaponReadback::nozzle_pos)();
        }
    };

    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    struct Frame
    {
        int width = 0;
        int height = 0;
        std::vector<unsigned char> pixels;
    };

    void save(const Frame& frame, const std::filesystem::path& path)
    {
        std::ofstream file(path, std::ios::binary);
        file << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
        for (size_t pixel = 0; pixel < frame.pixels.size(); pixel += 4)
        {
            file.write(reinterpret_cast<const char*>(frame.pixels.data() + pixel), 3);
        }
        require(file.good(), "capture write failed");
    }

    Vector2F vector_of(const JsonValue& value)
    {
        return { value.number("x"), value.number("y") };
    }

    struct Sprite
    {
        RectangleI base;
        std::optional<RectangleI> team;
        Vector2F pivot;
        Vector2F size;
        std::optional<Vector2F> anchor;
    };

    class Kit
    {
    public:
        Kit(const SpriteSheet* sheet, const std::filesystem::path& path) : sheet_(sheet)
        {
            const JsonDocument document = read_json_file(path.string().c_str());
            const JsonValue sprites = document.root().array("sprites");
            for (size_t index = 0; index < sprites.size(); ++index)
            {
                const JsonValue row = sprites.at(index);
                const std::string name = row.string("name");
                const JsonValue frames = row.object("frames");
                Sprite sprite;
                sprite.base = sheet_->sprite_frame(sheet_->resolve_sprite_frame(
                    frames.object("base").string("name"))).source_rectangle();
                const JsonValue native = row.object("native_size");
                require(native.number("w") == static_cast<float>(sprite.base.width) &&
                    native.number("h") == static_cast<float>(sprite.base.height),
                    "kit layout and loaded frame dimensions disagree");
                sprite.size = { static_cast<float>(sprite.base.width) * texel_world_size,
                    static_cast<float>(sprite.base.height) * texel_world_size };
                const JsonValue world = row.object("world_size");
                require(world.number("w") == sprite.size.x && world.number("h") == sprite.size.y,
                    "kit layout is not four world units per native texel");
                sprite.pivot = vector_of(row.object("pivot"));
                if (name == "painter" || name == "sprayer" || name == "roller")
                {
                    sprite.team = sheet_->sprite_frame(sheet_->resolve_sprite_frame(
                        frames.object("team").string("name"))).source_rectangle();
                    require(sprite.team->width == sprite.base.width &&
                        sprite.team->height == sprite.base.height,
                        "team overlay and base dimensions disagree");
                }
                if (name == "sprayer" || name == "roller")
                {
                    sprite.anchor = vector_of(row.object("anchors").object(
                        name == "sprayer" ? "muzzle" : "contact"));
                }
                require(sprites_.emplace(name, sprite).second, "duplicate kit sprite");
            }
            for (const char* name : { "painter", "sprayer", "roller", "platform",
                "wall", "shelf", "tank", "puff", "pixel" })
            {
                std::ignore = sprites_.at(name);
            }
        }

        const Sprite& sprite(const std::string& name) const { return sprites_.at(name); }

        void draw(DrawList& list, const std::string& name, const Vector2F& position,
            const Colour& tint = Colour::white, float rotation = 0.0f,
            SpriteFlip flip = SpriteFlip::none, bool overlay = false) const
        {
            const Sprite& item = sprite(name);
            const Vector2F origin = item.anchor.has_value()
                ? flipped(item, item.pivot, flip) : item.pivot;
            const RectangleI& source = overlay ? item.team.value() : item.base;
            // The source overload intentionally supplies the metadata pivot once;
            // SpriteSheet's named-frame overload adds a frame origin of its own.
            sheet_->draw(list, source, RectangleF(position, item.size), tint,
                rotation, origin, flip);
        }

        void fill(DrawList& list, const RectangleF& rectangle, const Colour& tint) const
        {
            sheet_->draw(list, sprite("pixel").base, rectangle, tint);
        }

        static Vector2F flipped(const Sprite& item, Vector2F point, SpriteFlip flip)
        {
            if (flip == SpriteFlip::horizontal || flip == SpriteFlip::both)
            {
                point.x = static_cast<float>(item.base.width) - point.x;
            }
            if (flip == SpriteFlip::vertical || flip == SpriteFlip::both)
            {
                point.y = static_cast<float>(item.base.height) - point.y;
            }
            return point;
        }

    private:
        const SpriteSheet* sheet_;
        std::map<std::string, Sprite> sprites_;
    };

    struct Mannequin
    {
        Vector2F center;
        bool facing_right;
        PlayerTeam team;
        WeaponType type;
        std::unique_ptr<Weapon> weapon;
    };

    class Study
    {
    public:
        Study(Application& app, const GameTuning& tuning, const Presentation& presentation,
            const Kit& kit, const PlaySettings& preferences, TeamColourPair palette) :
            app_(app), tuning_(tuning), presentation_(presentation), kit_(kit),
            colours_(TeamColourTools::team_colours(palette)),
            hud_(app.render_resources(), &presentation.hud())
        {
            require(kit.sprite("painter").size == tuning.player().size,
                "kit body dimensions differ from the actual player rectangle");
            for (int index = 0; index < 4; ++index)
            {
                const bool right = index == 0 || index == 3;
                const WeaponType type = index % 2 == 0 ? WeaponType::sprayer : WeaponType::roller;
                const PlayerTeam team = index < 2 ? PlayerTeam::a : PlayerTeam::b;
                const Vector2F center(1712.0f + static_cast<float>(index) * 208.0f, 940.0f);
                people_.push_back({ center, right, team, type,
                    WeaponBuilder::build_weapon(&tuning, team, index, index,
                        colours_.team_colour(team), type, center, app.render_resources(),
                        app.audio_resources(), &preferences) });
            }
            label_font_ = app.render_resources()->resolve_sprite_font("courier_new_bold_16");
            pose(0, false);
        }

        void pose(int tick, bool animate)
        {
            for (size_t index = 0; index < people_.size(); ++index)
            {
                Mannequin& person = people_[index];
                person.center.x = 1712.0f + static_cast<float>(index) * 208.0f +
                    (animate ? static_cast<float>(tick) * 0.5f : 0.0f);
                PlayerInputData input;
                input.connected = true;
                input.shoot_direction_requested = true;
                const float sweep = animate ? static_cast<float>(tick) * (2.0f * pi / 90.0f) : 0.0f;
                input.shoot_angle = std::remainder((person.facing_right ? 0.0f : pi) + sweep,
                    2.0f * pi);
                input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
                // Zero elapsed time and no firing: use the real tool positioning
                // workflow without a simulated shot, ammunition loss or audio cue.
                const auto projectiles = person.weapon->update_and_get_projectiles(input,
                    person.center, Vector2F::ZERO, person.facing_right, 0.0f);
                require(projectiles.empty() && person.weapon->ammo() == tuning_.starting_ammo(),
                    "art positioning unexpectedly produced gameplay work");
            }
        }

        void layout(int count, const Vector2I& resolution, bool moving_start = false)
        {
            count_ = count;
            app_.resolution_manager()->set_resolution_exactly(resolution);
            std::ignore = app_.renderer()->window_size_changed(resolution.x, resolution.y);
            app_.viewport_manager()->set_layout(count == 1 ? ScreenLayout::one_player : ScreenLayout::four_player);
            cameras_.clear();
            for (int view = 0; view < count; ++view)
            {
                const Viewport viewport = app_.viewport_manager()->player_viewport(view);
                Camera previous;
                previous.scale = static_cast<float>(resolution.y) / presentation_.camera().reference_height;
                const Vector2F extent = previous.visible_rectangle(viewport).size();
                previous.translation = focus(view) - extent / 2.0f;
                if (moving_start) { previous.translation.x = -4096.0f; }
                cameras_.push_back(camera_.follow(focus(view), viewport,
                    static_cast<float>(resolution.y), presentation_.camera(), previous, bounds_));
                const Vector2F actual = cameras_.back().visible_rectangle(viewport).size();
                const Vector2F expected = count == 1 ? Vector2F(1920, 1080) : Vector2F(960, 540);
                require((actual - expected).length() < 0.01f,
                    "real PlayerCamera framing differs from the retained style-study canvas");
            }
        }

        void follow()
        {
            for (int view = 0; view < count_; ++view)
            {
                auto& previous = cameras_[static_cast<size_t>(view)];
                previous = camera_.follow(focus(view), app_.viewport_manager()->player_viewport(view),
                    app_.resolution_manager()->resolution_vec().y, presentation_.camera(), previous, bounds_);
            }
        }

        Frame render(bool painted)
        {
            Renderer& renderer = *app_.renderer();
            renderer.begin_frame();
            renderer.set_view_count(count_);
            for (int view = 0; view < count_; ++view)
            {
                DrawList list = renderer.view(view);
                const Viewport viewport = app_.viewport_manager()->player_viewport(view);
                list.set_viewport(viewport);
                list.set_camera(cameras_[static_cast<size_t>(view)]);
                list.set_filter(TextureFilter::point);
                environment(list, painted);
                for (const Mannequin& person : people_)
                {
                    const Colour team = colours_.team_colour(person.team);
                    const SpriteFlip body_flip = person.facing_right ? SpriteFlip::none : SpriteFlip::horizontal;
                    const Vector2F body = person.center;
                    kit_.draw(list, "painter", body, Colour::white, 0.0f, body_flip);
                    kit_.draw(list, "painter", body, team, 0.0f, body_flip, true);
                    const Weapon& weapon = *person.weapon;
                    const char* name = person.type == WeaponType::sprayer ? "sprayer" : "roller";
                    const SpriteFlip flip = WeaponReadback::flipped(weapon) ? SpriteFlip::vertical : SpriteFlip::none;
                    kit_.draw(list, name, WeaponReadback::position(weapon), Colour::white, WeaponReadback::angle(weapon), flip);
                    // Painted sleeve is an art-state comparison, not a claim of
                    // firing. The real roller is neutral when it is idle.
                    kit_.draw(list, name, WeaponReadback::position(weapon),
                        person.type == WeaponType::roller && !painted ? Colour::white : team,
                        WeaponReadback::angle(weapon), flip, true);
                }
                // A fixed contact sprite for scale/material review, not an event.
                kit_.draw(list, "puff", { 2000.0f, 1000.0f });
                list.set_camera(Camera::DEFAULT_CAMERA);
                const Mannequin& owner = people_[static_cast<size_t>(view)];
                hud_.draw_gameplay_interface(list, viewport.size(), view, view, owner.team,
                    0.75f, 0.625f, 150.0f, colours_.team_colour(owner.team));
                list.draw_text(label_font_, L"ART STUDY - STATIC MANNEQUINS", { 12.0f, viewport.height - 24.0f },
                    Colour::white, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
                // Match the game's pane divider placement, without another view.
                list.set_camera(Camera(viewport));
                for (const RectangleF& divider : app_.viewport_manager()->viewport_dividers())
                {
                    kit_.fill(list, divider, Colour::black);
                }
            }
            renderer.submit();
            const Vector2F size = renderer.back_buffer_size();
            require(size == app_.resolution_manager()->resolution_vec(),
                "rendered back buffer differs from the requested study resolution");
            Frame frame{ static_cast<int>(size.x), static_cast<int>(size.y), {} };
            renderer.read_back_buffer(frame.pixels);
            require(frame.pixels.size() == static_cast<size_t>(frame.width) *
                static_cast<size_t>(frame.height) * 4, "readback size mismatch");
            return frame;
        }

        void geometry(std::ostream& output, const std::string& capture, int tick) const
        {
            for (int view = 0; view < count_; ++view)
            {
                const Camera& camera = cameras_[static_cast<size_t>(view)];
                const Viewport viewport = app_.viewport_manager()->player_viewport(view);
                const RectangleF extent = camera.visible_rectangle(viewport);
                for (size_t index = 0; index < people_.size(); ++index)
                {
                    const Mannequin& person = people_[index];
                    const Weapon& weapon = *person.weapon;
                    const char* name = person.type == WeaponType::sprayer ? "sprayer" : "roller";
                    const Sprite& sprite = kit_.sprite(name);
                    const SpriteFlip flip = WeaponReadback::flipped(weapon) ? SpriteFlip::vertical : SpriteFlip::none;
                    const Vector2F pivot = Kit::flipped(sprite, sprite.pivot, flip);
                    const Vector2F anchor = Kit::flipped(sprite, sprite.anchor.value(), flip);
                    const Vector2F authored = WeaponReadback::position(weapon) + Vector2F::rotate_vector(
                        (anchor - pivot) * texel_world_size, WeaponReadback::angle(weapon));
                    const Vector2F gameplay = WeaponReadback::nozzle(weapon);
                    // Record the rasterizer's destination-edge truncation too.
                    // Continuous angles/fractional scaling are not pixel-stable
                    // merely because the metadata's world anchors coincide.
                    const RectangleF destination = camera.calculate_view_rectangle(
                        RectangleF(WeaponReadback::position(weapon), sprite.size));
                    const Vector2F raster_size(
                        std::trunc(destination.right()) - std::trunc(destination.left()),
                        std::trunc(destination.bottom()) - std::trunc(destination.top()));
                    const Vector2F native_size(static_cast<float>(sprite.base.width),
                        static_cast<float>(sprite.base.height));
                    const Vector2F raster = Vector2F(std::trunc(destination.x), std::trunc(destination.y)) +
                        Vector2F::rotate_vector((anchor - pivot) / native_size * raster_size, WeaponReadback::angle(weapon));
                    const Vector2F screen_gameplay = camera.calculate_view_position(gameplay);
                    output << capture << ',' << tick << ',' << count_ << ',' << view << ',' << index << ',' << name
                        << ',' << viewport.width << ',' << viewport.height << ',' << extent.x << ',' << extent.y
                        << ',' << extent.width << ',' << extent.height << ',' << texel_world_size * camera.scale
                        << ',' << WeaponReadback::angle(weapon) << ',' << WeaponReadback::flipped(weapon)
                        << ',' << gameplay.x << ',' << gameplay.y << ',' << authored.x << ',' << authored.y
                        << ',' << (authored - gameplay).length() << ',' << (raster - screen_gameplay).length() << '\n';
                    require(std::isfinite(authored.x) && std::isfinite(authored.y), "non-finite kit anchor");
                    require((authored - gameplay).length() <= 0.001f,
                        "authored kit anchor disagrees with the actual gameplay muzzle/contact");
                }
            }
        }

    private:
        Vector2F focus(int view) const
        {
            return count_ == 1 ? (people_.front().center + people_.back().center) / 2.0f :
                people_[static_cast<size_t>(view)].center;
        }

        void platform(DrawList& list, float x, float y, int tiles, bool painted) const
        {
            for (int tile = 0; tile < tiles; ++tile)
            {
                const float left = x + static_cast<float>(tile) * 128.0f;
                kit_.draw(list, "platform", { left, y });
                if (painted)
                {
                    kit_.fill(list, RectangleF(left, y, 128.0f, tuning_.paint().thickness),
                        tile % 2 == 0 ? colours_.team_a : colours_.team_b);
                }
            }
        }

        void environment(DrawList& list, bool painted) const
        {
            for (int y = 0; y < 12; ++y)
            {
                for (int x = 0; x < 32; ++x)
                {
                    kit_.draw(list, "wall", { static_cast<float>(x) * 128.0f, static_cast<float>(y) * 128.0f });
                }
            }
            for (float x : { 1408.0f, 1536.0f, 2432.0f, 2560.0f })
            {
                kit_.draw(list, "shelf", { x, 856.0f });
                kit_.draw(list, "shelf", { x, 728.0f });
            }
            kit_.draw(list, "tank", { 1792.0f, 808.0f });
            kit_.draw(list, "tank", { 2304.0f, 808.0f });
            platform(list, 0.0f, 1000.0f, 32, painted);
            platform(list, 1536.0f, 696.0f, 3, painted);
            platform(list, 2304.0f, 760.0f, 3, painted);
        }

        Application& app_;
        const GameTuning& tuning_;
        const Presentation& presentation_;
        const Kit& kit_;
        TeamColour colours_;
        InterfaceGameplay hud_;
        FontHandle label_font_;
        PlayerCamera camera_;
        std::vector<Mannequin> people_;
        std::vector<Camera> cameras_;
        int count_ = 1;
        const RectangleF bounds_{ 0.0f, 0.0f, 4096.0f, 1536.0f };
    };
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        require(argc == 5, "usage: workshop_style_visual <staged-content> <kit-directory> <output> <sequence-frames>");
        const auto content = std::filesystem::absolute(argv[1]);
        const auto kit_directory = std::filesystem::absolute(argv[2]);
        const auto output = std::filesystem::absolute(argv[3]);
        const int sequence_frames = std::stoi(argv[4]);
        require(sequence_frames >= 0 && sequence_frames <= 90, "sequence frames must be between 0 and 90");
        std::filesystem::create_directories(output);
        GameTuning tuning((content / "tuning.json").string().c_str());
        Presentation presentation((content / "presentation.json").string().c_str());
        PlaySettings preferences;
        preferences.music_volume = 0;
        preferences.sfx_volume = 0;
        ApplicationOptions options;
        options.window_class_name = L"ColourWarsWorkshopStyleProbe";
        options.window_title = L"ColourWars workshop art study";
        options.max_threads = 4;
        options.visible = false;
        Application app(options);
        app.initialize();
        const auto& device = app.renderer()->device_info();
        std::cout << "DEVICE backend=" << device.backend << " api=" << device.api
            << " name=" << device.device_name << " vendor=" << device.vendor_id
            << " device=" << device.device_id << '\n';
        require(device.backend == "d3d11" && device.kind == RenderDeviceKind::hardware,
            "art study requires D3D11 hardware");
        AssetManifest manifest = anchored_to_source(read_asset_manifest((content / "manifest.json").string().c_str()));
        // This probe loads production rendering/audio dependencies only. It does
        // not build a Level, and the extra kit is never in the runtime manifest.
        std::erase_if(manifest.entries, [](const AssetEntry& entry) { return entry.kind == "level"; });
        manifest.entries.push_back({ "sprite_sheet", kit_directory.generic_string() + "/", "workshop_kit", false });
        app.resource_loader()->load_manifest(std::move(manifest));
        Kit kit(app.render_resources()->sprite_sheet("workshop_kit"), kit_directory / "workshop_kit.layout.json");
        std::ofstream geometry(output / "geometry.csv");
        geometry << std::setprecision(9);
        geometry << "capture,tick,views,view,mannequin,tool,pane_width,pane_height,world_x,world_y,world_width,world_height,screen_pixels_per_native_texel,angle,vertical_flip,gameplay_anchor_x,gameplay_anchor_y,kit_anchor_x,kit_anchor_y,world_anchor_error,screen_anchor_error_pixels\n";
        int captures = 0;
        for (const TeamColourPair palette : { TeamColourPair::blue_orange, TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            Study study(app, tuning, presentation, kit, preferences, palette);
            for (int count : { 1, 4 })
            {
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                {
                    study.layout(count, size);
                    for (bool painted : { false, true })
                    {
                        const std::string name = std::string(TeamColourTools::palette_key(palette)) + "-" +
                            std::to_string(count) + "p-" + std::to_string(size.y) + (painted ? "-painted" : "-clean");
                        const Frame frame = study.render(painted);
                        for (int repeat = 0; repeat < 3; ++repeat)
                        {
                            require(frame.pixels == study.render(painted).pixels,
                                "identical art study redraw produced different pixels");
                        }
                        save(frame, output / (name + ".ppm"));
                        study.geometry(geometry, name, 0);
                        ++captures;
                        std::cout << "CAPTURE " << name << " redraws=3 identical\n";
                    }
                }
            }
        }
        if (sequence_frames > 0)
        {
            Study study(app, tuning, presentation, kit, preferences, TeamColourPair::blue_orange);
            for (int count : { 1, 4 })
            {
                study.pose(0, true);
                study.layout(count, { 1280, 720 }, true);
                const auto sequence = output / (std::to_string(count) + "p-sequence-60fps");
                std::filesystem::create_directories(sequence);
                for (int tick = 0; tick < sequence_frames; ++tick)
                {
                    study.pose(tick, true);
                    study.follow();
                    std::ostringstream name;
                    name << "frame-" << std::setfill('0') << std::setw(4) << tick;
                    save(study.render(true), sequence / (name.str() + ".ppm"));
                    study.geometry(geometry, std::to_string(count) + "p-sequence/" + name.str(), tick);
                }
            }
        }
        require(geometry.good(), "geometry record write failed");
        std::ofstream scope(output / "scope.txt");
        scope << "Renderer and camera art study only. Static kit mannequins, tiled decorative composition, synthetic paint strips and fixed HUD values.\n"
            << "Actual PlayerCamera, viewports, InterfaceGameplay, team palettes and Weapon aim/nozzle placement are reused. No Player/Scene collision, scored paint, movement animation, controller, audio or gameplay acceptance.\n"
            << "All sprites draw at 4 world units per source texel. Point sampling, continuous aim and unsnapped camera; no claim of integer screen scaling or pixel stability.\n"
            << "Roller sleeve tint in painted captures is an illustrative firing-state colour. Contact puff is a fixed size sample.\n"
            << "Sequence: blue_orange, 720p, 1/4 views, 60 fps playback, 0.5 world units camera-follow target movement per frame; sprayer sweeps one turn in 90 frames, roller retains actual fixed aim.\n"
            << "Screen anchor error includes the engine's destination-edge truncation; nonzero values are diagnostic, not an art acceptance failure.\n";
        require(scope.good(), "scope record write failed");
        std::cout << "PASS " << captures << " art-study captures; repeated drawing deterministic. Human motion/style review remains open.\n";
        app.quit();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "workshop style probe failed: " << error.what() << '\n';
        return 1;
    }
}
