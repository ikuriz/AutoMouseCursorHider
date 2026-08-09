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
    void DrawButton(const DRAWITEMSTRUCT& draw);
    void RefreshTexts();

    HWND _window = nullptr;
    HWND _edit = nullptr;
    HWND _upDown = nullptr;
    HWND _enabled = nullptr;
    HWND _startup = nullptr;
    HWND _explanation = nullptr;
    HWND _label = nullptr;
    HWND _unit = nullptr;
    HWND _languageLabel = nullptr;
    HWND _languageCombo = nullptr;
    HWND _ok = nullptr;
    HWND _cancel = nullptr;
    HFONT _bodyFont = nullptr;
    AppSettings* _settings = nullptr;
    bool _autoHideChecked = true;
    bool _startupChecked = false;
    Language _language = Language::English;
    bool _accepted = false;
};
