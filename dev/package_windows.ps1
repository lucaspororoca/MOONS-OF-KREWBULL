$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDir    = Join-Path $projectRoot 'builds\Moons of Krewbull - Build 1'
$runtimeBuildExe = Join-Path $buildDir 'Moons of Krewbull.runtime.exe'
$runtimeBinary = Join-Path $buildDir 'Moons of Krewbull.runtime'
$launcherExe = Join-Path $buildDir 'Moons of Krewbull.exe'
$launcherSrc = Join-Path $PSScriptRoot 'launcher.c++'
$encryptScript = Join-Path $PSScriptRoot 'encrypt_resources.py'
$ucrtBin     = 'C:\msys64\ucrt64\bin'
$gpp         = Join-Path $ucrtBin 'g++.exe'
$objdump     = Join-Path $ucrtBin 'objdump.exe'
$libsDir     = Join-Path $buildDir 'libs'

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}
if (-not (Test-Path $runtimeBuildExe)) {
    throw "Runtime executable not found: $runtimeBuildExe"
}
if (-not (Test-Path $objdump)) {
    throw "objdump not found: $objdump"
}
if (-not (Test-Path $gpp)) {
    throw "g++ not found: $gpp"
}
if (-not (Test-Path $launcherSrc)) {
    throw "Launcher source not found: $launcherSrc"
}

$pending = [System.Collections.Generic.Queue[string]]::new()
$visited = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase
)
$pending.Enqueue($runtimeBuildExe)

$copiedCount = 0

if (Test-Path $libsDir) {
    Remove-Item -LiteralPath $libsDir -Recurse -Force
}
New-Item -ItemType Directory -Path $libsDir -Force | Out-Null

while ($pending.Count -gt 0) {
    $binary = $pending.Dequeue()
    $imports = & $objdump -p $binary
    if ($LASTEXITCODE -ne 0) {
        throw "Could not inspect dependencies of $binary"
    }

    foreach ($line in $imports) {
        if ($line -match 'DLL Name:\s*(.+)$') {
            $name = $Matches[1].Trim()
            if ($visited.Add($name)) {
                $dependency = Join-Path $ucrtBin $name
                if (Test-Path $dependency) {
                    Copy-Item -LiteralPath $dependency -Destination $libsDir -Force
                    $copiedCount++
                    $pending.Enqueue($dependency)
                }
            }
        }
    }
}

$resourceSource = Join-Path $projectRoot 'resources'
$resourceArguments = [System.Collections.Generic.List[string]]::new()

if (Test-Path $resourceSource) {
    $resourceItems = @(Get-ChildItem -LiteralPath $resourceSource -Force)
    foreach ($item in $resourceItems) {
        $resourceTarget = Join-Path $buildDir $item.Name
        if ($item.PSIsContainer -and
            @(Get-ChildItem -LiteralPath $item.FullName -Force).Count -eq 0) {
            New-Item -ItemType Directory -Path $resourceTarget -Force | Out-Null
            continue
        }
        if (Test-Path $resourceTarget) {
            Remove-Item -LiteralPath $resourceTarget -Recurse -Force
        }
        Copy-Item -LiteralPath $item.FullName -Destination $buildDir -Recurse -Force
        $resourceArguments.Add($resourceTarget)
    }
    Write-Output "Resource folders copied beside the executable."
} else {
    Write-Output "Warning: resources folder not found at $resourceSource"
}

$legacyResourceRoot = Join-Path $buildDir 'resources'
if (Test-Path $legacyResourceRoot) {
    Remove-Item -LiteralPath $legacyResourceRoot -Recurse -Force
}

if ($resourceArguments.Count -gt 0) {
    & py $encryptScript @resourceArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Resource encryption failed with exit code $LASTEXITCODE"
    }
}

& $gpp -std=c++17 -municode -mwindows $launcherSrc -o $launcherExe
if ($LASTEXITCODE -ne 0) {
    throw "Could not build the Windows launcher."
}

Get-ChildItem -LiteralPath $buildDir -Filter '*.dll' -File | Remove-Item -Force

if (Test-Path $runtimeBinary) {
    Remove-Item -LiteralPath $runtimeBinary -Force
}
Move-Item -LiteralPath $runtimeBuildExe -Destination $runtimeBinary
[System.IO.File]::SetAttributes(
    $runtimeBinary,
    [System.IO.File]::GetAttributes($runtimeBinary) -bor [System.IO.FileAttributes]::Hidden
)

Write-Output "Copied $copiedCount DLLs to $libsDir"
Write-Output "Packaged one visible executable, hidden runtime, libraries, and encrypted resources in $buildDir"