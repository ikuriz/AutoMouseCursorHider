using System.Runtime.InteropServices;

namespace AutoMouseCursorHider;

public sealed class MouseActivityMonitor : IDisposable
{
    private const int WhMouseLl = 14;
    private const int WmMouseMove = 0x0200;
    private const int WmLButtonDown = 0x0201;
    private const int WmRButtonDown = 0x0204;
    private const int WmMButtonDown = 0x0207;
    private const int WmMouseWheel = 0x020A;
    private static MouseActivityMonitor? _current;
    private readonly HookProc _hookProc;
    private nint _hook;

    public MouseActivityMonitor()
    {
        _hookProc = HookCallback;
        _current = this;
        _hook = NativeMethods.SetWindowsHookEx(WhMouseLl, _hookProc, nint.Zero, 0);
        if (_hook == nint.Zero)
        {
            _current = null;
            throw new InvalidOperationException("无法安装全局鼠标监视器。");
        }
    }

    public event Action? Activity;

    public void Dispose()
    {
        if (_hook != nint.Zero)
        {
            NativeMethods.UnhookWindowsHookEx(_hook);
            _hook = nint.Zero;
        }

        if (ReferenceEquals(_current, this))
        {
            _current = null;
        }
    }

    private static nint HookCallback(int code, nint wParam, nint lParam)
    {
        if (code >= 0 && wParam is (WmMouseMove or WmLButtonDown or WmRButtonDown or WmMButtonDown or WmMouseWheel))
        {
            _current?.Activity?.Invoke();
        }

        return NativeMethods.CallNextHookEx(nint.Zero, code, wParam, lParam);
    }

    private delegate nint HookProc(int code, nint wParam, nint lParam);

    private static class NativeMethods
    {
        [DllImport("user32.dll", SetLastError = true)]
        internal static extern nint SetWindowsHookEx(int idHook, HookProc callback, nint module, uint threadId);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool UnhookWindowsHookEx(nint hook);

        [DllImport("user32.dll")]
        internal static extern nint CallNextHookEx(nint hook, int code, nint wParam, nint lParam);
    }
}
