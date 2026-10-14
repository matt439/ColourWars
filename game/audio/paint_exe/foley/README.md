# Physical sound candidates

These are the `POL#22` production candidates for players, paint, tools and the
outside room. They are not a live bank and have not passed author listening or
the one-, two- and four-player game acceptance. The frozen room/picture rule
still belongs to the runtime integration: room cues play outside the monitor;
the picture stays silent while the person is present.

`sources.json` records the exact original used by every edit, its primary asset
page, download URL, creator, licence, redistribution permission and SHA-256.
The source files are retained without alteration. `recipes.json` is the
editable production session, in source seconds. `build.py` renders its 64 cues
and writes `cues.json` and the per-output `PROVENANCE.json`.

Run `python game/audio/paint_exe/foley/build.py` from a checkout with Python,
NumPy, SciPy and FFmpeg. FFmpeg decodes only the retained chair MP3. The build
does not download anything. It verifies input hashes before rendering.

All waveforms originate in recordings of physical objects or people. Source
time selections, tape-style speed changes, EQ and layered foley give the
drawings their character. The sprayer uses a pump and pressured aerosol; the
mister uses a higher, finer aerosol texture; the roller combines compressed
foamy sponge with a wall brush. The roller is a designed surrogate, not a claim
that the original recording was a paint roller. Steps are six separate marker
gestures; landings are four paper gestures; splashes use six separate wet
contacts. The single pop is cut from real water bubbles. The sigh is a human
exhale and the chair uses a recorded wooden chair creak.

Masters are 44.1 kHz signed 16-bit PCM, mono except the dual-mono room tone.
The room source itself is mono; duplicating it preserves that spatial image
without inventing stereo movement. Each class uses an RMS target and peak
ceiling in its recipe. All ceilings are at most -2 dBFS, with decorative cues
far lower. `recommended_gain` also keeps optional details quiet or off.

One-shots are trimmed to the recorded gesture and given short edge fades.
Loops use an overlapping tail/head crossfade with the seam moved into the
body. They have a forward infinite `smpl` loop covering the full wave, with an
inclusive end frame and matching boundary samples. There is no fade-to-silence
at a loop boundary. The player redraw preserves the rhythm of actual scribbles.

The Movement Course has no ambience candidate: its deliberate silence is part
of the accepted inventory. Other Tier 3 candidates remain optional. In
particular, `world.detail` has a recommended gain of zero. The hit-confirm cue
exists for audition but shared speakers may make it unsuitable to enable.

## Licence evidence checked on 2026-10-14

- [BigSoundBank's licence page](https://bigsoundbank.com/licenses.html) names
  CC0 1.0 Universal and permits copying, distributing, adapting and commercial
  use without restrictions or permission. Each selected asset page separately
  names Joseph SARDIN, identifies its recording and carries the CC0 grant.
  The original BWF WAV URL comes from that asset page's public AudioObject
  metadata. Credit is optional; the suggested voluntary credit is
  **Additional sounds: Joseph SARDIN - BigSoundBank.com**.
- [OwlishMedia's Sound Effects Pack](https://opengameart.org/content/sound-effects-pack)
  identifies the uploader as OwlishMedia, says these are their recordings, and
  publishes the pack under CC0. `archive_member` identifies each retained file
  inside the original ZIP. The synthetic/scifi/UI files in that ZIP were not
  selected for these physical cues. Voluntary credit: **Recorded foley:
  OwlishMedia (owlstorm), CC0 1.0**.
- [Chair creaking back and forth 01.wav](https://freesound.org/people/unkleceeg/sounds/276473/)
  identifies unkleceeg, a Zoom Q2HD recording, and Creative Commons 0. The
  retained file is its public high-quality MP3 preview. It is not represented
  as the login-gated original WAV; its decoded data remains lossy. Voluntary
  credit: **Chair recording: unkleceeg, Freesound 276473, CC0 1.0**.

The [CC0 1.0 terms](https://creativecommons.org/publicdomain/zero/1.0/) allow
both the editable source recordings in this repository and derived waves in
the game bank and release ZIP. No attribution is required for these sources.
The registry and per-output provenance preserve voluntary credits and the
source-to-derivative chain even when many cues share one recording.
