// Track41: two independent192x16 track interiors; copied full HUD adoption only.
// Literal geometry/channels remain source hypotheses until native qualification.
enum class HudTrackReference { correct, straight, squared_base };
const char* hud_track_context(size_t context)
{ constexpr std::array<const char*,3> names{"empty","half","full"};return names.at(context); }
float hud_track_amount(size_t context)
{ constexpr std::array<float,3> amounts{0.0f,.5f,1.0f};return amounts.at(context); }
int hud_track_fill_width(size_t context)
{ constexpr std::array<int,3> widths{0,96,192};return widths.at(context); }
std::array<std::string,5> hud_track_resource_names(const HudPresentation& p)
{ return {p.meter_label_font,p.sprite_sheet,p.fill_frame,p.timer_font,p.player_identity_font}; }
std::vector<float> hud_track_fixed_style(const HudPresentation& p)
{
    std::vector<float> v{p.paper.keyline_width,p.paper.shadow_offset,p.meter_label_gap,p.reference_height,p.right_margin,
        p.ammo_top_margin,p.health_top_margin,p.box_fill_border,p.timer_reference_width,p.closing_clock.keyline_width,
        p.closing_clock.shadow_offset,p.closing_clock.tick_reach,p.closing_clock.beat_duration,p.player_identity_reference_width};
    for(const auto& point:{p.box_size,p.timer_offset,p.timer_shadow_offset,p.player_identity_offset,p.player_identity_shadow_offset,p.player_identity_padding})
    {v.insert(v.end(),{point.x,point.y});}
    for(const auto& c:{p.paper.ink,p.paper.paper,p.paper.shade,p.health_colour,p.timer_colour,p.timer_shadow_colour,
        p.closing_clock.ink,p.closing_clock.paper,p.closing_clock.shade,p.closing_clock.text,p.closing_clock.accent,
        p.player_identity_colour,p.player_identity_shadow_colour,p.player_identity_background_colour})
    {v.insert(v.end(),{c.r,c.g,c.b,c.a});}
    return v; // The sole omitted field is selected meter_track_colour; health_colour is frozen.
}
std::vector<float> hud_track_identity_style(const PlayerIdentityPresentation& p)
{
    std::vector<float> v{p.enabled?1.0f:0.0f,p.world_size.x,p.world_size.y,p.world_offset.x,p.world_offset.y,
        p.emphasis_duration,p.emphasis_scale,p.ui_size.x,p.ui_size.y,p.ui_gap,p.border};
    for(const auto& c:{p.background_colour,p.ink_colour}){v.insert(v.end(),{c.r,c.g,c.b,c.a});}return v;
}
bool hud_track_same_palette(const TeamPalettePresentation& a,const TeamPalettePresentation& b)
{
    if(a.colours.size()!=b.colours.size()||a.pairs.size()!=b.pairs.size()){return false;}
    for(size_t i=0;i<a.colours.size();++i)
    {if(a.colours[i].name!=b.colours[i].name||a.colours[i].colour!=b.colours[i].colour){return false;}}
    for(size_t i=0;i<a.pairs.size();++i)
    {if(a.pairs[i].key!=b.pairs[i].key||a.pairs[i].label!=b.pairs[i].label||
        a.pairs[i].colours.team_a!=b.pairs[i].colours.team_a||a.pairs[i].colours.team_b!=b.pairs[i].colours.team_b){return false;}}
    return true;
}
class HudTrackColourProbe
{
public:
    HudTrackColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output);
    void unchanged() const;
    Frame actual(size_t context);
    Frame adoption(size_t context,const Colour& colour);
    Frame raw(size_t context,const Colour& colour,HudTrackReference mode=HudTrackReference::correct);
    static std::string check_rois(size_t context,const Frame& actual,const Frame& raw,const Colour& colour);
    static void delta_suffix(size_t context,const Frame& actual,const Frame& baseline);
    static void fill_roles(size_t context,const Frame& actual,const Frame& baseline);
    static void check_control(size_t context,const Frame& actual,const Frame& correct_raw,const Frame& wrong,size_t& changed,size_t& unchanged_pixels,size_t& discriminations);
    static Frame compact(const Frame& source);
    void failure(size_t context,const char* state,const char* stage,const Frame& actual,const Frame* other=nullptr) const;
    size_t readbacks() const { return readbacks_; }
private:
    static size_t offset(const Frame& frame,int x,int y)
    { return (static_cast<size_t>(y)*static_cast<size_t>(frame.width)+static_cast<size_t>(x))*4; }
    Frame render(size_t context,const std::function<Frame()>& draw);
    void write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint);
    Application& app_;const Presentation& presentation_;const HudPresentation& look_;const PlayerIdentityPresentation& identity_;
    const TeamPalettePresentation& palette_;HudPresentation baseline_,expected_;PlayerIdentityPresentation expected_identity_;
    TeamPalettePresentation baseline_palette_;InterfaceGameplay hud_,reference_;Canvas canvas_;const TeamColour colours_;
    RenderResources::SpriteSheetHandle sheet_handle_;const SpriteSheet* sheet_;SpriteSheet::frame_handle fill_handle_;const SpriteFrame& frame_;
    std::array<FontHandle,3> fonts_;std::array<const Font*,3> font_pointers_{};std::array<Vector2F,5> metrics_{};
    const std::filesystem::path output_;std::vector<float> fixed_,fixed_identity_;const std::array<std::string,5> names_;
    const Vector2F resolution_{1280,720};const std::wstring device_=input_slots::device_name(input_slots::keyboard_and_mouse);
    size_t readbacks_=0;
};

HudTrackColourProbe::HudTrackColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output) :
    app_(app),presentation_(presentation),look_(presentation.hud()),identity_(presentation.player_identity()),palette_(presentation.team_palette()),
    baseline_(look_),expected_(baseline_),expected_identity_(identity_),baseline_palette_(palette_),
    hud_(app.render_resources(),&look_,&identity_),reference_(app.render_resources(),&expected_,&expected_identity_),
    canvas_(app.render_resources(),Colour(32,64,96)),colours_(TeamColourTools::team_colours(TeamColourPair::blue_orange,palette_)),
    sheet_handle_(app.render_resources()->resolve_sprite_sheet(look_.sprite_sheet)),sheet_(app.render_resources()->sprite_sheet(sheet_handle_)),
    fill_handle_(sheet_->resolve_sprite_frame(look_.fill_frame)),frame_(sheet_->sprite_frame(fill_handle_)),
    fonts_{app.render_resources()->resolve_sprite_font(look_.meter_label_font),app.render_resources()->resolve_sprite_font(look_.timer_font),
        app.render_resources()->resolve_sprite_font(look_.player_identity_font)},output_(output),fixed_(hud_track_fixed_style(look_)),
    fixed_identity_(hud_track_identity_style(identity_)),names_(hud_track_resource_names(look_))
{
    require(look_.sprite_sheet=="sprite_sheet_1"&&look_.fill_frame=="pixel"&&frame_.source_rectangle()==RectangleI(32,0,4,4)&&frame_.origin()==Vector2F::ZERO&&
        look_.health_colour==Colour(118,232,181)&&look_.reference_height==1080&&look_.player_identity_reference_width==1920&&
        look_.right_margin==.19f&&look_.health_top_margin==.09f&&look_.ammo_top_margin==.17f&&look_.box_size==Vector2F(.15f,.022f)&&
        look_.paper.keyline_width==2.0f&&look_.paper.ink==Colour(20,20,20)&&colours_.team_a==Colour(6,52,204)&&colours_.team_b==Colour(255,127,0),
        "Track fixture lost source-derived track/prefix, white frame, actual track or fixed caller palette assumptions");
    for(size_t i=0;i<fonts_.size();++i){font_pointers_[i]=app.render_resources()->font(fonts_[i]);}
    auto* resources=app.render_resources();metrics_[0]=resources->measure_text(fonts_[0],L"HEALTH");metrics_[1]=resources->measure_text(fonts_[0],L"PAINT");
    metrics_[2]=resources->measure_text(fonts_[1],L"2:04");metrics_[3]=resources->measure_text(fonts_[2],L"P1  /  TEAM A\n"+device_);
    metrics_[4]=resources->measure_text(fonts_[2],PlayerBadge::label(0));unchanged();
}
void HudTrackColourProbe::unchanged() const
{
    auto* resources=app_.render_resources();
    require(&look_==&presentation_.hud()&& &look_.paper==&presentation_.hud().paper&& &look_.closing_clock==&presentation_.hud().closing_clock&&
        &identity_==&presentation_.player_identity()&& &palette_==&presentation_.team_palette()&&
        hud_track_fixed_style(look_)==fixed_&&hud_track_fixed_style(expected_)==fixed_&&
        hud_track_resource_names(look_)==names_&&hud_track_resource_names(expected_)==names_&&
        hud_track_identity_style(identity_)==fixed_identity_&&hud_track_identity_style(expected_identity_)==fixed_identity_&&
        hud_track_same_palette(palette_,baseline_palette_)&&colours_.team_a==Colour(6,52,204)&&colours_.team_b==Colour(255,127,0)&&
        sheet_handle_==resources->resolve_sprite_sheet(look_.sprite_sheet)&&sheet_==resources->sprite_sheet(sheet_handle_)&&
        fill_handle_==sheet_->resolve_sprite_frame(look_.fill_frame)&& &frame_==&sheet_->sprite_frame(fill_handle_)&&
        resolution_==Vector2F(1280,720)&&device_==input_slots::device_name(input_slots::keyboard_and_mouse)&&
        app_.viewport_manager()->player_viewport(0).pixel_rect()==RectangleI(0,0,1280,720),
        "Track edit/draw changed borrowed HUD/identity/palette records, complete unselected style, resources, geometry or caller arguments");
    const std::array<std::string,3> names{look_.meter_label_font,look_.timer_font,look_.player_identity_font};
    for(size_t i=0;i<fonts_.size();++i)
    {require(fonts_[i]==resources->resolve_sprite_font(names[i])&&font_pointers_[i]==resources->font(fonts_[i]),"Track drawing changed captured font handles/resources");}
    require(metrics_[0]==resources->measure_text(fonts_[0],L"HEALTH")&&metrics_[1]==resources->measure_text(fonts_[0],L"PAINT")&&
        metrics_[2]==resources->measure_text(fonts_[1],L"2:04")&&metrics_[3]==resources->measure_text(fonts_[2],L"P1  /  TEAM A\n"+device_)&&
        metrics_[4]==resources->measure_text(fonts_[2],PlayerBadge::label(0)),"Track redraw changed captured font measurements");
}
Frame HudTrackColourProbe::render(size_t context,const std::function<Frame()>& draw)
{
    unchanged();const Colour selected=look_.meter_track_colour,expected_selected=expected_.meter_track_colour;
    std::srand(73401);const int next=std::rand();std::srand(73401);Frame result=draw();++readbacks_;
    if(result.width!=1280||result.height!=720){failure(context,"unknown","backbuffer-size",result);throw std::runtime_error("Track proof requires1280x720 backbuffer");}
    const bool fixed_draw=std::rand()==next&&look_.meter_track_colour==selected&&expected_.meter_track_colour==expected_selected;
    if(!fixed_draw){failure(context,"unknown","draw-state-or-rng",result);}require(fixed_draw,"Track draw consumed RNG or changed selected records");
    unchanged();return result;
}
Frame HudTrackColourProbe::actual(size_t context)
{
    const auto draw=[&]{return hud_fixture(app_,canvas_,hud_,1,colours_,hud_track_amount(context),hud_track_amount(context),false,-1);};
    Frame first=render(context,draw);const Frame repeat=render(context,draw);
    if(first.pixels!=repeat.pixels){failure(context,"unknown","repeat",first,&repeat);}
    require(first.pixels==repeat.pixels,"Frozen paired-track HUD actual repeat changed RGBA");return first;
}
Frame HudTrackColourProbe::adoption(size_t context,const Colour& colour)
{
    expected_.meter_track_colour=colour;
    return render(context,[&]{return hud_fixture(app_,canvas_,reference_,1,colours_,hud_track_amount(context),hud_track_amount(context),false,-1);});
}
void HudTrackColourProbe::write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint)
{
    list.draw_sprite(sheet_->texture(),frame_.source_rectangle(),RectangleF(static_cast<float>(left),static_cast<float>(top),
        static_cast<float>(right-left),static_cast<float>(bottom-top)),tint,0.0f,frame_.origin(),SpriteFlip::none,0.0f);
}

Frame HudTrackColourProbe::raw(size_t context,const Colour& colour,HudTrackReference mode)
{
    const Frame result=render(context,[&]
    {
        Renderer& renderer=*app_.renderer();renderer.begin_frame();renderer.set_view_count(1);DrawList list=renderer.view(0);
        list.set_viewport(app_.viewport_manager()->fullscreen_viewport());list.set_camera(Camera::DEFAULT_CAMERA);list.set_filter(TextureFilter::point);
        canvas_.draw(list,RectangleF(0,0,1280,720));write_raw(list,20,20,24,24,Colour(255,255,255));write_raw(list,25,20,29,24,Colour(0,0,0));
        constexpr std::array<int,2> tops{64,122};
        const float factor=mode==HudTrackReference::straight?1.0f:mode==HudTrackReference::squared_base?colour.a*colour.a:colour.a;
        const Colour selected(colour.r*factor,colour.g*factor,colour.b*factor,colour.a);
        for(size_t track=0;track<tops.size();++track)
        {
            const int top=tops[track];
            write_raw(list,1035,top-1,1230,top+17,Colour(20,20,20)); // Full opaque ink, not copied panel art.
            write_raw(list,1036,top,1228,top+16,selected); // Exactly one selected write per interior pixel.
            if(hud_track_fill_width(context)>0)
            {write_raw(list,1036,top,1036+hud_track_fill_width(context),top+16,track==0?Colour(118,232,181):Colour(6,52,204));}
        }
        return read(renderer);
    });
    const bool calibrated=monitor_rgb(result,21,21)==0xFFFFFFu&&monitor_rgb(result,26,21)==0&&
        result.pixels[offset(result,21,21)+3]==255&&result.pixels[offset(result,26,21)+3]==255;
    if(!calibrated){failure(context,"unknown","raw-calibration",result);}
    require(calibrated,"Track raw WHITE/BLACK frame RGBA calibration differs");return result;
}
void HudTrackColourProbe::failure(size_t context,const char* state,const char* stage,const Frame& actual,const Frame* other) const
{
    const std::string stem=std::string("hud-track-failure-")+hud_track_context(context)+"-"+state+"-"+stage;
    save(actual,output_/(stem+"-actual.ppm"));if(other!=nullptr){save(*other,output_/(stem+"-reference.ppm"));}
    const auto rgba=[&](const Frame& frame,const char* suffix)
    {std::ofstream bytes(output_/(stem+suffix),std::ios::binary);bytes.write(reinterpret_cast<const char*>(frame.pixels.data()),static_cast<std::streamsize>(frame.pixels.size()));
        bytes.close();return !bytes.fail();};
    const bool actual_written=rgba(actual,"-actual.rgba"),other_written=other!=nullptr&&rgba(*other,"-reference.rgba");
    std::ofstream receipt(output_/(stem+".json"));receipt<<"{\"context\":\""<<hud_track_context(context)<<"\",\"state\":\""<<state<<"\",\"stage\":\""<<stage
        <<"\",\"readbacks\":"<<readbacks_<<",\"health_roi\":[1036,64,1228,80],\"ammo_roi\":[1036,122,1228,138],\"fill_width\":"<<hud_track_fill_width(context)
        <<",\"frame_width\":"<<actual.width<<",\"frame_height\":"<<actual.height<<",\"rgba_order\":\"RGBA8-row-major\",\"actual_rgba_bytes\":"<<actual.pixels.size()
        <<",\"actual_rgba_written\":"<<(actual_written?"true":"false")<<",\"reference_rgba_written\":"<<(other_written?"true":"false")
        <<",\"diagnostic\":\"preserve-whole-composite-source-models-and-geometry-no-capture-fitting\"}\n";
}
std::string HudTrackColourProbe::check_rois(size_t context,const Frame& actual,const Frame& raw,const Colour& colour)
{
    constexpr std::array<int,2> tops{64,122};
    // Complete BOTH disjoint interiors: all6144 exact RGBA/alpha comparisons
    // precede any supplementary representative source-channel admission.
    for(const int top:tops){for(int y=top;y<top+16;++y){for(int x=1036;x<1228;++x)
    {
        const size_t at=offset(actual,x,y);require(same_pixel(actual,raw,at),"Track independent ordered ink/track/prefix differs from actual RGBA");
        require(actual.pixels[at+3]==255,"Track interior lost opaque ink alpha");
    }}}
    std::uint32_t f=0x141414u,q=f;
    if(colour.a==1.0f)
    {
        f=(static_cast<std::uint32_t>(std::round(colour.r*255))<<16)|(static_cast<std::uint32_t>(std::round(colour.g*255))<<8)|
            static_cast<std::uint32_t>(std::round(colour.b*255));q=f;
    }
    else if(colour.a!=0.0f)
    {
        require(colour==Colour(1,97,193,128),"Track source channels cover only declared partial palette");
        f=0x0A3B6Bu;q=0x0B3B6Bu; // Predicted before captures; original alpha128 is already8UNORM.
    }
    bool matches_f=true,matches_q=true;const int right=1036+hud_track_fill_width(context);
    for(size_t track=0;track<tops.size();++track){const int top=tops[track];
        const std::uint32_t fill=track==0?0x76E8B5u:0x0634CCu;
        for(int y=top;y<top+16;++y){for(int x=1036;x<1228;++x)
        {
            const auto rgb=monitor_rgb(actual,x,y);matches_f=matches_f&&(rgb==(x<right?fill:f));matches_q=matches_q&&(rgb==(x<right?fill:q));
        }}
    }
    // One coherent representative across ALL pixels of BOTH ROIs. No per-pixel
    // or per-track mixture/tolerance; this does not identify GPU internals.
    require(matches_f||matches_q,"Track whole paired ROI stack matches no frozen representative source model");
    if(matches_f&&matches_q){return "F_AF|Q_AF|F_AQ|Q_AQ";}
    return matches_f?"F_AF|F_AQ":"Q_AF|Q_AQ";
}
void HudTrackColourProbe::delta_suffix(size_t context,const Frame& actual,const Frame& baseline)
{
    const int left=1036+hud_track_fill_width(context);
    for(int y=0;y<actual.height;++y){for(int x=0;x<actual.width;++x)
    {
        if(same_pixel(actual,baseline,offset(actual,x,y))){continue;}
        require(x>=left&&x<1228&&((y>=64&&y<80)||(y>=122&&y<138)),"Track palette delta escaped its two visible suffixes or changed full cover");
    }}
}
void HudTrackColourProbe::fill_roles(size_t context,const Frame& actual,const Frame& baseline)
{
    constexpr std::array<int,2> tops{64,122};const int right=1036+hud_track_fill_width(context);
    for(size_t track=0;track<tops.size();++track){const int top=tops[track];
        for(int y=top;y<top+16;++y){for(int x=1036;x<right;++x)
        {
            const size_t at=offset(actual,x,y);require(same_pixel(actual,baseline,at),"Track edit changed fixed health/team-ammo prefix bytes");
            require(monitor_rgb(actual,x,y)==(track==0?0x76E8B5u:0x0634CCu)&&actual.pixels[at+3]==255,"Track changed literal opaque health/team fill role");
        }}
    }
}
void HudTrackColourProbe::check_control(size_t context,const Frame& actual,const Frame& correct_raw,const Frame& wrong,
    size_t& changed,size_t& unchanged_pixels,size_t& discriminations)
{
    const int width=hud_track_fill_width(context);require(width<192,"Fully covered track cannot discriminate selected material");
    constexpr std::array<int,2> tops{64,122};
    for(const int top:tops)
    {
        const size_t before_changed=changed,before_unchanged=unchanged_pixels;
        for(int y=top;y<top+16;++y){for(int x=1036;x<1228;++x)
        {
            const size_t at=offset(actual,x,y);require(wrong.pixels[at+3]==255,"Track wrong-RGB reference changed opaque ink alpha");
            if(x>=1036+width)
            {require(monitor_rgb(actual,x,y)!=monitor_rgb(wrong,x,y),"Track wrong reference did not discriminate every visible pixel of this track");++changed;}
            else{require(same_pixel(actual,wrong,at),"Track wrong reference changed a fixed prefix");++unchanged_pixels;}
        }}
        require(changed-before_changed==static_cast<size_t>((192-width)*16)&&unchanged_pixels-before_unchanged==static_cast<size_t>(width*16),
            "Track per-track negative changed/complement counts differ");++discriminations;
    }
    for(int y=0;y<wrong.height;++y){for(int x=0;x<wrong.width;++x)
    {
        if(x>=1036+width&&x<1228&&((y>=64&&y<80)||(y>=122&&y<138))){continue;}
        require(same_pixel(correct_raw,wrong,offset(wrong,x,y)),"Track wrong reference leaked outside owned suffixes of the correct raw frame");
    }}
}
Frame HudTrackColourProbe::compact(const Frame& source)
{
    require(source.width==1280&&source.height==720,"Track paired stack requires the declared native source frame");
    Frame stack{192,32,{}};stack.pixels.reserve(192*32*4);
    for(const int top:{64,122}){for(int y=top;y<top+16;++y)
    {
        const size_t start=offset(source,1036,y);stack.pixels.insert(stack.pixels.end(),source.pixels.begin()+start,source.pixels.begin()+start+192*4);
    }}
    require(stack.pixels.size()==192*32*4,"Track paired ROI stack lost native pixels");return stack;
}

void hud_track_colour_consumer_checks(Application& app,const Presentation& presentation,const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);app.resolution_manager()->set_resolution_exactly({1280,720});
    std::ignore=app.renderer()->window_size_changed(1280,720);Presentation local=presentation;
    auto& look=const_cast<HudPresentation&>(local.hud());require(look.meter_track_colour==Colour(184,184,184),"Track authored default changed");
    HudTrackColourProbe probe(app,local,output);
    const std::array<Colour,4> colours{Colour(184,184,184),Colour(53,188,225),Colour(1,97,193,128),Colour(165,50,214,0)};
    constexpr std::array<const char*,4> states{"default","rgb","rgba128","alpha0"};
    std::array<Frame,3> baseline{};size_t rows=0,crops=0,controls=0,discriminations=0,changed=0,unchanged_pixels=0,full_cases=0,role_cases=0;
    std::ofstream records(output/"hud-track-colour-consumers.csv");
    records<<"state,context,background,fill_width,roi_rgba_match,adoption_match,repeat_match,literal_channels,delta_suffix,full_cover_invariant,fill_roles_unchanged,frozen_inputs_resources_rng,source_models,controls,track_discriminations,changed_control_pixels,unchanged_control_pixels,scope\n";
    for(size_t c=0;c<3;++c){for(size_t p=0;p<colours.size();++p)
    {
        look.meter_track_colour=colours[p];Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual(c);adoption=probe.adoption(c,colours[p]);raw=probe.raw(c,colours[p]);
            require(actual.pixels==adoption.pixels,"Track copied full-HUD consumer adoption differs");
            const std::string models=HudTrackColourProbe::check_rois(c,actual,raw,colours[p]);
            if(p==0){baseline[c]=actual;}
            HudTrackColourProbe::delta_suffix(c,actual,baseline[c]);HudTrackColourProbe::fill_roles(c,actual,baseline[c]);++role_cases;
            if(c==2){require(actual.pixels==baseline[2].pixels,"Fully covered tracks changed complete HUD across palettes");++full_cases;}
            else if(p>0)
            {
                for(const int top:{64,122})
                {
                    size_t delta=0;for(int y=top;y<top+16;++y){for(int x=1036+hud_track_fill_width(c);x<1228;++x)
                    {if(monitor_rgb(actual,x,y)!=monitor_rgb(baseline[c],x,y)){++delta;}}}
                    require(delta>0,"Track edit produced no visible delta in one of the two track suffixes");
                }
            }
            size_t case_controls=0,case_changed=0,case_unchanged=0,case_discriminations=0;
            const auto control=[&](HudTrackReference mode,const char* label)
            {
                const Frame wrong=probe.raw(c,colours[p],mode);
                try{HudTrackColourProbe::check_control(c,actual,raw,wrong,case_changed,case_unchanged,case_discriminations);}
                catch(...){probe.failure(c,states[p],label,actual,&wrong);throw;}
                ++case_controls;
            };
            if(c<2&&p==2){control(HudTrackReference::straight,"partial-straight");control(HudTrackReference::squared_base,"partial-squared");}
            else if(c<2&&p==3){control(HudTrackReference::straight,"alpha0-straight");}
            records<<states[p]<<','<<hud_track_context(c)<<",canvas32-64-96,"<<hud_track_fill_width(c)
                <<",true,true,true,true,true,"<<(c==2?"true":"not-applicable")<<",true,true,"<<models<<','<<case_controls<<','<<case_discriminations
                <<','<<case_changed<<','<<case_unchanged<<",consumer-only\n";
            ++rows;controls+=case_controls;discriminations+=case_discriminations;changed+=case_changed;unchanged_pixels+=case_unchanged;
            save(HudTrackColourProbe::compact(actual),output/(std::string("hud-track-colour-")+hud_track_context(c)+"-"+states[p]+".ppm"));++crops;
        }
        catch(...)
        {
            if(!actual.pixels.empty()){probe.failure(c,states[p],"case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){probe.failure(c,states[p],"adoption",adoption);}
            throw;
        }
    }}
    require(records.good()&&probe.readbacks()==54&&rows==12&&crops==12&&controls==6&&discriminations==12&&changed==27648&&unchanged_pixels==9216&&full_cases==4&&role_cases==12,
        "Track shared exact readback/row/control/mask/full-cover/role/paired-stack budget differs");
    std::ofstream summary(output/"hud-track-colour-consumer-summary.json");
    summary<<"{\"scope\":\"consumer-only\",\"viewport\":\"1280x720\",\"background\":\"canvas32-64-96\",\"event_fade\":false,\"raw_reference\":\"two-independent192x16-ink-track-prefix-interiors-only\",\"whole_frame_reference\":\"copied-complete-HUD-consumer-adoption-only\",\"health_roi_half_open\":[1036,64,1228,80],\"ammo_roi_half_open\":[1036,122,1228,138],\"capture_kind\":\"lossless-paired-ROI-stack\",\"paired_amounts_only\":true,\"readbacks\":54,\"csv_rows\":12,\"crops\":12,\"crop_width\":192,\"crop_height\":32,\"crop_pixels\":73728,\"ppm_bytes\":221352,\"contexts\":3,\"palettes\":4,\"roi_pixels\":6144,\"source_rois\":2,\"base_case_reads\":48,\"blank_reads\":0,\"selected_track_writes\":2,\"maximum_selected_writes_per_pixel\":1,\"controls\":6,\"track_discriminations\":12,\"partial_straight_reads\":2,\"partial_squared_reads\":2,\"alpha0_straight_reads\":2,\"changed_control_pixels\":27648,\"unchanged_control_pixels\":9216,\"full_cover_invariant_cases\":4,\"fill_role_guard_cases\":12,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0}\n";
    require(summary.good(),"Cannot retain Track shared summary");
    std::cout<<"PASS track consumer:54readbacks/12rows/12lossless paired192x32 ROI stacks73728nativepixels;6144independentRGBA/case,"
        "6controls/12track discriminations27648changed9216unchanged; coherent whole-composite source representatives. Full copiedHUD adoption only; zero all updates.\n";
}

#if COLOURWARS_DEV_TOOLS
void hud_track_tune_colour_checks(Application& app,Registry<LevelLoadedInfo>& levels,GameTuning& tuning,Presentation& presentation,
    const std::filesystem::path& content,const std::filesystem::path& output)
{
    const auto root=output/"hud-track-colour-tune-scratch",source=root/"source",staged=root/"staged";
    std::filesystem::create_directories(source);std::filesystem::create_directories(staged);
    constexpr std::array<const char*,2> files{"tuning.json","presentation.json"};std::array<std::string,2> real_bytes;
    for(size_t i=0;i<files.size();++i)
    {real_bytes[i]=tune_colour_bytes(content/files[i]);tune_colour_write(source/files[i],real_bytes[i]);tune_colour_write(staged/files[i],real_bytes[i]);}
    const NestedSaveProfile profile(root/"profile");const TuneColourTemp temporary(root/"temp");
    struct RestoreHudTrack
    {GameTuning& t;Presentation& p;GameTuning old_t;Presentation old_p;~RestoreHudTrack(){t=old_t;p=old_p;}}
        restore{tuning,presentation,tuning,presentation};
    Save preferences;PlaySettings play;play.music_volume=17;play.sfx_volume=83;play.aim_sensitivity=4;
    play.view_shake=false;play.aim_marker=true;play.reduce_effects=true;play.sticks[2].movement_deadzone=25;play.sticks[2].aim_deadzone=70;
    InputBindings bindings;bindings.keyboard_jump=Key::j;
    require(preferences.set_play_settings_and_save(play)&&preferences.set_input_bindings_and_save(bindings),"Cannot seed Track private preferences");
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
    GameData data;data.set_application(&app);data.set_save(&preferences);data.set_level_infos(&levels);data.set_stages(&stages);
    data.set_tuning(&tuning);data.set_presentation(&presentation);DebugDirector director(&tuning,&presentation);data.set_debug(&director);
    auto& impl=*director.impl();impl.set_content_directories(source,staged);impl.set_user_directory(root/"user");
    impl.set_script_directories({root/"scripts"});director.attach(&app,&data);impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output/"hud-track-colour-tune-commands.log").string()),"Cannot retain Track private command transcript");
    HudTrackColourProbe probe(app,presentation,output);JsonSpanDocument initial(real_bytes[1],"Track private source");
    const JsonLeaf* leaf=initial.find("hud.meter_track_colour");require(leaf!=nullptr,"Track source lacks exact existing token");
    require(presentation.hud().meter_track_colour==Colour(184,184,184),"Track Debug baseline differs from authored default");
    const std::string expected_saved=initial.replaced(*leaf,"\"0161C180\"");bool persisted=false;size_t commands=0,rows=0,role_cases=0;
    const auto unchanged=[&]
    {
        probe.unchanged();require(!impl.can_apply()&&impl.request().empty()&&!impl.in_match()&&
            tune_colour_bytes(save_path)==saved_preferences&&pref_values()==saved_values,
            "Track command changed private prefs/default display/JumpJ or queued work");
        for(size_t i=0;i<files.size();++i)
        {
            const auto& expected=i==1&&persisted?expected_saved:real_bytes[i];
            require(tune_colour_bytes(content/files[i])==real_bytes[i]&&tune_colour_bytes(source/files[i])==expected&&tune_colour_bytes(staged/files[i])==expected,
                "Track command changed actual content/private source/staged bytes beyond exact one token");
        }
    };
    const auto command=[&](const std::string& line)
    {
        const size_t before=impl.log().lines().size();std::srand(73401);const int next=std::rand();std::srand(73401);
        const auto status=impl.execute(line);++commands;require(status.ok&&std::rand()==next,"Actual Track command failed or consumed RNG");
        if(line.starts_with("tune "))
        {const auto& log=impl.log().lines();require(log.size()>before&&log.back().severity==DebugSeverity::info&&
            log.back().text.starts_with("tune: presentation.hud.meter_track_colour")&&log.back().text.ends_with("(live)"),"Track live command reply differs");}
    };
    Frame initial_raster{},saved_raster{};std::ofstream records(output/"hud-track-colour-tune.csv");
    records<<"state,context,background,fill_width,roi_rgba_match,adoption_match,repeat_match,literal_channels,delta_suffix,full_cover_invariant,fill_roles_unchanged,frozen_inputs_resources_rng,source_models,private_bytes,adoption\n";
    const auto check=[&](const char* state,const Colour& expected)
    {
        unchanged();require(presentation.hud().meter_track_colour==expected,"Track command parsed wrong literal channels");
        Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual(1);adoption=probe.adoption(1,expected);raw=probe.raw(1,expected);
            require(actual.pixels==adoption.pixels,"Track copied full-HUD adoption differs after actual command");
            const std::string models=HudTrackColourProbe::check_rois(1,actual,raw,expected);
            if(std::string_view(state)=="baseline"){initial_raster=actual;}
            HudTrackColourProbe::delta_suffix(1,actual,initial_raster);HudTrackColourProbe::fill_roles(1,actual,initial_raster);++role_cases;
            if(std::string_view(state)=="reset-original")
            {require(actual.pixels==initial_raster.pixels,"Track original reset failed exact whole-HUD RGBA restoration");}
            if(std::string_view(state)=="rgba128"){saved_raster=actual;}
            if(std::string_view(state)=="reset-saved"||std::string_view(state)=="reset-all-saved")
            {require(actual.pixels==saved_raster.pixels,"Track saved reset failed exact whole-HUD RGBA restoration");}
            unchanged();records<<state<<",half,canvas32-64-96,96,true,true,true,true,true,not-applicable,true,true,"<<models<<",true,live\n";++rows;
        }
        catch(...)
        {
            if(!actual.pixels.empty()){probe.failure(1,state,"debug-case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){probe.failure(1,state,"debug-adoption",adoption);}
            throw;
        }
    };
    check("baseline",Colour(184,184,184));command("tune presentation.hud.meter_track_colour RED");check("named",Colour(255,0,0));
    command("tune.reset presentation.hud.meter_track_colour");check("reset-original",Colour(184,184,184));
    command("tune presentation.hud.meter_track_colour 35BCE1");check("rgb",Colour(53,188,225));
    command("tune presentation.hud.meter_track_colour 0161C180");check("rgba128",Colour(1,97,193,128));
    const auto* document=impl.tune_document("presentation");require(document!=nullptr&&document->edits().size()==1&&document->edits().contains("hud.meter_track_colour"),
        "Track Save ledger differs from one exact token");
    command("tune.save presentation");persisted=true;unchanged();require(impl.tune_document("presentation")->edits().empty(),"Track Save failed rebase");
    command("tune presentation.hud.meter_track_colour A532D600");check("alpha0",Colour(165,50,214,0));
    command("tune.reset presentation.hud.meter_track_colour");check("reset-saved",Colour(1,97,193,128));
    command("tune presentation.hud.meter_track_colour 35BCE1");check("rgb-after-save",Colour(53,188,225));
    command("tune.reset presentation");check("reset-all-saved",Colour(1,97,193,128));
    require(impl.tune_document("presentation")->edits().empty()&&impl.tune_document("tuning")->edits().empty(),"Track final reset left document edits");
    require(records.good()&&commands==9&&rows==9&&role_cases==9&&probe.readbacks()==36,"Track Debug exact command/row/role/read budget differs");
    std::ofstream summary(output/"hud-track-colour-tune-summary.json");summary<<"{\"scope\":\"debug-validated-commands\",\"event_fade\":false,\"paired_context\":\"half\",\"commands\":9,\"states\":9,\"contexts\":1,\"readbacks\":36,\"csv_rows\":9,\"crops\":0,\"saved_tokens\":1,\"private_seed_saves\":2,\"fill_role_guard_cases\":9,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0}\n";
    require(summary.good(),"Cannot retain Track Debug summary");
    std::cout<<"PASS track tune:9real commands/9half-pair states/36additional readbacks/9rows; one exact Track token Save/rebase and original/saved whole-HUD resets,"
        "2private preference seeds/JumpJ/defaultdisplay/fixed health and team-ammo roles retained; zero all updates; no actualLevel/human/performance claim.\n";
}
#endif
