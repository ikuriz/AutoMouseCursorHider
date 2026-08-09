# Launch Feedback Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show Settings for manual launches, keep Windows startup silent, and focus the existing Settings window when the user launches a second copy.

**Architecture:** Keep the existing single-instance mutex and message-only dispatcher window. A failed second-instance acquisition finds the existing dispatcher by its registered window class and posts a private `WM_APP` message. The existing process centralizes Settings opening in one handler. The Startup registry command line receives `--startup`; an ordinary launch has no argument and posts a Settings request after initialization.

**Tech Stack:** Native C++17, Win32 message-only window, registry Run key, CMake/MSVC static CRT.

## Global Constraints

- Manual double-click opens Settings.
- Windows startup uses `--startup` and remains silent.
- A second manual launch never creates a second process, tray icon, or mouse hook.
- Startup arguments only control Settings visibility; they do not change auto-hide configuration.
- Unknown arguments are treated as silent startup.
- No administrator privileges, .NET runtime, or VC++ runtime may be introduced.
- Existing cursor-state and config-store tests must continue to pass.

---

### Task 1: Add startup-source and dispatcher-message primitives

**Files:**
- Modify: `native/src/startup_registration.cpp`
- Modify: `native/src/main.cpp`

**Interfaces:**
- `StartupRegistration::Enable` writes `"<exe-path>" --startup` to the current-user Run key.
- `kShowSettingsMessage` is a private `WM_APP` message handled by the dispatcher window.

- [ ] **Step 1: Write the failing test/check**

Add a code-level verification target to the manual checklist: inspect the Run value after enabling startup and confirm it contains the executable path followed by `--startup`. Existing `ConfigStoreTests` remains the automated regression check because registry integration has no test harness yet.

- [ ] **Step 2: Implement the startup command line**

Change `StartupRegistration::Enable` from:

```cpp
const auto quoted = L"\"" + executablePath + L"\"";
```

to:

```cpp
const auto quoted = L"\"" + executablePath + L"\" --startup";
```

Keep the existing `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` value name and disable behavior unchanged.

- [ ] **Step 3: Add launch-mode parsing**

Add a small helper in `main.cpp`:

```cpp
bool IsInteractiveLaunch(PCWSTR commandLine)
{
    return commandLine == nullptr || *commandLine == L'\0';
}
```

An empty command line is a manual double-click. `--startup` and every unknown argument are silent launches, so external callers cannot accidentally open Settings.

- [ ] **Step 4: Build the existing targets**

Run:

```powershell
cmake --build native/build --config Release
```

Expected: the native executable and both existing test executables build successfully.

- [ ] **Step 5: Commit**

```powershell
git add native/src/startup_registration.cpp native/src/main.cpp
git commit -m "feat: distinguish interactive and startup launches"
```

### Task 2: Forward a second manual launch to the existing instance

**Files:**
- Modify: `native/src/main.cpp`

**Interfaces:**
- `FindWindowExW(HWND_MESSAGE, nullptr, kWindowClass, nullptr)` locates the existing dispatcher.
- `PostMessageW(existingDispatcher, kShowSettingsMessage, 0, 0)` requests Settings.

- [ ] **Step 1: Handle an already-running instance before normal initialization**

After `InstanceLock::Acquire()` returns false, find the message-only dispatcher window. If found, post `kShowSettingsMessage` only for an interactive launch; then return 0. For `--startup`, return 0 without posting anything.

```cpp
const bool interactiveLaunch = IsInteractiveLaunch(commandLine);
InstanceLock instanceLock;
if (!instanceLock.Acquire())
{
    if (interactiveLaunch)
    {
        if (const HWND existing = FindWindowExW(HWND_MESSAGE, nullptr, kWindowClass, nullptr))
        {
            PostMessageW(existing, kShowSettingsMessage, 0, 0);
        }
    }
    return 0;
}
```

- [ ] **Step 2: Add the dispatcher handler**

Handle `kShowSettingsMessage` in `WindowProc` by calling the same Settings-opening function used by `TrayController::kSettingsCommand`. Do not create a new dialog implementation or a second tray object.

- [ ] **Step 3: Verify single-instance behavior manually**

Run the EXE, double-click it again, and confirm the original Settings window opens/focuses while Task Manager still shows one process and one tray icon.

- [ ] **Step 4: Commit**

```powershell
git add native/src/main.cpp
git commit -m "feat: focus settings on repeated manual launch"
```

### Task 3: Open Settings only for an interactive first launch

**Files:**
- Modify: `native/src/main.cpp`

**Interfaces:**
- The existing Settings flow is factored into `ShowSettings(AppContext&)` and used by tray commands and `kShowSettingsMessage`.

- [ ] **Step 1: Factor the current Settings command**

Move the body of `TrayController::kSettingsCommand` into `ShowSettings(AppContext&)`, preserving startup registration, config save, enabled-state application, language refresh, and tray refresh behavior exactly.

- [ ] **Step 2: Post the first-launch request**

After dispatcher creation, mouse monitor installation, tray creation, and timer creation succeed, post `kShowSettingsMessage` when `interactiveLaunch` is true:

```cpp
if (interactiveLaunch)
{
    PostMessageW(app.dispatcher, kShowSettingsMessage, 0, 0);
}
```

This lets the normal message loop own the dialog request and avoids showing Settings during partial initialization.

- [ ] **Step 3: Verify startup modes**

Check all four cases:

1. Double-click with no arguments opens Settings.
2. Launch with `--startup` shows only the tray icon.
3. Double-click while already running focuses Settings.
4. Tray Settings still opens Settings normally.

- [ ] **Step 4: Commit**

```powershell
git add native/src/main.cpp
git commit -m "feat: show settings for interactive launches"
```

### Task 4: Run regression tests and publish the executable

**Files:**
- Modify: `native/publish/AutoMouseCursorHider.exe` (generated output only)

- [ ] **Step 1: Build and run tests**

```powershell
cmake --build native/build --config Release
native/build/Release/CursorStateTests.exe
native/build/Release/ConfigStoreTests.exe
```

Expected output:

```text
PASS native cursor state tests
PASS native config store tests
```

- [ ] **Step 2: Replace the publish binary**

Stop any running `AutoMouseCursorHider` process, then copy `native/build/Release/AutoMouseCursorHider.exe` to `native/publish/AutoMouseCursorHider.exe`.

- [ ] **Step 3: Perform the end-to-end launch checklist**

Use the published binary and verify manual launch, `--startup`, repeated launch, tray Settings, pause/resume, cursor restoration, and clean exit.

- [ ] **Step 4: Commit the implementation and generated release binary only if the repository convention tracks it**

```powershell
git status --short
git add native/src native/publish/AutoMouseCursorHider.exe
git commit -m "feat: add launch feedback and silent startup"
```
