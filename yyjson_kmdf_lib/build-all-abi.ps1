<#
.SYNOPSIS
Builds yyjson_kmdf_lib for WDK-supported ABI targets.

.DESCRIPTION
Builds the static library for x64 and ARM64 by default. Use -Configuration
and -Platform to narrow the matrix.

.EXAMPLE
./build-all-abi.ps1 -Configuration Release

.EXAMPLE
./build-all-abi.ps1 -Platform ARM64
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'All')]
    [string]$Configuration = 'All',

    [ValidateSet('x64', 'ARM64', 'All')]
    [string]$Platform = 'All',

    [switch]$Rebuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-MSBuild {
    $programFilesX86 = ${env:ProgramFiles(x86)}
    if ($programFilesX86) {
        $vswhere = Join-Path -Path $programFilesX86 -ChildPath 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (Test-Path -LiteralPath $vswhere) {
            $resolved = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' |
                Select-Object -First 1
            if ($resolved) {
                return $resolved
            }
        }
    }

    $command = Get-Command -Name msbuild -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw 'MSBuild was not found. Install Visual Studio Build Tools with WDK support or run this script from a developer shell.'
}

$configurations = if ($Configuration -eq 'All') { @('Debug', 'Release') } else { @($Configuration) }
$platforms = if ($Platform -eq 'All') { @('x64', 'ARM64') } else { @($Platform) }
$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
$projectPath = Join-Path -Path $PSScriptRoot -ChildPath 'yyjson_kmdf_lib.vcxproj'

if (-not (Test-Path -LiteralPath $projectPath)) {
    throw "Project file not found: $projectPath"
}

$msbuild = Resolve-MSBuild

foreach ($cfg in $configurations) {
    foreach ($abi in $platforms) {
        $label = "$cfg|$abi"
        Write-Host "Building yyjson_kmdf_lib $label"

        & $msbuild $projectPath "/t:$target" "/p:Configuration=$cfg" "/p:Platform=$abi" '/m' '/nologo'
        if ($LASTEXITCODE -ne 0) {
            throw "MSBuild failed for $label with exit code $LASTEXITCODE."
        }
    }
}

Write-Host 'All requested yyjson_kmdf_lib builds completed.'
