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
    private readonly ICursorController _cursor;
    private readonly IDisposable _instanceLease;
    private readonly DelaySettingsStore _settings;
    private readonly InstanceSignals _signals;
    private readonly object _runtimeGate = new();
    private readonly SynchronizationContext _uiContext;
    private readonly Stopwatch _clock = Stopwatch.StartNew();
    private bool _disposed;

    public TrayApplicationContext(
        CursorRuntime runtime,
        RuntimeState state,
        ICursorController cursor,
        IDisposable instanceLease,
        DelaySettingsStore settings,
        InstanceSignals signals,
        Func<Form>? settingsFactory = null)
    {
        _runtime = runtime ?? throw new ArgumentNullException(nameof(runtime));
        _state = state ?? throw new ArgumentNullException(nameof(state));
        _cursor = cursor ?? throw new ArgumentNullException(nameof(cursor));
        _instanceLease = instanceLease ?? throw new ArgumentNullException(nameof(instanceLease));
        _settings = settings ?? throw new ArgumentNullException(nameof(settings));
        _signals = signals ?? throw new ArgumentNullException(nameof(signals));
        _uiContext = SynchronizationContext.Current ?? new WindowsFormsSynchronizationContext();

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
            Icon = SystemIcons.Application,
            Text = "AutoMouseCursorHider",
            Visible = true,
            ContextMenuStrip = menu
        };
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
                _uiContext.Post(_ => ExitThread(), null);
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

            try
            {
                var position = _cursor.GetPosition();
                var action = _runtime.ObserveSample(position, _clock.Elapsed);
                if (action != CursorAction.None)
                {
                    _uiContext.Post(_ => ApplyCursorAction(action), null);
                }
            }
            catch (System.ComponentModel.Win32Exception)
            {
                // A desktop/API failure is retried on the next background tick.
            }
        }
    }

    private void ApplyCursorAction(CursorAction action)
    {
        lock (_runtimeGate)
        {
            if (!_disposed)
            {
                _runtime.ApplyAction(action);
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
