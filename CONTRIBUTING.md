# 贡献指南

感谢你关注并参与 GeoDark 的开发！在提交代码或创建合并请求（Merge Request）之前，请阅读以下指南以确保代码风格与质量标准的一致性。

---

## 1. 开发环境要求

- **操作系统**：Windows 11 x64（或配备最新更新的 Windows 10 x64）
- **C++ 编译器**：Microsoft Visual Studio 2022（MSVC v143+，含“使用 C++ 的桌面开发”工作负载）
- **Windows SDK**：包含 C++/WinRT 投影头文件的 Windows 10/11 SDK
- **构建工具**：CMake 3.21 或更高版本
- **可选工具**：Go 1.22+（用于构建与维护应用图标生成工具 `tools/icon/main.go`）

---

## 2. 本地构建与验证流水线

在提交修改之前，请务必在本地完成全套编译与自动化测试验证：

```powershell
# 1. 配置 CMake 生成目录
cmake -S . -B build -A x64

# 2. 编译 Release 目标
cmake --build build --config Release

# 3. 运行自动化算法测试集
ctest --test-dir build -C Release --output-on-failure
```

### 守护进程性能验证
任何对核心层（`src/core.cpp`、`src/solar.cpp`、`src/location.cpp` 等）的修改，均需使用性能测试脚本确认未引入额外的 CPU 唤醒或内存泄漏：

```powershell
powershell -File tools\measure.ps1 -Minutes 1
```
守护进程私有工作集内存均值应维持在 5MB 以内。

---

## 3. 代码规范与架构约束

- **C++ 标准**：遵循现代 C++17 规范，全面使用 RAII 机制管理 Win32 句柄与 COM 智能指针（如 `wil`、`Microsoft::WRL::ComPtr`）。
- **编码与字符集**：源文件必须以 UTF-8 编码保存，CMake 已启用 `/utf-8` 编译参数。
- **单文件便携分发原则**：
  - `GeoDarkUI.exe` 必须保持绿色便携单文件分发形态。所有 HTML、CSS 与 JS 资源内嵌于头文件中，WebView2 必须静态链接 `WebView2LoaderStatic.lib`，严禁要求分发独立的外部 DLL 或资源文件夹。
- **后台守护进程极限纯净原则**：
  - `GeoDark.exe` 严禁引入 WebView2 或任何重量级 UI 依赖；
  - 严禁引入持续自旋循环（Spin-loop / Polling），所有唤醒动作必须基于系统事件或单次定时器驱动。
- **文档完整性**：
  - 增改用户可见行为、注册表键值或界面交互时，请同步更新 `README.md`、`CHANGELOG.md` 及相关架构文档。

---

## 4. 提交规范

- 提交信息（Commit Message）推荐使用语义化格式：
  - `feat: ...`（新增功能）
  - `fix: ...`（缺陷修复）
  - `docs: ...`（文档增补）
  - `refactor: ...`（重构优化，无行为变更）
- 提交前请仔细检查 `git status`，切勿将 `build/`、`dist/` 或临时文件意外提交到仓库。
