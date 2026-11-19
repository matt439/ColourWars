struct SetupStatusSample
{
    int row;
    std::wstring_view words;
    // Null for Team Select. On Weapon Select this is the expected description,
    // which changes between the tool and the sub steps.
    const std::wstring* description = nullptr;
};

void check_setup_status_pixels(Application& app, const Presentation& presentation,
    const Frame& actual, bool pseudo_localized, std::initializer_list<SetupStatusSample> statuses)
{
    const auto& style = presentation.menu();
    const auto& look = presentation.main_menu();
    const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
    const Camera camera = Camera::frame(RectangleF(Vector2F::zero, menu_consts::design_resolution), viewport);
    Renderer& renderer = *app.renderer();
    renderer.begin_frame();
    renderer.set_view_count(1);
    DrawList list = renderer.view(0);
    list.set_viewport(viewport);
    list.set_camera(camera);
    list.set_filter(TextureFilter::point);
    const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
    sheet->draw(list, sheet->resolve_sprite_frame(style.fill_frame), camera.visible_rectangle(viewport), Colour::white);
    list.set_filter(TextureFilter::linear);

    std::vector<RectangleI> samples;
    for (const SetupStatusSample& status : statuses)
    {
        const bool weapon = status.description != nullptr;
        const auto& select = weapon ? look.weapon_select : look.team_select;
        const Vector2F row = style.layout.position + Vector2F(0.0f, select.spacing.y * static_cast<float>(status.row + 2));
        Vector2F position = row + Vector2F(select.spacing.x * 2.0f + select.size.x * 3.0f + look.setup_focus_padding.x, 0.0f);
        const std::string& font = weapon ? look.weapon_description_font : style.detail_font;
        if (weapon)
        {
            const WrappedTextDropShadow description("expected-status-description", *status.description, font,
                row + Vector2F(select.spacing.x * 2.0f + look.weapon_description_x_offset, 0.0f),
                app.render_resources(), look.weapon_description_max_width, Colour::black, Colour::black,
                look.weapon_description_shadow_offset);
            const float name_top = row.y + select.size.y + style.icon_padding.y + style.rule_height +
                std::min(0.0f, style.detail_shadow_offset.y);
            position = Vector2F(description.bounds().x, std::max(description.bounds().bottom(), name_top));
        }
        std::wstring words(status.words);
        if (pseudo_localized) { words = L"[" + words + L"]"; }
        const UiText expected("expected-setup-status", words, font, position, app.render_resources(), look.art.ink);
        const Vector2F paper_size = weapon ? look.weapon_box_size : look.team_box_size;
        const RectangleF paper((Vector2F(menu_consts::design_resolution.x, style.footer_y) - paper_size) / 2.0f, paper_size);
        require(paper.contains(expected.bounds()) && expected.bounds().bottom() <= row.y + select.spacing.y,
            "Expected setup status escaped the paper or roster row");
        samples.push_back(pixel_bounds(camera.calculate_view_rectangle(expected.bounds())));
        expected.draw(list);
    }
    const Frame expected = read(renderer);
    for (const RectangleI& sample : samples)
    {
        size_t ink_pixels = 0;
        for (int y = sample.y; y < sample.bottom(); ++y)
        {
            for (int x = sample.x; x < sample.right(); ++x)
            {
                const size_t offset = pixel_offset(expected, x, y);
                // Deep ink is independent of the underlying paper shading.
                if (srgb_luminance(expected, offset) >= 0.045f) { continue; }
                require(srgb_luminance(actual, offset) < 0.15f, "Actual setup row lost the expected status lettering");
                ++ink_pixels;
            }
        }
        require(ink_pixels > 10, "Independent setup status did not rasterize visible lettering");
    }
}
