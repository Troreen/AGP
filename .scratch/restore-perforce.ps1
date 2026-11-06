$ErrorActionPreference = 'Stop'
$logRoot = Join-Path $PSScriptRoot 'perforce-restore'
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
function Is-Preserved([string]$relative) {
    $relative = $relative.Replace('\','/')
    return $relative -match '(^|/)(\.git|\.agents|\.codex|\.scratch|AgentConfig|Docs|\.claude|\.cursor|\.vscode|\.github)(/|$)' -or
        $relative -match '\.md$' -or $relative -match '(^|/)(\.gitignore|\.gitattributes|\.gitmodules|p4ignore\.txt|\.p4config|p4config\.txt|\.editorconfig|\.clang-format[^/]*|\.cursorrules)$' -or
        $relative -match '^Tools/Agents/'
}
function Read-P4($client, $arguments) {
    $output = @(& p4 -c $client -ztag -Mj @arguments)
    if ($LASTEXITCODE -ne 0 -and ($output -join '') -notmatch 'up-to-date|not opened|no file\(s\)|no files') { throw ($output -join "`n") }
    return @($output | ForEach-Object { $_ | ConvertFrom-Json })
}
function Apply-P4($client, $operation, $files, $label) {
    if ($files.Count -eq 0) { return }
    $list = Join-Path $logRoot "$client-$label-files.txt"
    [IO.File]::WriteAllLines($list, [string[]]$files, (New-Object Text.UTF8Encoding($false)))
    $output = @(& p4 -c $client -x $list @operation 2>&1)
    $output | Set-Content (Join-Path $logRoot "$client-$label-result.txt")
    if ($LASTEXITCODE -ne 0) { throw "Failed $label : $($output -join "`n")" }
    Write-Output "$client $label : $($files.Count) files"
}
$workspaces = @(
    @{Client='tarik.bergstrom_TGA-SP85_Mainline_884';Root='C:\Users\tarik.bergstrom\Documents\GitHub\AGP'},
    @{Client='tarik_content';Root='C:\Users\tarik.bergstrom\Perforce\AGP-Content'}
)
foreach ($workspace in $workspaces) {
    $client = $workspace.Client
    $root = [IO.Path]::GetFullPath($workspace.Root).TrimEnd('\')
    $opened = @(Read-P4 $client @('opened') | Where-Object { $_.depotFile })
    $revert = @($opened | Where-Object { -not (Is-Preserved ($_.clientFile -replace '^//[^/]+/','')) } | ForEach-Object { $_.depotFile })
    Apply-P4 $client @('revert') $revert 'revert'
    $preview = @(Read-P4 $client @('clean','-n',"//$client/..."))
    $preview | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $logRoot "$client-clean-preview.json")
    $clean = @($preview | Where-Object {
        if (-not $_.clientFile) { return $false }
        $absolute = [IO.Path]::GetFullPath($_.clientFile)
        if (-not $absolute.StartsWith($root+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Outside workspace: $absolute" }
        return -not (Is-Preserved $absolute.Substring($root.Length+1))
    } | ForEach-Object { $_.depotFile })
    Apply-P4 $client @('clean') $clean 'clean'
    $syncPreview = @(Read-P4 $client @('sync','-n',"//$client/..."))
    $syncPreview | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $logRoot "$client-sync-preview.json")
    $sync = @($syncPreview | Where-Object {
        if (-not $_.clientFile) { return $false }
        $absolute = [IO.Path]::GetFullPath($_.clientFile)
        if (-not $absolute.StartsWith($root+'\',[StringComparison]::OrdinalIgnoreCase)) { throw "Outside workspace: $absolute" }
        return -not (Is-Preserved $absolute.Substring($root.Length+1))
    } | ForEach-Object { $_.depotFile })
    Apply-P4 $client @('sync') $sync 'sync'
}
