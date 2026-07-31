using System.Diagnostics;
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
        using var instance = new Mutex(initiallyOwned: true, InstanceMutexName, out var createdNew);
        if (!createdNew)
        {
            return 0;
        }

        try
        {
            var controller = new WindowsCursorController();
            var runtime = new CursorRuntime(new CursorStateMachine(settings.ReadOrDefault()), controller);
            var stopwatch = Stopwatch.StartNew();
            var waitHandles = new[] { signals.Stop, signals.Reload };

            try
            {
                while (true)
                {
                    runtime.ProcessSample(controller.GetPosition(), stopwatch.Elapsed);

                    var waitResult = WaitHandle.WaitAny(waitHandles, 100);
                    if (waitResult == 0)
                    {
                        return 0;
                    }

                    if (waitResult == 1)
                    {
                        runtime.SetDelay(settings.ReadOrDefault());
                    }
                }
            }
            finally
            {
                runtime.Restore();
            }
        }
        finally
        {
            instance.ReleaseMutex();
        }
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
