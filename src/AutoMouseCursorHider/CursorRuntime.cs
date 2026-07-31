namespace AutoMouseCursorHider;

public interface ICursorController
{
    CursorPosition GetPosition();
    void Hide();
    void Show();
}

public sealed class CursorRuntime
{
    private readonly CursorStateMachine _state;
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
