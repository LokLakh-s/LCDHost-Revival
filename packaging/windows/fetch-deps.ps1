# Fetches the Windows dependencies of LCDHost at pinned versions, checks
# their SHA-256 and builds libwdi, then prints the variables qmake needs
# (and adds them to $GITHUB_ENV on GitHub Actions).
#
# Usage: pwsh packaging/windows/fetch-deps.ps1 -Destination <directory>
# Needs git, 7-Zip (7z) and Visual Studio 2022 with the x64 and ARM64 C++
# build tools. Run it again after changing a version: finished steps are kept.
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
    # Windows Driver Kit 8.0 redistributable components: the WinUSB and WDF
    # co-installers that libwdi embeds (https://go.microsoft.com/fwlink/p/?LinkID=253170).
    wdk = @{
        Url = 'https://download.microsoft.com/download/0/5/F/05FD6919-6250-425B-86ED-9B095E54065A/wdfcoinstaller.msi'
        Sha256 = '29314207814ce9d5d73695f7e9239539cf37c79e750b9d5ea5a5ef5487a583d6'
    }
}
$LibwdiTag = 'v1.5.1'
$LibwdiCommit = '9b23b82a2dd1cbffc16d46c212f92c6bf8c0c602'

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

function Find-MSBuild {
    $command = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $found = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\amd64\MSBuild.exe' |
        Select-Object -First 1
    if (-not $found) { throw 'MSBuild not found' }
    return $found
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

$libwdi = Join-Path $Destination 'libwdi'
if (-not (Test-Path "$libwdi\x64\Release\lib\libwdi.lib")) {
    if (Test-Path $libwdi) { Remove-Item -Recurse -Force $libwdi }
    git clone --quiet --depth 1 --branch $LibwdiTag https://github.com/pbatard/libwdi.git $libwdi
    if ($LASTEXITCODE) { throw "git clone failed with exit code $LASTEXITCODE" }
    $head = git -C $libwdi rev-parse HEAD
    if ($head -ne $LibwdiCommit) { throw "libwdi $LibwdiTag is $head, expected $LibwdiCommit" }

    $msi = Get-Download wdk
    $process = Start-Process msiexec -Wait -PassThru -ArgumentList '/a', "`"$msi`"", '/qn', "TARGETDIR=`"$libwdi\wdk`""
    if ($process.ExitCode) { throw "msiexec failed with exit code $($process.ExitCode)" }

    # Embed the WinUSB driver only: point WDK_DIR to the co-installers and
    # drop the libusb0 and libusbK defaults of libwdi's MSVC configuration.
    $wdk = "$libwdi\wdk\Windows Kits\8.0" -replace '\\', '/'
    $config = "$libwdi\msvc\config.h"
    (Get-Content $config) | ForEach-Object {
        if ($_ -match '^#define (LIBUSB0_DIR|LIBUSBK_DIR) ') { '' }
        elseif ($_ -match '^#define WDK_DIR ') { "#define WDK_DIR `"$wdk`"" }
        else { $_ }
    } | Set-Content $config

    & (Find-MSBuild) "$libwdi\libwdi.sln" /m /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=x64
    if ($LASTEXITCODE) { throw "MSBuild failed with exit code $LASTEXITCODE" }
}

$variables = [ordered]@{
    LIBUSB_DIR = $libusb -replace '\\', '/'
    HIDAPI_DIR = $hidapi -replace '\\', '/'
    LIBWDI_DIR = $libwdi -replace '\\', '/'
}
foreach ($name in $variables.Keys) {
    Write-Output "$name=$($variables[$name])"
    if ($env:GITHUB_ENV) {
        "$name=$($variables[$name])" | Out-File -Append -Encoding utf8 $env:GITHUB_ENV
    }
}
