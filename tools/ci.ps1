# GeoDark CI / 本地一键验证脚本。
# 与 .gitlab-ci.yml 共用；本地也可单独执行任意阶段。
#
# 用法（仓库根目录）：
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\ci.ps1              # 全部阶段
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\ci.ps1 -Configure -Build -Test
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\ci.ps1 -Test        # 只跑测试

[CmdletBinding()]
param(
    [switch]$Configure,
    [switch]$Build,
    [switch]$Test,
    [string]$BuildDir = 'build'
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

# 未指定任何阶段时执行全部。
if (-not ($Configure -or $Build -or $Test)) {
    $Configure = $true; $Build = $true; $Test = $true
}

# 定位 CMake：优先 PATH，其次 VS2022 自带副本。
function Find-CMake {
    $cmd = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        'C:\Program Files\Microsoft Visual Studio\2022\*\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    )
    foreach ($pattern in $candidates) {
        $hit = Get-Item $pattern -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }
    throw '未找到 cmake.exe，请安装 CMake 或 VS2022 C++ 桌面开发负载。'
}

$cmake = Find-CMake
Write-Host "使用 CMake: $cmake"

if ($Configure) {
    Write-Host '== 配置 ==' -ForegroundColor Cyan
    & $cmake -S $repo -B (Join-Path $repo $BuildDir) -A x64
    if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败（exit $LASTEXITCODE）" }
}

if ($Build) {
    Write-Host '== 构建 ==' -ForegroundColor Cyan
    & $cmake --build (Join-Path $repo $BuildDir) --config Release
    if ($LASTEXITCODE -ne 0) { throw "构建失败（exit $LASTEXITCODE）" }
}

if ($Test) {
    Write-Host '== 测试 ==' -ForegroundColor Cyan
    $ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
    & $ctest --test-dir (Join-Path $repo $BuildDir) -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "测试失败（exit $LASTEXITCODE）" }
}

Write-Host '== 全部通过 ==' -ForegroundColor Green
