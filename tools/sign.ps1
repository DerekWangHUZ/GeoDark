# GeoDark 一键代码签名脚本
# 用途：消除“打开文件 - 安全警告”启动弹窗（附件管理器对带 MotW 的未签名 exe 的拦截）。
# 原理：在当前用户证书区创建自建代码签名证书并加入本机信任，然后用 signtool 对 exe 签名。
#
# 用法（在仓库根目录）：
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\sign.ps1              # 只签 build\Release
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\sign.ps1 -Dist        # 同时签 dist\*\*.exe
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\sign.ps1 -Unblock     # 顺带清除仓库内 MotW 标记
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\sign.ps1 -InstallMachineTrust  # 附加：把证书装进本机信任区（需 UAC）
#
# 说明：
#   - 证书存于 Cert:\CurrentUser\My，公钥导入 CurrentUser 与（可选）LocalMachine 信任区。
#     附件管理器（“打开文件 - 安全警告”）只认 LocalMachine 的 TrustedPublisher；要彻底消除
#     弹窗请在首次运行时加 -InstallMachineTrust 并在 UAC 弹窗点“是”。
#   - 每次重新构建后 exe 会变回未签名，需要重新运行本脚本。
#   - 证书有效期 5 年；到期后删除旧证书（certmgr.msc）再运行本脚本即可重建。
#   - 兼容 Windows PowerShell 5.1（OpenFlags）与 PowerShell 7+（X509OpenFlags）。

[CmdletBinding()]
param(
    [string]$RepoRoot = '',
    [switch]$Dist,
    [switch]$Unblock,
    [switch]$InstallMachineTrust,
    [string]$Subject = 'CN=GeoDark (Derek Wang)',
    [int]$Years = 5
)

$ErrorActionPreference = 'Stop'
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $PSScriptRoot }

$SigntoolCandidates = @(
    "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe",
    "$env:ProgramFiles\Windows Kits\10\bin\*\x64\signtool.exe"
)
$signtool = Get-Item $SigntoolCandidates -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if (-not $signtool) { throw "未找到 signtool.exe，请安装 Windows SDK 或手动指定路径。" }

function Get-GeodarkCertificate {
    $existing = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert -ErrorAction SilentlyContinue |
        Where-Object { $_.Subject -eq $Subject -and $_.NotAfter -gt (Get-Date) } |
        Sort-Object NotAfter -Descending | Select-Object -First 1
    if ($existing) {
        Write-Host "使用已有证书: $($existing.Subject) Thumbprint=$($existing.Thumbprint)"
        return $existing
    }

    Write-Host "创建自建代码签名证书: $Subject (有效期 $Years 年) ..."
    $cert = New-SelfSignedCertificate -Subject $Subject -Type CodeSigningCert `
        -KeyAlgorithm RSA -KeyLength 3072 -HashAlgorithm SHA256 `
        -KeyExportPolicy Exportable -KeyUsage DigitalSignature `
        -NotAfter (Get-Date).AddYears($Years) -CertStoreLocation Cert:\CurrentUser\My
    Write-Host "已创建: Thumbprint=$($cert.Thumbprint)"
    return $cert
}

function Get-X509OpenFlagsType {
    # .NET Framework 5.1 里叫 OpenFlags，.NET Core/7+ 里叫 X509OpenFlags
    $t = 'System.Security.Cryptography.X509Certificates.OpenFlags' -as [type]
    if (-not $t) { $t = 'System.Security.Cryptography.X509Certificates.X509OpenFlags' -as [type] }
    if (-not $t) { throw "无法解析 X509 OpenFlags 枚举类型。" }
    return $t
}

function Install-GeodarkTrust {
    param($Certificate)

    $openFlagsType = Get-X509OpenFlagsType
    foreach ($storeName in 'Root', 'TrustedPeople') {
        $store = New-Object System.Security.Cryptography.X509Certificates.X509Store(
            $storeName, [System.Security.Cryptography.X509Certificates.StoreLocation]::CurrentUser)
        try {
            $store.Open($openFlagsType::ReadWrite)
            $already = $store.Certificates | Where-Object { $_.Thumbprint -eq $Certificate.Thumbprint }
            if ($already) {
                Write-Host "信任区 $storeName 已包含该证书，跳过导入。"
            } else {
                Write-Host "正在导入信任区 $storeName ...（首次导入 Root 时系统会弹一次确认框，请在屏幕上点“是”）"
                $store.Add($Certificate)
                Write-Host "已导入信任区: Cert:\CurrentUser\$storeName"
            }
        } finally {
            $store.Close()
        }
    }
}

function Invoke-Sign {
    param($Certificate, [string[]]$Files)

    $targets = @()
    foreach ($file in $Files) {
        $sig = Get-AuthenticodeSignature -LiteralPath $file
        if ($sig.Status -eq 'Valid') {
            Write-Host "跳过（已有效签名）: $file"
        } else {
            $targets += $file
        }
    }
    if (-not $targets) { Write-Host '没有需要签名的文件。'; return }

    # 用随机口令导出临时 PFX，签完即删；口令仅存在于本次进程内
    $pfxPath = Join-Path $env:TEMP "geodark_sign_$PID.pfx"
    try {
        $plain = New-Object byte[] 32
        $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
        try { $rng.GetBytes($plain) } finally { $rng.Dispose() }
        $password = ConvertTo-SecureString -String ([Convert]::ToBase64String($plain)) -Force -AsPlainText
        Export-PfxCertificate -Cert $Certificate -FilePath $pfxPath -Password $password | Out-Null
        & $signtool sign /fd SHA256 /f $pfxPath /p ([Convert]::ToBase64String($plain)) @targets
        if ($LASTEXITCODE -ne 0) { throw "signtool 退出码 $LASTEXITCODE" }
    } finally {
        Remove-Item -LiteralPath $pfxPath -Force -ErrorAction SilentlyContinue
    }

    foreach ($file in $targets) {
        $sig = Get-AuthenticodeSignature -LiteralPath $file
        Write-Host ("{0}  =>  {1}" -f (Split-Path $file -Leaf), $sig.Status)
        if ($sig.Status -ne 'Valid') { throw "签名验证失败: $file ($($sig.StatusMessage))" }
    }
}

# --- 主流程 ---
$patterns = @('GeoDark.exe', 'GeoDarkUI.exe', 'geodark_solar_tests.exe')
$targets = @()
foreach ($pattern in $patterns) {
    $targets += Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'build\Release') -Filter $pattern -File -ErrorAction SilentlyContinue
    if ($Dist) {
        $targets += Get-ChildItem -LiteralPath (Join-Path $RepoRoot 'dist') -Recurse -Filter $pattern -File -ErrorAction SilentlyContinue
    }
}
$targets = $targets | Select-Object -Unique -ExpandProperty FullName
if (-not $targets) { throw "未找到可签名的 exe，请先执行: cmake --build build --config Release" }

if ($Unblock) {
    Write-Host '清除仓库内 GeoDark 文件的 MotW 标记 ...'
    Get-ChildItem -LiteralPath $RepoRoot -Recurse -File -Include GeoDark.exe, GeoDarkUI.exe, *.zip -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty FullName | ForEach-Object { Unblock-File -LiteralPath $_ -ErrorAction SilentlyContinue }
}

$cert = Get-GeodarkCertificate
Install-GeodarkTrust -Certificate $cert

# 附件管理器只认 LocalMachine 的信任判定：把证书装入本机 Root + TrustedPublisher（需要管理员）
if ($InstallMachineTrust) {
    $cerPath = Join-Path $env:TEMP "geodark_trust_$PID.cer"
    try {
        [System.IO.File]::WriteAllBytes($cerPath, $cert.Export('Cert'))
        $inner = "Import-Certificate -FilePath `"$cerPath`" -CertStoreLocation Cert:\LocalMachine\Root | Out-Null; " +
                 "Import-Certificate -FilePath `"$cerPath`" -CertStoreLocation Cert:\LocalMachine\TrustedPublisher | Out-Null; " +
                 "Remove-Item -LiteralPath `"$cerPath`" -Force -ErrorAction SilentlyContinue"
        Write-Host '正在请求管理员权限安装本机信任（请在 UAC 弹窗点“是”）...'
        $proc = Start-Process -FilePath 'powershell' -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-Command', $inner) -Verb RunAs -Wait -PassThru
        if ($proc.ExitCode -ne 0) { throw "本机信任安装失败（退出码 $($proc.ExitCode)）。" }
        Write-Host '已导入 Cert:\LocalMachine\Root 与 Cert:\LocalMachine\TrustedPublisher。'
    } finally {
        Remove-Item -LiteralPath $cerPath -Force -ErrorAction SilentlyContinue
    }
}

Invoke-Sign -Certificate $cert -Files $targets
Write-Host "`n完成。重新构建后请再次运行本脚本以保持签名。" -ForegroundColor Green
