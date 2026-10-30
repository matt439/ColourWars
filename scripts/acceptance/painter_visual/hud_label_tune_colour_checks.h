// Paths49/50: one prepared HUD, three foreground roles and identity-only shadow.
// Bounds/backgrounds are pinned source hypotheses, not fitted raster results.
enum class HudLabelSubject { foreground, shadow, pair };
enum class HudLabelReference { correct, straight, squared, reversed_order };
const wchar_t* hud_label_identity() { return L"P1  /  TEAM A\nKeyboard/mouse"; }
const char* hud_label_subject(HudLabelSubject subject)
{ return subject == HudLabelSubject::foreground ? "foreground" : subject == HudLabelSubject::shadow ? "shadow" : "pair"; }
std::array<RectangleI, 3> hud_label_rois()
{ return { RectangleI(133, 63, 380, 111), RectangleI(1561, 59, 125, 28), RectangleI(1561, 145, 97, 28) }; }
std::vector<float> hud_label_fixed_style(const HudPresentation& look, HudLabelSubject subject)
{
    auto copy = look;
    if (subject != HudLabelSubject::shadow) { copy.player_identity_colour = Colour(20, 20, 20); }
    if (subject != HudLabelSubject::foreground) { copy.player_identity_shadow_colour = Colour(0, 0, 0, 34); }
    auto values = hud_track_fixed_style(copy);
    const auto& track = look.meter_track_colour; values.insert(values.end(), { track.r, track.g, track.b, track.a });
    return values;
}
class HudLabelColourProbe
{
public:
    HudLabelColourProbe(Application& app, const Presentation& presentation, const std::filesystem::path& output,
        HudLabelSubject subject) : app_(app), presentation_(presentation), look_(presentation.hud()),
        identity_(presentation.player_identity()), expected_(look_), expected_identity_(identity_),
        hud_(app.render_resources(), &look_, &identity_), copied_(app.render_resources(), &expected_, &expected_identity_),
        canvas_(app.render_resources(), Colour(32, 64, 96)),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        pixel_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))), subject_(subject),
        fixed_(hud_label_fixed_style(look_, subject)), identity_fixed_(hud_track_identity_style(identity_)),
        names_(hud_track_resource_names(look_)), palette_(presentation.team_palette()), output_(output)
    {
        require(look_.player_identity_colour == Colour(20, 20, 20) &&
            look_.player_identity_shadow_colour == Colour(0, 0, 0, 34) &&
            look_.player_identity_font == "lato_black_36" && look_.meter_label_font == "lato_black_24" &&
            look_.timer_font == "lato_black_48" && look_.player_identity_reference_width == 1920 &&
            look_.reference_height == 1080 && look_.player_identity_offset == Vector2F(.03f, .05f) &&
            look_.player_identity_padding == Vector2F(16, 10) && look_.player_identity_shadow_offset == Vector2F(2, 2) &&
            look_.right_margin == .19f && look_.health_top_margin == .09f && look_.ammo_top_margin == .17f &&
            look_.meter_label_gap == 6 && look_.paper.keyline_width == 2 && look_.paper.shadow_offset == 4 &&
            look_.paper.paper == Colour(255, 245, 223) && look_.paper.shade == Colour(0, 0, 0, 85) &&
            identity_.enabled && identity_.ui_size == Vector2F(54, 42) && identity_.ui_gap == 12 &&
            look_.sprite_sheet == "sprite_sheet_1" && look_.fill_frame == "pixel" &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO,
            "HUD label fixture requires its pinned authored font/layout/paper/badge profile");
        const std::array<std::string, 3> names{ look_.player_identity_font, look_.meter_label_font, look_.timer_font };
        for (size_t i = 0; i < names.size(); ++i)
        { fonts_[i] = app_.render_resources()->resolve_sprite_font(names[i]); font_pointers_[i] = app_.render_resources()->font(fonts_[i]); }
        unchanged();
    }
    void unchanged() const
    {
        auto* resources = app_.render_resources();
        require(&look_ == &presentation_.hud() && &identity_ == &presentation_.player_identity() &&
            hud_label_fixed_style(look_, subject_) == fixed_ && hud_label_fixed_style(expected_, subject_) == fixed_ &&
            hud_track_identity_style(identity_) == identity_fixed_ && hud_track_identity_style(expected_identity_) == identity_fixed_ &&
            hud_track_resource_names(look_) == names_ && hud_track_resource_names(expected_) == names_ &&
            hud_track_same_palette(presentation_.team_palette(), palette_) &&
            resources->sprite_sheet(look_.sprite_sheet) == sheet_ &&
            &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) == &pixel_ &&
            pixel_.source_rectangle() == RectangleI(32, 0, 4, 4) && pixel_.origin() == Vector2F::ZERO &&
            app_.viewport_manager()->player_viewport(0).pixel_rect() == RectangleI(0, 0, 1920, 1080),
            "HUD label edit changed unselected UI, badge, clock, fills, palette, resource or viewport");
        const std::array<std::string, 3> names{ look_.player_identity_font, look_.meter_label_font, look_.timer_font };
        for (size_t i = 0; i < names.size(); ++i)
        { require(fonts_[i] == resources->resolve_sprite_font(names[i]) && font_pointers_[i] == resources->font(fonts_[i]), "HUD label font handle/pointer changed"); }
        // Lato Black 36/24 (95c1da3b): whole-pixel line spacings 58 and 38.
        require(font_pointers_[0]->line_spacing() == 58.0f && font_pointers_[1]->line_spacing() == 38.0f &&
            resources->measure_text(fonts_[0], hud_label_identity()) == Vector2F(382, 116) &&
            resources->measure_text(fonts_[1], L"HEALTH") == Vector2F(129, 38) &&
            resources->measure_text(fonts_[1], L"PAINT") == Vector2F(101, 38),
            "HUD source-derived case-sensitive font metrics changed");
        for (const auto pair : { std::pair<char32_t, char32_t>(U'P', U'p'), { U'H', U'h' }, { U'A', U'a' } })
        {
            require(font_pointers_[0]->find(pair.first) && font_pointers_[0]->find(pair.second) &&
                font_pointers_[0]->find(pair.first) != font_pointers_[0]->find(pair.second), "Font audit requires distinct ordinal uppercase/lowercase glyphs");
        }
        const auto positions = origins();
        require(positions[0] == Vector2F(123.59999847412109375f, 54) &&
            positions[1] == Vector2F(1555.199951171875f, 53.20000457763671875f) &&
            positions[2] == Vector2F(1555.199951171875f, 139.600006103515625f),
            "HUD label source float32 origins changed; decimal rounding is not permitted");
        const RectangleF identity_face(Vector2F(1920.0f * .03f, 54) - Vector2F(16, 10),
            Vector2F(382 + 66, 116) + Vector2F(32, 20));
        const float inside_bottom = identity_face.bottom() - 2;
        require(inside_bottom == 178 && static_cast<int>(inside_bottom - 6) == 172 &&
            static_cast<int>(inside_bottom - 4) == hud_label_rois()[0].bottom() && identity_face.right() - 2 - 6 > hud_label_rois()[0].right() &&
            look_.paper.shade.a * .12f == .04f, "HUD identity paper lower strip hypothesis changed");
        bounds(font_pointers_[0], hud_label_identity(), positions[0], Vector2F(2, 2), hud_label_rois()[0]);
        bounds(font_pointers_[1], L"HEALTH", positions[1], Vector2F::ZERO, hud_label_rois()[1]);
        bounds(font_pointers_[1], L"PAINT", positions[2], Vector2F::ZERO, hud_label_rois()[2]);
    }
    std::array<Vector2F, 3> origins() const
    {
        // Preserve source float32 operation order: the rounded product
        // 57.599998474121094f plus 66 is 123.599998474121094f (literal 123.6f).
        // Meter y subtracts each actual font measurement.
        const float x = 1920.0f - 1920.0f * .19f;
        return { Vector2F(1920.0f * .03f, 1080.0f * .05f) + Vector2F(54 + 12, 0),
            Vector2F(x, 1080.0f * .09f) - Vector2F(0, 38.0f + 6),
            Vector2F(x, 1080.0f * .17f) - Vector2F(0, 38.0f + 6) };
    }
    Frame actual()
    { return render([&](DrawList& list) { draw_hud(list, hud_); }); }
    Frame adoption()
    {
        expected_.player_identity_colour = look_.player_identity_colour;
        expected_.player_identity_shadow_colour = look_.player_identity_shadow_colour;
        return render([&](DrawList& list) { draw_hud(list, copied_); });
    }
    Frame raw(HudLabelReference mode = HudLabelReference::correct)
    {
        return render([&](DrawList& list)
        {
            for (const auto& box : hud_label_rois()) { rectangle(list, box, Colour(255, 245, 223)); }
            // Full multiline identity is NOT flat paper: only pass3's source
            // shade enters rows172/173. Literal crop material precedes text.
            rectangle(list, { 133, 172, 380, 2 }, Colour(0.0f, 0.0f, 0.0f, .04f));
            list.set_filter(TextureFilter::linear);
            const auto tint = [&](const Colour& colour, bool selected)
            {
                const float factor = !selected || mode == HudLabelReference::correct || mode == HudLabelReference::reversed_order ?
                    colour.a : mode == HudLabelReference::straight ? 1.0f : colour.a * colour.a;
                return Colour(colour.r * factor, colour.g * factor, colour.b * factor, colour.a);
            };
            const Colour foreground = tint(look_.player_identity_colour, subject_ != HudLabelSubject::shadow);
            const Colour shadow = tint(look_.player_identity_shadow_colour, subject_ != HudLabelSubject::foreground);
            const auto positions = origins();
            const auto draw_foreground = [&]()
            { list.draw_text(fonts_[0], hud_label_identity(), positions[0], foreground, 1.0f, 0.0f, Vector2F::ZERO, 0.0f); };
            const auto draw_shadow = [&]()
            { list.draw_text(fonts_[0], hud_label_identity(), positions[0] + Vector2F(2, 2), shadow, 1.0f, 0.0f, Vector2F::ZERO, 0.0f); };
            if (mode == HudLabelReference::reversed_order)
            { require(subject_ == HudLabelSubject::shadow, "HUD reversed order is an identity-shadow control"); draw_foreground(); draw_shadow(); }
            else { draw_shadow(); draw_foreground(); }
            list.draw_text(fonts_[1], L"HEALTH", positions[1], foreground, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
            list.draw_text(fonts_[1], L"PAINT", positions[2], foreground, 1.0f, 0.0f, Vector2F::ZERO, 0.0f);
        });
    }
    Frame observe()
    {
        const Frame observed = actual(), repeated = actual(), copied = adoption(), reference = raw();
        try
        {
            require(observed.pixels == repeated.pixels, "HUD label repeated draw changed complete RGBA");
            require(observed.pixels == copied.pixels, "Retained live HUD label adoption differs from copied HUD");
            compare_rois(observed, reference);
        }
        catch (...) { failure("observation", observed, reference); throw; }
        reference_ = reference; return observed;
    }
    std::array<size_t, 3> control(HudLabelReference mode)
    {
        const Frame wrong = raw(mode); std::array<size_t, 3> differences{};
        try
        {
            for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
            {
                const int region = at_region(x, y); const bool same = same_pixel(reference_, wrong, offset(x, y));
                if (region < 0) { require(same, "HUD wrong reference escaped the complete declared text support"); }
                else if (!same) { ++differences[static_cast<size_t>(region)]; }
            } }
            require(differences[0] > 0 && differences[0] <= 42180, "HUD identity negative tint/order must discriminate");
            if (subject_ == HudLabelSubject::foreground)
            { require(differences[1] > 0 && differences[1] <= 3500 && differences[2] > 0 && differences[2] <= 2716, "Both HEALTH and PAINT call sites must separately discriminate"); }
            else { require(differences[1] == 0 && differences[2] == 0, "Identity shadow reference changed an unshadowed meter label"); }
            if (mode == HudLabelReference::reversed_order)
            {
                const size_t at = offset(139, 91);
                require(reference_.pixels[at] == 20 && reference_.pixels[at + 1] == 20 && reference_.pixels[at + 2] == 20 &&
                    reference_.pixels[at + 3] == 255 && !same_pixel(reference_, wrong, at),
                    "HUD identity shadow lost its ordinal-font opaque overlap witness");
            }
        }
        catch (...) { failure("negative-control", reference_, wrong); throw; }
        return differences;
    }
    void locality(const Frame& observed, const Frame& baseline, bool changed) const
    {
        std::array<size_t, 3> differences{};
        for (int y = 0; y < 1080; ++y) { for (int x = 0; x < 1920; ++x)
        {
            const int region = at_region(x, y); const bool same = same_pixel(observed, baseline, offset(x, y));
            if (region < 0 || (subject_ == HudLabelSubject::shadow && region != 0))
            { require(same, "HUD label tint changed badge, clock, fill, paper or an unselected text region"); }
            else if (!same) { ++differences[static_cast<size_t>(region)]; }
        } }
        require(changed ? differences[0] > 0 : differences[0] == 0, "HUD identity selected response differs");
        if (subject_ == HudLabelSubject::foreground)
        { require(changed ? differences[1] > 0 && differences[2] > 0 : differences[1] == 0 && differences[2] == 0, "HUD foreground must reach HEALTH and PAINT independently"); }
    }
    static Frame compact(const Frame& frame, size_t region) { return foreground_crop(frame, hud_label_rois().at(region), 0); }
    size_t readbacks() const { return readbacks_; }
    void failure(const char* reason, const Frame& observed, const Frame& reference) const
    {
        const std::string stem = std::string("hud-label-failure-") + hud_label_subject(subject_) + '-' + reason;
        for (size_t region = 0; region < 3; ++region)
        {
            save(compact(observed, region), output_ / (stem + '-' + std::to_string(region) + "-actual.ppm"));
            save(compact(reference, region), output_ / (stem + '-' + std::to_string(region) + "-reference.ppm"));
        }
        const auto rgba = [&](const Frame& frame, const char* suffix)
        {
            std::ofstream file(output_ / (stem + suffix), std::ios::binary);
            file.write(reinterpret_cast<const char*>(frame.pixels.data()), static_cast<std::streamsize>(frame.pixels.size()));
            require(file.good(), "Cannot preserve HUD label failure RGBA");
        };
        rgba(observed, "-actual.rgba"); rgba(reference, "-reference.rgba");
        std::ofstream receipt(output_ / (stem + ".json"));
        receipt << "{\"scope\":\"source-hypothesis-failure-no-fitting\",\"subject\":\"" << hud_label_subject(subject_)
            << "\",\"frame_width\":1920,\"frame_height\":1080,\"readbacks\":" << readbacks_
            << ",\"identity_roi\":[133,63,380,111],\"health_roi\":[1561,59,125,28],\"paint_roi\":[1561,145,97,28]}\n";
        require(receipt.good(), "Cannot preserve HUD label failure receipt");
    }
private:
    static void bounds(const Font* font, const wchar_t* text, const Vector2F& origin, const Vector2F& extra, const RectangleI& roi)
    {
        float pen_x = 0, pen_y = 0;
        for (const wchar_t letter : std::wstring_view(text))
        {
            if (letter == L'\n') { pen_x = 0; pen_y += font->line_spacing(); continue; }
            const Glyph* glyph = font->find(static_cast<char32_t>(letter)); require(glyph != nullptr, "HUD label literal needs an ordinal glyph, not fallback");
            pen_x = std::max(0.0f, pen_x + glyph->x_offset);
            if (glyph->subrect.width > 1 || glyph->subrect.height > 1)
            {
                const float left = origin.x + pen_x, top = origin.y + pen_y + glyph->y_offset;
                require(left - 1 >= static_cast<float>(roi.x) && top - 1 >= static_cast<float>(roi.y) &&
                    left + static_cast<float>(glyph->subrect.width) + extra.x + 1 <= static_cast<float>(roi.right()) &&
                    top + static_cast<float>(glyph->subrect.height) + extra.y + 1 <= static_cast<float>(roi.bottom()),
                    "HUD label glyph/shadow support escaped source-declared one-pixel-expanded ROI");
            }
            pen_x += static_cast<float>(glyph->subrect.width) + glyph->x_advance;
        }
    }
    static void draw_hud(DrawList& list, const InterfaceGameplay& hud)
    { hud.draw_gameplay_interface_prepared(list, { 1920, 1080 }, 0, hud_label_identity(), .5f, .5f, 123.25f, Colour(6, 52, 204), false, true); }
    static int at_region(int x, int y)
    {
        const auto boxes = hud_label_rois();
        for (size_t i = 0; i < boxes.size(); ++i)
        { const auto& b = boxes[i]; if (x >= b.x && x < b.right() && y >= b.y && y < b.bottom()) { return static_cast<int>(i); } }
        return -1;
    }
    static size_t offset(int x, int y) { return (static_cast<size_t>(y) * 1920 + static_cast<size_t>(x)) * 4; }
    static bool same_pixel(const Frame& a, const Frame& b, size_t at)
    { return std::equal(a.pixels.begin() + at, a.pixels.begin() + at + 4, b.pixels.begin() + at); }
    static void compare_rois(const Frame& observed, const Frame& reference)
    {
        size_t count = 0;
        for (const auto& box : hud_label_rois()) { for (int y = box.y; y < box.bottom(); ++y) { for (int x = box.x; x < box.right(); ++x)
        { const size_t at = offset(x, y); require(same_pixel(observed, reference, at) && observed.pixels[at + 3] == 255, "HUD label ordered glyph/paper RGBA reference differs"); ++count; } } }
        require(count == 48396, "HUD label independent three-region comparison count differs");
    }
    void rectangle(DrawList& list, const RectangleI& box, const Colour& colour) const
    {
        list.set_filter(TextureFilter::point);
        list.draw_sprite(sheet_->texture(), pixel_.source_rectangle(), RectangleF(static_cast<float>(box.x), static_cast<float>(box.y),
            static_cast<float>(box.width), static_cast<float>(box.height)), colour, 0.0f, Vector2F::ZERO, SpriteFlip::none, 0.0f);
    }
    Frame render(const std::function<void(DrawList&)>& draw)
    {
        unchanged(); const size_t budget = subject_ == HudLabelSubject::foreground ? 19U : subject_ == HudLabelSubject::shadow ? 20U : 60U;
        require(readbacks_ < budget, "HUD label probe exceeded its declared finite readback budget");
        const std::array<Colour, 4> before{ look_.player_identity_colour, look_.player_identity_shadow_colour,
            expected_.player_identity_colour, expected_.player_identity_shadow_colour };
        std::srand(495017); const int next = std::rand(); std::srand(495017);
        Renderer& renderer = *app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list = renderer.view(0); list.set_viewport(app_.viewport_manager()->player_viewport(0));
        list.set_camera(Camera::DEFAULT_CAMERA); list.set_filter(TextureFilter::point);
        canvas_.draw(list, { 0, 0, 1920, 1080 }); draw(list);
        rectangle(list, { 20, 20, 4, 4 }, Colour::white); rectangle(list, { 25, 20, 4, 4 }, Colour::black);
        Frame frame = read(renderer); ++readbacks_;
        require(frame.width == 1920 && frame.height == 1080 && frame.pixels.size() == 1920 * 1080 * 4, "HUD label requires a native 1920 by 1080 RGBA backbuffer");
        for (int y = 20; y < 24; ++y) { for (int x = 20; x < 29; ++x)
        {
            if (x == 24) { continue; } const size_t at = offset(x, y); const unsigned char c = x < 24 ? 255 : 0;
            require(frame.pixels[at] == c && frame.pixels[at + 1] == c && frame.pixels[at + 2] == c && frame.pixels[at + 3] == 255,
                "HUD label WHITE/BLACK RGBA calibration differs");
        } }
        require(std::rand() == next && before == std::array<Colour, 4>{ look_.player_identity_colour, look_.player_identity_shadow_colour,
            expected_.player_identity_colour, expected_.player_identity_shadow_colour }, "HUD label draw consumed RNG or changed selected live/copied colours");
        unchanged(); return frame;
    }
    Application& app_; const Presentation& presentation_; const HudPresentation& look_; const PlayerIdentityPresentation& identity_;
    HudPresentation expected_; PlayerIdentityPresentation expected_identity_; InterfaceGameplay hud_, copied_; Canvas canvas_;
    const SpriteSheet* sheet_; const SpriteFrame& pixel_; HudLabelSubject subject_;
    std::array<FontHandle, 3> fonts_{}; std::array<const Font*, 3> font_pointers_{};
    std::vector<float> fixed_, identity_fixed_; std::array<std::string, 5> names_; TeamPalettePresentation palette_;
    const std::filesystem::path output_; size_t readbacks_ = 0; Frame reference_;
};

void hud_label_colour_consumer_checks(Application& app, const Presentation& presentation, const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);
    constexpr std::array<const char*, 4> states{ "default", "rgb", "partial", "zero" };
    constexpr std::array<const char*, 3> regions{ "identity", "health", "paint" };
    size_t reads = 0, rows = 0, crops = 0, control_frames = 0, discriminations = 0, control_pixels = 0;
    std::ofstream csv(output / "hud-label-colour-main.csv");
    csv << "role,state,raw_rgba_match,adoption_match,repeat_match,locality_match,frozen_style_resources_rng,controls,roi_discriminations,identity_control_pixels,health_control_pixels,paint_control_pixels,scope\n";
    for (const HudLabelSubject subject : { HudLabelSubject::foreground, HudLabelSubject::shadow })
    {
        Presentation local = presentation; auto& look = const_cast<HudPresentation&>(local.hud());
        auto& selected = subject == HudLabelSubject::foreground ? look.player_identity_colour : look.player_identity_shadow_colour;
        HudLabelColourProbe probe(app, local, output, subject); const Colour original = selected;
        const std::array<Colour, 4> colours{ original, Colour(53, 188, 225), Colour(32, 96, 192, 128), Colour(165, 50, 214, 0) };
        Frame baseline;
        for (size_t state = 0; state < states.size(); ++state)
        {
            selected = colours[state]; const Frame frame = probe.observe(); if (state == 0) { baseline = frame; }
            try { probe.locality(frame, baseline, state != 0); }
            catch (...) { probe.failure("selected-locality", frame, baseline); throw; }
            std::array<size_t, 3> changed{}; size_t controls = 0, roi_controls = 0;
            const auto control = [&](HudLabelReference mode)
            {
                const auto result = probe.control(mode); ++controls;
                for (size_t region = 0; region < result.size(); ++region)
                { changed[region] += result[region]; if (result[region] > 0) { ++roi_controls; } }
            };
            if (state == 2) { control(HudLabelReference::straight); control(HudLabelReference::squared);
                if (subject == HudLabelSubject::shadow) { control(HudLabelReference::reversed_order); } }
            if (state == 3) { control(HudLabelReference::straight); }
            csv << hud_label_subject(subject) << ',' << states[state] << ",true,true,true,true,true," << controls << ',' << roi_controls << ','
                << changed[0] << ',' << changed[1] << ',' << changed[2] << ",ordered-hud-label-glyph-paper-rois\n";
            const size_t region_count = subject == HudLabelSubject::foreground ? 3U : 1U;
            for (size_t region = 0; region < region_count; ++region)
            {
                const std::string name = std::string("hud-label-colour-") + hud_label_subject(subject) + '-' + states[state] + '-' + regions[region] + ".ppm";
                save(HudLabelColourProbe::compact(frame, region), output / name); ++crops;
            }
            ++rows; control_frames += controls; discriminations += roi_controls; control_pixels += changed[0] + changed[1] + changed[2];
        }
        require(probe.readbacks() == (subject == HudLabelSubject::foreground ? 19U : 20U), "HUD label shared role budget differs"); reads += probe.readbacks();
    }
    require(csv.good() && reads == 39 && rows == 8 && crops == 16 && control_frames == 7 && discriminations == 13 &&
        control_pixels > 0 && control_pixels <= 313908, "HUD label shared finite counters differ");
    std::ofstream summary(output / "hud-label-colour-consumer-summary.json");
    summary << "{\"scope\":\"three-foreground-labels-and-identity-shadow\",\"context\":\"prepared-normal-123.25\",\"readbacks\":39,\"csv_rows\":8,\"foreground_readbacks\":19,\"shadow_readbacks\":20,"
        "\"crops\":16,\"crop_pixels\":362304,\"ppm_bytes\":1087140,\"roi_pixels\":48396,\"independent_observations\":8,\"roi_pixel_checks\":387168,\"repeat_checks\":8,\"copied_adoption_checks\":8,"
        "\"controls\":7,\"roi_control_discriminations\":13,\"control_discriminating_pixels\":" << control_pixels << ",\"shadow_meter_invariance_checks\":6,\"calibration_pixel_checks\":1248,\"private_game_saves\":0,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot preserve HUD label shared summary");
    std::cout << "PASS HUD label pair: 39 reads, 8 rows, 16 native crops; all three foreground roles and identity-only ordered shadow, "
        "13 separate ROI tint/order discriminations, source paper lower strip and exact badge/clock/fill exterior.\n";
}

#if COLOURWARS_DEV_TOOLS
void hud_label_tune_colour_checks(Application& app, Registry<LevelLoadedInfo>& levels, GameTuning& tuning,
    Presentation& presentation, const std::filesystem::path& content, const std::filesystem::path& output)
{
    const auto root = output / "hud-label-tune-scratch", source = root / "source", staged = root / "staged";
    require(!std::filesystem::exists(root), "HUD label private scratch must be fresh");
    std::filesystem::create_directories(source); std::filesystem::create_directories(staged);
    constexpr std::array<const char*, 2> files{ "tuning.json", "presentation.json" }; std::array<std::string, 2> bytes;
    for (size_t i = 0; i < files.size(); ++i)
    { bytes[i] = tune_colour_bytes(content / files[i]); tune_colour_write(source / files[i], bytes[i]); tune_colour_write(staged / files[i], bytes[i]); }
    const NestedSaveProfile profile(root / "profile"); const TuneColourTemp temporary(root / "temp");
    struct RestoreLabels { GameTuning& t; Presentation& p; GameTuning old_t; Presentation old_p; ~RestoreLabels() { t = old_t; p = old_p; } }
        restore{ tuning, presentation, tuning, presentation };
    Save preferences;
    const auto preference_values = [&]()
    {
        auto values = limb_preference_values(preferences.play_settings());
        values.insert(values.end(), { static_cast<int>(preferences.resolution()), preferences.fullscreen() ? 1 : 0,
            static_cast<int>(preferences.team_palette()), static_cast<int>(preferences.input_bindings().keyboard_jump) }); return values;
    };
    const auto preferences_before = preference_values(); StageList stages;
    for (const char* name : { "movement_course", "close_quarters", "king_of_the_hill", "turbulence", "crossfeed" })
    { const auto* info = levels.get(name); require(info != nullptr, "HUD label requires shipped stage metadata"); stages.push_back({ name, info->display_name(), info->icon_frame(), info->include_in_random() }); }
    GameData data; data.set_application(&app); data.set_save(&preferences); const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "crossfeed", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning); data.set_presentation(&presentation); DebugDirector director(&tuning, &presentation); data.set_debug(&director);
    auto& impl = *director.impl(); impl.set_content_directories(source, staged); impl.set_user_directory(root / "user");
    impl.set_script_directories({ root / "scripts" }); director.attach(&app, &data); impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output / "hud-label-tune-commands.log").string()), "Cannot retain HUD label command transcript");
    HudLabelColourProbe probe(app, presentation, output, HudLabelSubject::pair);
    const std::string foreground_path = "hud.player_identity.colour", shadow_path = "hud.player_identity.shadow_colour";
    JsonSpanDocument initial(bytes[1], "HUD label private source"); const auto* foreground_token = initial.find(foreground_path);
    require(foreground_token != nullptr, "HUD foreground token missing");
    JsonSpanDocument first_saved(initial.replaced(*foreground_token, "\"2060C080\""), "HUD label first private replacement");
    const auto* shadow_token = first_saved.find(shadow_path); require(shadow_token != nullptr, "HUD shadow token missing");
    const std::string saved_bytes = first_saved.replaced(*shadow_token, "\"E17C3580\""); bool persisted = false;
    size_t commands = 0, rows = 0; Frame baseline, saved;
    const auto unchanged = [&]()
    {
        probe.unchanged(); require(preference_values() == preferences_before && !std::filesystem::exists(root / "profile/ColourWars/save_data.json") &&
            !impl.in_match() && !impl.can_apply() && impl.request().empty() && impl.total_updates() == 0 && impl.level_updates() == 0,
            "HUD label changed private defaults, wrote a game Save, queued gameplay or advanced updates");
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto& expected = i == 1 && persisted ? saved_bytes : bytes[i];
            require(tune_colour_bytes(content / files[i]) == bytes[i] && tune_colour_bytes(source / files[i]) == expected &&
                tune_colour_bytes(staged / files[i]) == expected, "HUD label changed private bytes beyond two exact saved tokens");
        }
    };
    const auto command = [&](const std::string& line)
    {
        const size_t before = impl.log().lines().size(); std::srand(495023); const int next = std::rand(); std::srand(495023);
        require(impl.execute(line).ok && std::rand() == next, "HUD label real command acceptance/RNG differs"); ++commands;
        if (line.starts_with("tune "))
        {
            const auto& log = impl.log().lines(); require(log.size() > before && log.back().severity == DebugSeverity::info &&
                log.back().text.starts_with("tune: presentation.hud.player_identity.") && log.back().text.ends_with("(live)"),
                "HUD label command lacks fresh live-adoption reply");
        }
    };
    std::ofstream csv(output / "hud-label-tune.csv");
    csv << "state,foreground_token,shadow_token,raw_rgba_match,adoption_match,repeat_match,sibling_preserved,private_bytes,frozen_style_resources_rng,scope\n";
    const auto check = [&](const char* state, const Colour& foreground, const Colour& shadow, const char* foreground_word, const char* shadow_word)
    {
        unchanged(); require(presentation.hud().player_identity_colour == foreground && presentation.hud().player_identity_shadow_colour == shadow,
            "HUD label selected/sibling exact live channels differ");
        const auto* document = impl.tune_document("presentation"); require(document != nullptr, "HUD label document disappeared");
        for (const auto& leaf : { std::pair<std::string, std::string>(foreground_path, foreground_word), { shadow_path, shadow_word } })
        {
            const auto* token = document->current().find(leaf.first);
            require(token && document->current().token(*token) == '"' + leaf.second + '"', "HUD label exact quoted selected/sibling token differs");
        }
        const Frame frame = probe.observe();
        if (rows == 0) { baseline = frame; }
        if (std::string_view(state) == "reset-original-foreground" || std::string_view(state) == "reset-original-shadow" ||
            std::string_view(state) == "reset-zero-foreground" || std::string_view(state) == "reset-zero-shadow")
        { require(frame.pixels == baseline.pixels, "HUD label original reset lost complete HUD RGBA"); }
        if (std::string_view(state) == "joint-pending") { saved = frame; }
        if (std::string_view(state) == "reset-saved-foreground" || std::string_view(state) == "reset-saved-shadow")
        { require(frame.pixels == saved.pixels, "HUD label saved field reset lost exact sibling/full HUD RGBA"); }
        if (std::string_view(state) == "joint-pending")
        {
            require(document->edits().size() == 2 && document->edits().at(foreground_path).current_token == "\"2060C080\"" &&
                document->edits().at(shadow_path).current_token == "\"E17C3580\"", "HUD pending sibling ledger was lost");
        }
        csv << state << ',' << foreground_word << ',' << shadow_word << ",true,true,true,true,true,true,debug-paired-hud-label-live\n";
        ++rows; unchanged();
    };
    const Colour original(20, 20, 20), original_shadow(0, 0, 0, 34), red(255, 0, 0), zero(165, 50, 214, 0), rgb(53, 188, 225);
    const Colour partial(32, 96, 192, 128), shadow_partial(225, 124, 53, 128);
    check("baseline", original, original_shadow, "141414", "00000022");
    command("tune presentation.hud.player_identity.colour RED"); check("named-foreground", red, original_shadow, "RED", "00000022");
    command("tune.reset presentation.hud.player_identity.colour"); check("reset-original-foreground", original, original_shadow, "141414", "00000022");
    command("tune presentation.hud.player_identity.shadow_colour RED"); check("named-shadow", original, red, "141414", "RED");
    command("tune.reset presentation.hud.player_identity.shadow_colour"); check("reset-original-shadow", original, original_shadow, "141414", "00000022");
    command("tune presentation.hud.player_identity.colour A532D600"); check("zero-foreground", zero, original_shadow, "A532D600", "00000022");
    command("tune.reset presentation.hud.player_identity.colour"); check("reset-zero-foreground", original, original_shadow, "141414", "00000022");
    command("tune presentation.hud.player_identity.shadow_colour A532D600"); check("zero-shadow", original, zero, "141414", "A532D600");
    command("tune.reset presentation.hud.player_identity.shadow_colour"); check("reset-zero-shadow", original, original_shadow, "141414", "00000022");
    command("tune presentation.hud.player_identity.colour 2060C080"); check("foreground-pending", partial, original_shadow, "2060C080", "00000022");
    command("tune presentation.hud.player_identity.shadow_colour E17C3580"); check("joint-pending", partial, shadow_partial, "2060C080", "E17C3580");
    command("tune.save presentation"); persisted = true; unchanged();
    require(impl.tune_document("presentation")->edits().empty(), "HUD paired two-token Save did not rebase");
    command("tune presentation.hud.player_identity.colour 35BCE1"); check("after-save-foreground", rgb, shadow_partial, "35BCE1", "E17C3580");
    command("tune.reset presentation.hud.player_identity.colour"); check("reset-saved-foreground", partial, shadow_partial, "2060C080", "E17C3580");
    command("tune presentation.hud.player_identity.shadow_colour 35BCE1"); check("after-save-shadow", partial, rgb, "2060C080", "35BCE1");
    command("tune.reset presentation.hud.player_identity.shadow_colour"); check("reset-saved-shadow", partial, shadow_partial, "2060C080", "E17C3580");
    require(csv.good() && commands == 15 && rows == 15 && probe.readbacks() == 60 && impl.tune_document("presentation")->edits().empty() &&
        impl.tune_document("tuning")->edits().empty(), "HUD label paired Debug finite counters/final ledger differ");
    std::ofstream summary(output / "hud-label-tune-summary.json");
    summary << "{\"scope\":\"debug-paired-hud-label-live\",\"commands\":15,\"states\":15,\"readbacks\":60,\"csv_rows\":15,\"crops\":0,\"saved_tokens\":2,\"presentation_save_commands\":1,"
        "\"roi_pixel_checks\":725940,\"repeat_checks\":15,\"copied_adoption_checks\":15,\"sibling_checks\":15,\"calibration_pixel_checks\":1920,\"private_game_saves\":0,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(), "Cannot preserve HUD label paired Debug summary");
    std::cout << "PASS HUD label real paired commands: 15 statements and rendered states, 60 additional reads, 15 rows, "
        "two exact private tokens saved/rebased, original and saved resets with sibling preservation. No game Saves or updates.\n";
}
#endif
