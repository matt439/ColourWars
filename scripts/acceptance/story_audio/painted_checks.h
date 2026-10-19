// Included after the story-audio fixtures, within their anonymous namespace.
// Uses MovementCourse, movement_input and choice_plays from movement_checks.h
// and paint_surface and paint_contact from paint_checks.h.
// Use an actual rectangular projectile with an explicit splash footprint so
// boundary checks do not depend on a particular tool's authored shot size.
void paint_box(World& world, StructurePaintable& surface,
    const mattmath::RectangleF& box, PlayerTeam team = PlayerTeam::a)
{
    using namespace mattmath;
    const auto& tuning = world.tuning.projectile(ProjectileType::jet);
    const RectangleF supplied(box.x + tuning.details.col_rect_size.x / 2.0f,
        box.y + tuning.details.col_rect_size.y / 2.0f, box.width, box.height);
    ProjectileJet shot(supplied, Vector2F::ZERO, team, 0, Colour::blue,
        &world.render, &tuning);
    paint_contact(shot, surface);
}

StructurePaintable& painted_course_surface(MovementCourse& course,
    const mattmath::RectangleF& rectangle,
    const PaintableFaces& faces = { false, true, false, false },
    std::span<const mattmath::RectangleF> obstacles = {})
{
    auto* surface = course.scene.add(std::make_unique<StructurePaintable>(
        "sprite_sheet_1", "square_white_4", rectangle,
        std::make_unique<mattmath::RectangleF>(rectangle), &course.world.render,
        &course.world.audio, &course.world.preferences, &course.tuning.paint(),
        CollisionObjectType::structure_paintable, TeamColour{}, faces, obstacles));
    course.scene.end_tick();
    return *surface;
}

void await_painted_landing(MovementCourse& course)
{
    const Player& player = *course.players.front();
    for (int tick = 0; tick < 600; ++tick)
    {
        const float next_speed = std::min(player.velocity().y + course.tuning.player().gravity * weapon_step,
            course.tuning.player().max_velocity.y);
        if (next_speed > 0.0f && player.rectangle().bottom() + next_speed * weapon_step >= 0.0f) { return; }
        course.tick();
    }
    throw std::runtime_error("Painted landing fixture never reached its next floor contact");
}

void check_painted_support_query(World& world)
{
    using namespace mattmath;
    const float pitch = world.tuning.paint().size.x;
    auto tiles = paint_surface(world, { 0, 0, pitch * 4.0f, 100 });
    require(tiles->top_paint_team(0.0f) == PlayerTeam::none,
        "Unpainted top reported painted support");
    paint_box(world, *tiles, { 0.1f, 0.1f, 0.5f, 0.5f });
    paint_box(world, *tiles, { pitch + 0.1f, 0.1f, 0.5f, 0.5f }, PlayerTeam::b);
    require(tiles->top_paint_team(0.0f) == PlayerTeam::a &&
        tiles->top_paint_team(pitch - 0.001f) == PlayerTeam::a &&
        tiles->top_paint_team(pitch) == PlayerTeam::b &&
        tiles->top_paint_team(pitch * 2.0f) == PlayerTeam::none &&
        tiles->top_paint_team(-0.001f) == PlayerTeam::none &&
        tiles->top_paint_team(pitch * 4.0f) == PlayerTeam::none,
        "Top paint query lost exact half-open tile or surface boundaries");

    auto side_only = paint_surface(world, { 0, 0, 100, 100 }, { true, false, true, true });
    paint_box(world, *side_only, { 0, 0, 100, 100 });
    require(side_only->paint_total().team_a > 0.0f &&
        side_only->top_paint_team(50.0f) == PlayerTeam::none,
        "Painted side/bottom became top support when the top face was disabled");
    auto other_faces = paint_surface(world, { 0, 0, 100, 100 }, { true, true, true, true });
    paint_box(world, *other_faces, { 0.1f, 50.0f, 0.5f, 0.5f });
    paint_box(world, *other_faces, { 50.0f, 99.0f, 0.5f, 0.5f });
    require(other_faces->paint_total().team_a > 0.0f &&
        other_faces->top_paint_team(0.1f) == PlayerTeam::none &&
        other_faces->top_paint_team(50.0f) == PlayerTeam::none,
        "Painted side/bottom leaked through an enabled neutral top");

    MovementCourse clipped(world, world.tuning);
    const RectangleF obstacle(30, -50, 40, 50);
    auto& surface = painted_course_surface(clipped, { 0, 0, 100, 100 },
        { false, true, false, false }, std::span<const RectangleF>(&obstacle, 1));
    paint_box(world, surface, { 0, 0, 100, 100 });
    require(surface.top_paint_team(29.999f) == PlayerTeam::a &&
        surface.top_paint_team(30.0f) == PlayerTeam::none &&
        surface.top_paint_team(69.999f) == PlayerTeam::none &&
        surface.top_paint_team(70.0f) == PlayerTeam::a &&
        surface.top_paint_team(100.0f) == PlayerTeam::none,
        "Clipped top paint query filled buried intervals or lost an exposed edge");
}

void check_painted_player_contacts(World& world)
{
    using namespace mattmath;
    const auto& sounds = world.tuning.player().movement_sounds;
    world.preferences.sfx_volume = 40;
    for (const auto team : { PlayerTeam::none, PlayerTeam::a, PlayerTeam::b })
    {
        MovementCourse course(world, world.tuning);
        course.add();
        auto& surface = painted_course_surface(course, { -1000, 0, 4000, 100 });
        if (team != PlayerTeam::none) { paint_box(world, surface, surface.shape()->bounding_box(), team); }
        course.land();
        world.begin = recorded_sounds(world.device).size();
        for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(1.0f)); }
        require(team == PlayerTeam::none ?
            choice_plays(world, sounds.step) > 0 && choice_plays(world, sounds.step_painted) == 0 :
            choice_plays(world, sounds.step_painted) > 0 && choice_plays(world, sounds.step) == 0,
            "Actual Player did not distinguish neutral and friendly/enemy painted top support");
    }

    // A straddled seam is one completed support footprint. Dispatch can skip
    // the second floor after the first removes penetration, so its measured
    // contact must still participate regardless of object insertion order.
    for (bool wet_first : { false, true })
    {
        MovementCourse course(world, world.tuning);
        course.add(-world.tuning.player().size.x / 2.0f);
        const auto add_dry = [&] { course.surface({ -1000, 0, 1000, 100 }); };
        const auto add_wet = [&]
        {
            auto& surface = painted_course_surface(course, { 0, 0, 1000, 100 });
            paint_box(world, surface, surface.shape()->bounding_box());
        };
        if (wet_first) { add_wet(); add_dry(); }
        else { add_dry(); add_wet(); }
        course.land();
        world.begin = recorded_sounds(world.device).size();
        course.tick(movement_input(0.0f, true));
        course.land();
        require(choice_plays(world, sounds.step_painted) == 1 && choice_plays(world, sounds.land) == 0,
            "Dry/wet seam landing depended on which floor resolved first");
    }

    // Subtracting the player's height from an authored fractional floor and
    // adding it back need not reproduce the floor's exact float. Both landing
    // and walking must still recognise its painted support after resolution.
    MovementCourse fractional(world, world.tuning);
    constexpr float fractional_top = 0.1f;
    fractional.add(0.0f, fractional_top);
    auto& fractional_surface = painted_course_surface(fractional, { -1000, fractional_top, 4000, 100 });
    paint_box(world, fractional_surface, fractional_surface.shape()->bounding_box());
    fractional.land();
    world.begin = recorded_sounds(world.device).size();
    fractional.tick(movement_input(0.0f, true));
    fractional.land();
    require(choice_plays(world, sounds.step_painted) == 1 && choice_plays(world, sounds.land) == 0,
        "Fractional top lost painted landing support through float cancellation");
    world.begin = recorded_sounds(world.device).size();
    for (int tick = 0; tick < 60; ++tick) { fractional.tick(movement_input(1.0f)); }
    require(choice_plays(world, sounds.step_painted) > 0 && choice_plays(world, sounds.step) == 0,
        "Fractional top lost painted walking support through float cancellation");

    // Paint on another surface, or on the current support's underside, does not
    // describe the player's resolved top contact.
    for (bool wrong_surface : { false, true })
    {
        MovementCourse course(world, world.tuning);
        course.add();
        auto& surface = painted_course_surface(course,
            { -1000, wrong_surface ? 100.0f : 0.0f, 4000, 100 },
            wrong_surface ? PaintableFaces{ false, true, false, false } :
                PaintableFaces{ true, false, true, true });
        paint_box(world, surface, surface.shape()->bounding_box());
        if (wrong_surface) { course.floor(); }
        course.land();
        world.begin = recorded_sounds(world.device).size();
        for (int tick = 0; tick < 60; ++tick) { course.tick(movement_input(1.0f)); }
        require(choice_plays(world, sounds.step) > 0 && choice_plays(world, sounds.step_painted) == 0,
            "Player selected paint from another surface or a non-supporting face");
    }

    // A clipped top can still provide physical support. Its removed scoring
    // strip must remain dry even though exposed strips on either side are wet.
    MovementCourse clipped(world, world.tuning);
    clipped.add();
    const RectangleF obstacle(-100, -50, 2000, 50);
    auto& clipped_surface = painted_course_surface(clipped, { -1000, 0, 4000, 100 },
        { false, true, false, false }, std::span<const RectangleF>(&obstacle, 1));
    paint_box(world, clipped_surface, clipped_surface.shape()->bounding_box());
    clipped.land();
    world.begin = recorded_sounds(world.device).size();
    for (int tick = 0; tick < 60; ++tick) { clipped.tick(movement_input(1.0f)); }
    require(choice_plays(world, sounds.step) > 0 && choice_plays(world, sounds.step_painted) == 0,
        "Player treated a clipped top interval as painted support");

    // A later wall separation changes the footprint after its floor callback.
    // Read the final midpoint rather than caching the paint at first contact.
    for (bool ends_painted : { false, true })
    {
        MovementCourse course(world, world.tuning);
        Player& player = course.add();
        auto& surface = painted_course_surface(course, { -1000, 0, 4000, 100 });
        const float width = world.tuning.player().size.x;
        paint_box(world, surface, { ends_painted ? width * 2.0f : 0.0f,
            0.0f, width, world.tuning.paint().thickness });
        course.land();
        course.tick(movement_input(0.0f, true));
        await_painted_landing(course);
        world.begin = recorded_sounds(world.device).size();
        player.set_player_input({});
        course.scene.update(weapon_step);
        course.scene.resolve();
        const RectangleF wall_rectangle(-width, -1000, width, 1000);
        Structure wall("sprite_sheet_1", "square_white_4", wall_rectangle,
            std::make_unique<RectangleF>(wall_rectangle), &world.render, CollisionObjectType::structure);
        player.on_contact(wall, { -1.0f, 0.0f }, width * 2.0f);
        player.end_contacts(course.scene.contacts());
        player.update_prev_rectangle();
        course.scene.end_tick();
        require(ends_painted ?
            choice_plays(world, sounds.step_painted) == 1 && choice_plays(world, sounds.land) == 0 :
            choice_plays(world, sounds.step_painted) == 0 && choice_plays(world, sounds.land) == 1,
            "Landing used paint beneath an earlier footprint instead of the final resolved midpoint");
    }

    // Painting can arrive on either side of the Player's support callback in
    // the same contact phase. Selection belongs after both callbacks.
    for (bool paint_first : { false, true })
    {
        MovementCourse course(world, world.tuning);
        Player& player = course.add();
        auto& surface = painted_course_surface(course, { -1000, 0, 4000, 100 });
        course.land();
        course.tick(movement_input(0.0f, true));
        await_painted_landing(course);
        world.begin = recorded_sounds(world.device).size();
        player.set_player_input({});
        course.scene.update(weapon_step);
        if (paint_first) { paint_box(world, surface, surface.shape()->bounding_box()); }
        course.scene.resolve();
        if (!paint_first) { paint_box(world, surface, surface.shape()->bounding_box()); }
        player.end_contacts(course.scene.contacts());
        player.update_prev_rectangle();
        course.scene.end_tick();
        require(choice_plays(world, sounds.step_painted) == 1 && choice_plays(world, sounds.land) == 0,
            "Landing paint selection depended on projectile/Player contact callback order");
    }
}

void check_painted_audio_budget(World& world)
{
    const auto& sounds = world.tuning.player().movement_sounds;
    SoundBank* bank = world.audio.sound_bank(world.tuning.player().sound_bank);
    MovementAudio audio(&sounds, bank, &world.preferences, 0);
    const auto contact = [&](float distance, float dt, bool wet)
    {
        audio.end_contacts(true, true, true, 0.0f, distance, 0.0f, 0.0f, dt, wet);
    };
    world.preferences.sfx_volume = 40;
    world.begin = recorded_sounds(world.device).size();
    contact(0.0f, weapon_step, false);
    contact(sounds.stride_distance * 0.6f, sounds.min_step_interval, false);
    contact(sounds.stride_distance * 0.5f, weapon_step, true);
    require(choice_plays(world, sounds.step_painted) == 1 && choice_plays(world, sounds.step) == 0,
        "Crossing from dry to wet discarded or duplicated accumulated stride distance");
    contact(sounds.stride_distance, 0.001f, false);
    require(choice_plays(world, sounds.step) == 0,
        "Changing surface bypassed the shared footstep cooldown");
    contact(sounds.stride_distance, sounds.min_step_interval, true);
    require(choice_plays(world, sounds.step_painted) == 2 && choice_plays(world, sounds.step) == 0,
        "Dry/wet contacts maintained separate stride or cooldown budgets");
    for (float dt : { 0.0f, -1.0f }) { contact(sounds.stride_distance * 100.0f, dt, true); }
    require(choice_plays(world, sounds.step_painted) == 2,
        "Nonpositive contacts emitted painted footsteps");
    contact(sounds.stride_distance * 100.0f, 100.0f, true);
    contact(0.1f, sounds.min_step_interval, true);
    require(choice_plays(world, sounds.step_painted) == 3,
        "Long painted stride burst steps or retained a catch-up backlog");

    // All four seats start on their own deterministic variant, then cycle only
    // when an audible wet contact is requested.
    world.begin = recorded_sounds(world.device).size();
    std::vector<MovementAudio> seats;
    for (int seat = 0; seat < 4; ++seat)
    {
        seats.emplace_back(&sounds, bank, &world.preferences, seat);
        seats.back().end_contacts(true, true, true, 0, 0, 0, 0, weapon_step, true);
        seats.back().end_contacts(true, true, true, 0, sounds.stride_distance,
            0, 0, sounds.min_step_interval, true);
    }
    require(choice_plays(world, sounds.step_painted) == 4,
        "Four painted walkers shared a stride/cooldown history");
    for (const auto& sound : sounds.step_painted)
    {
        require(world.wave_plays(sound.name) == 1, "Painted walkers shared their initial variant");
        world.wave_volume_is(sound, 0.4f);
    }
    for (size_t step = 1; step < sounds.step_painted.size(); ++step)
    {
        seats.front().end_contacts(true, true, true, 0, sounds.stride_distance,
            0, 0, sounds.min_step_interval, true);
        require(world.wave_plays(sounds.step_painted[step].name) == 2,
            "Painted footsteps skipped or repeated their deterministic variants");
    }
    world.preferences.sfx_volume = 0;
    world.begin = recorded_sounds(world.device).size();
    seats.front().end_contacts(true, true, true, 0, sounds.stride_distance,
        0, 0, sounds.min_step_interval, true);
    world.wave_volume_is(sounds.step_painted.front(), 0.0f);
}

void check_painted_landings(World& world)
{
    const auto& sounds = world.tuning.player().movement_sounds;
    world.preferences.sfx_volume = 40;
    MovementCourse course(world, world.tuning);
    Player& player = course.add();
    auto& surface = painted_course_surface(course, { -1000, 0, 4000, 100 });
    paint_box(world, surface, surface.shape()->bounding_box(), PlayerTeam::b);
    world.begin = recorded_sounds(world.device).size();
    course.land();
    require(choice_plays(world, sounds.step_painted) == 0,
        "Spawn settling played a painted landing");
    course.tick(movement_input(0.0f, true));
    course.land();
    const float gain = std::clamp(course.downward_speed.front() / sounds.landing_full_speed,
        sounds.landing_min_gain, 1.0f);
    require(choice_plays(world, sounds.step_painted) == 1 && choice_plays(world, sounds.land) == 0 &&
        choice_plays(world, sounds.step) == 0 &&
        close_to(world.last_wave_volume(sounds.step_painted.front()),
            sounds.step_painted.front().volume * gain * 0.4f),
        "Painted landing did not replace the dry cue with the same impact gain");
    course.tick({}, 0.0f);
    course.tick({}, -weapon_step);
    course.tick();
    require(choice_plays(world, sounds.step_painted) == 1,
        "Stationary or nonpositive painted support repeated its landing");

    // A buffered landing can launch again in the same contact phase, while
    // still producing exactly one wet impact and no walked step.
    world.begin = recorded_sounds(world.device).size();
    course.tick(movement_input(0.0f, true));
    bool queued = false;
    for (int tick = 0; tick < 150 && choice_plays(world, sounds.step_painted) == 0; ++tick)
    {
        const bool press = !queued && player.velocity().y > 0.0f && player.rectangle().bottom() > -25.0f;
        course.tick(movement_input(0.0f, press));
        queued = queued || press;
    }
    require(queued && player.velocity().y < 0.0f && choice_plays(world, sounds.step_painted) == 1 &&
        choice_plays(world, sounds.land) == 0 && choice_plays(world, sounds.step) == 0,
        "Buffered painted landing/takeoff missed its impact or invented an ordinary step");
    world.preferences.sfx_volume = 0;
    world.begin = recorded_sounds(world.device).size();
    course.land();
    require(choice_plays(world, sounds.step_painted) == 1, "Buffered painted takeoff did not rearm landing");
    for (const auto& sound : sounds.step_painted)
    {
        if (world.wave_plays(sound.name) > 0) { world.wave_volume_is(sound, 0.0f); }
    }

    course.tick(movement_input(0.0f, true));
    await_painted_landing(course);
    world.begin = recorded_sounds(world.device).size();
    course.tick({}, weapon_step, true);
    require(choice_plays(world, sounds.step_painted) == 0 && choice_plays(world, sounds.land) == 0,
        "A fatal contact played painted landing feedback before the death state changed");
}
