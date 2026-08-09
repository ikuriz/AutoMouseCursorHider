#pragma once

#include <array>

#include <windows.h>

class CursorManager
{
public:
    CursorManager() = default;
    CursorManager(const CursorManager&) = delete;
    CursorManager& operator=(const CursorManager&) = delete;
    ~CursorManager();

    bool Hide();
    bool Restore();
    bool RestoreAndRefresh();
    bool RetryRestore();
    bool IsHidden() const;
    bool IsRestorePending() const;

private:
    static constexpr UINT kSpiSetCursors = 0x0057;
    static constexpr UINT kSpifSendChange = 0x0002;
    static constexpr std::array<UINT, 13> kSystemCursorIds{
        32512, 32513, 32514, 32515, 32516,
        32642, 32643, 32644, 32645, 32646,
        32648, 32649, 32650};

    bool _hidden = false;
    bool _restorePending = false;
};
