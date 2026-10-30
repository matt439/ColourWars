// Limb43/44: independent literal procedural ROIs; copied full fixture adoption only.
// Ten explicit zero-delta Weapon preparations precede observations; no Player/Level is built.
enum class LimbReference { correct, straight_arm, squared_arm, straight_hand, squared_hand };
std::array<std::string,5> limb_figure_names(const FigurePresentation& p)
{return {p.sprite_sheet,p.rest_arms_frame,p.rest_arms_paint_frame,p.sleeves_frame,p.sleeves_paint_frame};}
std::vector<float> limb_figure_fixed(const FigurePresentation& p)
{
    const auto& q=p.quiet;std::vector<float> v{p.canvas_size.x,p.canvas_size.y,p.body_inset,p.rear_shoulder.x,p.rear_shoulder.y,
        p.front_shoulder.x,p.front_shoulder.y,p.arm_width,p.elbow_drop,p.hand_size,p.hand_edge,q.enabled?1.0f:0.0f,
        q.settle_time,q.breath_period,q.breath_height,q.shift_period,q.shift_distance,q.blink_min_interval,q.blink_max_interval,
        q.blink_duration,q.lean_distance,q.lean_acceleration,q.lean_settle_time,q.eyelid_height,q.face_colour.r,q.face_colour.g,q.face_colour.b,q.face_colour.a};
    for(const auto& eye:q.eyes){v.insert(v.end(),{eye.x,eye.y,eye.width,eye.height});}
    for(const auto& grip:p.grips){v.insert(v.end(),{grip.rear.x,grip.rear.y,grip.front.x,grip.front.y});}return v;
}
std::vector<float> limb_fill_fixed(const MovementFeedbackPresentation& p)
{
    std::vector<float> v{p.enabled?1.0f:0.0f,p.pixel_size,p.landing_min_speed,p.full_impact_speed,p.minimum_strength,p.keyline_width};
    for(const auto& b:{p.takeoff,p.landing,p.step,p.skid}){v.insert(v.end(),{b.duration,b.spread,b.lift,b.particle_size});}
    for(const auto& c:{p.colour,p.highlight,p.keyline}){v.insert(v.end(),{c.r,c.g,c.b,c.a});}return v;
}
std::vector<float> limb_motion_fixed(const ToolMotionPresentation& p)
{
    std::vector<float> v{p.enabled?1.0f:0.0f,p.shot_pose_split,p.roller_travel_per_turn,p.roller_lift_time,p.roller_settle_time,
        p.hose_travel_per_sway,p.hose_settle_time,p.bucket_travel_per_swing,p.bucket_settle_time,p.mist_tail_duration,p.mist_tail_distance,
        p.mist_tail_size.x,p.mist_tail_size.y,p.mist_tail_opacity};
    for(const auto& t:p.tools){v.insert(v.end(),{t.shot_recovery,t.valve_time});}return v;
}
std::vector<int> limb_preference_values(const PlaySettings& p)
{
    std::vector<int> v{p.aim_sensitivity,p.music_volume,p.sfx_volume,p.view_shake?1:0,p.aim_marker?1:0,p.reduce_effects?1:0};
    for(const auto& s:p.sticks){v.push_back(s.movement_deadzone);v.push_back(s.aim_deadzone);}return v;
}
std::vector<float> limb_input_values(const PlayerInputData& p)
{
    return {p.x_movement,p.jump_pressed?1.0f:0.0f,p.jump_held?1.0f:0.0f,p.shoot_angle,p.shoot_direction.x,p.shoot_direction.y,
        p.shoot_direction_requested?1.0f:0.0f,p.primary_shoot?1.0f:0.0f,p.drop_through?1.0f:0.0f,p.toggle_debug?1.0f:0.0f,
        p.connected?1.0f:0.0f,
        p.left_analog_stick.x,p.left_analog_stick.y,p.right_analog_stick.x,p.right_analog_stick.y};
}
std::vector<float> limb_tool_values(const Weapon& tool)
{
    const auto pos=tool.draw_pos(),nozzle=tool.nozzle_pos();const auto box=tool.draw_bounds();const auto& m=tool.motion_state();
    require(tool.feedback()!=nullptr,"Limb actual builder omitted borrowed weapon feedback");const auto& f=tool.feedback()->state();
    return {static_cast<float>(tool.type()),tool.rotation(),pos.x,pos.y,nozzle.x,nozzle.y,box.x,box.y,box.width,box.height,
        tool.ammo(),tool.ready_to_shoot()?1.0f:0.0f,static_cast<float>(tool.readiness()),tool.shooting_this_update()?1.0f:0.0f,
        static_cast<float>(tool.visual_pose()),static_cast<float>(m.pose),m.pressure,m.valve,m.travel_phase,m.lift,m.compression,
        m.bucket_swing,m.hose_sway,m.mist_tail_remaining,f.muzzle_remaining,f.empty_remaining,f.recovered_remaining};
}
FigurePresentation limb_disabled_look(const FigurePresentation& p){auto copy=p;copy.quiet.enabled=false;return copy;}
class LimbColourProbe
{
public:
    LimbColourProbe(Application& app,const GameTuning& tuning,const Presentation& presentation,const std::filesystem::path& output);
    void unchanged() const;
    Frame actual();Frame adoption(const Colour& arm,const Colour& hand);
    Frame raw(const Colour& arm,const Colour& hand,LimbReference mode=LimbReference::correct);
    void check_rois(const Frame& actual,const Frame& raw,const char* state) const;
    static void check_visible_guards(const Frame& actual,const Frame& baseline,const Colour& arm,bool nondefault);
    static void delta_support(const Frame& actual,const Frame& baseline);
    static void check_control(const Frame& actual,const Frame& correct_raw,const Frame& wrong,LimbReference mode,
        bool alpha_zero_arm,size_t& changed,size_t& unchanged_pixels,size_t& witnesses,size_t& discriminating,size_t& saturated_white);
    static Frame compact(const Frame& frame){return foreground_crop(frame,{0,0,1280,480},0);}
    void failure(const char* state,const char* stage,const Frame& actual,const Frame* other=nullptr,int roi=-1) const;
    size_t readbacks() const{return readbacks_;}size_t calibration_checks() const{return calibration_checks_;}
    size_t preparation_updates() const{return preparation_updates_;}
private:
    Frame render(const std::function<void(DrawList&)>& draw);
    void draw_fixture(DrawList& list,const FigureAttachments& attachment) const;
    void write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint) const;
    Application& app_;const GameTuning& tuning_;const Presentation& presentation_;const FigurePresentation& look_;const MovementFeedbackPresentation& fill_;
    FigurePresentation baseline_,expected_,disabled_look_;MovementFeedbackPresentation baseline_fill_;ToolMotionPresentation baseline_motion_;
    FigureAttachments attachment_,reference_,disabled_;Canvas canvas_;PlaySettings preferences_;
    RenderResources::SpriteSheetHandle fill_sheet_handle_,art_sheet_handle_;const SpriteSheet* fill_sheet_;const SpriteSheet* art_sheet_;
    SpriteSheet::frame_handle fill_handle_;const SpriteFrame& frame_;std::array<SpriteSheet::frame_handle,4> art_handles_{};
    std::array<const SpriteFrame*,4> art_frames_{};std::array<RectangleI,4> art_sources_{};std::array<Vector2F,4> art_origins_{};
    std::array<std::unique_ptr<Weapon>,10> tools_{};std::array<PlayerInputData,10> inputs_{};
    std::array<std::vector<float>,10> tool_values_{},input_values_{};std::array<ToolMotionState,10> motion_values_{};
    std::array<WeaponFeedbackState,10> feedback_values_{};std::array<RectangleF,10> bounds_{};
    const std::filesystem::path output_;std::vector<float> fixed_,disabled_fixed_,fill_fixed_,motion_fixed_;std::vector<int> preference_values_;
    size_t preparation_updates_=0,readbacks_=0,calibration_checks_=0;
};
LimbColourProbe::LimbColourProbe(Application& app,const GameTuning& tuning,const Presentation& presentation,const std::filesystem::path& output):
    app_(app),tuning_(tuning),presentation_(presentation),look_(presentation.figure()),fill_(presentation.movement_feedback()),
    baseline_(look_),expected_(look_),disabled_look_(limb_disabled_look(look_)),baseline_fill_(fill_),baseline_motion_(presentation.tool_motion()),
    attachment_(&fill_,&look_,app.render_resources()),reference_(&fill_,&expected_,app.render_resources()),disabled_(&fill_,&disabled_look_,app.render_resources()),
    canvas_(app.render_resources(),Colour(32,64,96)),fill_sheet_handle_(app.render_resources()->resolve_sprite_sheet(fill_.sprite_sheet)),
    art_sheet_handle_(app.render_resources()->resolve_sprite_sheet(look_.sprite_sheet)),fill_sheet_(app.render_resources()->sprite_sheet(fill_sheet_handle_)),
    art_sheet_(app.render_resources()->sprite_sheet(art_sheet_handle_)),fill_handle_(fill_sheet_->resolve_sprite_frame(fill_.fill_frame)),
    frame_(fill_sheet_->sprite_frame(fill_handle_)),output_(output),fixed_(limb_figure_fixed(look_)),disabled_fixed_(limb_figure_fixed(disabled_look_)),
    fill_fixed_(limb_fill_fixed(fill_)),motion_fixed_(limb_motion_fixed(presentation.tool_motion()))
{
    require(look_.canvas_size==Vector2F(68,120)&&look_.body_inset==8.0f&&look_.rear_shoulder==Vector2F(16,62)&&look_.front_shoulder==Vector2F(52,62)&&
        look_.arm_width==4.0f&&look_.elbow_drop==12.0f&&look_.hand_size==16.0f&&look_.hand_edge==4.0f&&look_.quiet.enabled&&
        look_.quiet.eyes[0]==RectangleF(24,24,4,8)&&look_.quiet.eyes[1]==RectangleF(40,24,4,8)&&look_.quiet.eyelid_height==4.0f&&look_.quiet.face_colour==Colour(255,255,255)&&
        fill_.sprite_sheet=="sprite_sheet_1"&&fill_.fill_frame=="pixel"&&look_.sprite_sheet=="painter"&&
        frame_.source_rectangle()==RectangleI(32,0,4,4)&&frame_.origin()==Vector2F::ZERO,
        "Limb fixture lost shipped source geometry, solid fill or unselected face underlayer");
    const std::array<std::string,4> names{look_.rest_arms_frame,look_.rest_arms_paint_frame,look_.sleeves_frame,look_.sleeves_paint_frame};
    const std::array<RectangleI,4> sources{RectangleI(76,4,68,120),RectangleI(76,516,68,120),RectangleI(148,4,68,120),RectangleI(148,516,68,120)};
    for(size_t i=0;i<names.size();++i)
    {art_handles_[i]=art_sheet_->resolve_sprite_frame(names[i]);art_frames_[i]=&art_sheet_->sprite_frame(art_handles_[i]);
        art_sources_[i]=art_frames_[i]->source_rectangle();art_origins_[i]=art_frames_[i]->origin();
        require(art_sources_[i]==sources[i]&&art_origins_[i]==Vector2F::ZERO,"Limb captured sprite guard metadata changed");}
    preferences_.music_volume=preferences_.sfx_volume=0;preferences_.view_shake=false;preference_values_=limb_preference_values(preferences_);
    std::ignore=limb_masks();
    for(size_t i=0;i<tools_.size();++i)
    {
        const auto& c=limb_contexts()[i];const RectangleF figure=limb_canvas(i);const Vector2F centre=figure.position()+Vector2F(34,60);
        tools_[i]=WeaponBuilder::build_weapon(&tuning_,PlayerTeam::a,0,0,Colour(6,52,204),c.tool,centre,app_.render_resources(),app_.audio_resources(),
            &preferences_,&presentation_.tool_motion(),&presentation_.weapon_feedback());
        auto& input=inputs_[i];input.connected=true;input.shoot_direction_requested=true;input.shoot_angle=c.facing?0.0f:PI;
        input.shoot_direction=Vector2F::unit_vec_from_angle(input.shoot_angle);
        const auto before_motion=tools_[i]->motion_state();const float ammo=tools_[i]->ammo();
        const auto shots=tools_[i]->update_and_get_projectiles(input,centre,Vector2F::ZERO,c.facing,0.0f);++preparation_updates_;
        require(shots.empty()&&tools_[i]->ammo()==ammo&&!tools_[i]->shooting_this_update()&&tools_[i]->motion_state()==before_motion&&
            tools_[i]->type()==c.tool&&tools_[i]->rotation()==c.angle&&tools_[i]->draw_pos()==Vector2F(c.draw_x,c.draw_y),
            "Limb zero-delta nonshoot preparation lost real source pose, paid ammo or advanced mechanism state");
        tool_values_[i]=limb_tool_values(*tools_[i]);input_values_[i]=limb_input_values(inputs_[i]);motion_values_[i]=tools_[i]->motion_state();
        feedback_values_[i]=tools_[i]->feedback()->state();bounds_[i]=attachment_.bounds(figure,c.facing,tools_[i].get());
    }
    require(preparation_updates_==10,"Limb requires exactly ten declared zero-delta Weapon preparations");unchanged();
}
void LimbColourProbe::unchanged() const
{
    auto* resources=app_.render_resources();const auto& motion=presentation_.tool_motion();
    require(&look_==&presentation_.figure()&& &look_.quiet==&presentation_.figure().quiet&& &fill_==&presentation_.movement_feedback()&&
        limb_figure_fixed(look_)==fixed_&&limb_figure_fixed(expected_)==fixed_&&limb_figure_fixed(disabled_look_)==disabled_fixed_&&
        limb_figure_names(look_)==limb_figure_names(baseline_)&&limb_figure_names(expected_)==limb_figure_names(baseline_)&&
        limb_figure_names(disabled_look_)==limb_figure_names(baseline_)&&limb_fill_fixed(fill_)==fill_fixed_&&
        fill_.sprite_sheet==baseline_fill_.sprite_sheet&&fill_.fill_frame==baseline_fill_.fill_frame&&
        limb_motion_fixed(motion)==motion_fixed_&&motion.mist_sheet==baseline_motion_.mist_sheet&&motion.mist_frame==baseline_motion_.mist_frame&&
        limb_preference_values(preferences_)==preference_values_&&preferences_.music_volume==0&&preferences_.sfx_volume==0&&
        !preferences_.reduce_effects&&preparation_updates_==10&&
        app_.viewport_manager()->fullscreen_viewport().pixel_rect()==RectangleI(0,0,1280,720),
        "Limb changed actual/copied unselected Figure/quiet/grip/fill/motion records, captured names, caller preferences or viewport");
    require(fill_sheet_handle_==resources->resolve_sprite_sheet(fill_.sprite_sheet)&&art_sheet_handle_==resources->resolve_sprite_sheet(look_.sprite_sheet)&&
        fill_sheet_==resources->sprite_sheet(fill_sheet_handle_)&&art_sheet_==resources->sprite_sheet(art_sheet_handle_)&&
        fill_handle_==fill_sheet_->resolve_sprite_frame(fill_.fill_frame)&& &frame_==&fill_sheet_->sprite_frame(fill_handle_)&&
        frame_.source_rectangle()==RectangleI(32,0,4,4)&&frame_.origin()==Vector2F::ZERO,
        "Limb changed retained atlas/frame handles, pointers, solid source or origin");
    const std::array<std::string,4> names{look_.rest_arms_frame,look_.rest_arms_paint_frame,look_.sleeves_frame,look_.sleeves_paint_frame};
    for(size_t i=0;i<names.size();++i)
    {require(art_handles_[i]==art_sheet_->resolve_sprite_frame(names[i])&&art_frames_[i]==&art_sheet_->sprite_frame(art_handles_[i])&&
        art_frames_[i]->source_rectangle()==art_sources_[i]&&art_frames_[i]->origin()==art_origins_[i],"Limb captured rest/sleeve frame metadata changed");}
    for(size_t i=0;i<tools_.size();++i)
    {
        const auto& c=limb_contexts()[i];const auto figure=limb_canvas(i);
        require(limb_tool_values(*tools_[i])==tool_values_[i]&&limb_input_values(inputs_[i])==input_values_[i]&&
            tools_[i]->motion_state()==motion_values_[i]&&tools_[i]->feedback()->state()==feedback_values_[i]&&
            attachment_.bounds(figure,c.facing,tools_[i].get())==bounds_[i]&&reference_.bounds(figure,c.facing,tools_[i].get())==bounds_[i]&&
            disabled_.bounds(figure,c.facing,tools_[i].get())==bounds_[i]&&attachment_.bounds(figure,c.facing,nullptr)==figure,
            "Limb draw/command changed actual tool motion/ammo/readiness/pivot/nozzle/input or public attachment bounds");
    }
}
void LimbColourProbe::write_raw(DrawList& list,int left,int top,int right,int bottom,const Colour& tint) const
{
    list.draw_sprite(fill_sheet_->texture(),frame_.source_rectangle(),RectangleF(static_cast<float>(left),static_cast<float>(top),
        static_cast<float>(right-left),static_cast<float>(bottom-top)),tint,0.0f,frame_.origin(),SpriteFlip::none,0.0f);
}
void LimbColourProbe::draw_fixture(DrawList& list,const FigureAttachments& attachment) const
{
    for(size_t i=0;i<tools_.size();++i)
    {const auto& c=limb_contexts()[i];const RectangleF figure=limb_canvas(i);attachment.draw_arms(list,figure,c.facing,*tools_[i]);attachment.draw_hands(list,figure,*tools_[i]);}
    for(size_t i=10;i<12;++i){attachment.draw_blink(list,limb_canvas(i),limb_contexts()[i].facing);}
    disabled_.draw_blink(list,{180.0f,344.0f,68.0f,120.0f},true);disabled_.draw_blink(list,{260.0f,344.0f,68.0f,120.0f},false);
    attachment.draw_rest_arms(list,{400.0f,340.0f,68.0f,120.0f},true,Colour(6,52,204));
    attachment.draw_rest_arms(list,{500.0f,340.0f,68.0f,120.0f},false,Colour(6,52,204));
    attachment.draw_sleeves(list,{600.0f,340.0f,68.0f,120.0f},true,Colour(6,52,204));
    attachment.draw_sleeves(list,{700.0f,340.0f,68.0f,120.0f},false,Colour(6,52,204));
}
Frame LimbColourProbe::render(const std::function<void(DrawList&)>& draw)
{
    unchanged();const std::array<Colour,6> selected{look_.arm_colour,look_.hand_colour,expected_.arm_colour,expected_.hand_colour,disabled_look_.arm_colour,disabled_look_.hand_colour};
    std::srand(41977);const int next=std::rand();std::srand(41977);Renderer& renderer=*app_.renderer();renderer.begin_frame();renderer.set_view_count(1);
    DrawList list=renderer.view(0);list.set_viewport(app_.viewport_manager()->fullscreen_viewport());list.set_camera(Camera::DEFAULT_CAMERA);list.set_filter(TextureFilter::point);
    canvas_.draw(list,{0.0f,0.0f,1280.0f,720.0f});draw(list);
    // Actual WHITE/BLACK texel+coverage calibration stays inside every full-frame
    // guard. No added read and no exemption from repeat/control/reset checks.
    list.set_filter(TextureFilter::point);write_raw(list,20,20,24,24,Colour(255,255,255));write_raw(list,25,20,29,24,Colour(0,0,0));
    Frame result=read(renderer);++readbacks_;
    if(result.width!=1280||result.height!=720){failure("unknown","frame-size",result);throw std::runtime_error("Limb requires1280x720 native backbuffer");}
    bool calibrated=true;
    for(int y=20;y<24;++y){for(int x=20;x<29;++x)
    {if(x==24){continue;}const auto at=limb_offset(x,y);const unsigned char value=x<24?255:0;
        calibrated=calibrated&&result.pixels[at]==value&&result.pixels[at+1]==value&&result.pixels[at+2]==value&&result.pixels[at+3]==255;}}
    if(!calibrated){failure("unknown","WHITE-BLACK-calibration",result);}require(calibrated,"Limb actual32pixel WHITE/BLACK RGBA calibration failed");calibration_checks_+=32;
    const std::array<Colour,6> after{look_.arm_colour,look_.hand_colour,expected_.arm_colour,expected_.hand_colour,disabled_look_.arm_colour,disabled_look_.hand_colour};
    const bool frozen=std::rand()==next&&selected==after;if(!frozen){failure("unknown","draw-state-or-rng",result);}
    require(frozen,"Limb draw consumed gameplay RNG or changed selected actual/copied records");unchanged();return result;
}
Frame LimbColourProbe::actual()
{
    disabled_look_.arm_colour=look_.arm_colour;disabled_look_.hand_colour=look_.hand_colour;
    const auto draw=[&](DrawList& list){draw_fixture(list,attachment_);};Frame first=render(draw);const Frame repeat=render(draw);
    if(first.pixels!=repeat.pixels){failure("unknown","repeat",first,&repeat);}require(first.pixels==repeat.pixels,"Limb repeated frozen fixture changed complete RGBA");return first;
}
Frame LimbColourProbe::adoption(const Colour& arm,const Colour& hand)
{
    expected_.arm_colour=arm;expected_.hand_colour=hand;disabled_look_.arm_colour=arm;disabled_look_.hand_colour=hand;
    return render([&](DrawList& list){draw_fixture(list,reference_);});
}
Frame LimbColourProbe::raw(const Colour& arm,const Colour& hand,LimbReference mode)
{
    const float a=mode==LimbReference::straight_arm?1.0f:mode==LimbReference::squared_arm?arm.a*arm.a:arm.a;
    const float h=mode==LimbReference::straight_hand?1.0f:mode==LimbReference::squared_hand?hand.a*hand.a:hand.a;
    const Colour arm_tint(arm.r*a,arm.g*a,arm.b*a,arm.a),hand_tint(hand.r*h,hand.g*h,hand.b*h,hand.a);
    // Engine-only literal rectangles: no attachment draw, line/fill helper,
    // production premultiply or analytical GPU-precision oracle is reused.
    return render([&](DrawList& list){for(const auto& d:limb_dabs())
    {write_raw(list,d.left,d.top,d.right,d.bottom,d.material==LimbMaterial::arm?arm_tint:d.material==LimbMaterial::hand?hand_tint:Colour(255,255,255));}});
}
void LimbColourProbe::check_rois(const Frame& actual,const Frame& raw,const char* state) const
{
    size_t checked=0;
    // All21142 RGBA/alpha comparisons over all12 complete literal interiors.
    for(size_t i=0;i<limb_contexts().size();++i){const auto& c=limb_contexts()[i];for(int y=c.top;y<c.bottom;++y){for(int x=c.left;x<c.right;++x)
    {const auto at=limb_offset(x,y);const bool equal=same_pixel(actual,raw,at)&&actual.pixels[at+3]==255;
        if(!equal){failure(state,"ROI-RGBA",actual,&raw,static_cast<int>(i));}
        require(equal,"Limb independent ordered literal ROI differs from actual RGBA/opaque alpha");++checked;}}}
    require(checked==21142,"Limb complete independent ROI pixel count changed");
}
void LimbColourProbe::delta_support(const Frame& actual,const Frame& baseline)
{
    const auto& masks=limb_masks();
    for(int y=0;y<720;++y){for(int x=0;x<1280;++x)
    {if(!same_pixel(actual,baseline,limb_offset(x,y))){require(masks.selected[limb_at(x,y)]!=0,"Limb pair delta escaped source selected union or changed full-frame guards");}}}
}
void LimbColourProbe::check_visible_guards(const Frame& actual,const Frame& baseline,const Colour& arm,bool nondefault)
{
    // Active eyelid witnesses remain distinct from WHITE for each declared
    // nonzero arm palette; alpha0 instead exposes the unchanged WHITE face.
    for(const auto& w:limb_witnesses()){if(w.context>=10)
    {require(monitor_rgb(actual,w.x,w.y)==0xFFFFFFu?arm.a==0.0f:arm.a>0.0f,"Limb active lid did not match selected-alpha/face-underlayer state");}}
    for(const int left:{202,282}){for(int y=366;y<378;++y){for(int x=left;x<left+24;++x)
    {const auto at=limb_offset(x,y);require(monitor_rgb(actual,x,y)==0x204060u&&actual.pixels[at+3]==255,"Limb quiet-disabled blink still drew into Canvas");}}}
    for(const int left:{400,500,600,700})
    {
        size_t nonblank=0;for(int y=340;y<460;++y){for(int x=left;x<left+68;++x)
        {const auto at=limb_offset(x,y);require(same_pixel(actual,baseline,at),"Limb edit changed captured rest/sleeve sprite bytes");
            if(monitor_rgb(actual,x,y)!=0x204060u){++nonblank;}}}
        require(nonblank>0,"Limb captured sprite guard baseline drew no native pixels");
    }
    if(nondefault)
    {for(const auto& c:limb_contexts())
        {size_t delta=0;for(int y=c.top;y<c.bottom;++y){for(int x=c.left;x<c.right;++x){if(!same_pixel(actual,baseline,limb_offset(x,y))){++delta;}}}
            require(delta>0,"Limb nondefault pair had no visible change in an applicable direct context");}}
}
void LimbColourProbe::check_control(const Frame& actual,const Frame& correct_raw,const Frame& wrong,LimbReference mode,
    bool alpha_zero_arm,size_t& changed,size_t& unchanged_pixels,size_t& witnesses,size_t& discriminating,size_t& saturated_white)
{
    require(mode!=LimbReference::correct,"Limb correct raw is not a negative control");
    const bool arm=mode==LimbReference::straight_arm||mode==LimbReference::squared_arm;const unsigned char bit=arm?1:2;
    const auto& masks=limb_masks();size_t checked=0,witness_count=0,discriminating_count=0,saturated_white_count=0;
    for(const auto& c:limb_contexts()){for(int y=c.top;y<c.bottom;++y){for(int x=c.left;x<c.right;++x)
    {
        const auto at=limb_offset(x,y);require(wrong.pixels[at+3]==255,"Limb negative changed opaque ROI alpha");
        if(same_pixel(actual,wrong,at)){++unchanged_pixels;}
        else{require((masks.selected[limb_at(x,y)]&bit)!=0,"Limb negative escaped its own field support");++changed;}++checked;
    }}}
    for(const auto& w:limb_witnesses()){if((w.material==LimbMaterial::arm)==arm)
    {
        const auto at=limb_offset(w.x,w.y);
        // At alpha0, straight RGB adds to the opaque WHITE face paint-out and
        // clamps to WHITE under ONE/INV_SRC_ALPHA ADD. These four frozen lids
        // require literal equality, not a false discriminatory inequality.
        if(alpha_zero_arm&&mode==LimbReference::straight_arm&&w.context>=10)
        {
            for(size_t channel=0;channel<4;++channel)
            {require(actual.pixels[at+channel]==255&&wrong.pixels[at+channel]==255,"Limb alpha0 lid control lost mandatory saturated WHITE RGBA");}
            ++saturated_white;++saturated_white_count;
        }
        else
        {require(!same_pixel(actual,wrong,at),"Limb straight/double-base control failed a frozen discriminatory role witness");++discriminating;++discriminating_count;}
        ++witnesses;++witness_count;
    }}
    for(int y=0;y<720;++y){for(int x=0;x<1280;++x)
    {if(!(masks.selected[limb_at(x,y)]&bit)){require(same_pixel(correct_raw,wrong,limb_offset(x,y)),"Limb wrong raw changed exact full-frame exterior/calibration");}}}
    const size_t expected_saturated=alpha_zero_arm&&mode==LimbReference::straight_arm?size_t{4}:size_t{0};
    require(checked==21142&&witness_count==(arm?size_t{64}:size_t{20})&&saturated_white_count==expected_saturated&&
        discriminating_count+saturated_white_count==witness_count,"Limb per-control ROI/discriminatory/saturated-WHITE witness checks differ");
}
void LimbColourProbe::failure(const char* state,const char* stage,const Frame& actual,const Frame* other,int roi) const
{
    const std::string stem=std::string("limb-failure-")+state+"-"+stage;save(actual,output_/(stem+"-actual.ppm"));if(other!=nullptr){save(*other,output_/(stem+"-reference.ppm"));}
    const auto rgba=[&](const Frame& frame,const char* suffix){std::ofstream bytes(output_/(stem+suffix),std::ios::binary);
        bytes.write(reinterpret_cast<const char*>(frame.pixels.data()),static_cast<std::streamsize>(frame.pixels.size()));bytes.close();return !bytes.fail();};
    const bool actual_written=rgba(actual,"-actual.rgba"),other_written=other!=nullptr&&rgba(*other,"-reference.rgba");
    std::ofstream receipt(output_/(stem+".json"));receipt<<"{\"state\":\""<<state<<"\",\"stage\":\""<<stage<<"\",\"readbacks\":"<<readbacks_
        <<",\"ROI_index\":"<<roi<<",\"context\":\"combined-five-tools-two-facings-plus-blink\",\"frame_width\":"<<actual.width<<",\"frame_height\":"<<actual.height
        <<",\"rgba_order\":\"RGBA8-row-major\",\"actual_RGBA_byte_count\":"<<actual.pixels.size()<<",\"actual_written\":"<<(actual_written?"true":"false")
        <<",\"reference_written\":"<<(other_written?"true":"false")<<",\"source_geometry_SHA\":\"2CBD2C63B0EBF4D433A20E3C59A46A2C5808D1F97F25E54A9FA877BC7828EEA4\","
        "\"diagnostic\":\"preserve-source-geometry-and-witness-hypotheses-no-fitting-no-tolerance\"}\n";
}

void limb_colour_consumer_checks(Application& app,const GameTuning& tuning,const Presentation& presentation,const std::filesystem::path& output)
{
    app.viewport_manager()->set_layout(ScreenLayout::one_player);app.resolution_manager()->set_resolution_exactly({1280,720});
    std::ignore=app.renderer()->window_size_changed(1280,720);Presentation local=presentation;auto& look=const_cast<FigurePresentation&>(local.figure());
    require(look.arm_colour==Colour(0,0,0)&&look.hand_colour==Colour(255,255,255),"Limb authored opaque pair changed");
    LimbColourProbe probe(app,tuning,local,output);
    const std::array<Colour,4> arms{Colour(0,0,0),Colour(53,188,225),Colour(32,96,192,128),Colour(165,50,214,0)};
    const std::array<Colour,4> hands{Colour(255,255,255),Colour(225,124,53),Colour(192,64,96,128),Colour(214,90,50,0)};
    constexpr std::array<const char*,4> states{"default","rgb","rgba128","alpha0"};
    Frame baseline{};size_t rows=0,crops=0,controls=0,witnesses=0,discriminating=0,saturated_white=0,changed=0,unchanged_pixels=0;
    std::ofstream records(output/"limb-colour-consumers.csv");
    records<<"state,background,roi_rgba_match,adoption_match,repeat_match,opaque_alpha,active_lids,quiet_disabled_blank,captured_layers_unchanged,delta_support,frozen_resources_inputs_rng,controls,control_witness_checks,control_discriminating_witness_checks,control_saturated_white_checks,control_changed_pixels,control_unchanged_pixels,scope\n";
    for(size_t p=0;p<arms.size();++p)
    {
        look.arm_colour=arms[p];look.hand_colour=hands[p];Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual();adoption=probe.adoption(arms[p],hands[p]);raw=probe.raw(arms[p],hands[p]);
            require(actual.pixels==adoption.pixels,"Limb copied full direct fixture differs; this comparison establishes adoption only");
            probe.check_rois(actual,raw,states[p]);if(p==0){baseline=actual;}
            LimbColourProbe::delta_support(actual,baseline);LimbColourProbe::check_visible_guards(actual,baseline,arms[p],p>0);
            size_t case_controls=0,case_witnesses=0,case_discriminating=0,case_saturated_white=0,case_changed=0,case_unchanged=0;
            const auto control=[&](LimbReference mode,const char* label)
            {
                const Frame wrong=probe.raw(arms[p],hands[p],mode);
                try{LimbColourProbe::check_control(actual,raw,wrong,mode,arms[p].a==0.0f,case_changed,case_unchanged,case_witnesses,case_discriminating,case_saturated_white);}
                catch(...){probe.failure(states[p],label,actual,&wrong);throw;}++case_controls;
            };
            if(p==2)
            {control(LimbReference::straight_arm,"partial-arm-straight");control(LimbReference::squared_arm,"partial-arm-squared");
                control(LimbReference::straight_hand,"partial-hand-straight");control(LimbReference::squared_hand,"partial-hand-squared");}
            else if(p==3){control(LimbReference::straight_arm,"alpha0-arm-straight");control(LimbReference::straight_hand,"alpha0-hand-straight");}
            require(case_changed+case_unchanged==case_controls*21142&&case_witnesses==(p==2?size_t{168}:p==3?size_t{84}:size_t{0})&&case_discriminating==(p==2?size_t{168}:p==3?size_t{80}:size_t{0})&&case_saturated_white==(p==3?size_t{4}:size_t{0}),
                "Limb per-case observed diagnostic/witness accounting differs");
            records<<states[p]<<",canvas32-64-96,true,true,true,true,true,true,true,true,true,"<<case_controls<<','<<case_witnesses<<','<<case_discriminating<<','<<case_saturated_white<<','<<case_changed<<','<<case_unchanged<<",direct-consumer-only\n";
            ++rows;controls+=case_controls;witnesses+=case_witnesses;discriminating+=case_discriminating;saturated_white+=case_saturated_white;changed+=case_changed;unchanged_pixels+=case_unchanged;
            save(LimbColourProbe::compact(actual),output/(std::string("limb-colour-")+states[p]+".ppm"));++crops;
        }
        catch(...)
        {if(!actual.pixels.empty()){probe.failure(states[p],"case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){probe.failure(states[p],"adoption",adoption);}throw;}
    }
    require(records.good()&&probe.readbacks()==22&&probe.calibration_checks()==704&&probe.preparation_updates()==10&&rows==4&&crops==4&&controls==6&&witnesses==252&&discriminating==248&&saturated_white==4&&
        changed>0&&changed<=25536&&unchanged_pixels>=101316&&changed+unchanged_pixels==126852,"Limb shared exact budget or observed diagnostic support bounds differ");
    std::ofstream summary(output/"limb-colour-consumer-summary.json");
    summary<<"{\"scope\":\"direct-consumer-only\",\"raw_reference\":\"12-literal-procedural-ROIs-engine-only\",\"whole_frame_reference\":\"copied-retained-fixture-adoption-only\",\"capture_kind\":\"native-contiguous-fixture-crop\",\"event_fade\":false,\"background\":\"canvas32-64-96\",\"viewport\":\"1280x720\",\"readbacks\":22,\"csv_rows\":4,\"crops\":4,\"crop_width\":1280,\"crop_height\":480,\"crop_pixels\":2457600,\"ppm_bytes\":7372864,\"palettes\":4,\"armed_contexts\":10,\"blink_contexts\":2,\"quiet_disabled_contexts\":2,\"captured_sprite_guards\":4,\"roi_pixels\":21142,\"literal_rectangles\":396,\"base_case_reads\":16,\"actual_reads\":8,\"adoption_reads\":4,\"raw_positive_reads\":4,\"controls\":6,\"partial_arm_straight_reads\":1,\"partial_arm_squared_reads\":1,\"partial_hand_straight_reads\":1,\"partial_hand_squared_reads\":1,\"alpha0_arm_straight_reads\":1,\"alpha0_hand_straight_reads\":1,\"control_witness_checks\":252,\"control_discriminating_witness_checks\":248,\"control_saturated_white_checks\":4,\"calibration_pixels_per_read\":32,\"calibration_pixel_checks\":704,\"preparation_weapon_updates\":10,\"preparation_weapon_dt\":0,\"preparation_projectiles\":0,\"arm_support_pixels\":7232,\"hand_support_pixels\":1280,\"arm_witnesses\":64,\"hand_witnesses\":20,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0,\"control_changed_pixels\":"<<changed<<",\"control_unchanged_pixels\":"<<unchanged_pixels<<"}\n";
    require(summary.good(),"Cannot retain Limb shared summary");
    std::cout<<"PASS limb consumer:22readbacks/4rows/4native1280x480fixture crops;12procedural ROIs21142RGBA/case,6controls252role-checks(248discriminatory+4saturated-WHITE),"
        "704calibrationRGBA;10explicit zero-delta Weapon preparations/zero observation updates; full copied fixture adoption only.\n";
}

#if COLOURWARS_DEV_TOOLS
void limb_tune_colour_checks(Application& app,Registry<LevelLoadedInfo>& levels,GameTuning& tuning,Presentation& presentation,
    const std::filesystem::path& content,const std::filesystem::path& output)
{
    const auto root=output/"limb-colour-tune-scratch",source=root/"source",staged=root/"staged";
    std::filesystem::create_directories(source);std::filesystem::create_directories(staged);
    constexpr std::array<const char*,2> files{"tuning.json","presentation.json"};std::array<std::string,2> real_bytes;
    for(size_t i=0;i<files.size();++i)
    {real_bytes[i]=tune_colour_bytes(content/files[i]);tune_colour_write(source/files[i],real_bytes[i]);tune_colour_write(staged/files[i],real_bytes[i]);}
    const NestedSaveProfile profile(root/"profile");const TuneColourTemp temporary(root/"temp");
    struct RestoreLimb
    {GameTuning& t;Presentation& p;GameTuning old_t;Presentation old_p;~RestoreLimb(){t=old_t;p=old_p;}}
        restore{tuning,presentation,tuning,presentation};
    Save preferences;PlaySettings play;play.music_volume=17;play.sfx_volume=83;play.aim_sensitivity=4;
    play.view_shake=false;play.aim_marker=true;play.reduce_effects=true;play.sticks[2].movement_deadzone=25;play.sticks[2].aim_deadzone=70;
    InputBindings bindings;bindings.keyboard_jump=Key::j;
    require(preferences.set_play_settings_and_save(play)&&preferences.set_input_bindings_and_save(bindings),"Cannot seed Limb private preferences");
    const auto save_path=root/"profile/ColourWars/save_data.json";const std::string saved_preferences=tune_colour_bytes(save_path);
    const auto pref_values=[&]
    {
        auto v=limb_preference_values(preferences.play_settings());v.insert(v.end(),{static_cast<int>(preferences.resolution()),preferences.fullscreen()?1:0,
            static_cast<int>(preferences.team_palette()),static_cast<int>(preferences.input_bindings().keyboard_jump)});return v;
    };
    const auto saved_values=pref_values();StageList stages;
    for(const char* name:{"movement_course","close_quarters","king_of_the_hill","turbulence"})
    {const auto* info=levels.get(name);stages.push_back({name,info->display_name(),info->icon_frame(),info->include_in_random()});}
    GameData data;data.set_application(&app);data.set_save(&preferences);data.set_level_infos(&levels);data.set_stages(&stages);
    data.set_tuning(&tuning);data.set_presentation(&presentation);DebugDirector director(&tuning,&presentation);data.set_debug(&director);
    auto& impl=*director.impl();impl.set_content_directories(source,staged);impl.set_user_directory(root/"user");
    impl.set_script_directories({root/"scripts"});director.attach(&app,&data);impl.log().set_mirror_to_debugger(false);
    require(impl.log().set_transcript((output/"limb-colour-tune-commands.log").string()),"Cannot retain Limb private command transcript");
    LimbColourProbe probe(app,tuning,presentation,output);JsonSpanDocument initial(real_bytes[1],"Limb private source");
    const auto* arm_leaf=initial.find("figure.arm_colour");require(arm_leaf!=nullptr,"Limb source lacks exact existing arm token");
    JsonSpanDocument arm_saved(initial.replaced(*arm_leaf,"\"2060C080\""),"Limb saved arm token");
    const auto* hand_leaf=arm_saved.find("figure.hand_colour");require(hand_leaf!=nullptr,"Limb source lacks exact existing hand token");
    const std::string expected_saved=arm_saved.replaced(*hand_leaf,"\"C0406080\"");bool persisted=false;size_t commands=0,rows=0,named_checks=0;
    const auto unchanged=[&]
    {
        probe.unchanged();require(!impl.can_apply()&&impl.request().empty()&&!impl.in_match()&&
            tune_colour_bytes(save_path)==saved_preferences&&pref_values()==saved_values,"Limb command changed private preferences/default display/JumpJ or queued work");
        for(size_t i=0;i<files.size();++i)
        {
            const auto& expected=i==1&&persisted?expected_saved:real_bytes[i];
            require(tune_colour_bytes(content/files[i])==real_bytes[i]&&tune_colour_bytes(source/files[i])==expected&&tune_colour_bytes(staged/files[i])==expected,
                "Limb command changed actual content/private source/staged bytes beyond exact two quoted tokens");
        }
    };
    const auto command=[&](const std::string& line)
    {
        const size_t before=impl.log().lines().size();std::srand(73043);const int next=std::rand();std::srand(73043);
        const auto status=impl.execute(line);++commands;require(status.ok&&std::rand()==next,"Actual Limb command failed or consumed gameplay RNG");
        if(line.starts_with("tune "))
        {const auto& log=impl.log().lines();require(log.size()>before&&log.back().severity==DebugSeverity::info&&
            log.back().text.starts_with("tune: presentation.figure.")&&log.back().text.ends_with("(live)"),"Limb live command reply differs");}
    };
    Frame initial_raster{},saved_raster{};std::ofstream records(output/"limb-colour-tune.csv");
    records<<"state,background,roi_rgba_match,adoption_match,repeat_match,opaque_alpha,active_lids,quiet_disabled_blank,captured_layers_unchanged,delta_support,frozen_resources_inputs_rng,private_bytes,adoption,named_arm_token_preserved,arm_token,hand_token\n";
    const auto check=[&](const char* state,const Colour& arm,const Colour& hand,const char* arm_token,const char* hand_token)
    {
        unchanged();require(presentation.figure().arm_colour==arm&&presentation.figure().hand_colour==hand,"Limb command parsed wrong pair channels");
        const auto* document=impl.tune_document("presentation");require(document!=nullptr,"Limb live document disappeared");
        const auto& current=document->current();const auto* a=current.find("figure.arm_colour");const auto* h=current.find("figure.hand_colour");
        require(a!=nullptr&&h!=nullptr&&current.token(*a)==std::string("\"")+arm_token+'"'&&current.token(*h)==std::string("\"")+hand_token+'"',
            "Limb live current document did not retain exact pair tokens");
        Frame actual{},adoption{},raw{};
        try
        {
            actual=probe.actual();adoption=probe.adoption(arm,hand);raw=probe.raw(arm,hand);
            require(actual.pixels==adoption.pixels,"Limb copied full fixture adoption differs after actual command");probe.check_rois(actual,raw,state);
            if(std::string_view(state)=="baseline"){initial_raster=actual;}
            LimbColourProbe::delta_support(actual,initial_raster);
            LimbColourProbe::check_visible_guards(actual,initial_raster,arm,std::string_view(state)!="baseline"&&std::string_view(state)!="reset-original");
            if(std::string_view(state)=="reset-original"){require(actual.pixels==initial_raster.pixels,"Limb original reset failed exact whole-fixture RGBA");}
            if(std::string_view(state)=="partial-both"){saved_raster=actual;}
            if(std::string_view(state)=="reset-hand-saved"){require(actual.pixels==saved_raster.pixels,"Limb saved pair reset failed exact whole-fixture RGBA");}
            if(std::string_view(state)=="named-arm")
            {require(document->edits().size()==1&&document->edits().contains("figure.arm_colour")&&document->edits().at("figure.arm_colour").current_token=="\"RED\"",
                "Limb named arm input lost its actual quoted RED ledger token");++named_checks;}
            if(std::string_view(state)=="reset-arm-saved")
            {require(document->edits().size()==1&&document->edits().contains("figure.hand_colour")&&document->edits().at("figure.hand_colour").current_token=="\"D65A3200\"",
                "Limb saved arm reset discarded or changed the sibling hand edit");}
            unchanged();records<<state<<",canvas32-64-96,true,true,true,true,true,true,true,true,true,true,live,"
                <<(std::string_view(state)=="named-arm"?"true":"not-applicable")<<','<<arm_token<<','<<hand_token<<'\n';++rows;
        }
        catch(...)
        {if(!actual.pixels.empty()){probe.failure(state,"debug-case",actual,raw.pixels.empty()?nullptr:&raw);}
            if(!adoption.pixels.empty()){probe.failure(state,"debug-adoption",adoption);}throw;}
    };
    require(presentation.figure().arm_colour==Colour(0,0,0)&&presentation.figure().hand_colour==Colour(255,255,255),"Limb Debug baseline differs from authored pair");
    check("baseline",Colour(0,0,0),Colour(255,255,255),"BLACK","WHITE");
    command("tune presentation.figure.arm_colour RED");check("named-arm",Colour(255,0,0),Colour(255,255,255),"RED","WHITE");
    command("tune.reset presentation.figure.arm_colour");check("reset-original",Colour(0,0,0),Colour(255,255,255),"BLACK","WHITE");
    command("tune presentation.figure.arm_colour 2060C080");check("partial-arm",Colour(32,96,192,128),Colour(255,255,255),"2060C080","WHITE");
    command("tune presentation.figure.hand_colour C0406080");check("partial-both",Colour(32,96,192,128),Colour(192,64,96,128),"2060C080","C0406080");
    const auto* document=impl.tune_document("presentation");require(document!=nullptr&&document->edits().size()==2&&
        document->edits().contains("figure.arm_colour")&&document->edits().contains("figure.hand_colour")&&
        document->edits().at("figure.arm_colour").current_token=="\"2060C080\""&&document->edits().at("figure.hand_colour").current_token=="\"C0406080\"",
        "Limb Save ledger differs from two exact quoted tokens");
    command("tune.save presentation");persisted=true;unchanged();require(impl.tune_document("presentation")->edits().empty(),"Limb two-token Save failed rebase");
    command("tune presentation.figure.arm_colour A532D600");check("zero-arm",Colour(165,50,214,0),Colour(192,64,96,128),"A532D600","C0406080");
    command("tune presentation.figure.hand_colour D65A3200");check("zero-both",Colour(165,50,214,0),Colour(214,90,50,0),"A532D600","D65A3200");
    command("tune.reset presentation.figure.arm_colour");check("reset-arm-saved",Colour(32,96,192,128),Colour(214,90,50,0),"2060C080","D65A3200");
    command("tune.reset presentation.figure.hand_colour");check("reset-hand-saved",Colour(32,96,192,128),Colour(192,64,96,128),"2060C080","C0406080");
    require(impl.tune_document("presentation")->edits().empty()&&impl.tune_document("tuning")->edits().empty(),"Limb final saved pair reset left document edits");
    require(records.good()&&commands==9&&rows==9&&named_checks==1&&probe.readbacks()==36&&probe.calibration_checks()==1152&&probe.preparation_updates()==10,
        "Limb Debug exact commands/states/calibration/preparation/readback budget differs");
    std::ofstream summary(output/"limb-colour-tune-summary.json");
    summary<<"{\"scope\":\"debug-validated-commands\",\"event_fade\":false,\"background\":\"canvas32-64-96\",\"adoption\":\"live\",\"commands\":9,\"states\":9,\"contexts\":12,\"readbacks\":36,\"csv_rows\":9,\"crops\":0,\"saved_tokens\":2,\"private_seed_saves\":2,\"named_arm_token_checks\":1,\"calibration_pixels_per_read\":32,\"calibration_pixel_checks\":1152,\"preparation_weapon_updates\":10,\"preparation_weapon_dt\":0,\"preparation_projectiles\":0,\"local_updates\":0,\"player_updates\":0,\"level_updates\":0,\"scene_updates\":0,\"state_updates\":0,\"input_updates\":0,\"time_updates\":0,\"input_polls\":0,\"observation_weapon_updates\":0}\n";
    require(summary.good(),"Cannot retain Limb Debug summary");
    std::cout<<"PASS limb tune:9real commands/9states/36additional readbacks/9rows; no-raster2quoted-tokenSave/rebase, sibling-retaining saved resets,"
        "2private preferences/JumpJ/defaultdisplay;10zero-delta Weapon preps and0observation updates; direct attachments only.\n";
}
#endif
