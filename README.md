# GeoDark

GeoDark 是面向 Windows 11 x64 的轻量自动深浅色切换工具。它按 Windows 定位和本地太阳计算结果切换应用与系统主题。

## 构建

需要 MSVC C++ 构建工具、Windows SDK（含 C++/WinRT 投影头文件）和 CMake。使用 x64 Developer PowerShell：

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

构建产物位于 `build\Release\GeoDark.exe` 和 `build\Release\GeoDarkUI.exe`。把两个文件放在同一个可写或只读目录即可运行；设置和状态保存在当前用户注册表中。

## 使用

1. 启动 `GeoDarkUI.exe`，选择 Windows 自动定位或输入经纬度。
2. 设置日出、日落偏移。正数推迟切换，负数提前。
3. 按“保存设置”。首次自动定位会启动同一后台 EXE 的临时授权窗口，点击“请求定位权限”。
4. 按需勾选“登录 Windows 后自动运行”。启动项只作用于当前用户，无需管理员权限。

关闭设置窗口不影响后台。后台进程不显示托盘图标，不运行 Windows 服务，也不访问第三方定位或太阳时间 API。太阳时间完全本地计算；Windows 定位服务本身可能使用网络。定位不可用时使用最近一次有效位置；首次没有位置时保持现有主题，并在 UI 中输入手动坐标。手动修改主题会保留到下一次日出或日落。

## 设计与限制

- `GeoDark.exe` 使用隐藏顶层窗口接收时间与电源广播，并用单次定时器等待下一次事件；定位在启动、唤醒、网络变化及每 6 小时进行一次。
- 自动定位采用 Windows `Geolocator` 默认精度，未持续订阅位置，网络未变化时的新位置最多可能延迟约 6 小时发现。
- 默认只管理当前用户的应用与系统深浅色值。部分已打开的第三方程序可能只在自身重启后读取主题设置。
- NOAA 太阳公式得到的是近似时间，高纬度和特殊大气条件下误差可能增大。
- 目标空闲私有工作集小于 5 MB，但应以目标 Windows 11 设备上的实际测量为准。

后台运行后，可按需使用 `powershell -File tools\measure.ps1` 做 1 分钟的资源采样；使用 `-Minutes` 可调整时长。`ThreadContextSwitchesObserved` 是线程上下文切换计数，只作为唤醒活动的近似指标。
