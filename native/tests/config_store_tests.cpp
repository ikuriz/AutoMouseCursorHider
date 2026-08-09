#include "../src/config_store.h"

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
    std::cout << "PASS native config store tests\n";
    return EXIT_SUCCESS;
}

