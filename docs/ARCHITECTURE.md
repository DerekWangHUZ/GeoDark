# 系统架构设计说明

本文档详细介绍 GeoDark 的系统架构、模块职责、进程间协作通信、算法模型与显示子系统设计。

---

## 一、双进程分工架构

GeoDark 采用**守护进程与配置工具解耦**的轻量化双进程模型：

```
+-------------------------------------------------------------+
|                  配置程序 (GeoDarkUI.exe)                   |
|  - Win32 无边框磨砂阴影宿主 (DwmExtendFrameIntoClientArea)  |
|  - 嵌入式 Microsoft Edge WebView2 运行时                    |
|  - 卡片化现代桌面交互系统 / 实时 Cascadia Mono 诊断控制台   |
|  - 按需启动，配置完成后可立即完全退出                       |
+-------------------------------------------------------------+
                               |
         1. 写入 Config / OverrideCommand 指令
               2. 触发命名事件 (SetEvent)
                               v
      +-------------------------------------------------+
      | 注册表存储: HKCU\Software\GeoDark               |
      |   子键: Config / State / OverrideCommand        |
      | 命名事件: Local\GeoDarkRefreshLocation          |
      |           Local\GeoDarkManualOverride          |
      +-------------------------------------------------+
                               ^
                               | 监听与读取配置
+-------------------------------------------------------------+
|                  后台守护进程 (GeoDark.exe)                 |
|  - 无界面、零托盘图标的 Win32 原生后台进程                  |
|  - 监听系统电源 (WM_POWERBROADCAST) 与时钟变更广播          |
|  - Windows C++/WinRT Geolocator 定位引擎                    |
|  - NOAA 太阳周期离线计算与极昼/极夜状态机                   |
|  - 基于单次高精度定时器唤醒，非轮询自旋                     |
|  - 调用 Windows 私有 API 切换应用与系统深浅色主题           |
+-------------------------------------------------------------+
```

### 1. 守护进程 (`GeoDark.exe`)
- **生命周期**：登录 Windows 后由 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 启动（可选），常驻后台静默运行。
- **资源占用**：私有工作集通常在 2MB ~ 5MB 之间，无定时自旋轮询（Tick Loop），完全由系统事件与单次定时器（One-shot Timer）驱动唤醒。
- **系统广播监听**：创建隐藏顶层窗口，监听 `WM_TIMECHANGE`、`WM_TIMEZONECHANGE`、`WM_POWERBROADCAST`（睡眠与恢复）及网络可用性变化通知。

### 2. 配置程序 (`GeoDarkUI.exe`)
- **生命周期**：按需启动，独立进程。提供直观的可视化状态大屏、诊断日志与参数配置面板。
- **无残留**：窗口关闭即终止，不会在系统后台残留多余的 WebView2 渲染进程。

---

## 二、进程间通信 (IPC) 与注册表设计

### 1. IPC 机制
GeoDark 使用**两个具名 Win32 事件**（均为 `Local\` 命名空间，仅限当前登录会话）加**注册表通知**完成双向通信：

- **`Local\GeoDarkRefreshLocation`**（自动复位事件）：配置程序请求守护进程立即重新定位；守护进程收到后执行定位流程。
- **`Local\GeoDarkManualOverride`**（自动复位事件）：配置程序在 `...\GeoDark\OverrideCommand` 写入一条序列号指令（手动锁定主题意图）后触发；守护进程唤醒并合并该指令。
- **注册表通知（`RegNotifyChangeKeyValue`）**：守护进程对 `Config` 与系统主题键（`Themes\Personalize`）各挂一个通知事件，键值变化时重读配置/检测外部主题变更；采用「先重挂通知、再读值」的顺序避免一次性语义丢事件。
- **状态所有权**：`State` 子键由守护进程**独占写入**；配置程序只读。UI 的手动主题切换通过 `OverrideCommand` 指令（序列号防重放）交由守护进程合并，避免两进程并发整包写状态造成丢更新。

### 2. 注册表数据结构 (`HKEY_CURRENT_USER\Software\GeoDark`)

#### 子键 `Config`（配置程序写，守护进程读）

| 键名 (Value) | 类型 | 说明 | 默认值 |
| :--- | :--- | :--- | :--- |
| `AutoEnabled` | `REG_DWORD` | 1 为自动模式，0 为关闭自动切换 | `1` |
| `LocationMode` | `REG_DWORD` | 0 为 Windows 定位，1 为手动经纬度 | `0` |
| `HasManualCoordinates` | `REG_DWORD` | 手动坐标是否有效 | `0` |
| `ManualLatitude` | `REG_SZ` | 手动填写的纬度字符串 (例如 `"31.23000000"`) | `"0"` |
| `ManualLongitude` | `REG_SZ` | 手动填写的经度字符串 (例如 `"121.47000000"`) | `"0"` |
| `SunriseOffsetMinutes` | `REG_DWORD` | 日出切换偏移分钟（正数推迟，负数提前，写入时钳制 -120~120） | `0` |
| `SunsetOffsetMinutes` | `REG_DWORD` | 日落切换偏移分钟（同上） | `0` |

#### 子键 `State`（守护进程独占写，配置程序只读）

| 键名 (Value) | 类型 | 说明 |
| :--- | :--- | :--- |
| `HasLocation` / `Latitude` / `Longitude` / `AccuracyMeters` | `REG_DWORD` / `REG_SZ` | 最近一次有效定位（坐标为 8 位小数字符串，读回时做范围校验） |
| `LocationUtc` / `LastAttemptUtc` | `REG_SZ` | 定位成功/尝试时间（Unix 秒，字符串存储） |
| `ActiveSource` | `REG_DWORD` | 0 无 / 1 Windows / 2 缓存 / 3 手动 |
| `LocationError` / `ThemeError` | `REG_SZ` | 最近一次定位/主题错误描述（供 UI 展示） |
| `ExpectedLight` / `NextSwitchUtc` / `NextSunriseUtc` / `NextSunsetUtc` / `Polar` | `REG_DWORD` / `REG_SZ` | 当前日照相位与下一次交界时刻 |
| `OverrideActive` / `OverridePhaseLight` / `OverrideUntilUtc` | `REG_DWORD` / `REG_SZ` | 手动覆盖意图（生效中 / 锁定相位 / 截止 UTC 秒） |

#### 子键 `OverrideCommand`（配置程序写，守护进程消费）

| 键名 (Value) | 类型 | 说明 |
| :--- | :--- | :--- |
| `Sequence` | `REG_SZ` | 单调递增指令序号（守护进程只处理比已见序号更大的指令） |
| `Lock` | `REG_DWORD` | 1 = 锁定当前日照相位为手动意图，0 = 解除覆盖 |
| `UntilUtc` | `REG_SZ` | 覆盖截止时刻（Unix 秒）；过期时由守护进程按实时日照重算 |

开机自启不使用注册表 `GeoDark` 子键内的开关，而是标准的 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 启动项（值为安装目录 `GeoDark.exe` 的带引号路径）。

---

## 三、核心算法与策略模型

### 1. 太阳位置计算模块 (`src/solar.cpp`)
GeoDark 实现了美国国家海洋和大气管理局（NOAA）太阳位置近似公式，纯本地运行：
1. **儒略日与儒略世纪数**：根据 UTC 秒时间戳计算天文学 Julian Century ($T$)。
2. **太阳几何平黄经与平近点角**：
   $$L_0 = 280.46646 + 36000.76983 \cdot T$$
   $$M = 357.52911 + 35999.05029 \cdot T$$
3. **中心差与太阳真黄经**：校正地球公转轨道偏心率对视位置的影响，并叠加黄经章动与真黄经修正。
4. **太阳赤纬 ($\delta$) 与均时差 ($EoT$)**：精确计算太阳照射地表的倾角与由于黄赤交角和椭圆轨道引起的时间差。
5. **太阳高度角**：给定 UTC 时刻与经纬度，直接计算
   $$\sin(h) = \sin(\phi)\sin(\delta) + \cos(\phi)\cos(\delta)\cos(H)$$
   其中时角 $H$ 由「UTC 分钟 + 均时差 + 4 × 经度」导出，**不依赖任何本地日历/时区/日期线假设**。

**求交界的实现方式**：`calculate_solar_schedule` 以 5 分钟步长对太阳高度角相对晨昏线（$h = -0.833^\circ$，含视半径 16' 与大气折射 34'）的差值采样，检测到符号翻转后二分逼近到 1 秒精度得到日出/日落时刻；偏移分钟直接加在交界时刻上。该「连续函数采样 + 求根」方法天然规避了极地、时区边界与国际日期变更线附近的日历歧义：

- **极昼/极夜**：观测窗口（前 3 天至后 5 天）内无任何穿越即 `polar = true`，当前相位由高度角直接判定，全天维持浅色/深色主题。
- **夏令时**：仅是本地显示转换，UTC 交界时刻不受影响。
- **回放正确性**：错过交界（如休眠跨过日落）后唤醒，`light_now` 由「当前时刻之前最后一个事件」回放得出，结果与持续运行一致。

### 2. 决策与意图保护策略 (`src/policy.cpp`)
- **自动切换**：在日照周期计算出的当前状态与系统当前主题不一致时，执行平滑切换。
- **外部变更检测**：守护进程在每次决策前读取真实主题值；`theme_diverges` 纯函数判定「可读的当前主题 ≠ 日照预期」（读不到的值 -1 永不算分歧，避免误触发）。
- **首次决策意图保护**：守护进程本次会话尚未应用过主题时（开机/重启后第一次决策），若系统主题与日照预期不一致，视为用户在守护进程离线期间的选择，播种手动覆盖而非强行翻转——与运行中检测到的外部变更行为一致。
- **手动覆盖持久化 (Manual Override)**：
  - 若用户在非切换时刻手动修改了 Windows 系统设置中的深浅色主题，GeoDark 会捕获到主题不一致。
  - 策略引擎不会强行将主题回滚，而是记录当前手动意图，并将其锁定持续到**下一个日落或日出时刻**（`until_utc` 每次决策顺延至最新交界点）。
  - 到达下一个天然交界点后，手动覆盖自动失效，无缝恢复天文自动跟随。
  - 覆盖基线随日照相位失效自动清空（例如白天锁了深色，进入夜间后覆盖解除）。

---

## 四、UI 架构与高 DPI 渲染子系统

### 1. 宿主与视图层架构
- **窗口骨架**：`src/ui.cpp` 创建 Win32 无边框原生窗口，样式为 `WS_POPUP | WS_THICKFRAME`，结合 `DwmExtendFrameIntoClientArea` 保留 Windows 11 原生贴靠与窗口阴影特效。
- **通信桥梁**：
  - 前端向 C++ 发送 JSON：`window.chrome.webview.postMessage(jsonString)`
  - C++ 响应并推送到前端：`ICoreWebView2::PostWebMessageAsJson`
- **零依赖便携编译**：通过静态链接 `WebView2LoaderStatic.lib`，程序免去随身携带 `WebView2Loader.dll` 的痛点。单文件绿色运行。

### 2. Per-Monitor V2 高 DPI 处理流程

```
[可执行文件启动]
        │
        ├─► 读取内嵌 resources/geodark.manifest (PerMonitorV2 声明，由 CMake 模板生成)
        │
        ├─► wWinMain 调用 SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)
        │
        ├─► GetDpiForSystem() / GetDpiForWindow() 静态获取实际物理 DPI (如 144 DPI @ 150%)
        │
        ├─► 窗口尺寸缩放: MulDiv(940, dpi, 96), MulDiv(660, dpi, 96)
        │
        ├─► WebView2 接收设备像素比 devicePixelRatio = 1.5，进行原生亚像素光栅化
        │
        └─► 响应 WM_DPICHANGED 消息: 跨屏拖拽时无缝重设窗口 Bounds 并重绘视口
```

- **CSS 渲染级优化**：在 `src/ui_html.h` 中应用了 `-webkit-font-smoothing: antialiased`、`-moz-osx-font-smoothing: grayscale`、`text-rendering: optimizeLegibility` 与 `shape-rendering: geometricPrecision`，确保在高分辨率屏幕上文字笔画与矢量 SVG 均呈现最高锐度。

---

## 五、测试与持续集成

| 套件 | 覆盖范围 | 说明 |
| :--- | :--- | :--- |
| `geodark_solar_tests`（CTest: solar） | 日照算法 + 策略引擎 | 上海昼夜、异地对时、DST、跨国际日期变更线、极昼极夜、偏移对称性、手动覆盖决策、`theme_diverges` 分歧判定 |
| `geodark_bridge_tests`（CTest: bridge） | WebView2 桥协议 | `bridge_json.h` 的字符串/整数/布尔提取、JSON 转义与 `\uXXXX`（含 UTF-16 代理对）解码、非法输入容错 |

- 本地一键验证：`powershell -File tools\ci.ps1`（配置 + 构建 + 全部测试）。
- CI：仓库内置 CI 配置在推送时执行 build/test 两阶段（需要带 MSVC 的 Windows Runner）。
- 版本号唯一来源是 `CMakeLists.txt` 的 `project(GeoDark VERSION ...)`；`resources/*.in` 模板经 `configure_file` 生成 rc 与 manifest，无需手工多处同步。
