# AutoMouseCursorHider 🖱️

**轻量、原生的 Windows 鼠标自动隐藏工具：鼠标静止时自动隐藏，移动后立即恢复。**

无需安装、无需 .NET、无需管理员权限。下载一个小型 EXE，双击即可运行，程序安静地驻留在系统托盘中。

[English README](README.md) · [最新版本](../../releases/latest)

![AutoMouseCursorHider 演示](docs/images/cursor-demo.gif)

## 为什么选择 AutoMouseCursorHider？

适合阅读、写作、演示、观看视频以及各种不希望鼠标停在画面上的场景。程序低调运行，并在鼠标移动或程序退出时安全恢复光标。

## 功能

- 全局自动隐藏鼠标指针，移动后立即恢复
- 可调节隐藏延迟：**0.1–3600 秒**（默认 **3 秒**）
- 系统托盘菜单：设置、暂停/恢复、退出
- 保护任务管理器、高权限窗口、UAC 和安全桌面中的光标
- 支持开机启动
- 支持 English / 简体中文界面
- 单实例、便携式、单文件 EXE
- 不联网、不收集数据、无需管理员权限
- 暂停或退出时自动恢复系统光标

## 截图

### 设置窗口

![设置窗口](docs/images/settings.png)

### 托盘菜单

![托盘菜单](docs/images/tray-menu.png)

## 下载和运行

1. 从 Releases 下载最新的 **[AutoMouseCursorHider.exe](../../releases/latest)**。
2. 双击运行，无需安装。
3. 在系统通知区域找到程序图标，右键即可设置、暂停、恢复或退出。

由于程序尚未进行商业代码签名，Windows SmartScreen 首次运行时可能显示安全提示。如果文件来自本项目，请选择“仍要运行”。

## 系统要求

- Windows 10 或 Windows 11
- 64 位 x64 系统
- 不需要 .NET 或 Visual C++ Runtime
- 不需要管理员权限

## 技术说明

程序采用 Windows 原生 Win32 API 构建。这属于实现细节，不影响正常使用；在受支持的 Windows 10/11 x64 系统上可直接运行，也不需要额外安装运行时。

## 构建（开发者）

安装包含 MSVC、Windows SDK 和 CMake 的 Visual Studio Build Tools，然后运行：

```powershell
cmake -S native -B native/build -G "Visual Studio 18 2026" -A x64
cmake --build native/build --config Release
```

生成的程序位于 `native/build/Release/AutoMouseCursorHider.exe`。

## 版本历史

- **v2.4.0** — 修复任务管理器及其他高权限窗口中的光标恢复问题。
- **v2.3.0** — UAC 和安全桌面期间保持光标可见。
- **v2.2.0** — 设置窗口新增“退出”按钮。
- **v2.1.2** — 修复资源管理器/任务栏图标兼容性并更新应用图标。
- **v2.1.1** — 更新应用和托盘图标，并适当放大鼠标主体。
- **v2.1.0** — 加入启动反馈、静默启动、托盘设置、双语界面和统一状态模型。
- **v2.0.0** — 原生 Win32 重构版。
- **v1.0.0** — 早期版本。

## License

暂未指定开源许可证。
