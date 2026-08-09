#include "cursor_manager.h"

#include <array>

namespace
{
constexpr int kCursorSize = 32;
constexpr std::size_t kMaskBytes = kCursorSize * kCursorSize / 8;
}

CursorManager::~CursorManager()
{
    Restore();
}

bool CursorManager::Hide()
{
    if (_hidden)
    {
        return true;
    }

    std::array<BYTE, kMaskBytes> andMask{};
    std::array<BYTE, kMaskBytes> xorMask{};
    andMask.fill(0xFF);
    xorMask.fill(0x00);

    _hidden = true;
    _restorePending = false;
    for (const auto id : kSystemCursorIds)
    {
        const HCURSOR cursor = CreateCursor(nullptr, 0, 0, kCursorSize, kCursorSize,
                                             andMask.data(), xorMask.data());
        if (cursor == nullptr || !SetSystemCursor(cursor, id))
        {
            if (cursor != nullptr)
            {
                DestroyCursor(cursor);
            }
            Restore();
            return false;
        }
    }

    return true;
}

bool CursorManager::Restore()
{
    if (!SystemParametersInfoW(kSpiSetCursors, 0, nullptr, kSpifSendChange))
    {
        _restorePending = true;
        return false;
    }

    _hidden = false;
    _restorePending = false;
    return true;
}

bool CursorManager::RestoreAndRefresh()
{
    if (!Restore())
    {
        return false;
    }

    POINT point{};
    if (GetCursorPos(&point))
    {
        SetCursorPos(point.x, point.y);
    }
    return true;
}

bool CursorManager::RetryRestore()
{
    return !_restorePending || RestoreAndRefresh();
}

bool CursorManager::IsHidden() const
{
    return _hidden;
}

bool CursorManager::IsRestorePending() const
{
    return _restorePending;
}

