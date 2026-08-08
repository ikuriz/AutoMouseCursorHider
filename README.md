# AutoMouseCursorHider

轻量级 Windows 鼠标光标隐藏器：鼠标静止一段时间后自动隐藏，移动后立即显示。

## 使用

发布后的 `AutoMouseCursorHider.exe` 默认无窗口运行，首次使用的静止时间为 3 秒。

```powershell
.\publish\AutoMouseCursorHider.exe
.\publish\AutoMouseCursorHider.exe --delay 5
.\publish\AutoMouseCursorHider.exe --stop
```

- `--delay <秒数>`：将延迟设为 0.1–3600 秒。运行中的实例会在下一次最多 100 ms 的等待结束后重载；没有运行实例时会直接启动一个。
- `--stop`：安全停止运行中的实例，并恢复本程序隐藏的光标。
- `--help`：显示命令说明。

## 开机自启

```powershell
.\publish\AutoMouseCursorHider.exe --startup
.\publish\AutoMouseCursorHider.exe --no-startup
```

`--startup` 只在当前用户的注册表启动项中登记本 EXE，无需管理员权限；`--no-startup` 只删除本程序自己的启动项。移动或重命名 EXE 后，请重新运行 `--startup`。

如果想同时设置启动延迟，可运行：

```powershell
.\publish\AutoMouseCursorHider.exe --startup --delay 5
```

## 安全与资源占用

- 不联网、不收集数据、不申请管理员权限。
- 不创建服务、计划任务或后台注入；只使用 Windows 光标 API、当前用户注册表和命名事件。
- 单实例运行，每 100 ms 读取一次鼠标位置，其余时间由等待句柄阻塞，避免忙等。
- 程序在鼠标移动、`--stop` 或异常退出路径中都会尝试恢复自己隐藏的光标。

## 构建

构建机需要 .NET 8 SDK，以及 Visual Studio Build Tools（含 **Desktop development with C++** 工作负载）。

```powershell
dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj
dotnet publish src/AutoMouseCursorHider/AutoMouseCursorHider.csproj -c Release -r win-x64 -o publish
```

发布完成后，可将 `publish\AutoMouseCursorHider.exe` 单独复制到任意固定目录使用。
