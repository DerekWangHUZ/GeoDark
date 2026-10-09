# 变更记录

本项目遵循语义化版本规范。所有重要更新与问题修复均记录于此。

## [0.3.1] - 2026-10-09

### 修复

- **彻底排查并修复“开机自启弹窗”与“GeoDarkUI 白屏”**：
  - 根因一（已确证）：仓库目录带有 **`Low Mandatory Level`（低完整性）NTFS 标签**（目录 ACL 中含非本机 SID 的显式 ACE，系整目录连同 ACL 从其他机器复制/恢复时带入，标签经 (OI)(CI) 继承到全部新建文件）——Windows 将低完整性内容一律按 Internet 区处理：附件管理器弹“打开文件 - 安全警告”，WebView2 加载器在该路径下无法创建浏览器进程（UI 必现白屏）。因果验证：给空目录打同样标签 → 同一 exe 立即 zone=3；标签提回 Medium → 立即恢复 zone=0 且 WebView2 正常。签名、机器信任、`Unblock-File`、杀软排除项/信任组/暂停保护均无效（标记在 NTFS SACL 上，不在文件内容或 `Zone.Identifier` 里）。卡巴斯基 2026-09-30 的 `VHO:Trojan.Win32.Khalesi.gen` 行为误报（`detects.db` 可查）为低完整性上下文引发的次生噪声，非根因。修复：`icacls <仓库目录> /setintegritylevel (OI)(CI)M`（详见 docs/TROUBLESHOOTING.md 第 9 条）。
  - 根因二：`ui.cpp` 将 WebView2 初始化失败完全静默，任何初始化错误（含用户数据目录损坏）都表现为白屏。
  - 修复措施：
    - 新增 `tools\install.ps1`：安装到 `%LOCALAPPDATA%\Programs\GeoDark`、解除锁定、校验签名、把开机自启（HKCU Run）指向安装目录并启动守护进程；
    - `ui.cpp`：初始化各环节失败时写入 `%LOCALAPPDATA%\GeoDark\ui.log`；首次失败自动重置 WebView2 用户数据目录（改名保留现场）并重试一次；仍失败弹出带错误码的说明框——不再静默白屏；窗口销毁时调用 `controller_->Close()`，避免遗留 msedgewebview2 孤儿进程；
    - 新增 `tools\diag-zone.ps1`：一键检测任意路径被判定为何种安全区域；
    - 新增 `tools\run.ps1`：构建后一键镜像到干净路径并启动，开发测试不受路径判定影响（支持 `-Build` / `-Daemon` / `-Verify`）；
    - `docs/TROUBLESHOOTING.md` 新增第 9 节“白屏”，并在第 7 节补充路径级判定说明。

### 变更

- 版本号 0.3.0 → 0.3.1（CMake / 资源 / 清单同步）。

## [0.3.0] - 2026-10-09（已撤回，由 0.3.1 取代）

> 注：0.3.0 发行版已撤回。其全部变更（签名工具与版本号）已包含在 0.3.1 中，并附带上述弹窗/白屏修复。

### 新增

- **开发工具 `tools/sign.ps1` 一键代码签名脚本**：自动创建自建代码签名证书（`CN=GeoDark (Derek Wang)`，5 年有效期）、导入用户级与本机信任区（`-InstallMachineTrust`，需 UAC），并用 Windows SDK signtool 签名 `build\Release` 与（可选）`dist\` 全部 exe；支持 `-Unblock` 清除仓库内 MotW 标记。
- **故障排查文档新增第 7 条**：“启动时弹出‘打开文件 - 安全警告’（无法验证发布者）”，说明 MotW + 未签名机制，并给出签名脚本、手动解除锁定与组策略三种处理方式。

### 文档

- `README.md` 本地构建章节新增“代码签名”小节，提醒重新构建后需重新签名。

## [0.2.0] - 2026-10-01

### 新增

- **全新现代化 UI 交互系统**：
  - 像素级复用现代桌面 UI 设计规范，引入无边框磨砂圆角卡片、macOS / Fluent 风格红黄绿窗口控制与环境状态药丸（`.env-pill`）。
  - 集成 Cascadia Mono 等宽实时控制台，实时滚动输出系统状态、地理定位变动与诊断日志。
  - 参数配置面板全面改用圆角卡片分区、iOS 风格切换开关（Switch）与日照偏移双列微调器。
  - 采用无阻塞浮层 Toast 与操作即时校验反馈。
  - 界面自动平滑响应 Windows 系统级深色/浅色配色切换。
- **专属天体应用图标**：
  - 设计并生成融合金阳、弦月、地球经纬椭圆与罗盘定位点的专属 Logo，兼顾“Geo”地理与“Dark”光暗寓意。
  - 新增 Go 算法抗锯齿图标生成工具 `tools/icon/main.go`，输出包含 16px 至 256px 完整层级的 Windows `.ico` 与 512x512 PNG 图标。
  - 配置 Windows 资源脚本 `resources/geodark.rc`，使 `GeoDark.exe` 与 `GeoDarkUI.exe` 在资源管理器、任务栏、Alt+Tab 与窗口标题中原生呈现专属图标。
- **Per-Monitor V2 高 DPI 锐利显示**：
  - 新增 `resources/geodark.manifest` 应用程序清单，声明 `PerMonitorV2, PerMonitor` 感知级别。
  - 在 `ui.cpp` 入口注入 `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`。
  - 实现 `WM_DPICHANGED` 跨屏拖拽与缩放事件响应，动态根据物理 DPI（如 125%、150%、200%）自适应缩放视口与窗口布局。
  - 注入 CSS 亚像素字体抗锯齿与 SVG 矢量精度优化规则，彻底根除 DWM 双线性插值位图拉伸模糊。

### 优化与修复

- **绿色单文件分发优化**：在 CMake 中配置静态链接官方 `WebView2LoaderStatic.lib`，所有前端 HTML/CSS 资源直接内嵌，输出不依赖额外动态库的独立便携式可执行文件。
- **架构功能保真**：严格隔离 UI 层与核心层，后台守护进程 `GeoDark.exe`、注册表键值规范、IPC 命名事件与 NOAA 算法 100% 原样保留。

### 文档

- 全面更新项目根目录 `README.md`，增加高分辨率截图、徽标排版、架构流程图与功能总览。
- 新增系统架构文档 `docs/ARCHITECTURE.md`、故障排查手册 `docs/TROUBLESHOOTING.md`、安全策略 `SECURITY.md` 与贡献指南 `CONTRIBUTING.md`。

---

## [0.1.0] - 2026-09-24

### 初始发布

- **原生 C++ / Win32 后台静默守护进程 (`GeoDark.exe`)**：
  - 基于隐藏顶层窗口接收系统时间调整与电源广播，采用单次定时器进行零自旋唤醒。
  - 私有内存占用小于 5MB，无常驻托盘图标。
- **高精度本地太阳周期计算**：
  - 内置 NOAA 太阳公式，纯本地离线计算天文日出、日落与晨昏蒙影时间。
  - 完善的极昼、极夜、夏令时与跨国际日期变更线测试用例。
- **Windows 定位服务集成**：
  - 通过 C++/WinRT 异步调用 Windows 原生 `Geolocator`，支持自动定位与离线/历史坐标容灾降级。
- **用户行为意图保护（手动覆盖）**：
  - 记录用户临时在系统设置中手动更改主题的行为，锁定该状态直至下一次日落或日出时刻。
- **按需配置工具 (`GeoDarkUI.exe`)**：
  - 提供开机自启、定位模式、经纬度输入与日照偏移调整。
- **开箱即用构建与测试套件**：
  - 提供 CMake 构建规则、自动化 CTest 算法单测与资源测量脚本 `tools/measure.ps1`。
