[CmdletBinding()]
param([string] $OutputDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
. (Join-Path $PSScriptRoot 'check-path.ps1')
$painterRepo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$painterRunner = Join-Path $PSScriptRoot 'check-painter.ps1'
$selectionOutput = if ($OutputDirectory) { Resolve-CheckFileSystemPath $OutputDirectory } else {
    Join-Path $painterRepo 'out/validation/painter-selection'
}
$selectionRun = [IO.Path]::GetFullPath((Join-Path $selectionOutput ([Guid]::NewGuid().ToString('N'))))
$selectionPrefix = $selectionOutput.TrimEnd([char[]]@('\','/')) + [IO.Path]::DirectorySeparatorChar
if (-not $selectionRun.StartsWith($selectionPrefix, [StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $selectionRun)) {
    throw 'Painter selection scratch must be new and beneath its selected output root.'
}
[IO.Directory]::CreateDirectory($selectionRun) | Out-Null
if (([IO.File]::GetAttributes($selectionRun) -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'Painter selection scratch must be an ordinary directory.'
}
$selectionWorking = Join-Path $selectionRun 'working'
[IO.Directory]::CreateDirectory($selectionWorking) | Out-Null
$selectionUtf8 = [Text.UTF8Encoding]::new($false, $true)
$selectionInitialPath = $env:PATH
$selectionInitialProfile = $env:LOCALAPPDATA
$selectionInitialLocation = (Get-Location).Path
$selectionInitialProcessDirectory = [Environment]::CurrentDirectory
$selectionChecks = [Collections.Generic.List[object]]::new()
$selectionFailures = [Collections.Generic.List[string]]::new()
$selectionObserved = @{}
$missingCmake = 'Run this script from an x64 Visual Studio Developer shell (cmake is missing).'
$focusConflict = 'Choose only one focused painter matrix.'
$trialUnsupported = 'TrialPath is supported only with TurbulenceMoverOnly, TurbulenceShoulderOnly or TurbulenceSlabOnly.'
$unmapped = "Focused painter selector 'FutureUnmappedOnly' has no native scope argument."
$compileOnlySelector = 'CompileOnly compiles both pad readers and takes no focused painter selector.'
$trialSelectors = @('TurbulenceMoverOnly','TurbulenceShoulderOnly','TurbulenceSlabOnly')
$debugSelectors = @('TuneColoursOnly','TeamTuneColoursOnly','WeaponTuneColoursOnly')
$selectionResult = [ordered]@{
    schema_version=1; scope='actual-painter-focused-preflight'; outcome='running'; run=$selectionRun
    shell_version=$PSVersionTable.PSVersion.ToString(); shell_edition=$PSVersionTable.PSEdition
    started_utc=[DateTime]::UtcNow.ToString('o'); checks=$selectionChecks
    cmake_or_compiler_started=$false; native_probe_started=$false
    source_hashes=@(); focused_switches=@()
}

function Require-Selection([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw $Message }
}
function Save-SelectionResult {
    [IO.File]::WriteAllText((Join-Path $selectionRun 'result.json'),
        ($selectionResult | ConvertTo-Json -Depth 18), $selectionUtf8)
}
function Read-PainterAst([string] $Path) {
    $selectionTokens=$null
    $selectionErrors=$null
    $selectionAst=[Management.Automation.Language.Parser]::ParseFile($Path,[ref]$selectionTokens,[ref]$selectionErrors)
    Require-Selection (@($selectionErrors).Count -eq 0) "Painter source did not parse: $Path"
    return $selectionAst
}
# CompileOnly (CI#2) ends in Only but selects no matrix; it is checked separately.
function Get-OnlySwitches($Ast) {
    return @($Ast.ParamBlock.Parameters | Where-Object {
        $_.StaticType -eq [Management.Automation.SwitchParameter] -and $_.Name.VariablePath.UserPath -like '*Only' -and
        $_.Name.VariablePath.UserPath -cne 'CompileOnly'
    } | ForEach-Object { $_.Name.VariablePath.UserPath })
}
function Invoke-PainterSelection([string] $CaseName, [hashtable] $Options, [string] $RunnerPath,
    [string] $OutputArgument='', [string] $ExpectedOutput='') {
    # Dot-source in this private function scope. Initialize observations so the
    # unmodified baseline fails with the intended diagnostic, even in StrictMode.
    $focusedSelectors=@()
    $probeArguments=@()
    $painterCompileVariants=@()
    $trialResolved=$null
    $trialHashBefore=$null
    $trialHashAfterAttempted=$false
    $outputRoot=$null
    $selectionCaught=$null
    $selectionBeforePath=$env:PATH
    $selectionBeforeProfile=$env:LOCALAPPDATA
    $selectionBeforeLocation=(Get-Location).Path
    $selectionBeforeProcessDirectory=[Environment]::CurrentDirectory
    $selectionPrivateOutput=if ($ExpectedOutput) { $ExpectedOutput } else {
        Join-Path $selectionRun ('runner-' + $CaseName)
    }
    $selectionOptions=@{}
    foreach ($selectionKey in $Options.Keys) { $selectionOptions[$selectionKey]=$Options[$selectionKey] }
    $selectionOptions.OutputDirectory=if ($OutputArgument) { $OutputArgument } else { $selectionPrivateOutput }
    $selectionOptions.SkipBuild=$true
    try {
        $env:PATH=''
        foreach ($selectionTool in @('cmake','cl','git')) {
            Require-Selection (-not (Get-Command $selectionTool -ErrorAction SilentlyContinue)) `
                "Run qualification in a fresh -NoProfile host without a preexisting $selectionTool command."
        }
        try { . $RunnerPath @selectionOptions } catch { $selectionCaught=$_ }
        Require-Selection ($null -ne $selectionCaught) 'The actual painter runner did not stop during preflight.'
        $selectionActualOutput=if ($null -ne $outputRoot) { [string]$outputRoot } else { $selectionPrivateOutput }
        $selectionScratchPrefix=$selectionRun + [IO.Path]::DirectorySeparatorChar
        Require-Selection ($selectionActualOutput.StartsWith($selectionScratchPrefix,
            [StringComparison]::OrdinalIgnoreCase)) 'Painter preflight output escaped its private checker scratch.'
        $selectionFiles=@(if ([IO.Directory]::Exists($selectionActualOutput)) {
            Get-ChildItem -LiteralPath $selectionActualOutput -File -Recurse
        })
        # Valid routing reaches tool preflight after only the existing private
        # preference fixture is created. No probe/build evidence is allowed.
        foreach ($selectionFile in $selectionFiles) {
            Require-Selection ($selectionFile.FullName.EndsWith(
                [string][IO.Path]::DirectorySeparatorChar + 'profile' + [IO.Path]::DirectorySeparatorChar +
                'ColourWars' + [IO.Path]::DirectorySeparatorChar + 'save_data.json',
                [StringComparison]::OrdinalIgnoreCase) -and
                [IO.File]::ReadAllText($selectionFile.FullName) -ceq '{"aim_marker":false}') `
                'Painter selection preflight created an unexpected artifact.'
        }
        Require-Selection ($selectionFiles.Count -le 1) 'Painter preflight created more than its one isolated save fixture.'
        return [pscustomobject]@{
            diagnostic=$selectionCaught.Exception.Message
            selectors=[string[]]@($focusedSelectors)
            arguments=[string[]]@($probeArguments)
            compile_variants=[bool[]]@($painterCompileVariants)
            trial_resolved=$trialResolved
            trial_sha256=$trialHashBefore
            trial_after_attempted=$trialHashAfterAttempted
            output_directory=$selectionPrivateOutput
            output_created=[IO.Directory]::Exists($selectionPrivateOutput)
            actual_output_directory=$outputRoot
            actual_output_created=[IO.Directory]::Exists($selectionActualOutput)
            artifact_files=$selectionFiles.Count
        }
    } finally {
        $env:PATH=$selectionBeforePath
        $env:LOCALAPPDATA=$selectionBeforeProfile
        Set-Location -LiteralPath $selectionBeforeLocation
        [Environment]::CurrentDirectory=$selectionBeforeProcessDirectory
    }
}
function Check-PainterSelection([string] $CaseName, [hashtable] $Options, [string] $Diagnostic,
    [bool] $NoOutput=$false, [scriptblock] $Validate=$null, [string] $RunnerPath=$painterRunner) {
    $selectionRecord=[ordered]@{name=$CaseName;outcome='running'}
    $selectionChecks.Add($selectionRecord)
    try {
        $selectionObservation=Invoke-PainterSelection $CaseName $Options $RunnerPath
        $selectionRecord.observed=$selectionObservation
        $selectionObserved[$CaseName]=$selectionObservation
        Require-Selection ($selectionObservation.diagnostic -ceq $Diagnostic) `
            "Expected '$Diagnostic'; got '$($selectionObservation.diagnostic)'."
        if ($NoOutput) {
            Require-Selection (-not $selectionObservation.output_created -and $selectionObservation.artifact_files -eq 0) `
                'Refused selection created its output directory before failing.'
        } else {
            Require-Selection ($selectionObservation.output_created -and $selectionObservation.artifact_files -eq 1) `
                'Valid selection did not reach the existing private-save/tool preflight boundary.'
        }
        if ($null -ne $Validate) { & $Validate $selectionObservation }
        Require-Selection ([string]::Equals($env:PATH,$selectionInitialPath,[StringComparison]::Ordinal) -and
            [string]::Equals($env:LOCALAPPDATA,$selectionInitialProfile,[StringComparison]::Ordinal) -and
            (Get-Location).Path -ceq $selectionWorking -and
            [Environment]::CurrentDirectory -ceq $selectionWorking) 'Selection invocation did not restore caller environment/location.'
        $selectionRecord.outcome='passed'
        Write-Host "PASS painter selection: $CaseName"
    } catch {
        $selectionRecord.outcome='failed'
        $selectionRecord.diagnostic=$_.Exception.Message
        $selectionFailures.Add(('{0}: {1}' -f $CaseName,$_.Exception.Message))
    } finally { Save-SelectionResult }
}

function Check-PainterPaths([string] $CaseName, [bool] $AbsoluteOutput, [string] $TrialSelector='') {
    $pathRecord=[ordered]@{name=$CaseName;outcome='running'}
    $selectionChecks.Add($pathRecord)
    $pathBeforeLocation=(Get-Location).Path
    $pathBeforeProcessDirectory=[Environment]::CurrentDirectory
    $pathBeforeEnvironment=$env:PATH
    $pathBeforeProfile=$env:LOCALAPPDATA
    try {
        # Keep the private process directory below Windows' current-directory
        # length limit even when the caller's evidence root is already nested.
        $caseRoot=Join-Path $selectionRun ('path-' + $selectionChecks.Count)
        $providerDirectory=Join-Path $caseRoot ('provider space ' + [char]0x03A9)
        $processDirectory=Join-Path $caseRoot ('process space ' + [char]0x03A9)
        foreach ($directory in @($providerDirectory,$processDirectory)) {
            [IO.Directory]::CreateDirectory($directory) | Out-Null
        }
        $outputLeaf='evidence space ' + [char]0x03A9
        $expectedOutput=if ($AbsoluteOutput) { Join-Path $caseRoot $outputLeaf } else { Join-Path $providerDirectory $outputLeaf }
        $wrongOutput=Join-Path $processDirectory $outputLeaf
        $outputArgument=if ($AbsoluteOutput) { $expectedOutput } else { $outputLeaf }
        $pathOptions=@{IdentityOnly=$true}
        $providerTrial=$null
        $processTrial=$null
        $providerDigest=$null
        $processDigest=$null
        if ($TrialSelector) {
            $trialLeaf='private trial ' + [char]0x03A9 + '.json'
            $providerTrial=Join-Path $providerDirectory $trialLeaf
            $processTrial=Join-Path $processDirectory $trialLeaf
            [IO.File]::WriteAllText($providerTrial,'{"qualification":"provider trial"}',$selectionUtf8)
            [IO.File]::WriteAllText($processTrial,'{"qualification":"process decoy"}',$selectionUtf8)
            $providerDigest=(Get-FileHash -LiteralPath $providerTrial -Algorithm SHA256).Hash
            $processDigest=(Get-FileHash -LiteralPath $processTrial -Algorithm SHA256).Hash
            $pathOptions=@{TrialPath=$trialLeaf}
            $pathOptions[$TrialSelector]=$true
        }
        Set-Location -LiteralPath $providerDirectory
        [Environment]::CurrentDirectory=$processDirectory
        $pathRecord.provider_directory=$providerDirectory
        $pathRecord.process_directory=$processDirectory
        $pathRecord.output_argument=$outputArgument
        $pathRecord.expected_output=$expectedOutput
        $pathRecord.process_relative_output=$wrongOutput
        $pathObservation=Invoke-PainterSelection $CaseName $pathOptions $painterRunner `
            -OutputArgument $outputArgument -ExpectedOutput $expectedOutput
        $pathRecord.observed=$pathObservation
        $pathRecord.process_relative_output_created=[IO.Directory]::Exists($wrongOutput)
        $pathRecord.expected_trial=$providerTrial
        $pathRecord.expected_trial_sha256=$providerDigest
        $pathRecord.process_trial_sha256=$processDigest
        Require-Selection ($pathObservation.diagnostic -ceq $missingCmake) `
            "Path preflight did not reach missing CMake: $($pathObservation.diagnostic)"
        Require-Selection ($pathObservation.actual_output_directory -ceq $expectedOutput -and
            $pathObservation.output_created -and -not $pathRecord.process_relative_output_created) `
            'HYG17 painter OutputDirectory resolved against process cwd instead of caller PowerShell location.'
        Require-Selection ($pathObservation.artifact_files -eq 1) 'Painter path preflight lost its single private save fixture.'
        if ($TrialSelector) {
            Require-Selection ($providerDigest -cne $processDigest -and
                $pathObservation.trial_resolved -ceq $providerTrial -and
                $pathObservation.trial_sha256 -ceq $providerDigest -and
                $pathObservation.arguments.Count -eq 2 -and
                $pathObservation.arguments[1] -ceq ('"' + $providerTrial + '"') -and
                -not $pathObservation.trial_after_attempted) `
                'HYG17 painter relative TrialPath did not retain the provider file, initial hash and quoted native argument.'
            Require-Selection ((Get-FileHash -LiteralPath $providerTrial -Algorithm SHA256).Hash -ceq $providerDigest -and
                (Get-FileHash -LiteralPath $processTrial -Algorithm SHA256).Hash -ceq $processDigest) `
                'Painter path preflight changed private provider/decoy trial bytes.'
        }
        Require-Selection ((Get-Location).Path -ceq $providerDirectory -and
            [Environment]::CurrentDirectory -ceq $processDirectory -and
            [string]::Equals($env:PATH,$pathBeforeEnvironment,[StringComparison]::Ordinal) -and
            [string]::Equals($env:LOCALAPPDATA,$pathBeforeProfile,[StringComparison]::Ordinal)) `
            'Painter path preflight changed caller environment or either current directory.'
        $pathRecord.outcome='passed'
        Write-Host "PASS painter selection: $CaseName"
    } catch {
        $pathRecord.outcome='failed'
        $pathRecord.diagnostic=$_.Exception.Message
        $selectionFailures.Add(('{0}: {1}' -f $CaseName,$_.Exception.Message))
    } finally {
        $env:PATH=$pathBeforeEnvironment
        $env:LOCALAPPDATA=$pathBeforeProfile
        Set-Location -LiteralPath $pathBeforeLocation
        [Environment]::CurrentDirectory=$pathBeforeProcessDirectory
        Save-SelectionResult
    }
}

Save-SelectionResult
Write-Host "Painter-selection evidence: $selectionRun"
try {
    $sourcePaths=@($PSCommandPath,$painterRunner,
        (Join-Path $PSScriptRoot 'check-painter/hash.ps1'),
        (Join-Path $PSScriptRoot 'check-painter/contact-sheet.ps1'))
    $pathHelper=Join-Path $PSScriptRoot 'check-path.ps1'
    if (Test-Path -LiteralPath $pathHelper -PathType Leaf) { $sourcePaths += $pathHelper }
    $selectionResult.source_hashes=@(foreach ($sourcePath in $sourcePaths) {
        [ordered]@{path=$sourcePath;sha256=(Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash}
    })
    $actualAst=Read-PainterAst $painterRunner
    $onlySwitches=@(Get-OnlySwitches $actualAst)
    Require-Selection ($onlySwitches.Count -gt 0 -and
        @($onlySwitches | Sort-Object -Unique).Count -eq $onlySwitches.Count) 'Focused switch declarations are missing or repeated.'
    $selectionResult.focused_switches=$onlySwitches
    $selectionResult.focused_switch_count=$onlySwitches.Count
    foreach ($policyName in @($trialSelectors)+@($debugSelectors)) {
        Require-Selection ($onlySwitches -ccontains $policyName) "Expected policy selector is not an actual switch: $policyName"
    }

    # Only the parameter AST is changed in this fault copy. Original runner and
    # preflight dependencies stay untouched; only required helpers are copied.
    $mirrorScripts=Join-Path $selectionRun 'fault-workspace/scripts'
    $mirrorHelpers=Join-Path $mirrorScripts 'check-painter'
    [IO.Directory]::CreateDirectory($mirrorHelpers) | Out-Null
    $faultRunner=Join-Path $mirrorScripts 'check-painter.ps1'
    $sourceText=[IO.File]::ReadAllText($painterRunner)
    $outputParameter=@($actualAst.ParamBlock.Parameters | Where-Object { $_.Name.VariablePath.UserPath -ceq 'OutputDirectory' })
    Require-Selection ($outputParameter.Count -eq 1) 'Could not locate the actual runner output parameter for fault injection.'
    $insertionOffset=$outputParameter[0].Extent.StartOffset
    $insertion='[switch] $FutureUnmappedOnly,' + [Environment]::NewLine + '    '
    $faultText=$sourceText.Insert($insertionOffset,$insertion)
    Require-Selection ($faultText.Remove($insertionOffset,$insertion.Length) -ceq $sourceText) `
        'The synthetic selector changed source outside its one parameter insertion.'
    $sourceBytes=[IO.File]::ReadAllBytes($painterRunner)
    $sourceHasBom=$sourceBytes.Length -ge 3 -and $sourceBytes[0] -eq 239 -and $sourceBytes[1] -eq 187 -and $sourceBytes[2] -eq 191
    [IO.File]::WriteAllText($faultRunner,$faultText,[Text.UTF8Encoding]::new($sourceHasBom))
    foreach ($helper in @('hash.ps1','contact-sheet.ps1')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot ('check-painter/' + $helper)) -Destination (Join-Path $mirrorHelpers $helper)
    }
    if (Test-Path -LiteralPath $pathHelper -PathType Leaf) {
        Copy-Item -LiteralPath $pathHelper -Destination (Join-Path $mirrorScripts 'check-path.ps1')
    }
    $faultAst=Read-PainterAst $faultRunner
    $faultSwitches=@(Get-OnlySwitches $faultAst)
    Require-Selection ($faultSwitches.Count -eq $onlySwitches.Count+1 -and
        $faultSwitches -ccontains 'FutureUnmappedOnly') 'The fault copy did not add exactly one focused switch.'
    $selectionResult.fault_copy=[ordered]@{
        path=$faultRunner;sha256=(Get-FileHash -LiteralPath $faultRunner -Algorithm SHA256).Hash
        added_parameter='FutureUnmappedOnly';only_parameter_inserted=$true
    }
    Set-Location -LiteralPath $selectionWorking
    # GetFullPath uses the process directory, which PowerShell's provider
    # location does not consistently update across supported hosts.
    [Environment]::CurrentDirectory=$selectionWorking

    # First baseline assertion: old source reaches missing-cmake and creates
    # output. It must refuse the injected bound selector before that boundary.
    Check-PainterSelection 'synthetic-unmapped-true' @{FutureUnmappedOnly=$true} $unmapped $true $null $faultRunner
    Require-Selection ($selectionChecks[0].outcome -ceq 'passed') `
        'The actual synthetic unmapped selector was not refused before tool/output preflight.'

    $trialPath=Join-Path $selectionWorking ('private trial ' + [char]0x03A9 + '.json')
    [IO.File]::WriteAllText($trialPath,'{"qualification":"selection preflight only"}',$selectionUtf8)
    $trialBytes=[IO.File]::ReadAllBytes($trialPath)
    $trialDigest=(Get-FileHash -LiteralPath $trialPath -Algorithm SHA256).Hash
    $selectionResult.private_trial=[ordered]@{path=$trialPath;sha256=$trialDigest;bytes=$trialBytes.Length}
    $tokensBySelector=@{}
    foreach ($selectorName in $onlySwitches) {
        $selectorOptions=@{}
        $selectorOptions[$selectorName]=$true
        $isTrial=$trialSelectors -ccontains $selectorName
        if ($isTrial) { $selectorOptions.TrialPath=$trialPath }
        # Independent naming grammar catches swapped mappings without a second
        # 100-row table. Native selectors are plain tokens, not command options.
        $expectedToken=[regex]::Replace($selectorName.Substring(0,$selectorName.Length-4),
            '(?<=[a-z0-9])(?=[A-Z])','-').ToLowerInvariant()
        Check-PainterSelection ('selector-' + $selectorName) $selectorOptions $missingCmake $false {
            param($actual)
            Require-Selection ($actual.selectors.Count -eq 1 -and $actual.selectors[0] -ieq $selectorName) `
                'The actual bound focused selector was not preserved.'
            Require-Selection ($actual.arguments.Count -eq $(if ($isTrial) {2} else {1}) -and
                $actual.arguments[0] -ceq $expectedToken) 'A focused selector did not produce its one exact native flag.'
            if ($isTrial) {
                Require-Selection ($actual.arguments[1] -ceq ('"' + [IO.Path]::GetFullPath($trialPath) + '"') -and
                    $actual.trial_resolved -ceq [IO.Path]::GetFullPath($trialPath) -and
                    $actual.trial_sha256 -ceq $trialDigest -and -not $actual.trial_after_attempted) `
                    'Trial preflight changed its quoted path, resolved path, initial hash or launch boundary.'
            }
            Require-Selection ((Get-FileHash -LiteralPath $trialPath -Algorithm SHA256).Hash -ceq $trialDigest) `
                'Selector preflight changed the private trial bytes.'
        }
        if ($selectionObserved.ContainsKey('selector-' + $selectorName)) {
            $observedArguments=$selectionObserved['selector-' + $selectorName].arguments
            if ($observedArguments.Count -gt 0) { $tokensBySelector[$selectorName]=$observedArguments[0] }
        }
    }
    $selectionResult.native_tokens=$tokensBySelector
    Require-Selection ($tokensBySelector.Count -eq $onlySwitches.Count -and
        @($tokensBySelector.Values | Sort-Object -Unique).Count -eq $onlySwitches.Count) `
        'Focused native flags are missing or duplicated across actual switch declarations.'

    foreach ($presetName in @('x64-debug','x64-release')) {
        Check-PainterSelection ('default-' + $presetName) @{Preset=$presetName} $missingCmake $false {
            param($actual)
            Require-Selection ($actual.selectors.Count -eq 0 -and $actual.arguments.Count -eq 0) `
                'Default routing acquired a focused native flag.'
        }
    }
    $allFalse=@{Preset='x64-release'}
    foreach ($selectorName in $onlySwitches) { $allFalse[$selectorName]=$false }
    Check-PainterSelection 'all-explicit-false' $allFalse $missingCmake $false {
        param($actual)
        Require-Selection ($actual.selectors.Count -eq 0 -and $actual.arguments.Count -eq 0) `
            'Explicit false focused switches were treated as active.'
    }
    Check-PainterSelection 'false-plus-true' @{IdentityOnly=$false;QuietOnly=$true} $missingCmake $false {
        param($actual)
        Require-Selection ($actual.selectors.Count -eq 1 -and $actual.selectors[0] -ieq 'QuietOnly' -and
            $actual.arguments.Count -eq 1 -and $actual.arguments[0] -ceq 'quiet') 'A false switch interfered with the one active selector.'
    }
    Check-PainterSelection 'case-insensitive-binding' @{iDeNtItYoNlY=$true} $missingCmake $false {
        param($actual)
        Require-Selection ($actual.selectors.Count -eq 1 -and $actual.selectors[0] -ieq 'IdentityOnly' -and
            $actual.arguments.Count -eq 1 -and $actual.arguments[0] -ceq 'identity') 'Focused parameter binding became case-sensitive.'
    }
    Check-PainterSelection 'two-focused-conflict' @{IdentityOnly=$true;QuietOnly=$true} $focusConflict $true

    # CI#2: CompileOnly is no focused selector, compiles both readers and
    # refuses a focused matrix before any output or tool preflight.
    $compileSwitch=@($actualAst.ParamBlock.Parameters | Where-Object {
        $_.Name.VariablePath.UserPath -ceq 'CompileOnly' -and $_.StaticType -eq [Management.Automation.SwitchParameter]
    })
    Require-Selection ($compileSwitch.Count -eq 1) 'The actual painter runner has no CompileOnly switch.'
    foreach ($presetName in @('x64-debug','x64-release')) {
        Check-PainterSelection ('compile-only-' + $presetName) @{Preset=$presetName;CompileOnly=$true} $missingCmake $false {
            param($actual)
            Require-Selection ($actual.selectors.Count -eq 0 -and $actual.arguments.Count -eq 0) `
                'CompileOnly became a focused selector or acquired a native flag.'
            Require-Selection ($actual.compile_variants.Count -eq 2 -and -not $actual.compile_variants[0] -and
                $actual.compile_variants[1]) 'CompileOnly did not select the normal then the scripted pad reader.'
        }
    }
    Check-PainterSelection 'default-one-normal-reader' @{IdentityOnly=$true} $missingCmake $false {
        param($actual)
        Require-Selection ($actual.compile_variants.Count -eq 1 -and -not $actual.compile_variants[0]) `
            'A normal-reader matrix did not keep its one normal compile.'
    }
    Check-PainterSelection 'default-one-scripted-reader' @{RumbleOnly=$true} $missingCmake $false {
        param($actual)
        Require-Selection ($actual.compile_variants.Count -eq 1 -and $actual.compile_variants[0]) `
            'A scripted-reader matrix did not keep its one scripted compile.'
    }
    Check-PainterSelection 'compile-only-false' @{CompileOnly=$false;QuietOnly=$true} $missingCmake $false {
        param($actual)
        Require-Selection ($actual.selectors.Count -eq 1 -and $actual.arguments.Count -eq 1 -and
            $actual.compile_variants.Count -eq 1) 'An explicitly false CompileOnly changed focused routing.'
    }
    Check-PainterSelection 'compile-only-with-selector' @{CompileOnly=$true;IdentityOnly=$true} $compileOnlySelector $true
    Check-PainterSelection 'compile-only-with-two-selectors' @{CompileOnly=$true;IdentityOnly=$true;QuietOnly=$true} $focusConflict $true
    Check-PainterSelection 'synthetic-false-default' @{FutureUnmappedOnly=$false} $missingCmake $false {
        param($actual)
        Require-Selection ($actual.selectors.Count -eq 0 -and $actual.arguments.Count -eq 0) 'An explicitly false unmapped switch became active.'
    } $faultRunner
    Check-PainterSelection 'synthetic-false-plus-real' @{FutureUnmappedOnly=$false;IdentityOnly=$true} $missingCmake $false {
        param($actual)
        Require-Selection ($actual.selectors.Count -eq 1 -and $actual.arguments.Count -eq 1 -and
            $actual.arguments[0] -ceq 'identity') 'A false unmapped switch changed real focused routing.'
    } $faultRunner
    Check-PainterSelection 'synthetic-true-plus-real' @{FutureUnmappedOnly=$true;IdentityOnly=$true} $focusConflict $true $null $faultRunner

    Check-PainterSelection 'trial-without-selector' @{TrialPath=$trialPath} $trialUnsupported $true
    Check-PainterSelection 'trial-with-false-selector' @{TrialPath=$trialPath;TurbulenceMoverOnly=$false} $trialUnsupported $true
    Check-PainterSelection 'trial-with-nontrial-selector' @{TrialPath=$trialPath;IdentityOnly=$true} $trialUnsupported $true
    foreach ($trialSelector in $trialSelectors) {
        $trialOptions=@{}
        $trialOptions[$trialSelector]=$true
        Check-PainterSelection ($trialSelector + '-missing-trial') $trialOptions "$trialSelector requires an explicit TrialPath." $true
        $trialOptions.TrialPath=' '
        Check-PainterSelection ($trialSelector + '-blank-trial') $trialOptions "$trialSelector requires an explicit TrialPath." $true
        $trialOptions.TrialPath=Join-Path $selectionWorking 'absent.json'
        Check-PainterSelection ($trialSelector + '-absent-trial') $trialOptions `
            ("TrialPath is not a readable existing file: " + $trialOptions.TrialPath) $true
        $trialOptions.TrialPath=[IO.Path]::GetFileName($trialPath)
        Check-PainterSelection ($trialSelector + '-relative-trial') $trialOptions $missingCmake $false {
            param($actual)
            Require-Selection ($actual.arguments.Count -eq 2 -and $actual.arguments[1] -ceq ('"' + $trialPath + '"') -and
                $actual.trial_resolved -ceq $trialPath -and $actual.trial_sha256 -ceq $trialDigest) `
                'Relative private trial routing did not preserve its one quoted canonical path/hash.'
        }
    }
    Check-PainterSelection 'trial-missing-before-conflict' @{TurbulenceMoverOnly=$true;IdentityOnly=$true} `
        'TurbulenceMoverOnly requires an explicit TrialPath.' $true
    Check-PainterSelection 'trial-name-precedence' @{TurbulenceMoverOnly=$true;TurbulenceShoulderOnly=$true;TurbulenceSlabOnly=$true} `
        'TurbulenceSlabOnly requires an explicit TrialPath.' $true
    Check-PainterSelection 'valid-trials-before-conflict' @{TurbulenceMoverOnly=$true;TurbulenceShoulderOnly=$true;TrialPath=$trialPath} `
        $focusConflict $true {
            param($actual)
            Require-Selection ($actual.trial_resolved -ceq $trialPath -and $actual.trial_sha256 -ceq $trialDigest) `
                'Trial validation/hash no longer precedes the focused conflict.'
        }
    foreach ($debugSelector in $debugSelectors) {
        $debugOptions=@{Preset='x64-release'}
        $debugOptions[$debugSelector]=$true
        $debugError="$debugSelector requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance."
        Check-PainterSelection ($debugSelector + '-release') $debugOptions $debugError $true
        $debugOptions.IdentityOnly=$true
        Check-PainterSelection ($debugSelector + '-before-conflict') $debugOptions $debugError $true
    }
    Check-PainterSelection 'debug-refusal-order' @{Preset='x64-release';TuneColoursOnly=$true;TeamTuneColoursOnly=$true;WeaponTuneColoursOnly=$true} `
        'TuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.' $true
    Check-PainterSelection 'team-before-weapon-refusal' @{Preset='x64-release';TuneColoursOnly=$false;TeamTuneColoursOnly=$true;WeaponTuneColoursOnly=$true} `
        'TeamTuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.' $true
    Check-PainterSelection 'trial-before-debug-refusal' @{Preset='x64-release';TurbulenceMoverOnly=$true;TuneColoursOnly=$true} `
        'TurbulenceMoverOnly requires an explicit TrialPath.' $true
    Check-PainterSelection 'valid-trial-before-debug-refusal' @{Preset='x64-release';TurbulenceMoverOnly=$true;TrialPath=$trialPath;TuneColoursOnly=$true} `
        'TuneColoursOnly requires x64-debug; Release developer-tool exclusion is checked by CPU acceptance.' $true {
            param($actual)
            Require-Selection ($actual.trial_resolved -ceq $trialPath -and $actual.trial_sha256 -ceq $trialDigest) `
                'Trial validation/hash no longer precedes the Release developer-tool refusal.'
        }
    Require-Selection ((Get-FileHash -LiteralPath $trialPath -Algorithm SHA256).Hash -ceq $trialDigest) 'Qualification changed private trial bytes.'
    Check-PainterSelection 'omit-worktree-status-true' @{IdentityOnly=$true;OmitWorktreeStatus=$true} $missingCmake $false
    Check-PainterSelection 'omit-worktree-status-false' @{IdentityOnly=$true;OmitWorktreeStatus=$false} $missingCmake $false
    Check-PainterPaths 'relative-output-provider-location' $false
    Check-PainterPaths 'absolute-output-independent-location' $true
    foreach ($trialSelector in $trialSelectors) {
        Check-PainterPaths ($trialSelector + '-relative-path-provider-location') $true $trialSelector
    }
    Require-Selection ($selectionFailures.Count -eq 0) ("Painter selection failed:" + [Environment]::NewLine + ($selectionFailures -join [Environment]::NewLine))
    $selectionResult.outcome='passed'
} catch {
    $selectionResult.outcome='failed'
    $selectionResult.diagnostic=$_.Exception.Message
    throw
} finally {
    $env:PATH=$selectionInitialPath
    $env:LOCALAPPDATA=$selectionInitialProfile
    Set-Location -LiteralPath $selectionInitialLocation
    [Environment]::CurrentDirectory=$selectionInitialProcessDirectory
    $selectionResult.caller_path_restored=[string]::Equals($env:PATH,$selectionInitialPath,[StringComparison]::Ordinal)
    $selectionResult.caller_profile_restored=[string]::Equals($env:LOCALAPPDATA,$selectionInitialProfile,[StringComparison]::Ordinal)
    $selectionResult.caller_location_restored=(Get-Location).Path -ceq $selectionInitialLocation
    $selectionResult.caller_process_directory_restored=[Environment]::CurrentDirectory -ceq $selectionInitialProcessDirectory
    $selectionResult.sources_unchanged=$true
    foreach ($sourceHash in $selectionResult.source_hashes) {
        if ((Get-FileHash -LiteralPath $sourceHash.path -Algorithm SHA256).Hash -cne $sourceHash.sha256) {
            $selectionResult.sources_unchanged=$false
            $selectionResult.outcome='failed'
        }
    }
    $selectionResult.completed_utc=[DateTime]::UtcNow.ToString('o')
    Save-SelectionResult
    Write-Host "Painter-selection evidence: $selectionRun"
}
Require-Selection ($selectionResult.outcome -ceq 'passed') 'Painter qualification did not retain unchanged source files.'
Write-Host 'PASS painter selection without CMake, compiler or native probe startup.'
