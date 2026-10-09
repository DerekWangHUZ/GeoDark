# 安全策略与隐私模型

## 1. 报告安全漏洞

如果您在 GeoDark 中发现了安全缺陷或潜在隐患，请通过 GitLab 提交**保密 Issue（Confidential Issue）**，或直接联系项目维护者，切勿在公开 Issue 或讨论区披露细节。

在报告中请尽可能详细地说明：
- 受影响的程序版本（例如 v0.3.1）
- 具体的触发条件与复现步骤
- 潜在的安全影响与利用场景
- 建议的修复方案（若有）

维护团队将在核实后尽快完成修复并发布安全更新。

---

## 2. 权限与安全边界模型

GeoDark 在设计上始终遵循**最小特权（Least Privilege）**与**零云端依赖**原则：

- **普通用户权限运行（`asInvoker`）**：
  无论是后台守护进程 `GeoDark.exe` 还是配置程序 `GeoDarkUI.exe`，应用程序清单（Manifest）均声明 `level="asInvoker"`。程序运行、保存设置以及注册开机自启均不需要、也不会请求 Windows 管理员权限（UAC 提权）。
- **注册表范围隔离**：
  程序的所有读写操作被严格限制在当前用户的注册表目录 `HKEY_CURRENT_USER\Software\GeoDark` 及启动项目录 `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`，不会触碰全局机器项 `HKEY_LOCAL_MACHINE`。
- **IPC 通信安全**：
  配置程序与守护进程之间的同步采用带有命名空间前缀的 Win32 事件对象（`Local\GeoDarkSettingsChangedEvent`），仅限于当前用户登录会话内有效，阻止跨会话越权劫持。

---

## 3. 用户隐私与数据安全

- **绝对本地化（Zero-Telemetry）**：
  GeoDark 不包含任何遥测、埋点、统计代码，亦不架设任何数据收集服务端。
- **地理位置隐私保护**：
  当开启 Windows 自动定位时，程序通过官方 C++/WinRT API 仅向系统请求基础经纬度坐标。获取到的坐标仅在本地执行天文几何公式以计算日出与日落时间。
  任何坐标数据均**绝不**上传至外部网络，亦不会向第三方分析服务共享。
- **免凭据免网络依赖**：
  计算日照时间纯粹依赖本地算法（NOAA 太阳位置公式），无需注册任何云端天气 API Key，应用内不存储、不管理任何网络账号与密码凭据。
