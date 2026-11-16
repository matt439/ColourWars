# Shipped first-run package qualification (2026-11-15)

This record covers package integrity and isolated startup of the published
v0.1.0 download on 2026-11-15. It does not qualify later source builds, CRT,
controller recovery or a complete first match.

## Exact input

- Official release: https://github.com/matt439/ColourWars/releases/tag/v0.1.0
- Asset: ColourWars-0.1.0-windows-x64.zip (21,986,443 bytes).
- SHA256: fa3f8244277cebd2660509b4a447d2ac87982d88f8b5565fae06aeabf2c50896.
- Digest matched the release asset metadata after download.
- Scripts checked out at release commit 9e7dbaaf128b937793b8ab4fdee95b0e052dec98
  so version-resource checks match the asset rather than 0.1.1 development.

## Executed evidence

`check-release-package.ps1` passed 35 checks on 69 extracted files. This covers
required redistributables, content/licences, accepted audio-bank hash, absence
of development payload, and executable symbol-path conventions.

`check-startup.ps1 -SystemOnlyPath -RequireAudio -StartupSeconds 5` passed on
MattPC with a new disposable profile and unrelated working directory. The
hidden process created its window, save and stages folder; first window fit
and per-monitor v2 DPI awareness passed. Graceful close and diagnostics checks
passed. Sound was muted by the harness; this does not establish listening
quality. The extracted package did not need developer-tool directories on PATH.

Evidence was retained locally on the test machine under the user's TEMP folder,
`colourwars-first-match-20261115`: `extract` and
`startup/955702f068f14eb28aa7802ffa7e8528/result.json`.
This temporary evidence directory is not a durable or public artifact.

## First-match and recovery boundary

The packaged HOW-TO-PLAY gives extraction, menu route, keyboard/bot/roster entry,
Movement Course, settings location and update/removal guidance. Those directions
were inspected on 2026-11-15, not enacted with real participants.

Source probes were not rebuilt or executed for this check. This shipped
executable was not taken through controller recovery or a first match.

An attended first-match check would need an actual keyboard/pad session: new
profile, Movement Course, complete round, Apply/Back persistence,
unplug/reclaim/Resume, then quit and restart. A hidden packaged Release startup cannot demonstrate
that input sequence by itself. No SmartScreen bypass, installer, foreground
input, live profile, or remote publication occurred in this check.
