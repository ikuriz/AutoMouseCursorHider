# AutoMouseCursorHider 设计说明

## 目标

提供一个面向 Windows 10/11 x64 的轻量级单文件可执行程序：鼠标静止达到设定时间后自动隐藏，鼠标一移动立即显示。程序支持静默运行、当前用户开机自启与安全退出。

## 非目标

- 不联网、不收集数据、不注入或监控其他进程。
- 不修改执行策略、系统级注册表、服务或计划任务。
- 不支持多显示器之外的特殊光标策略；系统光标行为以 Windows API 为准。

## 交付物与技术栈

- `src/AutoMouseCursorHider/`：C# 源码与项目文件。
- 目标框架：.NET 8，发布方式：Windows x64 Native AOT 单文件 `WinExe`。
- 运行时不依赖 .NET 安装；构建需要 .NET 8 SDK 与 Native AOT Windows 工具链。
- `tests/`：不依赖网络包的状态机测试项目。
- `README.md`：构建、发布和命令行使用说明。

## 运行行为

1. 普通启动默认无窗口、以已保存的延迟运行（首次为 3 秒）。使用同目录的 `AutoMouseCursorHider.exe --stop` 可退出。
2. 主循环每 100 ms 通过 `GetCursorPos` 读取屏幕坐标，其余时间阻塞等待，避免忙等。
3. 坐标变化时立即显示光标并重置静止计时；连续静止时间达到延迟时，仅调用一次隐藏操作。
4. 每次由本程序隐藏的光标都会在移动、停止命令、异常处理或进程正常退出路径中恢复一次。
5. 使用命名互斥体确保仅有一个运行实例；第二个普通启动不创建第二个隐藏循环。

## 命令行接口

| 命令 | 行为 |
| --- | --- |
| 无参数 | 无窗口启动，使用已保存的延迟（首次为 3 秒）。 |
| `--delay <秒数>` | 将延迟设为 0.1 至 3600 秒；若已有实例运行则立即重载，否则启动新实例。非法值返回非零退出码。 |
| `--startup` | 在 `HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run` 写入当前 EXE 的绝对路径。 |
| `--no-startup` | 删除本程序的上述当前用户启动项。 |
| `--stop` | 向运行实例发送命名停止事件；没有运行实例时成功退出。 |
| `--help` | 显示用法并退出。 |

控制类选项（`--startup`、`--no-startup`、`--stop`、`--help`）不启动隐藏循环。`--delay` 是唯一可与启动共用的设置选项。未知参数、冲突控制选项和缺失的 `--delay` 值都返回非零退出码。

## 安全与自启动

- 自启动只影响当前 Windows 用户，绝不请求 UAC 提权。
- 启动项值由程序根据 `Environment.ProcessPath` 构造，并对路径进行 Windows 命令行引号转义，避免空格或引号造成命令注入。
- 删除启动项只删除本程序拥有的固定值名；不会触碰其他启动项。
- 延迟值仅保存在 `HKCU\\Software\\AutoMouseCursorHider` 的 `DelayMilliseconds` 中；`--delay` 通过命名事件请求运行实例重载该值。
- 程序始终无窗口运行，但不隐藏进程行为；用户始终可用同目录的 `AutoMouseCursorHider.exe --stop` 退出。

## 结构

- `Program`：解析参数、执行控制命令、建立单实例与应用生命周期。
- `CursorStateMachine`：纯逻辑，根据时间和坐标产生 `Hide`、`Show` 或无动作；不直接调用 Win32 API。
- `CursorController`：封装 `GetCursorPos` 与与本程序配对的光标显示/隐藏调用。
- `StartupManager`：限定在当前用户 Run 键范围内的读写与删除。
- `SettingsStore`：在当前用户专属注册表键中验证、读取与保存延迟值。
- `InstanceSignals`：提供命名停止事件与设置重载事件，供 `--stop` 和 `--delay` 安全控制运行实例。

## 错误处理

- 注册表不可写时，`--startup` 和 `--no-startup` 显示错误并返回非零退出码。
- 无法读取鼠标坐标时，程序显示错误、确保恢复自己隐藏的光标，然后退出。
- 未处理异常由最外层捕获；退出前调用一次恢复逻辑。

## 验证

- 状态机测试覆盖：静止期、达到阈值隐藏、移动立即显示、重复轮询不重复发出操作、不同延迟、边界延迟与非法参数。
- 构建验证：`dotnet test`、`dotnet publish`（`win-x64` Native AOT）均应成功。
- 人工 Windows 验证：无窗口启动与 `--stop`、运行中 `--delay` 重载、`--startup`/`--no-startup`、光标移动恢复。
