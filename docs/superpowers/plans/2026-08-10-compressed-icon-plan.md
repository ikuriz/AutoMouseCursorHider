# Compressed Application Icon Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace the uncompressed DIB icon frames with PNG-compressed ICO frames while keeping the current new artwork and resource ID.

**Architecture:** Update the PowerShell icon generator to encode each rendered frame as PNG, preserve the existing ICO directory/resource embedding, rebuild the native executable, and validate both size and icon extraction.

**Tech Stack:** PowerShell, System.Drawing, CMake/MSVC, Win32 resources.

## Global Constraints

- Keep the current artwork and `IDI_APP` resource ID.
- Do not change cursor hiding, tray behavior, settings, or localization.
- Target Windows 10/11 and reduce the release executable substantially.

### Task 1: Generate compressed ICO

**Files:**
- Modify: `native/assets/build_icon.ps1`
- Generated: `native/assets/AutoMouseCursorHider.ico`

- [ ] Render the existing 16, 24, 32, 48, 64, 128, and 256 pixel frames exactly as today.
- [ ] Encode each frame as PNG bytes and write those bytes into the ICO directory, using the PNG payload length and offset.
- [ ] Preserve the 256-pixel directory convention (`width=0`, `height=0`) and current resource filename.

### Task 2: Rebuild and validate

**Files:**
- Generated: `native/build/Release/AutoMouseCursorHider.exe`
- Generated: `native/publish/AutoMouseCursorHider.exe`

- [ ] Regenerate the ICO and rebuild Release.
- [ ] Confirm the executable is substantially smaller than 662,016 bytes.
- [ ] Confirm the executable contains resource ID 103 and that `System.Drawing.Icon.ExtractAssociatedIcon` can extract the associated icon.
- [ ] Run the existing native cursor-state and config-store tests.

### Task 3: Record the change

- [ ] Commit the generator and generated icon changes with a focused message.
- [ ] Report the measured executable size and validation results before preparing any new release.
