#include "settings_dialog.h"

#include <commctrl.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
constexpr wchar_t kClassName[] = L"AutoMouseCursorHider.Native.Settings.v2";
constexpr int kEditId = 5001;
constexpr int kUpDownId = 5002;
constexpr int kStartupId = 5003;
constexpr int kUnitId = 5004;
constexpr int kSubtitleId = 5005;
constexpr int kTitleId = 5006;
}

bool SettingsDialog::ShowModal(HWND owner, AppSettings& settings)
{
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_UPDOWN_CLASS};
    InitCommonControlsEx(&controls);

    WNDCLASSW klass{};
    klass.lpfnWndProc = WindowProc;
    klass.hInstance = GetModuleHandleW(nullptr);
    klass.lpszClassName = kClassName;
    klass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    klass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    RegisterClassW(&klass);

    POINT cursorPoint{};
    GetCursorPos(&cursorPoint);
    HMONITOR monitor = MonitorFromPoint(cursorPoint, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    GetMonitorInfoW(monitor, &monitorInfo);
    constexpr int width = 560;
    constexpr int height = 360;
    const int x = monitorInfo.rcWork.left + ((monitorInfo.rcWork.right - monitorInfo.rcWork.left) - width) / 2;
    const int y = monitorInfo.rcWork.top + ((monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) - height) / 2;

    _settings = &settings;
    _accepted = false;
    _window = CreateWindowExW(WS_EX_DLGMODALFRAME, kClassName, L"AutoMouseCursorHider - Settings",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, x, y,
                              width, height, owner, nullptr, klass.hInstance, this);
    if (_window == nullptr)
    {
        _settings = nullptr;
        return false;
    }

    EnableWindow(owner, FALSE);
    ShowWindow(_window, SW_SHOW);
    UpdateWindow(_window);
    MSG message{};
    while (IsWindow(_window) && GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageW(_window, &message))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    _settings = nullptr;
    return _accepted;
}

void SettingsDialog::UpdateValue(HWND edit, double delta)
{
    double value = 3.0;
    ReadValue(edit, value);
    value = StepDelayValue(value, delta > 0.0 ? 1 : -1);
    std::wostringstream text;
    text << std::fixed << std::setprecision(1) << value;
    SetWindowTextW(edit, text.str().c_str());
}

double SettingsDialog::StepDelayValue(double currentSeconds, int direction)
{
    return StepDelay(currentSeconds, direction);
}

bool SettingsDialog::ReadValue(HWND edit, double& value)
{
    wchar_t buffer[64]{};
    GetWindowTextW(edit, buffer, ARRAYSIZE(buffer));
    return ConfigStore::TryParseDelay(buffer, value);
}

void SettingsDialog::DrawButton(const DRAWITEMSTRUCT& draw)
{
    const bool checkbox = draw.CtlID == kStartupId;
    const bool pressed = (draw.itemState & ODS_SELECTED) != 0;
    HBRUSH background = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(draw.hDC, &draw.rcItem, background);
    DeleteObject(background);

    if (checkbox)
    {
        RECT box{draw.rcItem.left, draw.rcItem.top + 5, draw.rcItem.left + 20, draw.rcItem.top + 25};
        HBRUSH boxBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(draw.hDC, &box, boxBrush);
        DeleteObject(boxBrush);
        FrameRect(draw.hDC, &box, GetSysColorBrush(COLOR_GRAYTEXT));
        if (SendMessageW(draw.hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(35, 110, 220));
            const auto oldPen = SelectObject(draw.hDC, pen);
            MoveToEx(draw.hDC, box.left + 4, box.top + 10, nullptr);
            LineTo(draw.hDC, box.left + 8, box.top + 14);
            LineTo(draw.hDC, box.left + 16, box.top + 5);
            SelectObject(draw.hDC, oldPen);
            DeleteObject(pen);
        }
        RECT text = draw.rcItem;
        text.left += 30;
        SetBkMode(draw.hDC, TRANSPARENT);
        DrawTextW(draw.hDC, L"Start with Windows", -1, &text, DT_SINGLELINE | DT_VCENTER);
        return;
    }

    const COLORREF fill = draw.CtlID == IDOK ? RGB(35, 110, 220) : RGB(245, 246, 248);
    const COLORREF textColor = draw.CtlID == IDOK ? RGB(255, 255, 255) : RGB(40, 45, 52);
    RECT button = draw.rcItem;
    InflateRect(&button, -1, -1);
    HBRUSH buttonBrush = CreateSolidBrush(pressed ? RGB(25, 90, 190) : fill);
    FillRect(draw.hDC, &button, buttonBrush);
    DeleteObject(buttonBrush);
    FrameRect(draw.hDC, &button, CreateSolidBrush(RGB(210, 214, 220)));
    SetTextColor(draw.hDC, textColor);
    SetBkMode(draw.hDC, TRANSPARENT);
    wchar_t label[32]{};
    GetWindowTextW(draw.hwndItem, label, ARRAYSIZE(label));
    DrawTextW(draw.hDC, label, -1, &button, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

LRESULT CALLBACK SettingsDialog::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* dialog = reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        dialog = static_cast<SettingsDialog*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        dialog->_window = window;
        dialog->_titleFont = CreateFontW(24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        dialog->_bodyFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        return TRUE;
    }
    if (dialog == nullptr)
    {
        return DefWindowProcW(window, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_CREATE:
        {
            dialog->_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"3.0",
                                             WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 270, 132, 150, 32,
                                             window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditId)), nullptr, nullptr);
            dialog->_upDown = CreateWindowExW(0, UPDOWN_CLASSW, nullptr,
                                               WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS, 420, 132, 28, 32,
                                               window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kUpDownId)), nullptr, nullptr);
            dialog->_startup = CreateWindowExW(0, L"BUTTON", nullptr,
                                                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_OWNERDRAW, 48, 245, 300, 30,
                                                window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStartupId)), nullptr, nullptr);
            HWND title = CreateWindowExW(0, L"STATIC", L"AutoMouseCursorHider", WS_CHILD | WS_VISIBLE,
                                         32, 24, 480, 34, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTitleId)), nullptr, nullptr);
            HWND subtitle = CreateWindowExW(0, L"STATIC", L"Configure the idle time before the cursor is hidden.", WS_CHILD | WS_VISIBLE,
                                            34, 61, 480, 24, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSubtitleId)), nullptr, nullptr);
            HWND label = CreateWindowExW(0, L"STATIC", L"Hide after", WS_CHILD | WS_VISIBLE,
                                         52, 141, 180, 24, window, nullptr, nullptr, nullptr);
            HWND unit = CreateWindowExW(0, L"STATIC", L"seconds", WS_CHILD | WS_VISIBLE,
                                        458, 141, 80, 24, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kUnitId)), nullptr, nullptr);
            HWND ok = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                      350, 292, 86, 36, window, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            HWND cancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                          446, 292, 86, 36, window, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
            for (HWND control : {dialog->_edit, dialog->_upDown, title, subtitle, label, unit, ok, cancel})
            {
                SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(dialog->_bodyFont), TRUE);
            }
            SendMessageW(title, WM_SETFONT, reinterpret_cast<WPARAM>(dialog->_titleFont), TRUE);
            std::wostringstream text;
            text << std::fixed << std::setprecision(1) << dialog->_settings->delaySeconds;
            SetWindowTextW(dialog->_edit, text.str().c_str());
            SendMessageW(dialog->_startup, BM_SETCHECK,
                         dialog->_settings->startupEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            RECT card{24, 96, 536, 224};
            HBRUSH cardBrush = CreateSolidBrush(RGB(247, 248, 250));
            FillRect(dc, &card, cardBrush);
            DeleteObject(cardBrush);
            FrameRect(dc, &card, GetSysColorBrush(COLOR_3DLIGHT));
            EndPaint(window, &paint);
        }
        return 0;
    case WM_DRAWITEM:
        DrawButton(*reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
        return TRUE;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
        {
            const auto dc = reinterpret_cast<HDC>(wParam);
            SetBkColor(dc, GetSysColor(COLOR_WINDOW));
            SetBkMode(dc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        }
    case WM_NOTIFY:
        if (reinterpret_cast<const NMHDR*>(lParam)->idFrom == kUpDownId &&
            reinterpret_cast<const NMHDR*>(lParam)->code == UDN_DELTAPOS)
        {
            const auto* change = reinterpret_cast<const NMUPDOWN*>(lParam);
            UpdateValue(dialog->_edit, change->iDelta < 0 ? 0.5 : -0.5);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            double delay = 0.0;
            if (!ReadValue(dialog->_edit, delay))
            {
                MessageBoxW(window, L"Enter a value from 0.1 to 3600 seconds.", L"Invalid value", MB_ICONWARNING);
                return 0;
            }
            dialog->_settings->delaySeconds = delay;
            dialog->_settings->startupEnabled = SendMessageW(dialog->_startup, BM_GETCHECK, 0, 0) == BST_CHECKED;
            dialog->_accepted = true;
            DestroyWindow(window);
            return 0;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        if (dialog->_titleFont != nullptr) DeleteObject(dialog->_titleFont);
        if (dialog->_bodyFont != nullptr) DeleteObject(dialog->_bodyFont);
        dialog->_titleFont = nullptr;
        dialog->_bodyFont = nullptr;
        dialog->_window = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

