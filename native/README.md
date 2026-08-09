# Native Win32 v2

This directory contains the native Windows implementation. It is built with
MSVC and the Windows SDK, using the static CRT so the release executable does
not require the VC++ runtime.

```powershell
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -S native -B native\build -G "Visual Studio 18 2026" -A x64
& $cmake --build native\build --config Release
```

The native target is experimental until all v2 acceptance tests pass. The
verified .NET v1.0 application remains the fallback release.

