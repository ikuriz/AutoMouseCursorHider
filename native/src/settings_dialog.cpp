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
    RegisterClassW(&klass);

    _settings = &settings;
    _accepted = false;
    _window = CreateWindowExW(WS_EX_DLGMODALFRAME, kClassName, L"AutoMouseCursorHider Settings",
                              WS_CAPTION | WS_SYSMENU | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                              360, 180, owner, nullptr, klass.hInstance, this);
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
    value = std::clamp(value + delta, 0.1, 3600.0);
    value = std::round(value * 10.0) / 10.0;
    std::wostringstream text;
    text << std::fixed << std::setprecision(1) << value;
    SetWindowTextW(edit, text.str().c_str());
}

bool SettingsDialog::ReadValue(HWND edit, double& value)
{
    wchar_t buffer[64]{};
    GetWindowTextW(edit, buffer, ARRAYSIZE(buffer));
    return ConfigStore::TryParseDelay(buffer, value);
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
                                             WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 150, 28, 120, 26,
                                             window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditId)), nullptr, nullptr);
            dialog->_upDown = CreateWindowExW(0, UPDOWN_CLASSW, nullptr,
                                               WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS, 270, 28, 24, 26,
                                               window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kUpDownId)), nullptr, nullptr);
            dialog->_startup = CreateWindowExW(0, L"BUTTON", L"Start with Windows",
                                                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 30, 70, 230, 24,
                                                window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStartupId)), nullptr, nullptr);
            CreateWindowExW(0, L"STATIC", L"Hide after seconds:", WS_CHILD | WS_VISIBLE,
                            30, 33, 115, 20, window, nullptr, nullptr, nullptr);
            CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                            185, 112, 75, 28, window, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE,
                            270, 112, 75, 28, window, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
            std::wostringstream text;
            text << std::fixed << std::setprecision(1) << dialog->_settings->delaySeconds;
            SetWindowTextW(dialog->_edit, text.str().c_str());
            SendMessageW(dialog->_startup, BM_SETCHECK,
                         dialog->_settings->startupEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        return 0;
    case WM_VSCROLL:
        if (reinterpret_cast<HWND>(lParam) == dialog->_upDown)
        {
            UpdateValue(dialog->_edit, LOWORD(wParam) == SB_LINEUP ? 0.5 : -0.5);
            return 0;
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
    default:
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
