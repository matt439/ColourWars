// Included inside painter_visual.cpp's private namespace, before the Options
// defaults and paused Options leaves: one independent Options page reference.

// The page the reference draws: the focused row, the four values, the status
// line and, on a paused page, the palette-timing caption beside the heading.
struct OptionsReference
{
    int selected = 0;
    std::array<std::wstring, 4> values;
    bool resolution_disabled = false;
    std::wstring status;
    bool status_warning = false;
    // Empty on the main menu's Options page, which has no caption.
    std::wstring caption;
    std::string caption_font;
    Vector2F caption_position;
    // true draws the ordinary probe Canvas under the menu scrim. false matches
    // a paused page with covers_screen()==true: begin_frame's clear stays in
    // the aspect-ratio margins beneath one menu scrim.
    bool canvas_underlay = false;
};

// The independent shipped Options labels, in row order.
constexpr std::array<const wchar_t*, 8> options_reference_labels{ L"Resolution", L"Fullscreen", L"Vsync",
    L"Team palette", L"Controls / audio", L"Restore defaults", L"Apply", L"Back" };

// A settled independent Options frame. Call after motion has settled, or while
// Reduce effects suppresses it. It uses the independently implemented
// setup_paper_reference and never draws production menu, paper or icon widgets.
Frame options_reference(Application& app, const Canvas& canvas, const Presentation& captured,
    const OptionsReference& page)
{
    const auto& style = captured.menu(); const auto& look = captured.main_menu();
    auto art = look.art; art.focus_padding = look.setup_focus_padding;
    art.shadow_offset = look.setup_shadow_offset;
    const RectangleF design(Vector2F::zero, menu_consts::design_resolution);
    const RectangleF paper((Vector2F(menu_consts::design_resolution.x, style.footer_y) -
        look.options_box_size) / 2.0f, look.options_box_size);
    const float row_height = look.options_spacing.y - art.focus_padding.y * 2.0f;
    const float value_width = paper.right() - art.shadow_offset - art.focus_padding.x - look.options_value_x;
    require(row_height > 0.0f && value_width > 0.0f,
        "Independent Options has no fitted value budget");
    std::vector<std::unique_ptr<UiText>> text;
    std::array<RectangleF, 8> bounds;
    const auto add = [&](const std::wstring& copy, const std::string& font,
        const Vector2F& position, const Colour& colour)
    {
        auto widget = std::make_unique<UiText>("independent-options", copy, font,
            position, app.render_resources(), colour);
        auto* result = widget.get(); text.push_back(std::move(widget)); return result;
    };
    const auto* heading = add(L"Options", style.heading_font, look.options_heading_position, art.ink);
    require(paper.contains(heading->bounds()), "Independent Options heading escapes paper");
    const UiText* caption = nullptr;
    if (!page.caption.empty())
    {
        caption = add(page.caption, page.caption_font, page.caption_position, art.ink);
        require(paper.contains(caption->bounds()) && heading->bounds().right() <= caption->bounds().x,
            "Independent Options heading and palette timing caption overlap or escape paper");
    }
    for (int index = 0; index < 8; ++index)
    {
        const Vector2F position = look.options_position +
            Vector2F(0.0f, look.options_spacing.y * static_cast<float>(index));
        auto* label = add(options_reference_labels[static_cast<size_t>(index)], look.options_font, position,
            index == 0 && page.resolution_disabled ? art.disabled_ink :
                index == page.selected ? art.focus_ink : art.ink);
        if (index < 4)
        {
            const float label_width = look.options_value_x - position.x - art.focus_padding.x;
            require(label_width > 0.0f, "Independent Options has no label budget");
            const RectangleF raw_label = label->bounds();
            label->set_scale(std::min({ 1.0f, label_width / raw_label.width,
                row_height / raw_label.height }));
            auto* value = add(page.values[static_cast<size_t>(index)], look.options_font,
                Vector2F(look.options_value_x, position.y), art.ink);
            const RectangleF raw_value = value->bounds();
            require(raw_value.width > 0.0f && raw_value.height > 0.0f,
                "Independent Options needs a nonempty value");
            value->set_scale(std::min({ 1.0f, value_width / raw_value.width,
                row_height / raw_value.height }));
            bounds[static_cast<size_t>(index)] = RectangleF::union_of(label->bounds(), value->bounds());
        }
        else { bounds[static_cast<size_t>(index)] = label->bounds(); }
        RectangleF padded = bounds[static_cast<size_t>(index)]; padded.inflate(art.focus_padding);
        require(paper.contains(padded) &&
            (index == 0 || padded.y >= bounds[static_cast<size_t>(index - 1)].bottom() + art.focus_padding.y),
            "Independent Options row/card escapes paper or reaches the preceding row");
        if (index == 0)
        {
            require(heading->bounds().bottom() <= padded.y && (caption == nullptr || caption->bounds().bottom() <= padded.y),
                "Independent Options heading/caption reaches its first card");
        }
    }
    auto* status = add(page.status, captured.prompt_bar().font, look.options_status_position,
        page.status_warning ? look.options_warning_colour : art.focus_ink);
    const float status_width = paper.right() - art.shadow_offset - art.focus_padding.x -
        look.options_status_position.x;
    if (!page.status.empty())
    {
        require(status_width > 0.0f, "Independent Options has no status width");
        if (status->bounds().width > status_width) { status->set_scale(status_width / status->bounds().width); }
        RectangleF back = bounds.back(); back.inflate(art.focus_padding);
        require(paper.contains(status->bounds()) && !back.intersects(status->bounds()),
            "Independent Options status escapes paper or overlaps Back");
    }

    Renderer& renderer = *app.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
    const Viewport viewport = app.viewport_manager()->fullscreen_viewport();
    const Camera camera = Camera::frame(design, viewport);
    DrawList list = renderer.view(0); list.set_viewport(viewport);
    list.set_camera(Camera::default_camera); list.set_filter(TextureFilter::point);
    if (page.canvas_underlay)
    {
        canvas.draw(list, RectangleF(Vector2F::zero, app.resolution_manager()->resolution_vec()));
    }
    list.set_camera(camera);
    const SpriteSheet* sheet = app.render_resources()->sprite_sheet(style.sprite_sheet);
    const auto texel = sheet->resolve_sprite_frame(style.fill_frame);
    sheet->draw(list, texel, camera.visible_rectangle(viewport), style.scrim_colour);
    sheet->draw(list, texel, design, look.home_background_colour);
    setup_paper_reference(list, *app.render_resources(), style, art, paper,
        bounds[static_cast<size_t>(page.selected)]);
    sheet->draw(list, texel, RectangleF(0.0f, style.footer_y, menu_consts::design_resolution.x,
        menu_consts::design_resolution.y - style.footer_y), style.scrim_colour);
    sheet->draw(list, texel, RectangleF(style.brand_position.x, style.footer_y,
        menu_consts::design_resolution.x - style.brand_position.x * 2.0f, style.rule_height),
        style.panel_edge_colour);
    list.set_filter(TextureFilter::linear);
    for (const auto& widget : text) { widget->draw(list); }
    const auto& footer = captured.prompt_bar();
    const colourwars::UiTextDropShadow prompt("independent-options-prompt",
        L"Left/Right Change    A/Enter Select    B/Esc Back", footer.font, footer.position,
        app.render_resources(), footer.colour, footer.shadow_colour, footer.shadow_offset);
    prompt.draw(list);
    return read(renderer);
}
