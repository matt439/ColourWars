// Body38: nine local ordered-overlap patches, not a full raw silhouette mask.
// The copied-record whole-frame reference proves retained adoption only.
enum class SilhouetteBodyReference { correct, straight, squared_base, collapsed_one, collapsed_two };
struct SilhouetteBodyPatch { int x, y, writes; };

std::vector<SilhouetteBodyPatch> silhouette_body_patches(size_t context)
{
    if (context == 0) { return { {640,400,1},{640,478,2},{640,520,2},{932,640,1},{945,659,2},{945,652,3} }; }
    if (context == 1) { return { {640,588,2} }; }
    return { {1120,285,1},{1093,280,2} };
}
const char* silhouette_body_context_name(size_t context)
{ constexpr std::array<const char*,3> names{ "adult-rest-mug","adult-half-stand","child-rest" }; return names.at(context); }
const char* silhouette_body_write_counts(size_t context)
{ constexpr std::array<const char*,3> counts{ "1;2;2;1;2;3","2","1;2" }; return counts.at(context); }
std::vector<float> silhouette_body_fixed_style(const OpeningPresentation& p)
{
    OpeningPresentation stable=p; stable.silhouette=Colour(14,14,14);
    auto values=program_fixed_style(stable);
    values.insert(values.end(),{p.program_colour.r,p.program_colour.g,p.program_colour.b,p.program_colour.a}); return values;
}

class SilhouetteBodyColourProbe
{
public:
    SilhouetteBodyColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output);
    void unchanged() const;
    Frame actual(size_t context);
    Frame adoption(size_t context,const Colour& body);
    Frame raw(size_t context,const Colour& body,SilhouetteBodyReference mode=SilhouetteBodyReference::correct);
    static void check_patches(size_t context,const Frame& actual,const Frame& raw,const Colour& body);
    static void check_control(size_t context,const Frame& actual,const Frame& wrong,SilhouetteBodyReference mode,
        size_t& rejected,size_t& unchanged_count);
    static void exterior(size_t context,const Frame& actual);
    static void delta(size_t context,const Frame& actual,const Frame& baseline);
    static Frame compact(size_t context,const Frame& source);
    void failure(size_t context,const char* stage,const Frame& first,const Frame* second=nullptr) const;
    size_t readbacks() const { return readbacks_; }
private:
    static RectangleI footprint(size_t context)
    { return context==0 ? RectangleI(300,290,700,430) : context==1 ? RectangleI(300,110,700,610) : RectangleI(1000,220,240,240); }
    static size_t offset(const Frame& frame,int x,int y)
    { return (static_cast<size_t>(y)*static_cast<size_t>(frame.width)+static_cast<size_t>(x))*4; }
    static void uniform_patch(const Frame& frame,const SilhouetteBodyPatch& patch);
    void raw_write(DrawList& list,const RectangleI& r,const Colour& tint);
    void draw_person(const Silhouette& person,DrawList& list,size_t context) const;
    Frame render(const std::function<void(DrawList&)>& draw);
    Application& app_; const Presentation& presentation_; const OpeningPresentation& look_; OpeningPresentation baseline_,expected_;
    Silhouette person_,reference_; const SpriteSheet* sheet_; const SpriteFrame& frame_; const std::filesystem::path output_;
    std::vector<float> fixed_; std::array<MonitorPersona,3> personas_{}; std::array<SilhouettePose,3> poses_{};
    std::array<std::vector<float>,3> persona_before_{};
    const std::array<RectangleF,3> screens_{RectangleF(0,0,1280,720),RectangleF(0,0,1280,720),RectangleF(0,0,640,360)};
    const Camera child_camera_{Vector2F(-800,-40),1.0f}; size_t readbacks_=0;
};

SilhouetteBodyColourProbe::SilhouetteBodyColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output) :
    app_(app),presentation_(presentation),look_(presentation.opening()),baseline_(look_),expected_(baseline_),
    person_(&look_,app.render_resources()),reference_(&expected_,app.render_resources()),
    sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))),
    output_(output),fixed_(silhouette_body_fixed_style(look_))
{
    require(look_.adult_height==.62f && look_.child_height==.44f && look_.silhouette_art.pen_width==.008f &&
        look_.silhouette_art.shade_width==.025f && look_.silhouette_art.ink==Colour(0,0,0) &&
        look_.silhouette_art.rim==Colour(52,52,52) && look_.silhouette_art.shade==Colour(8,8,8) &&
        look_.sprite_sheet=="sprite_sheet_1" && look_.fill_frame=="pixel" &&
        frame_.source_rectangle()==RectangleI(32,0,4,4) && frame_.origin()==Vector2F::ZERO,
        "Body fixture lost fixed caller geometry, opaque black ink or white-frame assumptions");
    personas_[0].mug=true; poses_[1].stand=.5f; personas_[2].child=true;
    for(size_t c=0;c<personas_.size();++c){persona_before_[c]=monitor_persona_observation(personas_[c]);}
}
void SilhouetteBodyColourProbe::unchanged() const
{
    require(&look_==&presentation_.opening() && &look_.silhouette_art==&presentation_.opening().silhouette_art &&
        silhouette_body_fixed_style(look_)==fixed_ && silhouette_body_fixed_style(expected_)==fixed_ &&
        look_.sprite_sheet==baseline_.sprite_sheet && look_.fill_frame==baseline_.fill_frame &&
        look_.palette_sheet==baseline_.palette_sheet && look_.palette_frames==baseline_.palette_frames && look_.percent_font==baseline_.percent_font &&
        sheet_==app_.render_resources()->sprite_sheet(look_.sprite_sheet) &&
        &frame_==&sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame)),
        "Body drawing/edit changed borrowed records, unrelated values or captured resources");
    for(size_t c=0;c<personas_.size();++c){require(persona_before_[c]==monitor_persona_observation(personas_[c]) &&
        poses_[c].sigh==0.0f && poses_[c].leave==0.0f && poses_[c].stand==(c==1?.5f:0.0f) &&
        screens_[c]==(c==2?RectangleF(0,0,640,360):RectangleF(0,0,1280,720)),"Body drawing changed fixed caller persona/pose/screen");}
    require(child_camera_.translation==Vector2F(-800,-40) && child_camera_.scale==1.0f,"Body child camera changed");
}
void SilhouetteBodyColourProbe::draw_person(const Silhouette& person,DrawList& list,size_t context) const
{ list.set_camera(context==2?child_camera_:Camera::DEFAULT_CAMERA); person.draw(list,screens_.at(context),personas_.at(context),poses_.at(context)); }
Frame SilhouetteBodyColourProbe::render(const std::function<void(DrawList&)>& draw)
{
    unchanged(); const Colour selected=look_.silhouette,expected_selected=expected_.silhouette;
    std::srand(31817);const int next=std::rand();std::srand(31817);
    const Canvas canvas(app_.render_resources(),Colour(32,64,96));Renderer& renderer=*app_.renderer();renderer.begin_frame();renderer.set_view_count(1);
    DrawList list=renderer.view(0);list.set_viewport(app_.viewport_manager()->fullscreen_viewport());list.set_camera(Camera::DEFAULT_CAMERA);
    list.set_filter(TextureFilter::point);canvas.draw(list,{0,0,1280,720});draw(list);++readbacks_;Frame result=read(renderer);
    if(result.width!=1280||result.height!=720){failure(0,"backbuffer-size",result);throw std::runtime_error("Body proof requires exact1280x720 backbuffer");}
    require(std::rand()==next && look_.silhouette==selected && expected_.silhouette==expected_selected,"Body draw consumed RNG or changed selected records");
    unchanged();return result;
}
Frame SilhouetteBodyColourProbe::actual(size_t context)
{
    const auto draw=[&](DrawList& list){draw_person(person_,list,context);};Frame first=render(draw);const Frame repeat=render(draw);
    if(first.pixels!=repeat.pixels){failure(context,"repeat",first,&repeat);}
    require(first.pixels==repeat.pixels,"Body frozen actual redraw changed RGBA pixels");return first;
}
Frame SilhouetteBodyColourProbe::adoption(size_t context,const Colour& body)
{ expected_.silhouette=body;return render([&](DrawList& list){draw_person(reference_,list,context);}); }
void SilhouetteBodyColourProbe::failure(size_t context,const char* stage,const Frame& first,const Frame* second) const
{
    const std::string stem=std::string("silhouette-body-failure-")+stage;
    save(first,output_/(stem+"-first.ppm"));if(second!=nullptr){save(*second,output_/(stem+"-second.ppm"));}
    std::ofstream receipt(output_/(stem+".json"));receipt<<"{\"context_index\":"<<context<<",\"stage\":\""<<stage
        <<"\",\"readbacks\":"<<readbacks_<<",\"diagnostic\":\"preserve-sites-and-investigate-source-layer-hypothesis\"}\n";
}
void SilhouetteBodyColourProbe::raw_write(DrawList& list,const RectangleI& r,const Colour& tint)
{ list.draw_sprite(sheet_->texture(),frame_.source_rectangle(),RectangleF(static_cast<float>(r.x),static_cast<float>(r.y),
    static_cast<float>(r.width),static_cast<float>(r.height)),tint,0.0f,frame_.origin(),SpriteFlip::none,0.0f); }
Frame SilhouetteBodyColourProbe::raw(size_t context,const Colour& body,SilhouetteBodyReference mode)
{
    const Frame result=render([&](DrawList& list)
    {
        raw_write(list,{20,20,3,3},Colour(255,255,255));raw_write(list,{24,20,3,3},Colour(0,0,0));
        const float factor=mode==SilhouetteBodyReference::straight?1.0f:mode==SilhouetteBodyReference::squared_base?body.a*body.a:body.a;
        const Colour tint(body.r*factor,body.g*factor,body.b*factor,body.a);
        for(const auto& patch:silhouette_body_patches(context))
        {
            const RectangleI box(patch.x-1,patch.y-1,3,3);raw_write(list,box,Colour(0,0,0));
            const int count=mode==SilhouetteBodyReference::collapsed_one?1:mode==SilhouetteBodyReference::collapsed_two?std::min(2,patch.writes):patch.writes;
            // Literal local multiplicities only. No silhouette/mass/tessellation
            // helper constructs this independent per-write framebuffer oracle.
            for(int pass=0;pass<count;++pass){raw_write(list,box,tint);}
        }
    });
    if(monitor_rgb(result,21,21)!=0xFFFFFFu || monitor_rgb(result,25,21)!=0 ||
        result.pixels[offset(result,21,21)+3]!=255 || result.pixels[offset(result,25,21)+3]!=255)
    {failure(context,"raw-calibration",result);throw std::runtime_error("Body raw WHITE/BLACK RGBA calibration failed");}
    return result;
}
void SilhouetteBodyColourProbe::uniform_patch(const Frame& frame,const SilhouetteBodyPatch& patch)
{
    const size_t first=offset(frame,patch.x,patch.y);
    for(int y=patch.y-1;y<=patch.y+1;++y){for(int x=patch.x-1;x<=patch.x+1;++x)
    {const size_t at=offset(frame,x,y);for(size_t channel=0;channel<4;++channel)
        {require(frame.pixels[at+channel]==frame.pixels[first+channel],"Body declared3x3 RGBA patch is not uniform");}}}
}
void SilhouetteBodyColourProbe::check_patches(size_t context,const Frame& actual,const Frame& raw,const Colour& body)
{
    for(const auto& patch:silhouette_body_patches(context))
    {
        uniform_patch(actual,patch);uniform_patch(raw,patch);
        for(int y=patch.y-1;y<=patch.y+1;++y){for(int x=patch.x-1;x<=patch.x+1;++x)
        {
            const size_t at=offset(actual,x,y);require(same_pixel(actual,raw,at),"Body local ordered tint reference differs from actual RGBA");
            require(actual.pixels[at+3]==255,"Body interior lost opaque BLACK undercoat alpha");
            if(body.a==0.0f){require(monitor_rgb(actual,x,y)==0,"Zero-alpha body must retain BLACK ink rather than Canvas");}
            if(body.a==1.0f)
            {const auto rgb=(static_cast<std::uint32_t>(std::round(body.r*255))<<16)|(static_cast<std::uint32_t>(std::round(body.g*255))<<8)|
                static_cast<std::uint32_t>(std::round(body.b*255));require(monitor_rgb(actual,x,y)==rgb,"Body opaque literal RGB calibration differs");}
        }}
    }
}
void SilhouetteBodyColourProbe::check_control(size_t context,const Frame& actual,const Frame& wrong,SilhouetteBodyReference mode,
    size_t& rejected,size_t& unchanged_count)
{
    for(const auto& patch:silhouette_body_patches(context))
    {
        uniform_patch(actual,patch);uniform_patch(wrong,patch);
        const bool affected=mode==SilhouetteBodyReference::collapsed_one?patch.writes>1:
            mode==SilhouetteBodyReference::collapsed_two?patch.writes>2:true;
        for(int y=patch.y-1;y<=patch.y+1;++y){for(int x=patch.x-1;x<=patch.x+1;++x)
        {const bool equal=same_pixel(actual,wrong,offset(actual,x,y));require(affected?!equal:equal,
            "Body wrong-reference control did not separately reject/preserve every declared patch pixel");}}
        if(affected){++rejected;}else{++unchanged_count;}
    }
}
void SilhouetteBodyColourProbe::exterior(size_t context,const Frame& actual)
{
    const RectangleI box=footprint(context);
    for(int y=0;y<actual.height;++y){for(int x=0;x<actual.width;++x)
    {if(x<box.x||x>=box.x+box.width||y<box.y||y>=box.y+box.height)
        {const size_t at=offset(actual,x,y);require(actual.pixels[at]==32&&actual.pixels[at+1]==64&&actual.pixels[at+2]==96&&actual.pixels[at+3]==255,
            "Body actual art escaped loose source-derived exterior bound");}}}
}
void SilhouetteBodyColourProbe::delta(size_t context,const Frame& actual,const Frame& baseline)
{
    const RectangleI box=footprint(context);
    for(int y=0;y<actual.height;++y){for(int x=0;x<actual.width;++x)
    {if(x<box.x||x>=box.x+box.width||y<box.y||y>=box.y+box.height)
        {require(same_pixel(actual,baseline,offset(actual,x,y)),"Body selected delta escaped loose exterior bound");}}}
}
Frame SilhouetteBodyColourProbe::compact(size_t context,const Frame& source)
{
    if(context==2){return foreground_crop(source,{1016,224,208,224},0);}
    const std::array<Vector2I,2> origins=context==0?std::array<Vector2I,2>{Vector2I(560,304),Vector2I(856,496)}:
        std::array<Vector2I,2>{Vector2I(560,124),Vector2I(560,496)};
    Frame result{320,224,{}};result.pixels.reserve(320*224*4);
    for(int y=0;y<224;++y){for(const auto& origin:origins)
    {const size_t at=offset(source,origin.x,origin.y+y);result.pixels.insert(result.pixels.end(),source.pixels.begin()+at,source.pixels.begin()+at+160*4);}}
    return result;
}

void silhouette_body_colour_consumer_checks(Application& app,const Presentation& presentation,const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);app.resolution_manager()->set_resolution_exactly({1280,720});
    std::ignore=app.renderer()->window_size_changed(1280,720);Presentation local=presentation;
    auto& look=const_cast<OpeningPresentation&>(local.opening());require(look.silhouette==Colour(14,14,14),"Body authored default changed");
    SilhouetteBodyColourProbe probe(app,local,output);
    const std::array<Colour,4> colours{Colour(14,14,14),Colour(53,188,225),Colour(32,96,192,128),Colour(165,50,214,0)};
    constexpr std::array<const char*,4> states{"default","rgb","rgba128","alpha0"};
    std::array<Frame,3> baseline{};size_t rows=0,crops=0,controls=0,rejected=0,unchanged=0;
    std::ofstream records(output/"silhouette-body-colour-consumers.csv");
    records<<"state,context,background,patch_count,write_counts,raw_patch_channels,uniform_rgba,adoption_match,exterior_and_delta,frozen_inputs_resources_rng,controls,affected_rejections,unaffected_guards,scope\n";
    for(size_t c=0;c<3;++c){for(size_t p=0;p<colours.size();++p)
    {
        look.silhouette=colours[p];const Frame actual=probe.actual(c),adoption=probe.adoption(c,colours[p]),raw=probe.raw(c,colours[p]);
        try
        {
            require(actual.pixels==adoption.pixels,"Body copied full-frame consumer adoption differs");
            SilhouetteBodyColourProbe::check_patches(c,actual,raw,colours[p]);SilhouetteBodyColourProbe::exterior(c,actual);
            if(p==0){baseline[c]=actual;}else{SilhouetteBodyColourProbe::delta(c,actual,baseline[c]);require(actual.pixels!=baseline[c].pixels,"Body edit produced no actual delta");}
            size_t row_controls=0,row_rejected=0,row_unchanged=0;
            const auto control=[&](SilhouetteBodyReference mode)
            {const Frame wrong=probe.raw(c,colours[p],mode);
                try{SilhouetteBodyColourProbe::check_control(c,actual,wrong,mode,row_rejected,row_unchanged);}
                catch(...){probe.failure(c,"wrong-reference",actual,&wrong);throw;}++row_controls;};
            if(p==2){control(SilhouetteBodyReference::straight);control(SilhouetteBodyReference::squared_base);
                control(SilhouetteBodyReference::collapsed_one);if(c==0){control(SilhouetteBodyReference::collapsed_two);}}
            if(p==3){control(SilhouetteBodyReference::straight);}
            controls+=row_controls;rejected+=row_rejected;unchanged+=row_unchanged;
            records<<states[p]<<','<<silhouette_body_context_name(c)<<",canvas32-64-96,"<<silhouette_body_patches(c).size()<<','<<silhouette_body_write_counts(c)
                <<",true,true,true,true,true,"<<row_controls<<','<<row_rejected<<','<<row_unchanged<<",consumer-only\n";++rows;
            save(SilhouetteBodyColourProbe::compact(c,actual),output/(std::string("silhouette-body-colour-")+silhouette_body_context_name(c)+"-"+states[p]+".ppm"));++crops;
        }
        catch(...){probe.failure(c,"shared-assertion",actual,&raw);throw;}
    }}
    require(records.good()&&probe.readbacks()==61&&rows==12&&crops==12&&controls==13&&rejected==34&&unchanged==8,"Body shared exact budget differs");
    std::ofstream summary(output/"silhouette-body-colour-consumer-summary.json");summary<<"{\"scope\":\"consumer-only\",\"readbacks\":61,\"csv_rows\":12,\"crops\":12,\"crop_pixels\":759808,\"ppm_bytes\":2279604,\"contexts\":3,"
        "\"patches\":9,\"patch_width\":3,\"patch_height\":3,\"raw_effective_underlays\":1,\"blank_reads\":0,\"controls\":13,\"partial_straight_reads\":3,\"partial_squared_reads\":3,\"collapsed_one_reads\":3,\"collapsed_two_reads\":1,"
        "\"alpha0_straight_reads\":3,\"affected_rejections\":34,\"unaffected_guards\":8,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,"
        "\"viewport\":\"1280x720\",\"event_fade\":false,\"whole_frame_reference\":\"copied-production-consumer-adoption-only\",\"raw_reference\":\"nine-local-ordered-rgba-patches-only\",\"adult_crop\":[320,224],\"child_crop\":[208,224]}\n";
    require(summary.good(),"Cannot write Body shared summary");
    std::cout<<"PASS silhouette-body:61readbacks/12rows/12crops759808pixels;9local3x3 RGBA patches, ordered1/2/3 separate writes overBLACK,13controls/34patch refusals/8unchanged guards. "
        "Copied full frame adoption only; frozen caller/resource/RNG and zero all updates; no complete-mask or actual-Level claim.\n";
}
#if COLOURWARS_DEV_TOOLS
void silhouette_body_tune_colour_checks(Application& app,Registry<LevelLoadedInfo>& levels,GameTuning& tuning,Presentation& presentation,
    const std::filesystem::path& content,const std::filesystem::path& output)
{
    const auto root=output/"silhouette-body-colour-tune-scratch",source=root/"source",staged=root/"staged";
    std::filesystem::create_directories(source);std::filesystem::create_directories(staged);
    constexpr std::array<const char*,2> files{"tuning.json","presentation.json"};std::array<std::string,2> real_bytes;
    for(size_t i=0;i<files.size();++i){real_bytes[i]=tune_colour_bytes(content/files[i]);
        tune_colour_write(source/files[i],real_bytes[i]);tune_colour_write(staged/files[i],real_bytes[i]);}
    const NestedSaveProfile profile(root/"profile");const TuneColourTemp temporary(root/"temp");
    struct RestoreSilhouetteBody {GameTuning& t;Presentation& p;GameTuning old_t;Presentation old_p;~RestoreSilhouetteBody(){t=old_t;p=old_p;}}
        restore{tuning,presentation,tuning,presentation};
    Save preferences;PlaySettings play;play.music_volume=17;play.sfx_volume=83;play.aim_sensitivity=4;
    play.view_shake=false;play.aim_marker=true;play.reduce_effects=true;play.sticks[2].movement_deadzone=25;play.sticks[2].aim_deadzone=70;
    InputBindings bindings;bindings.keyboard_jump=Key::j;
    require(preferences.set_play_settings_and_save(play)&&preferences.set_input_bindings_and_save(bindings),"Cannot seed Body private preferences");
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
    require(impl.log().set_transcript((output/"silhouette-body-colour-tune-commands.log").string()),"Cannot retain Body transcript");
    SilhouetteBodyColourProbe probe(app,presentation,output);JsonSpanDocument initial(real_bytes[1],"Body private source");
    const JsonLeaf* leaf=initial.find("opening.silhouette");require(leaf!=nullptr,"Body source lacks exact existing token");
    const std::string expected_saved=initial.replaced(*leaf,"\"2060C080\"");bool persisted=false;size_t commands=0,rows=0;
    const auto unchanged=[&]
    {probe.unchanged();require(!impl.can_apply()&&impl.request().empty()&&!impl.in_match()&&tune_colour_bytes(save_path)==saved_preferences&&pref_values()==saved_values,
        "Body commands changed private prefs/default display/JumpJ or queued work");
        for(size_t i=0;i<files.size();++i){const auto& expected=i==1&&persisted?expected_saved:real_bytes[i];
            require(tune_colour_bytes(content/files[i])==real_bytes[i]&&tune_colour_bytes(source/files[i])==expected&&tune_colour_bytes(staged/files[i])==expected,
                "Body command changed actual content/private bytes beyond exact one token");}};
    const auto command=[&](const std::string& line)
    {const size_t before=impl.log().lines().size();std::srand(31817);const int next=std::rand();std::srand(31817);
        const auto status=impl.execute(line);++commands;require(status.ok&&std::rand()==next,"Actual Body command failed or consumed RNG");
        if(line.starts_with("tune ")){const auto& log=impl.log().lines();require(log.size()>before&&log.back().severity==DebugSeverity::info&&
            log.back().text.starts_with("tune: presentation.opening.silhouette")&&log.back().text.ends_with("(live)"),"Body live reply differs");}};
    std::array<Frame,3> initial_raster{},saved_raster{};
    std::ofstream records(output/"silhouette-body-colour-tune.csv");
    records<<"state,context,background,patch_count,write_counts,raw_patch_channels,uniform_rgba,adoption_match,exterior_and_delta,frozen_inputs_resources_rng,private_bytes,adoption\n";
    const auto check=[&](const char* state,const Colour& expected)
    {
        unchanged();require(presentation.opening().silhouette==expected,"Body command parsed wrong literal channels");
        for(size_t c=0;c<3;++c)
        {
            const Frame actual=probe.actual(c),adoption=probe.adoption(c,expected),raw=probe.raw(c,expected);
            try
            {
                require(actual.pixels==adoption.pixels,"Body copied full-consumer adoption differs after actual command");
                SilhouetteBodyColourProbe::check_patches(c,actual,raw,expected);SilhouetteBodyColourProbe::exterior(c,actual);
                if(std::string_view(state)=="baseline"){initial_raster[c]=actual;}else{SilhouetteBodyColourProbe::delta(c,actual,initial_raster[c]);}
                if(std::string_view(state)=="reset-original"){require(actual.pixels==initial_raster[c].pixels,"Body original reset failed whole-frame restoration");}
                if(std::string_view(state)=="rgba128"){saved_raster[c]=actual;}
                if(std::string_view(state)=="reset-saved"||std::string_view(state)=="reset-all-saved")
                {require(actual.pixels==saved_raster[c].pixels,"Body saved reset failed exact whole-consumer RGBA restoration");}
                records<<state<<','<<silhouette_body_context_name(c)<<",canvas32-64-96,"<<silhouette_body_patches(c).size()<<','<<silhouette_body_write_counts(c)
                    <<",true,true,true,true,true,true,live\n";++rows;
            }
            catch(...){probe.failure(c,"debug-assertion",actual,&raw);throw;}
        }
        unchanged();
    };
    check("baseline",Colour(14,14,14));command("tune presentation.opening.silhouette RED");check("named",Colour(255,0,0));
    command("tune.reset presentation.opening.silhouette");check("reset-original",Colour(14,14,14));
    command("tune presentation.opening.silhouette 35BCE1");check("rgb",Colour(53,188,225));
    command("tune presentation.opening.silhouette 2060C080");check("rgba128",Colour(32,96,192,128));
    const auto* document=impl.tune_document("presentation");require(document!=nullptr&&document->edits().size()==1&&document->edits().contains("opening.silhouette"),"Body save ledger differs from one exact token");
    command("tune.save presentation");persisted=true;unchanged();require(impl.tune_document("presentation")->edits().empty(),"Body Save failed to rebase");
    command("tune presentation.opening.silhouette A532D600");check("alpha0",Colour(165,50,214,0));
    command("tune.reset presentation.opening.silhouette");check("reset-saved",Colour(32,96,192,128));
    command("tune presentation.opening.silhouette 35BCE1");check("rgb-after-save",Colour(53,188,225));
    command("tune.reset presentation");check("reset-all-saved",Colour(32,96,192,128));
    require(impl.tune_document("presentation")->edits().empty()&&impl.tune_document("tuning")->edits().empty(),"Body final reset left edits");
    require(records.good()&&commands==9&&rows==27&&probe.readbacks()==108,"Body Debug exact budget differs");
    std::ofstream summary(output/"silhouette-body-colour-tune-summary.json");summary<<"{\"scope\":\"debug-validated-commands\",\"commands\":9,\"states\":9,\"contexts\":3,\"readbacks\":108,\"csv_rows\":27,\"crops\":0,\"saved_tokens\":1,\"event_fade\":false,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0}\n";
    require(summary.good(),"Cannot write Body Debug summary");
    std::cout<<"PASS silhouette-body tune:9real commands/9states/3contexts/108readbacks/27rows; one exact Body token Save/rebase, original+saved whole-RGBA resets, "
        "private PlaySettings+JumpJ/default display and actual content retained; nine local ordered patches, zero all updates.\n";
}
#endif
