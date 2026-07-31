namespace AutoMouseCursorHider;

public readonly record struct CursorPosition(int X, int Y);

public enum CursorAction
{
    None,
    Hide,
    Show
}

public sealed class CursorStateMachine
{
    private readonly TimeSpan _delay;
    private bool _hasPosition;
    private bool _hidden;
    private CursorPosition _lastPosition;
    private TimeSpan _lastMovement;

    public CursorStateMachine(TimeSpan delay)
    {
        if (delay < TimeSpan.FromMilliseconds(100) || delay > TimeSpan.FromHours(1))
        {
            throw new ArgumentOutOfRangeException(nameof(delay));
        }

        _delay = delay;
    }

    public CursorAction Observe(CursorPosition position, TimeSpan now)
    {
        if (!_hasPosition)
        {
            _hasPosition = true;
            _lastPosition = position;
            _lastMovement = now;
            return CursorAction.None;
        }

        if (position != _lastPosition)
        {
            _lastPosition = position;
            _lastMovement = now;

            if (_hidden)
            {
                _hidden = false;
                return CursorAction.Show;
            }

            return CursorAction.None;
        }

        if (!_hidden && now - _lastMovement >= _delay)
        {
            _hidden = true;
            return CursorAction.Hide;
        }

        return CursorAction.None;
    }

    public CursorAction Restore()
    {
        if (!_hidden)
        {
            return CursorAction.None;
        }

        _hidden = false;
        return CursorAction.Show;
    }
}
