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
                1. 写入设置    |  2. 触发命名事件 (SetEvent)
                               v
      +-------------------------------------------------+
      | 注册表存储: HKCU\Software\GeoDark               |
      | 命名事件: Local\GeoDarkSettingsChangedEvent     |
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
- **通知信号**：配置程序修改设置并保存至注册表后，调用 Win32 `SetEvent` 激活命名事件 `Local\GeoDarkSettingsChangedEvent`。
- **守护进程响应**：守护进程在后台线程或通过 `MsgWaitForMultipleObjectsEx` 捕获该事件，立刻重新载入注册表参数并重新计算下一次太阳交界时刻。

### 2. 注册表数据结构 (`HKEY_CURRENT_USER\Software\GeoDark`)

| 键名 (Value) | 类型 | 说明 | 默认值 |
| :--- | :--- | :--- | :--- |
| `AutoMode` | `REG_DWORD` | 1 为自动模式，0 为关闭自动切换 | `1` |
| `UseWindowsLocation` | `REG_DWORD` | 1 为使用系统定位，0 为使用手动经纬度 | `1` |
| `ManualLatitude` | `REG_SZ` | 手动填写的纬度字符串 (例如 `"31.230000"`) | `""` |
| `ManualLongitude` | `REG_SZ` | 手动填写的经度字符串 (例如 `"121.470000"`) | `""` |
| `SunriseOffsetMinutes` | `REG_DWORD` | 日出切换偏移分钟（正数推迟，负数提前） | `0` |
| `SunsetOffsetMinutes` | `REG_DWORD` | 日落切换偏移分钟（正数推迟，负数提前） | `0` |
| `RunOnLogon` | `REG_DWORD` | 是否在 Windows 登录时自动启动 | `1` |
| `LastValidLatitude` | `REG_SZ` | 最近一次获取成功的有效纬度缓存 | `""` |
| `LastValidLongitude` | `REG_SZ` | 最近一次获取成功的有效经度缓存 | `""` |
| `ManualOverrideUntil` | `REG_QWORD` | 手动覆盖有效截止时间（UTC Unix 时间戳） | `0` |
| `ManualOverrideLight` | `REG_DWORD` | 手动覆盖锁定的主题（1 浅色，0 深色） | `0` |

---

## 三、核心算法与策略模型

### 1. NOAA 太阳计算模块 (`src/solar.cpp`)
GeoDark 实现了美国国家海洋和大气管理局（NOAA）太阳位置近似公式，纯本地运行：
1. **儒略日与儒略世纪数**：根据系统 UTC 秒时间戳计算天文学 Julian Century ($T$)。
2. **太阳几何平黄经与平近点角**：
   $$L_0 = 280.46646 + 36000.76983 \cdot T$$
   $$M = 357.52911 + 35999.05029 \cdot T$$
3. **中心差与太阳真黄经**：校正地球公转轨道偏心率对视位置的影响。
4. **太阳赤纬 ($\delta$) 与均时差 ($EoT$)**：精确计算太阳照射地表的倾角与由于黄赤交角和椭圆轨道引起的时间差。
5. **地平纬圈时角 ($H_0$)**：
   $$\cos(H_0) = \frac{\cos(90.833^\circ) - \sin(\phi)\sin(\delta)}{\cos(\phi)\cos(\delta)}$$
   其中标准天顶角设为 $90.833^\circ$（包含太阳视半径 $16'$ 与大气折射折算 $34'$）。
6. **极昼与极夜特判**：
   - 当 $\cos(H_0) > 1$ 时为极夜，全天维持深色主题。
   - 当 $\cos(H_0) < -1$ 时为极昼，全天维持浅色主题。

### 2. 决策与意图保护策略 (`src/policy.cpp`)
- **自动切换**：在日照周期计算出的当前状态与系统当前主题不一致时，执行平滑切换。
- **手动覆盖持久化 (Manual Override)**：
  - 若用户在非切换时刻手动修改了 Windows 系统设置中的深浅色主题，GeoDark 会捕获到主题不一致。
  - 策略引擎不会强行将主题回滚，而是记录当前手动意图，并将其锁定持续到**下一个日落或日出时刻**。
  - 到达下一个天然交界点后，手动覆盖自动失效，无缝恢复天文自动跟随。

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
        ├─► 读取内嵌 resources/geodark.manifest (PerMonitorV2 声明)
        │
        ├─► wWinMain 调用 SetProcessDpiAwarenessContext(PER_MONITOR_AWARE_V2)
        │
        ├─► GetDpiForSystem() / GetDpiForWindow() 动态获取实际物理 DPI (如 144 DPI @ 150%)
        │
        ├─► 窗口尺寸缩放: MulDiv(940, dpi, 96), MulDiv(660, dpi, 96)
        │
        ├─► WebView2 接收设备像素比 devicePixelRatio = 1.5，进行原生亚像素光栅化
        │
        └─► 响应 WM_DPICHANGED 消息: 跨屏拖拽时无缝重设窗口 Bounds 并重绘视口
```

- **CSS 渲染级优化**：在 `src/ui_html.h` 中应用了 `-webkit-font-smoothing: antialiased`、`-moz-osx-font-smoothing: grayscale`、`text-rendering: optimizeLegibility` 与 `shape-rendering: geometricPrecision`，确保在高分辨率屏幕上文字笔画与矢量 SVG 均呈现最高锐度。
