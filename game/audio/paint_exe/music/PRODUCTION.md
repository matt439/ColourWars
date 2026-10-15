# Paint.exe acoustic music production

Produced for `POL#22` on 2026-10-14 and accepted by the author the same day; the
exact masters and response are in [acceptance.json](../acceptance.json). `ROAD#3` in-game mix
acceptance remains pending. The 2026-10-15 runtime migration uses these masters
for existing menu/arena bindings; results music, ending stings and the gameplay
mix remain in `POL#23`.

The arrangements and melodies were authored by Codex specifically for ColourWars.
They use recorded acoustic instruments: a tenor ukulele, an upright piano,
cajon, handclaps, small shaker, bongos and claves. There are no imported songs,
synthesizer patches, generated oscillators, or legacy game sounds. The editable
composition is `score.json`; `render.py` renders it directly from the retained
recordings. The project's MIT licence covers the score and renderer. The sampled
recordings retain their publishers' CC0 1.0 dedication.

## Arrangements

| Cue | Title | Form | Tempo | Identity |
|---|---|---|---|---|
| `music.menu` | Cursor at Rest | 32 bars: A B C A | 88 BPM | Sparse ukulele melody, upright-piano answer, body taps; C-major-sixth home with a minor middle |
| `music.close_quarters` | Desk Drawer Dash | 40 bars: A A B C A | 126 BPM | Short piano phrases, damped offbeat ukulele, cajon; alternate lead, minor detour and high final reprise |
| `music.king_of_the_hill` | Cardboard Crown | 40 bars: A B C B A | 112 BPM | Broad climbing melody in G, alternating bass and strums, claps; a quieter minor passage followed by the full ensemble |
| `music.turbulence` | Paper Fan Fandango | 48 bars: A B A C B A | 138 BPM | D-dorian tune, bongo cross-rhythms and a two-bar clave; contrasting lead, harmonic side-step, low-density return and final lift |
| `music.end_sting.close_quarters` | Desk Drawer cadence | One-shot, under 2 seconds | 126 BPM | Dominant fragment settling onto the arena's C home chord |
| `music.end_sting.king_of_the_hill` | Cardboard Crown cadence | One-shot, under 2 seconds | 112 BPM | Short D-dominant-to-G resolution |
| `music.end_sting.turbulence` | Paper Fan cadence | One-shot, under 2 seconds | 138 BPM | Short A-dominant-to-D-minor resolution |
| `music.results` | Fresh Palette | One-shot, about 5.25 seconds | 96 BPM | A compact I-vi-IV-V-I coda with a falling clap pulse |

Every letter names an eight-bar section with its own written melody and harmony;
reprises change the lead, density, answer voice or final-register treatment as
specified in `form`. This is not one short loop repeated to meet a duration.
No track represents the Movement Course, which has no music.

## Reproduction and editing

From the repository root, using Python 3.11 or newer:

```powershell
python -m pip install -r game/audio/paint_exe/music/requirements.txt
python game/audio/paint_exe/music/render.py
```

`--only music.menu` renders one named candidate for quick editing, but does not
rewrite the aggregate `cues.json` or `measurements.json`. After editing, run the
full command to refresh those files. The dependency versions above match the
production run. No network, plugin, DAW, soundfont installation or audio device
is required to reproduce the masters. All required recordings are committed.

`score.json` uses scientific pitch names (`C4` is MIDI 60). Each melody string is
one 4/4 bar; a token such as `E5:0.5` is an eighth note, and `r:1` a quarter rest.
Chord voicings, written melodies, form, lead, dynamics, percussion style and
deterministic performance seed are editable there. The emitted
`scores/*.events.json` are inspection records of the actual sample, frame,
pitch, gain, pan and role of every event. They are derived; edit the score.

The VCSL piano uses the older octave convention: its `C3` file is scientific
middle C (`C4`, MIDI 60). This was checked against the recording's roughly
262 Hz fundamental and 523 Hz second harmonic; the renderer applies the octave
mapping once. The FreePats ukulele already uses scientific pitch names.

## Export settings and modifications

- All eight masters are stereo PCM signed 16-bit little-endian WAV at 44,100 Hz.
  Original source recordings are retained byte-for-byte, including their own
  rates and bit depths. Rendering downmixes each to mono before placing it in
  the small stereo ensemble; this is a creative mix choice, not a change to the
  preserved source.
- Rendering removes pre-attack microphone noise, removes sample DC, resamples,
  applies note-by-note transposition to the nearest available recorded pitch,
  normalizes instrument samples, sets velocities and damped note-off releases,
  and adds two very quiet reflections from the recorded mix. Performance timing
  and velocity vary by a fixed seed. No live randomness enters an export.
- Loops have a RIFF `smpl` region from frame zero to the inclusive final frame.
  Their total frames are exact multiples of 500 for the original stereo
  512-byte MS-ADPCM target. Runtime qualification superseded that encoded
  format: XAudio2 rejects its 500-frame blocks. The accepted PCM masters retain
  their original tempo and frames; the tempo adjustment made during production
  is less than 0.004 BPM, recorded exactly in `measurements.json`.
- Instrument decays wrap across the music boundary. A 64-frame correction
  matches the final sample to the first without a fade-to-silence gap. The loop
  is the full arrangement, not a subregion of a padded file.
- One-shots are trimmed to their natural recorded tails, with a short de-click.
  The two-second working allocation is not exported as silence. All three
  ending stings fit the existing two-second glass-out window.
- Menu target RMS is -23 dBFS; arena target RMS is -21 dBFS. These are production
  reference levels, not final game gains and not LUFS claims. All rendered
  peaks are below -1 dBFS. Exact peaks, RMS, durations, event counts and SHA256
  hashes are recorded per asset in `measurements.json`.
- Final MS-ADPCM conversion and XWB packing are separate from these editable
  PCM masters. The current builder reblocks FFmpeg output into 524-byte stereo
  blocks containing 512 frames. It extends encoded loops by at most 511 frames
  using the boundary sample, records the padding and decoded seam measurements,
  and checks the XWB metadata separately. No accepted PCM master is changed by
  this derived-format correction. The author accepted those masters with the
  complete cue set; actual one-, two- and four-player gameplay checks remain ROAD#3.

## Source provenance

`sources.json` records all 36 individual recordings, their exact URLs (including
the immutable VCSL revision), SHA256, creator, licence, licence evidence, date,
archive member where relevant, and both source/release redistribution rights.
`cues.json` maps each exported wave to the exact source IDs it used. This is the
per-asset provenance record: no cue depends on an unnamed source. All eight use
the same renderer and score, on 2026-10-14, with the edits and export settings
above. No attribution is legally required under either source's CC0 dedication;
the credits below are retained voluntarily.

| Source | Creator | Licence and rights | Retained evidence |
|---|---|---|---|
| FreePats Ukulele, version 2026-08-11 | Mateusz Dąbrowski / FreePats | CC0 1.0; source redistribution and release-bank/ZIP use permitted | `licenses/FreePats-CC0-1.0.txt`, original `sources/ukulele/README.txt` and `LICENSE.txt` |
| Versilian Community Sample Library, revision `c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e` | Versilian Studios LLC / Sam Gossner | CC0 1.0; source redistribution and release-bank/ZIP use permitted | `licenses/VCSL-CC0-1.0.txt`, `licenses/VCSL-README.md` |

Primary source pages, checked 2026-10-14:

- [FreePats Ukulele and its explicit CC0 statement](https://freepats.zenvoid.org/GuitarFamily/ukulele.html).
- [FreePats WAV release archive](https://github.com/freepats/ukulele1/releases/download/2026-08-11/Ukulele-SFZ%2BWAV-20260811.7z).
- [VCSL publisher's description and unrestricted CC0 use statement](https://github.com/sgossner/VCSL/blob/c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e/README.md).
- [VCSL licence at the retained revision](https://github.com/sgossner/VCSL/blob/c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e/LICENSE).

Voluntary credit: **Acoustic recordings: Mateusz Dąbrowski / FreePats and
Versilian Studios LLC / Sam Gossner (CC0 1.0). Original ColourWars arrangements:
Codex, 2026-10-14.** Retain this provenance beside the production sources; do not
represent the music as a commissioned human performance. Author acceptance is
recorded separately; no detailed listening observations are claimed.
