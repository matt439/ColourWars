// Included inside painter_visual.cpp's private namespace after the shared
// raster helpers. Exact ammunition fixtures isolate reservoir information from
// muzzle pulses, valve motion and the player's separate HUD meter.

class SupplyWeapon : public RelativeVelocityWeapon
{
public:
    using RelativeVelocityWeapon::RelativeVelocityWeapon;
    using Weapon::set_ammo;

    void aim(float angle, bool right)
    {
        PlayerInputData input;
        input.shoot_angle = angle;
        input.shoot_direction = Vector2F::unit_vec_from_angle(angle);
        input.shoot_direction_requested = true;
        this->update_movement_and_rotation(input, this->player_center(), Vector2F::ZERO, right);
    }

    void pose(ToolMotionPose value)
    {
        // This fixture selects each authored pose without paying a shot: a
        // cosmetic pressure pulse must never substitute for stored paint.
        const_cast<ToolMotionState&>(this->motion_state()).pose = value;
    }

    const AnimationStrip& supply() const
    {
        return this->sprite_sheet()->animation_strip(this->sprite_sheet()->
            resolve_animation_strip(this->details().frame_name + "_supply"));
    }

    void draw_supply(DrawList& list, int index) const
    {
        const RectangleI& base = this->sprite_sheet()->sprite_frame(this->frame()).source_rectangle();
        const RectangleI& source = this->supply().frame_rect(index);
        require(source.width == base.width && source.height == base.height,
            "Supply strip does not share the tool canvas; cropped draws can drift under camera scaling");
        this->sprite_sheet()->draw(list, source, RectangleF(this->draw_pos(), this->details().size),
            this->paint_colour(), this->rotation(), Vector2F(0, static_cast<float>(base.height) / 2),
            this->invert_y() ? SpriteFlip::vertical : SpriteFlip::none);
    }

    std::vector<float> state() const
    {
        const ToolMotionState& motion = this->motion_state();
        return { this->ammo(), this->ammo_timer(), this->shoot_timer(), this->rotation(),
            this->draw_pos().x, this->draw_pos().y, this->nozzle_pos().x, this->nozzle_pos().y,
            static_cast<float>(this->readiness()), this->shooting_this_update() ? 1.0f : 0.0f,
            static_cast<float>(motion.pose), motion.pressure, motion.valve, motion.travel_phase,
            motion.lift, motion.compression, motion.bucket_swing, motion.hose_sway,
            motion.mist_tail_remaining };
    }
};

Frame supply_fixture(Application& app, const Canvas& canvas, const SupplyWeapon* tool,
    const Viewport& viewport, const Camera& camera, int overlay = -1)
{
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(viewport);
    list.set_camera(camera);
    list.set_filter(TextureFilter::point);
    canvas.draw(list, camera.visible_rectangle(viewport));
    if (tool != nullptr)
    {
        tool->draw(list);
        if (overlay >= 0) { tool->draw_supply(list, overlay); }
    }
    return read(renderer);
}

void supply_raster_case(Application& app, const Canvas& canvas, SupplyWeapon& tool,
    const Viewport& viewport, float scale, float maximum, PlaySettings& preferences,
    const std::filesystem::path& output, const std::string& stem, std::ofstream& records)
{
    const RectangleI pane = viewport.pixel_rect();
    const Camera camera(tool.draw_pos() - Vector2F(static_cast<float>(pane.width) / (2 * scale),
        static_cast<float>(pane.height) / (2 * scale)), scale);
    const auto draw = [&] { return supply_fixture(app, canvas, &tool, viewport, camera); };
    const Frame blank = supply_fixture(app, canvas, nullptr, viewport, camera);
    tool.set_ammo(0);
    const Frame empty = draw();
    const int last = tool.supply().frame_count() - 1;
    require(last >= 2, "Supply strip needs distinct empty, half and full states");
    // Compose the authored full/half art directly over the actual empty tool.
    // These references bypass Weapon's ammo-to-frame selection completely.
    const Frame expected_full = supply_fixture(app, canvas, &tool, viewport, camera, last);
    const Frame expected_half = supply_fixture(app, canvas, &tool, viewport, camera, (last + 1) / 2);
    tool.set_ammo(maximum);
    const Frame full = draw();
    require(full.pixels == expected_full.pixels,
        "Full supply is missing or misregistered with its authored tool canvas");
    tool.set_ammo(maximum * 0.5f);
    const auto before = tool.state();
    std::srand(39481);
    const int expected_random = std::rand();
    std::srand(39481);
    const Frame half = draw();
    require(half.pixels == expected_half.pixels,
        "Half supply did not select and register the authored bottom-filled state");
    require(half.pixels == draw().pixels && before == tool.state(),
        "Frozen reservoir drawing changed pixels or weapon state");
    require(std::rand() == expected_random, "Reservoir drawing consumed random numbers");
    const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = true;
    require(half.pixels == draw().pixels && before == tool.state(),
        "Reduce effects changed stored-paint information or weapon state");
    preferences.reduce_effects = false;
    tool.set_ammo(-maximum);
    require(draw().pixels == empty.pixels, "Negative ammunition was not visually clamped empty");
    tool.set_ammo(maximum * 2);
    require(draw().pixels == full.pixels, "Oversized ammunition was not visually clamped full");
    tool.set_ammo(maximum * 0.5f);

    size_t full_pixels = 0, half_pixels = 0;
    for (size_t offset = 0; offset < full.pixels.size(); offset += 4)
    {
        if (!same_pixel(full, empty, offset))
        {
            // An empty reservoir still has an opaque neutral interior. Filling
            // it must never grow the silhouette or add paint outside the tool.
            require(!same_pixel(empty, blank, offset),
                "Supply introduced pixels outside the empty tool silhouette");
            ++full_pixels;
        }
        if (!same_pixel(half, empty, offset))
        {
            require(same_pixel(full, half, offset),
                "Half supply escaped the full reservoir or changed permanent hardware/nozzle paint");
            ++half_pixels;
        }
    }
    require(full_pixels > half_pixels && half_pixels > 0,
        "Full, half and empty reservoirs are not distinct at native gameplay size");
    const RectangleI bounds = foreground_delta_bounds(full, blank);
    save(foreground_crop(full, bounds), output / (stem + "-full-detail.ppm"));
    save(foreground_crop(half, bounds), output / (stem + "-half-detail.ppm"));
    save(foreground_crop(empty, bounds), output / (stem + "-empty-detail.ppm"));
    records << stem << ',' << scale << ',' << static_cast<int>(tool.type()) << ',' <<
        tool.visual_pose() << ',' << full_pixels << ',' << half_pixels << '\n';
}

void supply_paid_cycle(Application& app, const Canvas& canvas, SupplyWeapon& tool,
    const Viewport& viewport, float scale, const GameTuning& tuning,
    const std::filesystem::path& output, const std::string& stem)
{
    const RectangleI pane = viewport.pixel_rect();
    const Camera camera(tool.draw_pos() - Vector2F(static_cast<float>(pane.width) / (2 * scale),
        static_cast<float>(pane.height) / (2 * scale)), scale);
    const auto draw = [&] { return supply_fixture(app, canvas, &tool, viewport, camera); };
    tool.set_motion_presentation(nullptr);
    tool.reset_ammo();
    const Frame full = draw();
    const Frame blank = supply_fixture(app, canvas, nullptr, viewport, camera);
    const RectangleI bounds = foreground_delta_bounds(full, blank);
    PlayerInputData input;
    input.primary_shoot = true;
    input.shoot_direction_requested = true;
    input.shoot_angle = tool.rotation();
    input.shoot_direction = Vector2F::unit_vec_from_angle(input.shoot_angle);
    bool captured_half = false;
    int paid = 0;
    const float cost = tuning.weapon(tool.type()).details.ammo_usage;
    for (int tick = 0; tick < 3000 && tool.ammo() >= cost; ++tick)
    {
        const float previous = tool.ammo();
        auto projectiles = tool.update_and_get_projectiles(input, Vector2F(400, 400), Vector2F::ZERO, true, dt);
        if (!projectiles.empty())
        {
            ++paid;
            require(tool.ammo() < previous, "Paid supply fixture failed to spend ammunition");
        }
        if (!captured_half && tool.ammo() <= tuning.starting_ammo() * 0.5f)
        {
            const Frame half = draw();
            require(half.pixels != full.pixels, "Paid depletion did not lower the visible reservoir");
            save(foreground_crop(half, bounds), output / (stem + "-paid-half-detail.ppm"));
            captured_half = true;
        }
    }
    require(paid > 0 && captured_half && tool.ammo() < cost,
        "Paid supply fixture failed to reach its full-shot affordability boundary");
    const Frame depleted = draw();
    require(depleted.pixels != full.pixels, "Unaffordable tool retained a full reservoir");
    save(foreground_crop(depleted, bounds), output / (stem + "-paid-depleted-detail.ppm"));
    input.primary_shoot = false;
    bool captured_recovery = false;
    for (int tick = 0; tick < 600 && tool.ammo() < tuning.starting_ammo(); ++tick)
    {
        auto projectiles = tool.update_and_get_projectiles(input, Vector2F(400, 400), Vector2F::ZERO, true, dt);
        require(projectiles.empty(), "Supply regeneration fixture emitted an unpaid projectile");
        if (!captured_recovery && tool.ammo() >= tuning.starting_ammo() * 0.5f)
        {
            const Frame recovering = draw();
            require(recovering.pixels != depleted.pixels && recovering.pixels != full.pixels,
                "Regeneration supplied no intermediate reservoir state");
            save(foreground_crop(recovering, bounds), output / (stem + "-regenerating-detail.ppm"));
            captured_recovery = true;
        }
    }
    require(captured_recovery && tool.ammo() == tuning.starting_ammo() && draw().pixels == full.pixels,
        "Natural regeneration failed to restore the full reservoir");
    tool.set_ammo(0);
    require(draw().pixels != full.pixels, "Zero ammunition retained full supply artwork");
    tool.reset_ammo();
    require(draw().pixels == full.pixels, "Ammo reset failed to restore the original full supply artwork");
}

void supply_menu_checks(Application& app, const Canvas& canvas, const Presentation& presentation,
    const std::filesystem::path& output)
{
    app.resolution_manager()->set_resolution_exactly(Vector2I(1280, 720));
    std::ignore = app.renderer()->window_size_changed(1280, 720);
    const SpriteSheet* sheet = app.render_resources()->sprite_sheet("weapon_paint");
    const RectangleF box(100, 100, 140, 60);
    MenuIcon icon("supply-preview", "sprayer", box, app.render_resources(), Colour::white,
        &presentation.menu(), "weapon_paint", "pixel");
    const auto draw = [&](const std::string& supply)
    {
        Renderer& renderer = *app.renderer();
        renderer.begin_frame();
        renderer.set_view_count(1);
        DrawList list = renderer.view(0);
        list.set_viewport(app.viewport_manager()->fullscreen_viewport());
        list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point);
        canvas.draw(list, RectangleF(0, 0, 1280, 720));
        icon.draw(list);
        if (!supply.empty())
        {
            const AnimationStrip& strip = sheet->animation_strip(sheet->resolve_animation_strip(supply));
            const RectangleI& source = strip.frame_rect(strip.frame_count() - 1);
            const Vector2F source_size(static_cast<float>(source.width), static_cast<float>(source.height));
            const Vector2F fitted = source_size * std::min(box.width / source_size.x, box.height / source_size.y);
            sheet->draw(list, source, RectangleF(box.center() - fitted / 2, fitted), Colour::cyan);
        }
        return read(renderer);
    };
    icon.set_paint_colour(Colour::cyan);
    icon.set_sprite_frame("sniper", "sniper_paint");
    const Frame plain_sniper = draw("");
    for (const std::string name : { "sprayer", "mister" })
    {
        icon.set_sprite_frame(name, name + "_paint");
        const Frame empty = draw("");
        const Frame expected = draw(name + "_supply");
        icon.set_sprite_frame(name, name + "_paint", name + "_supply");
        const Frame full = draw("");
        require(full.pixels == expected.pixels && full.pixels != empty.pixels,
            "Menu weapon preview omitted or misregistered its full reservoir");
        require(full.pixels == draw("").pixels, "Frozen supply preview changed pixels");
        save(foreground_crop(full, RectangleI(90, 90, 160, 80)),
            output / ("supply-menu-" + name + "-detail.ppm"));
        icon.set_sprite_frame("sniper", "sniper_paint");
        require(draw("").pixels == plain_sniper.pixels,
            "Changing menu weapon retained the previous tool's reservoir");
    }
}

void supply_art_checks(Application& app, LevelBuilder& builder, const Canvas& canvas,
    const GameTuning& tuning, Presentation& presentation, PlaySettings& preferences,
    const std::filesystem::path& output)
{
    const RestoreFlag restore_reduced{ preferences.reduce_effects, preferences.reduce_effects };
    preferences.reduce_effects = false;
    supply_menu_checks(app, canvas, presentation, output);
    AudioResources audio;
    audio.add_sound_bank("paint_exe", SoundBank::silent());
    std::ofstream records(output / "supply.csv");
    records << "capture,camera_scale,tool,pose,full_pixels,half_pixels\n";
    for (int count : { 1, 4 })
    {
        for (const TeamColourPair palette : { TeamColourPair::blue_orange,
            TeamColourPair::pink_green, TeamColourPair::blue_yellow })
        {
            for (const WeaponType type : { WeaponType::sprayer, WeaponType::mister })
            {
                auto level = build(app, builder, count, palette, false, false, type);
                auto inputs = neutral_inputs();
                settle(*level, inputs);
                for (const Vector2I size : { Vector2I(1280, 720), Vector2I(1920, 1080), Vector2I(2560, 1440) })
                {
                    if (size.x != 1280 && (count != 4 || palette != TeamColourPair::blue_orange)) { continue; }
                    resize(app, *level, size);
                    const auto before = observation(*level);
#if COLOURWARS_DEV_TOOLS
                    const MatchFingerprint fingerprint = MatchFingerprint::of(*level, 0);
#endif
                    const Frame context = render(*level, *app.renderer());
                    require(context.pixels == render(*level, *app.renderer()).pixels,
                        "Frozen supply scene changed pixels");
                    const std::string prefix = "supply-" + std::to_string(count) + "p-" +
                        TeamColourTools::palette_key(palette) + "-" +
                        (type == WeaponType::sprayer ? "sprayer" : "mister") + "-" + std::to_string(size.x);
                    save(context, output / (prefix + "-context.ppm"));
                    for (int quadrant = 0; quadrant < 4; ++quadrant)
                    {
                        if (size.x != 1280 && quadrant != 0 && quadrant != 2) { continue; }
                        const int view = count == 1 ? 0 : quadrant;
                        const Player& player = level->player(view);
                        const Viewport& viewport = app.viewport_manager()->player_viewport(view);
                        SupplyWeapon tool(&tuning, player.team(), player.player_num(), view,
                            player.team_colour(), type, Vector2F(400, 400), app.render_resources(),
                            &audio, &preferences);
                        tool.set_motion_presentation(&presentation.tool_motion());
                        const float angle = PI * (-0.75f + 0.5f * static_cast<float>(quadrant));
                        for (const bool right : { false, true })
                        {
                            tool.aim(angle, right);
                            tool.pose(static_cast<ToolMotionPose>(quadrant));
                            const std::string stem = prefix + "-q" + std::to_string(quadrant + 1) +
                                (right ? "-right" : "-left");
                            supply_raster_case(app, canvas, tool, viewport, player.camera().scale,
                                tuning.starting_ammo(), preferences, output, stem, records);
                        }
                        if (palette == TeamColourPair::blue_orange && quadrant == 0 && size.x == 1280)
                        {
                            supply_paid_cycle(app, canvas, tool, viewport, player.camera().scale,
                                tuning, output, prefix);
                            // Turning mechanism animation off must retain the
                            // reservoir instead of hiding the information.
                            supply_raster_case(app, canvas, tool, viewport, player.camera().scale,
                                tuning.starting_ammo(), preferences, output, prefix + "-motion-off", records);
                        }
                    }
                    require(before == observation(*level), "Supply drawing changed public match state");
#if COLOURWARS_DEV_TOOLS
                    require(MatchFingerprint::difference(fingerprint, MatchFingerprint::of(*level, 0)).empty(),
                        "Supply drawing changed the replay fingerprint");
#endif
                }
            }
        }
    }
    require(records.good(), "Supply record write failed");
    std::cout << "PASS supply full/half/empty, authored-layer registration, static silhouette/hardware, "
        "native 1P/4P palettes/quadrants/facings/poses, reduced effects, motion-off, paid depletion, "
        "natural regeneration/reset and frozen pixels/state/fingerprint\n";
}
