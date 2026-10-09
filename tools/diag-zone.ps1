# Print the URL security zone Windows assigns to a file/directory path.
# This is the verdict the Attachment Manager ("Open File - Security Warning")
# and the WebView2 loader act upon. A path reported as Internet zone (3) will
# trigger startup prompts for its executables and can break WebView2-based UIs
# (white window), even when no Zone.Identifier ADS exists on any file.
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\diag-zone.ps1 <path> [morePaths...]
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\diag-zone.ps1 build\Release\GeoDarkUI.exe
#
# Zone reference: 0=LocalMachine 1=Intranet 2=Trusted 3=Internet 4=Restricted

param([Parameter(Mandatory = $true, Position = 0, ValueFromRemainingArguments = $true)][string[]]$Path)

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

foreach ($p in $Path) {
    $resolved = (Resolve-Path -LiteralPath $p -ErrorAction SilentlyContinue)
    if (-not $resolved) { Write-Output ("{0} => path not found" -f $p); continue }
    $full = $resolved.Path
    $url = 'file:///' + ($full -replace '\\', '/')
    $result = [GeoZone.Probe]::MapZone($url)
    $zone = if ($result -match 'zone=(\d+)') { $Matches[1] } else { '?' }
    $zoneName = switch ($zone) {
        '0' { 'LocalMachine' }
        '1' { 'Intranet' }
        '2' { 'Trusted' }
        '3' { 'Internet' }
        '4' { 'Restricted' }
        default { 'Unknown' }
    }
    $verdict = if ($zone -eq '0') { 'OK (trusted local path)' } else { 'FLAGGED: expect security prompts here; WebView2 UI may white-screen from this path' }
    Write-Output ("{0}`n  => {1}  [{2}]`n  => {3}" -f $full, $result, $zoneName, $verdict)
}
