#include "localization.h"

#include <windows.h>

#include <cwctype>

namespace
{
bool IsChineseLocale()
{
    wchar_t locale[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH) <= 0)
    {
        return false;
    }
    std::wstring name(locale);
    for (auto& character : name)
    {
        character = static_cast<wchar_t>(std::towlower(character));
    }

    // Only use the Simplified Chinese table when Windows explicitly reports a
    // Simplified script/region. Traditional Chinese currently falls back to English.
    return name == L"zh-cn" || name == L"zh-sg" || name == L"zh-my" ||
           name.find(L"-hans") != std::wstring::npos || name == L"zh-chs";
}
}

Language Localization::Resolve(LanguageMode mode)
{
    if (mode == LanguageMode::SimplifiedChinese || (mode == LanguageMode::Auto && IsChineseLocale()))
    {
        return Language::SimplifiedChinese;
    }
    return Language::English;
}

LanguageMode Localization::ParseMode(const std::wstring& value)
{
    if (value == L"zh-CN") return LanguageMode::SimplifiedChinese;
    if (value == L"en-US") return LanguageMode::English;
    return LanguageMode::Auto;
}

const wchar_t* Localization::ModeValue(LanguageMode mode)
{
    switch (mode)
    {
    case LanguageMode::English: return L"en-US";
    case LanguageMode::SimplifiedChinese: return L"zh-CN";
    default: return L"auto";
    }
}

const wchar_t* Localization::Text(Language language, StringId id)
{
    if (language == Language::SimplifiedChinese)
    {
        switch (id)
        {
        case StringId::Settings: return L"\u8bbe\u7f6e";
        case StringId::Pause: return L"\u6682\u505c";
        case StringId::Resume: return L"\u6062\u590d";
        case StringId::Exit: return L"\u9000\u51fa";
        case StringId::WindowTitle: return L"AutoMouseCursorHider - \u8bbe\u7f6e";
        case StringId::Explanation: return L"\u9f20\u6807\u9759\u6b62\u540e\u81ea\u52a8\u9690\u85cf\u3002";
        case StringId::Delay: return L"\u5ef6\u8fdf\u65f6\u95f4";
        case StringId::Seconds: return L"\u79d2";
        case StringId::EnableAutoHide: return L"\u542f\u7528\u81ea\u52a8\u9690\u85cf";
        case StringId::StartWithWindows: return L"\u5f00\u673a\u65f6\u542f\u52a8";
        case StringId::Language: return L"\u8bed\u8a00";
        case StringId::SystemDefault: return L"\u7cfb\u7edf\u9ed8\u8ba4 (Auto)";
        case StringId::SimplifiedChinese: return L"\u7b80\u4f53\u4e2d\u6587";
        case StringId::English: return L"English";
        case StringId::Ok: return L"\u786e\u5b9a";
        case StringId::Cancel: return L"\u53d6\u6d88";
        case StringId::InvalidValue: return L"\u8f93\u5165\u503c\u65e0\u6548";
        case StringId::InvalidValueText: return L"\u8bf7\u8f93\u5165 0.1 \u5230 3600 \u79d2\u4e4b\u95f4\u7684\u6570\u503c\u3002";
        case StringId::SaveFailed: return L"\u8bbe\u7f6e\u4fdd\u5b58\u5931\u8d25\u3002";
        case StringId::AppName: return L"AutoMouseCursorHider";
        }
    }

    switch (id)
    {
    case StringId::Settings: return L"Settings";
    case StringId::Pause: return L"Pause";
    case StringId::Resume: return L"Resume";
    case StringId::Exit: return L"Exit";
    case StringId::WindowTitle: return L"AutoMouseCursorHider - Settings";
    case StringId::Explanation: return L"The mouse cursor hides after inactivity.";
    case StringId::Delay: return L"Delay";
    case StringId::Seconds: return L"seconds";
    case StringId::EnableAutoHide: return L"Enable auto-hide";
    case StringId::StartWithWindows: return L"Start with Windows";
    case StringId::Language: return L"Language";
    case StringId::SystemDefault: return L"System default (Auto)";
    case StringId::SimplifiedChinese: return L"Simplified Chinese";
    case StringId::English: return L"English";
    case StringId::Ok: return L"OK";
    case StringId::Cancel: return L"Cancel";
    case StringId::InvalidValue: return L"Invalid value";
    case StringId::InvalidValueText: return L"Enter a value from 0.1 to 3600 seconds.";
    case StringId::SaveFailed: return L"Unable to save settings.";
    case StringId::AppName: return L"AutoMouseCursorHider";
    }
    return L"";
}
