#include "tray.h"

TrayController::~TrayController()
{
    Remove();
}

bool TrayController::Create(HWND dispatcher, Language language, bool enabled)
{
    if (dispatcher == nullptr || _created)
    {
        return false;
    }

    _dispatcher = dispatcher;
    _language = language;
    _enabled = enabled;
    _menu = CreatePopupMenu();
    _icon = LoadIconW(nullptr, IDI_INFORMATION);
    if (_menu == nullptr || _icon == nullptr)
    {
        Remove();
        return false;
    }

    AppendMenuW(_menu, MF_STRING, kSettingsCommand, Localization::Text(_language, StringId::Settings));
    AppendMenuW(_menu, MF_STRING, kPauseCommand,
                Localization::Text(_language, _enabled ? StringId::Pause : StringId::Resume));
    AppendMenuW(_menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(_menu, MF_STRING, kExitCommand, Localization::Text(_language, StringId::Exit));

    _notify.cbSize = sizeof(_notify);
    _notify.hWnd = dispatcher;
    _notify.uID = 1;
    _notify.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    _notify.uCallbackMessage = kTrayMessage;
    _notify.hIcon = _icon;
    lstrcpynW(_notify.szTip, L"AutoMouseCursorHider", ARRAYSIZE(_notify.szTip));
    if (!Shell_NotifyIconW(NIM_ADD, &_notify))
    {
        Remove();
        return false;
    }

    _created = true;
    return true;
}

void TrayController::Remove()
{
    if (_created)
    {
        Shell_NotifyIconW(NIM_DELETE, &_notify);
        _created = false;
    }

    if (_menu != nullptr)
    {
        DestroyMenu(_menu);
        _menu = nullptr;
    }

    _icon = nullptr;
    _dispatcher = nullptr;
}

void TrayController::SetEnabled(bool enabled)
{
    _enabled = enabled;
    RefreshTexts();
}

void TrayController::SetLanguage(Language language)
{
    _language = language;
    RefreshTexts();
}

void TrayController::RefreshTexts()
{
    if (_menu == nullptr) return;
    ModifyMenuW(_menu, kSettingsCommand, MF_BYCOMMAND | MF_STRING, kSettingsCommand,
                Localization::Text(_language, StringId::Settings));
    ModifyMenuW(_menu, kPauseCommand, MF_BYCOMMAND | MF_STRING, kPauseCommand,
                Localization::Text(_language, _enabled ? StringId::Pause : StringId::Resume));
    ModifyMenuW(_menu, kExitCommand, MF_BYCOMMAND | MF_STRING, kExitCommand,
                Localization::Text(_language, StringId::Exit));
}

void TrayController::HandleTrayMessage(LPARAM message)
{
    if (message == WM_RBUTTONUP || message == WM_CONTEXTMENU)
    {
        ShowMenu();
    }
}

void TrayController::ShowMenu()
{
    if (_menu == nullptr || _dispatcher == nullptr)
    {
        return;
    }

    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(_dispatcher);
    TrackPopupMenu(_menu, TPM_RIGHTBUTTON, point.x, point.y, 0, _dispatcher, nullptr);
    PostMessageW(_dispatcher, WM_NULL, 0, 0);
}
