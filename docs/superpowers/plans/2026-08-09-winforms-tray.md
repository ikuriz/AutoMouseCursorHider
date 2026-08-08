# WinForms 托盘界面 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 AutoMouseCursorHider 改造成带系统托盘和 WinForms 设置窗口的低资源 Windows 单文件应用。

**Architecture:** 保留现有 `CursorStateMachine` 和 Win32 鼠标控制器，将后台循环改为 `ApplicationContext` 驱动的单 UI 线程状态宿主。`NotifyIcon` 负责托盘交互，单实例互斥锁负责防止多个进程同时改变 ShowCursor 计数，设置窗口通过一个可复用的 `SettingsForm` 修改内存配置并持久化。

**Tech Stack:** .NET 8 Windows Desktop SDK、WinForms、C#、Win32 P/Invoke、现有 .NET 测试项目。

## Global Constraints

- 目标平台为 `net8.0-windows`、`win-x64`。
- 发布为 self-contained、single-file、`WinExe`，不需要用户安装 .NET Runtime。
- 默认不显示主窗口；运行状态通过托盘图标和菜单表达。
- 延迟合法范围为 `0.1–3600` 秒，支持小数；配置文件使用 invariant 格式保存。
- 不联网、不提权、不加入第三方运行时或遥测。
- 任意暂停、退出和异常路径都要恢复鼠标可见状态。

---

### Task 1: 将项目切换到 WinForms 自包含发布

**Files:**
- Modify: `src/AutoMouseCursorHider/AutoMouseCursorHider.csproj`
- Modify: `tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj`
- Modify: `README.md`

**Interfaces:**
- Produces a `net8.0-windows` WinForms-capable project with `UseWindowsForms=true` and no `PublishAot` requirement.

- [ ] **Step 1: Write the failing build check**

  Add a test/build check in the plan execution that imports `System.Windows.Forms.NotifyIcon` from the application project and asserts the project builds without AOT-only trimming errors.

- [ ] **Step 2: Run the check to verify it fails**

  Run `dotnet build src/AutoMouseCursorHider/AutoMouseCursorHider.csproj`; expected failure is the missing WinForms framework reference before the project property is added.

- [ ] **Step 3: Update project configuration**

  Set `<UseWindowsForms>true</UseWindowsForms>`, retain `OutputType=WinExe`, remove `<PublishAot>true</PublishAot>`, and add release properties for `SelfContained`, `PublishSingleFile`, `RuntimeIdentifier=win-x64`, and `PublishTrimmed=false`.

- [ ] **Step 4: Run build and publish checks**

  Run `dotnet build -c Release` and `dotnet publish -c Release -r win-x64 --self-contained true`; expected result is a successful WinForms build and a single executable in `publish`.

- [ ] **Step 5: Commit**

  `git add src/AutoMouseCursorHider/AutoMouseCursorHider.csproj tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj README.md && git commit -m "build: enable self-contained WinForms publishing"`

### Task 2: 抽离单实例与后台运行状态服务

**Files:**
- Create: `src/AutoMouseCursorHider/RuntimeState.cs`
- Create: `src/AutoMouseCursorHider/SingleInstance.cs`
- Modify: `src/AutoMouseCursorHider/CursorRuntime.cs`
- Modify: `tests/AutoMouseCursorHider.Tests/Program.cs`

**Interfaces:**
- `RuntimeState` exposes `IsPaused`, `Delay`, `StatusChanged`, `Pause()`, `Resume()`, and `SetDelay(TimeSpan)`.
- `SingleInstance.TryAcquire(string name, out IDisposable lease)` returns false without starting a second process.
- Existing `CursorRuntime` remains responsible for sampling, hiding, showing, and `Restore()`.

- [ ] **Step 1: Write failing state tests**

  Add tests asserting that `Pause()` makes the cursor visible and prevents hide transitions, `Resume()` starts a fresh idle interval, and `SetDelay()` resets the interval.

- [ ] **Step 2: Run focused tests**

  Run `dotnet run --project tests/AutoMouseCursorHider.Tests`; expected failures identify the new state API as absent.

- [ ] **Step 3: Implement the state service**

  Keep all state mutation on the UI thread; call `CursorRuntime.Restore()` on pause and before disposal. Do not add polling threads or global hooks.

- [ ] **Step 4: Run tests**

  Confirm all existing cursor tests and the new pause/resume/delay-reset tests pass.

- [ ] **Step 5: Commit**

  `git add src/AutoMouseCursorHider/RuntimeState.cs src/AutoMouseCursorHider/SingleInstance.cs src/AutoMouseCursorHider/CursorRuntime.cs tests/AutoMouseCursorHider.Tests/Program.cs && git commit -m "feat: add single-instance runtime state"`

### Task 3: 添加托盘 ApplicationContext 和菜单

**Files:**
- Create: `src/AutoMouseCursorHider/TrayApplicationContext.cs`
- Create: `src/AutoMouseCursorHider/TrayIconFactory.cs`
- Modify: `src/AutoMouseCursorHider/Program.cs`
- Modify: `tests/AutoMouseCursorHider.Tests/Program.cs`

**Interfaces:**
- `TrayApplicationContext : ApplicationContext` owns `NotifyIcon`, `System.Windows.Forms.Timer`, `RuntimeState`, and settings store.
- Menu items are exactly `打开设置`, dynamic `暂停/恢复`, and `退出`.
- `OpenSettingsRequested`, `PauseRequested`, `ExitRequested` are exercised through internal methods in tests rather than UI automation.

- [ ] **Step 1: Write failing menu/state tests**

  Add pure tests for menu labels, dynamic pause/resume text, and disposal restoring the cursor and removing the icon.

- [ ] **Step 2: Run focused tests**

  Run the test executable and verify failures before implementation.

- [ ] **Step 3: Implement the context**

  Create the icon on the UI thread, set `Visible=true`, route left-click/double-click to settings, and use a 100 ms WinForms timer to sample `GetPosition()` and call `ProcessSample()`.

- [ ] **Step 4: Update Program.Main**

  Keep `--help`, `--delay`, `--startup`, `--no-startup`, and `--stop` compatibility. The default `Run` path acquires the mutex and calls `Application.Run(new TrayApplicationContext(...))`; command paths exit before creating the UI.

- [ ] **Step 5: Run tests and manual smoke check**

  Run tests, then launch the Release EXE and verify a tray icon appears within three seconds, no console window appears, and `退出` removes the icon.

- [ ] **Step 6: Commit**

  `git add src/AutoMouseCursorHider/TrayApplicationContext.cs src/AutoMouseCursorHider/TrayIconFactory.cs src/AutoMouseCursorHider/Program.cs tests/AutoMouseCursorHider.Tests/Program.cs && git commit -m "feat: add tray application context"`

### Task 4: 实现 WinForms 设置窗口

**Files:**
- Create: `src/AutoMouseCursorHider/SettingsForm.cs`
- Create: `src/AutoMouseCursorHider/SettingsForm.Designer.cs`
- Modify: `src/AutoMouseCursorHider/TrayApplicationContext.cs`
- Modify: `src/AutoMouseCursorHider/WindowsConfiguration.cs`
- Modify: `tests/AutoMouseCursorHider.Tests/Program.cs`

**Interfaces:**
- `SettingsForm` receives an initial `double delaySeconds`, `bool startupEnabled`, and callbacks `Func<SettingsDraft, bool> ApplySettings`.
- `SettingsDraft` contains `double DelaySeconds` and `bool StartupEnabled`.
- `WindowsConfiguration` adds `TryReadDelay(out double)` and startup read/write helpers without changing invariant persistence.

- [ ] **Step 1: Write failing validation tests**

  Test accepted values `0.1`, `2.5`, and `3600`; reject `0`, negatives, non-numeric text, `NaN`, infinity, and values above `3600`. Test applying a new value resets the runtime interval.

- [ ] **Step 2: Run focused tests**

  Run the test executable and verify validation tests fail before the form/parser exists.

- [ ] **Step 3: Implement controls and validation**

  Add a labeled `NumericUpDown` with decimal places, increment, minimum, and maximum; allow direct text entry; show a small error label instead of a modal error for invalid input. Wire `应用` and `取消`, and hide on close instead of disposing the application.

- [ ] **Step 4: Implement startup checkbox**

  Use the existing current-user Run-key manager with the absolute quoted executable path. Display registry errors inline and leave the previous valid setting active.

- [ ] **Step 5: Run tests and manual UI check**

  Verify decimal entry, spinner increments, apply/cancel behavior, pause status text, and reopening the same window.

- [ ] **Step 6: Commit**

  `git add src/AutoMouseCursorHider/SettingsForm.cs src/AutoMouseCursorHider/SettingsForm.Designer.cs src/AutoMouseCursorHider/TrayApplicationContext.cs src/AutoMouseCursorHider/WindowsConfiguration.cs tests/AutoMouseCursorHider.Tests/Program.cs && git commit -m "feat: add WinForms settings window"`

### Task 5: 完成退出恢复、错误提示和单文件发布

**Files:**
- Modify: `src/AutoMouseCursorHider/TrayApplicationContext.cs`
- Modify: `src/AutoMouseCursorHider/Program.cs`
- Modify: `README.md`
- Modify: `.gitignore` if publish artifacts need exclusion

**Interfaces:**
- `TrayApplicationContext.Dispose(bool)` is idempotent and always calls runtime restore, timer stop, icon disposal, and mutex release.
- `Program.Report` remains the only path for command-line errors and uses a message box because the app has no console.

- [ ] **Step 1: Write failure-path tests**

  Add tests that calling dispose twice is safe, pause/exit restores visibility, and a second instance does not create another tray context.

- [ ] **Step 2: Run focused tests**

  Confirm the new failure-path tests initially fail where cleanup is incomplete.

- [ ] **Step 3: Implement deterministic cleanup**

  Wrap timer, notify icon, settings form, runtime, and mutex lease in deterministic disposal; guard cleanup with a private `_disposed` flag.

- [ ] **Step 4: Update documentation**

  Document tray usage, settings range, pause/resume, exit behavior, command-line compatibility, and the exact publish path.

- [ ] **Step 5: Verify release artifact**

  Run `dotnet test`, `dotnet publish -c Release -r win-x64 --self-contained true`, inspect that only the intended EXE is delivered, and launch it manually for the acceptance checklist.

- [ ] **Step 6: Commit**

  `git add src/AutoMouseCursorHider/TrayApplicationContext.cs src/AutoMouseCursorHider/Program.cs README.md .gitignore && git commit -m "feat: finalize tray cleanup and release packaging"`

## Self-review checklist

- Spec coverage: tray-only startup and menu are covered in Task 3; WinForms numeric input and decimal validation in Task 4; startup, security, single instance, and cleanup in Tasks 2, 4, and 5; self-contained release in Tasks 1 and 5.
- Placeholder scan: all tasks name concrete files, commands, interfaces, validation values, and expected outcomes; no TBD/TODO steps remain.
- Type consistency: `RuntimeState`, `SingleInstance`, `TrayApplicationContext`, `SettingsForm`, and `SettingsDraft` are introduced before their consumers and use the signatures listed above.
