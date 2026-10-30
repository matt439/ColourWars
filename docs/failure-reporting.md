# Caught failure reporting

`wWinMain` is the final boundary for C++ exceptions that escape startup or the
application run loop. It exits with code 1 after reporting the original cause to
stderr and a local Windows error dialog. The caption distinguishes startup from
runtime failure, and the dialog identifies what the game was doing.

The implementation is in [fatal_report.cpp](../game/fatal_report.cpp), with phase
transitions in [main.cpp](../game/main.cpp). This closes gap-audit 7.3's misleading
startup caption and missing persisted diagnostic record. The historical audit
remains unchanged.

## Phase and original cause

| Record phase | Work in progress | Category |
|---|---|---|
| `preferences` | Resolving startup paths and reading player settings | startup |
| `content_definitions` | Reading tuning and presentation | startup |
| `application_initialization` | Initializing the application, window, graphics and audio | startup |
| `manifest` | Loading manifest assets | startup |
| `content_validation` | Validating content and its references | startup |
| `launch_configuration` | Applying developer launch arguments | startup |
| `run_loop` | Constructing the initial menu and running the application | runtime |

The original `what()` text is retained in stderr and the dialog. Nested
exceptions are shown in order with `Caused by:` separators, including a standard
exception nested inside a non-standard wrapper. A non-standard exception with
no printable interface is identified as such. Reporting is bounded to 16 causes
and 65,536 diagnostic bytes; omitted causes and truncated dialog text are marked.

Single-cause startup stderr keeps its existing spelling:

```text
startup failure: No Direct3D hardware device found
```

The exact missing-adapter line still supports the narrowly scoped Release skip
in `check-startup.ps1`. Runtime errors use `runtime failure:`. Metadata-write
status appears in the dialog, without adding another stderr line.

## Persisted metadata and privacy

The latest caught failure replaces
`%LOCALAPPDATA%\ColourWars\last-error.log`. There is one record, no accumulated
history and no upload. If `LOCALAPPDATA` is unavailable, no file is written; there
is no fallback beside the executable or in the working directory.

Format 1 is a UTF-8 `key=value` record containing only these controlled fields:

- UTC timestamp, phase and startup/runtime category.
- Game and engine revisions, configuration, render backend and audio backend
  from the generated build stamp.
- Ordered, fixed exception-kind names. Standard system/generic error categories
  may include their numeric error code. Custom category names, messages and codes
  are omitted.
- Fixed reason codes for the existing player-out-of-bounds and missing-hardware
  diagnostics, when the original message matches exactly.
- Cause-count and diagnostic-text truncation flags.

Arbitrary exception messages, RTTI names, file paths, command-line arguments,
environment values and save or match contents are never copied into this file.
The local dialog and stderr deliberately retain the original diagnostic, which
can contain a content filename or another value supplied by the failing code.
This distinction avoids trying to sanitize arbitrary exception text before
persisting it.

Writing is best effort. A complete temporary sibling is flushed and atomically
replaces the old record; a failed write leaves the previous complete record in
place and removes its temporary file. Directory creation, formatting, path lookup
or writing must not replace the original error. If report construction itself
fails, a minimal stderr/dialog path still presents the original top-level cause.
The process still returns 1 whether or not metadata could be saved.

## Verification

From an x64 Visual Studio Developer shell, with `VCPKG_ROOT` set:

```powershell
scripts/run-acceptance.ps1 -Preset x64-debug
scripts/run-acceptance.ps1 -Preset x64-release
scripts/check-failure-reporting.ps1 -Preset x64-debug -SkipBuild
scripts/check-failure-reporting.ps1 -Preset x64-release -SkipBuild
```

The `fatal_report` consumer case needs no window or graphics/audio device. It
checks phases, nested causes, non-standard exceptions, Unicode, bounds, exact
metadata fields and privacy, and isolated per-user path handling. Persistence
checks cover complete replacement, locked and read-only files, a directory at
the target, a blocked parent, cleanup and subsequent recovery. Its original
causes and previous complete records must survive failed writes.

The separate entry-point probe copies the actual executable and runtime DLLs
into an isolated directory, then supplies malformed tuning that fails before
device initialization. It reads and dismisses only that process's error dialog.
Both writable and blocked-log cases require the complete original parser cause
in the dialog and exact stderr, exit code 1, correct metadata status and no
private fixture values in the report. Evidence is retained under
`out/validation/failure-reporting/<preset>/<run>/`. CI runs the probe in both
jobs and uploads its receipts, dialog text and stderr. It takes about a second
and needs no focus or input; see
[code-hygiene.md](code-hygiene.md#registration-and-failure-reporting-checks-in-ci-ci3).
An existing isolated save containing a private marker must remain byte-for-byte
unchanged in both cases.

Normal startup remains covered by `scripts/check-startup.ps1`, including the
accepted audio bank and graceful shutdown.

## Boundary

This is caught C++ failure reporting, not a general crash handler. Structured
exceptions, termination from `noexcept` violations and exceptions that cross
native Win32 callback boundaries are outside this boundary. In particular, the
engine's modal window callback can tick the game outside the ordinary run-loop
call stack. This change does not add recovery to the menu, an exception
hierarchy, minidumps or engine callback containment.
