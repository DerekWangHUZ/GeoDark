# 变更记录

本项目遵循语义化版本规范。所有重要更新与问题修复均记录于此。

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
