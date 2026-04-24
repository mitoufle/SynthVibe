$dest = 'C:\Program Files\Common Files\VST3'
$src  = Join-Path $PSScriptRoot 'build\AISynth_artefacts\Release\VST3\AI Synth.vst3'

$max = 0
Get-ChildItem $dest -Directory -Filter 'AI Synth v*.vst3' | ForEach-Object {
    if ($_.Name -match 'AI Synth v(\d+)\.vst3') {
        $n = [int]$Matches[1]
        if ($n -gt $max) { $max = $n }
    }
}

$next      = $max + 1
$bundleName = "AI Synth v$next"
$target    = Join-Path $dest "$bundleName.vst3"

Write-Host "Deploying as: $bundleName.vst3"
xcopy /E /I /Q $src $target

# VST3 spec: inner DLL must match the bundle folder name
$innerDir = Join-Path $target 'Contents\x86_64-win'
$oldDll   = Join-Path $innerDir 'AI Synth.vst3'
$newDll   = Join-Path $innerDir "$bundleName.vst3"
if (Test-Path $oldDll) {
    Rename-Item $oldDll $newDll
    Write-Host "Renamed inner binary to: $bundleName.vst3"
}

Write-Host "Done."
