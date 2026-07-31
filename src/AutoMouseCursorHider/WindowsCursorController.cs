using System.ComponentModel;
using System.Runtime.InteropServices;

namespace AutoMouseCursorHider;

public sealed partial class WindowsCursorController : ICursorController
{
    public CursorPosition GetPosition()
    {
        if (!NativeMethods.GetCursorPos(out var point))
        {
            throw new Win32Exception(Marshal.GetLastWin32Error());
        }

        return new CursorPosition(point.X, point.Y);
    }

    public void Hide()
    {
        NativeMethods.ShowCursor(false);
    }

    public void Show()
    {
        NativeMethods.ShowCursor(true);
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct NativePoint
    {
        public int X;
        public int Y;
    }

    private static partial class NativeMethods
    {
        [LibraryImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static partial bool GetCursorPos(out NativePoint point);

        [LibraryImport("user32.dll")]
        internal static partial int ShowCursor([MarshalAs(UnmanagedType.Bool)] bool show);
    }
}
