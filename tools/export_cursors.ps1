# tools/export_cursors.ps1
#
# Exports the ACTIVE Windows cursor scheme (HKCU:\Control Panel\Cursors) into
# src/SexyAppFramework/platform/uwp/UwpCursors.h so the gamepad-driven virtual
# cursor on UWP/Xbox uses the exact same textures as the desktop mouse.
#
# Usage:
#   pwsh tools/export_cursors.ps1
#
# The header stores each cursor as raw 0xAARRGGBB pixels (the MemoryImage bit
# format used by the Sexy App Framework), extracted from the .cur files. Only
# classic DIB (non-PNG) .cur entries are supported.

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$outPath = Join-Path $repoRoot "src\SexyAppFramework\platform\uwp\UwpCursors.h"
$wantSize = 32

function Get-CursorSource($regKey) {
    $v = Get-ItemProperty -Path $regKey -ErrorAction SilentlyContinue
    $map = @{
        Pointer  = $v.Arrow
        Hand     = $v.Hand
        Text     = $v.IBeam
        Move     = $v.SizeAll
        No       = $v.No
    }
    return $map
}

function Read-CurPixels($path, $wantSize) {
    $b = [System.IO.File]::ReadAllBytes($path)
    $count = [BitConverter]::ToUInt16($b, 4)
    $entry = $null
    $smallest = $null
    for ($i = 0; $i -lt $count; $i++) {
        $o = 6 + $i * 16
        $w = $b[$o]; $h = $b[$o + 1]
        if ($w -eq 0) { $w = 256 }
        if ($h -eq 0) { $h = 256 }
        if ($null -eq $smallest -or $w -lt $smallest.W) { $smallest = @{ W = $w; H = $h; O = $o } }
        if ($w -eq $wantSize -and $h -eq $wantSize) { $entry = @{ W = $w; H = $h; O = $o }; break }
    }
    if ($null -eq $entry) {
        if ($null -eq $smallest) { throw "no images in $path" }
        $entry = $smallest
    }
    $o = $entry.O
    $p = [BitConverter]::ToUInt32($b, $o + 12)
    $size = [BitConverter]::ToUInt32($b, $o + 8)
    $hx = [BitConverter]::ToUInt16($b, $o + 4)
    $hy = [BitConverter]::ToUInt16($b, $o + 6)

    $biSize = [BitConverter]::ToUInt32($b, $p)
    $biW = [BitConverter]::ToInt32($b, $p + 4)
    $biH = [BitConverter]::ToInt32($b, $p + 8)
    $biBpp = [BitConverter]::ToUInt16($b, $p + 14)
    $biComp = [BitConverter]::ToUInt32($b, $p + 16)
    if ($biComp -ne 0) { throw "unsupported compression $biComp in $path" }

    $imgH = [Math]::Abs($biH) / 2
    $xorOff = $p + $biSize
    $pal = @()
    if ($biBpp -le 8) {
        $colorsUsed = [BitConverter]::ToUInt32($b, $p + 32)
        $palSize = if ($colorsUsed -gt 0) { $colorsUsed } else { 1 -shl $biBpp }
        for ($j = 0; $j -lt $palSize; $j++) {
            $q = $xorOff + $j * 4
            $pal += ,@([int]$b[$q + 2], [int]$b[$q + 1], [int]$b[$q])
        }
        $xorOff += $palSize * 4
    }
    $rowBytes = [Math]::Ceiling($biW * $biBpp / 32) * 4
    $andRowBytes = [Math]::Ceiling($biW / 32) * 4
    $andBase = $xorOff + $imgH * $rowBytes

    $pixels = New-Object 'System.Drawing.Color[,]' $imgH, $biW
    for ($y = 0; $y -lt $imgH; $y++) {
        $srcRow = $xorOff + ($imgH - 1 - $y) * $rowBytes
        $andRow = $andBase + ($imgH - 1 - $y) * $andRowBytes
        for ($x = 0; $x -lt $biW; $x++) {
            $a = 255; $r = 0; $g = 0; $bl = 0
            if ($biBpp -eq 32) {
                $q = $srcRow + $x * 4
                $bl = $b[$q]; $g = $b[$q + 1]; $r = $b[$q + 2]; $a = $b[$q + 3]
            } elseif ($biBpp -eq 24) {
                $q = $srcRow + $x * 3
                $bl = $b[$q]; $g = $b[$q + 1]; $r = $b[$q + 2]
            } else {
                $bit = $x * $biBpp
                $byteIdx = [Math]::Floor($bit / 8); $shift = 8 - $biBpp - ($bit % 8)
                $idx = ($b[$srcRow + $byteIdx] -shr $shift) -band ((1 -shl $biBpp) - 1)
                $c = $pal[$idx]; $r = $c[0]; $g = $c[1]; $bl = $c[2]
            }
            $andBit = ($b[$andRow + [Math]::Floor($x / 8)] -shr (7 - ($x % 8))) -band 1
            if ($andBit -eq 1) { $a = 0 }
            $pixels[$y, $x] = [System.Drawing.Color]::FromArgb($a, $r, $g, $bl)
        }
    }
    return @{ Pixels = $pixels; W = [int]$biW; H = [int]$imgH; HX = [int]$hx; HY = [int]$hy }
}

$src = Get-CursorSource "HKCU:\Control Panel\Cursors"

# role name -> (registry value, id). Only DIB .cur files are decoded; ANI files
# (Wait/AppStarting) are skipped and fall back to the pointer.
$roles = @(
    @{ Id = "Pointer"; Reg = "Pointer"; Fb = $null },
    @{ Id = "Hand";    Reg = "Hand";    Fb = "Pointer" },
    @{ Id = "Text";    Reg = "Text";    Fb = "Pointer" },
    @{ Id = "No";      Reg = "No";      Fb = "Pointer" },
    @{ Id = "Move";    Reg = "Move";    Fb = "Pointer" }
)

$data = @{}
foreach ($r in $roles) {
    $path = $src[$r.Reg]
    $ok = $false
    if ($path -and (Test-Path -LiteralPath $path)) {
        if ([System.IO.Path]::GetExtension($path) -ieq ".cur") {
            try {
                $decoded = Read-CurPixels $path $wantSize
                $data[$r.Id] = $decoded
                $ok = $true
                Write-Host ("{0,-8} <- {1}  ({2}x{3} hotspot {4},{5})" -f $r.Id, $path, $decoded.W, $decoded.H, $decoded.HX, $decoded.HY)
            } catch {
                Write-Warning ("{0,-8} decode failed ({1}), falling back" -f $r.Id, $_.Exception.Message)
            }
        } else {
            Write-Warning ("{0,-8} not a .cur file ({1}), falling back" -f $r.Id, $path)
        }
    } else {
        Write-Warning ("{0,-8} missing ({1}), falling back" -f $r.Id, $path)
    }
    if (-not $ok -and $r.Fb -and $data.ContainsKey($r.Fb)) {
        $data[$r.Id] = $data[$r.Fb]
        Write-Warning ("{0,-8} using {1} fallback" -f $r.Id, $r.Fb)
    }
}

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("/*")
[void]$sb.AppendLine(" * Generated by tools/export_cursors.ps1 from the active Windows cursor scheme.")
[void]$sb.AppendLine(" * Do not edit by hand -- re-run the script after changing your Windows cursor.")
[void]$sb.AppendLine(" */")
[void]$sb.AppendLine("#ifndef __UWP_CURSORS_H__")
[void]$sb.AppendLine("#define __UWP_CURSORS_H__")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("#include <cstdint>")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("namespace Sexy {")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("struct UwpCursorData")
[void]$sb.AppendLine("{")
[void]$sb.AppendLine("    const uint32_t* mPixels; // 0xAARRGGBB, mWidth*mHeight entries")
[void]$sb.AppendLine("    int mWidth;")
[void]$sb.AppendLine("    int mHeight;")
[void]$sb.AppendLine("    int mHotspotX;")
[void]$sb.AppendLine("    int mHotspotY;")
[void]$sb.AppendLine("};")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("enum UwpCursorId")
[void]$sb.AppendLine("{")
[void]$sb.AppendLine("    UWP_CURSOR_POINTER = 0,")
[void]$sb.AppendLine("    UWP_CURSOR_HAND,")
[void]$sb.AppendLine("    UWP_CURSOR_TEXT,")
[void]$sb.AppendLine("    UWP_CURSOR_NO,")
[void]$sb.AppendLine("    UWP_CURSOR_MOVE,")
[void]$sb.AppendLine("    UWP_CURSOR_COUNT")
[void]$sb.AppendLine("};")
[void]$sb.AppendLine("")

foreach ($r in $roles) {
    $d = $data[$r.Id]
    $name = "kUwpCursorPixels_" + $r.Id
    $n = $d.W * $d.H
    [void]$sb.AppendLine("static const uint32_t $name[$n] = {")
    $line = ""
    $perLine = 8
    for ($i = 0; $i -lt $n; $i++) {
        $x = $i % $d.W
        $y = [Math]::Floor($i / $d.W)
        $c = $d.Pixels[$y, $x]
        $argb = (([int]$c.A -shl 24) -bor ([int]$c.R -shl 16) -bor ([int]$c.G -shl 8) -bor [int]$c.B)
        $hex = "0x{0:X8}" -f $argb
        $line += $hex
        if ($i -lt $n - 1) { $line += ", " }
        if (($i % $perLine) -eq ($perLine - 1) -and $i -lt $n - 1) {
            [void]$sb.AppendLine("    " + $line)
            $line = ""
        }
    }
    if ($line) { [void]$sb.AppendLine("    " + $line) }
    [void]$sb.AppendLine("};")
    [void]$sb.AppendLine("")
}

[void]$sb.AppendLine("static const UwpCursorData kUwpCursorData[UWP_CURSOR_COUNT] = {")
foreach ($r in $roles) {
    $d = $data[$r.Id]
    [void]$sb.AppendLine("    { kUwpCursorPixels_" + $r.Id + ", " + $d.W + ", " + $d.H + ", " + $d.HX + ", " + $d.HY + " },")
}
[void]$sb.AppendLine("};")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("} // namespace Sexy")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("#endif //__UWP_CURSORS_H__")

$dir = Split-Path -Parent $outPath
if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
[System.IO.File]::WriteAllText($outPath, $sb.ToString())
Write-Host ""
Write-Host "Wrote $outPath"
