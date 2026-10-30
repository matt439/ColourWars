// Crack42: complete independent115x34 front-details ROI per source path.
// Copied complete front-details frames establish adoption only, not full art.
enum class CrackReference { correct, straight, squared_base, collapsed_coverage, all_ink_first };
std::vector<float> crack_fixed_style(const OpeningPresentation& p)
{
    OpeningPresentation stable=p;stable.crack=Colour(58,58,58);auto values=program_fixed_style(stable);
    // program_fixed_style omits program_colour: append the ACTUAL input record,
    // never a baseline-substituted value, for live AND each copied look.
    values.insert(values.end(),{p.program_colour.r,p.program_colour.g,p.program_colour.b,p.program_colour.a});return values;
}
class CrackColourProbe
{
public:
    CrackColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output);
    void unchanged() const;
    Frame actual(size_t context);
    Frame adoption(size_t context,const Colour& colour);
    Frame raw(size_t context,const Colour& colour,CrackReference mode=CrackReference::correct);
    static void check_roi(size_t context,const Frame& actual,const Frame& raw,const Colour& colour);
    static void check_control(size_t context,const Frame& actual,const Frame& positive_raw,const Frame& wrong,CrackReference mode,size_t& changed,size_t& unchanged_pixels);
    static void delta_only_effective(size_t context,const Frame& actual,const Frame& baseline);
    static Frame compact(size_t context,const Frame& source){return foreground_crop(source,{crack_left(context),472,115,34},0);}
    void failure(size_t context,const char* state,const char* stage,const Frame& actual,const Frame* other=nullptr) const;
    size_t readbacks() const{return readbacks_;}size_t calibration_checks() const{return calibration_checks_;}
private:
    static size_t offset(const Frame& frame,int x,int y){return (static_cast<size_t>(y)*static_cast<size_t>(frame.width)+static_cast<size_t>(x))*4;}
    void write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint);
    Frame render(size_t context,const std::function<void(DrawList&)>& draw);
    const Monitor& monitor(size_t context) const{return context==0?monitor_a_:monitor_b_;}
    const Monitor& reference(size_t context) const{return context==0?reference_a_:reference_b_;}
    OpeningPresentation& expected(size_t context){return context==0?expected_a_:expected_b_;}
    Application& app_;const Presentation& presentation_;const OpeningPresentation& look_;OpeningPresentation baseline_,expected_a_,expected_b_;
    Monitor monitor_a_,monitor_b_,reference_a_,reference_b_;Canvas canvas_;
    RenderResources::SpriteSheetHandle sheet_handle_,palette_handle_;const SpriteSheet* sheet_;const SpriteSheet* palette_sheet_;
    SpriteSheet::frame_handle frame_handle_;const SpriteFrame& frame_;std::vector<SpriteSheet::frame_handle> palette_handles_;
    std::vector<const SpriteFrame*> palette_frames_;const std::filesystem::path output_;std::vector<float> fixed_;
    std::array<std::vector<float>,2> geometry_{};const RectangleF screen_{0,0,1280,720};const Camera camera_=Camera::DEFAULT_CAMERA;
    size_t readbacks_=0,calibration_checks_=0;
};
CrackColourProbe::CrackColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output):
    app_(app),presentation_(presentation),look_(presentation.opening()),baseline_(look_),expected_a_(baseline_),expected_b_(baseline_),
    monitor_a_(&look_,app.render_resources(),crack_persona(0)),monitor_b_(&look_,app.render_resources(),crack_persona(1)),
    reference_a_(&expected_a_,app.render_resources(),crack_persona(0)),reference_b_(&expected_b_,app.render_resources(),crack_persona(1)),
    canvas_(app.render_resources(),Colour(32,64,96)),sheet_handle_(app.render_resources()->resolve_sprite_sheet(look_.sprite_sheet)),
    palette_handle_(app.render_resources()->resolve_sprite_sheet(look_.palette_sheet)),sheet_(app.render_resources()->sprite_sheet(sheet_handle_)),
    palette_sheet_(app.render_resources()->sprite_sheet(palette_handle_)),frame_handle_(sheet_->resolve_sprite_frame(look_.fill_frame)),
    frame_(sheet_->sprite_frame(frame_handle_)),output_(output),fixed_(crack_fixed_style(look_))
{
    require(look_.sprite_sheet=="sprite_sheet_1"&&look_.fill_frame=="pixel"&&frame_.source_rectangle()==RectangleI(32,0,4,4)&&frame_.origin()==Vector2F::ZERO&&
        look_.monitor_art.pen_width==.004f&&look_.monitor_art.corner_step==.012f&&look_.monitor_art.ink==Colour(0,0,0)&&look_.monitor_art.highlight==Colour(238,238,238),
        "Crack fixture lost white-frame metadata, guarded geometry or BLACK/EEEEEE materials");
    for(const auto& name:look_.palette_frames)
    {const auto handle=palette_sheet_->resolve_sprite_frame(name);palette_handles_.push_back(handle);palette_frames_.push_back(&palette_sheet_->sprite_frame(handle));}
    for(size_t c=0;c<2;++c){geometry_[c]=glass_monitor_geometry(monitor(c));std::ignore=crack_coverage(c);}unchanged();
}
void CrackColourProbe::unchanged() const
{
    auto* resources=app_.render_resources();
    require(&look_==&presentation_.opening()&& &look_.monitor_art==&presentation_.opening().monitor_art&&
        sheet_handle_==resources->resolve_sprite_sheet(look_.sprite_sheet)&&palette_handle_==resources->resolve_sprite_sheet(look_.palette_sheet)&&
        sheet_==resources->sprite_sheet(sheet_handle_)&&palette_sheet_==resources->sprite_sheet(palette_handle_)&&
        frame_handle_==sheet_->resolve_sprite_frame(look_.fill_frame)&& &frame_==&sheet_->sprite_frame(frame_handle_)&&
        screen_==RectangleF(0,0,1280,720)&&camera_.translation==Vector2F::ZERO&&camera_.scale==1.0f&&
        app_.viewport_manager()->fullscreen_viewport().pixel_rect()==RectangleI(0,0,1280,720),"Crack changed borrowed addresses/resources/caller screen/camera");
    for(const OpeningPresentation* style:{&look_,&expected_a_,&expected_b_})
    {
        require(crack_fixed_style(*style)==fixed_&&style->program_colour==baseline_.program_colour&&
            style->sprite_sheet==baseline_.sprite_sheet&&style->fill_frame==baseline_.fill_frame&&style->palette_sheet==baseline_.palette_sheet&&
            style->palette_frames==baseline_.palette_frames&&style->percent_font==baseline_.percent_font&&style->bezel_shades==baseline_.bezel_shades&&
            style->sticky_notes==baseline_.sticky_notes&&style->stickers==baseline_.stickers,"Crack changed actual/copy program RGBA or complete unselected look/arrays/names");
    }
    for(size_t c=0;c<2;++c)
    {
        const auto persona=crack_persona(c);
        for(const Monitor* item:{&monitor(c),&reference(c)})
        {const auto& p=item->persona();require(p.sticky_notes.empty()&&p.stickers.empty()&&p.crack.size()==5&&
            monitor_persona_observation(p)==monitor_persona_observation(persona),"Crack changed full captured persona or point order");}
        require(glass_monitor_geometry(monitor(c))==geometry_[c]&&glass_monitor_geometry(reference(c))==geometry_[c]&&
            monitor(c).screen_for_glass({100,80,640,360})==reference(c).screen_for_glass({100,80,640,360}),"Crack public retained Monitor geometry changed");
    }
    for(size_t i=0;i<look_.palette_frames.size();++i)
    {require(palette_handles_[i]==palette_sheet_->resolve_sprite_frame(look_.palette_frames[i])&&palette_frames_[i]==&palette_sheet_->sprite_frame(palette_handles_[i]),
        "Crack captured swatch resource changed");}
}
void CrackColourProbe::write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint)
{
    list.draw_sprite(sheet_->texture(),frame_.source_rectangle(),RectangleF(static_cast<float>(left),static_cast<float>(top),
        static_cast<float>(right-left),static_cast<float>(bottom-top)),tint,0.0f,frame_.origin(),SpriteFlip::none,0.0f);
}
Frame CrackColourProbe::render(size_t context,const std::function<void(DrawList&)>& draw)
{
    unchanged();const Colour selected=look_.crack,a=expected_a_.crack,b=expected_b_.crack;
    std::srand(71893);const int next=std::rand();std::srand(71893);Renderer& renderer=*app_.renderer();renderer.begin_frame();renderer.set_view_count(1);
    DrawList list=renderer.view(0);list.set_viewport(app_.viewport_manager()->fullscreen_viewport());list.set_camera(camera_);list.set_filter(TextureFilter::point);
    canvas_.draw(list,screen_);draw(list);
    // Fixed unselected overlays on EVERY actual/copied/raw/control read. They
    // stay in all full-frame guards; they are not mask exemptions or extra reads.
    list.set_filter(TextureFilter::point);write_raw(list,20,20,24,24,Colour(255,255,255));write_raw(list,25,20,29,24,Colour(0,0,0));
    Frame result=read(renderer);++readbacks_;
    if(result.width!=1280||result.height!=720){failure(context,"unknown","backbuffer-size",result);throw std::runtime_error("Crack requires1280x720 native frame");}
    bool calibrated=true;for(int y=20;y<24;++y){for(int x=20;x<29;++x)
    {if(x==24){continue;}const size_t at=offset(result,x,y);const unsigned char value=x<24?255:0;
        calibrated=calibrated&&result.pixels[at]==value&&result.pixels[at+1]==value&&result.pixels[at+2]==value&&result.pixels[at+3]==255;}}
    if(!calibrated){failure(context,"unknown","WHITE-BLACK-calibration",result);}require(calibrated,"Crack actual WHITE/BLACK32pixel RGBA calibration differs");calibration_checks_+=32;
    const bool frozen=std::rand()==next&&look_.crack==selected&&expected_a_.crack==a&&expected_b_.crack==b;
    if(!frozen){failure(context,"unknown","draw-state-or-rng",result);}require(frozen,"Crack draw changed selected records or consumed RNG");unchanged();return result;
}
Frame CrackColourProbe::actual(size_t context)
{
    const auto draw=[&](DrawList& list){monitor(context).draw_front_details(list,screen_);};Frame first=render(context,draw);const Frame repeat=render(context,draw);
    if(first.pixels!=repeat.pixels){failure(context,"unknown","repeat",first,&repeat);}require(first.pixels==repeat.pixels,"Crack frozen front-details repeat changed RGBA");return first;
}
Frame CrackColourProbe::adoption(size_t context,const Colour& colour)
{expected(context).crack=colour;return render(context,[&](DrawList& list){reference(context).draw_front_details(list,screen_);});}
Frame CrackColourProbe::raw(size_t context,const Colour& colour,CrackReference mode)
{
    return render(context,[&](DrawList& list)
    {
        if(context==0){write_raw(list,5,8,8,711,Colour(238,238,238));} // Only front-details-only left prefix.
        const float factor=mode==CrackReference::straight?1.0f:mode==CrackReference::squared_base?colour.a*colour.a:colour.a;
        const Colour tint(colour.r*factor,colour.g*factor,colour.b*factor,colour.a);
        const auto write=[&](const CrackDab& d){write_raw(list,d.left,d.top,d.right,d.bottom,d.selected?tint:Colour(0,0,0));};
        if(mode==CrackReference::all_ink_first||mode==CrackReference::collapsed_coverage)
        {
            for(const auto& d:crack_dabs(context)){if(!d.selected){write(d);}}
            if(mode==CrackReference::all_ink_first){for(const auto& d:crack_dabs(context)){if(d.selected){write(d);}}}
            else
            {const auto& map=crack_coverage(context);const int left=crack_left(context);
                for(int y=472;y<506;++y){for(int x=left;x<left+115;++x){if(map.effective[static_cast<size_t>((y-472)*115+x-left)]){write_raw(list,x,y,x+1,y+1,tint);}}}}
        }
        else{for(const auto& d:crack_dabs(context)){write(d);}} // Persegment ink then thin in frozen original order.
    });
}
void CrackColourProbe::failure(size_t context,const char* state,const char* stage,const Frame& actual,const Frame* other) const
{
    const std::string stem=std::string("crack-failure-")+crack_context_name(context)+"-"+state+"-"+stage;save(actual,output_/(stem+"-actual.ppm"));
    if(other!=nullptr){save(*other,output_/(stem+"-reference.ppm"));}
    const auto rgba=[&](const Frame& f,const char* suffix){std::ofstream bytes(output_/(stem+suffix),std::ios::binary);
        bytes.write(reinterpret_cast<const char*>(f.pixels.data()),static_cast<std::streamsize>(f.pixels.size()));bytes.close();return !bytes.fail();};
    const bool actual_written=rgba(actual,"-actual.rgba"),other_written=other!=nullptr&&rgba(*other,"-reference.rgba");
    std::ofstream receipt(output_/(stem+".json"));receipt<<"{\"context\":\""<<crack_context_name(context)<<"\",\"state\":\""<<state<<"\",\"stage\":\""<<stage
        <<"\",\"readbacks\":"<<readbacks_<<",\"roi\":["<<crack_left(context)<<",472,"<<crack_left(context)+115<<",506],\"frame_width\":"<<actual.width
        <<",\"frame_height\":"<<actual.height<<",\"rgba_order\":\"RGBA8-row-major\",\"actual_rgba_bytes\":"<<actual.pixels.size()
        <<",\"actual_rgba_written\":"<<(actual_written?"true":"false")<<",\"reference_rgba_written\":"<<(other_written?"true":"false")
        <<",\"source_models\":\"F_AF|Q_AF|F_AQ|Q_AQ\",\"diagnostic\":\"preserve-failures-source-order-and-channels-no-capture-fitting\"}\n";
}
void CrackColourProbe::check_roi(size_t context,const Frame& actual,const Frame& raw,const Colour& colour)
{
    const int left=crack_left(context);const auto& map=crack_coverage(context);
    // Complete3910 actual/raw RGBA and opaque-alpha checks FIRST. Supplementary
    // source channels do not substitute for the independent ordered geometry.
    for(int y=472;y<506;++y){for(int x=left;x<left+115;++x)
    {const size_t at=offset(actual,x,y);require(same_pixel(actual,raw,at),"Crack independent ordered ROI differs from actual RGBA");
        require(actual.pixels[at+3]==255,"Crack ROI lost its opaque underlayer alpha");}}
    std::uint32_t opaque=0;
    if(colour.a==1.0f)
    {opaque=(static_cast<std::uint32_t>(std::round(colour.r*255))<<16)|(static_cast<std::uint32_t>(std::round(colour.g*255))<<8)|
        static_cast<std::uint32_t>(std::round(colour.b*255));}
    else if(colour.a!=0.0f){require(colour==Colour(32,96,192,128),"Crack source channels cover only the declared partial palette");}
    for(int y=472;y<506;++y){for(int x=left;x<left+115;++x)
    {
        const size_t local=static_cast<size_t>((y-472)*115+x-left);const auto count=map.effective[local];
        std::uint32_t expected=map.ink[local]?0x000000u:context==0&&x>=5&&x<8?0xEEEEEEu:0x204060u;
        if(count)
        {
            if(colour.a==1.0f){expected=opaque;}
            else if(colour.a==0.0f){expected=0x000000u;}
            else{expected=count==1?0x103060u:0x184890u;}
        }
        require(monitor_rgb(actual,x,y)==expected,"Crack complete ROI differs from frozen literal source channels/order");
    }}
    // F_AF/Q_AF/F_AQ/Q_AQ are equivalent over EACH complete declared ROI.
    // Exact whole-ROI admission here is not a claim about GPU internals or a
    // universal precision guarantee, and admits no per-pixel model fitting.
}
void CrackColourProbe::delta_only_effective(size_t context,const Frame& actual,const Frame& baseline)
{
    const int left=crack_left(context);const auto& map=crack_coverage(context);
    for(int y=0;y<actual.height;++y){for(int x=0;x<actual.width;++x)
    {
        if(same_pixel(actual,baseline,offset(actual,x,y))){continue;}
        require(x>=left&&x<left+115&&y>=472&&y<506&&map.effective[static_cast<size_t>((y-472)*115+x-left)]!=0,
            "Crack edit escaped final selected support or changed calibration/unselected full-frame bytes");
    }}
}
void CrackColourProbe::check_control(size_t context,const Frame& actual,const Frame& positive_raw,const Frame& wrong,
    CrackReference mode,size_t& changed,size_t& unchanged_pixels)
{
    require(mode!=CrackReference::correct,"Crack positive reference cannot be a negative control");
    const int left=crack_left(context);const auto& map=crack_coverage(context);const size_t before=changed,before_unchanged=unchanged_pixels;
    for(int y=472;y<506;++y){for(int x=left;x<left+115;++x)
    {
        const size_t local=static_cast<size_t>((y-472)*115+x-left),at=offset(actual,x,y);
        const bool discriminate=mode==CrackReference::collapsed_coverage?map.effective[local]>1:
            mode==CrackReference::all_ink_first?map.ever[local]!=map.effective[local]:map.effective[local]!=0;
        require(wrong.pixels[at+3]==255,"Crack wrong reference changed opaque ROI alpha");
        if(discriminate){require(monitor_rgb(actual,x,y)!=monitor_rgb(wrong,x,y),"Crack negative failed an owned discriminating pixel");++changed;}
        else{require(same_pixel(actual,wrong,at),"Crack negative changed its exact unchanged complement");++unchanged_pixels;}
    }}
    const size_t expected=mode==CrackReference::collapsed_coverage?size_t{90}:mode==CrackReference::all_ink_first?
        (context==0?size_t{11}:size_t{12}):(context==0?size_t{179}:size_t{178});
    require(changed-before==expected&&unchanged_pixels-before_unchanged==3910-expected,"Crack per-control changed/complement counts differ");
    for(int y=0;y<wrong.height;++y){for(int x=0;x<wrong.width;++x)
    {
        if(x>=left&&x<left+115&&y>=472&&y<506&&map.ever[static_cast<size_t>((y-472)*115+x-left)]){continue;}
        require(same_pixel(positive_raw,wrong,offset(wrong,x,y)),"Crack wrong reference escaped ever-selected support/calibration guard");
    }}
}

void crack_colour_consumer_checks(Application& app,const Presentation& presentation,const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);app.resolution_manager()->set_resolution_exactly({1280,720});
    std::ignore=app.renderer()->window_size_changed(1280,720);Presentation local=presentation;
    auto& look=const_cast<OpeningPresentation&>(local.opening());require(look.crack==Colour(58,58,58),"Crack authored default changed");
    CrackColourProbe probe(app,local,output);const std::array<Colour,4> colours{
        Colour(58,58,58),Colour(53,188,225),Colour(32,96,192,128),Colour(165,50,214,0)};
    constexpr std::array<const char*,4> states{"default","rgb","rgba128","alpha0"};
    std::array<Frame,2> baseline{};size_t rows=0,crops=0,controls=0,changed=0,unchanged_pixels=0;
    std::ofstream records(output/"crack-colour-consumers.csv");
    records<<"state,context,background,roi_rgba_match,adoption_match,repeat_match,literal_channels,effective_delta,frozen_inputs_resources_rng,calibration_rgba,source_models,controls,changed_control_pixels,unchanged_control_pixels,scope\n";
    for(size_t c=0;c<2;++c){for(size_t p=0;p<colours.size();++p)
    {
        look.crack=colours[p];Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual(c);adoption=probe.adoption(c,colours[p]);raw=probe.raw(c,colours[p]);
            require(actual.pixels==adoption.pixels,"Crack copied complete front-details adoption differs");
            CrackColourProbe::check_roi(c,actual,raw,colours[p]);if(p==0){baseline[c]=actual;}
            CrackColourProbe::delta_only_effective(c,actual,baseline[c]);
            if(p>0){require(actual.pixels!=baseline[c].pixels,"Crack edited palette produced no visible selected delta");}
            size_t case_controls=0,case_changed=0,case_unchanged=0;
            const auto control=[&](CrackReference mode,const char* label)
            {
                const Frame wrong=probe.raw(c,colours[p],mode);
                try{CrackColourProbe::check_control(c,actual,raw,wrong,mode,case_changed,case_unchanged);}
                catch(...){probe.failure(c,states[p],label,actual,&wrong);throw;}++case_controls;
            };
            if(p==2)
            {control(CrackReference::straight,"partial-straight");control(CrackReference::squared_base,"partial-squared");
                control(CrackReference::collapsed_coverage,"partial-collapsed");control(CrackReference::all_ink_first,"partial-all-ink-first");}
            else if(p==3){control(CrackReference::straight,"alpha0-straight");}
            records<<states[p]<<','<<crack_context_name(c)<<",canvas32-64-96,true,true,true,true,true,true,true,F_AF|Q_AF|F_AQ|Q_AQ,"
                <<case_controls<<','<<case_changed<<','<<case_unchanged<<",consumer-only\n";
            ++rows;controls+=case_controls;changed+=case_changed;unchanged_pixels+=case_unchanged;
            save(CrackColourProbe::compact(c,actual),output/(std::string("crack-colour-")+crack_context_name(c)+"-"+states[p]+".ppm"));++crops;
        }
        catch(...)
        {
            if(!actual.pixels.empty()){probe.failure(c,states[p],"case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){probe.failure(c,states[p],"adoption",adoption);}throw;
        }
    }}
    require(records.good()&&probe.readbacks()==42&&probe.calibration_checks()==1344&&rows==8&&crops==8&&controls==10&&changed==1274&&unchanged_pixels==37826,
        "Crack exact shared readback/calibration/crop/control/mask budget differs");
    std::ofstream summary(output/"crack-colour-consumer-summary.json");
    summary<<"{\"scope\":\"consumer-only\",\"viewport\":\"1280x720\",\"background\":\"canvas32-64-96\",\"event_fade\":false,\"actual_entry\":\"Monitor::draw_front_details-only-plus-calibration\",\"raw_reference\":\"ordered-segment-ink-then-thin-literal115x34-interior-only\",\"whole_frame_reference\":\"copied-complete-frontdetails-adoption-only\",\"left_roi_half_open\":[2,472,117,506],\"right_roi_half_open\":[1162,472,1277,506],\"capture_kind\":\"native-contiguous-ROI\",\"source_models\":\"F_AF|Q_AF|F_AQ|Q_AQ\",\"readbacks\":42,\"csv_rows\":8,\"crops\":8,\"crop_width\":115,\"crop_height\":34,\"crop_pixels\":31280,\"ppm_bytes\":93952,\"contexts\":2,\"palettes\":4,\"roi_pixels\":3910,\"base_case_reads\":32,\"actual_reads\":16,\"adoption_reads\":8,\"raw_positive_reads\":8,\"blank_reads\":0,\"calibration_only_reads\":0,\"calibration_pixels_per_read\":32,\"calibration_pixel_checks\":1344,\"controls\":10,\"partial_straight_reads\":2,\"partial_squared_reads\":2,\"collapsed_reads\":2,\"all_ink_first_reads\":2,\"alpha0_straight_reads\":2,\"changed_control_pixels\":1274,\"unchanged_control_pixels\":37826,\"ink_submissions_per_path\":77,\"selected_submissions_per_path\":165,\"left_effective_pixels\":179,\"right_effective_pixels\":178,\"ever_selected_pixels_per_path\":182,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0}\n";
    require(summary.good(),"Cannot retain Crack shared summary");
    std::cout<<"PASS crack consumer:42readbacks/8rows/8native115x34 contiguous ROI crops;3910independentRGBA/case,10controls1274changed37826unchanged,"
        "1344WHITE-BLACK calibration checks; exact coherent source representatives, copied front-details adoption only; zero all updates.\n";
}

#if COLOURWARS_DEV_TOOLS
void crack_tune_colour_checks(Application& app,Registry<LevelLoadedInfo>& levels,GameTuning& tuning,Presentation& presentation,
    const std::filesystem::path& content,const std::filesystem::path& output)
{
    const auto root=output/"crack-colour-tune-scratch",source=root/"source",staged=root/"staged";
    std::filesystem::create_directories(source);std::filesystem::create_directories(staged);
    constexpr std::array<const char*,2> files{"tuning.json","presentation.json"};std::array<std::string,2> real_bytes;
    for(size_t i=0;i<files.size();++i)
    {real_bytes[i]=tune_colour_bytes(content/files[i]);tune_colour_write(source/files[i],real_bytes[i]);tune_colour_write(staged/files[i],real_bytes[i]);}
    const NestedSaveProfile profile(root/"profile");const TuneColourTemp temporary(root/"temp");
    struct RestoreCrack
    {GameTuning& t;Presentation& p;GameTuning old_t;Presentation old_p;~RestoreCrack(){t=old_t;p=old_p;}}
        restore{tuning,presentation,tuning,presentation};
    Save preferences;PlaySettings play;play.music_volume=17;play.sfx_volume=83;play.aim_sensitivity=4;
    play.view_shake=false;play.aim_marker=true;play.reduce_effects=true;play.sticks[2].movement_deadzone=25;play.sticks[2].aim_deadzone=70;
    InputBindings bindings;bindings.keyboard_jump=Key::j;
    require(preferences.set_play_settings_and_save(play)&&preferences.set_input_bindings_and_save(bindings),"Cannot seed Crack private preferences");
    const auto save_path=root/"profile/ColourWars/save_data.json";const std::string saved_preferences=tune_colour_bytes(save_path);
    const auto pref_values=[&]
    {
        const auto& p=preferences.play_settings();std::vector<int> v{static_cast<int>(preferences.resolution()),preferences.fullscreen()?1:0,
            static_cast<int>(preferences.team_palette()),p.aim_sensitivity,p.music_volume,p.sfx_volume,p.view_shake?1:0,p.aim_marker?1:0,
            p.reduce_effects?1:0,static_cast<int>(preferences.input_bindings().keyboard_jump)};
        for(const auto& stick:p.sticks){v.push_back(stick.movement_deadzone);v.push_back(stick.aim_deadzone);}return v;
    };
    const auto saved_values=pref_values();StageList stages;
    for(const char* name:{"movement_course","close_quarters","king_of_the_hill","turbulence"})
    {const auto* info=levels.get(name);stages.push_back({name,info->display_name(),info->icon_frame(),info->include_in_random()});}
    GameData data;data.set_application(&app);data.set_save(&preferences);const StageCatalog stage_catalog = acceptance::catalog_of(levels, stages, { "close_quarters", "king_of_the_hill", "movement_course", "turbulence" }); data.set_stage_catalog(&stage_catalog);
    data.set_tuning(&tuning);data.set_presentation(&presentation);DebugDirector director(&tuning,&presentation);data.set_debug(&director);
    auto& impl=*director.impl();impl.set_content_directories(source,staged);impl.set_user_directory(root/"user");
    impl.set_script_directories({root/"scripts"});director.attach(&app,&data);impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output/"crack-colour-tune-commands.log").string()),"Cannot retain Crack private command transcript");
    CrackColourProbe probe(app,presentation,output);JsonSpanDocument initial(real_bytes[1],"Crack private source");
    const JsonLeaf* leaf=initial.find("opening.crack");require(leaf!=nullptr,"Crack source lacks exact existing token");
    require(presentation.opening().crack==Colour(58,58,58),"Crack Debug baseline differs from authored default");
    const std::string expected_saved=initial.replaced(*leaf,"\"2060C080\"");bool persisted=false;size_t commands=0,rows=0,named_checks=0;
    const auto unchanged=[&]
    {
        probe.unchanged();require(!impl.can_apply()&&impl.request().empty()&&!impl.in_match()&&
            tune_colour_bytes(save_path)==saved_preferences&&pref_values()==saved_values,
            "Crack command changed private prefs/default display/JumpJ or queued work");
        for(size_t i=0;i<files.size();++i)
        {
            const auto& expected=i==1&&persisted?expected_saved:real_bytes[i];
            require(tune_colour_bytes(content/files[i])==real_bytes[i]&&tune_colour_bytes(source/files[i])==expected&&tune_colour_bytes(staged/files[i])==expected,
                "Crack command changed actual content/private source/staged bytes beyond exact one token");
        }
    };
    const auto command=[&](const std::string& line)
    {
        const size_t before=impl.log().lines().size();std::srand(73401);const int next=std::rand();std::srand(73401);
        const auto status=impl.execute(line);++commands;require(status.ok&&std::rand()==next,"Actual Crack command failed or consumed RNG");
        if(line.starts_with("tune "))
        {const auto& log=impl.log().lines();require(log.size()>before&&log.back().severity==DebugSeverity::info&&
            log.back().text.starts_with("tune: presentation.opening.crack")&&log.back().text.ends_with("(live)"),"Crack live command reply differs");}
    };
    Frame initial_raster{},saved_raster{};std::ofstream records(output/"crack-colour-tune.csv");
    records<<"state,context,background,roi_rgba_match,adoption_match,repeat_match,literal_channels,effective_delta,frozen_inputs_resources_rng,calibration_rgba,source_models,private_bytes,adoption,named_token_preserved\n";
    const auto check=[&](const char* state,const Colour& expected)
    {
        unchanged();require(presentation.opening().crack==expected,"Crack command parsed wrong literal channels");
        Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual(0);adoption=probe.adoption(0,expected);raw=probe.raw(0,expected);
            require(actual.pixels==adoption.pixels,"Crack copied complete front-details adoption differs after actual command");
            CrackColourProbe::check_roi(0,actual,raw,expected);
            if(std::string_view(state)=="baseline"){initial_raster=actual;}
            CrackColourProbe::delta_only_effective(0,actual,initial_raster);
            if(std::string_view(state)=="reset-original")
            {require(actual.pixels==initial_raster.pixels,"Crack original reset failed exact complete-frontdetails RGBA restoration");}
            if(std::string_view(state)=="rgba128"){saved_raster=actual;}
            if(std::string_view(state)=="reset-saved"||std::string_view(state)=="reset-all-saved")
            {require(actual.pixels==saved_raster.pixels,"Crack saved reset failed exact complete-frontdetails RGBA restoration");}
            if(std::string_view(state)=="named")
            {
                const auto* named_document=impl.tune_document("presentation");
                require(named_document!=nullptr&&named_document->edits().size()==1&&named_document->edits().contains("opening.crack")&&
                    named_document->edits().at("opening.crack").current_token=="\"RED\"","Crack named token was normalized instead of preserving quoted RED");++named_checks;
            }
            unchanged();records<<state<<",left,canvas32-64-96,true,true,true,true,true,true,true,F_AF|Q_AF|F_AQ|Q_AQ,true,live,"
                <<(std::string_view(state)=="named"?"true":"not-applicable")<<"\n";++rows;
        }
        catch(...)
        {
            if(!actual.pixels.empty()){probe.failure(0,state,"debug-case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){probe.failure(0,state,"debug-adoption",adoption);}
            throw;
        }
    };
    check("baseline",Colour(58,58,58));command("tune presentation.opening.crack RED");check("named",Colour(255,0,0));
    command("tune.reset presentation.opening.crack");check("reset-original",Colour(58,58,58));
    command("tune presentation.opening.crack 35BCE1");check("rgb",Colour(53,188,225));
    command("tune presentation.opening.crack 2060C080");check("rgba128",Colour(32,96,192,128));
    const auto* document=impl.tune_document("presentation");require(document!=nullptr&&document->edits().size()==1&&document->edits().contains("opening.crack")&&
        document->edits().at("opening.crack").current_token=="\"2060C080\"",
        "Crack Save ledger differs from one exact token");
    command("tune.save presentation");persisted=true;unchanged();require(impl.tune_document("presentation")->edits().empty(),"Crack Save failed rebase");
    command("tune presentation.opening.crack A532D600");check("alpha0",Colour(165,50,214,0));
    command("tune.reset presentation.opening.crack");check("reset-saved",Colour(32,96,192,128));
    command("tune presentation.opening.crack 35BCE1");check("rgb-after-save",Colour(53,188,225));
    command("tune.reset presentation");check("reset-all-saved",Colour(32,96,192,128));
    require(impl.tune_document("presentation")->edits().empty()&&impl.tune_document("tuning")->edits().empty(),"Crack final reset left document edits");
    require(records.good()&&commands==9&&rows==9&&named_checks==1&&probe.readbacks()==36&&probe.calibration_checks()==1152,"Crack Debug exact command/row/named-token/calibration/read budget differs");
    std::ofstream summary(output/"crack-colour-tune-summary.json");summary<<"{\"scope\":\"debug-validated-commands\",\"event_fade\":false,\"context\":\"left\",\"source_models\":\"F_AF|Q_AF|F_AQ|Q_AQ\",\"commands\":9,\"states\":9,\"contexts\":1,\"readbacks\":36,\"csv_rows\":9,\"crops\":0,\"saved_tokens\":1,\"private_seed_saves\":2,\"named_token_checks\":1,\"calibration_pixels_per_read\":32,\"calibration_pixel_checks\":1152,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0}\n";
    require(summary.good(),"Cannot retain Crack Debug summary");
    std::cout<<"PASS crack tune:9real commands/9left states/36additional readbacks/9rows; one exact quoted Crack token Save/rebase and original/saved complete-frontdetails resets,"
        "1152WHITE-BLACK calibration checks/quotedRED preserved/2private preference seeds/JumpJ/defaultdisplay; zero all updates; no actualLevel/human/performance claim.\n";
}
#endif
