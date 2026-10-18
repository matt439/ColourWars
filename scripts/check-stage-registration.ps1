[CmdletBinding()]
param(
    [ValidateSet('x64-debug', 'x64-release')]
    [string] $Preset = 'x64-debug',
    [switch] $SkipBuild,
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Assert-Check {
    param([bool] $Condition, [string] $Message)
    if (-not $Condition) { throw $Message }
}

function Write-Utf8 {
    param([string] $Path, [string] $Text)
    [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
}

function Get-Hash {
    param([string] $Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Get-Inventory {
    param([string] $Root)
    $result = [ordered]@{}
    foreach ($file in Get-ChildItem -LiteralPath $Root -File -Recurse | Sort-Object FullName) {
        $name = $file.FullName.Substring($Root.Length + 1).Replace('\', '/')
        $result[$name] = [ordered]@{ sha256 = Get-Hash $file.FullName; bytes = $file.Length }
    }
    return ,$result
}

function Assert-Inventory {
    param($Before, $After, [string[]] $Changed = @())
    Assert-Check ($Before.Count -eq $After.Count) 'Registration added or removed an unexpected source file.'
    foreach ($name in $Before.Keys) {
        Assert-Check ($After.Contains($name)) "Registration removed source file: $name"
        if ($name -notin $Changed) {
            Assert-Check ($Before[$name].sha256 -ceq $After[$name].sha256) "Unexpected source change: $name"
        }
    }
}

function Copy-EvidenceFile {
    param([string] $Source, [string] $Destination)
    [IO.Directory]::CreateDirectory((Split-Path -Parent $Destination)) | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination
    Assert-Check ((Get-Hash $Source) -ceq (Get-Hash $Destination)) "Copy differs: $Source"
}

function Invoke-NativeCheck {
    param([string] $Command, [string[]] $Arguments, [string] $Name, [int] $ExpectedExit = 0)
    # Only native processes set this code. PowerShell script checks below use
    # terminating errors plus their own result.json receipt instead.
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $lines = & $Command @Arguments 2>&1
        $code = $LASTEXITCODE
    }
    finally { $ErrorActionPreference = $previous }
    $log = Join-Path $run "$Name.log"
    $lines | Out-File -LiteralPath $log -Encoding utf8
    $script:commands.Add([ordered]@{
        name = $Name; command = $Command; arguments = @($Arguments)
        expected_exit = $ExpectedExit; exit_code = $code; log = $log; log_sha256 = Get-Hash $log
    })
    Assert-Check ($code -eq $ExpectedExit) "$Name exited with $code (expected $ExpectedExit). See $log"
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repo "out/acceptance/$Preset/stage-registration" }
$evidence = [IO.Path]::GetFullPath($OutputDirectory)
$run = Join-Path $evidence ([Guid]::NewGuid().ToString('N'))
$scratch = Join-Path $run 'repository'
$content = Join-Path $scratch 'game/content'
$build = Join-Path $repo "out/build/$Preset"
$commands = [Collections.Generic.List[object]]::new()
$sourceHashes = [ordered]@{}
$result = [ordered]@{
    outcome = 'failed'; preset = $Preset; run_directory = $run
    source_repository = $repo; scratch_repository = $scratch
    source_asset_count = 0; registered_asset_count = 0; copied_runtime_bytes = 0
    changed_files = @(); commands = $commands; source_unchanged = $false
    startup = $null; diagnostic = ''
}
Assert-Check (-not (Test-Path -LiteralPath $run)) "The GUID evidence directory already exists: $run"
[IO.Directory]::CreateDirectory($content) | Out-Null

Push-Location -LiteralPath $repo
try {
    Assert-Check ($null -ne (Get-Command cmake -ErrorAction SilentlyContinue)) 'cmake is required.'
    if (-not $SkipBuild) {
        Assert-Check ($null -ne (Get-Command cl -ErrorAction SilentlyContinue)) 'Run from an x64 Visual Studio Developer shell (cl is missing).'
        Assert-Check ([bool]$env:INCLUDE -and [bool]$env:LIB -and
            (-not $env:VSCMD_ARG_TGT_ARCH -or $env:VSCMD_ARG_TGT_ARCH -eq 'x64')) 'Run from an x64 Visual Studio Developer shell.'
        Invoke-NativeCheck 'cmake' @('--preset', $Preset) 'configure-game'
        Invoke-NativeCheck 'cmake' @('--build', '--preset', $Preset, '--parallel', '2',
            '--target', 'ColourWarsRegisterStage', 'ColourWars') 'build-game-and-tool'
    }

    $cachePath = Join-Path $build 'CMakeCache.txt'
    Assert-Check (Test-Path -LiteralPath $cachePath -PathType Leaf) "No configured build at $build. Run without -SkipBuild first."
    $cache = @{}
    foreach ($line in Get-Content -LiteralPath $cachePath) {
        if ($line -match '^([^/#][^:]*):[^=]*=(.*)$') { $cache[$Matches[1]] = $Matches[2] }
    }
    Assert-Check ($cache.ContainsKey('CMAKE_HOME_DIRECTORY') -and $cache.ContainsKey('CMAKE_BUILD_TYPE')) 'The build cache lacks source/configuration provenance.'
    Assert-Check ([IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY) -eq $repo) 'The build belongs to another checkout.'
    $configuration = if ($Preset -eq 'x64-debug') { 'Debug' } else { 'Release' }
    Assert-Check ($cache.CMAKE_BUILD_TYPE -ceq $configuration) "$Preset must contain a $configuration build."
    $tool = Join-Path $build 'editor/ColourWarsRegisterStage.exe'
    $game = Join-Path $build 'game/ColourWars.exe'
    foreach ($file in @($tool, $game)) {
        Assert-Check (Test-Path -LiteralPath $file -PathType Leaf) "Missing executable: $file. Run without -SkipBuild first."
        $sourceHashes[$file] = Get-Hash $file
    }
    foreach ($file in @($PSCommandPath, (Join-Path $PSScriptRoot 'check-startup.ps1'))) {
        $sourceHashes[$file] = Get-Hash $file
    }

    # Ask the existing CMake list for its exact reviewed runtime files; never
    # copy game/content wholesale (private/editable assets may be alongside it).
    $exporter = Join-Path $run 'export-runtime-assets.cmake'
    Write-Utf8 $exporter @'
cmake_minimum_required(VERSION 3.26)
include("${RUNTIME_FILE}")
string(REPLACE ";" "\n" assets "${COLOURWARS_RUNTIME_ASSETS}")
file(WRITE "${ASSETS_FILE}" "${assets}\n")
string(REPLACE ";" "\n" optional "${COLOURWARS_OPTIONAL_RUNTIME_ASSETS}")
file(WRITE "${OPTIONAL_FILE}" "${optional}\n")
'@
    $assetsFile = Join-Path $run 'baseline-runtime-assets.txt'
    $optionalFile = Join-Path $run 'optional-runtime-assets.txt'
    Invoke-NativeCheck 'cmake' @("-DRUNTIME_FILE=$repo/cmake/runtime_assets.cmake",
        "-DASSETS_FILE=$assetsFile", "-DOPTIONAL_FILE=$optionalFile", '-P', $exporter) 'enumerate-assets'
    $assets = @(Get-Content -LiteralPath $assetsFile | Where-Object { $_ -ne '' })
    $optional = @(Get-Content -LiteralPath $optionalFile | Where-Object { $_ -ne '' })
    Assert-Check ($assets.Count -gt 0 -and $assets.Count -eq @($assets | Sort-Object -Unique).Count) 'Runtime list is empty or contains duplicate files.'
    $result.source_asset_count = $assets.Count
    foreach ($asset in $assets) {
        Assert-Check ($asset -cmatch '^[A-Za-z0-9_.-]+(/[A-Za-z0-9_.-]+)*$' -and
            $asset -notmatch '(^|/)\.\.?(/|$)') "Unsafe runtime asset path: $asset"
        $from = Join-Path $repo "game/content/$asset"
        if (-not (Test-Path -LiteralPath $from -PathType Leaf)) {
            Assert-Check ($asset -in $optional) "Missing reviewed runtime asset: $asset"
            continue
        }
        $sourceHashes[$from] = Get-Hash $from
        $result.copied_runtime_bytes += (Get-Item -LiteralPath $from).Length
        Copy-EvidenceFile $from (Join-Path $content $asset)
    }

    # Existing validators keep their normal relative metadata paths in this
    # small source tree. These are support metadata, not extra runtime assets.
    $support = @('cmake/runtime_assets.cmake', 'cmake/release_assets.cmake',
        'cmake/stage_runtime_assets.cmake', 'cmake/check_wave_bank.cmake',
        'cmake/check_sprite_fonts.cmake', 'scripts/audio/runtime-bank.json',
        'game/art/sources/fonts/fonts.json')
    foreach ($file in $support) {
        $from = Join-Path $repo $file
        $sourceHashes[$from] = Get-Hash $from
        Copy-EvidenceFile $from (Join-Path $scratch $file)
    }
    Write-Utf8 (Join-Path $scratch 'CMakeLists.txt') @'
cmake_minimum_required(VERSION 3.26)
project(ColourWarsRegistrationAssets LANGUAGES NONE)
# The same configure-time agreement check used by the game/package build.
include(cmake/release_assets.cmake)
'@

    $stageName = 'registration_probe'
    $stageAsset = "levels/$stageName.json"
    Assert-Check ($stageAsset -notin $assets) 'The integration stage name is already registered.'
    $stage = Join-Path $content $stageAsset
    Copy-EvidenceFile (Join-Path $content 'levels/movement_course.json') $stage
    $invalid = Join-Path $content 'levels/registration_invalid.json'
    Write-Utf8 $invalid '{}'
    $badName = Join-Path $content 'levels/bad stage.json'
    Copy-EvidenceFile $stage $badName
    $outside = Join-Path $run 'outside.json'
    Copy-EvidenceFile $stage $outside
    $outsideHash = Get-Hash $outside
    $before = Get-Inventory $scratch
    $before | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'source-before.json') -Encoding utf8
    $manifestPath = Join-Path $content 'manifest.json'
    $runtimePath = Join-Path $scratch 'cmake/runtime_assets.cmake'
    $manifestBefore = [IO.File]::ReadAllText($manifestPath)
    $runtimeBefore = [IO.File]::ReadAllText($runtimePath)

    Invoke-NativeCheck $tool @('--repo', $scratch, '--stage', $stage) 'preview'
    Assert-Inventory $before (Get-Inventory $scratch)
    Invoke-NativeCheck $tool @('--repo', $scratch, '--stage', $outside, '--apply') 'reject-outside' 1
    Invoke-NativeCheck $tool @('--repo', $scratch, '--stage', $invalid, '--apply') 'reject-invalid-level' 1
    Invoke-NativeCheck $tool @('--repo', $scratch, '--stage', $badName, '--apply') 'reject-invalid-basename' 1
    Invoke-NativeCheck $tool @('--unexpected') 'reject-bad-arguments' 2
    # A directory junction needs no privileged file-symlink permission and
    # deterministically exercises the API's reparse-chain refusal on Windows.
    $junction = Join-Path $run 'linked-repository'
    $null = New-Item -ItemType Junction -Path $junction -Target $scratch
    Assert-Check ((Get-Item -LiteralPath $junction).Attributes -band [IO.FileAttributes]::ReparsePoint) 'The junction fixture is not a reparse point.'
    $result['junction'] = [ordered]@{ path = $junction; target = $scratch; retained = $true }
    Invoke-NativeCheck $tool @('--repo', $junction, '--stage', "game/content/$stageAsset", '--apply') 'reject-junction' 1
    Assert-Inventory $before (Get-Inventory $scratch)
    Assert-Check ((Get-Hash $outside) -ceq $outsideHash) 'Rejected registration changed the outside file.'

    Invoke-NativeCheck $tool @('--repo', $scratch, '--stage', $stage, '--apply') 'apply'
    $after = Get-Inventory $scratch
    $changed = @('game/content/manifest.json', 'cmake/runtime_assets.cmake')
    Assert-Inventory $before $after $changed
    foreach ($name in $changed) {
        Assert-Check ($before[$name].sha256 -cne $after[$name].sha256) "Apply did not update $name"
    }
    $result.changed_files = $changed
    $manifestAfter = [IO.File]::ReadAllText($manifestPath)
    $runtimeAfter = [IO.File]::ReadAllText($runtimePath)
    # Independently remove exactly the new literal entry. Every other byte in
    # both reviewed files must survive, including formatting/comments/order.
    $removeName = ',(?<space>\s*)"' + [regex]::Escape($stageName) + '"'
    $nameMatches = [regex]::Matches($manifestAfter, $removeName)
    Assert-Check ($nameMatches.Count -eq 1) 'Manifest must gain exactly one stage-name entry.'
    Assert-Check ([regex]::new($removeName).Replace($manifestAfter, '', 1) -ceq $manifestBefore) 'Manifest changed beyond its one added stage name.'
    $removeAsset = '(?m)^[ \t]*' + [regex]::Escape($stageAsset) + '\r?\n'
    Assert-Check ([regex]::Matches($runtimeAfter, $removeAsset).Count -eq 1) 'Runtime list must gain exactly one new stage path.'
    Assert-Check ([regex]::new($removeAsset).Replace($runtimeAfter, '', 1) -ceq $runtimeBefore) 'Runtime list changed beyond its one added stage path.'
    $after | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'source-after-apply.json') -Encoding utf8
    Invoke-NativeCheck $tool @('--repo', $scratch, '--stage', $stage, '--apply') 'repeat-noop'
    Assert-Inventory $after (Get-Inventory $scratch)
    Invoke-NativeCheck $tool @('--repo', $scratch, '--stage', $stage) 'registered-preview-noop'
    Assert-Inventory $after (Get-Inventory $scratch)

    # A fresh configure proves the actual complete manifest/list agreement;
    # no compiler, engine submodule copy or full project rebuild is involved.
    $configured = Join-Path $run 'asset-configure'
    $configureArgs = @('-S', $scratch, '-B', $configured, '-G', 'Ninja')
    if ($cache.ContainsKey('CMAKE_MAKE_PROGRAM') -and $cache.CMAKE_MAKE_PROGRAM) {
        $configureArgs += "-DCMAKE_MAKE_PROGRAM=$($cache.CMAKE_MAKE_PROGRAM)"
    }
    Invoke-NativeCheck 'cmake' $configureArgs 'configure-registered-assets'
    $registeredAssetsFile = Join-Path $run 'registered-runtime-assets.txt'
    Invoke-NativeCheck 'cmake' @("-DRUNTIME_FILE=$runtimePath", "-DASSETS_FILE=$registeredAssetsFile",
        "-DOPTIONAL_FILE=$run/registered-optional-assets.txt", '-P', $exporter) 'enumerate-registered-assets'
    $registeredAssets = @(Get-Content -LiteralPath $registeredAssetsFile | Where-Object { $_ -ne '' })
    Assert-Check ($registeredAssets.Count -eq $assets.Count + 1 -and
        @($registeredAssets | Where-Object { $_ -ceq $stageAsset }).Count -eq 1 -and
        @($assets | Where-Object { $_ -cnotin $registeredAssets }).Count -eq 0) 'Registered runtime allowlist must add only the new stage.'
    $result.registered_asset_count = $registeredAssets.Count

    $freshBuild = Join-Path $run 'fresh-build'
    $staged = Join-Path $freshBuild 'game'
    [IO.Directory]::CreateDirectory($staged) | Out-Null
    Copy-EvidenceFile $game (Join-Path $staged 'ColourWars.exe')
    foreach ($dll in Get-ChildItem -LiteralPath (Split-Path -Parent $game) -Filter '*.dll' -File) {
        $sourceHashes[$dll.FullName] = Get-Hash $dll.FullName
        Copy-EvidenceFile $dll.FullName (Join-Path $staged $dll.Name)
    }
    $marker = Join-Path $staged 'registration-unrelated.keep'
    Write-Utf8 $marker 'Root files beside the game are not owned by content staging.'
    $shellBefore = Get-Inventory $staged
    Invoke-NativeCheck 'cmake' @("-DCONTENT_DIR=$content", "-DBUILD_DIR=$freshBuild",
        "-DSTAGE_DIR=$staged", '-P', (Join-Path $scratch 'cmake/stage_runtime_assets.cmake')) 'stage-registered-assets'
    $stagedInventory = Get-Inventory $staged
    foreach ($name in $shellBefore.Keys) {
        Assert-Check ($stagedInventory.Contains($name) -and
            $stagedInventory[$name].sha256 -ceq $shellBefore[$name].sha256) "Staging changed an unrelated game-root file: $name"
    }
    $presentAssets = @($registeredAssets | Where-Object { Test-Path -LiteralPath (Join-Path $content $_) -PathType Leaf })
    Assert-Check ($stagedInventory.Count -eq $shellBefore.Count + $presentAssets.Count) 'Fresh game contains unreviewed staged files.'
    foreach ($asset in $presentAssets) {
        Assert-Check ($stagedInventory.Contains($asset) -and
            $stagedInventory[$asset].sha256 -ceq (Get-Hash (Join-Path $content $asset))) "Staged runtime asset differs: $asset"
    }
    Assert-Check ($stagedInventory[$stageAsset].sha256 -ceq $before["game/content/$stageAsset"].sha256) 'The newly registered level was not staged byte for byte.'
    $stagedInventory | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'staged-assets.json') -Encoding utf8

    $startupOutput = Join-Path $run 'startup'
    $startupArgs = @{ Executable = Join-Path $staged 'ColourWars.exe'; OutputDirectory = $startupOutput; RequireAudio = $true }
    if ($Preset -eq 'x64-release') { $startupArgs.AllowNoHardwareAdapter = $true }
    & (Join-Path $PSScriptRoot 'check-startup.ps1') @startupArgs |
        Out-File -LiteralPath (Join-Path $run 'startup-script.log') -Encoding utf8
    $receipts = @(Get-ChildItem -LiteralPath $startupOutput -Filter result.json -Recurse -File)
    Assert-Check ($receipts.Count -eq 1) 'Expected exactly one fresh startup result receipt.'
    $startup = Get-Content -LiteralPath $receipts[0].FullName -Raw | ConvertFrom-Json
    Assert-Check ($startup.outcome -eq 'passed' -or
        ($Preset -eq 'x64-release' -and $startup.outcome -eq 'skipped-no-hardware-adapter')) 'Fresh registered game startup did not pass its required content checks.'
    $result.startup = [ordered]@{ receipt = $receipts[0].FullName; sha256 = Get-Hash $receipts[0].FullName; outcome = $startup.outcome }
    Assert-Inventory $after (Get-Inventory $scratch)
    $result.outcome = 'passed'
    $result.diagnostic = 'Scratch-only preview/apply/no-op/rejection, add-only file changes, existing CMake agreement and staging checks passed; startup outcome is recorded separately.'
}
catch {
    $result.diagnostic = $_.Exception.Message
    throw
}
finally {
    try {
        foreach ($file in $sourceHashes.Keys) {
            Assert-Check ((Get-Hash $file) -ceq $sourceHashes[$file]) "The real checkout/build input changed during this check: $file"
        }
        $result.source_unchanged = ($sourceHashes.Count -gt 0)
    }
    catch { $result.outcome = 'failed'; $result.diagnostic = $_.Exception.Message; throw }
    finally {
        $sourceHashes | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $run 'real-input-hashes.json') -Encoding utf8
        $result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8
        Pop-Location
        Write-Host "Stage registration evidence: $run"
    }
}
Write-Host "PASS stage registration integration ($Preset). Evidence: $run"
