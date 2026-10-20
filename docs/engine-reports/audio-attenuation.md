# Request: shared music/SFX attenuation for waves and persistent effects

Local **ENG#6** draft. **Unfiled.** Use Labrador's Engine request form when
publishing; this document records no upstream request or implementation.

## Need and exhibit

A client has shared Music and SFX volume preferences. It combines each user
gain with the authored level at every emitter, then reapplies that product to
each retained effect after preferences change or playback resumes. Independent
emitters repeat the same user-gain policy. Both logical groups can share one
bank, so a bank-wide multiplier alone would not distinguish Music from SFX.

The [standalone synthetic exhibit](../../scripts/engine-reports/audio_gain_exhibit.cpp)
reduces this existing workaround to two looping effects and two independent
one-shot emitters in one bank. It uses Labrador's existing null AudioDevice
implementation, SoundBank/AudioResources and synthetic wave names only. No game
objects, assets, loaders, Application, window or physical audio device are used.

The emitter helpers multiply authored volume by the supplied SFX preference.
The two retained effects also require separate volume setters when preferences
change. A raw-emitter negative control omits that multiplication and therefore
records its ordinary local volume. This is an extraction request, not a defect
in the documented per-call volume semantics.

## Checked revision and current boundary

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC**:
`52e809a5a0229d37db4373faa7853df4c861a310`. The entire local engine worktree was
checked against that `FETCH_HEAD` at 19:50:08 UTC, with no differences, before
the two standalone runs.

At that revision, `engine/audio/audio_device.h` exposes per-call and per-voice
volume, with no master or logical-group gain control. `SoundBank::play_wave`
returns void; `play_effect` and `set_effect_volume` target a named persistent
effect. `AudioResources` is a bank registry. Application exposes that registry
but keeps its AudioDevice private; exposing the device alone would not add a
gain operation absent from the seam itself.

## Reproduce

The example was compiled directly from five translation units with MSVC
**19.51.36256.0**, C++20, `/EHsc /W4 /WX /permissive- /Zc:__cplusplus /MP1`.
Debug used `/MDd /Od /D_DEBUG`; Release used `/MD /O2 /DNDEBUG`.

The exact source set is the exhibit plus:

```text
engine/audio/sound_bank.cpp
engine/audio/audio_resources.cpp
engine/audio/null/audio_device.cpp
engine/math/scalar.cpp
```

No prebuilt engine library was linked. The null implementation supplies the
AudioDevice symbols; scalar.cpp supplies SoundBank's non-inline clamp. Registry
and Handle are header-only. No DirectXTK/XAudio2, doctest, renderer, assets,
vcpkg dependency or engine CMake configuration is needed by this reduced target.

From an x64 VS Developer PowerShell at this checkout's root, these commands
reproduce the same compiler flags and sources. Output paths differ from the
retained runs; the exhibit bytes are identical. A separate Labrador checkout
can substitute its repository root for `$engineSource`.

```powershell
$exhibitSource = [IO.Path]::GetFullPath('scripts/engine-reports/audio_gain_exhibit.cpp')
$engineSource = [IO.Path]::GetFullPath('external/labrador')
$exhibitOutput = [IO.Path]::GetFullPath('out/validation/audio-attenuation')
$engineSources = @(
    "$engineSource/engine/audio/sound_bank.cpp",
    "$engineSource/engine/audio/audio_resources.cpp",
    "$engineSource/engine/audio/null/audio_device.cpp",
    "$engineSource/engine/math/scalar.cpp"
)
foreach ($configuration in 'debug', 'release') {
    $directory = Join-Path $exhibitOutput $configuration
    New-Item -ItemType Directory -Force $directory | Out-Null
    $executable = Join-Path $directory 'audio_gain_exhibit.exe'
    $arguments = @('/nologo', '/std:c++20', '/EHsc', '/W4', '/WX', '/permissive-',
        '/Zc:__cplusplus', '/MP1', ('/I' + $engineSource), ('/Fe' + $executable))
    $arguments += if ($configuration -eq 'debug') { '/MDd', '/Od', '/D_DEBUG' }
        else { '/MD', '/O2', '/DNDEBUG' }
    Push-Location -LiteralPath $directory
    try {
        & cl.exe @arguments $exhibitSource @engineSources
        if ($LASTEXITCODE -ne 0) { throw "$configuration compile failed" }
        & $executable
        if ($LASTEXITCODE -ne 0) { throw "$configuration exhibit failed" }
    }
    finally { Pop-Location }
}
```

**Actual and expected:** both retained runs exited 0 with empty stderr and:

```text
0,0.800
1,0.400
2,0.600
3,0.900
4,0.200
5,0.200
6,0.300
7,0.450
8,0.900
PASS synthetic per-emitter gain exhibit: 9 recorded commands; no hardware, audio clock or listening claim
```

Rows 0-3 are initial requests. Rows 4-5 refresh the two retained effects
individually after user gains become Music 0.25/SFX 0.5. Rows 6-7 are future
one-shots whose independent emitters repeat the SFX multiplication. Row8 is the
raw-emitter control, recording 0.9 rather than the user-adjusted 0.45.
Assertions independently check all nine kinds/volumes, voice targeting, loop
flags, wave selection and unchanged pitch/pan. They remain active in Release.

The tested source SHA-256 is
`E8443588229A949DC15FD99D4A42A8BADDB0111A2B61973D3B71350C554A3798`.

## Meaning and requested mechanism

The null backend records what the engine was asked to play. It has no audio
clock or physical mixer and does not model automatic sound completion. These
checks establish the requests and duplicated client arithmetic; they do not
measure loudness, overlap, latency, completion or an already-playing one-shot's
audibility. The returned void type establishes the absence of a retained
per-play handle, without pretending that the recording measures live sound.

Consider shared attenuation that can express at least Music and SFX user gains
across wave and persistent-effect paths, independently of authored per-sound
levels. A bounded master/group facility, its exact API and whether this amount
of repeated code warrants extraction remain maintainer decisions. The client
would retain group assignment, authored levels, fades and pause ducking.

**T1** applies: extract a shared mechanism from an existing client's repeated
policy while keeping the facility limited to that demonstrated need. The earlier
local report also mentioned listener models and voice budgets; neither is
demonstrated by this exhibit, and both are outside this fileable slice.

No listening, physical-device, human comfort/readability or performance
acceptance is claimed. Refresh upstream master before filing.
