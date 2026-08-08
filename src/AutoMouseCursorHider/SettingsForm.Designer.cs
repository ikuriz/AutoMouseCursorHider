using System.Drawing;
using System.Windows.Forms;

namespace AutoMouseCursorHider;

public partial class SettingsForm
{
    private Label _statusLabel = null!;
    private Label _delayLabel = null!;
    private NumericUpDown _delayInput = null!;
    private CheckBox _startupCheckBox = null!;
    private Label _errorLabel = null!;
    private Button _applyButton = null!;
    private Button _cancelButton = null!;

    private void InitializeComponent()
    {
        Text = "AutoMouseCursorHider 设置";
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        ClientSize = new Size(360, 210);

        _statusLabel = new Label { AutoSize = true, Location = new Point(20, 18), Text = "状态：运行中" };
        _delayLabel = new Label { AutoSize = true, Location = new Point(20, 58), Text = "隐藏时长（秒）：" };
        _delayInput = new NumericUpDown
        {
            Location = new Point(145, 54),
            Size = new Size(150, 23),
            DecimalPlaces = 2,
            Increment = 0.5M,
            Minimum = 0.1M,
            Maximum = 3600M,
            ThousandsSeparator = false
        };
        _startupCheckBox = new CheckBox { AutoSize = true, Location = new Point(20, 95), Text = "开机自动启动" };
        _errorLabel = new Label { AutoSize = false, ForeColor = Color.Firebrick, Location = new Point(20, 125), Size = new Size(320, 32) };
        _applyButton = new Button { DialogResult = DialogResult.None, Location = new Point(175, 170), Size = new Size(80, 25), Text = "应用" };
        _cancelButton = new Button { DialogResult = DialogResult.Cancel, Location = new Point(265, 170), Size = new Size(80, 25), Text = "取消" };

        Controls.AddRange([_statusLabel, _delayLabel, _delayInput, _startupCheckBox, _errorLabel, _applyButton, _cancelButton]);
        AcceptButton = _applyButton;
        CancelButton = _cancelButton;
    }
}
