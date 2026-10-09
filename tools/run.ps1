# GeoDark 开发者快速测试：把 build\Release 的产物镜像到干净路径并从那里启动。
#
# 为什么需要它：仓库路径（...\source\repos\GeoDark\）可能被安全软件的路径级判定
# 标记为 Internet 区域——直接运行 build\Release 下的 exe 会弹“打开文件 - 安全警告”，
# GeoDarkUI 的 WebView2 也可能初始化失败（白屏）。镜像到 %TEMP% 下运行即可完全绕开
# （详见 docs/TROUBLESHOOTING.md 第 9 条）。
#
# 用法（仓库根目录）：
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run.ps1            # 镜像并启动 UI
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run.ps1 -Build     # 先构建再启动
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run.ps1 -Daemon    # 同时启动守护进程
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run.ps1 -Verify    # 只检查路径判定状态

[CmdletBinding()]
param(
    [switch]$Build,
    [switch]$Daemon,
    [switch]$Verify
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$src = Join-Path $repo 'build\Release'
$mirror = Join-Path $env:TEMP 'GeoDarkDevRun'

if ($Verify) {
    powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'diag-zone.ps1') `
        (Join-Path $src 'GeoDarkUI.exe') (Join-Path $mirror 'GeoDarkUI.exe')
    return
}

if ($Build) {
    $cmake = Get-Item 'C:\Program Files (x86)\Microsoft Visual Studio\2022\*\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
    if (-not $cmake) { throw '未找到 cmake.exe，请安装 VS2022 CMake 组件。' }
    & $cmake --build (Join-Path $repo 'build') --config Release
    if ($LASTEXITCODE -ne 0) { throw "构建失败（退出码 $LASTEXITCODE）" }
}

foreach ($n in @('GeoDark.exe', 'GeoDarkUI.exe')) {
    $p = Join-Path $src $n
    if (-not (Test-Path $p)) { throw "缺少 $p，请先构建。" }
}

New-Item -ItemType Directory -Force -Path $mirror | Out-Null
foreach ($n in @('GeoDark.exe', 'GeoDarkUI.exe')) {
    Copy-Item (Join-Path $src $n) $mirror -Force
    Unblock-File (Join-Path $mirror $n) -ErrorAction SilentlyContinue
}

if ($Daemon) { Start-Process -FilePath (Join-Path $mirror 'GeoDark.exe') }
Start-Process -FilePath (Join-Path $mirror 'GeoDarkUI.exe')
Write-Host "已从镜像路径启动：$mirror（不是仓库路径，不受路径级判定影响）"
Write-Host '提示：正式使用请运行 tools\install.ps1 安装到用户程序目录。'
