#pragma once

#include <windows.h>
#include <shellapi.h>

class TrayController
{
public:
    static constexpr UINT kSettingsCommand = 40001;
    static constexpr UINT kPauseCommand = 40002;
    static constexpr UINT kExitCommand = 40003;
    static constexpr UINT kTrayMessage = WM_APP + 2;

    TrayController() = default;
    TrayController(const TrayController&) = delete;
    TrayController& operator=(const TrayController&) = delete;
    ~TrayController();

    bool Create(HWND dispatcher);
    void Remove();
    void SetPaused(bool paused);
    void HandleTrayMessage(LPARAM message);

private:
    void ShowMenu();

    HWND _dispatcher = nullptr;
    HMENU _menu = nullptr;
    HICON _icon = nullptr;
    NOTIFYICONDATAW _notify{};
    bool _created = false;
};
