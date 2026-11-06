$ErrorActionPreference = 'Stop'

$version = '5.0.0-beta8'
$archiveUrl = "https://github.com/premake/premake-core/releases/download/v$version/premake-$version-windows.zip"
$archiveHash = 'E64CE2ED8778E0098F63674CCA61FE33941B5F0C8D9A4AFD651152BDEA3758AB'
$executableHash = '2301E3E23FF3074CB83A5EA6103D68C7EA81DAD56B786807C84B0643CDDEA31B'
$premakePath = Join-Path $PSScriptRoot 'premake5.exe'

try {
    if (Test-Path -LiteralPath $premakePath) {
        if ((Get-FileHash -Algorithm SHA256 -LiteralPath $premakePath).Hash -ne $executableHash) {
            throw "Premake\premake5.exe does not match the pinned $version release. Remove it to download a verified copy."
        }
        Write-Host "Using Premake $version."
        exit 0
    }

    $temporaryDirectory = Join-Path ([System.IO.Path]::GetTempPath()) ("agp-premake-" + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $temporaryDirectory | Out-Null
    try {
        $archivePath = Join-Path $temporaryDirectory 'premake.zip'
        Write-Host "Downloading Premake $version from its official release..."
        Invoke-WebRequest -Uri $archiveUrl -OutFile $archivePath -UseBasicParsing
        if ((Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath).Hash -ne $archiveHash) {
            throw 'The Premake download failed its SHA-256 check.'
        }

        $extractPath = Join-Path $temporaryDirectory 'extracted'
        Expand-Archive -LiteralPath $archivePath -DestinationPath $extractPath
        $downloadedExe = Join-Path $extractPath 'premake5.exe'
        if (!(Test-Path -LiteralPath $downloadedExe) -or
            (Get-FileHash -Algorithm SHA256 -LiteralPath $downloadedExe).Hash -ne $executableHash) {
            throw 'The extracted Premake executable failed its SHA-256 check.'
        }

        Move-Item -LiteralPath $downloadedExe -Destination $premakePath
        Write-Host "Installed verified Premake $version at $premakePath."
    }
    finally {
        Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force -ErrorAction SilentlyContinue
    }
}
catch {
    Write-Error "Could not prepare Premake: $_"
    exit 1
}
