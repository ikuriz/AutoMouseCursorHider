#include "startup_registration.h"

#include <windows.h>

namespace
{
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kValueName[] = L"AutoMouseCursorHider";
}

bool StartupRegistration::IsEnabled()
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
    {
        return false;
    }
    DWORD type = 0;
    DWORD bytes = 0;
    const auto result = RegQueryValueExW(key, kValueName, nullptr, &type, nullptr, &bytes);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ) && bytes > sizeof(wchar_t);
}

bool StartupRegistration::Enable(const std::wstring& executablePath)
{
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
    {
        return false;
    }
    const auto quoted = L"\"" + executablePath + L"\" --startup";
    const auto result = RegSetValueExW(key, kValueName, 0, REG_SZ,
                                       reinterpret_cast<const BYTE*>(quoted.c_str()),
                                       static_cast<DWORD>((quoted.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool StartupRegistration::Disable()
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
    {
        return true;
    }
    const auto result = RegDeleteValueW(key, kValueName);
    RegCloseKey(key);
    return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}
