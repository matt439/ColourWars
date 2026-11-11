# Contributing to ColourWars

ColourWars is a Windows local split-screen paint-shooter and the first client
of the [Labrador engine](https://github.com/matt439/Labrador). Check
[TODO.md](TODO.md) and an item's owner document before starting work. Describe
new proposals in an issue, including any accepted behaviour they would change.

## Reporting a bug

Use the [bug report form](https://github.com/matt439/ColourWars/issues/new?template=bug_report.yml).
Include steps to reproduce, what you expected, what happened, and the release
or source revision. For an error, copy the full dialog with **Ctrl+C** while it
is focused: `last-error.log` does not contain that message. Include
`build-info.txt` from beside the executable and
`%LOCALAPPDATA%\ColourWars\last-error.log` when available; for a build failure,
include the relevant build log instead.

Reports and attachments are public. Review them for personal information and
redact private paths or names in dialog text and build logs. Do not attach your
save, the whole local-data folder or a memory dump. The game does not upload
diagnostics automatically; [failure reporting](docs/failure-reporting.md)
describes the small local report and its limits.

## Changing the game

Read [AGENTS.md](AGENTS.md) for the working rules,
[architecture](docs/architecture.md) for ownership and dependency boundaries, and
the [OOP guide](docs/oop-guide.md) for the object model. The game's deep
inheritance and virtual dispatch are deliberate: do not flatten its hierarchies
to match the engine's style. Engine changes belong in Labrador, not in the
`external/labrador` submodule here. Describe client-side API problems in the
[engine-report ledger](docs/review/engine-reports.md).

Follow the existing naming and repository-root include conventions. Keep
player-facing copy in `presentation.json`, and preserve asset provenance and
licences. Update the relevant owner docs with a change; remove a completed
TODO row in the same commit.

The [build instructions](docs/development.md#building) cover Visual Studio,
`VCPKG_ROOT` and the CMake presets. Run the checks appropriate to the change;
[AGENTS.md](AGENTS.md#tests) and [scripts/README.md](scripts/README.md) explain
the acceptance runners. This repository intentionally has no CTest targets.
Report what you checked and any hardware, listening or play checks still needed.

## Commits and pull requests

Commit completed, validated work on a working branch, not `master`. Keep related
work on that branch; open a pull request when it is ready for review. Agents
must wait for the author's explicit request to push or open a pull request.
Explain the problem, resulting behaviour and validation in the pull request.

`master` accepts changes only through the required `gate` check. Keep the branch
up to date with `master`, merge changes from `master` when it moves, and wait
for the new run to pass before merging. Never bypass or weaken the ruleset,
`gate`, its required jobs or lint to land a change.
