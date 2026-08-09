# Icon Refresh Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the application and tray icon with the supplied full-frame minimalist mouse artwork while preserving the complete square composition.

**Architecture:** Keep the existing resource-based icon pipeline. Store the supplied PNG as the source asset, regenerate the multi-size ICO without content cropping, and keep `app.rc` plus `TrayController` pointing to the shared `IDI_APP` resource.

**Tech Stack:** PowerShell/System.Drawing asset script, Windows ICO resource, native Win32 C++17, CMake/MSVC.

## Global Constraints

- Preserve the complete square artwork and its white space in this first iteration.
- Use one generated ICO for both the EXE and tray icon.
- Do not change cursor hiding, settings, startup, or localization behavior.
- Do not add .NET, VC++ runtime, or other runtime dependencies.

---

### Task 1: Replace the icon source and regenerate the ICO

**Files:**
- Add: `native/assets/app-source.png`
- Modify: `native/assets/build_icon.ps1`
- Modify: `native/assets/AutoMouseCursorHider.ico`

- [ ] Copy the supplied PNG into `native/assets/app-source.png` without cropping or editing.
- [ ] Remove the previous artwork-detection crop from `build_icon.ps1`; resize the complete square source directly into 16, 24, 32, 48, 64, 128, and 256 pixel PNG frames.
- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File native/assets/build_icon.ps1` and confirm the ICO is regenerated.

### Task 2: Verify native resource integration

**Files:**
- Inspect: `native/src/app.rc`
- Inspect: `native/src/tray.cpp`
- Modify: `native/CMakeLists.txt` only if the asset path is not already listed

- [ ] Confirm `IDI_APP ICON "../assets/AutoMouseCursorHider.ico"` remains the resource definition.
- [ ] Confirm `TrayController` loads `IDI_APP` rather than the generic Windows information icon.
- [ ] Keep the existing single ICO reference so EXE and tray visuals stay identical.

### Task 3: Build, test, and publish

**Files:**
- Generate: `native/publish/AutoMouseCursorHider.exe`

- [ ] Build Release x64 with the existing CMake generator.
- [ ] Run `CursorStateTests.exe` and `ConfigStoreTests.exe`; both must pass.
- [ ] Copy the verified Release EXE to `native/publish/AutoMouseCursorHider.exe` after stopping any running copy.
- [ ] Inspect the executable and tray icon manually; if the full-frame artwork is too faint at tray size, report that result before starting a cropped second iteration.
- [ ] Commit the source image, generated ICO, script, and any required resource changes.
