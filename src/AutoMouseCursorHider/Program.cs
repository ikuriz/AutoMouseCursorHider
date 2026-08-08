using System.Runtime.InteropServices;

namespace AutoMouseCursorHider;

internal static partial class Program
{
    private const string InstanceMutexName = @"Local\AutoMouseCursorHider.Instance.v1";
    private const string Usage = "AutoMouseCursorHider\n\n" +
        "--delay <seconds>   Set delay from 0.1 to 3600 seconds.\n" +
        "--startup           Enable current-user startup.\n" +
        "--no-startup        Disable current-user startup.\n" +
        "--stop              Stop the running instance.\n" +
        "--help              Show this help.";

    [STAThread]
    private static int Main(string[] args)
    {
        try
        {
            Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            if (!CommandLine.TryParse(args, out var command, out var error))
            {
                return Report(error, 2);
            }

            var settings = new DelaySettingsStore();
            using var signals = new InstanceSignals();

            if (command.Delay is { } delay)
            {
                settings.Write(delay);
                InstanceSignals.SignalReload();
            }

            switch (command.Kind)
            {
                case AppCommandKind.Help:
                    ShowMessage(Usage, "AutoMouseCursorHider", 0x00000040);
                    return 0;
                case AppCommandKind.Startup:
                    new StartupManager().Install(Environment.ProcessPath ?? throw new InvalidOperationException("Executable path is unavailable."));
                    return 0;
                case AppCommandKind.RemoveStartup:
                    new StartupManager().Remove();
                    return 0;
                case AppCommandKind.Stop:
                    InstanceSignals.SignalStop();
                    return 0;
                case AppCommandKind.Run:
                    return Run(settings, signals);
                default:
                    return Report("Unsupported command.", 2);
            }
        }
        catch (Exception exception)
        {
            return Report(exception.Message, 1);
        }
    }

    private static int Run(DelaySettingsStore settings, InstanceSignals signals)
    {
        if (!SingleInstance.TryAcquire(InstanceMutexName, out var lease) || lease is null)
        {
            return 0;
        }

        var cursorManager = new SystemCursorManager();
        var controller = new SystemCursorController(cursorManager);
        var runtime = new CursorRuntime(new CursorStateMachine(settings.ReadOrDefault()), controller);
        var state = new RuntimeState(runtime, settings.ReadOrDefault());
        var startup = new StartupManager();
        Func<Form> settingsFactory = () => new SettingsForm(
            state.Delay,
            startup.IsInstalled(),
            state.IsPaused,
            draft =>
            {
                settings.Write(draft.Delay);
                if (draft.StartupEnabled)
                {
                    startup.Install(Environment.ProcessPath ?? throw new InvalidOperationException("Executable path is unavailable."));
                }
                else
                {
                    startup.Remove();
                }

                state.SetDelay(draft.Delay);
                return null;
            });
        using var mouseMonitor = new MouseActivityMonitor();
        using var context = new TrayApplicationContext(runtime, state, mouseMonitor, lease, settings, signals, settingsFactory);
        Application.Run(context);
        return 0;
    }

    private static int Report(string message, int exitCode)
    {
        ShowMessage(message, "AutoMouseCursorHider", 0x00000010);
        return exitCode;
    }

    private static void ShowMessage(string message, string caption, uint type)
    {
        NativeMethods.MessageBox(nint.Zero, message, caption, type);
    }

    private static partial class NativeMethods
    {
        [LibraryImport("user32.dll", EntryPoint = "MessageBoxW", StringMarshalling = StringMarshalling.Utf16)]
        internal static partial int MessageBox(nint windowHandle, string text, string caption, uint type);
    }
}
