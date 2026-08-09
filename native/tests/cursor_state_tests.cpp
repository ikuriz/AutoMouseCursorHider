#include "../src/cursor_state.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    CursorState state(3.0);
    state.OnActivity(1000);
    state.OnTimer(3999);
    Require(!state.ShouldHide(), "does not hide before threshold");
    state.OnTimer(4000);
    Require(state.ShouldHide(), "hides at threshold");

    state.OnActivity(4010);
    Require(!state.ShouldHide(), "activity restores visible state");

    state.Pause();
    state.OnTimer(100000);
    Require(state.Phase() == CursorPhase::Paused, "pause blocks hiding");
    state.Resume(200000);
    state.OnTimer(202999);
    Require(!state.ShouldHide(), "resume starts a fresh interval");
    state.OnTimer(203000);
    Require(state.ShouldHide(), "resume eventually hides");

    state.SetDelay(0.1);
    Require(state.DelayMs() == 100, "minimum delay is 100 milliseconds");
    state.SetDelay(3601.0);
    Require(state.DelayMs() == 3600000, "maximum delay is one hour");

    std::cout << "PASS native cursor state tests\n";
    return EXIT_SUCCESS;
}

