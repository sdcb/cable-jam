param(
    [string]$Output = $PSScriptRoot
)

# Verifies the MP3 cue set that is embedded into the executable. This mirrors
# the reference project's asset workflow while keeping the checked-in cues
# deterministic for release builds.
$files = @(
    "ui_button_click.mp3",
    "ui_confirm.mp3",
    "ui_cancel.mp3",
    "cable_pull.mp3",
    "cable_blocked.mp3",
    "level_complete.mp3"
)

foreach ($file in $files) {
    $path = Join-Path $Output $file
    if (-not (Test-Path $path)) {
        Write-Warning "Missing $path"
    } else {
        Write-Host "Found $path"
    }
}
