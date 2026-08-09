#pragma once

#include <string>
#include "localization.h"

struct AppSettings
{
    bool enabled = true;
    double delaySeconds = 3.0;
    bool startupEnabled = false;
    LanguageMode language = LanguageMode::Auto;
};

class ConfigStore
{
public:
    static AppSettings Load();
    static bool Save(const AppSettings& settings);
    static bool TryParseDelay(const std::wstring& text, double& seconds);

private:
    static std::wstring Path();
};
