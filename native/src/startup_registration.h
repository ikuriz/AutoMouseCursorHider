#pragma once

#include <string>

class StartupRegistration
{
public:
    static bool IsEnabled();
    static bool Enable(const std::wstring& executablePath);
    static bool Disable();
};

