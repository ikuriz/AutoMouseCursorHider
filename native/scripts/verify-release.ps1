param(
    [string]$Executable = "$PSScriptRoot\..\build\Release\AutoMouseCursorHider.exe"
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path -LiteralPath $Executable)) {
    throw "Executable not found: $Executable"
}

$dumpbin = Get-ChildItem "C:\Program Files (x86)\Microsoft Visual Studio" -Filter dumpbin.exe -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $dumpbin) {
    throw "dumpbin.exe was not found. Install the MSVC build tools."
}

$headers = & $dumpbin /headers $Executable | Out-String
if ($headers -notmatch "8664 machine") {
    throw "The executable is not an x64 PE image."
}

$dependencies = & $dumpbin /dependents $Executable | Out-String
$forbidden = 'VCRUNTIME|MSVCP|ucrtbase|CORECLR|HOSTFXR|HOSTPOLICY|api-ms-win-crt'
if ($dependencies -match $forbidden) {
    throw "The executable has a forbidden runtime dependency:`n$dependencies"
}

Write-Output "PASS: x64 native executable has no .NET or VC++ runtime dependency."
Write-Output (Get-Item -LiteralPath $Executable | Select-Object FullName, Length)

