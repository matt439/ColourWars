# Request: optional development content-staging convenience

Local **ENG#15** draft. **Unfiled.** Use Labrador's Engine request form when
publishing; this records no upstream request or implementation. This exhibit
supports a development-staging convenience, with runtime installation left
outside its proof.

## Need and exhibit

A client keeps an explicit list of runtime assets beside its executable,
matching Labrador's executable-relative content root. Asset-only changes must
arrive even when the executable does not relink. A missing declared optional
asset must not leave its previous staged copy active. Unlisted source files
must remain outside the runtime mirror.

The [synthetic fixture](../../scripts/engine-reports/runtime-staging/CMakeLists.txt)
reduces the existing client workaround to an always-run target, a caller-owned
required/optional list and a copy/remove script. Its console uses public
`executable_directory`, `resolved_under` and UTF-8 path conversion APIs. A
misleading required file at its working directory proves that it reads the
executable-relative copy. All three source assets are newly written text;
there is no game code, content, Application, window, renderer or audio device.

The script copies explicit entries and removes only a missing optional file
still named in the list. It neither copies a directory wholesale nor prunes
arbitrary unlisted output. The fixture is a representative workaround, not a
production staging-helper implementation or safety certification.

## Checked revision and existing answer

Upstream master was freshly fetched on **2026-10-20 at 19:34:53 UTC** as
`52e809a5a0229d37db4373faa7853df4c861a310`. The entire engine worktree was
confirmed unchanged against that fetched commit at 19:50:08 UTC.

[`engine/app/content_root.h`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/engine/app/content_root.h)
already specifies executable-relative resolution and exposes the functions
used here. There is no demonstrated path-resolution defect.
[`samples/minimal/CMakeLists.txt`](https://github.com/matt439/Labrador/blob/52e809a5a0229d37db4373faa7853df4c861a310/samples/minimal/CMakeLists.txt)
independently carries an always-run content target, explaining why an
executable's `POST_BUILD` alone misses asset-only edits. Its browser branch
already embeds assets with Emscripten and is outside this request. The checked
CMake source has no common development content-staging helper.

## Reproduce

The retained runs used the VS-bundled CMake **4.3.1-msvc1**, Ninja and x64 MSVC
**19.51.36256.0**, with C++20 and `/EHsc /W4 /WX /permissive- /Zc:__cplusplus`.
Debug used `/MDd /Ob0 /Od /RTC1 /Zi /D_DEBUG`; Release used
`/MD /O2 /Ob2 /DNDEBUG`. CMake linked the existing matching
`LabradorEngine.lib` and `MattMath.lib` plus its default Win32 system libraries.
The fixture builds only its console and staging target, without rebuilding
the engine or inheriting its `labrador_settings` target.

Use CMake 3.26+, Ninja and an x64 MSVC Developer PowerShell with those engine
libraries already built at the checked revision. Run from this checkout's
root. For a separate engine checkout, substitute its source/library paths.
Choose a fresh run directory for each configuration; all mutations below are
inside that new fixture copy.

```powershell
$fixtureOriginal = (Resolve-Path 'scripts/engine-reports/runtime-staging').Path
$runRoot = Join-Path (Get-Location) 'out/validation/runtime-staging-debug'
if (Test-Path -LiteralPath $runRoot) { throw 'Choose a fresh run directory' }
New-Item -ItemType Directory -Path $runRoot | Out-Null
$fixtureSource = Join-Path $runRoot 'source'
Copy-Item -LiteralPath $fixtureOriginal -Destination $fixtureSource -Recurse
$fixtureBuild = Join-Path $runRoot 'build'
$engineSource = (Resolve-Path 'external/labrador').Path
$engineLibraries = (Resolve-Path 'out/build/x64-debug/external/labrador/engine').Path
cmake -S $fixtureSource -B $fixtureBuild -G Ninja -DCMAKE_BUILD_TYPE=Debug `
    "-DLABRADOR_ROOT=$engineSource" `
    "-DLABRADOR_LIBRARY=$engineLibraries/LabradorEngine.lib" `
    "-DMATH_LIBRARY=$engineLibraries/math/MattMath.lib"
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
cmake --build $fixtureBuild --verbose --parallel 1
if ($LASTEXITCODE -ne 0) { throw 'Initial build failed' }
$exhibit = Join-Path $fixtureBuild 'run/RuntimeStagingExhibit.exe'
$initialHash = (Get-FileHash -LiteralPath $exhibit -Algorithm SHA256).Hash
$initialTicks = (Get-Item -LiteralPath $exhibit).LastWriteTimeUtc.Ticks

function Invoke-StagingPhase($required, $optional) {
    Push-Location (Join-Path $fixtureBuild 'unrelated-cwd')
    try {
        & $exhibit $required $optional
        if ($LASTEXITCODE -ne 0) { throw 'Fixture run failed' }
    } finally { Pop-Location }
}
function Assert-UnchangedExecutable {
    if ((Get-FileHash -LiteralPath $exhibit -Algorithm SHA256).Hash -ne $initialHash `
        -or (Get-Item -LiteralPath $exhibit).LastWriteTimeUtc.Ticks -ne $initialTicks) {
        throw 'Asset-only build changed executable'
    }
}

Invoke-StagingPhase baseline present
$requiredSource = Join-Path $fixtureSource 'content/runtime/required.txt'
[System.IO.File]::WriteAllText($requiredSource, "revised`n", [System.Text.UTF8Encoding]::new($false))
cmake --build $fixtureBuild --verbose --parallel 1
if ($LASTEXITCODE -ne 0) { throw 'Asset-edit build failed' }
Assert-UnchangedExecutable
Invoke-StagingPhase revised present

$marker = Join-Path $fixtureBuild 'run/not-owned.txt'
[System.IO.File]::WriteAllText($marker, 'preserve-me', [System.Text.UTF8Encoding]::new($false))
Remove-Item -LiteralPath (Join-Path $fixtureSource 'content/runtime/optional.txt')
cmake --build $fixtureBuild --verbose --parallel 1
if ($LASTEXITCODE -ne 0) { throw 'Optional-removal build failed' }
Assert-UnchangedExecutable
Invoke-StagingPhase revised absent
if ([System.IO.File]::ReadAllText($marker) -ne 'preserve-me') { throw 'Unowned marker changed' }

$stagedRequired = Join-Path $fixtureBuild 'run/runtime/required.txt'
$stagedHash = (Get-FileHash -LiteralPath $stagedRequired -Algorithm SHA256).Hash
Remove-Item -LiteralPath $requiredSource
cmake --build $fixtureBuild --verbose --parallel 1
if ($LASTEXITCODE -ne 1) { throw 'Missing required asset should produce build exit 1' }
Assert-UnchangedExecutable
if ((Get-FileHash -LiteralPath $stagedRequired -Algorithm SHA256).Hash -ne $stagedHash) {
    throw 'Missing-required build changed staged text'
}
```

Repeat from a fresh `...-release` run directory, changing `CMAKE_BUILD_TYPE` to
`Release` and the library directory to `out/build/x64-release/...`. The
single-config x64 MSVC constraints delimit this fixture's checks; they are not
requested restrictions on a generic helper. The fixture-local Git attributes
preserve LF source payloads on checkout, and CMake explicitly generates the
cwd decoy with `NEWLINE_STYLE UNIX`.

## Measured results

Each configuration produced these three stdout lines in separate runs, all
exit 0 with empty stderr:

```text
PASS runtime staging: required=baseline; optional=present; unlisted excluded; executable-relative read defeats cwd decoy
PASS runtime staging: required=revised; optional=present; unlisted excluded; executable-relative read defeats cwd decoy
PASS runtime staging: required=revised; optional=absent; unlisted excluded; executable-relative read defeats cwd decoy
```

Both asset-only builds ran staging without compiler/linker commands, while
the executable SHA-256 and modification time stayed unchanged. The absent
optional file was removed and the unrelated output marker survived. Each
missing-required build exited 1 naming `runtime/required.txt`, before changing
any staged-output hashes. Both configurations completed without CMake warnings.
All runtime assertions remain active in Release.

These observations establish the synthetic client mechanism and existing
engine path APIs. They establish no new engine helper, install tree, ZIP,
redistributable-runtime handling or clean-machine/hardware acceptance.

## Optional extraction and client policy

Consider a documented CMake convenience accepting a consuming target, source
content directory and explicit required/optional relative file lists. It could
own the small always-run target and copy/remove mechanics repeated by clients.
The spelling and whether this warrants a helper remain maintainer decisions;
a maintained documented pattern is also a possible answer. This is an
extraction request, not a packaging bug or missing CMake primitives.

The caller keeps asset selection, manifest/format validation, generated-asset
rules, fallback policy and stricter mirror ownership/pruning. No prebuilt
`find_package`/export consumer, automatic dependency discovery or arbitrary
directory cleanup is demonstrated. Runtime DLL/CRT installation remains a
separate proposal in ENG#15/PI-11; backend/Release restrictions, package names,
notices, ZIP layout and release acceptance remain client policy.

**T1:** extract mechanics demonstrated by the client and minimal sample while
the caller supplies asset policy. The generic CMake code is small enough that
a documented recipe may be the appropriate answer.
