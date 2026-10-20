# Dot-sourced only for -SilhouetteTuneColoursOnly after the actual native run.
$silhouetteSummaryPath=Join-Path $captures 'silhouette-colour-consumer-summary.json'
$silhouetteRowsPath=Join-Path $captures 'silhouette-colour-consumers.csv'
$silhouetteSummary=Get-Content -LiteralPath $silhouetteSummaryPath -Raw|ConvertFrom-Json
$silhouetteRows=@(Import-Csv -LiteralPath $silhouetteRowsPath)
$silhouetteStates=@('default','rgb','rgba','alpha0','restored-default')
$silhouettePairs=@(foreach($state in $silhouetteStates){foreach($background in @('black','nonblack')){"$state/$background"}})
$silhouetteActualPairs=@($silhouetteRows|ForEach-Object {"$($_.state)/$($_.background)"})
$silhouetteColumns=@('state','background','raw_patch_channels','uniform_patch_interiors','frozen_inputs_resources_rng','body_outside_scope','scope')
if($silhouetteSummary.scope -cne 'consumer-only' -or $silhouetteSummary.readbacks -ne 38 -or
    $silhouetteSummary.csv_rows -ne 10 -or $silhouetteSummary.crops -ne 8 -or $silhouetteSummary.controls -ne 10 -or
    $silhouetteSummary.player_updates -ne 0 -or $silhouetteSummary.level_updates -ne 0 -or $silhouetteSummary.state_updates -ne 0 -or
    $silhouetteSummary.input_updates -ne 0 -or $silhouetteSummary.time_updates -ne 0 -or $silhouetteSummary.event_fade -ne $false -or
    $silhouetteSummary.patch_roles -ne 3 -or $silhouetteSummary.patch_width -ne 3 -or $silhouetteSummary.patch_height -ne 3 -or
    $silhouetteSummary.ink_passes -ne 2 -or $silhouetteSummary.rim_passes -ne 1 -or $silhouetteSummary.shade_passes -ne 4 -or
    $silhouetteSummary.ink_sample_x -ne 963 -or $silhouetteSummary.ink_sample_y -ne 690 -or
    $silhouetteSummary.rim_sample_x -ne 932 -or $silhouetteSummary.rim_sample_y -ne 627 -or
    $silhouetteSummary.shade_sample_x -ne 959 -or $silhouetteSummary.shade_sample_y -ne 679 -or
    $silhouetteSummary.child_scope -cne 'aggregate-colour-adoption' -or $silhouetteRows.Count -ne 10 -or
    (($silhouetteRows[0].PSObject.Properties.Name)-join '|') -cne ($silhouetteColumns-join '|') -or
    (($silhouetteActualPairs|Sort-Object)-join '|') -cne (($silhouettePairs|Sort-Object)-join '|') -or
    @($silhouetteRows|Where-Object {$_.raw_patch_channels -cne 'true' -or $_.uniform_patch_interiors -cne 'true' -or
        $_.frozen_inputs_resources_rng -cne 'true' -or $_.body_outside_scope -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){
    throw 'Silhouette shared counters/schema/patch roles exceed the approved exact scope.'
}
$silhouetteCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'silhouette-colour-*.ppm' -File|Sort-Object Name)
$silhouetteNames=@(foreach($state in @('default','rgb','rgba','alpha0')){foreach($background in @('black','nonblack')){"silhouette-colour-$state-$background.ppm"}})
if($silhouetteCrops.Count -ne 8 -or (($silhouetteCrops.Name|Sort-Object)-join '|') -cne (($silhouetteNames|Sort-Object)-join '|')){
    throw 'Silhouette compact crops differ from the exact four palette/two underlay pairs.'
}
$silhouetteHeader=[Text.Encoding]::ASCII.GetBytes("P6`n440 240`n255`n")
$silhouetteBytes=0
$silhouetteCropEvidence=@(foreach($crop in $silhouetteCrops){
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $silhouetteHeader.Length+440*240*3){throw "Incorrect silhouette montage size: $($crop.Name)"}
    for($i=0;$i -lt $silhouetteHeader.Length;$i++){if($bytes[$i] -ne $silhouetteHeader[$i]){throw 'Incorrect canonical silhouette PPM header.'}}
    $silhouetteBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
if(@(Get-ChildItem -LiteralPath $captures -Filter 'silhouette-patch-failure-*' -File).Count){throw 'Silhouette raw patch failure artifacts remain.'}
$silhouetteLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$silhouetteSaveCalls=@($silhouetteLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$silhouetteSaveSuccesses=@($silhouetteLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){
    throw 'Unexpected failed private Save in the silhouette proof.'
}
$silhouetteArtifacts=[ordered]@{}
foreach($path in @($silhouetteSummaryPath,$silhouetteRowsPath)){
    $silhouetteArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
}
$silhouetteArtifacts.crops=$silhouetteCropEvidence
$silhouetteObserved=[ordered]@{consumer=$silhouetteSummary;player_updates=0;level_updates=0;state_updates=0;input_updates=0;time_updates=0;
    readbacks=38;csv_rows=10;crops=8;controls=10;crop_pixels=844800;ppm_bytes=$silhouetteBytes;
    private_save_calls=$silhouetteSaveCalls;successful_private_saves=$silhouetteSaveSuccesses;failed_private_saves=0;artifacts=$silhouetteArtifacts}
if($debug){
    $silhouetteTuneSummaryPath=Join-Path $captures 'silhouette-colour-tune-summary.json'
    $silhouetteTuneRowsPath=Join-Path $captures 'silhouette-colour-tune.csv'
    $silhouetteTranscriptPath=Join-Path $captures 'silhouette-colour-tune-commands.log'
    $silhouetteTuneSummary=Get-Content -LiteralPath $silhouetteTuneSummaryPath -Raw|ConvertFrom-Json
    $silhouetteTuneRows=@(Import-Csv -LiteralPath $silhouetteTuneRowsPath)
    $silhouetteTuneStates=@('baseline','ink-rgba','rim-rgba','shade-rgba','reset-ink-siblings','restored-ink',
        'ink-alpha0','rim-alpha0','all-alpha0','reset-rim-siblings','reset-all-saved','named-rim','final-reset-saved')
    $silhouetteTunePairs=@(foreach($state in $silhouetteTuneStates){foreach($background in @('black','nonblack')){"$state/$background"}})
    $silhouetteTuneActual=@($silhouetteTuneRows|ForEach-Object {"$($_.state)/$($_.background)"})
    $silhouetteTuneColumns=@('state','background','raw_patch_channels','uniform_patch_interiors','frozen_inputs_resources_rng','private_bytes','adoption')
    if($silhouetteTuneSummary.scope -cne 'debug-validated-commands' -or $silhouetteTuneSummary.commands -ne 13 -or
        $silhouetteTuneSummary.readbacks -ne 78 -or $silhouetteTuneSummary.csv_rows -ne 26 -or $silhouetteTuneSummary.crops -ne 0 -or
        $silhouetteTuneSummary.saved_tokens -ne 3 -or $silhouetteTuneSummary.event_fade -ne $false -or
        $silhouetteTuneSummary.player_updates -ne 0 -or $silhouetteTuneSummary.level_updates -ne 0 -or
        $silhouetteTuneSummary.state_updates -ne 0 -or $silhouetteTuneSummary.input_updates -ne 0 -or $silhouetteTuneSummary.time_updates -ne 0 -or
        $silhouetteTuneRows.Count -ne 26 -or (($silhouetteTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($silhouetteTuneColumns-join '|') -or
        (($silhouetteTuneActual|Sort-Object)-join '|') -cne (($silhouetteTunePairs|Sort-Object)-join '|') -or
        @($silhouetteTuneRows|Where-Object {$_.raw_patch_channels -cne 'true' -or $_.uniform_patch_interiors -cne 'true' -or
            $_.frozen_inputs_resources_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        (Get-Item -LiteralPath $silhouetteTranscriptPath).Length -le 0 -or $silhouetteSaveCalls -ne 2 -or $silhouetteSaveSuccesses -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'silhouette-colour-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'Silhouette Debug counters/schema/private-save evidence differs from the approved exact scope.'
    }
    foreach($path in @($silhouetteTuneSummaryPath,$silhouetteTuneRowsPath,$silhouetteTranscriptPath)){
        $silhouetteArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
    }
    $silhouetteObserved.developer_commands=$silhouetteTuneSummary
    $silhouetteObserved.readbacks=116;$silhouetteObserved.csv_rows=36
}
elseif($silhouetteSaveCalls -ne 0 -or $silhouetteSaveSuccesses -ne 0 -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'silhouette-colour-tune-*').Count){
    throw 'Release silhouette proof must remain literal-consumer-only without developer artifacts or private Saves.'
}
$result.observed_counts=$silhouetteObserved
