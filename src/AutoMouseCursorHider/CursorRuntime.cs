namespace AutoMouseCursorHider;

public interface ICursorController
{
    CursorPosition GetPosition();
    void Hide();
    void Show();
}

public sealed class CursorRuntime
{
    private CursorStateMachine _state;
    private readonly ICursorController _cursor;
    private readonly object _gate = new();

    public CursorRuntime(CursorStateMachine state, ICursorController cursor)
    {
        _state = state;
        _cursor = cursor;
    }

    public void ProcessSample(CursorPosition position, TimeSpan now)
    {
        ApplyAction(ObserveSample(position, now));
    }

    public void ProcessIdle(bool activityChanged, TimeSpan idle)
    {
        Apply(ObserveIdle(activityChanged, idle));
    }

    public CursorAction ObserveSample(CursorPosition position, TimeSpan now)
    {
        lock (_gate)
        {
            return _state.Observe(position, now);
        }
    }

    public CursorAction ObserveIdle(bool activityChanged, TimeSpan idle)
    {
        lock (_gate)
        {
            return _state.ObserveIdle(activityChanged, idle);
        }
    }

    public void ApplyAction(CursorAction action)
    {
        lock (_gate)
        {
            Apply(action);
        }
    }

    public void SetDelay(TimeSpan delay)
    {
        lock (_gate)
        {
            _state = new CursorStateMachine(delay);
        }
    }

    public void Restore()
    {
        lock (_gate)
        {
            Apply(_state.Restore());
        }
    }

    private void Apply(CursorAction action)
    {
        if (action == CursorAction.Hide)
        {
            _cursor.Hide();
        }
        else if (action == CursorAction.Show)
        {
            _cursor.Show();
        }
    }
}
