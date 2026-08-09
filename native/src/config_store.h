#pragma once

#include <string>

struct AppSettings
{
    double delaySeconds = 3.0;
    bool startupEnabled = false;
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

