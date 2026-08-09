#pragma once

#include <string>

enum class LanguageMode
{
    Auto,
    English,
    SimplifiedChinese
};

enum class Language
{
    English,
    SimplifiedChinese
};

enum class StringId
{
    Settings,
    Pause,
    Resume,
    Exit,
    WindowTitle,
    Explanation,
    Delay,
    Seconds,
    EnableAutoHide,
    StartWithWindows,
    Language,
    SystemDefault,
    SimplifiedChinese,
    English,
    Ok,
    Cancel,
    InvalidValue,
    InvalidValueText,
    SaveFailed,
    AppName
};

class Localization
{
public:
    static Language Resolve(LanguageMode mode);
    static LanguageMode ParseMode(const std::wstring& value);
    static const wchar_t* ModeValue(LanguageMode mode);
    static const wchar_t* Text(Language language, StringId id);
};

