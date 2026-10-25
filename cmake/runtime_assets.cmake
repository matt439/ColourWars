# The reviewed runtime content shared by development staging and Release.
# Never copy a content directory wholesale: it may contain private audio,
# editable source art, historical assets and asset-building tools.
set(COLOURWARS_RUNTIME_ASSETS
    manifest.json
    tuning.json
    presentation.json
    fonts/courier_prime_bold_16.spritefont
    fonts/lato_black_24.spritefont
    fonts/lato_black_36.spritefont
    fonts/lato_black_48.spritefont
    fonts/lato_black_72.spritefont
    fonts/lato_black_144.spritefont
    fonts/bowlby_one_144.spritefont
    textures/sprite_sheet_1.dds
    textures/sprite_sheet_1.json
    textures/painter.dds
    textures/painter.json
    textures/menu_paint.dds
    textures/menu_paint.json
    textures/weapon_paint.dds
    textures/weapon_paint.json
    textures/paint_fx.dds
    textures/paint_fx.json
    textures/terrain.dds
    textures/terrain.json
    levels/king_of_the_hill.json
    levels/turbulence.json
    levels/close_quarters.json
    levels/movement_course.json
    levels/crossfeed.json
    levels/press_tower.json
    sounds/paint_exe.json
    sounds/paint_exe.xwb
)

# Only these directories belong to the staging mirror. Root files such as
# executables, DLLs and developer captures must never be removed by staging.
set(COLOURWARS_RUNTIME_DIRECTORIES fonts textures levels sounds)
set(COLOURWARS_OPTIONAL_RUNTIME_ASSETS sounds/paint_exe.xwb)
