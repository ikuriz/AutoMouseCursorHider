using System.Globalization;

namespace AutoMouseCursorHider;

public enum AppCommandKind
{
    Run,
    Startup,
    RemoveStartup,
    Stop,
    Help
}

public sealed record AppCommand(AppCommandKind Kind, TimeSpan? Delay);

public static class CommandLine
{
    public static bool TryParse(string[] args, out AppCommand command, out string error)
    {
        AppCommandKind? kind = null;
        TimeSpan? delay = null;

        command = new AppCommand(AppCommandKind.Run, null);
        error = string.Empty;

        for (var index = 0; index < args.Length; index++)
        {
            var argument = args[index];

            if (argument == "--delay")
            {
                if (delay is not null || ++index == args.Length ||
                    !TryParseDelay(args[index], out var parsedDelay))
                {
                    error = "--delay must be a number from 0.1 to 3600.";
                    return false;
                }

                delay = parsedDelay;
                continue;
            }

            var parsedKind = argument switch
            {
                "--startup" => AppCommandKind.Startup,
                "--no-startup" => AppCommandKind.RemoveStartup,
                "--stop" => AppCommandKind.Stop,
                "--help" => AppCommandKind.Help,
                _ => (AppCommandKind?)null
            };

            if (parsedKind is null || kind is not null)
            {
                error = $"Unknown or conflicting option: {argument}";
                return false;
            }

            kind = parsedKind;
        }

        if (delay is not null && kind is not null and not AppCommandKind.Startup)
        {
            error = "--delay can only be used while starting or with --startup.";
            return false;
        }

        command = new AppCommand(kind ?? AppCommandKind.Run, delay);
        return true;
    }

    private static bool TryParseDelay(string value, out TimeSpan delay)
    {
        delay = default;

        if (!double.TryParse(value, NumberStyles.AllowDecimalPoint, CultureInfo.InvariantCulture, out var seconds) ||
            !double.IsFinite(seconds) || seconds < 0.1 || seconds > 3600)
        {
            return false;
        }

        delay = TimeSpan.FromSeconds(seconds);
        return true;
    }
}
