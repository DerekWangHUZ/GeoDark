# Print the URL security zone Windows assigns to a file/directory path.
# This is the verdict the Attachment Manager ("Open File - Security Warning")
# and the WebView2 loader act upon. A path reported as Internet zone (3) will
# trigger startup prompts for its executables and can break WebView2-based UIs
# (white window), even when no Zone.Identifier ADS exists on any file.
#
# The usual cause is a Low mandatory-integrity label on the directory. With
# -Fix this script resets that label to Medium (directories get OI|CI
# inheritance so children follow), then re-checks the zone.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\diag-zone.ps1 <path> [morePaths...]
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\diag-zone.ps1 <dir> -Fix
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\diag-zone.ps1 build\Release\GeoDarkUI.exe
#
# Zone reference: 0=LocalMachine 1=Intranet 2=Trusted 3=Internet 4=Restricted
# Integrity reference: Low/Medium/High/System

param(
    [switch]$Fix,
    [Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments = $true)][string[]]$Path
)

$src = @'
using System;
using System.Runtime.InteropServices;
namespace GeoZone {
  [ComImport, Guid("7b8a2d94-0ac9-11d1-896c-00c04fb6bfc4")]
  public class InternetSecurityManager {}
  [ComImport, Guid("79EAC9EE-BAF9-11CE-8C82-00AA004BA90B"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
  public interface IInternetSecurityManager {
    [PreserveSig] int SetSecuritySite(IntPtr pSite);
    [PreserveSig] int GetSecuritySite(IntPtr pSite);
    [PreserveSig] int MapUrlToZone([MarshalAs(UnmanagedType.LPWStr)] string url, out int zone, int flags);
    [PreserveSig] int GetSecurityId([MarshalAs(UnmanagedType.LPWStr)] string url, IntPtr pbSecurityId, ref int cbSecurityId, int reserved);
    [PreserveSig] int ProcessUrlAction([MarshalAs(UnmanagedType.LPWStr)] string url, int action, IntPtr pPolicy, ref int cbPolicy, IntPtr pContext, int cbContext, int flags, int reserved);
    [PreserveSig] int QueryCustomPolicy([MarshalAs(UnmanagedType.LPWStr)] string url, ref Guid guidKey, IntPtr ppPolicy, ref int cbPolicy, IntPtr pContext, int cbContext, int flags);
    [PreserveSig] int SetZoneMapping(int zone, [MarshalAs(UnmanagedType.LPWStr)] string pattern, int flags);
    [PreserveSig] int GetZoneMappings(int zone, out System.Runtime.InteropServices.ComTypes.IEnumString ppenumString, int flags);
  }
  public static class Probe {
    public static string MapZone(string url) {
      IInternetSecurityManager mgr = (IInternetSecurityManager)new InternetSecurityManager();
      int zone;
      int hr = mgr.MapUrlToZone(url, out zone, 0);
      return string.Format("hr=0x{0:X8} zone={1}", hr, zone);
    }
  }
}
'@
Add-Type -TypeDefinition $src -Language CSharp

function Get-ZoneResult([string]$full) {
    $url = 'file:///' + ($full -replace '\\', '/')
    $result = [GeoZone.Probe]::MapZone($url)
    $zone = if ($result -match 'zone=(\d+)') { $Matches[1] } else { '?' }
    return [pscustomobject]@{ Raw = $result; Zone = $zone }
}

function Get-IntegrityLevel([string]$full) {
    $text = (icacls $full 2>&1) -join "`n"
    if ($text -match 'Mandatory Label\\([A-Za-z]+) Mandatory Level') { return $Matches[1] }
    return 'Medium'
}

foreach ($p in $Path) {
    $resolved = (Resolve-Path -LiteralPath $p -ErrorAction SilentlyContinue)
    if (-not $resolved) { Write-Output ("{0} => path not found" -f $p); continue }
    $full = $resolved.Path
    $zone = Get-ZoneResult $full
    $zoneName = switch ($zone.Zone) {
        '0' { 'LocalMachine' }
        '1' { 'Intranet' }
        '2' { 'Trusted' }
        '3' { 'Internet' }
        '4' { 'Restricted' }
        default { 'Unknown' }
    }
    $level = Get-IntegrityLevel $full
    Write-Output ("{0}`n  => {1}  [{2}]`n  => integrity={3}" -f $full, $zone.Raw, $zoneName, $level)

    if ($Fix -and $zone.Zone -ne '0' -and $level -eq 'Low') {
        if (Test-Path -LiteralPath $full -PathType Container) {
            icacls $full /setintegritylevel '(OI)(CI)M' | Out-Null
        } else {
            icacls $full /setintegritylevel M | Out-Null
        }
        # MapUrlToZone caches the verdict inside a process, so the repair is
        # verified from a fresh PowerShell process instead of re-querying here.
        $verifyOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath $full 2>&1
        $verifyLine = ($verifyOutput | Select-String 'zone=' | Select-Object -First 1)
        $afterLevel = Get-IntegrityLevel $full
        Write-Output ("  => repaired: integrity={0}" -f $afterLevel)
        Write-Output ("  => rechecked in a new process: {0}" -f $(if ($verifyLine) { $verifyLine.Line.Trim() } else { 'no verdict' }))
        if (-not $verifyLine -or $verifyLine.Line -notmatch 'zone=0') {
            Write-Output "  => the label may sit on an ancestor directory; rerun -Fix on that directory"
        }
    } elseif ($zone.Zone -eq '0') {
        Write-Output "  => OK (trusted local path)"
    } else {
        $hint = if ($level -eq 'Low') { "run again with -Fix to reset the Low integrity label to Medium" } else { "inspect the path's inherited permissions and parent directories" }
        Write-Output ("  => FLAGGED: security prompts and WebView2 failures are expected here; {0}" -f $hint)
    }
}