#include "mouse_monitor.h"

MouseMonitor* MouseMonitor::_current = nullptr;

namespace
{
bool IsActivityMessage(WPARAM message)
{
    return message == WM_MOUSEMOVE ||
           message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
           message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN ||
           message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
}
}

MouseMonitor::~MouseMonitor()
{
    Uninstall();
}

bool MouseMonitor::Install(HWND dispatcher)
{
    if (dispatcher == nullptr || _hook != nullptr)
    {
        return false;
    }

    _dispatcher = dispatcher;
    _current = this;
    _hook = SetWindowsHookExW(WH_MOUSE_LL, HookProc, nullptr, 0);
    if (_hook == nullptr)
    {
        _current = nullptr;
        _dispatcher = nullptr;
        return false;
    }

    return true;
}

void MouseMonitor::Uninstall()
{
    if (_current == this)
    {
        _current = nullptr;
    }

    if (_hook != nullptr)
    {
        UnhookWindowsHookEx(_hook);
        _hook = nullptr;
    }

    _dispatcher = nullptr;
    InterlockedExchange(&_activityPosted, 0);
}

void MouseMonitor::AcknowledgeActivity()
{
    InterlockedExchange(&_activityPosted, 0);
}

LRESULT CALLBACK MouseMonitor::HookProc(int code, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    if (code >= 0 && _current != nullptr && IsActivityMessage(wParam))
    {
        if (InterlockedExchange(&_current->_activityPosted, 1) == 0)
        {
            if (!PostMessageW(_current->_dispatcher, kActivityMessage, 0, 0))
            {
                InterlockedExchange(&_current->_activityPosted, 0);
            }
        }
    }

    return CallNextHookEx(nullptr, code, wParam, lParam);
}
