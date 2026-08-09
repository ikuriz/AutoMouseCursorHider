# Icon Subject Scale Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Enlarge the mouse artwork consistently across all icon sizes and refresh the EXE resource identity so the application icon updates with the tray icon.

**Architecture:** Extend the existing icon-generation script with subject-bound detection and a square crop with a controlled margin. Change the `IDI_APP` numeric resource ID while keeping the same ICO resource used by `app.rc` and `TrayController`.

## Global Constraints

- Preserve aspect ratio and avoid stretching.
- Use the same ICO for EXE and tray.
- Do not change cursor behavior or runtime dependencies.

---

### Task 1: Generate a subject-focused ICO

**Files:**
- Modify: `native/assets/build_icon.ps1`
- Modify: `native/assets/AutoMouseCursorHider.ico`

- [ ] Detect non-white artwork bounds from `app-source.png` using a fixed threshold.
- [ ] Create a centered square crop using the larger content dimension plus a small safety margin.
- [ ] Resize that crop to 16, 24, 32, 48, 64, 128, and 256 pixels with the existing high-quality settings.
- [ ] Run the script and confirm the ICO is regenerated.

### Task 2: Refresh the EXE resource identity

**Files:**
- Modify: `native/src/resource.h`

- [ ] Change `IDI_APP` from `101` to `102`.
- [ ] Keep `app.rc` and `tray.cpp` using `IDI_APP`, so no separate icon path is introduced.

### Task 3: Build, test, and publish

**Files:**
- Generate: `native/publish/AutoMouseCursorHider.exe`

- [ ] Build Release x64.
- [ ] Run `CursorStateTests.exe` and `ConfigStoreTests.exe`.
- [ ] Copy the verified EXE to `native/publish` after stopping any running copy.
- [ ] Commit the script, ICO, resource ID, and generated source asset changes.
