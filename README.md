# AutoMouseCursorHider

轻量的 Windows 鼠标隐藏工具：鼠标静止达到设定时长后自动隐藏，移动后立即显示。

## 使用方式

双击 `publish\AutoMouseCursorHider.exe` 后，程序默认不显示主窗口，只在系统托盘运行。

右键托盘图标可以：

- 打开设置；
- 暂停或恢复隐藏；
- 退出程序。

设置窗口使用数字输入框和上下调节按钮，支持小数，范围为 `0.1–3600` 秒。也可以在设置窗口启用当前用户开机自启。

命令行兼容操作：

```powershell
.\publish\AutoMouseCursorHider.exe --delay 5
.\publish\AutoMouseCursorHider.exe --startup
.\publish\AutoMouseCursorHider.exe --no-startup
.\publish\AutoMouseCursorHider.exe --stop
.\publish\AutoMouseCursorHider.exe --help
```

程序只修改当前用户配置和当前用户启动项，不联网、不提权、不收集数据。单实例运行，暂停、退出和异常路径都会尝试恢复鼠标显示。

## 构建与测试

需要 .NET 8 SDK。Windows 发布使用 WinForms 自包含单文件配置：

```powershell
$env:DOTNET_CLI_HOME = "$PWD\.dotnet-home"
dotnet run --project tests\AutoMouseCursorHider.Tests\AutoMouseCursorHider.Tests.csproj -p:RestoreIgnoreFailedSources=true
dotnet publish src\AutoMouseCursorHider\AutoMouseCursorHider.csproj -c Release -r win-x64 --self-contained true -p:RestoreIgnoreFailedSources=true -o publish
```

发布完成后，应用位于 `publish\AutoMouseCursorHider.exe`，可直接复制到未安装 .NET Runtime 的 Windows x64 电脑上运行。
