using System.ComponentModel;
using System.Runtime.InteropServices;

namespace AutoMouseCursorHider;

public static class CursorVisibilityMath
{
    public static int HideCallsForDisplayCount(int displayCount)
    {
        return displayCount == int.MaxValue ? int.MaxValue : Math.Max(0, displayCount + 1);
    }
}

public sealed partial class WindowsCursorController : ICursorController
{
    private const int MaximumHideCalls = 256;
    private int _hideCallCount;

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
        if (_hideCallCount != 0)
        {
            return;
        }

        var displayCount = NativeMethods.ShowCursor(false);
        _hideCallCount = 1;
        var additionalCalls = Math.Min(
            CursorVisibilityMath.HideCallsForDisplayCount(displayCount),
            MaximumHideCalls - _hideCallCount);

        for (var index = 0; index < additionalCalls; index++)
        {
            NativeMethods.ShowCursor(false);
            _hideCallCount++;
        }
    }

    public void Show()
    {
        while (_hideCallCount > 0)
        {
            NativeMethods.ShowCursor(true);
            _hideCallCount--;
        }
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
