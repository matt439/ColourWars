# Dot-sourced by evidence-record.ps1 after its explicit precedence selection.
# Keep these bodies in caller scope: result key order and dynamic inputs matter.
param([string] $PainterArenaScope)

switch -Exact ($PainterArenaScope) {
    'TurbulenceSlabOnly' {
        $result.scope = 'turbulence-slab-only'
        $result.stage = 'turbulence_slab_trial'
        $result.player_counts = @(1, 2, 3, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer', 'sniper', 'mister', 'roller', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('before-descent', 'actual-mid-slab-supported-stop', 'actual-mid-slab-wet', 'actual-finished-level-overview', 'actual-EndMenuInitial', 'six-known-owned-interior-pixels', 'actual-upper-flat-stop', 'controlled-clean-heavy-paint', 'actual-stage-preview', 'native-crop-rgb-parity', 'frozen-local-paint-owner-shadow-state')
        $result.input_slots = @(3, 1, 2, 0)
        $result['trial_source'] = [ordered]@{ path=$trialResolved; sha256_before=$trialHashBefore; sha256_after=$null; unchanged=$false; registration='explicit-argument-only-probe-makes-no-registration-changes' }
        $result.inputs = 'An explicit quoted UTF-16 filesystem path supplies the 24-object fixed-slab source, whether the private trial or registered stage. Ten actual Levels/Scenes use production object/player builders, two real horizontal movers and their moving shadows, plus two static sloping scored slabs and their static shadows. Exactly20 initial placements establish fixed crossover stations and extra3P/4P upper waiting stations. Actual release-braking and bounded analog inputs reach the diagonal midpoint and upper flat stop, with no intermediate placement, jump, drop, respawn or cheat. Controlled projectile contacts establish heavy ownership on every paintable object, including the diagonal faces. Two additional short matches,1P clean and4P heavy, finish from actual source spawns without additional placements. Their real LevelEndInfo supplies Session.finish_round and actual EndMenuInitial construction, rather than synthetic results. Physical-pad and motor acceptance are separate.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Exactly33 native720p full frames,75 native crops,71 gameplay CSV rows plus4 ending CSV rows,67 readbacks and10 actual Levels, with at most8000 updates. All1/2/3/4 clean/heavy fixed-crossover, settled mid-slab and upper-flat stop phases plus isolated actual StagePreview retain actual Scene camera/pane ownership, whole actor bodies and complete diagonal/upper-stop bounds. Midpoints1320/2680 are at rest on the proper slope state with carrier0; feet follow the independently reconstructed actual top edge at the uphill body corner. Frozen draws preserve complete local paint corner/face/team/raw-owner/area, collider, static/moving shadow, view and public state, pixels, RNG and Debug fingerprints. Native RGBA crops and independently retained full-frame RGB rows agree. Four additional heavy-only mid-slab-wet frames follow eight friendly slab contacts, with no new Level, placement, update or paint-owner change. Their pose, geometry, held area and camera match the settled heavy stops. Seven retained pinhole candidates are independently projected into the source slab frame; one is outside the diagonal, while six interior pixels must be owned blue or its black edge in the retained PPM. This covers the four initial wet states and six known interior pixels, not every wet age or diagonal pixel. Two actual finished-Level overviews and two actual EndMenuInitial captures cover1P clean/4P heavy short matches; each retains a complete native frame/crop and actual score/result provenance. Their frozen state, draw RNG, Debug fingerprint and copied RGB remain checked. StagePreview checks authored initial-pose pixels/bounds/RNG only. Trial source hash is unchanged. Native images still need visual inspection for diagonal quantization seams and miniature treatment; no human readability/reaction, physical pads/motors, listening, performance, competitive balance or arena acceptance is established.'
    }
    'TurbulenceShoulderOnly' {
        $result.scope = 'turbulence-shoulder-only'
        $result.stage = 'turbulence_shoulder_trial'
        $result.player_counts = @(1, 2, 3, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer', 'sniper', 'mister', 'roller', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('before-descent', 'actual-mid-ramp-supported-stop', 'actual-upper-flat-stop', 'controlled-clean-heavy-paint', 'actual-stage-preview', 'native-crop-rgb-parity', 'frozen-full-paint-owner-state')
        $result.input_slots = @(3, 1, 2, 0)
        $result['trial_source'] = [ordered]@{ path=$trialResolved; sha256_before=$trialHashBefore; sha256_after=$null; unchanged=$false; registration='private-explicit-argument-only' }
        $result.inputs = 'An explicit quoted UTF-16 filesystem path supplies the isolated26-object shoulder composite. Eight actual Levels/Scenes use production object/player builders and owner-bound moving shadows; public LevelBuilder and longer route/score checks are separate CPU evidence. Exactly20 initial placements put front actors on their own fixed crossover and extra3P/4P actors on their upper approach. Three neutral settle ticks precede capture; actual release-braking and small analog inputs then reach the ramp midpoint and upper stop, without intermediate placement, jump, drop, respawn or cheat. Normal GamepadReader and fake rumble factory remain linked; no physical-pad or motor claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Exactly25 native720p full frames,61 native crops/CSV rows,51 readbacks and8 actual Levels, with at most6000 updates. All1/2/3/4 clean/heavy fixed-crossover, mid-ramp and upper-flat stop phases plus one isolated actual StagePreview retain actual Scene camera/pane ownership, whole actor bodies, whole-body ramp/flat warning patches and the complete shoe underside. Midpoints are1320/2680 within0.04, at rest on the proper ramp state/carrier0; actual feet follow the independent body-edge triangle plane (nominal827). Upper stops1060/2940 retain complete flat support. Clean/heavy public motion matches on every post-placement tick. Controlled contacts establish>99percent held paint. Frozen draws preserve pixels/public/structure state, complete paint corner/face/team/raw-owner/area and full mover/carrier/shadow/view state, RNG and Debug fingerprints. Source RGBA crops and independently retained RGB rows are checked separately. StagePreview checks only authored initial-pose pixels/bounds/RNG; source hash is unchanged. This establishes no human/full-trajectory, ending, readability/reaction-time, physical-pad/motor, listening, performance, balance, arena or moving/rotated scored-slope capability acceptance.'
    }
    'TurbulenceMoverOnly' {
        $result.scope = 'turbulence-mover-only'
        $result.stage = 'turbulence_mover_trial'
        $result.player_counts = @(1, 2, 3, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer', 'sniper', 'mister', 'roller', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('outer-endpoint-wait', 'qualified-inward-commit-window', 'actual-carried-inner-reversal', 'continuous-outward-ride', 'controlled-clean-heavy-paint', 'actual-stage-preview', 'native-crop-rgb-parity', 'frozen-owner-state-rng')
        $result.input_slots = @(3, 1, 2, 0)
        $result['trial_source'] = [ordered]@{ path=$trialResolved; sha256_before=$trialHashBefore; sha256_after=$null; unchanged=$false; registration='private-explicit-argument-only' }
        $result.inputs = 'An explicit UTF-16 filesystem argument supplies the private actual-editor-authored trial. Eight actual Levels own Scenes assembled with production object/player builders and real owner-bound moving shadows; public LevelBuilder loading and driven route/replay qualification are separate CPU evidence. Front riders are placed once, settle for six neutral ticks and remain on actual carriers across the four recorded phases; extra3P/4P actors wait on static upper approaches. Direct ProjectileSpray contacts supply controlled heavy paint. The normal GamepadReader and global fake rumble factory remain linked; no physical-device or motor claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Exactly33 native720p full frames,81 unscaled native crops/CSV rows,67 readbacks and8 actual Level builds, with at most6000 Level updates. Thirty-two actual Level captures cover1P/2P/3P/4P clean/heavy outer wait, qualified inward commit, carried inner reversal and continuing outward ride; one isolated actual local-registry StagePreview adds authored initial-pose context. All panes retain a whole player and standing-body-plus8unit safe upper-approach escape patch. Fixed crossover standing-body-plus80unit landing strips are required only in the CPU-qualified inward window (A right>=1230/B left<=2770, rider localgoals220/100 within8 and personal speed<1); outer endpoints are wait states. Actual Scene view/camera/pane metadata, exact motion/local face-team-raw-owner-area/carrier/shadow/view frozen snapshots, source-bound shadow/cull offsets, retained local credit and matched clean/heavy actor motion are checked. Native RGBA crop parity and independent retained RGB crop equality are distinct proofs. StagePreview checks repeated pixels, widget bounds and RNG only. Trial SHA256 is checked before/after; no asset or Random registration changes. Completed routes, gameplay pressure, replay, human readability/reaction time, physical pads/motors, listening, performance, slope, balance and arena acceptance remain separate gates.'
    }
    'TurbulenceBlockoutOnly' {
        $result.scope = 'turbulence-blockout-only'
        $result.stage = 'turbulence_blockout'
        $result.player_counts = @(1, 2, 3, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('lower-upper-approaches', 'immediate-supported-reversal', 'home-exits', 'controlled-clean-heavy-paint', 'actual-runtime-overview', 'actual-stage-preview', 'native-crop-rgb-parity', 'frozen-state-rng-debug-fingerprint')
        $result.input_slots = @(3, 1, 2, 0)
        $result.inputs = 'Ten actual Levels own Scenes assembled with production object/player builders; the public LevelBuilder loading path is separately qualified by CPU acceptance. Sparse-slot PlayerInputData drives eighteen lower/upper approach ticks after controlled placement and six settle ticks, plus six walking home-exit ticks. Direct ProjectileSpray contacts establish controlled heavy paint. The normal GamepadReader remains linked; no physical-input or completed-route claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Exactly twenty-one native720p full frames, fifty-one unscaled native crops/CSV rows and forty-three readbacks cover 1/2/3/4-player lower/upper approaches, 4P home exits, two actual short-match overviews and one isolated actual StagePreview. Ten Level builds and at most8000 updates; measured heavy paint exceeds99percent. Each gameplay pane keeps its full player, whole-body landing patch and immediate source-supported reversal patch inside its actual Scene camera. Upper reversal and home-ground patches retain an eight-world-unit support cap; eventual lower drops are outside the upper-reversal warning claim. Actual Scene view counts/cameras/viewports, staged patch geometry and full-frame/crop RGB parity are checked. Paired clean/heavy actor motion matches and every gameplay pane has visible paint difference. Repeated Level draws preserve observed public/paint/geometry state, views, pixels, RNG and Debug fingerprints; StagePreview checks pixels, widget bounds and RNG only. Human readability/reaction timing, completed routes, gameplay pressure, balance, movers, slopes, physical pads, listening, performance and arena acceptance remain open.'
    }
    'PressTowerOnly' {
        $result.scope = 'press-tower-only'
        $result.stage = 'press_tower'
        $result.player_counts = @(4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('four-pane-stair-landings', 'four-pane-hoist-exits', 'controlled-clean-heavy-paint', 'native-crop-rgb-parity', 'frozen-state-rng-debug-fingerprint')
        $result.input_slots = @(3, 1, 2, 0)
        $result.inputs = 'Two public builder-owned actual Press Tower Levels, with sparse physical-slot identifiers and neutral PlayerInputData. Authored stair, shoulder and crown-separated summit standing stations are deliberately placed and settled for six ticks after ninety neutral ticks retire countdown art. Direct projectile contacts establish controlled-heavy paint; the normal GamepadReader remains linked. No physical-input, foreground or route-traversal claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Exactly four native720p full frames and sixteen unscaled640x360 player-pane crops/CSV rows inspect stair landings and hoist exits in clean and controlled-heavy paint. Two actual Level builds and eight frozen readbacks use at most1404 neutral updates. Staged rectangles, current player size and camera/pane policy independently reconstruct station, target and viewport receipts; every crop RGB row equals its full-frame pane. Repeated drawing preserves pixels, public/paint state, RNG and Debug replay fingerprint. This is controlled native inspection only: no attended readability, spawn pressure, route feasibility, physical-pad, listening, performance, competitive balance or arena acceptance.'
    }
    'FerryDirectionOnly' {
        $result.scope = 'ferry-direction-only'
        $result.stage = 'turbulence_ferry_trial'
        $result.player_counts = @(1, 2, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer')
        $result.resolutions = @('1280x720')
        $result.actions = @('unpainted', 'a-moving', 'late-b', 'b-moving', 'b-stop', 'tie', 'a-stop', 'actual-stage-preview')
        $result.input_slots = @(3, 1, 2, 0)
        $result.inputs = 'Six manual production Scene/Level fixtures use public object/player builders and actual trial geometry. A-left fixtures change only authored drive headings in an isolated stage copy, with exact restore parity. Controlled projectile contacts and neutral sparse-slot inputs establish seven states. Native isolated views retain actual camera scale and hold a common sign station; miniature views retain the actual Stage Select fit scale and subtract ferry translation for independent shape comparisons. Full contexts use ordinary Level drawing.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Both team-A headings cover unpainted/tied holds, committed movement in either direction, a fresh repaint whose current ownership demand differs from the last committed movement, and both reached endpoints. Native1/2/4P and actual menu-fit miniature paths retain monochrome shape distinctions, same-state base differentials confined to cue bounds, reduced-effects parity, repeated pixels and unchanged public/paint/credit/motion/cue/RNG/Debug fingerprint. Actual StagePreview has an independent nonempty unstepped-sign witness. Exactly43 full contexts,169 unscaled native/monochrome crops,84 CSV rows and424 readbacks; at most5000 Level updates. This is controlled software/raster qualification; attended trial play and human readability remain ROAD#30. No physical-pad, listening, performance, competitive-balance or arena acceptance.'
    }
    'GatePermissionOnly' {
        $result.scope = 'gate-permission-only'
        $result.stage = 'turbulence_gate_trial'
        $result.player_counts = @(1, 2, 4)
        $result.palettes = @('blue_orange', 'pink_green', 'blue_yellow', 'purple_gold', 'red_azure', 'magenta_forest')
        $result.weapons = @('sprayer')
        $result.resolutions = @('1280x720')
        $result.actions = @('unpainted', 'open-a', 'late-b', 'held-a', 'open-b', 'held-a-tie', 'closed-tie', 'actual-stage-preview')
        $result.input_slots = @(3, 1, 2, 0)
        $result.inputs = 'Eight public LevelBuilder Levels use actual trial geometry and production settlement. Neutral sparse-slot PlayerInputData, controlled initial/occupant placements and direct real projectile contacts establish each inspection state. Retained initial native station cameras isolate state-shape comparisons; full contexts use current actual Level views. The miniature fit is computed from actual Stage Select presentation and stage overview bounds.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Seven states cover unpainted, either permitted team, paint arriving after settlement without an occupied hold, occupied enemy takeover, an occupied tie and final tied closure. All six palettes at4P plus blue/orange1P/2P use native-scale and actual menu-fit miniature rendering. Exact configured-ink masks establish shape distinctions without hue; qualified StructurePaintable base draws prove all changed pixels remain inside the plaque, clear of scoring faces. Repeated/reduced-effects draws preserve pixels, public state, score/credit, settled permission/hold, RNG and Debug fingerprint. Actual StagePreview has a nonempty native closed-sign pixel witness. Exactly22 full contexts,155 unscaled native/monochrome crops and112 CSV rows are retained. This establishes software/raster qualification; attended trial play, human readability, physical-pad, listening, performance and competitive balance acceptance remain separate.'
    }
    'SurfaceFinishOnly' {
        $result.scope = 'surface-finish-only'
        $result.Remove('stage')
        $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course', 'crossfeed', 'press_tower')
        $result.player_counts = @(1, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer', 'sniper', 'roller', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('literal-neutral-face-pixels', 'paintable-sealed-rulers', 'buried-and-disabled-faces', 'owned-corner-order', 'all-six-selected-real-levels-clean-heavy', 'frozen-state-rng-debug-fingerprint')
        $result.input_slots = @(3, 1, 2, 0)
        $result.inputs = 'Public builder-owned actual Levels with controlled stations on authored scoring tops, ninety neutral ticks to retire countdown art, controlled placement, six settling ticks and direct projectile contacts for heavy paint. Synthetic fixtures independently exercise per-face finish and corner order. No foreground or physical-device claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Smooth white primer versus sealed hatch: independent thin-ruler, top-only/all-face, buried interval and owned-corner pixels at half/quarter/eighth scale and fractional camera translation in one/four panes. All six selected real levels retain native 720p clean/heavy 1P/4P frames and each unscaled pane. Exactly 24 full frames, 60 pane crops, two synthetic boards and 60 readbacks; public/paint state, RNG, Debug fingerprint and score attribution survive drawing. Controlled station/paint setup establishes no route traversal, human readability, listening or physical-pad acceptance.'
    }
    'ParallaxOnly' {
        $result.scope = 'parallax-comparison-only'
        $result.Remove('stage')
        $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
        $result.player_counts = @(1, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @()
        $result.resolutions = @('1280x720', '1920x1080')
        $result.actions = @('32-clean-heavy-gameplay-camera-cases', 'ordinary-scene-factor-zero-parity', 'independent-raw-atlas-offset-reference', '16-literal-clipped-edges', 'opaque-static-foreground-masks', 'matching-opaque-prop-reference-samples', 'live-reduced-static-parity', 'actual-static-stage-preview', 'resize-return', 'frozen-draw-rng')
        $result.input_slots = @()
        $result.inputs = 'Comparison fixture only: real builder-owned scenery and painted terrain in Scene; PlayerCamera::follow uses the current C2 min(full-window height/1080, physical pane width/1280) gameplay scale for 1P/4P, authored spawns and explicitly placed landmark-near focus points with authored camera bounds. Independent panes, normal reader, no synthetic native activation or input. Placed points establish no route reachability.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Bounded static/A/B current-arena prototype only. A moves far sun/clouds/birds by2percent horizontal camera-centre lag capped32worldunits; B adds one shared1percent hill lag capped16, both snapped in4worldunits about each unchanged overview centre. Actual objects draw with local per-view cameras and matching culls; static props/shadows/ordinary terrain and paint restore the original camera. Ordinary Scene/factor0 parity precedes independent unculled raw-atlas/destination comparisons; frozen cloud phase is read through rectangle(), while isolated nondrifting Sun/hill edges use literal sources/displacements and negative/fractional origins outside the ordinary cull.32 clean/heavy cases, measured>99percent real paint ownership, opaque black/white foreground and matching prop-reference samples,4P independence, resize/back, Reduce effects plus cloud-anchor parity, neutral anchor and actual unchanged StagePreview.372 readbacks and exactly8 native1536x720 contact sheets, each512x360 cells: columns static/A/B, rows1Pspawn/4Pplaced-focus. Conservative far/hill cull-candidate counts do not imply raster contribution. Per-view CSV rows repeat framewide A/static and B/A deltas; count totals increment once per primary case. Far absence in gameplay panes is recorded, not hidden by changing zoom. No shipped parallax, new art/levels/schema, live-match transitions, route, human style/readability, physical-device, listening or performance acceptance.'
    }
    'LandmarkOnly' {
        $result.scope = 'static-landmarks-only'
        $result.Remove('stage')
        $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
        $result.player_counts = @(1, 4)
        $result.palettes = @('blue_orange', 'blue_yellow', 'pink_green')
        $result.weapons = @('roller')
        $result.resolutions = @('1280x720', '1920x1080')
        $result.actions = @('48-clean-heavy-pairs-96-primary-states', 'authored-overview-pane-framing', 'measured-heavy-owned-paint', 'positive-each-landmark-pixels', 'independent-raw-atlas-reference', 'terrain-paint-occlusion-order', 'native-scene-edge-parity', 'static-reduce-effects-update', 'actual-stage-preview', 'actual-level-monitor-overview', 'frozen-state-rng-debug-fingerprint', 'resize-return')
        $result.input_slots = @(3, 0, 1, 2)
        $result.inputs = 'Static actual LevelObjectBuilder/Scene geometry uses one/four pane layouts with unchanged authored overview cameras; it does not claim player gameplay framing. Supplemental actual LevelBuilder solo/four-player stories use neutral sparse PlayerInputData and private scratch profiles. Ordinary GamepadReader backend, no foreground/global input or physical-pad claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Exactly48 clean/heavy paired comparisons /96 primary coverage states across4 arenas x3 palettes x1P/4P pane layouts x720/1080. Heavy coverage is actual alternating-team projectile contact on every StructurePaintable, measured above99percent; each prop has positive surviving pixels. Literal native placement/WHITE/Visual/nonpaintable checks and raw atlas-source draws independently replace only the two scenery calls before actual terrain/paint, proving pixels, occlusion order and culling. Frozen drawing preserves geometry/paint attribution/RNG and native resize returns. Supplemental64 native clipped-edge cases compare actual Scene and miniature traversal to unculled raw atlas, with static update/Reduce effects silhouette invariance. Eight actual StagePreview size cases compare independently specified physical panes/cameras;24 actual Level monitor-preview/overview size cases compare identically seeded twins with a private stage copy omitting exactly the two props, preserving gameplay observations and frozen Debug replay fingerprints. Solo bypasses the monitor. All reference/repeat readbacks and paired story ticks are counted separately. Exactly8 compact native clean/heavy crops total,48-row primary CSV and bounded integration CSV; no broad older terrain matrix, full match gameplay route, human readability, listening, physical-device or performance acceptance.'
    }
    'RouteWarningOnly' {
          $result.scope = 'route-warning-only'
          $result.Remove('stage')
          $result.stages = @('crossfeed', 'movement_course')
          $result.player_counts = @(1, 2, 3, 4)
          $result.palettes = @('blue_orange')
          $result.weapons = @('sprayer', 'sniper', 'mister', 'bucket')
          $result.resolutions = @('1280x720')
          $result.actions = @('continuous-landing-interval-observations', 'actual-moving-approaches', 'single-paid-launch', 'post-tick-projectile-candidates', 'first-damage', 'selected-native-captures', 'frozen-state-rng')
          $result.input_slots = @(3, 1, 2, 0)
          $result.inputs = 'Actual Level/Player/Scene fixtures constructed through public builders. Controlled initial stations, fixed launch inputs and direct projectile heavy-paint contacts are labeled; all later movement and launch flight use ordinary per-slot PlayerInputData and the real simulation update order. Simulated 60 Hz ticks establish no physical-input or display-latency claim.'
          foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
          $result.checks = 'Focused native720p route/warning evidence with continuous post-Level-update observations and selected frozen captures: Crossfeed moving approaches and revised Movement Course tap/held/running landings in sparse 1/2/3/4-player layouts, clean and controlled-heavy states, plus representative single-paid-burst incoming aimed tools. Body-width landing strips and useful near edges are observed relative to actual commitment/contact; actor presence, projectile collision/cull candidates, HUD overlap and first damage are separate clocks. Bounds entering a pane do not establish surviving on-screen projectile pixels or human readability. No-hit cases are censored; launch/contact retirement can leave zero drawable frame opportunities. At most40 full frames,100 native crops,15000 Level updates and25000 observation rows; readbacks equal twice the full-frame count. Frozen drawing preserves pixels, observed state, draw RNG and Debug replay fingerprints. No other painter matrices, attended play, physical pads, listening, display/input latency, reaction-time or competitive-balance acceptance.'
          $result.checks += ' Mister first-candidate timing is the earliest geometric candidate among five pellets from one paid burst; it does not identify the damaging pellet or prove its warning duration. Native crop RGB bytes must match the corresponding full-frame pane exactly.'

    }
    'CrossfeedOnly' {
          $result.scope = 'crossfeed-only'
          $result.stage = 'crossfeed'
          $result.player_counts = @(1, 2, 3, 4)
          $result.palettes = @('blue_orange')
          $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
          $result.resolutions = @('1280x720')
          $result.actions = @('clean-heavy-approach', 'moving-camera-context', 'home-exits', 'runtime-overview', 'actual-stage-preview', 'frozen-state-rng')
          $result.input_slots = @(3, 1, 2, 0)
          $result.inputs = 'Actual Level/Player fixtures constructed through public builders; controlled initial stations and direct projectile heavy-paint contacts are labeled separately from real scripted movement. Ordinary per-slot PlayerInputData; no physical-input claim.'
          foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
          $result.checks = 'Twenty-one full native720p frames, 51 pane crops/CSV rows and 43 readbacks: clean/heavy 1/2/3/4-player approaches before/after movement, clean/heavy four-player home exits, actual solo runtime overviews and actual StagePreview. Measured heavy ownership exceeds99percent initially. Frozen pixels/public state/RNG and Debug replay fingerprint remain unchanged by drawing. Target visibility is recorded as an observation, not asserted human readability. Route traversal and weapon flight are separately qualified by CPU acceptance; attended matches, balance, physical pads and listening remain open.'

    }
    'CqBlockoutOnly' {
        $result.scope = 'cq-blockout-only'
        $result.stage = 'close_quarters'
        $result.player_counts = @(1, 4)
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer', 'sniper', 'roller', 'mister', 'bucket')
        $result.resolutions = @('1280x720')
        $result.actions = @('main-contest', 'lower-bypass', 'distributed-four-pane', 'runtime-overview', 'actual-stage-preview', 'frozen-state-rng')
        $result.input_slots = @(3, 1, 2, 0)
        $result.inputs = 'Actual builder-owned CQ Levels with players deliberately placed on authored route surfaces for composition inspection; neutral scripted per-slot input and ordinary GamepadReader. Route traversal is checked separately by cq_blockout CPU acceptance.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Five bounded native720p frames: solo main and lower routes, four mixed-tool players distributed across the loop and home, actual short-match runtime overview, and actual StagePreview miniature. Full frames and unscaled native crops accompany camera/position CSV receipts. Repeated frozen rendering preserves pixels, public simulation, draw RNG and Debug replay fingerprint. This establishes no attended play, human readability, physical-pad, listening or competitive balance acceptance.'
    }
    'MiniatureTerrainOnly' {
        $result.scope = 'miniature-terrain-only'
        $result.Remove('stage')
        $result.player_counts = @(1, 4)
        $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course', 'crossfeed', 'press_tower')
        $result.palettes = @('blue_orange')
        $result.weapons = @('roller')
        $result.actions = @('independent-edge-tick-pixels', 'masked-seam', 'lit-thin-ruler', 'ramp-silhouette', 'owned-paint-overlay', 'legacy-comparison', 'native-material-parity', 'native-scene-order-culling', 'opening-preview-push', 'active-pullback-overview', 'resize-return', 'bounded-active-camera-normalization', 'frozen-state-rng', 'actual-level-path-mode-transitions', 'low-scale-active-counterexample')
        $result.input_slots = @(3, 1, 2, 0)
        $result.inputs = 'Direct material/Structure and actual builder-owned Level draws with neutral sparse PlayerInputData. Normal GamepadReader backend; no physical-device claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Independent complete exposed-edge/masked-top-span/body pixel oracle and actual major-ruler tick/light-gap, one-pixel sealed ruler and original filled ramp silhouette checks. Legacy and adapted material comparisons at 1P/4P720p/1080p/QHD; exact point-sampled material equality at one cell per pixel and larger. Actual projectile-owned paint remains above terrain without changing score or attribution. All six selected stage overview compositions compare ordinary and path-selected miniature Scene rasters against the independent legacy traversal, including culling and native-scale order parity. Actual sparse-roster solo and four-player opening/preview/push/active/pullback/overview contexts in all six selected stages at all three native sizes preserve frozen public observations, RNG and Debug replay fingerprints, with exact resize returns. The measured Movement Course active deadzone cameras may differ by at most four float ULPs in Y after resize; unchanged scale/X and matching view/player cameras in each pane gate a temporary camera normalization, full-frame byte equality and exact restoration, recorded separately. A separate real four-player Crossfeed Level checks constructor/zero-dt and story-state path flags. Its 320x480 active panes remain ordinary at 0.125 camera scale (160-pixel pane width, with min(480/1080, 160/1280) = 0.125); the source-derived floor-bottom witness at local (32,211) is metal_dark 6E7275 ordinarily and keyline 000000 under a deliberately forced miniature-mode control, and restoring through zero-dt update must recover exact pixels. Three additional native readbacks and eleven path-state rows are reported separately. Selected native detail/context crops accompany compact comparison CSV; no video or full painter matrix. Muted rendering under possible concurrent Unreal validation does not establish human readability/style, physical-pad, listening or performance acceptance.'
    }
    'DriftOnly' {
        $result.scope = 'cloud-drift-only'
        $result.player_counts = @(1, 4)
        $result.stages = @('close_quarters', 'king_of_the_hill', 'turbulence', 'movement_course')
        $result.palettes = @('blue_orange')
        $result.weapons = @('sprayer')
        $result.actions = @('cloud-anchor', 'cloud-right', 'cloud-left', 'cloud-loop', 'scene-edge-culling', 'reduce-effects', 'nonpositive-nonfinite-time', 'frozen-redraw', 'draw-update-rng', 'resize-round-trip')
        $result.input_slots = @(3, 0, 1, 2)
        $result.inputs = 'Real DriftingVisual in Scene with ordinary render fan-out and direct unculled reference; neutral scripted per-slot PlayerInputData for real arena context captures. Normal GamepadReader backend; no physical-device claim.'
        foreach ($key in @($result.Keys | Where-Object { $_ -like '*_checks' })) { $result.Remove($key) }
        $result.checks = 'Native cloud_big art in real Scene at 1P/4P 720p/1080p/QHD: anchor exactly matches static Visual, both 48-unit extremes visibly differ, period closes exactly, all changed pixels lie inside projected cull bounds. Both edges compare Scene output against direct unculled drawing while the original anchor lies wholly outside the view. Immediate applied Reduce effects restores the anchor and freezes the phase; switching back restores the same displaced raster. Nonpositive/nonfinite time, repeated frozen drawing and draw/update RNG remain unchanged. Exact 720p resize round trip. Actual builder-owned solo contexts from all four arenas and a 4P1080 Close Quarters sky composition with explicitly placed players preserves frozen public simulation and Debug fingerprints. The 4P composition makes no gameplay-route or jump-reachability claim. Muted hardware rendering; human motion/readability, listening and performance acceptance remain separate.'
    }
    default { throw "Unknown painter arena evidence scope: $PainterArenaScope" }
}
