namespace AutoMouseCursorHider;

public interface ICursorController
{
    CursorPosition GetPosition();
    uint GetLastInputTick();
    void Hide();
    void Show();
}

public sealed class CursorRuntime
{
    private CursorStateMachine _state;
    private readonly ICursorController _cursor;

    public CursorRuntime(CursorStateMachine state, ICursorController cursor)
    {
        _state = state;
        _cursor = cursor;
    }

    public void ProcessSample(CursorPosition position, TimeSpan now)
    {
        Apply(_state.Observe(position, now));
    }

    public void ProcessIdle(bool activityChanged, TimeSpan idle)
    {
        Apply(_state.ObserveIdle(activityChanged, idle));
    }

    public void SetDelay(TimeSpan delay)
    {
        _state = new CursorStateMachine(delay);
    }

    public void Restore()
    {
        Apply(_state.Restore());
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
