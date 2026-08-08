namespace AutoMouseCursorHider;

public sealed class RuntimeState
{
    private readonly CursorRuntime _runtime;
    private TimeSpan _delay;

    public RuntimeState(CursorRuntime runtime, TimeSpan delay)
    {
        _runtime = runtime ?? throw new ArgumentNullException(nameof(runtime));
        ValidateDelay(delay);
        _delay = delay;
    }

    public bool IsPaused { get; private set; }
    public TimeSpan Delay => _delay;

    public event EventHandler? StatusChanged;

    public void Pause()
    {
        if (IsPaused)
        {
            return;
        }

        IsPaused = true;
        _runtime.Restore();
        StatusChanged?.Invoke(this, EventArgs.Empty);
    }

    public void Resume()
    {
        if (!IsPaused)
        {
            return;
        }

        IsPaused = false;
        _runtime.SetDelay(_delay);
        StatusChanged?.Invoke(this, EventArgs.Empty);
    }

    public void SetDelay(TimeSpan delay)
    {
        ValidateDelay(delay);
        _delay = delay;
        _runtime.SetDelay(delay);
        StatusChanged?.Invoke(this, EventArgs.Empty);
    }

    public void Restore() => _runtime.Restore();

    private static void ValidateDelay(TimeSpan delay)
    {
        if (delay < TimeSpan.FromMilliseconds(100) || delay > TimeSpan.FromHours(1))
        {
            throw new ArgumentOutOfRangeException(nameof(delay));
        }
    }
}
