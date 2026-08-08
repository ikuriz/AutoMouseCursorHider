using System.Globalization;

namespace AutoMouseCursorHider;

public readonly record struct SettingsDraft(TimeSpan Delay, bool StartupEnabled);

public partial class SettingsForm : Form
{
    private readonly Func<SettingsDraft, string?> _applySettings;

    public SettingsForm(TimeSpan delay, bool startupEnabled, bool paused, Func<SettingsDraft, string?> applySettings)
    {
        _applySettings = applySettings ?? throw new ArgumentNullException(nameof(applySettings));
        InitializeComponent();
        _delayInput.Text = delay.TotalSeconds.ToString("0.##", CultureInfo.CurrentCulture);
        _startupCheckBox.Checked = startupEnabled;
        _statusLabel.Text = paused ? "状态：已暂停" : "状态：运行中";
        _applyButton.Click += ApplyButton_Click;
        _decreaseButton.Click += (_, _) => StepDelay(-1);
        _increaseButton.Click += (_, _) => StepDelay(1);
    }

    private void StepDelay(int direction)
    {
        if (!SettingsValidation.TryParseDelay(_delayInput.Text, out var delay, out _))
        {
            delay = TimeSpan.FromSeconds(3);
        }

        var value = (decimal)delay.TotalSeconds;
        var next = direction < 0 ? DelayStepPolicy.Down(value) : DelayStepPolicy.Up(value);
        _delayInput.Text = next.ToString("0.##", CultureInfo.CurrentCulture);
        _delayInput.SelectionStart = _delayInput.Text.Length;
    }

    private void ApplyButton_Click(object? sender, EventArgs e)
    {
        if (!SettingsValidation.TryParseDelay(_delayInput.Text, out var delay, out var error))
        {
            _errorLabel.Text = error;
            return;
        }

        try
        {
            var applyError = _applySettings(new SettingsDraft(delay, _startupCheckBox.Checked));
            if (!string.IsNullOrEmpty(applyError))
            {
                _errorLabel.Text = applyError;
                return;
            }

            _errorLabel.Text = string.Empty;
            DialogResult = DialogResult.OK;
            Close();
        }
        catch (Exception exception)
        {
            _errorLabel.Text = exception.Message;
        }
    }
}
