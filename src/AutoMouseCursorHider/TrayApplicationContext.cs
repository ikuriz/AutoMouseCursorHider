using System.Drawing;
using System.Diagnostics;
using System.Windows.Forms;

namespace AutoMouseCursorHider;

public sealed class TrayApplicationContext : ApplicationContext
{
    private readonly NotifyIcon _notifyIcon;
    private readonly System.Threading.Timer _timer;
    private readonly RuntimeState _state;
    private readonly CursorRuntime _runtime;
    private readonly MouseActivityMonitor _mouseMonitor;
    private readonly SystemCursorManager _cursorManager;
    private readonly IDisposable _instanceLease;
    private readonly DelaySettingsStore _settings;
    private readonly InstanceSignals _signals;
    private readonly object _runtimeGate = new();
    private readonly Control _dispatcher;
    private readonly Stopwatch _clock = Stopwatch.StartNew();
    private TimeSpan _lastActivity;
    private int _activityQueued;
    private bool _disposed;

    public TrayApplicationContext(
        CursorRuntime runtime,
        RuntimeState state,
        MouseActivityMonitor mouseMonitor,
        SystemCursorManager cursorManager,
        IDisposable instanceLease,
        DelaySettingsStore settings,
        InstanceSignals signals,
        Func<Form>? settingsFactory = null)
    {
        _runtime = runtime ?? throw new ArgumentNullException(nameof(runtime));
        _state = state ?? throw new ArgumentNullException(nameof(state));
        _mouseMonitor = mouseMonitor ?? throw new ArgumentNullException(nameof(mouseMonitor));
        _cursorManager = cursorManager ?? throw new ArgumentNullException(nameof(cursorManager));
        _instanceLease = instanceLease ?? throw new ArgumentNullException(nameof(instanceLease));
        _settings = settings ?? throw new ArgumentNullException(nameof(settings));
        _signals = signals ?? throw new ArgumentNullException(nameof(signals));
        _dispatcher = new Control();
        _ = _dispatcher.Handle;
        _mouseMonitor.Activity += OnMouseActivity;

        var menu = new ContextMenuStrip();
        var settingsItem = new ToolStripMenuItem(TrayMenuLabels.Settings);
        var pauseItem = new ToolStripMenuItem(TrayMenuLabels.Pause(_state.IsPaused));
        var exitItem = new ToolStripMenuItem(TrayMenuLabels.Exit);
        menu.Items.Add(settingsItem);
        menu.Items.Add(pauseItem);
        menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add(exitItem);

        _notifyIcon = new NotifyIcon
        {
            Icon = SystemIcons.Information,
            Text = "AutoMouseCursorHider",
            BalloonTipTitle = "AutoMouseCursorHider",
            BalloonTipText = "程序正在运行，可在此图标的右键菜单中打开设置。",
            BalloonTipIcon = ToolTipIcon.Info,
            Visible = true,
            ContextMenuStrip = menu
        };
        _dispatcher.BeginInvoke(new Action(() => _notifyIcon.ShowBalloonTip(2500)));
        _notifyIcon.MouseClick += (_, args) =>
        {
            if (args.Button == MouseButtons.Left)
            {
                OpenSettings(settingsFactory);
            }
        };
        _notifyIcon.DoubleClick += (_, _) => OpenSettings(settingsFactory);

        settingsItem.Click += (_, _) => OpenSettings(settingsFactory);
        pauseItem.Click += (_, _) =>
        {
            lock (_runtimeGate)
            {
                if (_state.IsPaused)
                {
                    _state.Resume();
                }
                else
                {
                    _state.Pause();
                }
            }

            pauseItem.Text = TrayMenuLabels.Pause(_state.IsPaused);
            _notifyIcon.Text = _state.IsPaused ? "AutoMouseCursorHider（已暂停）" : "AutoMouseCursorHider（运行中）";
        };
        exitItem.Click += (_, _) => ExitThread();

        _timer = new System.Threading.Timer(_ =>
        {
            try
            {
                Tick();
            }
            catch (Exception)
            {
                // A transient desktop/API failure must not terminate the tray process.
            }
        }, null, TimeSpan.Zero, TimeSpan.FromMilliseconds(100));
    }

    private void Tick()
    {
        lock (_runtimeGate)
        {
            if (_disposed)
            {
                return;
            }

            if (_signals.Stop.WaitOne(0))
            {
                _dispatcher.BeginInvoke(new Action(ExitThread));
                return;
            }

            if (_signals.Reload.WaitOne(0))
            {
                _state.SetDelay(_settings.ReadOrDefault());
            }

            if (_state.IsPaused)
            {
                return;
            }

            _runtime.ProcessIdle(false, _clock.Elapsed - _lastActivity);
            if (_cursorManager.IsRestorePending)
            {
                _cursorManager.RestoreAndRefresh();
            }
        }
    }

    private void OnMouseActivity()
    {
        lock (_runtimeGate)
        {
            if (_disposed)
            {
                return;
            }

            // Keep the idle timestamp lightweight inside the hook. The actual
            // SPI_SETCURSORS call is posted to the WinForms message thread.
            _lastActivity = _clock.Elapsed;
            if (_state.IsPaused || !_cursorManager.IsHidden || Interlocked.Exchange(ref _activityQueued, 1) != 0)
            {
                return;
            }

            try
            {
                _dispatcher.BeginInvoke(new Action(HandleMouseActivity));
            }
            catch (InvalidOperationException)
            {
                Interlocked.Exchange(ref _activityQueued, 0);
            }
        }
    }

    private void HandleMouseActivity()
    {
        Interlocked.Exchange(ref _activityQueued, 0);
        lock (_runtimeGate)
        {
            if (!_disposed && !_state.IsPaused)
            {
                _runtime.ProcessIdle(true, TimeSpan.Zero);
            }
        }
    }

    private static void OpenSettings(Func<Form>? settingsFactory)
    {
        if (settingsFactory is null)
        {
            return;
        }

        using var form = settingsFactory();
        form.ShowDialog();
    }

    protected override void Dispose(bool disposing)
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        if (disposing)
        {
            lock (_runtimeGate)
            {
                _timer.Dispose();
                _runtime.Restore();
            }
            _notifyIcon.Visible = false;
            _notifyIcon.Dispose();
            _dispatcher.Dispose();
            _mouseMonitor.Dispose();
            _instanceLease.Dispose();
        }

        base.Dispose(disposing);
    }
}

public static class TrayMenuLabels
{
    public const string Settings = "打开设置";
    public const string Exit = "退出";

    public static string Pause(bool isPaused) => isPaused ? "恢复" : "暂停";
}
