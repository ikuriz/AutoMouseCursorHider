using AutoMouseCursorHider;

var failures = 0;

Run("hides at threshold and shows on movement", () =>
{
    var machine = new CursorStateMachine(TimeSpan.FromSeconds(3));
    var point = new CursorPosition(100, 200);

    Equal(CursorAction.None, machine.Observe(point, TimeSpan.Zero));
    Equal(CursorAction.None, machine.Observe(point, TimeSpan.FromSeconds(2.9)));
    Equal(CursorAction.Hide, machine.Observe(point, TimeSpan.FromSeconds(3)));
    Equal(CursorAction.None, machine.Observe(point, TimeSpan.FromSeconds(3.1)));
    Equal(CursorAction.Show, machine.Observe(new CursorPosition(101, 200), TimeSpan.FromSeconds(3.2)));
});

Run("parses an invariant-culture delay", () =>
{
    True(CommandLine.TryParse(["--delay", "2.5"], out var command, out _));
    Equal(AppCommandKind.Run, command.Kind);
    Equal(TimeSpan.FromMilliseconds(2500), command.Delay!.Value);
});

Run("allows a delay while installing startup", () =>
{
    True(CommandLine.TryParse(["--startup", "--delay", "5"], out var command, out _));
    Equal(AppCommandKind.Startup, command.Kind);
    Equal(TimeSpan.FromSeconds(5), command.Delay!.Value);
});

Run("rejects invalid delay values and conflicting commands", () =>
{
    False(CommandLine.TryParse(["--delay"], out _, out _));
    False(CommandLine.TryParse(["--delay", "0.09"], out _, out _));
    False(CommandLine.TryParse(["--delay", "3600.1"], out _, out _));
    False(CommandLine.TryParse(["--delay", "NaN"], out _, out _));
    False(CommandLine.TryParse(["--stop", "--startup"], out _, out _));
    False(CommandLine.TryParse(["--stop", "--delay", "2"], out _, out _));
    False(CommandLine.TryParse(["--unknown"], out _, out _));
    False(CommandLine.TryParse(["--help", "--help"], out _, out _));
});

Run("runtime hides and shows exactly once", () =>
{
    var cursor = new FakeCursorController();
    var runtime = new CursorRuntime(new CursorStateMachine(TimeSpan.FromSeconds(1)), cursor);

    runtime.ProcessSample(new CursorPosition(1, 1), TimeSpan.Zero);
    runtime.ProcessSample(new CursorPosition(1, 1), TimeSpan.FromSeconds(1));
    runtime.ProcessSample(new CursorPosition(1, 1), TimeSpan.FromSeconds(1.1));
    runtime.ProcessSample(new CursorPosition(2, 1), TimeSpan.FromSeconds(1.2));
    runtime.Restore();

    Equal(1, cursor.HideCount);
    Equal(1, cursor.ShowCount);
});

Run("startup command quotes a path containing spaces", () =>
{
    Equal("\"C:\\Program Files\\Cursor Tools\\AutoMouseCursorHider.exe\"",
        StartupManager.BuildRunCommand(@"C:\Program Files\Cursor Tools\AutoMouseCursorHider.exe"));
});

Run("new delay starts a fresh idle interval", () =>
{
    var cursor = new FakeCursorController();
    var runtime = new CursorRuntime(new CursorStateMachine(TimeSpan.FromSeconds(1)), cursor);

    runtime.ProcessSample(new CursorPosition(4, 4), TimeSpan.Zero);
    runtime.SetDelay(TimeSpan.FromSeconds(2));
    runtime.ProcessSample(new CursorPosition(4, 4), TimeSpan.FromSeconds(1));
    Equal(0, cursor.HideCount);
    runtime.ProcessSample(new CursorPosition(4, 4), TimeSpan.FromSeconds(3));
    Equal(1, cursor.HideCount);
});

Run("cursor visibility math compensates for an existing display count", () =>
{
    Equal(0, CursorVisibilityMath.HideCallsForDisplayCount(-1));
    Equal(1, CursorVisibilityMath.HideCallsForDisplayCount(0));
    Equal(4, CursorVisibilityMath.HideCallsForDisplayCount(3));
});

Run("runtime state pause and resume reset visibility", () =>
{
    var cursor = new FakeCursorController();
    var runtime = new CursorRuntime(new CursorStateMachine(TimeSpan.FromSeconds(1)), cursor);
    var state = new RuntimeState(runtime, TimeSpan.FromSeconds(1));

    runtime.ProcessSample(new CursorPosition(10, 10), TimeSpan.Zero);
    runtime.ProcessSample(new CursorPosition(10, 10), TimeSpan.FromSeconds(1));
    state.Pause();
    Equal(true, state.IsPaused);
    Equal(1, cursor.ShowCount);

    state.Resume();
    Equal(false, state.IsPaused);
    runtime.ProcessSample(new CursorPosition(10, 10), TimeSpan.FromSeconds(1.1));
    Equal(1, cursor.HideCount);
});

Run("runtime state changes delay and starts a fresh interval", () =>
{
    var cursor = new FakeCursorController();
    var runtime = new CursorRuntime(new CursorStateMachine(TimeSpan.FromSeconds(1)), cursor);
    var state = new RuntimeState(runtime, TimeSpan.FromSeconds(1));

    runtime.ProcessSample(new CursorPosition(20, 20), TimeSpan.Zero);
    state.SetDelay(TimeSpan.FromSeconds(2));
    runtime.ProcessSample(new CursorPosition(20, 20), TimeSpan.FromSeconds(1));
    Equal(0, cursor.HideCount);
    runtime.ProcessSample(new CursorPosition(20, 20), TimeSpan.FromSeconds(3));
    Equal(1, cursor.HideCount);
});

Run("tray menu stays compact and changes pause label", () =>
{
    Equal("打开设置", TrayMenuLabels.Settings);
    Equal("退出", TrayMenuLabels.Exit);
    Equal("暂停", TrayMenuLabels.Pause(false));
    Equal("恢复", TrayMenuLabels.Pause(true));
});

Run("settings validation accepts decimal delay and rejects unsafe values", () =>
{
    True(SettingsValidation.TryParseDelay("2.5", out var delay, out _));
    Equal(TimeSpan.FromMilliseconds(2500), delay);
    False(SettingsValidation.TryParseDelay("0", out _, out _));
    False(SettingsValidation.TryParseDelay("3600.1", out _, out _));
    False(SettingsValidation.TryParseDelay("NaN", out _, out _));
    False(SettingsValidation.TryParseDelay("abc", out _, out _));
});

Run("single instance rejects a second lease", () =>
{
    var name = $"Local\\AutoMouseCursorHider.Tests.{Guid.NewGuid():N}";
    True(SingleInstance.TryAcquire(name, out var first));
    True(first is not null);
    False(SingleInstance.TryAcquire(name, out var second));
    Equal(null, second);
    first!.Dispose();
});

Run("delay spinner snaps from minimum to the half-second sequence", () =>
{
    Equal(0.1M, DelayStepPolicy.Down(0.5M));
    Equal(0.5M, DelayStepPolicy.Up(0.1M));
    Equal(1.0M, DelayStepPolicy.Up(0.5M));
    Equal(0.5M, DelayStepPolicy.Down(1.0M));
});

Run("idle state hides from global inactivity and shows on activity", () =>
{
    var cursor = new FakeCursorController();
    var runtime = new CursorRuntime(new CursorStateMachine(TimeSpan.FromSeconds(3)), cursor);

    runtime.ProcessIdle(activityChanged: false, idle: TimeSpan.FromSeconds(2.9));
    runtime.ProcessIdle(activityChanged: false, idle: TimeSpan.FromSeconds(3));
    runtime.ProcessIdle(activityChanged: true, idle: TimeSpan.Zero);

    Equal(1, cursor.HideCount);
    Equal(1, cursor.ShowCount);
});

return failures == 0 ? 0 : 1;

void Run(string name, Action test)
{
    try
    {
        test();
        Console.WriteLine($"PASS {name}");
    }
    catch (Exception exception)
    {
        failures++;
        Console.Error.WriteLine($"FAIL {name}: {exception.Message}");
    }
}

void Equal<T>(T expected, T actual)
{
    if (!EqualityComparer<T>.Default.Equals(expected, actual))
    {
        throw new InvalidOperationException($"Expected {expected}, got {actual}.");
    }
}

void True(bool value)
{
    if (!value)
    {
        throw new InvalidOperationException("Expected true.");
    }
}

void False(bool value)
{
    if (value)
    {
        throw new InvalidOperationException("Expected false.");
    }
}

sealed class FakeCursorController : ICursorController
{
    public int HideCount { get; private set; }
    public int ShowCount { get; private set; }

    public CursorPosition GetPosition() => new(0, 0);
    public uint GetLastInputTick() => 0;
    public void Hide() => HideCount++;
    public void Show() => ShowCount++;
}
