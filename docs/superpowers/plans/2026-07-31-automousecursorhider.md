# AutoMouseCursorHider Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (\`- [ ]\`) syntax for tracking.

**Goal:** Build a Windows x64 Native AOT EXE that hides an idle cursor, reveals it on movement, and supports safe delay, startup, and stop controls.

**Architecture:** A pure state machine emits cursor actions. A small runtime applies them through an injected controller. A windowless host composes Windows P/Invoke, user-only registry configuration, named events, and a single-instance mutex.

**Tech Stack:** C# 12, .NET 8, net8.0-windows, P/Invoke, Native AOT (win-x64), package-free console tests.

## Global Constraints

- Support Windows 10/11 x64 only.
- Publish a self-contained Native AOT WinExe; end users need no .NET runtime.
- Do not network, elevate, inject, create services/tasks, or change execution policies.
- Use only HKCU\Software\AutoMouseCursorHider for delay settings and this app's value in HKCU\Software\Microsoft\Windows\CurrentVersion\Run.
- Sample GetCursorPos once per 100 ms and block between samples.
- Match every hide issued by this process with exactly one show before exit.
- Accept invariant-culture decimal delays from 0.1 through 3600 seconds.

## File Map

| File | Responsibility |
| --- | --- |
| .gitignore | Exclude build/publish output. |
| src/AutoMouseCursorHider/AutoMouseCursorHider.csproj | Windows Native AOT configuration. |
| src/AutoMouseCursorHider/CursorStateMachine.cs | Pure position/time transitions. |
| src/AutoMouseCursorHider/CommandLine.cs | Strict command parsing. |
| src/AutoMouseCursorHider/CursorRuntime.cs | Testable action application. |
| src/AutoMouseCursorHider/WindowsCursorController.cs | user32 cursor calls. |
| src/AutoMouseCursorHider/WindowsConfiguration.cs | HKCU settings/startup and named events. |
| src/AutoMouseCursorHider/Program.cs | Command handling and application lifetime. |
| tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj | Package-free test project. |
| tests/AutoMouseCursorHider.Tests/Program.cs | Test runner and all automated tests. |
| README.md | Build and usage instructions. |

---

### Task 1: Project and cursor state machine

**Files:**

- Modify: .gitignore
- Create: src/AutoMouseCursorHider/AutoMouseCursorHider.csproj
- Create: src/AutoMouseCursorHider/Program.cs
- Create: src/AutoMouseCursorHider/CursorStateMachine.cs
- Create: tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj
- Create: tests/AutoMouseCursorHider.Tests/Program.cs

**Interfaces:**

- Consumes: none.
- Produces: CursorPosition, CursorAction, and CursorStateMachine.Observe(CursorPosition, TimeSpan).

- [ ] **Step 1: Write the failing test and project metadata.**

Append these entries to .gitignore, preserving the existing .worktrees/ entry:

    bin/
    obj/
    publish/
    TestResults/

Create the app project with TargetFramework net8.0-windows, OutputType WinExe, nullable and implicit usings enabled, AllowUnsafeBlocks true, PublishAot true, InvariantGlobalization true, OptimizationPreference Size, and StripSymbols true. Create an executable test project targeting net8.0-windows that project-references the app project.

Create an empty `Program.Main` entry point so the app project can compile while the state-machine test is red.

In the test runner, add Run, Equal, True, and False assertion helpers and this test:

    var machine = new CursorStateMachine(TimeSpan.FromSeconds(3));
    var point = new CursorPosition(100, 200);
    Equal(CursorAction.None, machine.Observe(point, TimeSpan.Zero));
    Equal(CursorAction.None, machine.Observe(point, TimeSpan.FromSeconds(2.9)));
    Equal(CursorAction.Hide, machine.Observe(point, TimeSpan.FromSeconds(3)));
    Equal(CursorAction.None, machine.Observe(point, TimeSpan.FromSeconds(3.1)));
    Equal(CursorAction.Show, machine.Observe(new CursorPosition(101, 200), TimeSpan.FromSeconds(3.2)));

- [ ] **Step 2: Run the test to verify red.**

Run: dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj

Expected: CS0246 reports that CursorStateMachine, CursorPosition, and CursorAction are missing.

- [ ] **Step 3: Implement the minimal pure state machine.**

Create these types:

    public readonly record struct CursorPosition(int X, int Y);
    public enum CursorAction { None, Hide, Show }
    public sealed class CursorStateMachine
    {
        public CursorStateMachine(TimeSpan delay);
        public CursorAction Observe(CursorPosition position, TimeSpan now);
        public CursorAction Restore();
    }

Constructor accepts 100 ms through 1 hour only. The first observation stores position/time and returns None. Changed coordinates reset the timer and return Show only when already hidden. Unchanged coordinates at or past the delay return Hide once. Restore returns Show only when hidden.

- [ ] **Step 4: Run the test to verify green.**

Run: dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj

Expected: exit code 0 and one PASS line.

- [ ] **Step 5: Commit the tested foundation.**

    git add .gitignore src/AutoMouseCursorHider tests/AutoMouseCursorHider.Tests
    git commit -m "feat: add cursor state machine"

### Task 2: Parse commands before Windows actions

**Files:**

- Create: src/AutoMouseCursorHider/CommandLine.cs
- Modify: tests/AutoMouseCursorHider.Tests/Program.cs

**Interfaces:**

- Consumes: TimeSpan.
- Produces: AppCommandKind, AppCommand, and CommandLine.TryParse(string[] args, out AppCommand command, out string error).

- [ ] **Step 1: Write failing command-parser tests.**

Add tests asserting that --delay 2.5 produces Run with a 2500 ms delay; --startup --delay 5 produces Startup with a 5-second delay; and --delay with no value, --delay 0.09, --delay 3600.1, --stop --startup, and --stop --delay 2 all return false.

- [ ] **Step 2: Run the runner to verify red.**

Run: dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj

Expected: CS0103 reports missing parser/command types.

- [ ] **Step 3: Implement strict parsing.**

Create:

    public enum AppCommandKind { Run, Startup, RemoveStartup, Stop, Help }
    public sealed record AppCommand(AppCommandKind Kind, TimeSpan? Delay);
    public static bool TryParse(string[] args, out AppCommand command, out string error);

Recognize only --delay <seconds>, --startup, --no-startup, --stop, and --help. Use double.TryParse with NumberStyles.AllowDecimalPoint and CultureInfo.InvariantCulture. Permit --delay alone or with --startup; reject it with other controls. Reject duplicate/unknown switches, missing values, NaN/infinity, and out-of-range values with a nonempty error.

- [ ] **Step 4: Run the runner to verify green.**

Run: dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj

Expected: exit code 0 and four PASS lines.

- [ ] **Step 5: Commit parser behavior.**

    git add src/AutoMouseCursorHider/CommandLine.cs tests/AutoMouseCursorHider.Tests/Program.cs
    git commit -m "feat: parse cursor hider commands"

### Task 3: Apply state transitions through a runtime boundary

**Files:**

- Create: src/AutoMouseCursorHider/CursorRuntime.cs
- Create: src/AutoMouseCursorHider/WindowsCursorController.cs
- Modify: tests/AutoMouseCursorHider.Tests/Program.cs

**Interfaces:**

- Consumes: Task 1 state types.
- Produces: ICursorController and CursorRuntime with ProcessSample and Restore.

- [ ] **Step 1: Write a failing runtime test using a fake controller.**

Add FakeCursorController with HideCount/ShowCount. Feed the runtime an initial coordinate, the same coordinate after 1 second, the same coordinate again, then a moved coordinate. Assert HideCount is one and ShowCount is one even after Restore is called.

- [ ] **Step 2: Run the runner to verify red.**

Run: dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj

Expected: compilation fails because ICursorController and CursorRuntime do not exist.

- [ ] **Step 3: Implement the runtime and native controller.**

Create:

    public interface ICursorController
    {
        CursorPosition GetPosition();
        void Hide();
        void Show();
    }

    public sealed class CursorRuntime
    {
        public CursorRuntime(CursorStateMachine state, ICursorController cursor);
        public void ProcessSample(CursorPosition position, TimeSpan now);
        public void Restore();
    }

ProcessSample maps only Hide and Show actions to the controller. Restore applies its action only once.

In WindowsCursorController.cs, mark both WindowsCursorController and its nested NativeMethods class partial, then use source-generated P/Invoke declarations for user32.dll GetCursorPos and ShowCursor. GetPosition converts a private native POINT to CursorPosition and throws Win32Exception(Marshal.GetLastWin32Error()) on failure. Hide calls ShowCursor(false) once; Show calls ShowCursor(true) once.

- [ ] **Step 4: Verify tests and build.**

Run:

    dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj
    dotnet build src/AutoMouseCursorHider/AutoMouseCursorHider.csproj

Expected: five PASS lines; both commands exit 0.

- [ ] **Step 5: Commit runtime and interop.**

    git add src/AutoMouseCursorHider/CursorRuntime.cs src/AutoMouseCursorHider/WindowsCursorController.cs tests/AutoMouseCursorHider.Tests/Program.cs
    git commit -m "feat: drive cursor visibility from idle state"

### Task 4: Add safe current-user configuration and controls

**Files:**

- Create: src/AutoMouseCursorHider/WindowsConfiguration.cs
- Modify: tests/AutoMouseCursorHider.Tests/Program.cs

**Interfaces:**

- Consumes: Task 2 range rules.
- Produces: DelaySettingsStore, StartupManager, and InstanceSignals.

- [ ] **Step 1: Write a failing startup quoting test.**

Assert this exact behavior:

    StartupManager.BuildRunCommand(@"C:\Program Files\Cursor Tools\AutoMouseCursorHider.exe")
    == "\"C:\Program Files\Cursor Tools\AutoMouseCursorHider.exe\""

- [ ] **Step 2: Run the runner to verify red.**

Run: dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj

Expected: compilation fails because StartupManager is missing.

- [ ] **Step 3: Implement the user-only stores and signals.**

Create:

    public sealed class DelaySettingsStore
    {
        public TimeSpan ReadOrDefault();
        public void Write(TimeSpan delay);
    }
    public sealed class StartupManager
    {
        public void Install(string executablePath);
        public void Remove();
        public static string BuildRunCommand(string executablePath);
    }
    public sealed class InstanceSignals : IDisposable
    {
        public WaitHandle Stop { get; }
        public WaitHandle Reload { get; }
        public static void SignalStop();
        public static void SignalReload();
    }

Save a validated DelayMilliseconds DWORD under HKCU\Software\AutoMouseCursorHider and return 3000 ms when missing/invalid. Install/remove only the AutoMouseCursorHider value in the current-user Run key. BuildRunCommand must quote the executable and use standard Windows escaping for quotes/trailing backslashes. Create auto-reset Local\AutoMouseCursorHider.Stop.v1 and Local\AutoMouseCursorHider.Reload.v1 events. A missing event when signaling is a successful no-op.

- [ ] **Step 4: Verify configuration tests and build.**

Run:

    dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj
    dotnet build src/AutoMouseCursorHider/AutoMouseCursorHider.csproj

Expected: six PASS lines; both commands exit 0.

- [ ] **Step 5: Commit configuration primitives.**

    git add src/AutoMouseCursorHider/WindowsConfiguration.cs tests/AutoMouseCursorHider.Tests/Program.cs
    git commit -m "feat: add user startup and runtime controls"

### Task 5: Compose the single-instance windowless host

**Files:**

- Create: src/AutoMouseCursorHider/Program.cs
- Modify: tests/AutoMouseCursorHider.Tests/Program.cs

**Interfaces:**

- Consumes: all Task 1–4 interfaces.
- Produces: AutoMouseCursorHider.exe commands: default run, --delay, --startup, --no-startup, --stop, and --help.

- [ ] **Step 1: Write a failing delay-reset test.**

Create a runtime with 1-second delay, process a first sample at 0 seconds, call SetDelay(2 seconds), process at 1 second and assert no hide, then process at 3 seconds and assert one hide.

- [ ] **Step 2: Run the runner to verify red.**

Run: dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj

Expected: compilation fails because SetDelay does not yet exist.

- [ ] **Step 3: Correct SetDelay and implement Program.Main.**

First add CursorRuntime.SetDelay(TimeSpan delay), which replaces the state machine so its next sample establishes a fresh idle interval. Then implement this ordered flow:

    parse arguments
    → persist/signal an optional --delay
    → perform any control command and return
    → acquire Local\AutoMouseCursorHider.Instance.v1 mutex
    → sample / wait for Stop or Reload
    → restore cursor in finally

A second normal start returns success without a second loop. --delay writes settings, signals Reload, and starts a loop only if the mutex can be acquired. --stop only signals Stop and exits success. In the loop, sample once then WaitHandle.WaitAny(Stop, Reload, 100 ms); Reload reads settings and calls SetDelay. Enclose the created runtime and whole loop in try/finally. Display invalid input and caught exceptions with user32.dll MessageBoxW, returning 2 and 1; --help displays exact commands and returns 0.

- [ ] **Step 4: Verify the full Release build.**

Run:

    dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj
    dotnet build src/AutoMouseCursorHider/AutoMouseCursorHider.csproj -c Release

Expected: seven PASS lines and a successful Release build.

- [ ] **Step 5: Commit the host.**

    git add src/AutoMouseCursorHider/CursorRuntime.cs src/AutoMouseCursorHider/Program.cs tests/AutoMouseCursorHider.Tests/Program.cs
    git commit -m "feat: add windowless cursor hider host"

### Task 6: Publish, document, and verify

**Files:**

- Create: README.md

**Interfaces:**

- Consumes: Task 5 command interface.
- Produces: publish/AutoMouseCursorHider.exe and user-facing build/usage instructions.

- [ ] **Step 1: Write README commands and constraints.**

Include these commands:

    dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj
    dotnet publish src/AutoMouseCursorHider/AutoMouseCursorHider.csproj -c Release -r win-x64 -o publish
    .\publish\AutoMouseCursorHider.exe
    .\publish\AutoMouseCursorHider.exe --delay 5
    .\publish\AutoMouseCursorHider.exe --startup
    .\publish\AutoMouseCursorHider.exe --no-startup
    .\publish\AutoMouseCursorHider.exe --stop

State that delay defaults to 3 seconds, --delay reloads within the next 100 ms wait or starts an absent instance, --stop is the exit mechanism, and startup affects only current user. State build prerequisites: .NET 8 SDK plus Visual Studio Build Tools/Desktop development with C++.

- [ ] **Step 2: Run full automated verification.**

Run:

    dotnet run --project tests/AutoMouseCursorHider.Tests/AutoMouseCursorHider.Tests.csproj
    dotnet build src/AutoMouseCursorHider/AutoMouseCursorHider.csproj -c Release
    dotnet publish src/AutoMouseCursorHider/AutoMouseCursorHider.csproj -c Release -r win-x64 -o publish

Expected: all tests pass, both build commands exit 0, and publish/AutoMouseCursorHider.exe exists.

- [ ] **Step 3: Perform bounded Windows manual verification.**

Run the published EXE with --delay 1. After 1.2 seconds without movement verify hiding, then move the mouse and verify showing. While running, use --delay 2 and verify that it stays visible at 1 second but hides after 2 seconds. Use --stop and verify visibility. Use --startup and check the single named Run value, then use --no-startup and check it is absent.

- [ ] **Step 4: Commit documentation.**

    git add README.md
    git commit -m "docs: explain cursor hider usage"
