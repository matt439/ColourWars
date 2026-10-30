// Scribble39: complete independent67x67 note ROI; copied full-front adoption only.
// Frozen literal source tables are hypotheses until actual backend qualification.
enum class ScribbleReference { correct, straight, squared_base, collapsed_union };
struct ScribbleDab { int left, top, right, bottom, row; };
struct ScribbleCoverage
{
    std::array<unsigned char,67*67> counts{};
    std::array<int,67*67> rows{};
};
const char* scribble_context_name(size_t context)
{ constexpr std::array<const char*,2> names{"yellow-flat","blue-tilted"};return names.at(context); }
Colour scribble_paper(size_t context)
{ return context==0?Colour(255,242,122):Colour(174,235,255); }
const char* scribble_paper_token(size_t context) { return context==0?"FFF27A":"AEEBFF"; }
MonitorPersona scribble_persona(size_t context)
{
    require(context<2,"Unknown Scribble note context");MonitorPersona persona;
    persona.sticky_notes.push_back({Vector2F(.015f,.47f),context==0?size_t{0}:size_t{2},context==0?0.0f:.1f});return persona;
}
std::vector<float> scribble_fixed_style(const OpeningPresentation& p)
{
    OpeningPresentation stable=p;stable.scribble=Colour(48,80,192);auto values=program_fixed_style(stable);
    values.insert(values.end(),{p.program_colour.r,p.program_colour.g,p.program_colour.b,p.program_colour.a});return values;
}
const std::array<ScribbleDab,74>& scribble_dabs(size_t context)
{
    static constexpr std::array<ScribbleDab,74> a{{
        // Literal row0
        {25,356,28,359,0},
        {27,356,30,359,0},
        {29,356,31,359,0},
        {30,356,33,359,0},
        {32,356,35,359,0},
        {33,356,36,359,0},
        {35,356,38,359,0},
        {37,356,39,359,0},
        {38,356,41,359,0},
        {40,356,43,359,0},
        {42,356,44,359,0},
        {43,356,46,359,0},
        {45,356,48,359,0},
        {46,356,49,359,0},
        {48,356,51,359,0},
        {50,356,53,359,0},
        {51,356,54,359,0},
        {53,356,56,359,0},
        {55,356,57,359,0},
        {56,356,59,359,0},
        {58,356,61,359,0},
        {60,356,62,359,0},
        {61,356,64,359,0},
        {63,356,66,359,0},
        {64,356,67,359,0},
        {66,356,69,359,0},
        {68,356,70,359,0},
        {69,356,72,359,0},
        // Literal row1
        {25,368,28,371,1},
        {27,368,30,371,1},
        {29,368,31,371,1},
        {30,368,33,371,1},
        {32,368,35,371,1},
        {33,368,36,371,1},
        {35,368,38,371,1},
        {37,368,39,371,1},
        {38,368,41,371,1},
        {40,368,43,371,1},
        {42,368,44,371,1},
        {43,368,46,371,1},
        {45,368,48,371,1},
        {46,368,49,371,1},
        {48,368,51,371,1},
        {50,368,53,371,1},
        {51,368,54,371,1},
        {53,368,56,371,1},
        {55,368,57,371,1},
        {56,368,59,371,1},
        {58,368,61,371,1},
        {60,368,62,371,1},
        {61,368,64,371,1},
        {63,368,66,371,1},
        {64,368,67,371,1},
        {66,368,69,371,1},
        {68,368,70,371,1},
        {69,368,72,371,1},
        // Literal row2
        {25,381,28,383,2},
        {27,381,30,383,2},
        {29,381,31,383,2},
        {30,381,33,383,2},
        {32,381,35,383,2},
        {33,381,36,383,2},
        {35,381,38,383,2},
        {37,381,39,383,2},
        {38,381,41,383,2},
        {40,381,43,383,2},
        {41,381,44,383,2},
        {43,381,46,383,2},
        {45,381,47,383,2},
        {46,381,49,383,2},
        {48,381,51,383,2},
        {50,381,52,383,2},
        {51,381,54,383,2},
        {53,381,56,383,2},
    }};
    static constexpr std::array<ScribbleDab,74> b{{
        // Literal row0
        {25,356,28,359,0},
        {27,356,30,359,0},
        {29,356,31,359,0},
        {30,357,33,359,0},
        {32,357,35,360,0},
        {33,357,36,360,0},
        {35,357,38,360,0},
        {37,357,39,360,0},
        {38,357,41,360,0},
        {40,358,43,360,0},
        {42,358,44,360,0},
        {43,358,46,361,0},
        {45,358,48,361,0},
        {46,358,49,361,0},
        {48,358,51,361,0},
        {50,359,53,361,0},
        {51,359,54,361,0},
        {53,359,56,362,0},
        {55,359,57,362,0},
        {56,359,59,362,0},
        {58,359,61,362,0},
        {60,360,62,362,0},
        {61,360,64,362,0},
        {63,360,66,363,0},
        {64,360,67,363,0},
        {66,360,69,363,0},
        {68,360,70,363,0},
        {69,361,72,363,0},
        // Literal row1
        {25,368,28,371,1},
        {27,369,30,371,1},
        {29,369,31,371,1},
        {30,369,33,372,1},
        {32,369,35,372,1},
        {33,369,36,372,1},
        {35,369,38,372,1},
        {37,369,39,372,1},
        {38,370,41,372,1},
        {40,370,43,373,1},
        {42,370,44,373,1},
        {43,370,46,373,1},
        {45,370,48,373,1},
        {46,370,49,373,1},
        {48,371,51,373,1},
        {50,371,53,374,1},
        {51,371,54,374,1},
        {53,371,56,374,1},
        {55,371,57,374,1},
        {56,371,59,374,1},
        {58,372,61,374,1},
        {60,372,62,375,1},
        {61,372,64,375,1},
        {63,372,66,375,1},
        {64,372,67,375,1},
        {66,372,69,375,1},
        {68,373,70,375,1},
        {69,373,72,376,1},
        // Literal row2
        {25,381,28,383,2},
        {27,381,30,384,2},
        {29,381,31,384,2},
        {30,381,33,384,2},
        {32,381,35,384,2},
        {33,381,36,384,2},
        {35,382,38,384,2},
        {37,382,39,384,2},
        {38,382,41,385,2},
        {40,382,43,385,2},
        {41,382,44,385,2},
        {43,382,46,385,2},
        {45,383,47,385,2},
        {46,383,49,385,2},
        {48,383,51,386,2},
        {50,383,52,386,2},
        {51,383,54,386,2},
        {53,383,56,386,2},
    }};
    require(context<2,"Unknown Scribble literal context");return context==0?a:b;
}


const ScribbleCoverage& scribble_coverage(size_t context);
class ScribbleColourProbe
{
public:
    ScribbleColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output);
    void unchanged() const;
    Frame actual(size_t context);
    Frame adoption(size_t context,const Colour& colour);
    Frame raw(size_t context,const Colour& colour,ScribbleReference mode=ScribbleReference::correct);
    static void check_roi(size_t context,const Frame& actual,const Frame& raw,const Colour& colour);
    static void check_control(size_t context,const Frame& actual,const Frame& wrong,ScribbleReference mode,
        size_t& row_discriminations,size_t& changed,size_t& unchanged_pixels);
    static void delta_only_union(size_t context,const Frame& actual,const Frame& baseline);
    static Frame compact(const Frame& source) { return foreground_crop(source,{18,337,67,67},0); }
    void failure(size_t context,const char* state,const char* stage,const Frame& actual,const Frame* reference=nullptr) const;
    size_t readbacks() const { return readbacks_; }
private:
    static size_t offset(const Frame& frame,int x,int y)
    { return (static_cast<size_t>(y)*static_cast<size_t>(frame.width)+static_cast<size_t>(x))*4; }
    void write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint);
    Frame render(size_t context,const std::function<void(DrawList&)>& draw);
    const Monitor& monitor(size_t context) const { return context==0?monitor_a_:monitor_b_; }
    const Monitor& reference(size_t context) const { return context==0?reference_a_:reference_b_; }
    Application& app_;const Presentation& presentation_;const OpeningPresentation& look_;OpeningPresentation baseline_,expected_;
    Monitor monitor_a_,monitor_b_,reference_a_,reference_b_;
    const SpriteSheet* sheet_;const SpriteSheet* palette_sheet_;const SpriteFrame& frame_;
    std::vector<const SpriteFrame*> palette_frames_;const std::filesystem::path output_;std::vector<float> fixed_;
    std::array<std::vector<float>,2> geometry_{};
    const RectangleF screen_{0,0,1280,720};const Camera camera_=Camera::DEFAULT_CAMERA;size_t readbacks_=0;
};

std::array<int,3> scribble_single_counts(size_t context)
{ return context==0?std::array<int,3>{48,48,24}:std::array<int,3>{52,54,37}; }
std::array<int,3> scribble_double_counts(size_t context)
{ return context==0?std::array<int,3>{93,93,38}:std::array<int,3>{80,82,50}; }
const ScribbleCoverage& scribble_coverage(size_t context)
{
    static const auto maps=[]
    {
        std::array<ScribbleCoverage,2> result{};
        for(size_t c=0;c<2;++c)
        {
            auto& map=result[c];map.rows.fill(-1);
            for(const auto& dab:scribble_dabs(c))
            {
                require(dab.left>=18&&dab.right<=85&&dab.top>=337&&dab.bottom<=404&&
                    dab.left<dab.right&&dab.top<dab.bottom&&dab.row>=0&&dab.row<3,"Scribble literal dab escaped declared ROI");
                for(int y=dab.top;y<dab.bottom;++y){for(int x=dab.left;x<dab.right;++x)
                {
                    const size_t at=static_cast<size_t>((y-337)*67+x-18);
                    require(map.rows[at]<0||map.rows[at]==dab.row,"Scribble source rows unexpectedly overlap");
                    map.rows[at]=dab.row;++map.counts[at];require(map.counts[at]<=2,"Scribble source multiplicity exceeds frozen two writes");
                    require(x>=22&&x<77&&y>=349&&y<393,"Scribble selected site is not over plain opaque note paper");
                }}
            }
            std::array<int,3> singles{},doubles{};
            for(size_t at=0;at<map.counts.size();++at)
            {if(map.counts[at]){const size_t row=static_cast<size_t>(map.rows[at]);
                if(map.counts[at]==1){++singles[row];}else{++doubles[row];}}}
            require(singles==scribble_single_counts(c)&&doubles==scribble_double_counts(c),"Scribble literal union/multiplicity table changed");
        }
        return result;
    }();
    return maps.at(context);
}
ScribbleColourProbe::ScribbleColourProbe(Application& app,const Presentation& presentation,const std::filesystem::path& output) :
    app_(app),presentation_(presentation),look_(presentation.opening()),baseline_(look_),expected_(baseline_),
    monitor_a_(&look_,app.render_resources(),scribble_persona(0)),monitor_b_(&look_,app.render_resources(),scribble_persona(1)),
    reference_a_(&expected_,app.render_resources(),scribble_persona(0)),reference_b_(&expected_,app.render_resources(),scribble_persona(1)),
    sheet_(app.render_resources()->sprite_sheet(look_.sprite_sheet)),palette_sheet_(app.render_resources()->sprite_sheet(look_.palette_sheet)),
    frame_(sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))),output_(output),fixed_(scribble_fixed_style(look_))
{
    require(look_.sprite_sheet=="sprite_sheet_1"&&look_.fill_frame=="pixel"&&
        frame_.source_rectangle()==RectangleI(32,0,4,4)&&frame_.origin()==Vector2F::ZERO&&
        look_.monitor_art.pen_width==.004f&&look_.monitor_art.corner_step==.012f&&
        look_.bezel_shadow==Colour(0,0,0,89)&&look_.monitor_art.ink==Colour(0,0,0)&&look_.monitor_art.highlight==Colour(238,238,238)&&
        look_.sticky_notes.size()==4&&look_.sticky_notes[0]==Colour(255,242,122)&&look_.sticky_notes[2]==Colour(174,235,255),
        "Scribble fixture lost shipped white frame, geometry, unselected material or two actual paper assumptions");
    for(const auto& name:look_.palette_frames)
    {palette_frames_.push_back(&palette_sheet_->sprite_frame(palette_sheet_->resolve_sprite_frame(name)));}
    for(size_t c=0;c<2;++c){geometry_[c]=glass_monitor_geometry(monitor(c));}
    std::ignore=scribble_coverage(0);std::ignore=scribble_coverage(1);unchanged();
}
void ScribbleColourProbe::unchanged() const
{
    require(&look_==&presentation_.opening()&& &look_.monitor_art==&presentation_.opening().monitor_art&&
        scribble_fixed_style(look_)==fixed_&&scribble_fixed_style(expected_)==fixed_&&
        look_.sprite_sheet==baseline_.sprite_sheet&&look_.fill_frame==baseline_.fill_frame&&look_.palette_sheet==baseline_.palette_sheet&&
        look_.palette_frames==baseline_.palette_frames&&look_.percent_font==baseline_.percent_font&&
        look_.bezel_shades==baseline_.bezel_shades&&look_.sticky_notes==baseline_.sticky_notes&&look_.stickers==baseline_.stickers&&
        sheet_==app_.render_resources()->sprite_sheet(look_.sprite_sheet)&&palette_sheet_==app_.render_resources()->sprite_sheet(look_.palette_sheet)&&
        &frame_==&sheet_->sprite_frame(sheet_->resolve_sprite_frame(look_.fill_frame))&&
        screen_==RectangleF(0,0,1280,720)&&camera_.translation==Vector2F::ZERO&&camera_.scale==1.0f,
        "Scribble edit/draw changed borrowed records, unselected style, arrays/resources or caller screen/camera");
    for(size_t c=0;c<2;++c)
    {
        const auto expected_persona=scribble_persona(c);
        for(const Monitor* item:{&monitor(c),&reference(c)})
        {
            const auto& persona=item->persona();
            require(persona.sticky_notes.size()==1&&persona.stickers.empty()&&persona.crack.empty()&&
                monitor_persona_observation(persona)==monitor_persona_observation(expected_persona),
                "Scribble drawing changed complete captured persona or decoration vector counts");
        }
        require(glass_monitor_geometry(monitor(c))==geometry_[c]&&glass_monitor_geometry(reference(c))==geometry_[c],
            "Scribble captured public Monitor geometry diverged");
        const RectangleF target(100,80,640,360);
        require(monitor(c).screen_for_glass(target)==reference(c).screen_for_glass(target),"Scribble inverse glass transform diverged");
    }
    for(size_t i=0;i<look_.palette_frames.size();++i)
    {require(palette_frames_[i]==&palette_sheet_->sprite_frame(palette_sheet_->resolve_sprite_frame(look_.palette_frames[i])),
        "Scribble drawing changed captured palette resources");}
}
Frame ScribbleColourProbe::render(size_t context,const std::function<void(DrawList&)>& draw)
{
    unchanged();const Colour selected=look_.scribble,expected_selected=expected_.scribble;
    std::srand(52739);const int next=std::rand();std::srand(52739);
    Renderer& renderer=*app_.renderer();const Canvas canvas(app_.render_resources(),Colour(32,64,96));
    renderer.begin_frame();renderer.set_view_count(1);DrawList list=renderer.view(0);
    list.set_viewport(app_.viewport_manager()->fullscreen_viewport());list.set_camera(camera_);list.set_filter(TextureFilter::point);
    canvas.draw(list,screen_);draw(list);++readbacks_;Frame result=read(renderer);
    if(result.width!=1280||result.height!=720){failure(context,"unknown","backbuffer-size",result);throw std::runtime_error("Scribble requires1280x720 backbuffer");}
    const bool fixed_draw=std::rand()==next&&look_.scribble==selected&&expected_.scribble==expected_selected;
    if(!fixed_draw){failure(context,"unknown","draw-state-or-rng",result);}
    require(fixed_draw,"Scribble draw consumed RNG or changed selected records");
    unchanged();return result;
}
Frame ScribbleColourProbe::actual(size_t context)
{
    const auto draw=[&](DrawList& list){monitor(context).draw_front_details(list,screen_);};
    Frame first=render(context,draw);const Frame repeat=render(context,draw);
    if(first.pixels!=repeat.pixels){failure(context,"unknown","repeat",first,&repeat);}
    require(first.pixels==repeat.pixels,"Frozen Scribble front-detail repeat changed RGBA");return first;
}
Frame ScribbleColourProbe::adoption(size_t context,const Colour& colour)
{expected_.scribble=colour;return render(context,[&](DrawList& list){reference(context).draw_front_details(list,screen_);});}
void ScribbleColourProbe::write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint)
{
    list.draw_sprite(sheet_->texture(),frame_.source_rectangle(),RectangleF(static_cast<float>(left),static_cast<float>(top),
        static_cast<float>(right-left),static_cast<float>(bottom-top)),tint,0.0f,frame_.origin(),SpriteFlip::none,0.0f);
}
Frame ScribbleColourProbe::raw(size_t context,const Colour& colour,ScribbleReference mode)
{
    const Frame result=render(context,[&](DrawList& list)
    {
        write_raw(list,100,337,104,341,Colour(255,255,255));write_raw(list,105,337,109,341,Colour(0,0,0));
        // Five literal unselected layers in actual source order; no flat underlayer.
        write_raw(list,22,342,84,403,Colour(0.0f,0.0f,0.0f,89.0f/255.0f));
        write_raw(list,19,338,80,399,Colour(0,0,0));write_raw(list,22,341,77,396,scribble_paper(context));
        write_raw(list,22,341,77,349,Colour(0.0f,0.0f,0.0f,.12f));write_raw(list,24,393,74,396,Colour(238,238,238));
        const float factor=mode==ScribbleReference::straight?1.0f:mode==ScribbleReference::squared_base?colour.a*colour.a:colour.a;
        const Colour tint(colour.r*factor,colour.g*factor,colour.b*factor,colour.a);
        if(mode==ScribbleReference::collapsed_union)
        {
            // Exact same union, once per pixel: nonoverlapping unit-height runs.
            const auto& coverage=scribble_coverage(context);
            for(int y=337;y<404;++y){int x=18;while(x<85)
            {
                if(!coverage.counts[static_cast<size_t>((y-337)*67+x-18)]){++x;continue;}
                const int begin=x;++x;
                while(x<85&&coverage.counts[static_cast<size_t>((y-337)*67+x-18)]){++x;}
                write_raw(list,begin,y,x,y+1,tint);
            }}
        }
        else{for(const auto& dab:scribble_dabs(context)){write_raw(list,dab.left,dab.top,dab.right,dab.bottom,tint);}}
    });
    const bool calibrated=monitor_rgb(result,101,338)==0xFFFFFFu&&monitor_rgb(result,106,338)==0&&
        result.pixels[offset(result,101,338)+3]==255&&result.pixels[offset(result,106,338)+3]==255;
    if(!calibrated){failure(context,"unknown","raw-calibration",result);}
    require(calibrated,"Scribble raw WHITE/BLACK frame RGBA calibration differs");return result;
}
void ScribbleColourProbe::failure(size_t context,const char* state,const char* stage,const Frame& actual,const Frame* other) const
{
    const std::string stem=std::string("scribble-failure-")+scribble_context_name(context)+"-"+state+"-"+stage;
    save(actual,output_/(stem+"-actual.ppm"));if(other!=nullptr){save(*other,output_/(stem+"-reference.ppm"));}
    std::ofstream receipt(output_/(stem+".json"));receipt<<"{\"context\":\""<<scribble_context_name(context)<<"\",\"state\":\""<<state
        <<"\",\"stage\":\""<<stage<<"\",\"readbacks\":"<<readbacks_<<",\"roi\":[18,337,85,404],"
        "\"diagnostic\":\"preserve-source-hypothesis-and-investigate-layers-no-silent-site-moves\"}\n";
}

void ScribbleColourProbe::check_roi(size_t context,const Frame& actual,const Frame& raw,const Colour& colour)
{
    const auto& coverage=scribble_coverage(context);
    // Complete exact actual/raw RGBA and opaque-underlay checks FIRST. A later
    // supplementary source-model assertion cannot interrupt this primary pass.
    for(int y=337;y<404;++y){for(int x=18;x<85;++x)
    {
        const size_t at=offset(actual,x,y);
        require(same_pixel(actual,raw,at),"Scribble complete ordered67x67 raw ROI differs from actual RGBA");
        require(actual.pixels[at+3]==255,"Scribble note ROI lost opaque Canvas/paper alpha");
    }}
    // Two source-derived representative RGB blend models, not vertex packing
    // or proof of a GPU's internals/conformance. F retains the source RGB term;
    // Q rounds that term to8UNORM before blending. Selected alpha128/255 is
    // already8UNORM in both. The independent PPM verifier checks four whole-
    // crop RGB-float/RGB8 by alpha-float/alpha8 source representatives; their
    // selected128/255 tuples reduce to these two matrices. Require ONE coherent
    // matrix over the entire selected partial ROI; per-pixel mixing is refused.
    constexpr std::array<std::array<std::uint32_t,2>,2> partial_f{{{0x8FA99Du,0x5784AFu},{0x67A5DFu,0x4382CFu}}};
    constexpr std::array<std::array<std::uint32_t,2>,2> partial_q{{{0x8FA99Du,0x5784AEu},{0x67A5DFu,0x4382CFu}}};
    const bool partial=colour.a!=0.0f&&colour.a!=1.0f;bool matches_f=true,matches_q=true;
    require(!partial||colour==Colour(32,96,192,128),"Scribble source models cover only the declared partial palette");
    const auto paper=context==0?0xFFF27Au:0xAEEBFFu;
    const auto opaque=(static_cast<std::uint32_t>(std::round(colour.r*255))<<16)|
        (static_cast<std::uint32_t>(std::round(colour.g*255))<<8)|static_cast<std::uint32_t>(std::round(colour.b*255));
    for(int y=337;y<404;++y){for(int x=18;x<85;++x)
    {
        const size_t site=static_cast<size_t>((y-337)*67+x-18);
        if(coverage.counts[site])
        {
            if(colour.a==0.0f){require(monitor_rgb(actual,x,y)==paper,"Scribble alpha0 did not return actual opaque paper");}
            else if(colour.a==1.0f){require(monitor_rgb(actual,x,y)==opaque,"Scribble opaque literal channel calibration differs");}
            else
            {
                const size_t writes=static_cast<size_t>(coverage.counts[site]-1);const auto rgb=monitor_rgb(actual,x,y);
                matches_f=matches_f&&rgb==partial_f.at(context).at(writes);
                matches_q=matches_q&&rgb==partial_q.at(context).at(writes);
            }
        }
    }}
    require(!partial||matches_f||matches_q,"Scribble selected partial ROI matches neither one complete F nor Q source matrix");
    require(monitor_rgb(actual,18,337)==0x204060u&&monitor_rgb(actual,84,403)==0x204060u&&
        monitor_rgb(actual,30,350)==paper&&monitor_rgb(actual,30,394)==0xEEEEEEu,
        "Scribble unselected Canvas/paper/underline literal guard differs");
}
void ScribbleColourProbe::check_control(size_t context,const Frame& actual,const Frame& wrong,ScribbleReference mode,
    size_t& row_discriminations,size_t& changed,size_t& unchanged_pixels)
{
    require(mode!=ScribbleReference::correct,"Scribble control requires an intentionally wrong reference");
    const auto& coverage=scribble_coverage(context);std::array<int,3> changed_by_row{},single_guards{};
    for(int y=337;y<404;++y){for(int x=18;x<85;++x)
    {
        const size_t site=static_cast<size_t>((y-337)*67+x-18);const auto writes=coverage.counts[site];
        const bool affected=writes>0&&(mode!=ScribbleReference::collapsed_union||writes>1);
        const bool equal=same_pixel(actual,wrong,offset(actual,x,y));
        require(affected?!equal:equal,"Scribble wrong reference failed exact per-pixel affected/unchanged mask discrimination");
        if(affected){++changed;++changed_by_row[static_cast<size_t>(coverage.rows[site])];}
        else{++unchanged_pixels;if(writes==1){++single_guards[static_cast<size_t>(coverage.rows[site])];}}
    }}
    const auto singles=scribble_single_counts(context),doubles=scribble_double_counts(context);
    for(size_t row=0;row<3;++row)
    {
        const int expected=mode==ScribbleReference::collapsed_union?doubles[row]:singles[row]+doubles[row];
        require(expected>0&&changed_by_row[row]==expected,"Scribble control did not separately discriminate every selected row");
        if(mode==ScribbleReference::collapsed_union)
        {require(single_guards[row]==singles[row],"Scribble collapsed union changed single-covered pixels");}
        ++row_discriminations;
    }
}
void ScribbleColourProbe::delta_only_union(size_t context,const Frame& actual,const Frame& baseline)
{
    const auto& coverage=scribble_coverage(context);
    for(int y=0;y<actual.height;++y){for(int x=0;x<actual.width;++x)
    {
        if(same_pixel(actual,baseline,offset(actual,x,y))){continue;}
        require(x>=18&&x<85&&y>=337&&y<404,"Scribble colour delta escaped the independent ROI");
        require(coverage.counts[static_cast<size_t>((y-337)*67+x-18)]>0,"Scribble colour delta escaped the frozen selected union");
    }}
}
void scribble_colour_consumer_checks(Application& app,const Presentation& presentation,const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);app.resolution_manager()->set_resolution_exactly({1280,720});
    std::ignore=app.renderer()->window_size_changed(1280,720);Presentation local=presentation;
    auto& look=const_cast<OpeningPresentation&>(local.opening());require(look.scribble==Colour(48,80,192),"Scribble authored default changed");
    ScribbleColourProbe probe(app,local,output);
    const std::array<Colour,4> colours{Colour(48,80,192),Colour(53,188,225),Colour(32,96,192,128),Colour(165,50,214,0)};
    constexpr std::array<const char*,4> states{"default","rgb","rgba128","alpha0"};
    std::array<Frame,2> baseline{};size_t rows=0,crops=0,controls=0,row_discriminations=0,changed=0,unchanged_pixels=0;
    std::ofstream records(output/"scribble-colour-consumers.csv");
    records<<"state,context,background,paper,roi_rgba_match,adoption_match,repeat_match,guard_channels,delta_union,frozen_inputs_resources_rng,controls,control_row_discriminations,changed_control_pixels,unchanged_control_pixels,scope\n";
    for(size_t c=0;c<2;++c){for(size_t p=0;p<colours.size();++p)
    {
        look.scribble=colours[p];Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual(c);adoption=probe.adoption(c,colours[p]);raw=probe.raw(c,colours[p]);
            require(actual.pixels==adoption.pixels,"Scribble copied complete front-detail consumer adoption differs");
            ScribbleColourProbe::check_roi(c,actual,raw,colours[p]);
            if(p==0){baseline[c]=actual;}else
            {ScribbleColourProbe::delta_only_union(c,actual,baseline[c]);require(actual.pixels!=baseline[c].pixels,"Scribble edited palette produced no actual delta");}
            size_t case_controls=0,case_rows=0,case_changed=0,case_unchanged=0;
            const auto control=[&](ScribbleReference mode,const char* label)
            {
                const Frame wrong=probe.raw(c,colours[p],mode);
                try{ScribbleColourProbe::check_control(c,actual,wrong,mode,case_rows,case_changed,case_unchanged);}
                catch(...){probe.failure(c,states[p],label,actual,&wrong);throw;}
                ++case_controls;
            };
            if(p==2)
            {control(ScribbleReference::straight,"partial-straight");control(ScribbleReference::squared_base,"partial-squared");
                control(ScribbleReference::collapsed_union,"partial-collapsed-union");}
            else if(p==3){control(ScribbleReference::straight,"alpha0-straight");}
            records<<states[p]<<','<<scribble_context_name(c)<<",canvas32-64-96,"<<scribble_paper_token(c)
                <<",true,true,true,true,true,true,"<<case_controls<<','<<case_rows<<','<<case_changed<<','<<case_unchanged<<",consumer-only\n";
            ++rows;controls+=case_controls;row_discriminations+=case_rows;changed+=case_changed;unchanged_pixels+=case_unchanged;
            save(ScribbleColourProbe::compact(actual),output/(std::string("scribble-colour-")+scribble_context_name(c)+"-"+states[p]+".ppm"));++crops;
        }
        catch(...)
        {
            if(!actual.pixels.empty()){probe.failure(c,states[p],"case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){save(adoption,output/(std::string("scribble-failure-")+scribble_context_name(c)+"-"+states[p]+"-adoption.ppm"));}
            throw;
        }
    }}
    require(records.good()&&probe.readbacks()==40&&rows==8&&crops==8&&controls==8&&row_discriminations==24&&changed==2533&&unchanged_pixels==33379,
        "Scribble shared exact readback/row/control/crop budget differs");
    std::ofstream summary(output/"scribble-colour-consumer-summary.json");
    summary<<"{\"scope\":\"consumer-only\",\"readbacks\":40,\"csv_rows\":8,\"crops\":8,\"crop_width\":67,\"crop_height\":67,\"crop_pixels\":35912,\"ppm_bytes\":107840,"
        "\"contexts\":2,\"palettes\":4,\"roi_pixels\":4489,\"ordered_fixed_rectangles\":5,\"selected_dabs_per_context\":74,\"rows_per_note\":3,\"blank_reads\":0,\"controls\":8,"
        "\"partial_straight_reads\":2,\"partial_squared_reads\":2,\"partial_collapsed_union_reads\":2,\"alpha0_straight_reads\":2,\"control_row_discriminations\":24,"
        "\"changed_control_pixels\":2533,\"unchanged_control_pixels\":33379,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,"
        "\"viewport\":\"1280x720\",\"background\":\"canvas32-64-96\",\"event_fade\":false,\"raw_reference\":\"complete67x67-ordered-five-plus74-literal-rectangles-only\","
        "\"whole_frame_reference\":\"copied-production-front-consumer-adoption-only\",\"roi_half_open\":[18,337,85,404]}\n";
    require(summary.good(),"Cannot retain Scribble shared summary");
    std::cout<<"PASS scribble consumer:40readbacks/8rows/8native67x67 crops35912pixels; complete independent note ROI,8controls/24row discriminations,"
        "full copied-front adoption only; retained caller/resource/RNG and zero all updates. Actual Level/story/human/performance not exercised.\n";
}

#if COLOURWARS_DEV_TOOLS
void scribble_tune_colour_checks(Application& app,Registry<LevelLoadedInfo>& levels,GameTuning& tuning,Presentation& presentation,
    const std::filesystem::path& content,const std::filesystem::path& output)
{
    const auto root=output/"scribble-colour-tune-scratch",source=root/"source",staged=root/"staged";
    std::filesystem::create_directories(source);std::filesystem::create_directories(staged);
    constexpr std::array<const char*,2> files{"tuning.json","presentation.json"};std::array<std::string,2> real_bytes;
    for(size_t i=0;i<files.size();++i)
    {real_bytes[i]=tune_colour_bytes(content/files[i]);tune_colour_write(source/files[i],real_bytes[i]);tune_colour_write(staged/files[i],real_bytes[i]);}
    const NestedSaveProfile profile(root/"profile");const TuneColourTemp temporary(root/"temp");
    struct RestoreScribble
    {GameTuning& t;Presentation& p;GameTuning old_t;Presentation old_p;~RestoreScribble(){t=old_t;p=old_p;}}
        restore{tuning,presentation,tuning,presentation};
    Save preferences;PlaySettings play;play.music_volume=17;play.sfx_volume=83;play.aim_sensitivity=4;
    play.view_shake=false;play.aim_marker=true;play.reduce_effects=true;play.sticks[2].movement_deadzone=25;play.sticks[2].aim_deadzone=70;
    InputBindings bindings;bindings.keyboard_jump=Key::j;
    require(preferences.set_play_settings_and_save(play)&&preferences.set_input_bindings_and_save(bindings),"Cannot seed Scribble private preferences");
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
    require(impl.log().set_transcript((output/"scribble-colour-tune-commands.log").string()),"Cannot retain Scribble command transcript");
    ScribbleColourProbe probe(app,presentation,output);JsonSpanDocument initial(real_bytes[1],"Scribble private source");
    const JsonLeaf* leaf=initial.find("opening.scribble");require(leaf!=nullptr,"Scribble source lacks exact existing token");
    require(presentation.opening().scribble==Colour(48,80,192),"Scribble Debug baseline differs from authored default");
    const std::string expected_saved=initial.replaced(*leaf,"\"2060C080\"");bool persisted=false;size_t commands=0,rows=0;
    const auto unchanged=[&]
    {
        probe.unchanged();require(!impl.can_apply()&&impl.request().empty()&&!impl.in_match()&&
            tune_colour_bytes(save_path)==saved_preferences&&pref_values()==saved_values,
            "Scribble commands changed private preferences/default display/JumpJ or queued work");
        for(size_t i=0;i<files.size();++i)
        {
            const auto& expected=i==1&&persisted?expected_saved:real_bytes[i];
            require(tune_colour_bytes(content/files[i])==real_bytes[i]&&tune_colour_bytes(source/files[i])==expected&&tune_colour_bytes(staged/files[i])==expected,
                "Scribble command changed actual content/private source/staged bytes beyond exact one token");
        }
    };
    const auto command=[&](const std::string& line)
    {
        const size_t before=impl.log().lines().size();std::srand(52739);const int next=std::rand();std::srand(52739);
        const auto status=impl.execute(line);++commands;require(status.ok&&std::rand()==next,"Actual Scribble command failed or consumed RNG");
        if(line.starts_with("tune "))
        {const auto& log=impl.log().lines();require(log.size()>before&&log.back().severity==DebugSeverity::info&&
            log.back().text.starts_with("tune: presentation.opening.scribble")&&log.back().text.ends_with("(live)"),"Scribble live command reply differs");}
    };
    Frame initial_raster{},saved_raster{};std::ofstream records(output/"scribble-colour-tune.csv");
    records<<"state,context,background,paper,roi_rgba_match,adoption_match,repeat_match,guard_channels,delta_union,frozen_inputs_resources_rng,private_bytes,adoption\n";
    const auto check=[&](const char* state,const Colour& expected)
    {
        unchanged();require(presentation.opening().scribble==expected,"Scribble command parsed wrong literal channels");
        Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual(0);adoption=probe.adoption(0,expected);raw=probe.raw(0,expected);
            require(actual.pixels==adoption.pixels,"Scribble copied full-front adoption differs after actual command");
            ScribbleColourProbe::check_roi(0,actual,raw,expected);
            if(std::string_view(state)=="baseline"){initial_raster=actual;}
            else{ScribbleColourProbe::delta_only_union(0,actual,initial_raster);}
            if(std::string_view(state)=="reset-original")
            {require(actual.pixels==initial_raster.pixels,"Scribble original reset failed exact whole-consumer RGBA restoration");}
            if(std::string_view(state)=="rgba128"){saved_raster=actual;}
            if(std::string_view(state)=="reset-saved"||std::string_view(state)=="reset-all-saved")
            {require(actual.pixels==saved_raster.pixels,"Scribble saved reset failed exact whole-consumer RGBA restoration");}
            unchanged();records<<state<<",yellow-flat,canvas32-64-96,FFF27A,true,true,true,true,true,true,true,live\n";++rows;
        }
        catch(...)
        {
            if(!actual.pixels.empty()){probe.failure(0,state,"debug-case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){save(adoption,output/(std::string("scribble-failure-yellow-flat-")+state+"-debug-adoption.ppm"));}
            throw;
        }
    };
    check("baseline",Colour(48,80,192));command("tune presentation.opening.scribble RED");check("named",Colour(255,0,0));
    command("tune.reset presentation.opening.scribble");check("reset-original",Colour(48,80,192));
    command("tune presentation.opening.scribble 35BCE1");check("rgb",Colour(53,188,225));
    command("tune presentation.opening.scribble 2060C080");check("rgba128",Colour(32,96,192,128));
    const auto* document=impl.tune_document("presentation");require(document!=nullptr&&document->edits().size()==1&&document->edits().contains("opening.scribble"),
        "Scribble Save ledger differs from one exact token");
    command("tune.save presentation");persisted=true;unchanged();require(impl.tune_document("presentation")->edits().empty(),"Scribble Save failed rebase");
    command("tune presentation.opening.scribble A532D600");check("alpha0",Colour(165,50,214,0));
    command("tune.reset presentation.opening.scribble");check("reset-saved",Colour(32,96,192,128));
    command("tune presentation.opening.scribble 35BCE1");check("rgb-after-save",Colour(53,188,225));
    command("tune.reset presentation");check("reset-all-saved",Colour(32,96,192,128));
    require(impl.tune_document("presentation")->edits().empty()&&impl.tune_document("tuning")->edits().empty(),"Scribble final reset left document edits");
    require(records.good()&&commands==9&&rows==9&&probe.readbacks()==36,"Scribble Debug exact command/row/read budget differs");
    std::ofstream summary(output/"scribble-colour-tune-summary.json");summary<<"{\"scope\":\"debug-validated-commands\",\"commands\":9,\"states\":9,\"contexts\":1,"
        "\"readbacks\":36,\"csv_rows\":9,\"crops\":0,\"saved_tokens\":1,\"private_seed_saves\":2,\"event_fade\":false,"
        "\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0}\n";
    require(summary.good(),"Cannot retain Scribble Debug summary");
    std::cout<<"PASS scribble tune:9real commands/9states/ContextA/36additional reads/9rows; one exact Scribble token Save/rebase, original/saved full-RGBA reset,"
        "2private preference seed Saves/JumpJ/defaultdisplay retained, zero all updates; no actualLevel/story/human/performance claim.\n";
}
#endif
