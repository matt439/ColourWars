# Paint.exe audio production set

Produced on 2026-10-14 for **POL#22**. Open [index.html](index.html) in a browser
to hear individual cues and the same scripted scene at one, two and four players.
Start with Tier 1. There are **103 WAV masters covering all 85 cue groups** in
[audio-inventory §4](../../../docs/audio-inventory.md#4-the-new-cue-list):
49 Tier 1 waves, 37 Tier 2 waves and 17 Tier 3 waves.

**The author accepted the complete set on 2026-10-14**, replying "sounds good"
to the explicit request to accept it for POL#23 integration or name revisions.
POL#22 is closed; [acceptance.json](acceptance.json) records the exact 103 accepted
master hashes. No revisions were requested.
The 2026-10-15 runtime migration uses this set for the existing game bindings and
includes all 103 masters in the live bank. POL#23 retains new cue hooks and the
gameplay mix. ROAD#3 still owns actual multiplayer, pause/resume, volume and
release acceptance.

## What is here

- [music/PRODUCTION.md](music/PRODUCTION.md): four original acoustic arrangements
  (menu and the three arenas), three corresponding ending stings and a results
  coda. Ukulele, upright piano, cajon, bongos, shakers, claves and hand claps use
  recorded CC0 samples. The 76–87 second loops have contrasting sections and
  editable note/event records. No existing song, legacy track or synthesizer
  preset was used.
- [foley/README.md](foley/README.md): 64 physical cues from marker, paper, water,
  mechanisms and room recordings. Includes six footsteps, six splashes, four
  landings and four painted-step variants. The pop, sigh and chair creak are
  accepted sourced cues for STORY#1/STORY#2, not recordings by the repository author.
- [interface/project.json](interface/project.json): 31 short arrangements of
  recorded mouse switches, paper, instrument body taps, claps and ukulele notes.
  UI cues belong to the invented program; no real operating-system sound is used.
- [sources.json](sources.json): 71 retained source recordings, exact source URLs,
  creators, licence/redistribution fields and SHA-256 hashes. Some sources serve
  several cues; every retained recording is used.
- [PROVENANCE.md](PROVENANCE.md): a separate provenance record for every WAV.
  [cues.json](cues.json) is the complete mapping from cue to master and sources.
- [paint_exe.json](paint_exe.json): the production wave inventory beside the
  provenance record. It deliberately has no runtime effect instances; the live
  [definition](../../content/sounds/paint_exe.json) allocates explicit
  pause-aware music, weapon, per-player and transition voices.

Sources remain in their downloaded formats. Most are lossless WAV; the chair
creak is explicitly a CC0 high-quality MP3 preview, not a lossless original.
Its original WAV requires a Freesound login. This source choice is visible in
the foley record and included in the accepted set.

## Reproduce

Python 3.11+ with the versions in [requirements.txt](requirements.txt), FFmpeg
on PATH, and the repository's existing XWBTool are required. No network access,
DAW, game build, graphics device or audio device is needed to export/validate.
Run these from the repository root:

```powershell
python -m pip install -r game/audio/paint_exe/requirements.txt
python game/audio/paint_exe/music/render.py
python game/audio/paint_exe/foley/build.py
python game/audio/paint_exe/interface/render.py
python scripts/audio/prepare_review.py
python scripts/audio/audio_pipeline.py build
python scripts/audio/publish_runtime_bank.py
python -m unittest discover -s scripts/audio -p test_audio_pipeline.py
python -m unittest discover -s scripts/audio -p test_runtime_bank.py
```

The JSON scores/recipes are the editable production sessions. Renderers retain
the unchanged original recordings and write derivative masters only inside
their own subtree. `prepare_review.py` assembles the catalogue/provenance and
creates the listening page and its Ogg previews. Ogg is for browser audition
only, never a bank input. The exact preview events and fixed shared gain are in
[auditions/timeline.json](auditions/timeline.json). All four seats remain centred,
as with the current shared-speaker game; there is no attenuation as seats are
added. These are offline arrangements, not captured gameplay or mix acceptance.

The bank builder writes to `out/audio-production/`. It verifies original
and derivative hashes, coverage, source permissions, PCM formats, peaks, silence,
loop regions and the named XWB's actual samples/metadata. It never reuses the
legacy bank. Effects stay PCM16; music is encoded to MS-ADPCM and reblocked from
FFmpeg's output into **524-byte stereo blocks, 512 frames per block** for
XAudio2. The derived loop can gain at most 511 frames of tail padding; the
accepted PCM master is unchanged. FFmpeg drops `smpl`, so the builder restores
it and checks the XWB loop bounds. It decodes the compressed music and checks
that version too. The validator rejects block formats that XWB's mini-format
cannot represent.

The separate publication command verifies that the build uses the exact
accepted master hashes, reinspects the bank, then writes the tracked
`game/content/sounds/paint_exe.xwb` and its
[receipt](../../../scripts/audio/runtime-bank.json). It does not change runtime
bindings. Normal CMake game builds need neither Python nor FFmpeg: they verify
the bank's SHA-256/size, accepted wave order and explicit instance references
before staging it. Missing audio retains the optional silent fallback; a normal
Release package requires the validated bank. The bank compiler is retained at
`scripts/audio/tools/XWBTool.exe`.

The corrected runtime bank is **22,765,884 bytes**. The receipt retains its hash,
the encoded music export hashes, bounded loop padding, decoded measurements and
tool identities. All 103 accepted PCM masters are unchanged. The current 18
audio tests cover pipeline/publication integrity; extracted Release startup
with the real bank and the separate missing-bank fallback also pass. These
checks do not replace listening or multiplayer mix acceptance.

## Recorded technical qualification

[technical-checks.json](technical-checks.json) records the original production master/export
hashes, measurements and tool identity; original recording hashes are in
[sources.json](sources.json), checked by the same validator. The original isolated bank had **103 named entries,
22,772,676 bytes (21.72 MiB)**, under the 40 MB target. Every master is 44.1 kHz
PCM16, every looping master has a full-file forward `smpl` loop, every master and
decoded music export peaks below -1 dBFS, and there are zero onset/tail/seam flags.
The production qualification passed eleven pipeline tests, including a real ADPCM/XWB build and corrupt
metadata rejection. All three renderers reproduce their WAVs byte for byte.

That original encoded bank used 512-byte stereo blocks (500 frames), and the
production checks did not create an XAudio2 voice. Runtime probing on 2026-10-15
found XAudio2 rejects that format (`0x88960001`). The current bank builder uses
the 512-frame format described above; the original technical snapshots are
historical, and the accepted masters remain unchanged. Current migration checks
are recorded in [audio inventory §9](../../../docs/audio-inventory.md#9-runtime-migration-and-remaining-integration).

The listening page's local links and audio formats were checked. Browser UI
automation was unavailable during production; no automated playback or detailed
ear-test observations are claimed. The author's later acceptance is recorded
separately. Technical seam checks do not establish musical fit or intelligibility
under four-player load. The technical snapshots preserve the original pre-approval
state; [acceptance.json](acceptance.json) is the current acceptance record.

## Acceptance and remaining integration

The accepted set is the production committed in `5c0a286`. The catalogue,
provenance and listening page read its acceptance record; changing any master,
adding or removing a cue makes regenerated output pending review again.
The existing game bindings, story pop/sigh/chair cues, per-beat countdown/GO,
arena ending stings, results coda, bank validation and release distribution are
integrated. See
[audio inventory §9](../../../docs/audio-inventory.md#9-runtime-migration-and-remaining-integration)
for the remaining POL#23 hooks and mix work. ROAD#3 then qualifies actual
gains/mute, pause/focus/resume, multiplayer mix and both release paths. Tier 3
detail stays optional and defaults off or nearly off. The Movement Course has
no arena music or ending sting. Movement and painted-support cues are integrated.

The intro/reveal audition keeps picture sound silent while the person is
present. The winner/loser reactions follow the person's departure, as the
backstory specifies. No match/arena music plays under that room interval.
