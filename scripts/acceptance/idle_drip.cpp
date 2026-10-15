#include "game/pch.h"
#include "game/input_slots.h"
#include "game/objects/idle_drip.h"
#include "game/objects/level_builder.h"
#include "game/objects/structure.h"
#include "engine/assets/sprite_sheet_loader.h"
#include "engine/core/step_timer.h"
#include "engine/math/triangle.h"
#include "engine/render/sprite_font_file.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <type_traits>

using namespace labrador;
using namespace mattmath;

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) { throw std::runtime_error(message); }
    }

    bool nearly_equal(float first, float second)
    {
        return std::abs(first - second) < 0.0001f;
    }

    const float dt = static_cast<float>(StepTimer::TicksToSeconds(
        StepTimer::SecondsToTicks(1.0 / 60.0)));

    auto observation(const IdleDrip& drip)
    {
        return std::tuple{ drip.attached(), drip.bead_age(), drip.interval_remaining(),
            drip.bead_rectangle(), drip.bounds(),
            std::vector<IdleDrip::Drop>(drip.drops().begin(), drip.drops().end()) };
    }

    void attach(IdleDrip& drip, const Vector2F& nozzle = Vector2F::ZERO,
        float angle = 0.0f)
    {
        drip.advance(true, nozzle, angle, drip.interval_remaining() + 0.001f, {});
        require(drip.attached() && drip.bead_age() == 0.0f,
            "Quiet interval did not begin a fresh attached bead");
    }

    void detach(IdleDrip& drip, const IdleDripPresentation& look,
        const Vector2F& nozzle = Vector2F::ZERO)
    {
        attach(drip, nozzle);
        drip.advance(true, nozzle, 0.0f, look.grow_time + look.stretch_time + 0.001f, {});
        require(!drip.attached() && !drip.drops().empty() && drip.drops().back().age == 0.0f,
            "Grown and stretched bead did not detach into a fresh falling drop");
    }

    constexpr std::array supported_tools{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket };
    constexpr std::array roster{ WeaponType::sprayer, WeaponType::sniper, WeaponType::bucket, WeaponType::roller };

    class ContentVariant
    {
    public:
        explicit ContentVariant(const std::filesystem::path& original)
        {
            std::ifstream stream(original);
            require(stream.good(), "Cannot read idle-drip acceptance content");
            this->source_ = std::string(std::istreambuf_iterator<char>(stream), {});
            wchar_t name[MAX_PATH]{};
            const auto temporary = std::filesystem::temp_directory_path();
            require(GetTempFileNameW(temporary.c_str(), L"cwd", 0, name) != 0,
                "Cannot reserve idle-drip acceptance content file");
            this->path_ = name;
        }

        ~ContentVariant()
        {
            std::error_code ignored;
            std::filesystem::remove(this->path_, ignored);
        }

        template<typename Edit>
        const std::filesystem::path& write(Edit edit) const
        {
            rapidjson::Document json;
            json.Parse(this->source_.c_str());
            require(!json.HasParseError(), "Cannot parse idle-drip acceptance content");
            edit(json);
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            json.Accept(writer);
            std::ofstream stream(this->path_);
            stream.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
            require(stream.good(), "Cannot write idle-drip acceptance content");
            return this->path_;
        }

    private:
        std::filesystem::path path_;
        std::string source_;
    };

    // Only metadata is loaded. These objects never create a device or a save.
    struct World
    {
        RenderResources render;
        AudioResources audio;
        PlaySettings preferences;
        GameTuning tuning;
        Presentation presentation;
        Registry<LevelLoadedInfo> levels{ "Level" };
        ResolutionManager resolution;
        ViewportManager viewports{ &resolution };
        labrador::Mouse mouse;
        Partitioner partitioner;
        std::unique_ptr<LevelBuilder> builder;

        explicit World(const std::filesystem::path& content,
            const std::filesystem::path& tuning_source = {}) :
            tuning((tuning_source.empty() ? content / "tuning.json" : tuning_source).string().c_str()),
            presentation((content / "presentation.json").string().c_str())
        {
            for (const char* sheet : { "sprite_sheet_1", "painter", "menu_paint", "weapon_paint", "paint_fx" })
            {
                this->render.add_sprite_sheet(sheet, read_sprite_sheet(
                    (content / "textures" / (std::string(sheet) + ".json")).string().c_str(), TextureHandle{}));
            }
            for (const char* font : { "gill_sans_ultra_bold_144", "gill_sans_mt_bold_24",
                "gill_sans_mt_bold_36", "gill_sans_mt_bold_48", "gill_sans_mt_bold_72",
                "gill_sans_mt_bold_144", "courier_new_bold_16" })
            {
                const SpriteFontFile file = read_sprite_font_file(
                    (content / "fonts" / (std::string(font) + ".spritefont")).string());
                auto loaded = std::make_unique<Font>(TextureHandle{}, file.glyphs, file.line_spacing);
                if (loaded->contains(U'?')) { loaded->set_stand_in(U'?'); }
                this->render.add_font(font, std::move(loaded));
            }
            this->audio.add_sound_bank("paint_exe", SoundBank::silent());
            auto info = std::make_unique<LevelLoadedInfo>(
                (content / "levels/movement_course.json").string().c_str());
            info->validate();
            this->levels.add("movement_course", std::move(info));
            this->builder = std::make_unique<LevelBuilder>(&this->viewports, &this->render,
                &this->audio, &this->preferences, &this->levels, &this->tuning,
                &this->presentation, &this->resolution, nullptr, &this->partitioner, &this->mouse);
        }

        std::unique_ptr<Level> build(float duration = 300.0f,
            const std::array<WeaponType, 4>& tools = roster)
        {
            MenuLevelSettings settings;
            settings.set_stage("movement_course");
            settings.set_player_count(4);
            settings.set_screen_layout(ScreenLayout::four_player);
            settings.set_duration(duration);
            for (int index = 0; index < 4; ++index)
            {
                settings.set_player_team(index, PlayerTeam::a);
                settings.set_player_weapon(index, tools.at(static_cast<size_t>(index)));
            }
            settings.begin_input_claims(0);
            for (int slot = 1; slot < 4; ++slot) { require(settings.claim_input_slot(slot), "Could not claim CPU pad slot"); }
            this->viewports.set_layout(settings.screen_layout());
            return this->builder->build_level(settings, TeamColourPair::blue_orange, OpeningMode::off);
        }

        std::unique_ptr<Structure> structure(std::unique_ptr<Shape> shape,
            CollisionObjectType type = CollisionObjectType::structure)
        {
            const RectangleF rectangle = shape->bounding_box();
            return std::make_unique<Structure>("sprite_sheet_1", "square_white_4", rectangle,
                std::move(shape), &this->render, type);
        }
    };

    void check_profiles(World& world, const std::filesystem::path& content)
    {
        const auto& sprayer = world.presentation.idle_drip();
        const auto& sniper = world.presentation.idle_drip(WeaponType::sniper);
        const auto& bucket = world.presentation.idle_drip(WeaponType::bucket);
        require(sprayer.enabled && sprayer.interval_min == 2.8f && sprayer.interval_max == 6.2f &&
            sprayer.size == 6.0f && sprayer.stretch == 5.0f && sprayer.nozzle_inset == 8.5f &&
            sprayer.nozzle_underside == 3.0f,
            "Sprayer residual-paint profile changed while adding other tools");
        require(sniper.enabled && sniper.interval_min > sprayer.interval_min &&
            sniper.interval_max > sprayer.interval_max && sniper.size < sprayer.size &&
            sniper.stretch < sprayer.stretch && bucket.enabled &&
            bucket.interval_min > sprayer.interval_min && bucket.interval_max > sprayer.interval_max &&
            bucket.size < sprayer.size && bucket.stretch < sprayer.stretch,
            "Sniper or bucket lost its sparse, tool-specific residual-paint profile");
        require(!world.presentation.idle_drip(WeaponType::roller).enabled &&
            !world.presentation.idle_drip(WeaponType::mister).enabled,
            "An unsupported tool inherited an enabled residual-paint profile");
        for (const WeaponType type : supported_tools)
        {
            const auto& look = world.presentation.idle_drip(type);
            require(look.grow_time == sprayer.grow_time && look.stretch_time == sprayer.stretch_time &&
                look.lifetime == sprayer.lifetime && look.gravity == sprayer.gravity &&
                look.sprite_sheet == sprayer.sprite_sheet && look.fill_frame == sprayer.fill_frame &&
                look.keyline == sprayer.keyline && look.keyline_width == sprayer.keyline_width,
                "Tool residual paint did not inherit the shared liquid presentation");
        }

        ContentVariant invalid(content / "presentation.json");
        const auto reject = [&](const char* tool, const char* field, float value)
        {
            const auto& path = invalid.write([&](rapidjson::Document& json)
            {
                json["idle_drip"]["tools"][tool][field].SetFloat(value);
            });
            bool rejected = false;
            try { static_cast<void>(Presentation(path.string().c_str())); }
            catch (const std::exception& error)
            {
                const std::string message(error.what());
                rejected = message.find(path.string()) != std::string::npos &&
                    message.find(tool) != std::string::npos;
            }
            require(rejected, "Malformed drip profile did not fail with its source path and tool name");
        };
        reject("sniper", "interval_max", sniper.interval_min - 0.1f);
        reject("bucket", "size", bucket.keyline_width * 2.0f);
        reject("bucket", "nozzle_underside", 0.0f);
        std::cout << "PASS idle drip profiles: preserved sprayer, sparse sniper/bucket, unsupported tools and contextual content errors\n";
    }

    void check_quiet_intervals(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        IdleDrip first(&look, &world.render, Colour::white, 0);
        const float initial = first.interval_remaining();
        for (int tick = 0; tick < 1200; ++tick)
        {
            first.advance(false, Vector2F::ZERO, 0.0f, dt, {});
            require(!first.attached() && first.drops().empty() && first.interval_remaining() == initial,
                "Unsettled time consumed the next quiet interval or created a drip");
        }
        first.advance(true, Vector2F::ZERO, 0.0f, initial * 0.5f, {});
        require(!first.attached() && first.interval_remaining() < initial,
            "Settled sprayer skipped the wait or failed to age its quiet interval");
        first.advance(false, Vector2F::ZERO, 0.0f, dt, {});
        const float restarted = first.interval_remaining();
        require(restarted >= look.interval_min && restarted <= look.interval_max,
            "Activity did not schedule a complete new quiet interval");
        first.advance(true, Vector2F::ZERO, 0.0f, restarted * 0.5f, {});
        require(!first.attached(), "Interrupted quiet time leaked into the next idle interval");

        std::srand(8123);
        const int expected_random = std::rand();
        std::srand(8123);
        IdleDrip same_a(&look, &world.render, Colour::white, 3);
        IdleDrip same_b(&look, &world.render, Colour::white, 3);
        IdleDrip different(&look, &world.render, Colour::white, 4);
        bool varied = false;
        bool distinct = false;
        float previous = same_a.interval_remaining();
        for (int cycle = 0; cycle < 12; ++cycle)
        {
            require(observation(same_a) == observation(same_b),
                "Equal roster identities did not reproduce the local cosmetic sequence");
            distinct = distinct || same_a.interval_remaining() != different.interval_remaining();
            detach(same_a, look);
            detach(same_b, look);
            detach(different, look);
            const float interval = same_a.interval_remaining();
            require(interval >= look.interval_min && interval <= look.interval_max,
                "Local drip interval escaped its presentation range");
            varied = varied || interval != previous;
            previous = interval;
        }
        require(varied && distinct && std::rand() == expected_random,
            "Drip timing repeated uniformly, shared identities or consumed gameplay rand()");
        for (int cycle = 0; cycle < 6; ++cycle)
        {
            detach(same_a, look);
            std::ignore = std::rand();
            detach(same_b, look);
            require(observation(same_a) == observation(same_b),
                "Gameplay random draws changed the drip's private interval sequence");
        }
        std::cout << "PASS idle drip: settled waits, interrupted waits, varied per-seat intervals and isolated RNG\n";
    }

    void check_attachment_and_falling(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        const Vector2F nozzle(120.0f, 80.0f);
        IdleDrip right(&look, &world.render, Colour::white, 0);
        IdleDrip left(&look, &world.render, Colour::white, 0);
        attach(right, nozzle);
        attach(left, nozzle, PI);
        require(nearly_equal(right.bead_rectangle().center().x, nozzle.x - look.nozzle_inset) &&
            nearly_equal(left.bead_rectangle().center().x, nozzle.x + look.nozzle_inset) &&
            nearly_equal(right.bead_rectangle().y, nozzle.y + look.nozzle_underside) &&
            nearly_equal(left.bead_rectangle().y, right.bead_rectangle().y),
            "Facing flip moved the residual bead off the tool's nozzle underside");

        const RectangleF initial_bead = right.bead_rectangle();
        right.advance(true, nozzle, 0.0f, look.grow_time * 0.5f, {});
        const RectangleF growing = right.bead_rectangle();
        require(growing.width > initial_bead.width && nearly_equal(growing.height, growing.width) &&
            nearly_equal(growing.y, initial_bead.y), "Attached bead did not grow from its nozzle attachment");
        right.advance(true, nozzle, 0.0f, look.grow_time * 0.5f + look.stretch_time * 0.5f, {});
        const RectangleF stretched = right.bead_rectangle();
        require(nearly_equal(stretched.width, look.size) && stretched.height > stretched.width &&
            nearly_equal(stretched.y, initial_bead.y), "Grown bead did not stretch downwards before release");
        const Vector2F moved_nozzle(140.0f, 50.0f);
        const float angle = PI * 0.25f;
        left.advance(true, moved_nozzle, angle, look.grow_time * 0.25f, {});
        const Vector2F expected_anchor = moved_nozzle +
            Vector2F::rotate_vector(Vector2F(-look.nozzle_inset, look.nozzle_underside), angle);
        require(nearly_equal(left.bead_rectangle().center().x, expected_anchor.x) &&
            nearly_equal(left.bead_rectangle().y, expected_anchor.y),
            "Continuous aim or translation left an attached bead behind the nozzle");

        right.advance(true, nozzle, 0.0f, look.stretch_time, {});
        require(!right.attached() && right.drops().size() == 1, "Stretched bead failed to release");
        const Vector2F released = right.drops()[0].position;
        const float fall_step = std::min(0.05f, look.lifetime * 0.1f);
        right.advance(false, Vector2F(900, -900), PI, fall_step, {});
        require(right.drops().size() == 1 && right.drops()[0].position.x == released.x &&
            right.drops()[0].position.y > released.y,
            "Detached paint followed later aim or failed to fall under gravity");
        const float first_fall = right.drops()[0].position.y - released.y;
        const float before_second = right.drops()[0].position.y;
        right.advance(false, Vector2F(-900, 900), -PI * 0.5f, fall_step, {});
        require(right.drops()[0].position.y - before_second > first_fall,
            "Falling residual paint did not accelerate under gravity");

        left.advance(false, nozzle, 0.0f, dt, {});
        require(!left.attached() && left.drops().empty(), "Activity failed to cancel an attached bead");
        right.advance(false, nozzle, 0.0f, look.lifetime, {});
        require(right.drops().empty(), "Falling residual paint exceeded its bounded lifetime");
        std::cout << "PASS idle drip: both facings, aimed attachment, growth/stretch, independent gravity and cancellation\n";
    }

    void check_terrain(World& world)
    {
        const auto& look = world.presentation.idle_drip();
        IdleDrip thin(&look, &world.render, Colour::white, 0);
        detach(thin, look);
        const float x = thin.drops()[0].position.x;
        const float top = thin.drops()[0].position.y + 20.0f;
        auto platform = world.structure(std::make_unique<RectangleF>(x - 20.0f, top, 40.0f, 0.1f),
            CollisionObjectType::structure_jump_through);
        const std::array<CollisionObject*, 1> platform_targets{ platform.get() };
        thin.advance(false, Vector2F::ZERO, 0.0f, 0.5f, platform_targets);
        require(thin.drops().empty(), "Residual paint tunnelled through a thin jump-through platform");

        IdleDrip ramp(&look, &world.render, Colour::white, 0);
        detach(ramp, look, Vector2F(10.0f + look.nozzle_inset, 0.0f));
        auto slope = world.structure(std::make_unique<Triangle>(0.0f, 100.0f, 100.0f, 0.0f, 100.0f, 100.0f),
            CollisionObjectType::structure_ramp_right);
        const std::array<CollisionObject*, 1> ramp_targets{ slope.get() };
        ramp.advance(false, Vector2F::ZERO, 0.0f, 0.35f, ramp_targets);
        require(ramp.drops().size() == 1,
            "Ramp bounding box retired a drop still above its actual sloping surface");
        ramp.advance(false, Vector2F::ZERO, 0.0f, 0.25f, ramp_targets);
        require(ramp.drops().empty(), "Residual paint passed through the ramp's actual surface");

        IdleDrip blocked(&look, &world.render, Colour::white, 0);
        auto wall = world.structure(std::make_unique<RectangleF>(-20.0f, 0.0f, 40.0f, 30.0f));
        const std::array<CollisionObject*, 1> wall_targets{ wall.get() };
        blocked.advance(true, Vector2F::ZERO, 0.0f, blocked.interval_remaining() + 0.001f, wall_targets);
        require(!blocked.attached() && blocked.drops().empty(),
            "Attached paint grew inside terrain instead of disappearing on contact");

        IdleDrip ignored(&look, &world.render, Colour::white, 0);
        auto projectile_tag = world.structure(std::make_unique<RectangleF>(-20.0f, 0.0f, 40.0f, 30.0f),
            CollisionObjectType::projectile_spray_team_a);
        const std::array<CollisionObject*, 1> ignored_targets{ projectile_tag.get() };
        ignored.advance(true, Vector2F::ZERO, 0.0f, ignored.interval_remaining() + 0.001f, ignored_targets);
        require(ignored.attached(), "Residual paint treated a non-terrain collision object as scenery");
        std::cout << "PASS idle drip: swept thin-platform contact, true ramp geometry, attached contact and terrain filtering\n";
    }

    void check_freeze_disable_and_capacity(World& world)
    {
        static_assert(!std::is_base_of_v<CollisionObject, IdleDrip>);
        static_assert(std::is_same_v<decltype(&IdleDrip::draw), void (IdleDrip::*)(DrawList&) const>);
        auto look = world.presentation.idle_drip();
        look.lifetime = 100.0f;
        IdleDrip drip(&look, &world.render, Colour::white, 0);
        detach(drip, look);
        attach(drip);
        const auto before = observation(drip);
        for (float elapsed : { 0.0f, -1.0f, std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN() })
        {
            drip.advance(false, Vector2F(900, 900), PI, elapsed, {});
            drip.update(elapsed);
            require(observation(drip) == before, "Frozen or invalid delta advanced residual-paint state");
        }

        const auto falling = drip.drops()[0];
        drip.advance(false, Vector2F(900, 900), PI, dt, {});
        require(!drip.attached() && drip.drops().size() == 1 &&
            drip.drops()[0].position.x == falling.position.x &&
            drip.drops()[0].position.y > falling.position.y,
            "Activity cancelled an existing falling drop or retained its attached bead");
        attach(drip);
        const auto before_draw = observation(drip);

        look.enabled = false;
        DrawList unused;
        for (int view = 0; view < 4; ++view) { drip.draw(unused); }
        require(observation(drip) == before_draw, "Disabled draw fan-out mutated simulation state");
        drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        require(!drip.attached() && drip.drops().empty(), "Disabled presentation retained active residual paint");
        for (int tick = 0; tick < 600; ++tick) { drip.advance(true, Vector2F::ZERO, 0.0f, dt, {}); }
        require(!drip.attached() && drip.drops().empty(), "Disabled presentation emitted residual paint");

        look.enabled = true;
        look.interval_min = 0.001f;
        look.interval_max = 0.002f;
        look.grow_time = 0.001f;
        look.stretch_time = 0.001f;
        look.lifetime = 1000.0f;
        look.gravity = 0.0f;
        IdleDrip crowded(&look, &world.render, Colour::white, 0);
        for (int tick = 0; tick < 1000; ++tick)
        {
            crowded.advance(true, Vector2F::ZERO, 0.0f, dt, {});
            require(crowded.drops().size() <= 4, "Residual paint exceeded its fixed four-drop storage");
        }
        require(crowded.drops().size() == 4, "Capacity fixture never exercised full residual-paint storage");
        attach(crowded);
        crowded.clear();
        require(!crowded.attached() && crowded.drops().empty() && crowded.bounds() == RectangleF::ZERO,
            "Explicit scene-ending clear retained attached or falling residual paint");
        crowded.clear();
        require(!crowded.attached() && crowded.drops().empty(), "Residual-paint clear was not idempotent");
        std::cout << "PASS idle drip: frozen deltas, const/disabled draw, feature switch and fixed capacity\n";
    }

    void check_live_preference(World& world)
    {
        auto look = world.presentation.idle_drip();
        look.lifetime = 100.0f;
        PlaySettings preferences;
        IdleDrip drip(&look, &world.render, Colour::white, 0, &preferences);
        detach(drip, look);
        attach(drip);
        const auto before = observation(drip);
        preferences.reduce_effects = true;
        // A default list cannot accept sprites. Deliberate suppression must
        // return before touching it, including while the match is paused.
        DrawList suppressed;
        for (int view = 0; view < 4; ++view) { drip.draw(suppressed); }
        require(observation(drip) == before,
            "Reduced-effects draw mutated the frozen residual-paint state");
        drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        require(!drip.attached() && drip.drops().empty() && drip.bounds() == RectangleF::ZERO,
            "Reduced-effects update retained attached or detached residual paint");
        for (int tick = 0; tick < 600; ++tick)
        {
            drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        }
        require(!drip.attached() && drip.drops().empty(),
            "Reduced-effects sprayer generated new residual paint");
        preferences.reduce_effects = false;
        require(!drip.attached() && drip.drops().empty(),
            "Restoring effects resurrected an old attached or detached drip");
        drip.advance(true, Vector2F::ZERO, 0.0f, dt, {});
        require(!drip.attached() && drip.drops().empty() &&
            drip.interval_remaining() >= look.interval_min - dt,
            "Restoring effects skipped the fresh residual-paint wait");
        attach(drip);
        require(drip.drops().empty(), "Fresh bead inherited previous detached drops");
        std::cout << "PASS idle drip preference: frozen draw suppression, active clear, no new drips and fresh restart\n";
    }

    PlayerInputData input()
    {
        PlayerInputData result;
        result.connected = true;
        result.shoot_direction_requested = true;
        result.shoot_direction = Vector2F(1, 0);
        return result;
    }

    void compare_gameplay(const Level& first, const Level& second)
    {
        require(first.state() == second.state() && first.projectile_count() == second.projectile_count() &&
            first.collision_object_count() == second.collision_object_count(),
            "Idle-drip feature switch changed level state or gameplay-object counts");
        for (int view = 0; view < first.player_count(); ++view)
        {
            const Player& a = first.player(view);
            const Player& b = second.player(view);
            require(a.rectangle() == b.rectangle() && a.velocity() == b.velocity() &&
                a.state() == b.state() && a.move_state() == b.move_state() && a.health() == b.health() &&
                a.weapon_ammo() == b.weapon_ammo() &&
                a.primary_weapon().ready_to_shoot() == b.primary_weapon().ready_to_shoot() &&
                a.primary_weapon().rotation() == b.primary_weapon().rotation() &&
                a.primary_weapon().nozzle_pos() == b.primary_weapon().nozzle_pos(),
                "Idle-drip feature switch changed movement, damage, paint, shot readiness or muzzle");
        }
        const auto a = first.level_end_info();
        const auto b = second.level_end_info();
        require(a.player_areas == b.player_areas && a.team_a_area == b.team_a_area &&
            a.team_b_area == b.team_b_area && a.total_area == b.total_area,
            "Cosmetic idle drips captured a surface or changed the paint score");
    }

    void check_level_integration(World& enabled_world, const std::filesystem::path& content)
    {
        World disabled_world(content);
        for (const WeaponType type : supported_tools)
        {
            const_cast<IdleDripPresentation&>(disabled_world.presentation.idle_drip(type)).enabled = false;
        }
        std::srand(753);
        auto enabled = enabled_world.build(900.0f);
        std::srand(753);
        auto disabled = disabled_world.build(900.0f);
        require(enabled->idle_drip(0) != nullptr && enabled->idle_drip(1) != nullptr &&
            enabled->idle_drip(2) != nullptr && enabled->idle_drip(3) == nullptr,
            "Level did not attach residual paint to precisely the sprayer, sniper and bucket");
        {
            auto other = disabled_world.build(300.0f,
                { WeaponType::mister, WeaponType::sniper, WeaponType::bucket, WeaponType::sprayer });
            require(other->idle_drip(0) == nullptr && other->idle_drip(1) != nullptr &&
                other->idle_drip(2) != nullptr && other->idle_drip(3) != nullptr,
                "Mister inherited residual paint or supported tools depended on their roster position");
        }
        std::vector<PlayerInputData> inputs(input_slots::count, input());
        unsigned tick_number = 0;
        const auto tick = [&]
        {
            const unsigned seed = 753u + tick_number++;
            std::srand(seed);
            enabled->update(inputs, dt);
            std::srand(seed);
            disabled->update(inputs, dt);
            compare_gameplay(*enabled, *disabled);
            for (int view = 0; view < 3; ++view)
            {
                require(!disabled->idle_drip(view)->attached() && disabled->idle_drip(view)->drops().empty(),
                    "Disabled Level emitted a cosmetic drip");
            }
        };
        const auto wait_for_bead = [&](int view)
        {
            for (int step = 0; step < 1200 && (!enabled->idle_drip(view)->attached() ||
                enabled->idle_drip(view)->bead_age() != 0.0f); ++step) { tick(); }
            require(enabled->idle_drip(view)->attached() && enabled->idle_drip(view)->bead_age() == 0.0f,
                "Real Level never delivered settled player time to a fresh drip");
        };
        const auto untouched_score = enabled->level_end_info().player_areas;
        const std::array untouched_ammo{ enabled->player(0).weapon_ammo(),
            enabled->player(1).weapon_ammo(), enabled->player(2).weapon_ammo() };
        std::array<bool, 3> fell{};
        for (int step = 0; step < 1200; ++step)
        {
            tick();
            for (int view = 0; view < 3; ++view)
            {
                fell.at(static_cast<size_t>(view)) = fell.at(static_cast<size_t>(view)) ||
                    !enabled->idle_drip(view)->drops().empty();
                require(enabled->player(view).weapon_ammo() == untouched_ammo.at(static_cast<size_t>(view)),
                    "Real idle drip spent ammunition");
            }
        }
        require(std::all_of(fell.begin(), fell.end(), [](bool value) { return value; }) &&
            enabled->level_end_info().player_areas == untouched_score,
            "A real tool's idle drip failed to fall or captured a surface");

        for (int view = 0; view < 3; ++view)
        {
            wait_for_bead(view);
            const auto paused = observation(*enabled->idle_drip(view));
            for (int step = 0; step < 10; ++step) { enabled->update(inputs, 0.0f); }
            require(observation(*enabled->idle_drip(view)) == paused,
                "Frozen Level update advanced residual-paint state");

            // A bead follows the rendered wet lip, including the bucket's rim
            // well behind its projectile launch point. Use independent art
            // landmarks rather than deriving expectations from the drip code.
            for (const float angle : { 0.0f, PI, -PI * 0.25f, -PI * 0.75f, PI * 0.125f, PI * 0.875f })
            {
                auto& aimed = inputs.at(static_cast<size_t>(view));
                aimed.shoot_angle = angle;
                aimed.shoot_direction = Vector2F(std::cos(angle), std::sin(angle));
                tick();
                const Weapon& weapon = enabled->player(view).primary_weapon();
                const auto& look = enabled_world.presentation.idle_drip(weapon.type());
                Vector2F local = view == 1 ? Vector2F(99.0f, 3.0f) : Vector2F(46.0f, 8.0f);
                if (view == 0) { local = Vector2F(69.0f, 3.0f); }
                if (angle > PI * 0.5f || angle < -PI * 0.5f) { local.y = -local.y; }
                const Vector2F anchor = weapon.draw_pos() + Vector2F::rotate_vector(local, angle);
                const IdleDrip& drip = *enabled->idle_drip(view);
                require(weapon.rotation() == angle && drip.attached() &&
                    nearly_equal(drip.bead_rectangle().center().x, anchor.x) &&
                    nearly_equal(drip.bead_rectangle().y, anchor.y) &&
                    drip.bead_rectangle().width <= look.size,
                    "Real aimed/facing tool left its bead off the rendered wet lip");
            }
            inputs.at(static_cast<size_t>(view)) = input();
            inputs.at(static_cast<size_t>(view)).x_movement = 1.0f;
            tick();
            require(!enabled->idle_drip(view)->attached(), "Resolved walking failed to cancel the attached bead");
            inputs.at(static_cast<size_t>(view)) = input();
            wait_for_bead(view);
            const float ammo = enabled->player(view).weapon_ammo();
            inputs.at(static_cast<size_t>(view)).primary_shoot = true;
            tick();
            require(!enabled->idle_drip(view)->attached() && enabled->player(view).weapon_ammo() < ammo,
                "Paid firing failed to cancel the attached bead");
            inputs.at(static_cast<size_t>(view)) = input();
            wait_for_bead(view);
            inputs.at(static_cast<size_t>(view)).jump_pressed = true;
            inputs.at(static_cast<size_t>(view)).jump_held = true;
            tick();
            require(!enabled->idle_drip(view)->attached(), "Jumping failed to cancel the attached bead");
            inputs.at(static_cast<size_t>(view)) = input();
            wait_for_bead(view);
            require(enabled->kill_player(view) && disabled->kill_player(view), "Could not eliminate the drip fixture player");
            tick();
            require(!enabled->idle_drip(view)->attached(), "Elimination retained an attached residual bead");
            require(enabled->respawn_player(view) && disabled->respawn_player(view), "Could not respawn the drip fixture player");
            tick();
            require(!enabled->idle_drip(view)->attached(), "Respawn immediately re-used an old residual bead");

            wait_for_bead(view);
            enabled_world.preferences.reduce_effects = true;
            const auto before_draw = observation(*enabled->idle_drip(view));
            DrawList suppressed;
            enabled->idle_drip(view)->draw(suppressed);
            require(observation(*enabled->idle_drip(view)) == before_draw,
                "Live reduced-effects draw changed the frozen tool drip");
            for (int step = 0; step < 650; ++step)
            {
                tick();
                for (int supported = 0; supported < 3; ++supported)
                {
                    require(!enabled->idle_drip(supported)->attached() && enabled->idle_drip(supported)->drops().empty(),
                        "Live reduced-effects preference retained or restarted a tool drip");
                }
            }
            enabled_world.preferences.reduce_effects = false;
            tick();
            require(!enabled->idle_drip(view)->attached() && enabled->idle_drip(view)->drops().empty() &&
                enabled->idle_drip(view)->interval_remaining() >=
                    enabled_world.presentation.idle_drip(roster.at(static_cast<size_t>(view))).interval_min - dt,
                "Restoring real Level effects resurrected an old drip or skipped the fresh wait");
        }
        std::cout << "PASS idle drip: all three real tools, rendered lip transforms, pause/actions/death/respawn, live preference and gameplay equivalence\n";
    }

    void check_empty_held_fire(const std::filesystem::path& content)
    {
        ContentVariant empty_tank(content / "tuning.json");
        const auto& tuning = empty_tank.write([](rapidjson::Document& json)
        {
            json["weapons"]["starting_ammo"].SetFloat(0.001f);
            json["weapons"]["ammo_regen_delay"].SetFloat(1000.0f);
            json["weapons"]["ammo_regen_rate"].SetFloat(0.0f);
        });
        World world(content, tuning);
        auto level = world.build();
        std::vector<PlayerInputData> inputs(input_slots::count, input());
        for (int view = 0; view < 3; ++view)
        {
            for (int step = 0; step < 1200 && !level->idle_drip(view)->attached(); ++step)
            {
                level->update(inputs, dt);
            }
            require(level->idle_drip(view)->attached(), "Empty tool did not form a bead while genuinely idle");
            inputs.at(static_cast<size_t>(view)).primary_shoot = true;
            level->update(inputs, dt);
            require(!level->idle_drip(view)->attached() &&
                level->player(view).primary_weapon().readiness() == WeaponReadiness::empty &&
                !level->player(view).primary_weapon().shooting_this_update(),
                "An unaffordable held shot retained its bead or was mistaken for paid firing");
        }
        for (int step = 0; step < 1200; ++step)
        {
            level->update(inputs, dt);
            for (int view = 0; view < 3; ++view)
            {
                require(!level->idle_drip(view)->attached() && level->idle_drip(view)->drops().empty() &&
                    level->player(view).weapon_ammo() == 0.001f,
                    "Sustained empty held fire started a residual bead or changed ammunition");
            }
        }
        require(level->projectile_count() == 0, "Empty held-fire fixture emitted gameplay projectiles");
        std::cout << "PASS idle drip: empty held fire interrupts and suppresses all three tools beyond their quiet intervals\n";
    }

    void check_round_end(const std::filesystem::path& content)
    {
        World world(content);
        for (const WeaponType type : supported_tools)
        {
            auto& look = const_cast<IdleDripPresentation&>(world.presentation.idle_drip(type));
            look.interval_min = 0.01f;
            look.interval_max = 0.02f;
            look.grow_time = 0.01f;
            look.stretch_time = 0.01f;
            look.lifetime = 100.0f;
            look.gravity = 0.0f;
        }
        auto level = world.build(0.5f);
        const std::vector<PlayerInputData> inputs(input_slots::count, input());
        std::array<bool, 3> saw_attached{};
        std::array<bool, 3> saw_falling{};
        for (int step = 0; step < 600 && level->state() != LevelState::zoom_out; ++step)
        {
            level->update(inputs, dt);
            for (int view = 0; view < 3; ++view)
            {
                saw_attached.at(static_cast<size_t>(view)) = saw_attached.at(static_cast<size_t>(view)) ||
                    level->idle_drip(view)->attached();
                saw_falling.at(static_cast<size_t>(view)) = saw_falling.at(static_cast<size_t>(view)) ||
                    !level->idle_drip(view)->drops().empty();
            }
        }
        require(std::all_of(saw_attached.begin(), saw_attached.end(), [](bool value) { return value; }) &&
            std::all_of(saw_falling.begin(), saw_falling.end(), [](bool value) { return value; }) &&
            level->state() == LevelState::zoom_out,
            "Round-end fixture failed to exercise attached and falling paint before time-up");
        for (int view = 0; view < 3; ++view)
        {
            require(!level->idle_drip(view)->attached() && level->idle_drip(view)->drops().empty(),
                "Time-up froze residual paint on screen instead of clearing it with the tool");
        }
        for (int step = 0; step < 60; ++step) { level->update(inputs, dt); }
        for (int view = 0; view < 3; ++view)
        {
            require(!level->idle_drip(view)->attached() && level->idle_drip(view)->drops().empty(),
                "Round reveal restarted residual paint after its tool disappeared");
        }
        std::cout << "PASS idle drip: time-up clears and reveal cannot restart attached or falling paint\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "Usage: idle_drip.exe <content directory>");
        const std::filesystem::path content(argv[1]);
        World world(content);
        check_profiles(world, content);
        check_quiet_intervals(world);
        check_attachment_and_falling(world);
        check_terrain(world);
        check_freeze_disable_and_capacity(world);
        check_live_preference(world);
        check_level_integration(world, content);
        check_empty_held_fire(content);
        check_round_end(content);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL idle drip: " << error.what() << '\n';
        return 1;
    }
}
