#include "cursor_manager.h"
#include "cursor_state.h"
#include "config_store.h"
#include "instance_lock.h"
#include "mouse_monitor.h"
#include "settings_dialog.h"
#include "startup_registration.h"
#include "tray.h"

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
    TrayController tray;
    SettingsDialog settingsDialog;
    AppSettings settings;
    bool autoHideEnabled = true;
    bool paused = false;
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
    if (!app.autoHideEnabled)
    {
        app.cursorManager.RestoreAndRefresh();
        return;
    }

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
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case TrayController::kPauseCommand:
            app->paused = !app->paused;
            if (app->paused)
            {
                app->cursorState.Pause();
                app->cursorManager.RestoreAndRefresh();
            }
            else
            {
                app->cursorState.Resume(GetTickCount64());
            }
            app->tray.SetPaused(app->paused);
            return 0;
        case TrayController::kExitCommand:
            DestroyWindow(window);
            return 0;
        case TrayController::kSettingsCommand:
            {
                AppSettings updated = app->settings;
                if (app->settingsDialog.ShowModal(window, updated))
                {
                    wchar_t executablePath[MAX_PATH]{};
                    GetModuleFileNameW(nullptr, executablePath, ARRAYSIZE(executablePath));
                    const bool startupChanged = updated.startupEnabled
                        ? StartupRegistration::Enable(executablePath)
                        : StartupRegistration::Disable();
                    if (!startupChanged || !ConfigStore::Save(updated))
                    {
                        const auto language = Localization::Resolve(updated.language);
                        MessageBoxW(window, Localization::Text(language, StringId::SaveFailed),
                                    Localization::Text(language, StringId::AppName), MB_ICONERROR);
                    }
                    else
                    {
                        if (updated.enabled != app->autoHideEnabled)
                        {
                            app->autoHideEnabled = updated.enabled;
                            if (app->autoHideEnabled)
                            {
                                app->cursorState.Resume(GetTickCount64());
                            }
                            else
                            {
                                app->cursorState.Pause();
                                app->cursorManager.RestoreAndRefresh();
                            }
                        }
                        app->settings = updated;
                        app->cursorState.SetDelay(updated.delaySeconds);
                        app->tray.SetLanguage(Localization::Resolve(updated.language));
                    }
                }
            }
            return 0;
        default:
            break;
        }
        break;
    case TrayController::kTrayMessage:
        app->tray.HandleTrayMessage(lParam);
        return 0;
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
    InstanceLock instanceLock;
    if (!instanceLock.Acquire())
    {
        return 0;
    }

    if (!RegisterDispatcherClass(instance))
    {
        return 1;
    }

    AppContext app;
    app.settings = ConfigStore::Load();
    app.settings.startupEnabled = StartupRegistration::IsEnabled();
    app.cursorManager.RestoreAndRefresh();
    app.cursorState.SetDelay(app.settings.delaySeconds);
    app.autoHideEnabled = app.settings.enabled;
    if (!app.autoHideEnabled)
    {
        app.cursorState.Pause();
    }
    app.cursorState.OnActivity(GetTickCount64());
    app.dispatcher = CreateWindowExW(
        0, kWindowClass, L"AutoMouseCursorHider", 0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, instance, &app);
    if (app.dispatcher == nullptr || !app.mouseMonitor.Install(app.dispatcher) ||
        !app.tray.Create(app.dispatcher, Localization::Resolve(app.settings.language)))
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
