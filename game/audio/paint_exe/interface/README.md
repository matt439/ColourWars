# Interface and match arrangements

31 cues, edited by Codex on 2026-10-14 from recorded CC0 sources. Exact sample
selection, excerpts, pitch/speed edits, layer timing, filtering and peak targets
are in [project.json](project.json); [render.py](render.py) is the production
session renderer. Run it from any directory. It uses NumPy, SciPy and FFmpeg.

The two original OwlishMedia recordings and licence evidence are retained here.
Acoustic instrument/clap layers reference the unchanged [music sources](../music/sources.json).
There are no oscillators, synthetic noise layers, sampled OS cues or legacy game
waves. Every source is CC0 1.0; exact source/rights records are in
[sources.json](sources.json) and the [complete provenance](../PROVENANCE.md).
Original arrangements and production code use the repository MIT licence.

The export is PCM16 mono at 44.1 kHz. Each layer loses only its quiet lead/tail,
then gets its specified pitch/filter edit and short edge fades. Final one-shots
are trimmed after gain so quiet decorative cues have no padded silence.
`results.fill` wraps recorded percussion tails across its two-second cycle and
stores a full-file forward `smpl` loop. `ui.count_in` intentionally reuses the
`match.count_beat` arrangement, as required by the inventory.

UI focus is deliberately quieter than confirmation; refusal and countdown lead
the mix; cursor waver and cheating figures are nearly silent. Those values are
accepted production mastering choices, not final game settings. The author
accepted this set on 2026-10-14; [acceptance.json](../acceptance.json) records the
exact masters. Runtime integration and in-game mix acceptance remain pending.
