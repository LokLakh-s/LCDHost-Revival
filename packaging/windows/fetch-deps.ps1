# Fetches the Windows dependencies of LCDHost at pinned versions and checks
# their SHA-256, then prints the variables qmake needs (and adds them to
# $GITHUB_ENV on GitHub Actions).
#
# Usage: pwsh packaging/windows/fetch-deps.ps1 -Destination <directory>
# Needs 7-Zip (7z). Run it again after changing a version: finished steps are
# kept.
param([Parameter(Mandatory)][string]$Destination)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$Downloads = @{
    # Official binary release, signed by the libusb maintainer (checked once
    # with the keys from https://github.com/libusb/libusb/blob/master/KEYS).
    libusb = @{
        Url = 'https://github.com/libusb/libusb/releases/download/v1.0.30/libusb-1.0.30.7z'
        Sha256 = '7fb1dfec805b97983763d7d0ae244320da12add1003d4249c96cc4d586398c79'
    }
    hidapi = @{
        Url = 'https://github.com/libusb/hidapi/releases/download/hidapi-0.15.0/hidapi-win.zip'
        Sha256 = 'd18c43ec9506a2f6d7faa9c7e0a342c4b64fbae521b71b5d4ac0777fd24dda93'
    }
}

function Get-Download([string]$Name) {
    $entry = $Downloads[$Name]
    $file = Join-Path $Destination (Split-Path $entry.Url -Leaf)
    if (-not (Test-Path $file)) {
        Write-Host "Downloading $($entry.Url)"
        Invoke-WebRequest -Uri $entry.Url -OutFile $file
    }
    $hash = (Get-FileHash -Algorithm SHA256 $file).Hash.ToLowerInvariant()
    if ($hash -ne $entry.Sha256) {
        Remove-Item $file
        throw "${Name}: unexpected SHA-256 $hash"
    }
    return $file
}

New-Item -ItemType Directory -Force $Destination | Out-Null
$Destination = (Resolve-Path $Destination).Path

$libusb = Join-Path $Destination 'libusb'
if (-not (Test-Path "$libusb\include\libusb.h")) {
    $archive = Get-Download libusb
    7z x -y "-o$libusb" $archive | Out-Null
    if ($LASTEXITCODE) { throw "7z failed with exit code $LASTEXITCODE" }
}

$hidapi = Join-Path $Destination 'hidapi'
if (-not (Test-Path "$hidapi\include\hidapi.h")) {
    Expand-Archive -Force (Get-Download hidapi) $hidapi
}

$variables = [ordered]@{
    LIBUSB_DIR = $libusb -replace '\\', '/'
    HIDAPI_DIR = $hidapi -replace '\\', '/'
}
foreach ($name in $variables.Keys) {
    Write-Output "$name=$($variables[$name])"
    if ($env:GITHUB_ENV) {
        "$name=$($variables[$name])" | Out-File -Append -Encoding utf8 $env:GITHUB_ENV
    }
}
