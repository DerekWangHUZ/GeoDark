param(
    [ValidateRange(1, 1440)][int]$Minutes = 1,
    [ValidateRange(1, 3600)][int]$SampleSeconds = 60,
    [int]$TargetProcessId = 0
)

$ErrorActionPreference = 'Stop'
if ($TargetProcessId -eq 0) {
    $targets = @(Get-Process -Name GeoDark -ErrorAction Stop)
    if ($targets.Count -ne 1) { throw "Expected one GeoDark process; found $($targets.Count)." }
    $TargetProcessId = $targets[0].Id
}

function Get-Snapshot {
    param([int]$ProcessId)
    $process = Get-Process -Id $ProcessId -ErrorAction Stop
    $perf = Get-CimInstance Win32_PerfRawData_PerfProc_Process -Filter "IDProcess=$ProcessId"
    if (-not $perf) { throw "No performance counter for process $ProcessId." }
    $threads = @(Get-CimInstance Win32_PerfRawData_PerfProc_Thread -Filter "IDProcess=$ProcessId")
    $switches = @{}
    foreach ($thread in $threads) {
        $switches[[int]$thread.IDThread] = [long]$thread.ContextSwitchesPersec
    }
    [pscustomobject]@{
        Time = Get-Date
        CpuSeconds = [double]$process.CPU
        WorkingSetBytes = [long]$perf.WorkingSet
        PrivateWorkingSetBytes = [long]$perf.WorkingSetPrivate
        PrivateBytes = [long]$perf.PrivateBytes
        ThreadSwitches = $switches
    }
}

$first = Get-Snapshot -ProcessId $TargetProcessId
$samples = [System.Collections.Generic.List[object]]::new()
$samples.Add($first)
$previous = $first
$switchDelta = 0L
$deadline = $first.Time.AddMinutes($Minutes)
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Seconds $SampleSeconds
    $current = Get-Snapshot -ProcessId $TargetProcessId
    foreach ($entry in $current.ThreadSwitches.GetEnumerator()) {
        if ($previous.ThreadSwitches.ContainsKey($entry.Key)) {
            $delta = $entry.Value - $previous.ThreadSwitches[$entry.Key]
            if ($delta -gt 0) { $switchDelta += $delta }
        }
    }
    $samples.Add($current)
    $previous = $current
}

$elapsed = ($previous.Time - $first.Time).TotalSeconds
$cpuPercent = if ($elapsed -gt 0) { 100 * ($previous.CpuSeconds - $first.CpuSeconds) / $elapsed } else { 0 }
$private = @($samples | ForEach-Object PrivateWorkingSetBytes)
$total = @($samples | ForEach-Object WorkingSetBytes)
$binary = Get-Item (Join-Path $PSScriptRoot '..\build\Release\GeoDark.exe') -ErrorAction SilentlyContinue
[pscustomobject]@{
    ProcessId = $TargetProcessId
    DurationSeconds = [math]::Round($elapsed, 1)
    CpuPercentOfOneCore = [math]::Round($cpuPercent, 4)
    PrivateWorkingSetMeanMB = [math]::Round((($private | Measure-Object -Average).Average / 1MB), 3)
    PrivateWorkingSetMaxMB = [math]::Round((($private | Measure-Object -Maximum).Maximum / 1MB), 3)
    TotalWorkingSetMaxMB = [math]::Round((($total | Measure-Object -Maximum).Maximum / 1MB), 3)
    ThreadContextSwitchesObserved = $switchDelta
    ExeBytes = if ($binary) { $binary.Length } else { $null }
}
