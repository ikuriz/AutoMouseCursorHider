#include "../src/config_store.h"
#include "../src/delay_step.h"

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
    double value = 0.0;
    Require(ConfigStore::TryParseDelay(L"2.5", value) && value == 2.5, "accepts decimal delay");
    Require(!ConfigStore::TryParseDelay(L"0", value), "rejects zero");
    Require(!ConfigStore::TryParseDelay(L"3600.1", value), "rejects values over one hour");
    Require(!ConfigStore::TryParseDelay(L"NaN", value), "rejects NaN");
    Require(!ConfigStore::TryParseDelay(L"2 seconds", value), "rejects trailing text");
    Require(StepDelay(0.5, -1) == 0.1, "steps down from 0.5 to 0.1");
    Require(StepDelay(0.1, 1) == 0.5, "steps up from 0.1 to 0.5");
    Require(StepDelay(0.5, 1) == 1.0, "steps up from 0.5 to 1.0");
    Require(StepDelay(1.0, 1) == 1.5, "steps up by half-second ticks");
    Require(StepDelay(1.5, 1) == 2.0, "keeps regular half-second ticks");
    Require(StepDelay(0.6, 1) == 1.0, "snaps an arbitrary value to the next tick");
    Require(StepDelay(1.1, 1) == 1.5, "snaps 1.1 to 1.5");
    Require(StepDelay(0.6, -1) == 0.5, "snaps down to the previous regular tick");
    std::cout << "PASS native config store tests\n";
    return EXIT_SUCCESS;
}
