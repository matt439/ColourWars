// Glass37: all four selected writes, three retained public Monitor methods.
// Raw main proof covers ONLY five declared ROIs. Whole-frame copied Monitor
// equality proves adoption; it is not a full independent Monitor oracle.
enum class GlassMethod { main, picker, close };
enum class GlassWrite { none, main, palette, picker, close };
enum class GlassReference { correct, straight, squared_base };

std::vector<float> glass_fixed_style(const OpeningPresentation& p)
{
    OpeningPresentation unchanged = p; unchanged.glass_colour = Colour(22, 22, 22);
    auto values = program_fixed_style(unchanged); // Includes every other opening field except program.
    values.insert(values.end(), { p.program_colour.r, p.program_colour.g, p.program_colour.b, p.program_colour.a });
    return values;
}


std::vector<float> glass_monitor_geometry(const Monitor& monitor)
{
    const RectangleF screen(0, 0, 1280, 720); std::vector<float> values;
    for (const auto& r : { monitor.glass(screen), monitor.picture(screen), monitor.swatch(screen, 0),
        monitor.swatch(screen, 1), monitor.empty_slot(screen), monitor.close_button(screen) })
    { values.insert(values.end(), { r.x, r.y, r.width, r.height }); }
    return values;
}

class GlassColourProbe
{
public:
    GlassColourProbe(Application& app, const Presentation& presentation) :
        app_(app), presentation_(presentation), look_(presentation.opening()), baseline_(look_), expected_(baseline_),
        monitor_(&look_, app.render_resources(), {}), reference_(&expected_, app.render_resources(), {}),
        sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),
        palette_sheet_(app.render_resources()->sprite_sheet(look_.palette_sheet)),
        frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))), fixed_(glass_fixed_style(look_)),
        persona_(monitor_persona_observation(monitor_.persona())), geometry_(glass_monitor_geometry(monitor_))
    {
        require(look_.glass == RectangleF(.1f, .07f, .8f, .76f) && look_.palette_height == .09f &&
            look_.monitor_art.shadow_width == .024f && !look_.bezel_shades.empty() && look_.bezel_shades.front() == Colour(217, 208, 182) &&
            look_.bezel_shadow == Colour(0, 0, 0, 89) && look_.program_colour == Colour(44, 44, 44) &&
            look_.slot_outline == Colour(255, 255, 255) && look_.palette_frames.size() == 26 &&
            look_.sprite_sheet == "sprite_sheet_1" && look_.fill_frame == "pixel" &&
            frame_.source_rectangle() == RectangleI(32, 0, 4, 4) && frame_.origin() == Vector2F::ZERO,
            "Glass fixture lost shipped geometry, ordered underlayers, atlas or white-frame assumptions");
        for (const auto& name : look_.palette_frames)
        { palette_frames_.push_back(&palette_sheet_->sprite_frame(palette_sheet_->resolve_sprite_frame(name))); }
    }
    void unchanged() const
    {
        require(&look_ == &presentation_.opening() && &look_.monitor_art == &presentation_.opening().monitor_art &&
            glass_fixed_style(look_) == fixed_ && glass_fixed_style(expected_) == fixed_ &&
            look_.sprite_sheet == baseline_.sprite_sheet && look_.fill_frame == baseline_.fill_frame &&
            look_.palette_sheet == baseline_.palette_sheet && look_.palette_frames == baseline_.palette_frames &&
            look_.percent_font == baseline_.percent_font && sheet_ == app_.render_resources()->sprite_sheet(look_.sprite_sheet) &&
            palette_sheet_ == app_.render_resources()->sprite_sheet(look_.palette_sheet) &&
            &frame_ == &sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)) &&
            persona_ == monitor_persona_observation(monitor_.persona()) && persona_ == monitor_persona_observation(reference_.persona()) &&
            geometry_ == glass_monitor_geometry(monitor_) && geometry_ == glass_monitor_geometry(reference_) &&
            first_ == Colour(228, 83, 109) && second_ == Colour(74, 195, 127),
            "Glass edit/draw changed borrowed records, unrelated style/resources, captured persona/geometry or caller swatches");
        for (size_t index=0; index<look_.palette_frames.size(); ++index)
        { require(palette_frames_[index] == &palette_sheet_->sprite_frame(palette_sheet_->resolve_sprite_frame(look_.palette_frames[index])),
            "Glass drawing changed captured palette frame resources"); }
    }
    Frame actual(GlassMethod method, const Colour& background)
    {
        unchanged(); const auto draw = [&](DrawList& list) { draw_monitor(monitor_, list, method); };
        Frame result = render(background, draw);
        const Frame repeat = render(background, draw);
        require(result.pixels == repeat.pixels, "Frozen glass method changed pixels"); unchanged(); return result;
    }
    Frame adoption(GlassMethod method, const Colour& background, const Colour& colour)
    {
        expected_.glass_colour = colour; unchanged();
        return render(background, [&](DrawList& list) { draw_monitor(reference_, list, method); });
    }
    Frame blank(const Colour& background) { return render(background, [](DrawList&) {}); }
    Frame raw(GlassMethod method, const Colour& background, const Colour& colour,
        GlassWrite wrong_write = GlassWrite::none, GlassReference mode = GlassReference::correct)
    {
        return render(background, [&](DrawList& list)
        {
            const auto tint = [&](GlassWrite write)
            {
                const float factor = write != wrong_write || mode == GlassReference::correct ? colour.a :
                    mode == GlassReference::straight ? 1.0f : colour.a * colour.a;
                return Colour(colour.r * factor, colour.g * factor, colour.b * factor, colour.a);
            };
            const Colour white(255, 255, 255);
            if (method == GlassMethod::main)
            {
                write_raw(list, { 0, 0, 1280, 720 }, Colour(217, 208, 182));
                // Four literal, ordered central passes. Separate right/bottom
                // screen bands miss all five declared ROIs and are not claimed.
                const Colour quarter(0.0f, 0.0f, 0.0f, (89.0f / 255.0f) / 4.0f);
                for (const RectangleI r : { RectangleI(124, 46, 1045, 568), RectangleI(125, 47, 1039, 563),
                    RectangleI(126, 48, 1034, 558), RectangleI(127, 49, 1029, 552) }) { write_raw(list, r, quarter); }
                write_raw(list, { 119, 41, 1041, 565 }, Colour(0, 0, 0, 89));
                write_raw(list, { 128, 50, 1024, 547 }, tint(GlassWrite::main));
                write_raw(list, { 136, 59, 1007, 529 }, Colour(44, 44, 44));
                write_raw(list, { 136, 539, 1007, 49 }, tint(GlassWrite::palette));
            }
            else if (method == GlassMethod::picker)
            {
                write_raw(list, { 1046, 479, 97, 51 }, tint(GlassWrite::picker));
                for (const RectangleI r : { RectangleI(1046, 479, 97, 2), RectangleI(1046, 529, 97, 1),
                    RectangleI(1046, 481, 2, 48), RectangleI(1141, 481, 2, 48) }) { write_raw(list, r, white); }
                write_raw(list, { 1053, 486, 37, 37 }, first_); write_raw(list, { 1099, 486, 37, 37 }, second_);
            }
            else
            {
                write_raw(list, { 1112, 65, 25, 24 }, tint(GlassWrite::close));
                for (const RectangleI r : { RectangleI(1112, 65, 25, 2), RectangleI(1112, 87, 25, 2),
                    RectangleI(1112, 67, 3, 20), RectangleI(1134, 67, 3, 20) }) { write_raw(list, r, white); }
                // Literal 24 edge-truncated X dabs, frozen before native drawing.
                for (const RectangleI r : { RectangleI(1118,70,2,3), RectangleI(1119,71,2,3), RectangleI(1120,72,2,3),
                    RectangleI(1121,73,2,3), RectangleI(1122,74,2,3), RectangleI(1123,75,2,3), RectangleI(1124,76,2,3),
                    RectangleI(1125,77,2,3), RectangleI(1126,78,2,3), RectangleI(1127,79,2,3), RectangleI(1128,80,2,3), RectangleI(1129,81,2,3),
                    RectangleI(1129,70,2,3), RectangleI(1128,71,2,3), RectangleI(1127,72,2,3), RectangleI(1126,73,2,3),
                    RectangleI(1125,74,2,3), RectangleI(1124,75,2,3), RectangleI(1123,76,2,3), RectangleI(1122,77,2,3),
                    RectangleI(1121,78,2,3), RectangleI(1120,79,2,3), RectangleI(1119,80,2,3), RectangleI(1118,81,2,3) }) { write_raw(list, r, white); }
            }
        });
    }
    static std::array<RectangleI, 5> main_rois()
    { return { RectangleI(127,300,11,10), RectangleI(638,49,4,12), RectangleI(1142,300,11,10),
        RectangleI(638,580,4,18), RectangleI(638,538,4,4) }; }
    static RectangleI roi(GlassMethod method)
    { return method == GlassMethod::picker ? RectangleI(1045,478,99,53) : RectangleI(1111,64,27,26); }
    static bool roi_equal(GlassMethod method, const Frame& a, const Frame& b)
    {
        if (method == GlassMethod::main)
        { for (const auto& r : main_rois()) { if (foreground_crop(a,r,0).pixels != foreground_crop(b,r,0).pixels) { return false; } } return true; }
        return foreground_crop(a,roi(method),0).pixels == foreground_crop(b,roi(method),0).pixels;
    }
    static Vector2I selected_sample(GlassWrite write)
    { return write == GlassWrite::main ? Vector2I(132,304) : write == GlassWrite::palette ? Vector2I(640,541) :
        write == GlassWrite::picker ? Vector2I(1095,500) : Vector2I(1116,69); }
    static void wrong_rejected(GlassMethod method, GlassWrite write, const Frame& actual, const Frame& wrong)
    {
        const Vector2I p = selected_sample(write);
        require(!roi_equal(method, actual, wrong) && monitor_rgb(actual,p.x,p.y) != monitor_rgb(wrong,p.x,p.y),
            "Glass per-write wrong-alpha control was not independently discriminated");
    }
    static void guards(GlassMethod method, const Frame& actual, const Frame& raw, const Frame& blank, const Colour& colour)
    {
        require(roi_equal(method,actual,raw), "Glass independent ordered ROI differs from actual consumer");
        if (method == GlassMethod::main)
        {
            require(monitor_rgb(actual,640,320) == 0x2C2C2Cu, "Glass edit changed opaque unselected program");
            if (colour.a == 0.0f) { require(monitor_rgb(actual,640,541) == 0x2C2C2Cu &&
                monitor_rgb(actual,132,304) != monitor_rgb(blank,132,304), "Glass alpha0 lost lip/program underlayers"); }
        }
        else if (method == GlassMethod::picker)
        {
            require(monitor_rgb(actual,1046,480) == 0xFFFFFFu && monitor_rgb(actual,1060,500) == 0xE4536Du &&
                monitor_rgb(actual,1105,500) == 0x4AC37Fu && monitor_rgb(actual,1045,478) == monitor_rgb(blank,1045,478),
                "Glass picker changed caller swatches, white frame or exterior Canvas");
        }
        else
        {
            require(monitor_rgb(actual,1112,65) == 0xFFFFFFu && monitor_rgb(actual,1124,77) == 0xFFFFFFu &&
                monitor_rgb(actual,1111,64) == monitor_rgb(blank,1111,64), "Glass close lost white frame/X or exterior Canvas");
        }
        if (method != GlassMethod::main && colour.a == 0.0f)
        { const auto p=selected_sample(method == GlassMethod::picker ? GlassWrite::picker : GlassWrite::close);
          require(monitor_rgb(actual,p.x,p.y) == monitor_rgb(blank,p.x,p.y), "Glass alpha0 isolated face did not retain Canvas"); }
        if (colour.a == 1.0f)
        {
            const auto rgb = (static_cast<std::uint32_t>(std::round(colour.r*255))<<16) |
                (static_cast<std::uint32_t>(std::round(colour.g*255))<<8) | static_cast<std::uint32_t>(std::round(colour.b*255));
            for (const GlassWrite write : method == GlassMethod::main ? std::vector<GlassWrite>{GlassWrite::main,GlassWrite::palette} :
                std::vector<GlassWrite>{method == GlassMethod::picker ? GlassWrite::picker : GlassWrite::close})
            { const auto p=selected_sample(write); require(monitor_rgb(actual,p.x,p.y) == rgb, "Glass opaque calibration differs"); }
        }
    }
    static void delta_only_selected(GlassMethod method, const Frame& actual, const Frame& baseline)
    {
        for (int y=0;y<actual.height;++y) { for (int x=0;x<actual.width;++x)
        {
            const bool glass=x>=128&&x<1152&&y>=50&&y<597, program=x>=136&&x<1143&&y>=59&&y<588;
            const bool allowed=method==GlassMethod::main ? glass&&(!program||y>=539) : method==GlassMethod::picker ?
                x>=1046&&x<1143&&y>=479&&y<530 : x>=1112&&x<1137&&y>=65&&y<89;
            if (!allowed) { const size_t at=(static_cast<size_t>(y)*static_cast<size_t>(actual.width)+static_cast<size_t>(x))*4;
                require(same_pixel(actual,baseline,at), "Glass delta escaped selected consumer masks"); }
        } }
    }
    static Frame compact(GlassMethod method, const Frame& source)
    {
        if (method != GlassMethod::main) { return foreground_crop(source,roi(method),0); }
        Frame result{80,24,{}}; result.pixels.reserve(80*24*4);
        for (int y=0;y<24;++y) { for (const Vector2I origin : {Vector2I(126,296),Vector2I(632,44),Vector2I(1140,296),Vector2I(632,576),Vector2I(632,532)})
        { const size_t at=(static_cast<size_t>(origin.y+y)*static_cast<size_t>(source.width)+static_cast<size_t>(origin.x))*4;
          result.pixels.insert(result.pixels.end(),source.pixels.begin()+at,source.pixels.begin()+at+16*4); } }
        return result;
    }
    size_t readbacks() const { return readbacks_; }
private:
    void write_raw(DrawList& list, const RectangleI& r, const Colour& tint)
    { list.draw_sprite(sheet_->texture(),frame_.source_rectangle(),RectangleF(static_cast<float>(r.x),static_cast<float>(r.y),
        static_cast<float>(r.width),static_cast<float>(r.height)),tint,0.0f,frame_.origin(),SpriteFlip::none,0.0f); }
    void draw_monitor(const Monitor& monitor, DrawList& list, GlassMethod method)
    { const RectangleF screen(0,0,1280,720); if(method==GlassMethod::main){monitor.draw(list,screen);}
      else if(method==GlassMethod::picker){monitor.draw_picker(list,screen,first_,second_);}else{monitor.draw_close_button(list,screen);} }
    Frame render(const Colour& background, const std::function<void(DrawList&)>& draw)
    {
        std::srand(31737); const int next=std::rand(); std::srand(31737);
        const Canvas canvas(app_.render_resources(),background); Renderer& renderer=*app_.renderer(); renderer.begin_frame(); renderer.set_view_count(1);
        DrawList list=renderer.view(0); list.set_viewport(app_.viewport_manager()->fullscreen_viewport()); list.set_camera(Camera::DEFAULT_CAMERA);
        list.set_filter(TextureFilter::point); canvas.draw(list,{0,0,1280,720}); draw(list); ++readbacks_;
        Frame result=read(renderer); require(std::rand()==next,"Glass actual/reference drawing consumed RNG"); return result;
    }
    Application& app_; const Presentation& presentation_; const OpeningPresentation& look_; OpeningPresentation baseline_,expected_;
    Monitor monitor_,reference_; const SpriteSheet* sheet_; const SpriteSheet* palette_sheet_; const SpriteFrame& frame_;
    std::vector<float> fixed_,persona_,geometry_; std::vector<const SpriteFrame*> palette_frames_;
    const Colour first_{228,83,109},second_{74,195,127}; size_t readbacks_=0;
};

void glass_colour_consumer_checks(Application& app,const Presentation& presentation,const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player); app.resolution_manager()->set_resolution_exactly({1280,720});
    std::ignore=app.renderer()->window_size_changed(1280,720); Presentation local=presentation;
    auto& look=const_cast<OpeningPresentation&>(local.opening()); require(look.glass_colour==Colour(22,22,22),"Glass opaque default changed");
    GlassColourProbe probe(app,local); const std::array<Colour,2> backgrounds{Colour(0,0,0),Colour(32,64,96)};
    const std::array<Frame,2> blanks{probe.blank(backgrounds[0]),probe.blank(backgrounds[1])};
    const std::array<Colour,4> colours{Colour(22,22,22),Colour(53,188,225),Colour(32,96,192,128),Colour(165,50,214,0)};
    constexpr std::array<const char*,4> states{"default","rgb","rgba128","alpha0"}; constexpr std::array<const char*,3> methods{"main","picker","close"};
    std::array<std::array<Frame,2>,3> baseline{}; size_t rows=0,crops=0,partial_controls=0,zero_controls=0;
    std::ofstream records(output/"glass-colour-consumers.csv"); records<<"state,method,background,roi_raw_match,adoption_match,guards,frozen_style_persona_rng,controls,scope\n";
    for(size_t m=0;m<methods.size();++m) { const auto method=static_cast<GlassMethod>(m);
        for(size_t p=0;p<colours.size();++p) { look.glass_colour=colours[p];
            for(size_t b=0;b<(m==0?1u:2u);++b)
            {
                const Frame actual=probe.actual(method,backgrounds[b]),adoption=probe.adoption(method,backgrounds[b],colours[p]),raw=probe.raw(method,backgrounds[b],colours[p]);
                try
                {
                if(!GlassColourProbe::roi_equal(method,actual,raw)) { save(GlassColourProbe::compact(method,actual),output/"glass-failure-actual.ppm"); save(GlassColourProbe::compact(method,raw),output/"glass-failure-raw.ppm"); }
                require(actual.pixels==adoption.pixels,"Glass copied-dependency adoption differs"); GlassColourProbe::guards(method,actual,raw,blanks[b],colours[p]);
                if(p==0){baseline[m][b]=actual;}else{GlassColourProbe::delta_only_selected(method,actual,baseline[m][b]);require(actual.pixels!=baseline[m][b].pixels,"Glass selected palette produced no delta");}
                size_t controls=0;
                if(p>=2) { const auto writes=m==0?std::vector<GlassWrite>{GlassWrite::main,GlassWrite::palette}:std::vector<GlassWrite>{m==1?GlassWrite::picker:GlassWrite::close};
                    for(const GlassWrite write:writes) { GlassColourProbe::wrong_rejected(method,write,actual,probe.raw(method,backgrounds[b],colours[p],write,GlassReference::straight)); ++controls;
                        if(p==2){++partial_controls;GlassColourProbe::wrong_rejected(method,write,actual,probe.raw(method,backgrounds[b],colours[p],write,GlassReference::squared_base));++controls;++partial_controls;}else{++zero_controls;} }
                }
                const char* background=m==0?"opaque-beige":b==0?"black":"nonblack";
                records<<states[p]<<','<<methods[m]<<','<<background<<",true,true,true,true,"<<controls<<",consumer-only\n"; ++rows;
                save(GlassColourProbe::compact(method,actual),output/(std::string("glass-colour-")+methods[m]+"-"+states[p]+"-"+background+".ppm")); ++crops;
                }
                catch(...)
                {
                    save(GlassColourProbe::compact(method,actual),output/"glass-failure-actual.ppm");
                    save(GlassColourProbe::compact(method,raw),output/"glass-failure-raw.ppm");
                    std::ofstream failure(output/"glass-failure.json");failure<<"{\"method_index\":"<<m<<",\"palette_index\":"<<p
                        <<",\"background_index\":"<<b<<",\"diagnostic\":\"see-native-stderr-and-retained-native-context\"}\n";
                    throw;
                }
            }
        }
    }
    require(records.good()&&probe.readbacks()==100&&rows==20&&crops==20&&partial_controls==12&&zero_controls==6,"Glass shared budget differs");
    std::ofstream summary(output/"glass-colour-consumer-summary.json"); summary<<"{\"scope\":\"consumer-only\",\"readbacks\":100,\"csv_rows\":20,\"crops\":20,\"crop_pixels\":55272,\"blank_reads\":2,\"partial_controls\":12,\"alpha0_controls\":6,"
        "\"main_cases\":4,\"picker_cases\":8,\"close_cases\":8,\"main_effective_backdrops\":1,\"standalone_backdrops\":2,\"selected_writes\":4,\"main_raw_rois\":5,\"close_x_dabs\":24,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"viewport\":\"1280x720\",\"event_fade\":false,"
        "\"adoption_reference\":\"production-consumer-only\",\"alpha_reference\":\"independent-ordered-main-five-rois-and-standalone-rois\",\"main_crop\":[80,24],\"picker_crop\":[99,53],\"close_crop\":[27,26]}\n";
    require(summary.good(),"Cannot write Glass shared summary");
    std::cout<<"PASS glass:100readbacks/20rows/20crops55272pixels; all4writes, four separate quarter-shadow passes plus lip, five main ROIs and guarded picker/close ROIs. "
        "Main one effective beige backdrop; standalone two Canvas backdrops;12partial and6alpha0 per-write controls. Copied Monitor adoption only; zero all updates/RNG-neutral draws.\n";
}
#if COLOURWARS_DEV_TOOLS
void glass_tune_colour_checks(Application& app,Registry<LevelLoadedInfo>& levels,GameTuning& tuning,Presentation& presentation,
    const std::filesystem::path& content,const std::filesystem::path& output)
{
    const auto root=output/"glass-colour-tune-scratch",source=root/"source",staged=root/"staged";
    std::filesystem::create_directories(source);std::filesystem::create_directories(staged);
    constexpr std::array<const char*,2> files{"tuning.json","presentation.json"};std::array<std::string,2> real_bytes;
    for(size_t i=0;i<files.size();++i){real_bytes[i]=tune_colour_bytes(content/files[i]);
        tune_colour_write(source/files[i],real_bytes[i]);tune_colour_write(staged/files[i],real_bytes[i]);}
    const NestedSaveProfile profile(root/"profile");const TuneColourTemp temporary(root/"temp");
    struct RestoreGlass {GameTuning& t;Presentation& p;GameTuning old_t;Presentation old_p;~RestoreGlass(){t=old_t;p=old_p;}}
        restore{tuning,presentation,tuning,presentation};
    Save preferences;PlaySettings play;play.music_volume=17;play.sfx_volume=83;play.aim_sensitivity=4;
    play.view_shake=false;play.aim_marker=true;play.reduce_effects=true;play.sticks[2].movement_deadzone=25;play.sticks[2].aim_deadzone=70;
    InputBindings bindings;bindings.keyboard_jump=Key::j;
    require(preferences.set_play_settings_and_save(play)&&preferences.set_input_bindings_and_save(bindings),"Cannot seed glass private preferences");
    const auto save_path=root/"profile/ColourWars/save_data.json";const std::string saved_preferences=tune_colour_bytes(save_path);
    const auto pref_values=[&]
    {const auto& p=preferences.play_settings();std::vector<int> v{static_cast<int>(preferences.resolution()),preferences.fullscreen()?1:0,
        static_cast<int>(preferences.team_palette()),p.aim_sensitivity,p.music_volume,p.sfx_volume,p.view_shake?1:0,p.aim_marker?1:0,
        p.reduce_effects?1:0,static_cast<int>(preferences.input_bindings().keyboard_jump)};
        for(const auto& stick:p.sticks){v.push_back(stick.movement_deadzone);v.push_back(stick.aim_deadzone);}return v;};
    const auto saved_values=pref_values();StageList stages;
    for(const char* name:{"movement_course","close_quarters","king_of_the_hill","turbulence"})
    {const auto* info=levels.get(name);stages.push_back({name,info->display_name(),info->icon_frame(),info->include_in_random()});}
    GameData data;data.set_application(&app);data.set_save(&preferences);const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning);data.set_presentation(&presentation);DebugDirector director(&tuning,&presentation);data.set_debug(&director);
    auto& impl=*director.impl();impl.set_content_directories(source,staged);impl.set_user_directory(root/"user");
    impl.set_script_directories({root/"scripts"});director.attach(&app,&data);impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output/"glass-colour-tune-commands.log").string()),"Cannot retain glass transcript");
    GlassColourProbe probe(app,presentation);JsonSpanDocument initial(real_bytes[1],"glass private source");
    const JsonLeaf* leaf=initial.find("opening.glass_colour");require(leaf!=nullptr,"Glass source lacks exact existing token");
    const std::string expected_saved=initial.replaced(*leaf,"\"2060C080\"");bool persisted=false;size_t commands=0,rows=0;
    const auto unchanged=[&]
    {probe.unchanged();require(!impl.can_apply()&&impl.request().empty()&&!impl.in_match()&&tune_colour_bytes(save_path)==saved_preferences&&pref_values()==saved_values,
        "Glass commands changed private prefs/default display/JumpJ or queued work");
        for(size_t i=0;i<files.size();++i){const auto& expected=i==1&&persisted?expected_saved:real_bytes[i];
            require(tune_colour_bytes(content/files[i])==real_bytes[i]&&tune_colour_bytes(source/files[i])==expected&&tune_colour_bytes(staged/files[i])==expected,
                "Glass command changed actual content/private bytes beyond exact one token");}};
    const auto command=[&](const std::string& line)
    {const size_t before=impl.log().lines().size();std::srand(31737);const int next=std::rand();std::srand(31737);
        const auto status=impl.execute(line);++commands;require(status.ok&&std::rand()==next,"Actual glass command failed or consumed RNG");
        if(line.starts_with("tune ")){const auto& log=impl.log().lines();require(log.size()>before&&log.back().severity==DebugSeverity::info&&
            log.back().text.starts_with("tune: presentation.opening.glass_colour")&&log.back().text.ends_with("(live)"),"Glass live reply differs");}};
    const Colour background(0,0,0);std::array<Frame,3> initial_raster{},saved_raster{};
    constexpr std::array<const char*,3> methods{"main","picker","close"};
    std::ofstream records(output/"glass-colour-tune.csv");records<<"state,method,background,roi_raw_match,adoption_match,frozen_style_persona_rng,private_bytes,adoption\n";
    const auto check=[&](const char* state,const Colour& expected)
    {
        unchanged();require(presentation.opening().glass_colour==expected,"Glass command parsed wrong literal channels");
        for(size_t m=0;m<methods.size();++m)
        {
            const auto method=static_cast<GlassMethod>(m);const Frame actual=probe.actual(method,background),adoption=probe.adoption(method,background,expected),raw=probe.raw(method,background,expected);
            try
            {
            if(!GlassColourProbe::roi_equal(method,actual,raw)){save(GlassColourProbe::compact(method,actual),output/"glass-failure-debug-actual.ppm");save(GlassColourProbe::compact(method,raw),output/"glass-failure-debug-raw.ppm");}
            require(actual.pixels==adoption.pixels&&GlassColourProbe::roi_equal(method,actual,raw),"Retained glass failed per-method independent ROI or whole-consumer adoption after actual command");
            if(std::string_view(state)=="baseline"){initial_raster[m]=actual;}else{GlassColourProbe::delta_only_selected(method,actual,initial_raster[m]);}
            if(std::string_view(state)=="reset-original"){require(actual.pixels==initial_raster[m].pixels,"Glass original reset failed whole-frame restoration");}
            if(std::string_view(state)=="rgba128"){saved_raster[m]=actual;}
            if(std::string_view(state)=="reset-saved"||std::string_view(state)=="reset-all-saved")
            {require(actual.pixels==saved_raster[m].pixels,"Glass saved reset failed exact whole-consumer raster restoration");}
            records<<state<<','<<methods[m]<<','<<(m==0?"opaque-beige":"black")<<",true,true,true,true,live\n";++rows;
            }
            catch(...)
            {
                save(GlassColourProbe::compact(method,actual),output/"glass-failure-debug-actual.ppm");
                save(GlassColourProbe::compact(method,raw),output/"glass-failure-debug-raw.ppm");
                std::ofstream failure(output/"glass-failure-debug.json");failure<<"{\"method_index\":"<<m
                    <<",\"diagnostic\":\"see-native-stderr-and-retained-native-context\"}\n";throw;
            }
        }
        unchanged();
    };
    check("baseline",Colour(22,22,22));command("tune presentation.opening.glass_colour RED");check("named",Colour(255,0,0));
    command("tune.reset presentation.opening.glass_colour");check("reset-original",Colour(22,22,22));
    command("tune presentation.opening.glass_colour 35BCE1");check("rgb",Colour(53,188,225));
    command("tune presentation.opening.glass_colour 2060C080");check("rgba128",Colour(32,96,192,128));
    const auto* document=impl.tune_document("presentation");require(document!=nullptr&&document->edits().size()==1&&document->edits().contains("opening.glass_colour"),"Glass save ledger differs from one exact token");
    command("tune.save presentation");persisted=true;unchanged();require(impl.tune_document("presentation")->edits().empty(),"Glass Save failed to rebase");
    command("tune presentation.opening.glass_colour A532D600");check("alpha0",Colour(165,50,214,0));
    command("tune.reset presentation.opening.glass_colour");check("reset-saved",Colour(32,96,192,128));
    command("tune presentation.opening.glass_colour 35BCE1");check("rgb-after-save",Colour(53,188,225));
    command("tune.reset presentation");check("reset-all-saved",Colour(32,96,192,128));
    require(impl.tune_document("presentation")->edits().empty()&&impl.tune_document("tuning")->edits().empty(),"Glass final reset left edits");
    require(records.good()&&commands==9&&rows==27&&probe.readbacks()==108,"Glass Debug budget differs");
    std::ofstream summary(output/"glass-colour-tune-summary.json");summary<<"{\"scope\":\"debug-validated-commands\",\"commands\":9,\"states\":9,\"methods\":3,\"readbacks\":108,\"csv_rows\":27,\"crops\":0,\"saved_tokens\":1,"
        "\"standalone_backdrops\":1,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0}\n";
    require(summary.good(),"Cannot write Glass Debug summary");
    std::cout<<"PASS glass tune:9real commands/9states/3methods/108readbacks/27rows; one exact glass token Save/rebase, original+saved whole-raster resets, "
        "private PlaySettings+JumpJ/default display and actual content retained; one standalone black backdrop (shared owns two), zero all updates.\n";
}
#endif
