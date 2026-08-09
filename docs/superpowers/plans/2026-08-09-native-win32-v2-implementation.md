# Native Win32 v2.0 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild AutoMouseCursorHider as a self-contained native Win32 x64 application with no .NET or VC++ Runtime dependency.

**Architecture:** Keep the verified .NET `v1.0.0` untouched as a fallback. Add a separate `native/` C++ target using a hidden Win32 message window, a message-thread `WH_MOUSE_LL` hook, `SetSystemCursor` for hiding, and `SystemParametersInfoW(SPI_SETCURSORS)` plus cursor-position refresh for restoration. Add tray, single-instance, settings, INI, and startup integration in later stages.

**Tech Stack:** C++17, Win32 API, Windows SDK, MSVC x64, CMake, static CRT (`/MT`), Unicode, Windows 10/11.

## Global Constraints

- The existing .NET `v1.0.0` remains available and is not replaced until native v2.0 passes acceptance.
- The final native release must be one x64 EXE with no .NET or VC++ Runtime dependency.
- The program must not request administrator privileges, access the network, or collect data.
- Every Win32 API result must be checked; the low-level hook must always call `CallNextHookEx`.
- Cursor cleanup is required on startup, pause, normal exit, and retry paths after a restore failure.
- Use a high-DPI manifest and compile with `/MT`; do not use UPX.

---

### Task 1: Native build skeleton

**Files:**
- Create: `native/CMakeLists.txt`
- Create: `native/src/main.cpp`
- Create: `native/src/resource.h`
- Create: `native/src/app.rc`
- Create: `native/README.md`
- Modify: `.gitignore`

**Interfaces:**
- Produces a `AutoMouseCursorHiderNative.exe` target built with the Windows GUI subsystem.
- `wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)` creates no visible main window yet and exits cleanly after initializing the message loop.

- [ ] Install/verify the MSVC x64 C++ toolchain and CMake; configure with `cmake -S native -B native/build -G "Visual Studio 17 2022" -A x64`.
- [ ] Define Release flags with `/MT`, Unicode, `/DUNICODE`, `/D_UNICODE`, high-DPI manifest, and Windows subsystem.
- [ ] Add a minimal resource file containing the application icon and version metadata.
- [ ] Build the skeleton with `cmake --build native/build --config Release` and verify the EXE starts and exits without a console window.
- [ ] Run `dumpbin /dependents native/build/Release/AutoMouseCursorHiderNative.exe` and record that no VC++ runtime DLL is listed.
- [ ] Commit as `build: add native Win32 v2 skeleton`.

### Task 2: Cursor state and system replacement core

**Files:**
- Create: `native/src/cursor_manager.h`
- Create: `native/src/cursor_manager.cpp`
- Create: `native/src/cursor_state.h`
- Create: `native/tests/cursor_state_tests.cpp`
- Modify: `native/CMakeLists.txt`

**Interfaces:**
- `class CursorManager { bool Hide(); bool Restore(); bool RestoreAndRefresh(); bool RetryRestore(); bool IsHidden() const; bool IsRestorePending() const; }`.
- `class CursorState { void OnActivity(); void OnTimer(std::uint64_t now); bool ShouldHide() const; }`.
- `CursorManager` owns the 13 system cursor IDs and the transparent cursor mask allocation; `CursorState` contains no Win32 calls.

- [ ] Write failing state tests for visible-to-hidden threshold, activity-to-visible transition, pause behavior, delay reset, and restore retry state.
- [ ] Implement the pure state machine using monotonic tick values and a configurable delay range of 0.1 to 3600 seconds.
- [ ] Implement transparent 32x32 cursor creation with checked allocation and `SetSystemCursor` calls for all 13 IDs.
- [ ] Implement `Restore()` with `SystemParametersInfoW(SPI_SETCURSORS, 0, nullptr, SPIF_SENDCHANGE)` and `RestoreAndRefresh()` with `GetCursorPos` followed by `SetCursorPos` at the same coordinates.
- [ ] Ensure partial hide failure immediately attempts restoration and records a retryable state instead of throwing across the message loop.
- [ ] Build and run `cursor_state_tests`; expected result: all tests pass.
- [ ] Commit as `feat: add native cursor replacement core`.

### Task 3: Global hook and idle timer integration

**Files:**
- Create: `native/src/mouse_monitor.h`
- Create: `native/src/mouse_monitor.cpp`
- Modify: `native/src/main.cpp`
- Modify: `native/CMakeLists.txt`

**Interfaces:**
- `class MouseMonitor { bool Install(HWND dispatcher); void Uninstall(); }`.
- Hook callback only records activity and posts a private window message; it never calls `SystemParametersInfoW` directly.
- The dispatcher window handles `WM_APP + 1` for activity and `WM_TIMER` for idle checks.

- [ ] Write an integration test checklist for activity after hiding, activity in Explorer, activity on the desktop, and activity over a tray menu.
- [ ] Install `WH_MOUSE_LL` on the message-loop thread and root the callback function for its entire lifetime.
- [ ] Handle mouse move, button, wheel, and extra button messages; always call `CallNextHookEx`.
- [ ] On activity, update the monotonic timestamp immediately, coalesce duplicate posted messages, then restore on the message thread.
- [ ] Use `SetTimer` at 50–100 ms cadence for idle evaluation and retry pending restoration.
- [ ] On startup, call `RestoreAndRefresh()` before starting the timer; on teardown, uninstall the hook before restoring and destroying the dispatcher window.
- [ ] Run the native EXE and manually verify hidden-after-delay and visible-after-move in Explorer and on the desktop.
- [ ] Commit as `feat: add native global mouse monitor`.

### Task 4: Tray UI and single-instance control

**Files:**
- Create: `native/src/tray.h`
- Create: `native/src/tray.cpp`
- Create: `native/src/instance_lock.h`
- Create: `native/src/instance_lock.cpp`
- Modify: `native/src/main.cpp`
- Modify: `native/src/app.rc`

**Interfaces:**
- `class TrayController { bool Create(HWND); void Remove(); void SetPaused(bool); }`.
- `class InstanceLock { bool Acquire(); }` using a `Local\\AutoMouseCursorHider.Native.Instance.v2` mutex.
- Tray commands are private window messages: settings, pause/resume, and exit.

- [ ] Add a named mutex before creating the hook or changing cursors; a second launch exits without side effects.
- [ ] Register a `NOTIFYICONDATAW` icon with `Shell_NotifyIconW(NIM_ADD)` and provide a compact context menu.
- [ ] Implement pause/resume so pause restores the cursor and prevents future hides; resume resets the idle interval.
- [ ] Implement exit through `PostMessage` to the dispatcher so cleanup occurs on the message thread.
- [ ] Test duplicate launch, tray menu commands, pause/resume, normal exit, and cursor restoration after exit.
- [ ] Commit as `feat: add native tray and single instance`.

### Task 5: Native settings, INI, and startup registration

**Files:**
- Create: `native/src/settings_dialog.h`
- Create: `native/src/settings_dialog.cpp`
- Create: `native/src/config_store.h`
- Create: `native/src/config_store.cpp`
- Create: `native/src/startup_registration.h`
- Create: `native/src/startup_registration.cpp`
- Modify: `native/src/tray.cpp`
- Modify: `native/src/main.cpp`

**Interfaces:**
- `struct AppSettings { double delaySeconds; bool startupEnabled; }`.
- `ConfigStore::Load()` and `ConfigStore::Save(const AppSettings&)` use `%LOCALAPPDATA%\\AutoMouseCursorHider\\settings.ini`.
- `StartupRegistration::IsEnabled()`, `Enable(exePath)`, and `Disable()` use the current-user Run key only.

- [ ] Write parser tests for decimal values, invariant/current culture handling, bounds, malformed files, and safe defaults.
- [ ] Create the native dialog with a numeric edit control and up/down control; keep the label and value visually separated under DPI scaling.
- [ ] Save validated settings atomically, create the per-user directory if needed, and ignore malformed values in favor of the safe default.
- [ ] Add current-user startup enable/disable without elevation and quote the executable path correctly.
- [ ] Test persistence across restart, decimal editing, invalid input, startup toggling, and settings access from the tray menu.
- [ ] Commit as `feat: add native settings and startup configuration`.

### Task 6: Hardening, packaging, and dependency verification

**Files:**
- Modify: `native/CMakeLists.txt`
- Modify: `native/src/app.rc`
- Modify: `README.md`
- Create: `native/scripts/verify-release.ps1`
- Create: `native/publish/README.txt`

**Interfaces:**
- Release output is `native/publish/AutoMouseCursorHider.exe`.
- `verify-release.ps1` fails if the EXE is missing, not x64, linked to VC++ runtime DLLs, or accompanied by unexpected runtime files.

- [ ] Enable Release optimization, strip debug artifacts from the publish directory, and embed the high-DPI manifest.
- [ ] Run `dumpbin /headers` and `dumpbin /dependents` to verify x64, Windows GUI subsystem, and no VC++ runtime dependency.
- [ ] Run the EXE on a machine with .NET disabled/uninstalled and confirm it still starts; confirm no .NET DLL is loaded.
- [ ] Verify normal exit, pause, startup, forced termination followed by next launch, and all Win32 error paths restore cursors.
- [ ] Update README with download, safety, recovery, build, and antivirus notes; keep .NET v1.0 instructions as fallback.
- [ ] Commit as `release: prepare native Win32 v2.0` only after the complete acceptance checklist passes.

## Final acceptance checklist

- [ ] Static single-EXE release has no .NET or VC++ Runtime dependency.
- [ ] Mouse hides only after the configured idle interval in every tested application.
- [ ] Any mouse activity restores the cursor promptly without an exception dialog.
- [ ] Startup normalizes a stale cursor state from a prior forced termination.
- [ ] Tray pause/resume/settings/exit work without a visible main window.
- [ ] Duplicate launches do not create additional hooks or tray icons.
- [ ] Decimal settings, INI persistence, and current-user startup work.
- [ ] High-DPI display is readable and controls remain aligned.
- [ ] Existing .NET `v1.0.0` remains available as a fallback.
