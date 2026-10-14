# Paint.exe audio production candidates

Produced on 2026-10-14 for **POL#22**. Open [index.html](index.html) in a browser
to hear individual cues and the same scripted scene at one, two and four players.
Start with Tier 1. There are **103 WAV masters covering all 85 cue groups** in
[audio-inventory §4](../../../docs/audio-inventory.md#4-the-new-cue-list):
49 Tier 1 waves, 37 Tier 2 waves and 17 Tier 3 waves.

These are complete production candidates, **not listening-approved assets**.
POL#22 remains open for the author's listening pass and any resulting revisions.
No game sound binding or live bank changes here; POL#23 owns that migration.
ROAD#3 still owns actual multiplayer, pause/resume, volume and release acceptance.

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
  sourced candidates for STORY#1/STORY#2, not recordings by the repository author.
- [interface/project.json](interface/project.json): 31 short arrangements of
  recorded mouse switches, paper, instrument body taps, claps and ukulele notes.
  UI cues belong to the invented program; no real operating-system sound is used.
- [sources.json](sources.json): 71 retained source recordings, exact source URLs,
  creators, licence/redistribution fields and SHA-256 hashes. Some sources serve
  several cues; every retained recording is used.
- [PROVENANCE.md](PROVENANCE.md): a separate provenance record for every WAV.
  [cues.json](cues.json) is the complete mapping from cue to master and sources.
- [paint_exe.json](paint_exe.json): the candidate wave inventory beside the
  provenance record. It deliberately has no runtime effect instances; POL#23
  must allocate the pause-aware music/weapon/per-player voices.

Sources remain in their downloaded formats. Most are lossless WAV; the chair
creak is explicitly a CC0 high-quality MP3 preview, not a lossless original.
Its original WAV requires a Freesound login. This quality choice is visible in
the foley source record and should be checked in the listening pass.

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
python -m unittest discover -s scripts/audio -p test_audio_pipeline.py
```

The JSON scores/recipes are the editable production sessions. Renderers retain
the unchanged original recordings and write derivative masters only inside
their own subtree. `prepare_review.py` assembles the catalogue/provenance and
creates the listening page and its Ogg previews. Ogg is for browser audition
only, never a bank input. The exact preview events and fixed shared gain are in
[auditions/timeline.json](auditions/timeline.json). All four seats remain centred,
as with the current shared-speaker game; there is no attenuation as seats are
added. These are offline arrangements, not captured gameplay or mix acceptance.

The bank builder writes only to `out/audio-production/`. It verifies original
and derivative hashes, coverage, source permissions, PCM formats, peaks, silence,
loop regions and the named XWB's actual samples/metadata. It never reuses the
legacy bank. Effects stay PCM16; music is encoded to MS-ADPCM using **512-byte
stereo blocks, 500 frames per block**. FFmpeg drops `smpl`, so the builder restores
it and checks the XWB loop bounds. It decodes the compressed music and checks
that version too. The bundled XWBTool silently misrepresents larger default
ADPCM blocks; the validator rejects them.

## Recorded technical qualification

[technical-checks.json](technical-checks.json) records the final master/export
hashes, measurements and tool identity; original recording hashes are in
[sources.json](sources.json), checked by the same validator. The isolated bank has **103 named entries,
22,772,676 bytes (21.72 MiB)**, under the 40 MB target. Every master is 44.1 kHz
PCM16, every looping master has a full-file forward `smpl` loop, every master and
decoded music export peaks below -1 dBFS, and there are zero onset/tail/seam flags.
Eleven pipeline tests pass, including a real ADPCM/XWB build and corrupt
metadata rejection. All three renderers reproduce their WAVs byte for byte.

The listening page's local links and audio formats were checked. Browser UI
automation was unavailable in this session, so no browser playback or human
auditory review is claimed. Technical seam checks do not establish a pleasing
musical join, good timbre, or intelligibility under four-player load.

## Listening exit for POL#22

1. Listen to Tier 1 in the page, then the 1/2/4-player comparisons at a fixed
   speaker volume. Identify each tool without looking; check that hurt, empty,
   elimination and countdown remain clear. Check whether the music fits the art.
2. Listen across repeated music and weapon loop joins. Review repeated
   steps/landings/splashes, UI refusal versus cancel, and the three story cues.
3. Review Tiers 2 and 3; reject unnecessary decorative audio. Their production
   does not require enabling them: ambient/tool detail should default to off,
   and the Movement Course stays silent except for player/action cues.
4. Record accepted/rejected cue IDs and requested changes in audio-inventory §7.
   Once the listening revisions are settled, close POL#22 and begin POL#23.

The intro/reveal audition keeps picture sound silent while the person is
present. The winner/loser reactions follow the person's departure, as the
backstory specifies. No match/arena music plays under that room interval.
