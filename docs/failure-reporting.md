# Caught failure reporting

`wWinMain` is the final boundary for C++ exceptions that escape startup or the
application run loop. It exits with code 1 after reporting the original cause to
stderr and a local Windows error dialog. The caption distinguishes startup from
runtime failure, and the dialog identifies what the game was doing.

The implementation is in [fatal_report.cpp](../game/fatal_report.cpp), with phase
transitions in [main.cpp](../game/main.cpp). This closes gap-audit 7.3's misleading
startup caption and missing persisted diagnostic record. The historical audit
remains unchanged.

What the catch never sees, an unhandled structured exception or a call to
`std::terminate`, reaches two handlers that write the same record without a
dialog: the [crash record](#crash-record-rev63).

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

The sandbox trial's receipt reads this exact missing-adapter line
(`scripts/sandbox/logon.ps1`). Runtime errors use `runtime failure:`. Metadata-write
status appears in the dialog, without adding another stderr line: the dialog
names the path it wrote, or says it could not save.

The dialog also gives a local-PC and graphics-driver remedy for the classified
missing-hardware error, including when nested. Content-definition, manifest and
validation failures point to extracting the whole ZIP or rebuilding source
content. These last-resort strings live in C++ because presentation content may
be the failure itself. It calls the metadata a short error report, links the
issue tracker, and asks the player to copy the dialog with Ctrl+C and attach
`last-error.log` when available: the file alone does not contain the error.
This completes first-run review `FTR#11`.

## Persisted metadata and privacy

The latest caught failure replaces
`%LOCALAPPDATA%\ColourWars\last-error.log`. There is one record, no accumulated
history and no upload. If `LOCALAPPDATA` is unavailable, no file is written; there
is no fallback beside the executable or in the working directory.

Format 1 is a UTF-8 `key=value` record containing only these controlled fields:

- UTC timestamp, phase and startup/runtime category.
- Version (`project(VERSION)`, as in `build-info.txt`), game and engine
  revisions, configuration, render backend and audio backend from the
  generated build stamp. `version=` was added by `REL#5` without changing
  `report_format=1`: the record is `key=value`, and the checks name every key.
- Ordered, fixed exception-kind names. Standard system/generic error categories
  may include their numeric error code. Custom category names, messages and codes
  are omitted.
- A fixed reason code for one diagnostic: `no_hardware_device` when the
  engine's message matches exactly (it has no type of its own; engine report
  30). Reasons are never read from other messages. The game's own
  `player_out_of_bounds` reason went with `REV#1` (2026-11-14), which made
  leaving the arena an elimination, so nothing could raise it.
- Cause-count and diagnostic-text truncation flags.
- In a crash record only: `crash_kind`, `exception_code`, `crash_module`,
  `crash_module_name` and `crash_rva` ([crash record](#crash-record-rev63)).
  A module the game did not ship and Windows does not own is `other`, never
  named and given no address, so an injected overlay or hook stays anonymous;
  no address is written except as an RVA inside a classified image.

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

The `crash_record` consumer case checks the module classification table
(the executable, the engine's DLLs beside it and elsewhere, the app-local C++
runtime, the Windows tree, prefix and suffix lookalikes, case), the record's
key order and allowlist, that `other` is never named or addressed, an
over-long record refused, and that the caught record's keys did not move. It
then starts copies of itself with a private Unicode `LOCALAPPDATA` that install
the handlers and crash: `RaiseException` (recorded at `kernelbase.dll`, exit
code kept), `std::terminate` inside a `catch` (module `game`, the caught
`invalid_argument` as `cause_0`, exit code 3), a throw out of a `noexcept`
function (module `game`, `cause_0=runtime_error`), the same from a `noexcept`
function with a `/GS` buffer, whose frame handler is main-image code, and an
access violation (module `game`, exit code `0xC0000005`). Each game RVA must land within 128
bytes after a marker call's return address, each crash must replace the
previous complete record, and no record may carry a path, an argument, an
environment value or exception text. A last child finds a directory where the
record goes: it exits as before, the directory survives and no temporary file
is left. Under `x64-asan` the access-violation child is compiled out, because
AddressSanitizer reports that fault itself.

Normal startup remains covered by `scripts/check-startup.ps1`, including the
accepted audio bank and graceful shutdown; the handlers are installed there
and must not disturb it.

## Crash record (`REV#63`)

**Done 2026-11-14.** The author decided on 2026-11-13 that the game writes
no minidump
([record](review/2026-11-10-project-review.md#no-crash-dump-and-no-release-symbols-rev63));
this is the slice taken instead. [crash_record.cpp](../game/crash_record.cpp)
holds two handlers, and `wWinMain` installs them as its first statement, before
its `try`:

- an unhandled-exception filter (`SetUnhandledExceptionFilter`), for a
  structured exception that nothing handles: an access violation, a raised
  exception, or a C++ exception escaping a thread the CRT does not wrap;
- a terminate handler (`std::set_terminate`), for `std::terminate` called
  directly, a throw out of a `noexcept` function, or anything else that ends in
  terminate on the main thread.

The record is the caught path's `report_format=1` file at the same path, with
the same header (`format_record_header` in fatal_report.cpp, shared, so the two
cannot drift), and `phase`/`category` are the caught phase: `wWinMain` moves
the crash phase with its own (`set_crash_phase`). After `audio_backend` come:

| Key | Value |
|---|---|
| `crash_kind` | `structured_exception` or `terminate`; this is what tells a crash record from a caught one |
| `exception_code` | `0x%08X`, structured exceptions only (`0xC0000005` is an access violation, `0xE06D7363` an escaping C++ exception) |
| `crash_module` | `game`, `engine`, `system` or `other` |
| `crash_module_name` | lower-case file name, for `engine` and `system` only |
| `crash_rva` | `0x%08X` offset into that module, for every class but `other` |
| `cause_N`... | terminate only, when an exception is being handled: the caught path's fixed kind names |

Then `causes_truncated` and `message_truncated`, both `false` unless a terminate
cause chain was cut. For a structured exception the address is the faulting
instruction. For a terminate it is a return address: the first main-image frame
above the runtime's `terminate`, which is the call to `std::terminate` or the
throw that left a `noexcept` function; a main-image frame that ntdll called is
skipped, because that is a frame handler the exception dispatcher ran
(`__GSHandlerCheck_EH4`, linked into every `/GS` function that unwinds), not
the throw. Look up `crash_rva - 1` to land inside the call.

**The module rule.** `game` is the main image. Labrador is a static library, so
the engine's code is inside `ColourWars.exe` too and is `game` here; the
published [PDB](releasing.md#release-symbols-rel7) tells the two apart by
symbol. `engine` is one of the engine's runtime DLLs in the executable's own
folder, by an allowlist of file names: `DirectXTK.dll` and
`XAudio2_9Redist.dll`. `system` is a module under the Windows directory
(`GetSystemWindowsDirectoryW`, a whole-directory prefix), or a Microsoft C++
runtime in the executable's folder by an allowlist: `msvcp140*.dll`,
`vcruntime140*.dll`, `concrt140.dll`, `ucrtbase.dll` and `ucrtbased.dll`.
Everything else is `other`, including an unlisted DLL beside the executable.
Comparison ignores case and works on whole directories, so `C:\WindowsX` is not
the Windows directory.

**Safe in a crashed process.** Installing the handlers computes everything they
need: the record path, its folder and its temporary sibling
`last-error.log.crash.tmp`, the executable path and the Windows directory. The
filter then formats with `snprintf` into a static buffer and writes with raw
Win32: create the folder, write and flush the temporary file, and
`MoveFileExW` it over the record with write-through; a failure deletes the
temporary and leaves the previous complete record. No heap, no
`std::filesystem`. The terminate handler allocates only to classify the cause,
inside a `try`, because terminate is not heap corruption. One record per
process: the first handler to run claims it. With no `LOCALAPPDATA`, nothing is
written, as on the caught path. `SetThreadStackGuarantee` keeps 32 KiB for the
filter on the main thread.

**Nothing else changes.** There is no dialog: a message box is not safe in a
crashed process, so a player gets no in-game pointer to the record, which the
README and the player guide mention. The filter then calls the filter it
replaced (the CRT's, which turns an escaping C++ exception into terminate) and
otherwise continues the search, so Windows Error Reporting and the exit code
(the exception code) are what they were. The terminate handler ends in
`std::abort`, the default behaviour. Both handlers are in Debug and Release
alike.

## Boundary

The catch in `wWinMain` reports C++ exceptions with their message; the crash
record catches unhandled structured exceptions on any thread and terminations
on the main thread, with controlled fields only. Outside both:

- A stack overflow is best effort: the main thread keeps 32 KiB for the filter,
  other threads nothing.
- `std::set_terminate` is per thread in the MSVC runtime, so a terminate on
  another thread (for example an exception escaping a `std::thread`) ends in
  the default `abort` with no record.
- An exception swallowed inside a Win32 callback never arrives. In particular,
  the engine's modal window callback can tick the game outside the ordinary
  run-loop call stack.
- Fast-fail paths skip both handlers: a `/GS` buffer overrun, the CRT's
  invalid-parameter handler in Release, `abort` called directly (Release's
  fast-fails with its fault report) and heap-corruption detection.
- If two threads crash at once, only the first is recorded, and the process
  may end before it finishes writing.

There is still no recovery to the menu, no exception hierarchy, no minidump and
no engine callback containment.

### No match-step containment (`REV#4`)

Declined by the author on 2026-11-13, effective with `REV#1`, which landed on
2026-11-14
([record](review/2026-11-10-project-review.md#no-containment-around-the-match-or-replay-step-rev4)).
`REV#1` removed the only throw a player could reach, so the match and replay
step keep no containment of their own and an escaping exception still ends at
`wWinMain`. Reopen if a released build produces a runtime failure report
(phase `run_loop`) that `REV#1` does not explain.
