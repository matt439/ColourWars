[CmdletBinding()]
param(
    [ValidateSet('x64-debug','x64-release')][string]$Preset='x64-debug',
    [switch]$SkipBuild,
    [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$OutputDirectory) { $OutputDirectory=Join-Path $repo 'out/validation/display-awake' }
$run=Join-Path (Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) $Preset) ([Guid]::NewGuid().ToString('N'))
$working=Join-Path $run 'working';$profile=Join-Path $run 'profile';$evidence=Join-Path $run 'evidence'
foreach($directory in @($run,$working,$profile,$evidence,(Join-Path $profile 'ColourWars'))) { [IO.Directory]::CreateDirectory($directory)|Out-Null }
$save=Join-Path $profile 'ColourWars/save_data.json'
[IO.File]::WriteAllText($save,'{"aim_marker":false}',[Text.UTF8Encoding]::new($false));$saveHash=(Get-FileHash $save).Hash
function Quote-LiteralPath([string]$Path) {
    if($Path.Contains('"') -or $Path.Contains("`r") -or $Path.Contains("`n")){throw 'Unsupported quote/newline in path'}
    return '"'+[IO.Path]::GetFullPath($Path)+'"'
}
function Invoke-NativeLogged([string]$Command,[string[]]$Arguments,[string]$Log) {
    $savedPreference=$ErrorActionPreference;$ErrorActionPreference='Continue'
    try {$output=& $Command @Arguments 2>&1;$code=$LASTEXITCODE;$output|Out-File -LiteralPath $Log -Encoding utf8}
    finally {$ErrorActionPreference=$savedPreference}
    if($code -ne 0){throw "$Command exited $code; see $Log"}
}
function Assert-ExactKeys($Object,[string[]]$Keys) {
    if($Object -isnot [pscustomobject] -or (($Object.PSObject.Properties.Name|Sort-Object)-join'|') -cne (($Keys|Sort-Object)-join'|')){throw 'Probe summary schema differs'}
}
$previousPath=$env:PATH;$previousProfile=$env:LOCALAPPDATA;$process=$null;$originalFailure=$null
$result=[ordered]@{scope='actual-GameLevel-display-awake-lifecycle';outcome='running';preset=$Preset;run=$run;started_utc=[DateTime]::UtcNow.ToString('o');GPU_reads=0;crops=0;real_display_requests=0;OS_setting_changes=0}
function Save-LifecycleResult { $result|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding utf8 }
Save-LifecycleResult
Write-Host "Display-awake evidence: $run"
try {
    foreach($tool in @('cmake','cl','git','dumpbin')){if(!(Get-Command $tool -ErrorAction SilentlyContinue)){throw "Missing $tool; use x64 VS Developer environment"}}
    if(!$env:INCLUDE -or !$env:LIB -or ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64')){throw 'x64 VS Developer environment required'}
    $result.base_revision=(git -C $repo rev-parse HEAD).Trim()
    Push-Location -LiteralPath $repo
    try {
        if(!$SkipBuild){Invoke-NativeLogged 'cmake' @('--preset',$Preset) (Join-Path $run 'configure.log');Invoke-NativeLogged 'cmake' @('--build','--preset',$Preset) (Join-Path $run 'build.log')}
        $build=Join-Path $repo "out/build/$Preset";$cache=@{}
        foreach($line in Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt')){if($line -match '^([^/#][^:]*):[^=]*=(.*)$'){$cache[$Matches[1]]=$Matches[2]}}
        $debug=$Preset -eq 'x64-debug';$configuration=if($debug){'Debug'}else{'Release'}
        foreach($key in @('CMAKE_HOME_DIRECTORY','CMAKE_BUILD_TYPE','CMAKE_CXX_COMPILER','LABRADOR_RENDER_BACKEND','LABRADOR_AUDIO_BACKEND','VCPKG_INSTALLED_DIR','VCPKG_TARGET_TRIPLET')){if(!$cache.ContainsKey($key)){throw "Missing configured $key"}}
        if(![StringComparer]::OrdinalIgnoreCase.Equals([IO.Path]::GetFullPath($cache.CMAKE_HOME_DIRECTORY),$repo) -or $cache.CMAKE_BUILD_TYPE -ne $configuration -or $cache.LABRADOR_RENDER_BACKEND -ne 'd3d11' -or $cache.LABRADOR_AUDIO_BACKEND -ne 'xaudio2' -or $cache.VCPKG_TARGET_TRIPLET -ne 'x64-windows'){throw 'Wrong checkout/configuration/backend/triplet'}
        $game=Join-Path $build 'game';$objectDirectory=Join-Path $game 'CMakeFiles/ColourWars.dir'
        $objects=@(Get-ChildItem -LiteralPath $objectDirectory -Filter '*.obj' -File -Recurse|Where-Object{
            if($_.Name -eq 'cmake_pch.cxx.obj'){return $true}
            $relative=$_.FullName.Substring($objectDirectory.Length+1);$sourceRelative=$relative.Substring(0,$relative.Length-4)
            Test-Path -LiteralPath (Join-Path (Join-Path $repo 'game') $sourceRelative) -PathType Leaf
        }|Sort-Object FullName)
        $entry=@($objects|Where-Object{$_.Name -ceq 'main.cpp.obj'});$platform=@($objects|Where-Object{$_.Name -ceq 'display_awake.cpp.obj'})
        if($entry.Count -ne 1 -or $platform.Count -ne 1 -or $platform[0].FullName.Substring($objectDirectory.Length+1) -cne 'display_awake.cpp.obj'){throw 'Expected exactly the normal entrypoint and one thin display_awake.cpp.obj'}
        $linked=@($objects|Where-Object{$_.FullName -cne $entry[0].FullName -and $_.FullName -cne $platform[0].FullName})
        if(@($linked|Where-Object{$_.Name -ceq 'game_states.cpp.obj'}).Count -ne 1){throw 'Actual GameLevel object is absent'}
        $result.object_partition=[ordered]@{ordinary_replaced_entrypoint=[ordered]@{path=$entry[0].FullName;sha256=(Get-FileHash $entry[0].FullName).Hash};sole_additional_omission=[ordered]@{path=$platform[0].FullName;sha256=(Get-FileHash $platform[0].FullName).Hash;source='game/display_awake.cpp';source_sha256=(Get-FileHash (Join-Path $repo 'game/display_awake.cpp')).Hash};linked_count=$linked.Count;linked=@($linked|ForEach-Object{[ordered]@{path=$_.FullName;sha256=(Get-FileHash $_.FullName).Hash}})}
        $installed=Join-Path $cache.VCPKG_INSTALLED_DIR $cache.VCPKG_TARGET_TRIPLET;$dep=if($debug){Join-Path $installed 'debug/lib'}else{Join-Path $installed 'lib'}
        $libraries=@((Join-Path $build 'external/labrador/engine/LabradorEngine.lib'),(Join-Path $build 'external/labrador/engine/math/MattMath.lib'),(Join-Path $dep 'DirectXTK.lib'),(Join-Path $dep 'xaudio2_9redist.lib'))
        $source=Join-Path $PSScriptRoot 'acceptance/display_awake_lifecycle.cpp';$exe=Join-Path $run 'display_awake_lifecycle.exe';$rsp=Join-Path $run 'compile.rsp'
        $sourcePaths=@('game/display_awake.h','game/display_awake.cpp','game/states/game_states.h','game/states/game_states.cpp','scripts/acceptance/display_awake_lifecycle.cpp','scripts/check-display-awake.ps1')
        $result.source_hashes=@(foreach($path in $sourcePaths){[ordered]@{path=$path;sha256=(Get-FileHash (Join-Path $repo $path)).Hash}})
        $result.library_hashes=@(foreach($path in $libraries){[ordered]@{path=$path;sha256=(Get-FileHash $path).Hash}})
        $contentPaths=@('manifest.json','tuning.json','presentation.json','levels/movement_course.json')
        $result.content_hashes=@(foreach($path in $contentPaths){[ordered]@{path=$path;sha256=(Get-FileHash (Join-Path $game $path)).Hash}})
        $arguments=@('/nologo','/EHsc','/std:c++20','/W4','/WX','/permissive-','/DDIRECTX_TOOLKIT_IMPORT','/DUSING_XAUDIO2_REDIST','/DUSING_XINPUT','/DUNICODE','/D_UNICODE','/external:W0')
        if($debug){$arguments+='/MDd'}else{$arguments+=@('/MD','/DNDEBUG')}
        foreach($include in @($repo,(Join-Path $repo 'external/labrador'),(Join-Path $build 'generated'))){$arguments+='/I'+(Quote-LiteralPath $include)}
        foreach($include in @((Join-Path $repo 'external/labrador/external/rapidjson/include'),(Join-Path $installed 'include/directxtk'),(Join-Path $installed 'include/xaudio2redist'),(Join-Path $installed 'include'))){$arguments+='/external:I'+(Quote-LiteralPath $include)}
        $arguments+=@((Quote-LiteralPath $source),('/Fe:'+(Quote-LiteralPath $exe)),('/Fo:'+(Quote-LiteralPath (Join-Path $run 'display_awake_lifecycle.obj'))))
        $arguments+=@($linked|ForEach-Object{Quote-LiteralPath $_.FullName});$arguments+=@($libraries|ForEach-Object{Quote-LiteralPath $_})
        $arguments+=@('d3d11.lib','dxgi.lib','dxguid.lib','kernel32.lib','user32.lib','gdi32.lib','winspool.lib','shell32.lib','ole32.lib','oleaut32.lib','uuid.lib','comdlg32.lib','advapi32.lib')
        $arguments|Out-File -LiteralPath $rsp -Encoding unicode
        Invoke-NativeLogged $cache.CMAKE_CXX_COMPILER @("@$rsp") (Join-Path $run 'compile.log')
        Invoke-NativeLogged 'dumpbin' @('/nologo','/imports',$exe) (Join-Path $run 'imports.log')
        if(Select-String -LiteralPath (Join-Path $run 'imports.log') -Pattern '\bSetThreadExecutionState\b' -Quiet){throw 'Recording probe unexpectedly imports the real display-idle API'}
        $result.real_idle_import_absent=$true;$result.probe_sha256=(Get-FileHash $exe).Hash;$result.response_file_sha256=(Get-FileHash $rsp).Hash
        $env:PATH="$game;$previousPath";$env:LOCALAPPDATA=$profile
        try {$process=Start-Process -FilePath $exe -WindowStyle Hidden -PassThru -WorkingDirectory $working -ArgumentList @((Quote-LiteralPath $game),(Quote-LiteralPath $evidence)) -RedirectStandardOutput (Join-Path $run 'run.log') -RedirectStandardError (Join-Path $run 'run.err');$null=$process.Handle;$result.process_id=$process.Id;$result.probe_executable=$exe;$result.process_started_utc=[DateTime]::UtcNow.ToString('o');Save-LifecycleResult}
        finally {$env:PATH=$previousPath;$env:LOCALAPPDATA=$previousProfile}
        $timer=[Diagnostics.Stopwatch]::StartNew()
        while(!$process.WaitForExit(1000)){if($timer.Elapsed.TotalSeconds -gt 180){throw 'Owned lifecycle probe exceeded180 seconds'}}
        $process.WaitForExit();$result.process_id=$process.Id;$result.exit_code=$process.ExitCode
        if($process.ExitCode -ne 0){throw "Lifecycle probe exited $($process.ExitCode); inspect preserved trace and run.err"}
        $summary=Get-Content -LiteralPath (Join-Path $evidence 'display-awake-summary.json') -Raw|ConvertFrom-Json
        $counts=[ordered]@{cases=9;actual_GameLevel_constructions=8;actual_GameLevel_initializations=5;actual_restarts=1;events=15;acquire_attempts=8;successful_acquires=7;release_attempts=7;successful_recorder_releases=6;logical_clients_after=0;unexpected_thread=0;callback_capacity=32;probe_key_taps=14;probe_keyboard_polls=43;probe_mouse_polls=43;probe_state_updates=42;positive_state_updates=8;zero_state_updates=34;GPU_reads=0;GPU_draws=0;crops=0;real_display_requests=0;OS_setting_changes=0;game_saves=0}
        Assert-ExactKeys $summary (@($counts.Keys)+@('scope','outcome','ui_thread','overflow','thread_or_ownership_violation','request_semantics','release_failure_scope'))
        foreach($key in $counts.Keys){if(($summary.$key -isnot [int] -and $summary.$key -isnot [long]) -or $summary.$key -ne $counts[$key]){throw "Exact lifecycle count differs: $key"}}
        if($summary.scope -cne 'actual-GameLevel-display-awake-lifecycle' -or $summary.outcome -cne 'passed' -or $summary.overflow -isnot [bool] -or $summary.overflow -or $summary.thread_or_ownership_violation -isnot [bool] -or $summary.thread_or_ownership_violation -or $summary.ui_thread -le 0){throw 'Lifecycle outcome/thread/overflow proof differs'}
        if($summary.request_semantics -cne 'recorded bool keep and scripted result, not observed Win32 flags or OS request state' -or $summary.release_failure_scope -cne 'cleanup attempt and nonthrowing teardown only; physical OS clear unverified'){throw 'Recorder semantics were overstated'}
        $events=@(Import-Csv -LiteralPath (Join-Path $evidence 'display-awake-events.csv'));$cases=@(Import-Csv -LiteralPath (Join-Path $evidence 'display-awake-cases.csv'))
        if($events.Count -ne 15 -or ($events[0].PSObject.Properties.Name -join',') -cne 'ordinal,case,keep,returned,thread_id,client_owned_after' -or $cases.Count -ne 9 -or ($cases[0].PSObject.Properties.Name -join',') -cne 'case,start_event,end_event,logical_clients_after,ui_thread_only'){throw 'Exact lifecycle CSV schema/count differs'}
        $names=@('standalone-menu','normal-pause-focus-restart-quit','sequential-clear','acquire-failure','release-failure','owner-unwind','owner-init-failure','queued-cancel','application-exit');$starts=@(0,0,2,4,5,7,9,11,13);$ends=@(0,2,4,5,7,9,11,13,15)
        for($i=0;$i -lt 9;++$i){$row=$cases[$i];if($row.case -cne $names[$i] -or $row.start_event -cne [string]$starts[$i] -or $row.end_event -cne [string]$ends[$i] -or $row.logical_clients_after -cne '0' -or $row.ui_thread_only -cne 'true'){throw 'Exact case boundaries differ'}}
        $keeps=@($true,$false,$true,$false,$true,$true,$false,$true,$false,$true,$false,$true,$false,$true,$false);$owned=$false
        for($i=0;$i -lt 15;++$i){$row=$events[$i];$success=$i -notin @(4,6);$caseIndex=if($i -lt 2){1}elseif($i -lt 4){2}elseif($i -lt 5){3}elseif($i -lt 7){4}elseif($i -lt 9){5}elseif($i -lt 11){6}elseif($i -lt 13){7}else{8};if($keeps[$i]){$owned=$success}else{$owned=$false}
            if($row.ordinal -cne [string]($i+1) -or $row.case -cne $names[$caseIndex] -or $row.keep -cne $keeps[$i].ToString().ToLowerInvariant() -or $row.returned -cne $success.ToString().ToLowerInvariant() -or $row.thread_id -cne [string]$summary.ui_thread -or $row.client_owned_after -cne $owned.ToString().ToLowerInvariant()){throw 'Exact recorded bool/result/UI-thread/order differs'}}
        if((Get-FileHash $save).Hash -cne $saveHash){throw 'Lifecycle probe changed the isolated save'}
        foreach($pin in $result.source_hashes){if((Get-FileHash (Join-Path $repo $pin.path)).Hash -cne $pin.sha256){throw 'Source changed during lifecycle probe'}}
        foreach($pin in $result.content_hashes){if((Get-FileHash (Join-Path $game $pin.path)).Hash -cne $pin.sha256){throw 'Staged content changed during lifecycle probe'}}
        if(@(Get-ChildItem -LiteralPath $evidence -File|Where-Object{$_.Extension -in @('.ppm','.png','.rgba')}).Count){throw 'Zero-raster lifecycle probe emitted raster artifacts'}
        if(Select-String -LiteralPath (Join-Path $run 'run.log'),(Join-Path $run 'run.err') -Pattern 'Saving\.\.\.$','^Save was successful\.$','^save failure:','^display idle:' -Quiet){throw 'Unexpected game Save or real adapter diagnostic'}
        $result.observed=$summary;$result.evidence_hashes=@(Get-ChildItem -LiteralPath $evidence -File|ForEach-Object{[ordered]@{name=$_.Name;sha256=(Get-FileHash $_.FullName).Hash}});$result.outcome='passed'
    } finally {Pop-Location}
} catch {$originalFailure=$_;$result.outcome='failed';$result.diagnostic=$_.Exception.Message;throw}
finally {
    $env:PATH=$previousPath;$env:LOCALAPPDATA=$previousProfile
    $cleanupFailure=$null
    if($process){
        try {
            $confirmed=$process.HasExited
            if(!$confirmed){$process.Kill();$confirmed=$process.WaitForExit(5000)}
            $result.owned_process_exit_confirmed=[bool]$confirmed
            if($confirmed){$result.exit_code=$process.ExitCode}else{throw 'Owned lifecycle child exit could not be confirmed'}
        } catch {$cleanupFailure=$_;$result.outcome='failed';$result.cleanup_diagnostic=$_.Exception.Message;$result.owned_process_exit_confirmed=$false}
        finally {try{$process.Dispose()}catch{if(!$cleanupFailure){$cleanupFailure=$_;$result.outcome='failed';$result.cleanup_diagnostic=$_.Exception.Message}}}
    }
    $result.completed_utc=[DateTime]::UtcNow.ToString('o')
    try {Save-LifecycleResult} catch {if(!$cleanupFailure){$cleanupFailure=$_};Write-Warning ('Could not save final lifecycle receipt: '+$_.Exception.Message)}
    if($cleanupFailure -and !$originalFailure){throw $cleanupFailure}

    Write-Host "Display-awake evidence: $run"
}
