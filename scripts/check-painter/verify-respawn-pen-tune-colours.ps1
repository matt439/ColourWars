# Dot-sourced only for -RespawnPenTuneColoursOnly after the native process.
$penSummaryPath=Join-Path $captures 'respawn-pen-consumer-summary.json'
$penRowsPath=Join-Path $captures 'respawn-pen-consumers.csv'
$penSummary=Get-Content -LiteralPath $penSummaryPath -Raw|ConvertFrom-Json
$penRows=@(Import-Csv -LiteralPath $penRowsPath)
$penStates=@('default','rgb','rgba128','alpha0','below-draw-from','at-draw-from','reduce-effects','whole-dead','filled-dead')
$penPairs=@(foreach($state in $penStates){foreach($background in @('black','nonblack')){"$state/$background"}})
$penActualPairs=@($penRows|ForEach-Object {"$($_.state)/$($_.background)"})
$penColumns=@('state','background','normal_match','ordered_nib_channels','frozen_player_geometry_rng','control','scope')
if($penSummary.scope -cne 'consumer-only' -or $penSummary.state_updates -ne 0 -or
    $penSummary.level_updates -ne 0 -or $penSummary.input_updates -ne 0 -or
    $penSummary.player_preparation_updates -ne 3 -or $penSummary.readbacks -ne 62 -or
    $penSummary.csv_rows -ne 18 -or $penSummary.crops -ne 8 -or $penSummary.controls -ne 10 -or
    $penSummary.direct_outlines -ne 4 -or $penSummary.actual_players -ne 1 -or $penSummary.event_fade -ne $false -or
    $penRows.Count -ne 18 -or (($penRows[0].PSObject.Properties.Name)-join '|') -cne ($penColumns-join '|') -or
    (($penActualPairs|Sort-Object)-join '|') -cne (($penPairs|Sort-Object)-join '|') -or
    @($penRows|Where-Object {$_.normal_match -cne 'true' -or $_.ordered_nib_channels -cne 'true' -or
        $_.frozen_player_geometry_rng -cne 'true' -or $_.scope -cne 'consumer-only'}).Count){
    throw 'Respawn pen shared counters/schema/cases exceed the approved frozen scope.'
}
$penControls=@{default='opaque-legacy-parity';rgb='opaque-legacy-parity';rgba128='straight-and-squared-alpha-rejected';
    alpha0='straight-alpha-rejected';'below-draw-from'='blank-boundary';'at-draw-from'='blank-boundary';
    'reduce-effects'='essential-outline-retained';'whole-dead'='actual-player-pen-bypass';'filled-dead'='actual-player-pen-bypass'}
foreach($row in $penRows){if($row.control -cne $penControls[$row.state]){throw 'Respawn pen control label mismatch.'}}
$penCrops=@(Get-ChildItem -LiteralPath $captures -Filter 'respawn-pen-*.ppm' -File|Sort-Object Name)
$penNames=@(foreach($state in @('default','rgb','rgba128','alpha0')){foreach($background in @('black','nonblack')){"respawn-pen-$state-$background.ppm"}})
if($penCrops.Count -ne 8 -or (($penCrops.Name|Sort-Object)-join '|') -cne (($penNames|Sort-Object)-join '|')){
    throw 'Respawn pen crops do not match the exact eight palette/underlay cases.'
}
$penHeader=[Text.Encoding]::ASCII.GetBytes("P6`n440 160`n255`n")
$penBytes=0
$penCropEvidence=@(foreach($crop in $penCrops){
    $bytes=[IO.File]::ReadAllBytes($crop.FullName)
    if($bytes.Length -ne $penHeader.Length+440*160*3){throw "Incorrect compact pen crop size:$($crop.Name)"}
    for($i=0;$i -lt $penHeader.Length;$i++){if($bytes[$i] -ne $penHeader[$i]){throw 'Incorrect canonical pen PPM header.'}}
    $penBytes+=$bytes.Length
    [ordered]@{name=$crop.Name;bytes=$bytes.Length;sha256=(Get-FileHash -LiteralPath $crop.FullName).Hash}
})
$penLog=@(Get-Content -LiteralPath (Join-Path $run 'run.log'))
$penSaveCalls=@($penLog|Where-Object {$_ -match 'Saving\.\.\.$'}).Count
$penSaveSuccesses=@($penLog|Where-Object {$_ -ceq 'Save was successful.'}).Count
if(@(Get-Content -LiteralPath (Join-Path $run 'run.err')|Where-Object {$_ -match '^save failure:'}).Count){
    throw 'Unexpected failed private Save in the pen proof.'
}
$penArtifacts=[ordered]@{}
foreach($path in @($penSummaryPath,$penRowsPath)){
    $penArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
}
$penArtifacts.crops=$penCropEvidence
$penObserved=[ordered]@{consumer=$penSummary;state_updates=0;level_updates=0;input_updates=0;
    player_preparation_updates=3;readbacks=62;csv_rows=18;crops=8;controls=10;crop_pixels=563200;ppm_bytes=$penBytes;
    private_save_calls=$penSaveCalls;successful_private_saves=$penSaveSuccesses;failed_private_saves=0;artifacts=$penArtifacts}
if($debug){
    $penTuneSummaryPath=Join-Path $captures 'respawn-pen-tune-summary.json'
    $penTuneRowsPath=Join-Path $captures 'respawn-pen-tune-colours.csv'
    $penTranscriptPath=Join-Path $captures 'respawn-pen-tune-commands.log'
    $penTuneSummary=Get-Content -LiteralPath $penTuneSummaryPath -Raw|ConvertFrom-Json
    $penTuneRows=@(Import-Csv -LiteralPath $penTuneRowsPath)
    $penTuneStates=@('baseline','named','reset-original','rgb','rgba128','alpha0','reset-saved','rgb-after-save','reset-all-saved')
    $penTunePairs=@(foreach($state in $penTuneStates){foreach($background in @('black','nonblack')){"$state/$background"}})
    $penTuneActual=@($penTuneRows|ForEach-Object {"$($_.state)/$($_.background)"})
    $penTuneColumns=@('state','background','normal_match','ordered_nib_channels','frozen_player_geometry_rng','private_bytes','adoption')
    if($penTuneSummary.scope -cne 'debug-validated-commands' -or $penTuneSummary.state_updates -ne 0 -or
        $penTuneSummary.level_updates -ne 0 -or $penTuneSummary.input_updates -ne 0 -or
        $penTuneSummary.player_preparation_updates -ne 2 -or $penTuneSummary.commands -ne 9 -or
        $penTuneSummary.readbacks -ne 54 -or $penTuneSummary.csv_rows -ne 18 -or $penTuneSummary.crops -ne 0 -or
        $penTuneSummary.saved_tokens -ne 1 -or $penTuneSummary.event_fade -ne $false -or
        $penTuneRows.Count -ne 18 -or (($penTuneRows[0].PSObject.Properties.Name)-join '|') -cne ($penTuneColumns-join '|') -or
        (($penTuneActual|Sort-Object)-join '|') -cne (($penTunePairs|Sort-Object)-join '|') -or
        @($penTuneRows|Where-Object {$_.normal_match -cne 'true' -or $_.ordered_nib_channels -cne 'true' -or
            $_.frozen_player_geometry_rng -cne 'true' -or $_.private_bytes -cne 'true' -or $_.adoption -cne 'live'}).Count -or
        (Get-Item -LiteralPath $penTranscriptPath).Length -le 0 -or $penSaveCalls -ne 2 -or $penSaveSuccesses -ne 2 -or
        !(Test-Path -LiteralPath (Join-Path $captures 'respawn-pen-tune-scratch/profile/ColourWars/save_data.json') -PathType Leaf)){
        throw 'Respawn pen Debug counters/private-save/case evidence exceeds the approved scope.'
    }
    foreach($path in @($penTuneSummaryPath,$penTuneRowsPath,$penTranscriptPath)){
        $penArtifacts[(Split-Path $path -Leaf)]=[ordered]@{bytes=(Get-Item -LiteralPath $path).Length;sha256=(Get-FileHash -LiteralPath $path).Hash}
    }
    $penObserved.developer_commands=$penTuneSummary
    $penObserved.player_preparation_updates=5;$penObserved.readbacks=116;$penObserved.csv_rows=36
}
elseif($penSaveCalls -ne 0 -or $penSaveSuccesses -ne 0 -or
    @(Get-ChildItem -LiteralPath $captures -Filter 'respawn-pen-tune-*').Count){
    throw 'Release pen proof must stay literal-consumer-only without developer artifacts or private Saves.'
}
$result.observed_counts=$penObserved
