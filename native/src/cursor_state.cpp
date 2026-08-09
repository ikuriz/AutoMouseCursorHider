#include "cursor_state.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kMinimumDelaySeconds = 0.1;
constexpr double kMaximumDelaySeconds = 3600.0;

std::uint64_t DelayToMilliseconds(double seconds)
{
    const auto clamped = std::clamp(seconds, kMinimumDelaySeconds, kMaximumDelaySeconds);
    return static_cast<std::uint64_t>(std::llround(clamped * 1000.0));
}
}

CursorState::CursorState(double delaySeconds)
    : _delayMs(DelayToMilliseconds(delaySeconds)), _lastActivityMs(0), _phase(CursorPhase::Visible)
{
}

void CursorState::SetDelay(double delaySeconds)
{
    _delayMs = DelayToMilliseconds(delaySeconds);
    _lastActivityMs = 0;
    _phase = CursorPhase::Visible;
}

void CursorState::Pause()
{
    _phase = CursorPhase::Paused;
}

void CursorState::Resume(std::uint64_t nowMs)
{
    _lastActivityMs = nowMs;
    _phase = CursorPhase::Visible;
}

void CursorState::OnActivity(std::uint64_t nowMs)
{
    _lastActivityMs = nowMs;
    if (_phase != CursorPhase::Paused)
    {
        _phase = CursorPhase::Visible;
    }
}

void CursorState::OnTimer(std::uint64_t nowMs)
{
    if (_phase == CursorPhase::Paused || _phase == CursorPhase::Hidden || nowMs < _lastActivityMs)
    {
        return;
    }

    if (nowMs - _lastActivityMs >= _delayMs)
    {
        _phase = CursorPhase::Hidden;
    }
}

bool CursorState::ShouldHide() const
{
    return _phase == CursorPhase::Hidden;
}

CursorPhase CursorState::Phase() const
{
    return _phase;
}

std::uint64_t CursorState::LastActivityMs() const
{
    return _lastActivityMs;
}

std::uint64_t CursorState::DelayMs() const
{
    return _delayMs;
}

