# Debug shader playground

The playground puts three game-authored shaders over ColourWars' picture:
posterised colour, ink edges and halftone dots. Choose a look and adjust its
strength in the developer console. It is an opt-in Debug experiment for
writing and comparing shaders; no player setting is saved for it.

ColourWars already pins Labrador's custom-material API. The experiment uses
one HLSL post pass in
[game/shaders/shader_playground.hlsl](../game/shaders/shader_playground.hlsl).
It processes the whole picture, including menus, HUD text and split-pane
borders. The edge detector sees changes in the picture; it has no knowledge
of players, paint or collision geometry.

## Build and launch

From the repository root, build Debug with the existing build helper:

```powershell
powershell -File scripts/build.ps1 -Preset x64-debug
```

The normal Windows D3D11 build uses the Windows SDK's existing `fxc` shader
compiler. No additional shader tools are needed. The material is compiled
only for Debug and embedded in the executable; there is no runtime shader
file to stage beside it.

Launch the playground from PowerShell:

```powershell
$previousShaderPlayground = $env:COLOURWARS_SHADER_PLAYGROUND
try {
    $env:COLOURWARS_SHADER_PLAYGROUND = '1'
    Start-Process -FilePath (Resolve-Path 'out/build/x64-debug/game/ColourWars.exe').Path `
        -ArgumentList '+stage movement_course +intro off +shader posterise .75 6' -Wait
}
finally {
    $env:COLOURWARS_SHADER_PLAYGROUND = $previousShaderPlayground
}
```

Only the exact value `COLOURWARS_SHADER_PLAYGROUND=1` enables it. The launch
above starts the Movement Course without its monitor opening. Leave out the
arguments to start at the title and compare the menus instead. Add
`+players 4` to inspect the effect across four panes.

This mode disables CRT for the session, even when CRT is saved On or
`COLOURWARS_CRT_TRIAL` requests it. Labrador currently selects one post
material, so CRT and the playground cannot run together. The saved CRT
preference is untouched. Normal launches retain their existing rendering;
Release ignores the playground environment variable.

## Change the picture

Open the console with **`**, enter a command, then close it to see the
picture. Commands also work as launch statements, as in the example above.
See [the console reference](debug-console.md) for scripts, screenshots and
key binds.

The **F10 > view > shader** picker also selects a look with its default values.

| Command | Result |
|---|---|
| `shader` | Report the current look and parameters. |
| `shader off` | Show the original picture through the identity mode. |
| `shader posterise` | Posterised colour, strength `0.75`, six levels per colour channel. |
| `shader posterise 0.5 4` | Blend halfway toward four colour levels per channel. |
| `shader ink` | Darken image edges, strength `0.65`, one output pixel radius. |
| `shader ink 0.8 2` | Stronger ink with a two output pixel sampling radius. |
| `shader halftone` | Add halftone dots, strength `0.65`, six output pixel cells. |
| `shader halftone 0.4 10` | Lighter dots in larger cells. |

Each look accepts optional **strength**, then **detail**. Strength is `0..1`:
zero returns the original picture and one applies the full look. Detail is a
whole number: `2..16` levels for posterisation, `1..4` output pixels for ink radius, or
`3..24` output pixels for halftone cell size. Selecting a look resets its
omitted parameters to that look's defaults.

The material stays selected for the whole playground launch. `shader off`
returns the original pixels but retains the post pass and its small rendering
overhead. Restart without the environment variable for ordinary rendering.

## Write another look

Edit [shader_playground.hlsl](../game/shaders/shader_playground.hlsl), rebuild
Debug and restart the game. Shader source changes have no hot reload.
Parameter changes through `shader` apply without a rebuild or restart.

Labrador supplies `MaterialInput`, `scene_point(uv)` and `scene_linear(uv)`.
`input.uv` is the top-down position across the output, `input.output_pixel`
is its position in output pixels, and `scene_size.zw` is one input texel in
UV coordinates. Keep sampling through the helpers so the engine handles
texture orientation for each backend.

The shader reads `material_value(0)` as:

| Component | Meaning |
|---|---|
| `x` | Mode: `0` identity, `1` posterise, `2` ink, `3` halftone. |
| `y` | Strength. |
| `z` | Detail for the chosen mode. |
| `w` | Unused. |

The API provides eight groups of four floats through `PostValues`; the
playground currently uses only this first group. Preserve the scene's
premultiplied alpha when changing colour, keep denominators positive and
send finite parameter values. A look that needs time must receive it in a
parameter; the reserved frame timing fields do not supply a clock.

The engine's
[authoring guide](../external/labrador/website/src/content/docs/docs/guides/your-shader.mdx)
explains `labrador_add_material`, generated material headers, registration
and parameter updates. [game/CMakeLists.txt](../game/CMakeLists.txt) owns this
experiment's build registration. Custom sprite shaders, additional game
textures, multiple post passes and bloom need capabilities beyond this API.

## Validation and visual review

Validated locally on 2026-11-18:

- Debug and Release builds passed, as did the `debug_console` acceptance case
  in both presets, including the opt-in guard and absence from Release.
- Seven hidden, isolated D3D11 runs on the NVIDIA GeForce RTX 5080 passed
  25 checks across one-player, four-player and menu captures at 1280x720.
  Each look changed the image; every zero-strength look and `shader off`
  matched baseline RGB pixels exactly. One- and four-player `off` also
  matched separate ordinary no-post launches. Paused simulation checksums
  stayed unchanged; invalid strength and detail left settings and pixels
  unchanged. The PNG writer discards alpha, so captures do not qualify it.
- The playground ran with both saved CRT On and `COLOURWARS_CRT_TRIAL=1`,
  confirming the session override. Documentation links and whitespace passed.

Local captures, scripts and comparison results are retained under
`out/validation/shader-playground/`. This is focused functional evidence, not
a performance measurement or an art-direction acceptance. The author still
needs to judge team colour recognition, small text and the look in play,
particularly in four panes. These settings remain experimental values.
