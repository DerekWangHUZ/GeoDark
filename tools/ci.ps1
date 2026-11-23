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

# 完整性标签守卫：目录被标成 Low Mandatory Level 时，Windows 会把其中的可执行文件
# 一律按 Internet 区对待（安全警告弹窗、WebView2 白屏），产物不可信。构建前强制校正。
function Ensure-MediumIntegrity {
    $label = (icacls $repo 2>&1 | Select-String 'Mandatory Label\Low' -SimpleMatch)
    if ($label) {
        Write-Host '== 完整性标签：检测到 Low，正在校正为 Medium ==' -ForegroundColor Yellow
        cmd /c "icacls `"$repo`" /setintegritylevel (OI)(CI)Medium /C /Q" | Out-Null
        if ((icacls $repo 2>&1 | Select-String 'Mandatory Label\Low' -SimpleMatch)) {
            throw '目录完整性标签仍为 Low，校正失败，已中止构建。请手动执行：icacls "<目录>" /setintegritylevel (OI)(CI)Medium'
        }
        Write-Host '   已校正为 Medium' -ForegroundColor Green
    }
}
Ensure-MediumIntegrity

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
$buildPath = Join-Path $repo $BuildDir

if ($Configure) {
    Write-Host '== 配置 ==' -ForegroundColor Cyan
    & $cmake -S $repo -B $buildPath -A x64
    if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败（exit $LASTEXITCODE）" }

    # 守卫：assemblyIdentity 版本必须是四段，否则 Windows 加载器报
    # 「并行配置不正确」而构建仍然全绿（v1.0.0 曾因此发布失败）。
    $manifest = Join-Path $buildPath 'geodark.manifest'
    if (-not (Test-Path $manifest)) { throw "未生成清单: $manifest" }
    if ((Get-Content $manifest -Raw) -notmatch 'version="\d+\.\d+\.\d+\.\d+"') {
        throw "geodark.manifest 的 assemblyIdentity version 不是四段式，Windows 将报并行配置错误。"
    }
}

if ($Build) {
    Write-Host '== 构建 ==' -ForegroundColor Cyan
    & $cmake --build $buildPath --config Release
    if ($LASTEXITCODE -ne 0) { throw "构建失败（exit $LASTEXITCODE）" }
}

if ($Test) {
    Write-Host '== 测试 ==' -ForegroundColor Cyan
    $ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
    & $ctest --test-dir $buildPath -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "测试失败（exit $LASTEXITCODE）" }
}

Write-Host '== 全部通过 ==' -ForegroundColor Green
