#pragma once

#include "config_store.h"
#include "delay_step.h"

#include <windows.h>

class SettingsDialog
{
public:
    bool ShowModal(HWND owner, AppSettings& settings);
    static double StepDelayValue(double currentSeconds, int direction);

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static void UpdateValue(HWND edit, double delta);
    static bool ReadValue(HWND edit, double& value);

    HWND _window = nullptr;
    HWND _edit = nullptr;
    HWND _upDown = nullptr;
    HWND _startup = nullptr;
    AppSettings* _settings = nullptr;
    bool _accepted = false;
};
