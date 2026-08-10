#include "cursor_manager.h"
#include "cursor_state.h"
#include "config_store.h"
#include "instance_lock.h"
#include "mouse_monitor.h"
#include "settings_dialog.h"
#include "startup_registration.h"
#include "tray.h"

#include <windows.h>

#include <cwchar>

namespace
{
constexpr wchar_t kWindowClass[] = L"AutoMouseCursorHider.Native.Dispatcher.v2";
constexpr UINT_PTR kTimerId = 1;
constexpr UINT kTimerIntervalMs = 100;
constexpr UINT kShowSettingsMessage = WM_APP + 10;

struct AppContext
{
    HWND dispatcher = nullptr;
    CursorManager cursorManager;
    CursorState cursorState;
    MouseMonitor mouseMonitor;
    TrayController tray;
    SettingsDialog settingsDialog;
    AppSettings settings;
    bool secureDesktopActive = false;
};

bool IsSecureInputDesktop()
{
    HDESK desktop = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (desktop == nullptr)
    {
        // A desktop that cannot be opened is not safe to treat as the user's
        // interactive desktop. Fail safe: keep the cursor visible.
        return true;
    }

    wchar_t name[64]{};
    DWORD required = 0;
    const bool read = GetUserObjectInformationW(desktop, UOI_NAME, name, sizeof(name), &required) != FALSE;
    CloseDesktop(desktop);
    if (!read)
    {
        return true;
    }

    return _wcsicmp(name, L"Default") != 0;
}

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

    const bool secureDesktop = IsSecureInputDesktop();
    if (secureDesktop)
    {
        if (!app.secureDesktopActive)
        {
            app.secureDesktopActive = true;
            app.cursorState.Pause();
            app.cursorManager.RestoreAndRefresh();
        }
        return;
    }

    if (app.secureDesktopActive)
    {
        app.secureDesktopActive = false;
        if (app.settings.enabled)
        {
            app.cursorState.Resume(now);
        }
        else
        {
            app.cursorState.Pause();
        }
        app.cursorState.OnActivity(now);
    }

    if (!app.settings.enabled)
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

bool IsInteractiveLaunch(PCWSTR commandLine)
{
    return commandLine == nullptr || *commandLine == L'\0';
}

void ShowSettings(AppContext& app)
{
    AppSettings updated = app.settings;
    const auto result = app.settingsDialog.ShowModal(app.dispatcher, updated);
    if (result == SettingsDialog::Result::Exit)
    {
        DestroyWindow(app.dispatcher);
        return;
    }
    if (result != SettingsDialog::Result::Accepted)
    {
        return;
    }

    const bool oldEnabled = app.settings.enabled;
    wchar_t executablePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executablePath, ARRAYSIZE(executablePath));
    const bool startupChanged = updated.startupEnabled
        ? StartupRegistration::Enable(executablePath)
        : StartupRegistration::Disable();
    if (!startupChanged || !ConfigStore::Save(updated))
    {
        const auto language = Localization::Resolve(updated.language);
        MessageBoxW(app.dispatcher, Localization::Text(language, StringId::SaveFailed),
                    Localization::Text(language, StringId::AppName), MB_ICONERROR);
        return;
    }

    if (updated.enabled != oldEnabled)
    {
        if (updated.enabled)
        {
            app.cursorState.Resume(GetTickCount64());
        }
        else
        {
            app.cursorState.Pause();
            app.cursorManager.RestoreAndRefresh();
        }
    }
    app.settings = updated;
    app.cursorState.SetDelay(updated.delaySeconds);
    app.tray.SetLanguage(Localization::Resolve(updated.language));
    app.tray.SetEnabled(app.settings.enabled);
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
    case kShowSettingsMessage:
        ShowSettings(*app);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case TrayController::kPauseCommand:
            app->settings.enabled = !app->settings.enabled;
            if (!app->settings.enabled)
            {
                app->cursorState.Pause();
                app->cursorManager.RestoreAndRefresh();
            }
            else
            {
                app->cursorState.Resume(GetTickCount64());
            }
            ConfigStore::Save(app->settings);
            app->tray.SetEnabled(app->settings.enabled);
            return 0;
        case TrayController::kExitCommand:
            DestroyWindow(window);
            return 0;
        case TrayController::kSettingsCommand:
            ShowSettings(*app);
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

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR commandLine, int)
{
    const bool interactiveLaunch = IsInteractiveLaunch(commandLine);
    InstanceLock instanceLock;
    if (!instanceLock.Acquire())
    {
        if (interactiveLaunch)
        {
            if (const HWND existing = FindWindowExW(HWND_MESSAGE, nullptr, kWindowClass, nullptr))
            {
                PostMessageW(existing, kShowSettingsMessage, 0, 0);
            }
        }
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
    if (!app.settings.enabled)
    {
        app.cursorState.Pause();
    }
    app.cursorState.OnActivity(GetTickCount64());
    app.dispatcher = CreateWindowExW(
        0, kWindowClass, L"AutoMouseCursorHider", 0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, instance, &app);
    if (app.dispatcher == nullptr || !app.mouseMonitor.Install(app.dispatcher) ||
        !app.tray.Create(app.dispatcher, Localization::Resolve(app.settings.language), app.settings.enabled))
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

    if (interactiveLaunch)
    {
        PostMessageW(app.dispatcher, kShowSettingsMessage, 0, 0);
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
