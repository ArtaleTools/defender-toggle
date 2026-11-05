# Build the Defender toggle GUI with Zig.
#
# Requires Zig (https://ziglang.org, tested with 0.13.0). Put `zig` on PATH,
# or set $env:ZIG to the full path of zig.exe. No other toolchain or C runtime.
#
#   pwsh -File build.ps1
#
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$zig = if ($env:ZIG) { $env:ZIG } else { (Get-Command zig -ErrorAction SilentlyContinue).Source }
if (-not $zig) {
    throw 'Zig not found. Install from https://ziglang.org and add it to PATH, or set $env:ZIG to zig.exe.'
}
$zigDir = Split-Path $zig
$zigLib = Join-Path $zigDir 'lib\libc\include\any-windows-any'
Write-Host "Using zig: $zig"

# Resource (icon + manifest + embedded offline scripts)
& $zig rc /fo app.res app.rc
if ($LASTEXITCODE -ne 0) { throw "zig rc failed ($LASTEXITCODE)" }

# GUI, freestanding (no C runtime) for a tiny, fast binary.
# NOTE: flags are passed as an array so PowerShell does not mis-parse the
# `-Wl,--entry=start` / `-Wl,--subsystem,windows` tokens.
$cc = @(
    'cc', '-target', 'x86_64-windows-gnu', '-Os', '-s',
    '-fno-stack-protector', '-fno-builtin', '-nostdlib',
    '-isystem', $zigLib,
    '-Wl,--entry=start', '-Wl,--subsystem,windows',
    'defender_gui.c', 'app.res', '-o', 'defender_gui.exe',
    '-lkernel32', '-luser32', '-lgdi32', '-ladvapi32', '-limm32'
)
& $zig @cc
if ($LASTEXITCODE -ne 0) { throw "zig cc (gui) failed ($LASTEXITCODE)" }

Write-Host 'Built defender_gui.exe'
