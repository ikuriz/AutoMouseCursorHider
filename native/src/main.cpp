#include "cursor_manager.h"
#include "cursor_state.h"
#include "mouse_monitor.h"

#include <windows.h>

namespace
{
constexpr wchar_t kWindowClass[] = L"AutoMouseCursorHider.Native.Dispatcher.v2";
constexpr UINT_PTR kTimerId = 1;
constexpr UINT kTimerIntervalMs = 100;

struct AppContext
{
    HWND dispatcher = nullptr;
    CursorManager cursorManager;
    CursorState cursorState;
    MouseMonitor mouseMonitor;
};

void HandleActivity(AppContext& app)
{
    app.mouseMonitor.AcknowledgeActivity();
    const auto now = GetTickCount64();
    app.cursorState.OnActivity(now);
    if (app.cursorManager.IsHidden() || app.cursorManager.IsRestorePending())
    {
        app.cursorManager.RestoreAndRefresh();
    }
}

void HandleTimer(AppContext& app)
{
    const auto now = GetTickCount64();
    if (app.cursorManager.IsRestorePending())
    {
        app.cursorManager.RetryRestore();
    }

    app.cursorState.OnTimer(now);
    if (app.cursorState.ShouldHide() && !app.cursorManager.IsHidden())
    {
        if (!app.cursorManager.Hide())
        {
            app.cursorState.OnActivity(now);
        }
    }
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* app = reinterpret_cast<AppContext*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        app = static_cast<AppContext*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        app->dispatcher = window;
        return TRUE;
    }

    if (app == nullptr)
    {
        return DefWindowProcW(window, message, wParam, lParam);
    }

    switch (message)
    {
    case MouseMonitor::kActivityMessage:
        HandleActivity(*app);
        return 0;
    case WM_TIMER:
        if (wParam == kTimerId)
        {
            HandleTimer(*app);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, kTimerId);
        app->mouseMonitor.Uninstall();
        app->cursorManager.RestoreAndRefresh();
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

bool RegisterDispatcherClass(HINSTANCE instance)
{
    WNDCLASSEXW klass{};
    klass.cbSize = sizeof(klass);
    klass.hInstance = instance;
    klass.lpfnWndProc = WindowProc;
    klass.lpszClassName = kWindowClass;
    return RegisterClassExW(&klass) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    if (!RegisterDispatcherClass(instance))
    {
        return 1;
    }

    AppContext app;
    app.cursorManager.RestoreAndRefresh();
    app.cursorState.OnActivity(GetTickCount64());
    app.dispatcher = CreateWindowExW(
        0, kWindowClass, L"AutoMouseCursorHider", 0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, instance, &app);
    if (app.dispatcher == nullptr || !app.mouseMonitor.Install(app.dispatcher))
    {
        if (app.dispatcher != nullptr)
        {
            DestroyWindow(app.dispatcher);
        }
        return 1;
    }

    if (SetTimer(app.dispatcher, kTimerId, kTimerIntervalMs, nullptr) == 0)
    {
        DestroyWindow(app.dispatcher);
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
