namespace AutoMouseCursorHider;

public static class DelayStepPolicy
{
    private const decimal MinimumTypedValue = 0.1M;
    private const decimal SpinnerStep = 0.5M;

    public static decimal Down(decimal value)
    {
        return value <= SpinnerStep ? MinimumTypedValue : value - SpinnerStep;
    }

    public static decimal Up(decimal value)
    {
        return value <= MinimumTypedValue ? SpinnerStep : value + SpinnerStep;
    }
}
