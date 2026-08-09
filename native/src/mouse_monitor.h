#pragma once

#include <windows.h>

class MouseMonitor
{
public:
    static constexpr UINT kActivityMessage = WM_APP + 1;

    MouseMonitor() = default;
    MouseMonitor(const MouseMonitor&) = delete;
    MouseMonitor& operator=(const MouseMonitor&) = delete;
    ~MouseMonitor();

    bool Install(HWND dispatcher);
    void Uninstall();
    void AcknowledgeActivity();

private:
    static LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);

    HHOOK _hook = nullptr;
    HWND _dispatcher = nullptr;
    volatile LONG _activityPosted = 0;
    static MouseMonitor* _current;
};
