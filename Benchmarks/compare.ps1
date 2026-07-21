[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Refs,
    [string]$HarnessCommit,
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [ValidateRange(1, 1000000)]
    [int]$WarmupFrames = 300,
    [ValidateRange(1, 10000000)]
    [int]$SampleFrames = 1200,
    [ValidateRange(1, 20)]
    [int]$Repetitions = 3,
    [string]$Notes = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$resultsRoot = Join-Path $repoRoot "Benchmarks\results"
$runScript = Join-Path $PSScriptRoot "run.ps1"

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [switch]$ReturnText,
        [switch]$AllowFailure
    )

    $output = & git -C $WorkingDirectory @Arguments
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0 -and -not $AllowFailure) {
        throw "git $($Arguments -join ' ') failed with exit code $exitCode."
    }
    if ($ReturnText) {
        return (($output | Out-String).Trim())
    }
    return $exitCode
}

if (-not $HarnessCommit) {
    $HarnessCommit = Invoke-Git -WorkingDirectory $repoRoot -Arguments @(
        "log", "-1", "--format=%H", "--", "Source/Utilities/FrameBenchmark.cpp"
    ) -ReturnText
}
if (-not $HarnessCommit) {
    throw "The benchmark harness is not committed yet. Commit it as one focused commit, then rerun this command or pass -HarnessCommit explicitly."
}
$HarnessCommit = Invoke-Git -WorkingDirectory $repoRoot -Arguments @("rev-parse", $HarnessCommit) -ReturnText
$harnessShort = $HarnessCommit.Substring(0, 8)
$instrumentationPaths = @(
    "Source/Utilities/FrameBenchmark.cpp",
    "Source/Utilities/FrameBenchmark.h",
    "Source/Graphics/GraphicsEngine/RHI/RenderHardwareInterface.cpp",
    "Source/Graphics/GraphicsEngine/RHI/RenderHardwareInterface.h",
    "Source/Graphics/GraphicsEngine/GraphicsEngine.vcxproj",
    "Source/Graphics/GraphicsEngine/GraphicsEngine.vcxproj.filters"
)
$harnessPatch = & git -C $repoRoot diff "$HarnessCommit^" $HarnessCommit -- @instrumentationPaths
if ($LASTEXITCODE -ne 0 -or -not $harnessPatch) {
    throw "Could not extract the benchmark instrumentation from commit $HarnessCommit. Keep the harness in one focused commit."
}

$systemTempRoot = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempBase = Join-Path $systemTempRoot ("agp-benchmark-" + [Guid]::NewGuid().ToString("N"))
$tempBase = [System.IO.Path]::GetFullPath($tempBase)
if (-not $tempBase.StartsWith($systemTempRoot, [StringComparison]::OrdinalIgnoreCase) -or
    -not (Split-Path -Leaf $tempBase).StartsWith("agp-benchmark-")) {
    throw "Refusing to use an unexpected temporary worktree path: $tempBase"
}
New-Item -ItemType Directory -Force -Path $tempBase | Out-Null

try {
    foreach ($ref in $Refs) {
        $commit = Invoke-Git -WorkingDirectory $repoRoot -Arguments @("rev-parse", $ref) -ReturnText
        $safeRef = $ref -replace '[^A-Za-z0-9_-]', '-'
        $worktree = Join-Path $tempBase ($safeRef + "-" + $commit.Substring(0, 8))

        Write-Host "Preparing $ref ($($commit.Substring(0, 8)))..."
        Invoke-Git -WorkingDirectory $repoRoot -Arguments @("worktree", "add", "--detach", $worktree, $commit) | Out-Null
        try {
            $containsHarness = (Invoke-Git -WorkingDirectory $worktree -Arguments @(
                "merge-base", "--is-ancestor", $HarnessCommit, $commit
            ) -AllowFailure) -eq 0
            if ($containsHarness) {
                $harnessDescription = "in-tree:$harnessShort"
            }
            else {
                Write-Host "Applying benchmark-only instrumentation from $harnessShort without changing the measured ref..."
                $harnessPatch | & git -C $worktree apply --whitespace=nowarn -
                if ($LASTEXITCODE -ne 0) {
                    throw "The benchmark instrumentation did not apply cleanly to $ref."
                }
                $harnessDescription = "temporary:$harnessShort"
            }

            & $runScript `
                -Label $ref `
                -Configuration $Configuration `
                -WarmupFrames $WarmupFrames `
                -SampleFrames $SampleFrames `
                -Repetitions $Repetitions `
                -Notes $Notes `
                -RepoRoot $worktree `
                -ResultsRoot $resultsRoot `
                -CommitOverride $commit `
                -BranchOverride $ref `
                -Harness $harnessDescription `
                -DirtyOverride $false `
                -NoReport
        }
        finally {
            Invoke-Git -WorkingDirectory $repoRoot -Arguments @("worktree", "remove", "--force", $worktree) -AllowFailure | Out-Null
        }
    }
}
finally {
    # This directory is created by this script and contains only its temporary worktrees.
    if ($tempBase.StartsWith($systemTempRoot, [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $tempBase).StartsWith("agp-benchmark-") -and
        (Test-Path -LiteralPath $tempBase)) {
        Remove-Item -LiteralPath $tempBase -Recurse -Force
    }
    Invoke-Git -WorkingDirectory $repoRoot -Arguments @("worktree", "prune") -AllowFailure | Out-Null
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $python) {
    throw "Python was not found. Runs were saved, but the report could not be generated."
}

$reportPath = Join-Path $resultsRoot "report.html"
& $python.Source (Join-Path $PSScriptRoot "generate_report.py") --results $resultsRoot --output $reportPath
if ($LASTEXITCODE -ne 0) {
    throw "Report generation failed with exit code $LASTEXITCODE."
}

Write-Host "Comparison complete: $reportPath"
