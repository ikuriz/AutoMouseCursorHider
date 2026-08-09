# AutoMouseCursorHider

轻量原生的 Windows 鼠标自动隐藏工具。

鼠标静止一段时间后自动隐藏，移动后立即恢复。程序只在系统托盘运行，无常驻主窗口。

## 功能

- 全局自动隐藏鼠标指针，移动即恢复
- 托盘菜单：设置、暂停/恢复、退出
- 可设置隐藏延迟（0.1–3600 秒）
- 支持开机启动
- 中英双语界面（自动跟随系统语言）
- 单文件、无安装、无需管理员权限
- 不联网、不收集数据
- 退出时自动恢复系统光标

## 使用方法

1. 从 [Releases](../../releases) 下载 `AutoMouseCursorHider.exe`
2. 双击运行
3. 在系统托盘找到图标，右键即可设置、暂停或退出

> 首次运行时 Windows SmartScreen 可能提示未知应用，这是未进行代码签名的常见情况，确认文件来源可靠后选择“仍要运行”即可。

程序为原生 Win32 x64 应用，不依赖 .NET 或 Visual C++ Runtime。

## 构建（开发者）

需要 Windows 10/11 x64，以及包含 MSVC、Windows SDK 和 CMake 的 Visual Studio Build Tools。

```powershell
cmake -S native -B native/build -G "Visual Studio 18 2026" -A x64
cmake --build native/build --config Release
```

输出文件：

```text
native/build/Release/AutoMouseCursorHider.exe
```

## 版本

- `v2.0.0`：原生 Win32 重构，加入托盘设置、双语界面、统一状态和应用图标
- `v1.0.0`：早期版本

## License

暂未指定开源许可证。
