# GeoDark 安装脚本：把构建产物安装到用户程序目录并接管开机自启。
#
# 为什么要安装而不是直接从 build\Release 运行：
#   实测（2026-10，见 docs/TROUBLESHOOTING.md 第 9 节）开发仓库路径可能被系统
#   的路径级安全判定标记为“Internet 区域”，导致两个症状：
#     1. 双击/开机自启时弹出“打开文件 - 安全警告”；
#     2. GeoDarkUI.exe 的 WebView2 环境创建失败，窗口一片空白。
#   同一份 exe 复制到用户程序目录（或任何未被标记的路径）后两个症状全部消失。
#   本脚本负责：停止旧进程 → 复制并解除锁定 → 校验签名 → 把 HKCU Run 键
#   指向安装目录 → 启动守护进程。
#
# 用法（仓库根目录）：
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\install.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\install.ps1 -NoStart
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\install.ps1 -InstallDir "D:\Tools\GeoDark"

[CmdletBinding()]
param(
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\GeoDark",
    [string]$SourceDir = '',
    [switch]$NoStart
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
if (-not $SourceDir) { $SourceDir = Join-Path $repo 'build\Release' }

$names = @('GeoDark.exe', 'GeoDarkUI.exe')
$sources = @()
foreach ($n in $names) {
    $p = Join-Path $SourceDir $n
    if (Test-Path $p) { $sources += $p }
}
if (-not $sources) {
    throw "未找到可安装的程序：$SourceDir（请先执行 cmake --build build --config Release）"
}

# 1) 停止正在运行的实例（避免文件占用）
foreach ($n in $names) {
    Get-Process -Name ($n -replace '\.exe$', '') -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
}
Start-Sleep -Milliseconds 500

# 2) 复制并解除锁定（清理可能存在的 MotW，双保险）
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
foreach ($src in $sources) {
    $dst = Join-Path $InstallDir (Split-Path $src -Leaf)
    Copy-Item -LiteralPath $src -Destination $dst -Force
    Unblock-File -LiteralPath $dst -ErrorAction SilentlyContinue
    Write-Host ("已安装: {0}  ({1:N0} 字节)" -f $dst, (Get-Item $dst).Length)
}

# 3) 校验签名（未签名只警告不阻断）
foreach ($n in $names) {
    $p = Join-Path $InstallDir $n
    if (Test-Path $p) {
        $sig = Get-AuthenticodeSignature -LiteralPath $p
        Write-Host ("{0}: 签名状态 = {1}" -f $n, $sig.Status)
        if ($sig.Status -ne 'Valid') {
            Write-Warning "$n 未通过签名校验，开机自启/首次运行可能仍弹安全警告。请先运行 tools\sign.ps1 后重新安装。"
        }
    }
}

# 4) 更新开机自启（HKCU Run -> 安装目录的 GeoDark.exe）
$runKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$runValue = '"{0}"' -f (Join-Path $InstallDir 'GeoDark.exe')
if (-not (Get-Item $runKey -ErrorAction SilentlyContinue).Property.Contains('GeoDark')) {
    New-ItemProperty -Path $runKey -Name 'GeoDark' -Value $runValue -PropertyType String -Force | Out-Null
} else {
    Set-ItemProperty -Path $runKey -Name 'GeoDark' -Value $runValue
}
Write-Host "开机自启已指向: $runValue"

# 5) 启动守护进程
if (-not $NoStart) {
    Start-Process -FilePath (Join-Path $InstallDir 'GeoDark.exe')
    Start-Sleep -Milliseconds 900
    if (Get-Process -Name 'GeoDark' -ErrorAction SilentlyContinue) {
        Write-Host '守护进程 GeoDark.exe 已在运行。'
    } else {
        Write-Warning '守护进程未检测到，请手动运行安装目录中的 GeoDark.exe。'
    }
}

Write-Host "`n安装完成：$InstallDir"
Write-Host 'GeoDarkUI 请从安装目录启动（可将快捷方式发送到桌面/开始菜单）。'
