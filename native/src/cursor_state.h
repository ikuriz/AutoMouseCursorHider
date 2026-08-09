#pragma once

#include <cstdint>

enum class CursorPhase
{
    Visible,
    Hidden,
    Paused
};

class CursorState
{
public:
    explicit CursorState(double delaySeconds = 3.0);

    void SetDelay(double delaySeconds);
    void Pause();
    void Resume(std::uint64_t nowMs);
    void OnActivity(std::uint64_t nowMs);
    void OnTimer(std::uint64_t nowMs);

    bool ShouldHide() const;
    CursorPhase Phase() const;
    std::uint64_t LastActivityMs() const;
    std::uint64_t DelayMs() const;

private:
    std::uint64_t _delayMs;
    std::uint64_t _lastActivityMs;
    CursorPhase _phase;
};

