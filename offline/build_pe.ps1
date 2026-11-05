# build_pe.ps1 - one-time setup for the offline Defender switch (embedded in Defender toggle exe).
# Makes <Root>\pe\winre.wim from this PC's own WinRE image, with winpeshl.ini changed to run
# df_run.cmd, plus a hidden BCD ramdisk entry. Writes pe\entry-guid.txt last = ready marker.
# ASCII only (runs under Windows PowerShell 5.1). Log: <Root>\build.log. Exit 0 = ok.
param([string]$Root = 'C:\DefenderOffline')
$ErrorActionPreference = 'Stop'
$pe   = Join-Path $Root 'pe'
$log  = Join-Path $Root 'build.log'
$tmp  = Join-Path $pe 'winre.new.wim'
# inside the protected folder: Windows\Temp lets any user pre-create this path
$mnt  = Join-Path $Root 'mnt'
$dism = Join-Path $env:SystemRoot 'System32\Dism.exe'
$drv  = Split-Path -Qualifier $Root
$rel  = $Root.Substring($drv.Length)
$mounted = $false

function Log([string]$m) { Add-Content -LiteralPath $log -Value ((Get-Date -Format 'HH:mm:ss') + ' ' + $m) -Encoding ASCII }
function Run([string]$exe, [string[]]$a) {
    $out = & $exe @a 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { throw ("{0} {1} -> rc={2}: {3}" -f (Split-Path -Leaf $exe), ($a -join ' '), $LASTEXITCODE, $out.Trim()) }
    return $out
}
function NewGuid([string]$out) {
    if ($out -match '\{[0-9a-fA-F-]{36}\}') { return $Matches[0] }
    throw "no GUID in bcdedit output: $out"
}

try {
    New-Item -ItemType Directory -Force $pe | Out-Null
    Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue
    Log "build start, root=$Root"

    # 0. BCD backup (keep the first one ever taken)
    $bak = Join-Path $Root 'bcd-backup.bak'
    if (-not (Test-Path -LiteralPath $bak)) { Run bcdedit.exe @('/export', $bak) | Out-Null; Log 'BCD exported' }

    # 1. drop old entries from a previous build (entry first, then its device options)
    & bcdedit.exe /deletevalue '{bootmgr}' bootsequence 2>&1 | Out-Null
    foreach ($f in 'entry-guid.txt', 'entry-guid.tmp', 'opt-guid.txt') {
        $p = Join-Path $pe $f
        if (Test-Path -LiteralPath $p) {
            $g = (Get-Content -LiteralPath $p -Raw).Trim()
            & bcdedit.exe /delete $g /f 2>&1 | Out-Null
            Remove-Item -LiteralPath $p -Force
            Log "removed old BCD object $g"
        }
    }

    # 2. source WinRE image: recovery partition first, then System32\Recovery (RE disabled case)
    $src = $null
    $info = (& reagentc.exe /info 2>&1) -join "`n"
    # match the path itself, not the label (label text is localized)
    if ($info -match '(\\\\\?\\GLOBALROOT\\device\\harddisk\d+\\partition\d+\\\S+)') {
        $c = Join-Path $Matches[1] 'Winre.wim'
        if ([IO.File]::Exists($c)) { $src = $c }
    }
    if (-not $src) {
        $c = Join-Path $env:SystemRoot 'System32\Recovery\Winre.wim'
        if ([IO.File]::Exists($c)) { $src = $c }
    }
    if (-not $src) { Log "reagentc output: $info"; throw 'WinRE image not found (reagentc /info shows no location, no System32\Recovery\Winre.wim)' }
    Log "source $src"
    [IO.File]::Copy($src, $tmp, $true)
    [IO.File]::SetAttributes($tmp, [IO.FileAttributes]::Normal)
    Log ('copied ' + (Get-Item -LiteralPath $tmp).Length + ' bytes')

    $sdi = Join-Path $env:SystemRoot 'Boot\DVD\EFI\boot.sdi'
    if (-not [IO.File]::Exists($sdi)) { $sdi = Join-Path $env:SystemRoot 'Boot\DVD\PCAT\boot.sdi' }
    if (-not [IO.File]::Exists($sdi)) { throw 'boot.sdi not found' }
    $sdiDst = Join-Path $pe 'boot.sdi'
    if (Test-Path -LiteralPath $sdiDst) { [IO.File]::SetAttributes($sdiDst, [IO.FileAttributes]::Normal) }
    [IO.File]::Copy($sdi, $sdiDst, $true)

    # 3. inject df_run.cmd + winpeshl.ini
    if (Test-Path -LiteralPath $mnt) {
        & $dism /Unmount-Image "/MountDir:$mnt" /Discard 2>&1 | Out-Null
        Remove-Item -LiteralPath $mnt -Recurse -Force -ErrorAction SilentlyContinue
    }
    & $dism /Cleanup-Wim 2>&1 | Out-Null
    New-Item -ItemType Directory -Force $mnt | Out-Null
    Run $dism @('/Mount-Image', "/ImageFile:$tmp", '/Index:1', "/MountDir:$mnt") | Out-Null
    $mounted = $true
    Log 'mounted'
    Copy-Item -LiteralPath (Join-Path $Root 'df_run.cmd') -Destination (Join-Path $mnt 'Windows\System32\df_run.cmd') -Force
    $ini = "[LaunchApps]`r`nX:\Windows\System32\cmd.exe, /c X:\Windows\System32\df_run.cmd`r`n"
    [IO.File]::WriteAllText((Join-Path $mnt 'Windows\System32\winpeshl.ini'), $ini, [Text.ASCIIEncoding]::new())
    Run $dism @('/Unmount-Image', "/MountDir:$mnt", '/Commit') | Out-Null
    $mounted = $false
    Remove-Item -LiteralPath $mnt -Recurse -Force -ErrorAction SilentlyContinue
    Log 'committed'

    $wim = Join-Path $pe 'winre.wim'
    if (Test-Path -LiteralPath $wim) { [IO.File]::SetAttributes($wim, [IO.FileAttributes]::Normal); Remove-Item -LiteralPath $wim -Force }
    Move-Item -LiteralPath $tmp -Destination $wim

    # 4. hidden BCD entry (not added to the boot menu; used only via /bootsequence)
    # firmware_type is not localized, unlike bcdedit's text output
    $loader = if ($env:firmware_type -eq 'Legacy') { 'winload.exe' } else { 'winload.efi' }
    $opt = NewGuid (Run bcdedit.exe @('/create', '/d', 'DefenderOffline ramdisk', '/device'))
    Run bcdedit.exe @('/set', $opt, 'ramdisksdidevice', "partition=$drv") | Out-Null
    Run bcdedit.exe @('/set', $opt, 'ramdisksdipath', "$rel\pe\boot.sdi") | Out-Null
    [IO.File]::WriteAllText((Join-Path $pe 'opt-guid.txt'), $opt, [Text.ASCIIEncoding]::new())
    $ent = NewGuid (Run bcdedit.exe @('/create', '/d', 'DefenderOffline PE', '/application', 'osloader'))
    $entTmp = Join-Path $pe 'entry-guid.tmp'
    [IO.File]::WriteAllText($entTmp, $ent, [Text.ASCIIEncoding]::new())
    $dev = "ramdisk=[$drv]$rel\pe\winre.wim,$opt"
    Run bcdedit.exe @('/set', $ent, 'device', $dev) | Out-Null
    Run bcdedit.exe @('/set', $ent, 'osdevice', $dev) | Out-Null
    Run bcdedit.exe @('/set', $ent, 'path', "\windows\system32\$loader") | Out-Null
    Run bcdedit.exe @('/set', $ent, 'systemroot', '\windows') | Out-Null
    Run bcdedit.exe @('/set', $ent, 'winpe', 'yes') | Out-Null
    Run bcdedit.exe @('/set', $ent, 'nx', 'OptIn') | Out-Null
    Run bcdedit.exe @('/set', $ent, 'inherit', '{bootloadersettings}') | Out-Null
    Run bcdedit.exe @('/set', $ent, 'restartonfailure', 'no') | Out-Null
    Run bcdedit.exe @('/set', $ent, 'bootmenupolicy', 'Standard') | Out-Null
    Move-Item -LiteralPath $entTmp -Destination (Join-Path $pe 'entry-guid.txt') -Force
    Log "BCD entry $ent device $opt"
    Log 'OK'
    exit 0
}
catch {
    Log ('ERROR ' + $_.Exception.Message)
    if ($mounted) { & $dism /Unmount-Image "/MountDir:$mnt" /Discard 2>&1 | Out-Null }
    Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue
    exit 1
}
