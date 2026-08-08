using System.Globalization;

namespace AutoMouseCursorHider;

public static class SettingsValidation
{
    public static bool TryParseDelay(string? text, out TimeSpan delay, out string error)
    {
        delay = default;
        error = string.Empty;

        if (!double.TryParse(text, NumberStyles.Float, CultureInfo.CurrentCulture, out var seconds) &&
            !double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out seconds))
        {
            error = "请输入有效的数字。";
            return false;
        }

        if (double.IsNaN(seconds) || double.IsInfinity(seconds) || seconds < 0.1 || seconds > 3600)
        {
            error = "隐藏时长必须在 0.1 到 3600 秒之间。";
            return false;
        }

        delay = TimeSpan.FromSeconds(seconds);
        return true;
    }
}
