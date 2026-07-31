using Microsoft.Win32;
using System.Text;

namespace AutoMouseCursorHider;

public sealed class DelaySettingsStore
{
    private const string SettingsKeyPath = @"Software\AutoMouseCursorHider";
    private const string DelayValueName = "DelayMilliseconds";
    private static readonly TimeSpan MinimumDelay = TimeSpan.FromMilliseconds(100);
    private static readonly TimeSpan MaximumDelay = TimeSpan.FromHours(1);
    private static readonly TimeSpan DefaultDelay = TimeSpan.FromSeconds(3);

    public TimeSpan ReadOrDefault()
    {
        using var key = Registry.CurrentUser.OpenSubKey(SettingsKeyPath);
        var value = key?.GetValue(DelayValueName);

        return value is int milliseconds && milliseconds >= MinimumDelay.TotalMilliseconds &&
            milliseconds <= MaximumDelay.TotalMilliseconds
            ? TimeSpan.FromMilliseconds(milliseconds)
            : DefaultDelay;
    }

    public void Write(TimeSpan delay)
    {
        if (delay < MinimumDelay || delay > MaximumDelay)
        {
            throw new ArgumentOutOfRangeException(nameof(delay));
        }

        var milliseconds = checked((int)Math.Round(delay.TotalMilliseconds, MidpointRounding.AwayFromZero));
        using var key = Registry.CurrentUser.CreateSubKey(SettingsKeyPath, writable: true);
        key.SetValue(DelayValueName, milliseconds, RegistryValueKind.DWord);
    }
}

public sealed class StartupManager
{
    private const string RunKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private const string ValueName = "AutoMouseCursorHider";

    public void Install(string executablePath)
    {
        using var key = Registry.CurrentUser.CreateSubKey(RunKeyPath, writable: true);
        key.SetValue(ValueName, BuildRunCommand(executablePath), RegistryValueKind.String);
    }

    public void Remove()
    {
        using var key = Registry.CurrentUser.OpenSubKey(RunKeyPath, writable: true);
        key?.DeleteValue(ValueName, throwOnMissingValue: false);
    }

    public static string BuildRunCommand(string executablePath)
    {
        if (string.IsNullOrWhiteSpace(executablePath))
        {
            throw new ArgumentException("Executable path is required.", nameof(executablePath));
        }

        var command = new StringBuilder("\"");
        var backslashCount = 0;

        foreach (var character in executablePath)
        {
            if (character == '\\')
            {
                backslashCount++;
                continue;
            }

            if (character == '\"')
            {
                command.Append('\\', backslashCount * 2 + 1);
                command.Append(character);
                backslashCount = 0;
                continue;
            }

            command.Append('\\', backslashCount);
            command.Append(character);
            backslashCount = 0;
        }

        command.Append('\\', backslashCount * 2);
        command.Append('\"');
        return command.ToString();
    }
}

public sealed class InstanceSignals : IDisposable
{
    private const string StopEventName = @"Local\AutoMouseCursorHider.Stop.v1";
    private const string ReloadEventName = @"Local\AutoMouseCursorHider.Reload.v1";
    private readonly EventWaitHandle _stop;
    private readonly EventWaitHandle _reload;

    public InstanceSignals()
    {
        _stop = new EventWaitHandle(false, EventResetMode.AutoReset, StopEventName);
        _reload = new EventWaitHandle(false, EventResetMode.AutoReset, ReloadEventName);
    }

    public WaitHandle Stop => _stop;
    public WaitHandle Reload => _reload;

    public static void SignalStop()
    {
        Signal(StopEventName);
    }

    public static void SignalReload()
    {
        Signal(ReloadEventName);
    }

    public void Dispose()
    {
        _stop.Dispose();
        _reload.Dispose();
    }

    private static void Signal(string eventName)
    {
        try
        {
            using var signal = EventWaitHandle.OpenExisting(eventName);
            signal.Set();
        }
        catch (WaitHandleCannotBeOpenedException)
        {
        }
    }
}
