param([string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$uat = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
if (!(Test-Path -LiteralPath $uat)) { throw "Unreal Engine not found: $EngineRoot" }
$packageTemp=Join-Path $projectRoot 'Intermediate/PackagingTemp'
New-Item -ItemType Directory -Path $packageTemp -Force | Out-Null
$env:TEMP=$packageTemp
$env:TMP=$packageTemp
& $uat BuildCookRun "-project=$projectRoot/AnEmberRemains.uproject" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -archive "-archivedirectory=$projectRoot/Builds/HushedCloisters" -utf8output
if ($LASTEXITCODE -ne 0) { throw "Unreal packaging failed with exit code $LASTEXITCODE" }
