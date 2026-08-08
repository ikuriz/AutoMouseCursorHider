using System.Drawing;
using System.Diagnostics;
using System.Windows.Forms;

namespace AutoMouseCursorHider;

public sealed class TrayApplicationContext : ApplicationContext
{
    private readonly NotifyIcon _notifyIcon;
    private readonly System.Windows.Forms.Timer _timer;
    private readonly RuntimeState _state;
    private readonly CursorRuntime _runtime;
    private readonly ICursorController _cursor;
    private readonly IDisposable _instanceLease;
    private readonly DelaySettingsStore _settings;
    private readonly InstanceSignals _signals;
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
            if (_state.IsPaused)
            {
                _state.Resume();
            }
            else
            {
                _state.Pause();
            }

            pauseItem.Text = TrayMenuLabels.Pause(_state.IsPaused);
            _notifyIcon.Text = _state.IsPaused ? "AutoMouseCursorHider（已暂停）" : "AutoMouseCursorHider（运行中）";
        };
        exitItem.Click += (_, _) => ExitThread();

        _timer = new System.Windows.Forms.Timer { Interval = 100 };
        _timer.Tick += (_, _) => Tick();
        _timer.Start();
    }

    private void Tick()
    {
        if (_signals.Stop.WaitOne(0))
        {
            ExitThread();
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

        _runtime.ProcessSample(_cursor.GetPosition(), _clock.Elapsed);
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
            _timer.Stop();
            _timer.Dispose();
            _notifyIcon.Visible = false;
            _notifyIcon.Dispose();
            _runtime.Restore();
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
