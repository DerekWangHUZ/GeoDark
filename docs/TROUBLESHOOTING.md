# 常见问题与故障排查

本文档汇总了在使用 GeoDark 过程中可能遇到的常见疑问、定位授权异常、高 DPI 适配及第三方应用兼容性问题。

---

## 1. 定位服务不可用或显示“未授权”

### 现象
- 在 `GeoDarkUI` 状态栏中，定位状态显示为“定位服务未启用”或“位置未知”。
- 日志面板输出 `Geolocation access denied` 或获取超时。

### 排查与解决方法
1. **检查 Windows 系统定位总开关**：
   - 打开 Windows **“设置” -> “隐私和安全性” -> “位置”**。
   - 确保 **“定位服务”** 处于开启状态。
   - 确保 **“允许桌面应用访问你的位置”** 处于开启状态。
2. **在 GeoDarkUI 中触发授权弹窗**：
   - 在 GeoDarkUI 状态卡片中点击 **“请求定位权限”** 按钮。
   - 在 Windows 弹出的授权对话框中点击 **“允许”**。
3. **备选方案：切换为手动经纬度**：
   - 若设备无 GPS/Wi-Fi 定位硬件，或处于完全离线环境，可以在 GeoDarkUI 的“设置”页面中将定位方式改为 **“手动坐标”**。
   - 输入所在城市的经纬度（例如北京为 `39.90, 116.40`，上海为 `31.23, 121.47`），点击保存即可离线精准计算太阳时间。

---

## 2. 窗口文字或图标显示边缘发虚、模糊

### 现象
- 在 125%、150% 或 200% 等高缩放比例的屏幕上，窗口内容文字有插值拉伸感。

### 排查与解决方法
1. **确保使用最新 v0.2.0 或更高版本**：
   - 从 v0.2.0 起，程序已内嵌 `PerMonitorV2` 高 DPI 清单，并注入进程级 DPI 感知。
2. **检查文件属性中的外部兼容性覆盖**：
   - 右键点击 `GeoDarkUI.exe` -> 选择 **“属性”** -> 切换至 **“兼容性”** 选项卡。
   - 点击 **“更改高 DPI 设置”**。
   - **务必确保取消勾选“替代高 DPI 缩放行为”**。若启用了外部覆盖，系统将强制改写应用的感知声明，导致降级为位图模糊缩放。
3. **完全退出旧进程后再启动**：
   - 若刚刚更新了 EXE，请确保任务管理器中先前的旧进程已完全关闭。

---

## 3. 后台没有自动切换深浅色主题

### 排查与解决方法
1. **检查守护进程是否在运行**：
   - 按 `Ctrl + Shift + Esc` 打开任务管理器，查看“进程”或“详细信息”列表中是否存在 `GeoDark.exe`。
   - 若未运行，可在 GeoDark 目录下直接双击 `GeoDark.exe`，或在 GeoDarkUI 中勾选“登录 Windows 后自动运行”并点击保存。
2. **是否处于“用户手动覆盖锁定期”**：
   - 若您刚刚在 Windows 系统设置中手动切换了主题，GeoDark 的**意图保护机制**会自动锁定该状态，直到下一次日出或日落时刻。
   - 如需立即解除手动锁定并强制按太阳时间同步，只需在 `GeoDarkUI` 界面点击一次“切换主题”按钮或重新点击“保存设置”。
3. **检查自动模式开关**：
   - 确认在“参数设置”面板中，“自动模式”切换开关处于打开状态。

---

## 4. 移动程序目录后开机自启失效

### 原因
开机自启项在当前用户注册表中记录了 `GeoDark.exe` 的绝对文件路径。如果移动或重命名了程序所在的文件夹，Windows 登录时将无法找到目标文件。

### 解决方法
- 将文件夹放置在固定目录（如 `C:\Program Files\GeoDark` 或常用的工具目录）。
- 启动 `GeoDarkUI.exe`，重新点击一次 **“保存设置”**，程序会自动将更新后的完整路径写入注册表。

---

## 5. 部分第三方软件未跟随系统切换主题

### 原因说明
Windows 11 在主题切换时会向全局广播 `WM_SETTINGCHANGE` 消息。
- **现代化应用**（如 Windows 资源管理器、设置、Edge、Chrome、VS Code、Windows Terminal 等）会即时响应消息并热重载主题。
- **部分较老旧的第三方 Win32 应用**（如早期版本的第三方办公软件或自研客户端）只在进程初始化时读取一次系统配色注册表，无法动态响应广播。

### 解决方法
- 这类老旧软件在重启该软件窗口后即会自动应用当前系统深色/浅色配色。

---

## 6. 提示缺少 Microsoft Edge WebView2 运行时

### 现象
双击 `GeoDarkUI.exe` 时无反应或弹出 WebView2 初始化失败提示。

### 解决方法
- **Windows 11**：系统已原生默认预装 WebView2 运行时。若被精简版系统移除，可前往微软官方下载安装。
- **Windows 10 / Windows Server**：若未安装过新版 Microsoft Edge，请前往微软官网下载安装 **Microsoft Edge WebView2 Evergreen 独立安装程序**。
- 注：后台守护程序 `GeoDark.exe` 仅使用 Win32 原生 API，**完全不依赖** WebView2 运行时。

## 7. 启动时弹出“打开文件 - 安全警告”（无法验证发布者）

### 现象
- 双击或开机自启 `GeoDark.exe` / `GeoDarkUI.exe` 时，弹出“打开文件 - 安全警告”对话框，提示“无法验证发布者。你确定要运行此软件吗？”。

### 原因
Windows 附件管理器（Attachment Manager）对同时满足以下两个条件的程序弹出该警告：
1. 文件带有 **Mark of the Web（MotW，`Zone.Identifier` 数据流）**——凡是从网络下载、网盘同步、浏览器解压或跨机复制得到的文件都会带上；
2. 文件**未经数字签名**，或签名证书不受本机信任。

GeoDark 官方发行包当前未做商业代码签名，因此带 MotW 的副本每次启动都会触发提示。

### 解决方法
任选其一：

- **方法 A（本机开发者，推荐）**：使用仓库自带的签名脚本，为本机构建产物签发自建证书并安装本机信任：
  ```powershell
  # 首次运行（在仓库根目录）：签名 + 安装用户级信任，本机信任需在 UAC 弹窗点“是”
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\sign.ps1 -InstallMachineTrust
  # 之后每次重新构建后重新签名（无需再次提权）：
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\sign.ps1
  ```
  脚本会自动创建 `CN=GeoDark (Derek Wang)` 代码签名证书（5 年有效期，存于当前用户证书区），导入本机 `Root` 与 `TrustedPublisher` 信任区，并用 Windows SDK 的 signtool 签名 `build\Release` 下的全部 exe。加 `-Dist` 可一并签名 `dist\` 发行副本，加 `-Unblock` 可清除仓库内残留的 MotW 标记。
  注意：证书仅对当前 Windows 账户与本机生效；其他用户或其他机器仍会看到提示（属正常安全行为）。

- **方法 B（单文件临时解除）**：右键 exe → **属性** → 勾选 **“解除锁定”** → 确定；或用 PowerShell：
  ```powershell
  Unblock-File -LiteralPath "C:\path\to\GeoDark.exe"
  ```
  缺点：重新下载、解压或构建覆盖后 MotW 会再次出现，弹窗复发。

- **方法 C（组策略，不推荐普通用户）**：将 `.exe` 加入 `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Associations` 的 `LowRiskFileTypes`。这会全局放行所有 exe 的该检查，安全面过大，仅在完全理解后果时使用。

> **补充（0.3.1 实测）**：若签名、机器信任、解除锁定全部做完**仍然弹窗**，多半是目录带着 `Low Mandatory Level`（低完整性）NTFS 标签——Windows 把低完整性内容按 Internet 区处理；标签在 NTFS SACL 上而不在 `Zone.Identifier` 里，所以 `Unblock-File` 无效。用 [第 9 条](#9-geodarkui-启动后窗口一片空白白屏) 的 `tools\diag-zone.ps1` 验证，并用 `icacls <目录> /setintegritylevel (OI)(CI)M` 修复。

---

## 8. 性能与内存诊断

GeoDark 设计目标为极限静默与超低开销。若需检验当前后台进程的实际性能指标，可在项目目录中运行内置测试脚本：

```powershell
powershell -File tools\measure.ps1 -Minutes 1
```

**参考正常运行指标**：
- `CpuPercentOfOneCore`: `< 0.001%`（几乎不可测）
- `PrivateWorkingSetMeanMB`: `~2.5 MB` 至 `4.5 MB`
- `ThreadContextSwitchesObserved`: 极低（仅在系统睡眠、唤醒或网络切换时有零星唤醒）

---

## 9. GeoDarkUI 启动后窗口一片空白（白屏）

### 现象
- `GeoDarkUI.exe` 能弹出窗口（标题“GeoDark 设置”、可拖动），但整个窗口纯白，无任何界面内容。
- 旧版本（< 0.3.1）此时不报任何错误；0.3.1 起会在失败时弹窗提示错误码，并在 `%LOCALAPPDATA%\GeoDark\ui.log` 留下日志。

### 原因
白屏 = WebView2 环境创建失败，而 0.3.1 之前的版本把失败静默吞掉了。实测有两类触发条件：

1. **仓库目录带着 `Low Mandatory Level`（低完整性）NTFS 标签，Windows 把低完整性内容一律按 Internet 区处理**（本机 2026-10 的最终定论）。
   - 表现：`...\source\repos\GeoDark\` 下所有文件（含 README.md、新建的任意文件）被 `MapUrlToZone` 判为 Internet 区 → 附件管理器弹“打开文件 - 安全警告”；WebView2 加载器在该路径下无法创建浏览器进程 → UI 必现白屏。
   - 定位方法：`icacls <目录>` 中可见 `Mandatory Label\Low Mandatory Level:(OI)(CI)(NW)`；因果验证：给空目录打上同样标签，同一份 exe 立刻变 zone=3；标签提回 Medium，立刻恢复 zone=0 且 WebView2 正常。
   - 来源：目录 ACL 中存在**非本机 SID** 的显式 ACE（`S-1-5-21-2394...` 等），说明整个目录连同 ACL 是从别的机器整目录复制/恢复过来的，低完整性标签随之带入，并被 (OI)(CI) 继承到所有新建文件。签名、机器信任、`Unblock-File`、杀软排除项/信任组/暂停保护全都无效——因为标记不在文件内容或 `Zone.Identifier` 上，而在 NTFS 完整性标签（SACL）上。
   - 次生现象：卡巴斯基曾于 2026-09-30 对该目录下编译的 exe 给出行为误报 `VHO:Trojan.Win32.Khalesi.gen`（低完整性上下文中运行的未签名程序易被行为引擎盯上，记录见 `%ProgramData%\Kaspersky Lab\AVP21.26\Data\detects.db`）。它是本问题的次生噪声而非原因，是否处理均不影响修复。
2. **WebView2 用户数据目录（`%LOCALAPPDATA%\GeoDark\WebView2`）损坏**：浏览器进程启动即退。0.3.1 起会自动改名保留现场并重试一次。

### 解决方法
1. **给仓库目录打回正常完整性级别（根治，本机已验证）**：
   ```cmd
   icacls "C:\Users\DerekWang\source\repos\GeoDark" /setintegritylevel (OI)(CI)M
   ```
   子目录与文件会自动继承更新。完成后用 `tools\diag-zone.ps1` 复查 `zone=0`，弹窗与白屏同时消失，可直接在 `build\Release` 下构建、运行、测试。
2. **升级到 0.3.1+**：初始化失败会明确弹窗并写日志，不再静默白屏；用户数据目录损坏会自动重置重试。
3. **手动核验路径判定**：
   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File tools\diag-zone.ps1 "C:\path\to\GeoDarkUI.exe"
   ```
   输出 `zone=3` 即被判定为 Internet 区；`zone=0` 为正常本机区域。若修复后仍为 `zone=3`，用 `icacls <目录>` 检查 `Mandatory Label\Low` 是否残留或被外部再次写回。
4. **开发测试备用通道 `tools\run.ps1`**：构建后镜像到 `%TEMP%\GeoDarkDevRun` 并从那里启动（镜像副本不受任何路径判定影响）：
   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run.ps1 -Build     # 构建 + 镜像 + 启动
   powershell -NoProfile -ExecutionPolicy Bypass -File tools\run.ps1 -Verify    # 复查路径判定状态
   ```
5. **正式分发**：构建后运行 `tools\sign.ps1`（签名）与 `tools\install.ps1`（安装到用户程序目录并接管自启动）。

### 关联
- 该低完整性标签同样是第 7 条中“签名 + 机器信任 + 解除锁定后**仍然**弹窗”的原因：附件管理器把低完整性路径下的文件按 Internet 区对待。
