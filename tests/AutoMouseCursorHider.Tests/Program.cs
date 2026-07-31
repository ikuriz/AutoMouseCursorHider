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
