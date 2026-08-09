# Native Win32 v2

This directory contains the current native Windows implementation. It uses
the Win32 API, MSVC, the Windows SDK, and the static CRT, so the release
executable does not require .NET or the VC++ runtime.

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S native -B native\build -G "Visual Studio 18 2026" -A x64
& $cmake --build native\build --config Release
```

The release executable is written to `native/build/Release/AutoMouseCursorHider.exe`.
