# AutoMouseCursorHider

Lightweight, native Windows utility that automatically hides the mouse cursor.

The cursor hides after a period of inactivity and reappears as soon as you move the mouse. The app runs quietly in the system tray without a permanent main window.

[简体中文 README](README.zh-CN.md)

## Features

- Hide the mouse cursor globally and restore it on movement
- Tray menu for Settings, Pause/Resume, and Exit
- Adjustable hide delay from 0.1 to 3600 seconds
- Optional start with Windows
- English and Simplified Chinese interface
- Single-file app with no installer and no administrator rights required
- No network connection and no data collection
- Restores the system cursor when exiting

## Usage

1. Download `AutoMouseCursorHider.exe` from [Releases](../../releases)
2. Run the executable
3. Find the icon in the system tray and right-click it to configure, pause, resume, or exit

> Windows SmartScreen may show a warning because the executable is not commercially code-signed. After confirming that the file came from a trusted source, choose **Run anyway**.

This is a native Win32 x64 application. It does not require .NET or the Visual C++ Runtime.

## Build (developers)

Requirements: Windows 10/11 x64 and Visual Studio Build Tools with MSVC, the Windows SDK, and CMake.

```powershell
cmake -S native -B native/build -G "Visual Studio 18 2026" -A x64
cmake --build native/build --config Release
```

Output:

```text
native/build/Release/AutoMouseCursorHider.exe
```

## Versions

- `v2.1.1`: Refreshed application and tray icon with a larger mouse subject
- `v2.1.0`: Native Win32 release with launch feedback, silent startup, tray settings, bilingual UI, and unified state
- `v2.0.0`: Native Win32 rewrite
- `v1.0.0`: Early version

## License

No open-source license has been specified yet.
