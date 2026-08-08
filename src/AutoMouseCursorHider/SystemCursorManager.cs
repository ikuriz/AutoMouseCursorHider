using System.Runtime.InteropServices;

namespace AutoMouseCursorHider;

public static class SystemCursorIds
{
    public const uint Normal = 32512;
    public const uint IBeam = 32513;
    public const uint Wait = 32514;
    public const uint Cross = 32515;
    public const uint Up = 32516;
    public const uint SizeNwse = 32642;
    public const uint SizeNesw = 32643;
    public const uint SizeWe = 32644;
    public const uint SizeNs = 32645;
    public const uint SizeAll = 32646;
    public const uint No = 32648;
    public const uint Hand = 32649;
    public const uint AppStarting = 32650;

    public static readonly uint[] All =
    [Normal, IBeam, Wait, Cross, Up, SizeNwse, SizeNesw, SizeWe, SizeNs, SizeAll, No, Hand, AppStarting];
}

public sealed partial class SystemCursorManager
{
    private const uint SpiSetCursors = 0x0057;
    private const uint SpifSendChange = 0x0002;
    private bool _hidden;
    private bool _restorePending;

    public bool IsHidden => _hidden;
    public bool IsRestorePending => _restorePending;

    public void Hide()
    {
        if (_hidden)
        {
            return;
        }

        var andMask = new byte[128];
        Array.Fill(andMask, byte.MaxValue);
        var xorMask = new byte[128];
        var andHandle = Marshal.AllocHGlobal(andMask.Length);
        var xorHandle = Marshal.AllocHGlobal(xorMask.Length);
        try
        {
            Marshal.Copy(andMask, 0, andHandle, andMask.Length);
            Marshal.Copy(xorMask, 0, xorHandle, xorMask.Length);
            _hidden = true;
            _restorePending = false;

            foreach (var id in SystemCursorIds.All)
            {
                var cursor = NativeMethods.CreateCursor(IntPtr.Zero, 0, 0, 32, 32, andHandle, xorHandle);
                if (cursor == IntPtr.Zero || !NativeMethods.SetSystemCursor(cursor, id))
                {
                    Restore();
                    throw new InvalidOperationException("无法替换系统光标。");
                }
            }

        }
        finally
        {
            Marshal.FreeHGlobal(andHandle);
            Marshal.FreeHGlobal(xorHandle);
        }
    }

    public void Restore() => TryRestore();

    public bool RestoreAndRefresh()
    {
        if (!TryRestore())
        {
            return false;
        }

        if (NativeMethods.GetCursorPos(out var point))
        {
            NativeMethods.SetCursorPos(point.X, point.Y);
        }

        return true;
    }

    public bool TryRestore()
    {
        if (!_hidden && !_restorePending)
        {
            return true;
        }

        if (!NativeMethods.SystemParametersInfo(SpiSetCursors, 0, IntPtr.Zero, SpifSendChange))
        {
            _restorePending = true;
            return false;
        }

        _hidden = false;
        _restorePending = false;
        return true;
    }

    private static partial class NativeMethods
    {
        [LibraryImport("user32.dll", SetLastError = true)]
        internal static partial IntPtr CreateCursor(IntPtr instance, int hotSpotX, int hotSpotY, int width, int height, IntPtr andMask, IntPtr xorMask);

        [LibraryImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static partial bool SetSystemCursor(IntPtr cursor, uint id);

        [LibraryImport("user32.dll", EntryPoint = "SystemParametersInfoW", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static partial bool SystemParametersInfo(uint action, uint parameter, IntPtr value, uint flags);

        [LibraryImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static partial bool GetCursorPos(out Point point);

        [LibraryImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static partial bool SetCursorPos(int x, int y);

        [StructLayout(LayoutKind.Sequential)]
        internal struct Point
        {
            public int X;
            public int Y;
        }
    }
}

public sealed class SystemCursorController : ICursorController
{
    private readonly SystemCursorManager _manager;

    public SystemCursorController(SystemCursorManager manager) => _manager = manager;

    public CursorPosition GetPosition() => default;
    public void Hide() => _manager.Hide();
    public void Show() => _manager.RestoreAndRefresh();
}
