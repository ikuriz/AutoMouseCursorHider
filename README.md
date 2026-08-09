# AutoMouseCursorHider

轻量、原生的 Windows 鼠标自动隐藏工具。

鼠标静止达到设定时间后自动隐藏，移动鼠标立即恢复显示。程序默认只在系统托盘运行，不会打开常驻主窗口。

## 功能

- 全局自动隐藏鼠标指针，移动后立即恢复
- 托盘菜单：设置、暂停/恢复、退出
- 设置隐藏延迟（0.1–3600 秒）
- 支持开机启动
- 托盘和设置窗口支持 English / 简体中文
- 自动识别简体中文系统；繁体中文系统暂时回退 English
- 配置保存在当前 Windows 用户目录
- 无网络连接、无数据收集、无需管理员权限
- 单实例运行，退出时会恢复系统光标

## 直接运行

从 [Releases](https://github.com/ikuriz/AutoMouseCursorHider/releases) 下载 `AutoMouseCursorHider.exe`，双击即可运行。

程序是原生 Win32 x64 应用，不需要安装 .NET 或 Visual C++ Runtime。首次运行时 Windows SmartScreen 可能显示安全提示，这是未进行商业代码签名的常见提示。

启动后请在系统托盘找到 AutoMouseCursorHider 图标，右键即可打开设置、暂停/恢复或退出。

## 构建

开发环境：

- Windows 10/11 x64
- Visual Studio Build Tools（含 MSVC、Windows SDK、CMake）

构建命令：

```powershell
cmake -S native -B native/build -G "Visual Studio 18 2026" -A x64
cmake --build native/build --config Release
```

生成文件：

```text
native/build/Release/AutoMouseCursorHider.exe
```

## 版本

- `v1.0.0`：早期版本
- `v2.0.0`：原生 Win32 重构版，加入托盘设置、双语界面、统一状态模型和应用图标

## License

暂未指定开源许可证。
