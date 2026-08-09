#include "config_store.h"

#include <windows.h>
#include <shlobj.h>

#include <cerrno>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace
{
constexpr wchar_t kSection[] = L"Settings";
constexpr wchar_t kDelayKey[] = L"DelaySeconds";
constexpr wchar_t kStartupKey[] = L"StartupEnabled";
constexpr double kMinimumDelay = 0.1;
constexpr double kMaximumDelay = 3600.0;
}

std::wstring ConfigStore::Path()
{
    wchar_t localAppData[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, localAppData)))
    {
        return L"settings.ini";
    }
    return std::wstring(localAppData) + L"\\AutoMouseCursorHider\\settings.ini";
}

bool ConfigStore::TryParseDelay(const std::wstring& text, double& seconds)
{
    wchar_t* end = nullptr;
    errno = 0;
    const double parsed = wcstod(text.c_str(), &end);
    if (text.empty() || end == text.c_str() || *end != L'\0' || errno == ERANGE || !std::isfinite(parsed) ||
        parsed < kMinimumDelay || parsed > kMaximumDelay)
    {
        return false;
    }
    seconds = parsed;
    return true;
}

AppSettings ConfigStore::Load()
{
    AppSettings settings;
    const auto path = Path();
    wchar_t delayText[64]{};
    GetPrivateProfileStringW(kSection, kDelayKey, L"3.0", delayText, ARRAYSIZE(delayText), path.c_str());
    double delay = settings.delaySeconds;
    if (TryParseDelay(delayText, delay))
    {
        settings.delaySeconds = delay;
    }
    settings.startupEnabled = GetPrivateProfileIntW(kSection, kStartupKey, 0, path.c_str()) != 0;
    return settings;
}

bool ConfigStore::Save(const AppSettings& settings)
{
    const auto path = Path();
    const auto directory = std::filesystem::path(path).parent_path();
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
    {
        return false;
    }

    std::wostringstream delay;
    delay << std::fixed << std::setprecision(3) << settings.delaySeconds;
    return WritePrivateProfileStringW(kSection, kDelayKey, delay.str().c_str(), path.c_str()) &&
           WritePrivateProfileStringW(kSection, kStartupKey, settings.startupEnabled ? L"1" : L"0", path.c_str());
}

