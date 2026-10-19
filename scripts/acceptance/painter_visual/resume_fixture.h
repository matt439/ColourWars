// Included inside painter_visual.cpp's private namespace. Shared resume
// fixture helpers: the probe's root state, stack frame, observation and
// paper capture, used by the resume, music-flow, debug-frame and later menu
// headers.
class ResumeProbeRoot final : public State
{
public:
    void init() override {}
    void update(float) override {}
    void draw(Renderer&) const override {}
};

Frame resume_stack_frame(Application& app)
{
    app.renderer()->begin_frame();
    static_cast<StateContext&>(app).draw(*app.renderer());
    return read(*app.renderer());
}

std::vector<float> resume_observation(const Level& level)
{
    auto result = observation(level);
    result.push_back(static_cast<float>(level.projectile_count()));
    for (int view = 0; view < level.player_count(); ++view)
    {
        result.push_back(level.player(view).primary_weapon().ready_to_shoot() ? 1.0f : 0.0f);
    }
    return result;
}

void resume_capture(Application& app, GameLevel& game, Presentation& presentation,
    const std::filesystem::path& output, const std::string& stem)
{
    const Level& level = game.level();
    const auto before = resume_observation(level);
    const float remaining = game.resume_remaining();
    require(remaining > 0.0f, "Resume capture was taken outside its count-in");
    const Frame context = resume_stack_frame(app);
    require(context.pixels == resume_stack_frame(app).pixels,
        "Frozen GameLevel resume drawing changed pixels");
    save(context, output / (stem + "-context.ppm"));

    // Isolate the shared paper overlay from Debug's status strip and prove
    // every changed pixel stays in the authored shared card and its shadow.
    const Frame plain = render(level, *app.renderer());
    app.renderer()->begin_frame();
    level.draw(*app.renderer());
    level.draw_resume_countdown(*app.renderer(), remaining);
    const Frame overlay = read(*app.renderer());
    const Vector2F size = app.resolution_manager()->resolution_vec();
    const auto& look = presentation.match();
    const float scale = std::min(size.x / look.reference_size.x, size.y / look.reference_size.y);
    const Vector2F card_size = look.countdown_size * scale;
    const float shadow = look.countdown_art.shadow_offset * scale + 3.0f;
    const RectangleF card((size.x - card_size.x) / 2.0f - shadow,
        size.y * look.countdown_center_y - card_size.y / 2.0f - shadow,
        card_size.x + shadow * 2.0f, card_size.y + shadow * 2.0f);
    const RectangleI delta = foreground_delta_bounds(overlay, plain);
    require(delta.width > 0 && delta.height > 0 &&
        RectangleI(card).contains(delta), "Shared resume paper escaped its authored card bounds");
    save(foreground_crop(context, RectangleI(card)), output / (stem + "-detail.ppm"));
    require(before == resume_observation(level) && game.resume_remaining() == remaining,
        "Resume drawing advanced simulation, cosmetics or its real-time clock");
}
